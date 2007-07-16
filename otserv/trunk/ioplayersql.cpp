//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Player Loader/Saver based on MySQL
//////////////////////////////////////////////////////////////////////
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//////////////////////////////////////////////////////////////////////
#include "otpch.h"

#include "ioplayer.h"
#include "ioplayersql.h"
#include "ioaccount.h"
#include "item.h"
#include "town.h"
#include "configmanager.h"
#include "tools.h"
#include "definitions.h"

#include <boost/tokenizer.hpp>
#include <iostream>
#include <iomanip>

extern ConfigManager g_config;

typedef boost::tokenizer<boost::char_separator<char> > tokenizer;

#ifndef __GNUC__
#pragma warning( disable : 4005)
#pragma warning( disable : 4996)
#endif

IOPlayerSQL::IOPlayerSQL()
{
	//
}

bool IOPlayerSQL::loadPlayer(Player* player, std::string name)
{
	Database* mysql = Database::instance();
	DBQuery query;
	DBResult result;

	query << "SELECT * FROM players WHERE name='" << Database::escapeString(name) << "'";	
	if(!mysql->connect() ||!mysql->storeQuery(query, result) || result.getNumRows() != 1)
		return false;

	uint32_t accno = result.getDataInt("account_id");
	if(accno < 1)
		return false;

	// Getting all player properties
	player->setGUID(result.getDataInt("id"));

	player->accountNumber = accno;
	player->setSex((playersex_t)result.getDataInt("sex"));
	
	const PlayerGroup* group = getPlayerGroup(result.getDataInt("group_id"));
	if(group){
		player->accessLevel = group->m_access;
		player->maxDepotLimit = group->m_maxDepotItems;
		player->maxVipLimit = group->m_maxVip;
		player->setFlags(group->m_flags);
	}

	player->setDirection((Direction)result.getDataInt("direction"));
	player->experience = result.getDataInt("experience");
	player->level = result.getDataInt("level");
	player->soul = result.getDataInt("soul");
	player->capacity = result.getDataInt("cap");
	player->lastLoginSaved = result.getDataInt("lastlogin");
	player->setVocation(result.getDataInt("vocation"));
	player->updateBaseSpeed();
	
	player->mana = result.getDataInt("mana");
	player->manaMax = result.getDataInt("manamax");
	player->manaSpent = result.getDataInt("manaspent");
	player->magLevel = result.getDataInt("maglevel");

	player->health = result.getDataInt("health");
	if(player->health <= 0)
		player->health = 100;

	player->healthMax = result.getDataInt("healthmax");
	if(player->healthMax <= 0)
		player->healthMax = 100;

	player->defaultOutfit.lookType = result.getDataInt("looktype");
	player->defaultOutfit.lookHead = result.getDataInt("lookhead");
	player->defaultOutfit.lookBody = result.getDataInt("lookbody");
	player->defaultOutfit.lookLegs = result.getDataInt("looklegs");
	player->defaultOutfit.lookFeet = result.getDataInt("lookfeet");
	player->defaultOutfit.lookAddons = result.getDataInt("lookaddons");
	player->currentOutfit = player->defaultOutfit;

	#ifdef __SKULLSYSTEM__
	int32_t redSkullSeconds = result.getDataInt("redskulltime") - std::time(NULL);
	if(redSkullSeconds > 0){
		//ensure that we round up the number of ticks
		player->redSkullTicks = (redSkullSeconds + 2)*1000;
		if(result.getDataInt("redskull") == 1){
			player->skull = SKULL_RED;
		}
	}
	#endif

	unsigned long conditionsSize = 0;
	const char* conditions = result.getDataBlob("conditions", conditionsSize);
	PropStream propStream;
	propStream.init(conditions, conditionsSize);

	Condition* condition;
	while(condition = Condition::createCondition(propStream)){
		if(condition->unserialize(propStream)){
			player->storedConditionList.push_back(condition);
		}
		else{
			delete condition;
		}
	}

	player->lossPercent[LOSS_EXPERIENCE] = result.getDataInt("loss_experience");
	player->lossPercent[LOSS_MANASPENT] = result.getDataInt("loss_mana");
	player->lossPercent[LOSS_SKILLTRIES] = result.getDataInt("loss_skills");

	player->loginPosition.x = result.getDataInt("posx");
	player->loginPosition.y = result.getDataInt("posy");
	player->loginPosition.z = result.getDataInt("posz");

	player->town = result.getDataInt("town_id");
	Town* town = Towns::getInstance().getTown(player->town);
	if(town){
		player->masterPos = town->getTemplePosition();
	}
	//if posx == 0 AND posy == 0 AND posz == 0
	// login position is temple position
	Position loginPos = player->loginPosition;
	if(loginPos.x == 0 && loginPos.y == 0 && loginPos.z == 0){
		player->loginPosition = player->masterPos;
	}

	uint32_t rankid = result.getDataInt("rank_id");
	if(rankid){
		player->guildNick = result.getDataString("guildnick");	
		
		query << "SELECT guild_ranks.name as rank, guild_ranks.guild_id as guildid, guild_ranks.level as level, guilds.name as guildname FROM guild_ranks,guilds WHERE guild_ranks.id = '" << rankid << "' AND guild_ranks.guild_id = guilds.id";
		if(mysql->storeQuery(query, result)){
			player->guildName = result.getDataString("guildname");
			player->guildLevel = result.getDataInt("level");
			player->guildId = result.getDataInt("guildid");
			player->guildRank = result.getDataString("rank");
		}
		
	}

	//get password
	query << "SELECT password FROM accounts WHERE id='" << accno << "'";
	if(!mysql->storeQuery(query, result) || result.getNumRows() != 1)
		return false;

	player->password = result.getDataString("password");

	// we need to find out our skills
	// so we query the skill table
	query << "SELECT skillid,value,count FROM player_skills WHERE player_id='" << player->getGUID() << "'";
	if(mysql->storeQuery(query, result)){
		//now iterate over the skills
		for(uint32_t i = 0; i < result.getNumRows(); ++i){
			int skillid = result.getDataInt("skillid",i);
			if(skillid >= SKILL_FIRST && skillid <= SKILL_LAST){
				player->skills[skillid][SKILL_LEVEL] = result.getDataInt("value",i);
				player->skills[skillid][SKILL_TRIES] = result.getDataInt("count",i);
			}
		}
	}
	else{
		query.reset();
	}

	query << "SELECT player_id,name FROM player_spells WHERE player_id='" << player->getGUID() << "'";
	if(mysql->storeQuery(query, result)){
		for(uint32_t i = 0; i < result.getNumRows(); ++i){
			std::string spellName = result.getDataString("name",i);
			player->learnedInstantSpellList.push_back(spellName);
		}
	}
	else{
		query.reset();
	}

	//load inventory items
	ItemMap itemMap;

	query << "SELECT pid,sid,itemtype,count,attributes FROM player_items WHERE player_id='" << player->getGUID() <<"' ORDER BY sid DESC";
	if(mysql->storeQuery(query, result) && (result.getNumRows() > 0)){
		loadItems(itemMap, result);

		ItemMap::reverse_iterator it;
		ItemMap::iterator it2;
	
		for(it = itemMap.rbegin(); it != itemMap.rend(); ++it){
			Item* item = it->second.first;
			int pid = it->second.second;
			if(pid >= 1 && pid <= 10){
				player->__internalAddThing(pid, item);
			}
			else{
				it2 = itemMap.find(pid);
				if(it2 != itemMap.end()){
					if(Container* container = it2->second.first->getContainer()){
						container->__internalAddThing(item);
					}
				}
			}
		}
	}
	else{
		query.reset();
	}

	player->updateInventoryWeigth();
	player->updateItemsLight(true);
	player->setSkillsPercents();

	//load depot items
	itemMap.clear();
	
	query << "SELECT pid,sid,itemtype,count,attributes FROM player_depotitems WHERE player_id='" << player->getGUID() << "' ORDER BY sid DESC";
	if(mysql->storeQuery(query, result) && (result.getNumRows() > 0)){
		loadItems(itemMap, result);

		ItemMap::reverse_iterator it;
		ItemMap::iterator it2;
	
		for(it = itemMap.rbegin(); it != itemMap.rend(); ++it){
			Item* item = it->second.first;
			int pid = it->second.second;
			if(pid >= 0 && pid < 100){
				if(Container* c = item->getContainer()){
					if(Depot* depot = c->getDepot()){
						player->addDepot(depot, pid);
					}
					else{
							std::cout << "Error loading depot "<< pid << " for player " << 
								player->getGUID() << std::endl;
					}
				}
				else{
					std::cout << "Error loading depot "<< pid << " for player " << 
						player->getGUID() << std::endl;
				}
			}
			else{
				it2 = itemMap.find(pid);
				if(it2 != itemMap.end()){
					if(Container* container = it2->second.first->getContainer()){
						container->__internalAddThing(item);
					}
				}
			}
		}
	}
	else{
		query.reset();
	}
	
	//load storage map
	query << "SELECT `key`,`value` FROM player_storage WHERE player_id='" << player->getGUID() << "'";
	if(mysql->storeQuery(query,result)){
		for(uint32_t i=0; i < result.getNumRows(); ++i){
			uint32_t key = result.getDataInt("key",i);
			int32_t value = result.getDataInt("value",i);
			player->addStorageValue(key,value);
		}
	}
	else{
		query.reset();
	}

	//load vip
	query << "SELECT vip_id FROM player_viplist WHERE player_id='" << player->getGUID() << "'";		
	if(mysql->storeQuery(query,result)){
		for(uint32_t i = 0; i < result.getNumRows(); ++i){
			uint32_t vip_id = result.getDataInt("vip_id",i);
			std::string dummy_str;
			if(storeNameByGuid(*mysql, vip_id)){
				player->addVIP(vip_id, dummy_str, false, true);
			}
		}
	}
	else{
		query.reset();
	}

	return true;
}

bool IOPlayerSQL::saveItems(Player* player, const ItemBlockList& itemList, DBSplitInsert& query_insert)
{
	std::list<Container*> listContainer;

	typedef std::pair<Container*, int32_t> containerBlock;
	std::list<containerBlock> stack;

	int32_t parentId = 0;
	std::stringstream ss;

	int32_t runningId = 100;

	Item* item;
	int32_t pid;

	for(ItemBlockList::const_iterator it = itemList.begin(); it != itemList.end(); ++it){
		pid = it->first;
		item = it->second;
		++runningId;
		
		uint32_t attributesSize;

		PropWriteStream propWriteStream;
		item->serializeAttr(propWriteStream);
		const char* attributes = propWriteStream.getStream(attributesSize);

		ss << "(" << player->getGUID() << ","
			<< pid << ","
			<< runningId << ","
			<< item->getID() << ","
			<< (int32_t)item->getItemCountOrSubtype() << ",'"
			<< Database::escapeString(attributes, attributesSize) <<"')";
		
		if(!query_insert.addRow(ss.str())){
			return false;
		}
		
		ss.str("");
	      
		if(Container* container = item->getContainer()){
			stack.push_back(containerBlock(container, runningId));
		}
	}

	while(stack.size() > 0){		
		const containerBlock& cb = stack.front();
		Container* container = cb.first;
		parentId = cb.second;
		stack.pop_front();

		for(uint32_t i = 0; i < container->size(); ++i){
			++runningId;
			item = container->getItem(i);
			Container* container = item->getContainer();
			if(container){
				stack.push_back(containerBlock(container, runningId));
			}
			
			uint32_t attributesSize;

			PropWriteStream propWriteStream;
			item->serializeAttr(propWriteStream);
			const char* attributes = propWriteStream.getStream(attributesSize);

			ss << "(" << player->getGUID() <<","
				<< parentId << ","
				<< runningId <<","
				<< item->getID() << ","
				<< (int32_t)item->getItemCountOrSubtype() << ",'"
				<< Database::escapeString(attributes, attributesSize) <<"')";
			
			if(!query_insert.addRow(ss.str())){
				return false;
			}
			
			ss.str("");	
		}
	}

	if(!query_insert.executeQuery()){
		return false;
	}

	return true;
}

bool IOPlayerSQL::savePlayer(Player* player)
{
	player->preSave();

	Database* mysql = Database::instance();
	DBQuery query;
	DBResult result;
	
	if(!mysql->connect()){
		return false;
	}

	//check if the player have to be saved or not
	query << "SELECT save FROM players WHERE id='" << player->getGUID() << "'";
	if(!mysql->storeQuery(query,result) || (result.getNumRows() != 1) )
		return false;

	if(result.getDataInt("save") != 1) // If save var is not 1 don't save the player info
		return true;

	DBTransaction trans(mysql);
	if(!trans.start())
		return false;

	//serialize conditions
	PropWriteStream propWriteStream;

	for(ConditionList::const_iterator it = player->conditions.begin(); it != player->conditions.end(); ++it){
		if((*it)->isPersistent()){
			if(!(*it)->serialize(propWriteStream)){
				return false;
			}

			propWriteStream.ADD_UCHAR(CONDITIONATTR_END);
		}
	}

	uint32_t conditionsSize;
	const char* conditions = propWriteStream.getStream(conditionsSize);

	//First, an UPDATE query to write the player itself
	query << "UPDATE `players` SET ";
	query << "`level` = " << player->level << ", ";
	query << "`vocation` = " << (int)player->getVocationId() << ", ";
	query << "`health` = " << player->health << ", ";
	query << "`healthmax` = " << player->healthMax << ", ";
	query << "`direction` = " << (int)player->getDirection() << ", ";
	query << "`experience` = " << player->experience << ", ";
	query << "`lookbody` = " << (int)player->defaultOutfit.lookBody << ", ";
	query << "`lookfeet` = " << (int)player->defaultOutfit.lookFeet << ", ";
	query << "`lookhead` = " << (int)player->defaultOutfit.lookHead << ", ";
	query << "`looklegs` = " << (int)player->defaultOutfit.lookLegs << ", ";
	query << "`looktype` = " << (int)player->defaultOutfit.lookType << ", ";
	query << "`lookaddons` = " << (int)player->defaultOutfit.lookAddons << ", ";
	query << "`magLevel` = " << player->magLevel << ", ";
	query << "`mana` = " << player->mana << ", ";
	query << "`manamax` = " << player->manaMax << ", ";
	query << "`manaspent` = " << player->manaSpent << ", ";
	query << "`soul` = " << player->soul << ", ";
	query << "`town_id` = '" << player->town << "', ";
	query << "`posx` = '" << player->getLoginPosition().x << "', ";
	query << "`posy` = '" << player->getLoginPosition().y << "', ";
	query << "`posz` = '" << player->getLoginPosition().z << "', ";
	query << "`cap` = " << player->getCapacity() << ", ";
	query << "`sex` = " << player->sex << ", ";
	query << "`lastlogin` = " << player->lastlogin << ", ";
	query << "`lastip` = " << player->lastip << ", ";
	query << "`conditions` = '" << Database::escapeString(conditions, conditionsSize) << "', ";
	query << "`loss_experience` = " << (int)player->getLossPercent(LOSS_EXPERIENCE) << ", ";
	query << "`loss_mana` = " << (int)player->getLossPercent(LOSS_MANASPENT) << ", ";
	query << "`loss_skills` = " << (int)player->getLossPercent(LOSS_SKILLTRIES) << " ";

#ifdef __SKULLSYSTEM__
	int32_t redSkullTime = 0;
	if(player->redSkullTicks > 0){
		redSkullTime = std::time(NULL) + player->redSkullTicks/1000;
	}

	query << ", `redskulltime` = " << redSkullTime << ", ";
	int32_t redSkull = 0;
	if(player->skull == SKULL_RED){
		redSkull = 1;
	}

	query << "`redskull` = " << redSkull << " ";
#endif

	query << " WHERE `id` = "<< player->getGUID()
	#ifndef __USE_SQLITE__
	<<" LIMIT 1";
    #else
    ;
    #endif

	if(!mysql->executeQuery(query))
		return false;

	//skills
	for(int i = 0; i <= 6; i++){
		query << "UPDATE player_skills SET value = " << player->skills[i][SKILL_LEVEL] <<", count = "<< player->skills[i][SKILL_TRIES] << " WHERE player_id = " << player->getGUID() << " AND  skillid = " << i;

		if(!mysql->executeQuery(query))
			return false;
	}

	//learned spells
	query << "DELETE FROM player_spells WHERE player_id='"<< player->getGUID() << "'";

	if(!mysql->executeQuery(query)){
		return false;
	}

	std::stringstream ss;
	DBSplitInsert query_insert(mysql);
	query_insert.setQuery("INSERT INTO `player_spells` (`player_id` , `name` ) VALUES ");
	for(LearnedInstantSpellList::const_iterator it = player->learnedInstantSpellList.begin();
			it != player->learnedInstantSpellList.end(); ++it){
		ss << "(" << player->getGUID() <<",'"<< Database::escapeString(*it)<<"')";
		
		if(!query_insert.addRow(ss.str()))
			return false;
		
		ss.str("");
	}

	if(!query_insert.executeQuery()){
		return false;
	}

	//item saving
	query << "DELETE FROM player_items WHERE player_id='"<< player->getGUID() << "'";

	if(!mysql->executeQuery(query)){
		return false;
	}

	query_insert.setQuery("INSERT INTO `player_items` (`player_id` , `pid` , `sid` , `itemtype` , `count` , `attributes` ) VALUES ");
	
	ItemBlockList itemList;

	Item* item;
	for(int32_t slotId = 1; slotId <= 10; ++slotId){
		if(item = player->inventory[slotId]){
			itemList.push_back(itemBlock(slotId, item));
		}
	}

	if(!saveItems(player, itemList, query_insert)){
		return false;
	}

	//save depot items
	query << "DELETE FROM player_depotitems WHERE player_id='"<< player->getGUID() << "'";

	if(!mysql->executeQuery(query)){
		return false;
	}
	
	query_insert.setQuery("INSERT INTO `player_depotitems` (`player_id` , `pid` , `sid` , `itemtype` , `count` , `attributes` ) VALUES ");
	
	itemList.clear();
	for(DepotMap::iterator it = player->depots.begin(); it != player->depots.end(); ++it){
		itemList.push_back(itemBlock(it->first, it->second));
	}
	
	if(!saveItems(player, itemList, query_insert)){
		return false;
	}

	query.reset();
	query << "DELETE FROM player_storage WHERE player_id='"<< player->getGUID() << "'";

	if(!mysql->executeQuery(query)){
		return false;
	}

	ss.str("");
	query_insert.setQuery("INSERT INTO `player_storage` (`player_id` , `key` , `value` ) VALUES ");
	player->genReservedStorageRange();
	for(StorageMap::const_iterator cit = player->getStorageIteratorBegin(); cit != player->getStorageIteratorEnd();cit++){
		ss << "(" << player->getGUID() <<","<< cit->first <<","<< cit->second<<")";
		
		if(!query_insert.addRow(ss.str()))
			return false;
		
		ss.str("");
	}

	if(!query_insert.executeQuery()){
		return false;
	}

	//save vip list
	query.reset();
	query << "DELETE FROM `player_viplist` WHERE player_id='"<< player->getGUID() << "'";

	if(!mysql->executeQuery(query)){
		return false;
	}

	ss.str("");
	query_insert.setQuery("INSERT INTO `player_viplist` (`player_id` , `vip_id` ) VALUES ");
	for(VIPListSet::iterator it = player->VIPList.begin(); it != player->VIPList.end(); it++){
		ss << "(" << player->getGUID() <<","<< *it <<")";
		
		if(!query_insert.addRow(ss.str())){
			return false;
		}
		
		ss.str("");
	}

	if(!query_insert.executeQuery()){
		return false;
	}
    
	//End the transaction
	return trans.success();
}

bool IOPlayerSQL::storeNameByGuid(Database &mysql, uint32_t guid)
{
	DBQuery query;
	DBResult result;

	NameCacheMap::iterator it = nameCacheMap.find(guid);
	if(it != nameCacheMap.end())
		return true;

	query << "SELECT name FROM players WHERE id='" << guid << "'";

	if(!mysql.storeQuery(query, result) || result.getNumRows() != 1)
		return false;

	nameCacheMap[guid] = result.getDataString("name");
	return true;
}

bool IOPlayerSQL::getNameByGuid(uint32_t guid, std::string& name)
{
	NameCacheMap::iterator it = nameCacheMap.find(guid);
	if(it != nameCacheMap.end()){
		name = it->second;
		return true;
	}

	Database* mysql = Database::instance();
	DBQuery query;
	DBResult result;
	
	if(!mysql->connect()){
		return false;
	}

	query << "SELECT name FROM players WHERE id='" << guid << "'";
	if(!mysql->storeQuery(query, result) || result.getNumRows() != 1)
		return false;

	name = result.getDataString("name");
	nameCacheMap[guid] = name;

	return true;
}

bool IOPlayerSQL::getGuidByName(uint32_t &guid, std::string& name)
{
	GuidCacheMap::iterator it = guidCacheMap.find(name);
	if(it != guidCacheMap.end()){
		name = it->first;
		guid = it->second;
		return true;
	}

	Database* mysql = Database::instance();
	DBQuery query;
	DBResult result;
	
	if(!mysql->connect()){
		return false;
	}

	query << "SELECT name,id FROM players WHERE name='" << Database::escapeString(name) << "'";
	if(!mysql->storeQuery(query, result) || result.getNumRows() != 1)
		return false;

	name = result.getDataString("name");
	guid = result.getDataInt("id");

	guidCacheMap[name] = guid;
	return true;
}


bool IOPlayerSQL::getGuidByNameEx(uint32_t &guid, bool &specialVip, std::string& name)
{
	Database* mysql = Database::instance();
	DBQuery query;
	DBResult result;
	
	if(!mysql->connect()){
		return false;
	}

	query << "SELECT name,id,group_id FROM players WHERE name='" << Database::escapeString(name) << "'";
	if(!mysql->storeQuery(query, result) || result.getNumRows() != 1)
		return false;

	name = result.getDataString("name");
	guid = result.getDataInt("id");
	const PlayerGroup* group = getPlayerGroup(result.getDataInt("group_id"));
	if(group){
		specialVip = (0 != (group->m_flags & ((uint64_t)1 << PlayerFlag_SpecialVIP)));
	}
	else{
		specialVip = false;
	}
	return true;
}

bool IOPlayerSQL::getGuildIdByName(uint32_t &guildId, const std::string& guildName)
{
	Database* mysql = Database::instance();
	DBQuery query;
	DBResult result;
	
	if(!mysql->connect()){
		return false;
	}

	query << "SELECT id FROM guilds WHERE name='" << Database::escapeString(guildName) << "'";
	if(!mysql->storeQuery(query, result) || result.getNumRows() != 1)
		return false;

	guildId = result.getDataInt("id");
	return true;
}

bool IOPlayerSQL::playerExists(std::string name)
{
	Database* mysql = Database::instance();
	DBQuery query;
	DBResult result;
	
	if(!mysql->connect()){
		return false;
	}

	query << "SELECT name FROM players WHERE name='" << Database::escapeString(name) << "'";
	if(!mysql->storeQuery(query, result) || result.getNumRows() != 1)
		return false;

	return true;
}

const PlayerGroup* IOPlayerSQL::getPlayerGroup(uint32_t groupid)
{
	PlayerGroupMap::const_iterator it = playerGroupMap.find(groupid);
	
	if(it != playerGroupMap.end()){
		return it->second;
	}
	else{
		Database* mysql = Database::instance();
		DBQuery query;
		DBResult result;
	
		query << "SELECT * FROM groups WHERE id='" << groupid << "'";
		if(mysql->connect() && mysql->storeQuery(query, result) && (result.getNumRows() == 1)){
			PlayerGroup* group = new PlayerGroup;
			
			group->m_name = result.getDataString("name");
			group->m_flags = result.getDataLong("flags");
			group->m_access = result.getDataInt("access");
			group->m_maxDepotItems = result.getDataInt("maxdepotitems");
			group->m_maxVip = result.getDataInt("maxviplist");
			
			playerGroupMap[groupid] = group;
			return group;
		}
	}
	return NULL;
}

void IOPlayerSQL::loadItems(ItemMap& itemMap, DBResult& result)
{
	for(uint32_t i = 0; i < result.getNumRows(); ++i){	
		int sid = result.getDataInt("sid", i);
		int pid = result.getDataInt("pid", i);
		int type = result.getDataInt("itemtype", i);
		int count = result.getDataInt("count", i);
			
		unsigned long attrSize = 0;
		const char* attr = result.getDataBlob("attributes", attrSize, i);
			
		PropStream propStream;
		propStream.init(attr, attrSize);
			
		Item* item = Item::CreateItem(type, count);
		if(item){
			if(!item->unserializeAttr(propStream)){
				std::cout << "WARNING: Serialize error in IOPlayerSQL::loadItems" << std::endl;
			}
				
			std::pair<Item*, int> pair(item, pid);
			itemMap[sid] = pair;
		}
	}
}

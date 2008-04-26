//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Base class for the Account Loader/Saver
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

#include "definitions.h"
#include "ioplayer.h"
#include "ioaccount.h"
#include "item.h"
#include "town.h"
#include "configmanager.h"
#include "tools.h"
#include "definitions.h"

#include <iostream>
#include <iomanip>

extern ConfigManager g_config;

#ifndef __GNUC__
#pragma warning( disable : 4005)
#pragma warning( disable : 4996)
#endif

bool IOPlayer::loadPlayer(Player* player, const std::string& name, bool preload /*= false*/)
{
	Database* db = Database::instance();
	DBQuery query;
	DBResult* result;

	if(!(result = db->storeQuery("SELECT * FROM `players` WHERE `name` = " + db->escapeString(name)))){
	  	return false;
	}

	player->setGUID(result->getDataInt("id"));
	player->accountNumber = result->getDataInt("account_id");

	const PlayerGroup* group = getPlayerGroup(result->getDataInt("group_id"));
	if(group){
		player->accessLevel = group->m_access;
		player->maxDepotLimit = group->m_maxDepotItems;
		player->maxVipLimit = group->m_maxVip;
		player->setFlags(group->m_flags);
	}

	if(preload){
		//only loading basic info
		db->freeResult(result);
		return true;
	}

	// Getting all player properties
	player->setSex((playersex_t)result->getDataInt("sex"));
	player->setDirection((Direction)result->getDataInt("direction"));
	player->level = std::max((uint32_t)1, (uint32_t)result->getDataInt("level"));

	uint32_t currExpCount = Player::getExpForLevel(player->level);
	uint32_t nextExpCount = Player::getExpForLevel(player->level + 1);
	uint32_t experience = (uint32_t)result->getDataInt("experience");
	if(experience < currExpCount || experience  > nextExpCount){
		experience = currExpCount;
	}
	player->experience = experience;
	player->levelPercent = Player::getPercentLevel(player->experience - currExpCount, nextExpCount - currExpCount);
	player->soul = result->getDataInt("soul");
	player->capacity = result->getDataInt("cap");
	player->lastLoginSaved = result->getDataInt("lastlogin");
	player->setVocation(result->getDataInt("vocation"));
	player->mana = result->getDataInt("mana");
	player->manaMax = result->getDataInt("manamax");
	player->magLevel = result->getDataInt("maglevel");

	uint32_t nextManaCount = (uint32_t)player->vocation->getReqMana(player->magLevel + 1);
	uint32_t manaSpent = (uint32_t)result->getDataInt("manaspent");
	if(manaSpent > nextManaCount){
		//make sure its not out of bound
		manaSpent = 0;
	}
	player->manaSpent = manaSpent;
	player->magLevelPercent = Player::getPercentLevel(player->manaSpent, nextManaCount);
	player->health = result->getDataInt("health");
	player->healthMax = result->getDataInt("healthmax");
	player->defaultOutfit.lookType = result->getDataInt("looktype");
	player->defaultOutfit.lookHead = result->getDataInt("lookhead");
	player->defaultOutfit.lookBody = result->getDataInt("lookbody");
	player->defaultOutfit.lookLegs = result->getDataInt("looklegs");
	player->defaultOutfit.lookFeet = result->getDataInt("lookfeet");
	player->defaultOutfit.lookAddons = result->getDataInt("lookaddons");
	player->currentOutfit = player->defaultOutfit;

#ifdef __USE_SQL_PREMDAYS__
	player->premiumDays = result->getDataInt("premdays");
#endif

#ifdef __SKULLSYSTEM__
	int32_t redSkullSeconds = result->getDataInt("redskulltime") - std::time(NULL);
	if(redSkullSeconds > 0){
		//ensure that we round up the number of ticks
		player->redSkullTicks = (redSkullSeconds + 2)*1000;

		if(result->getDataInt("redskull") == 1){
			player->skull = SKULL_RED;
		}
	}
#endif

	unsigned long conditionsSize = 0;
	const char* conditions = result->getDataStream("conditions", conditionsSize);
	PropStream propStream;
	propStream.init(conditions, conditionsSize);

	Condition* condition;
	while((condition = Condition::createCondition(propStream))){
		if(condition->unserialize(propStream)){
			player->storedConditionList.push_back(condition);
		}
		else{
			delete condition;
		}
	}

	player->setLossPercent(LOSS_EXPERIENCE, result->getDataInt("loss_experience"));
	player->setLossPercent(LOSS_MANASPENT, result->getDataInt("loss_mana"));
	player->setLossPercent(LOSS_SKILLTRIES, result->getDataInt("loss_skills"));

	player->loginPosition.x = result->getDataInt("posx");
	player->loginPosition.y = result->getDataInt("posy");
	player->loginPosition.z = result->getDataInt("posz");

	player->town = result->getDataInt("town_id");
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

	uint32_t rankid = result->getDataInt("rank_id");

	// place it here and now we can drop all additional query instances as all data were loaded
	#ifndef __USE_SQL_PREMDAYS__
	time_t premEnd = result->getDataInt("premend");
	time_t timeNow = time(NULL);
	if(premEnd > 0){
		if(premEnd < timeNow){
			//update table
			query << "UPDATE `players` SET `premend` = 0 WHERE `id` = " << player->getGUID();
			db->executeQuery(query.str());
			query.str("");
		}
		else{
			player->premiumDays = (premEnd - timeNow)/86400;
		}
	}
	#endif

	player->balance = result->getDataInt("balance");

	player->guildNick = result->getDataString("guildnick");
	db->freeResult(result);

	if(rankid){
		query << "SELECT `guild_ranks`.`name` as `rank`, `guild_ranks`.`guild_id` as `guildid`, `guild_ranks`.`level` as `level`, `guilds`.`name` as `guildname` FROM `guild_ranks`, `guilds` WHERE `guild_ranks`.`id` = " << rankid << " AND `guild_ranks`.`guild_id` = `guilds`.`id`";
		if((result = db->storeQuery(query.str()))){
			player->guildName = result->getDataString("guildname");
			player->guildLevel = result->getDataInt("level");
			player->guildId = result->getDataInt("guildid");
			player->guildRank = result->getDataString("rank");

			db->freeResult(result);
		}
		query.str("");
	}

	//get password
	query << "SELECT `password` FROM `accounts` WHERE `id` = " << player->accountNumber;
	if(!(result = db->storeQuery(query.str()))){
		return false;
	}

	player->password = result->getDataString("password");
	db->freeResult(result);

	// we need to find out our skills
	// so we query the skill table
	query.str("");
	query << "SELECT `skillid`, `value`, `count` FROM `player_skills` WHERE `player_id` = " << player->getGUID();
	if((result = db->storeQuery(query.str()))){
		//now iterate over the skills
		do{
			int skillid = result->getDataInt("skillid");
			if(skillid >= SKILL_FIRST && skillid <= SKILL_LAST){
				uint32_t skillLevel = result->getDataInt("value");
				uint32_t skillCount = result->getDataInt("count");

				uint32_t nextSkillCount = player->vocation->getReqSkillTries(skillid, skillLevel + 1);
				if(skillCount > nextSkillCount){
					//make sure its not out of bound
					skillCount = 0;
				}

				player->skills[skillid][SKILL_LEVEL] = skillLevel;
				player->skills[skillid][SKILL_TRIES] = skillCount;
				player->skills[skillid][SKILL_PERCENT] = Player::getPercentLevel(skillCount, nextSkillCount);
			}
		}while(result->next());

		db->freeResult(result);
	}

	query.str("");
	query << "SELECT `name` FROM `player_spells` WHERE `player_id` = " << player->getGUID();
	if((result = db->storeQuery(query.str()))){
		do{
			std::string spellName = result->getDataString("name");
			player->learnedInstantSpellList.push_back(spellName);
		}while(result->next());

		db->freeResult(result);
	}

	//load inventory items
	ItemMap itemMap;

	query.str("");
	query << "SELECT `pid`, `sid`, `itemtype`, `count`, `attributes` FROM `player_items` WHERE `player_id` = " << player->getGUID() << " ORDER BY `sid` DESC";
	if((result = db->storeQuery(query.str()))){
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
				if(it2 != itemMap.end())
					if(Container* container = it2->second.first->getContainer()){
						container->__internalAddThing(item);
					}
			}
		}

		db->freeResult(result);
	}


	//load depot items
	itemMap.clear();

	query.str("");
	query << "SELECT `pid`, `sid`, `itemtype`, `count`, `attributes` FROM `player_depotitems` WHERE `player_id` = " << player->getGUID() << " ORDER BY `sid` DESC";
	if((result = db->storeQuery(query.str()))){
		loadItems(itemMap, result);

		ItemMap::reverse_iterator it;
		ItemMap::iterator it2;

		for(it = itemMap.rbegin(); it != itemMap.rend(); ++it){
			Item* item = it->second.first;
			int pid = it->second.second;
			if(pid >= 0 && pid < 100){
				if(Container* c = item->getContainer()){
					if(Depot* depot = c->getDepot())
						player->addDepot(depot, pid);
					else
						std::cout << "Error loading depot "<< pid << " for player " << player->getGUID() << std::endl;
				}
				else{
					std::cout << "Error loading depot "<< pid << " for player " <<
						player->getGUID() << std::endl;
				}
			}
			else{
				it2 = itemMap.find(pid);
				if(it2 != itemMap.end())
					if(Container* container = it2->second.first->getContainer()){
						container->__internalAddThing(item);
					}
			}
		}

		db->freeResult(result);
	}

	//load storage map
	query.str("");
	query << "SELECT `key`, `value` FROM `player_storage` WHERE `player_id` = " << player->getGUID();
	if((result = db->storeQuery(query.str()))){
		do{
			uint32_t key = result->getDataInt("key");
			int32_t value = result->getDataInt("value");
			player->addStorageValue(key,value);
		}while(result->next());
		db->freeResult(result);
	}

	//load vip
	query.str("");
	query << "SELECT `vip_id` FROM `player_viplist` WHERE `player_id` = " << player->getGUID();
	if((result = db->storeQuery(query.str()))){
		do{
			uint32_t vip_id = result->getDataInt("vip_id");
			std::string dummy_str;
			if(storeNameByGuid(*db, vip_id))
				player->addVIP(vip_id, dummy_str, false, true);
		}while(result->next());
		db->freeResult(result);
	}

	player->updateBaseSpeed();
	player->updateInventoryWeigth();
	player->updateItemsLight(true);

	return true;
}

bool IOPlayer::saveItems(Player* player, const ItemBlockList& itemList, DBInsert& query_insert)
{
	std::list<Container*> listContainer;
	std::stringstream stream;

	typedef std::pair<Container*, int32_t> containerBlock;
	std::list<containerBlock> stack;

	int32_t parentId = 0;
	int32_t runningId = 100;

	Database* db = Database::instance();
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

		stream << player->getGUID() << ", " << pid << ", " << runningId << ", " << item->getID() << ", " << (int32_t)item->getItemCountOrSubtype() << ", " << db->escapeBlob(attributes, attributesSize);

		if(!query_insert.addRow(stream)){
			return false;
		}

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
			if(Container* sub = item->getContainer()){
				stack.push_back(containerBlock(sub, runningId));
			}

			uint32_t attributesSize;

			PropWriteStream propWriteStream;
			item->serializeAttr(propWriteStream);
			const char* attributes = propWriteStream.getStream(attributesSize);

			stream << player->getGUID() << ", " << parentId << ", " << runningId << ", " << item->getID() << ", " << (int32_t)item->getItemCountOrSubtype() << ", " << db->escapeBlob(attributes, attributesSize);

			if(!query_insert.addRow(stream))
				return false;
		}
	}

	return true;
}

bool IOPlayer::savePlayer(Player* player)
{
	player->preSave();

	Database* db = Database::instance();
	DBQuery query;
	DBResult* result;

	//check if the player have to be saved or not
	query << "SELECT `save` FROM `players` WHERE `id` = " << player->getGUID();

	if(!(result = db->storeQuery(query.str()))){
		return false;
	}

	if(result->getDataInt("save") == 0){
		db->freeResult(result);
		return true;
	}
	db->freeResult(result);

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
	query.str("");
	query << "UPDATE `players` SET `level` = " << player->level
	<< ", `vocation` = " << (int)player->getVocationId()
	<< ", `health` = " << player->health
	<< ", `healthmax` = " << player->healthMax
	<< ", `direction` = " << (int)player->getDirection()
	<< ", `experience` = " << player->experience
	<< ", `lookbody` = " << (int)player->defaultOutfit.lookBody
	<< ", `lookfeet` = " << (int)player->defaultOutfit.lookFeet
	<< ", `lookhead` = " << (int)player->defaultOutfit.lookHead
	<< ", `looklegs` = " << (int)player->defaultOutfit.lookLegs
	<< ", `looktype` = " << (int)player->defaultOutfit.lookType
	<< ", `lookaddons` = " << (int)player->defaultOutfit.lookAddons
	<< ", `maglevel` = " << player->magLevel
	<< ", `mana` = " << player->mana
	<< ", `manamax` = " << player->manaMax
	<< ", `manaspent` = " << player->manaSpent
	<< ", `soul` = " << player->soul
	<< ", `town_id` = " << player->town
	<< ", `posx` = " << player->getLoginPosition().x
	<< ", `posy` = " << player->getLoginPosition().y
	<< ", `posz` = " << player->getLoginPosition().z
	<< ", `cap` = " << player->getCapacity()
	<< ", `sex` = " << player->sex
	<< ", `lastlogin` = " << player->lastLoginSaved
	<< ", `lastip` = " << player->lastip
	<< ", `conditions` = " << db->escapeBlob(conditions, conditionsSize)
	<< ", `loss_experience` = " << (int)player->getLossPercent(LOSS_EXPERIENCE)
	<< ", `loss_mana` = " << (int)player->getLossPercent(LOSS_MANASPENT)
	<< ", `loss_skills` = " << (int)player->getLossPercent(LOSS_SKILLTRIES)
	<< ", `balance` = " << player->balance;

#ifndef __USE_SQL_PREMDAYS__
	// there's no need to update prem end here, especially if it hasn't changed!
	// just be sure to only change premium time via a database query.
	/*uint32_t premEnd = 0;
	if(player->premiumDays > 0){
		premEnd = time(NULL) + player->premiumDays * 86400;
	}

	query << ", `premend` = " << premEnd;*/
#endif

#ifdef __SKULLSYSTEM__
	int32_t redSkullTime = 0;
	if(player->redSkullTicks > 0){
		redSkullTime = std::time(NULL) + player->redSkullTicks/1000;
	}

	query << ", `redskulltime` = " << redSkullTime;

	int32_t redSkull = 0;
	if(player->skull == SKULL_RED){
		redSkull = 1;
	}

	query << ", `redskull` = " << redSkull;
#endif

	query << " WHERE `id` = " << player->getGUID();

	DBTransaction transaction(db);
	if(!transaction.begin())
		return false;

	if(!db->executeQuery(query.str())){
		return false;
	}
	query.str("");

	//skills
	for(int i = 0; i <= 6; i++){
		query << "UPDATE `player_skills` SET `value` = " << player->skills[i][SKILL_LEVEL] << ", `count` = " << player->skills[i][SKILL_TRIES] << " WHERE `player_id` = " << player->getGUID() << " AND `skillid` = " << i;

		if(!db->executeQuery(query.str())){
			return false;
		}
		query.str("");
	}

	// deletes all player-related stuff

	query << "DELETE FROM `player_spells` WHERE `player_id` = " << player->getGUID();

	if(!db->executeQuery(query.str())){
		return false;
	}
	query.str("");

	query << "DELETE FROM `player_items` WHERE `player_id` = " << player->getGUID();

	if(!db->executeQuery(query.str())){
		return false;
	}
	query.str("");

	query << "DELETE FROM `player_depotitems` WHERE `player_id` = " << player->getGUID();

	if(!db->executeQuery(query.str())){
		return false;
	}
	query.str("");

	query << "DELETE FROM `player_storage` WHERE `player_id` = " << player->getGUID();

	if(!db->executeQuery(query.str())){
		return false;
	}
	query.str("");

	query << "DELETE FROM `player_viplist` WHERE `player_id` = " << player->getGUID();

	if(!db->executeQuery(query.str())){
		return false;
	}
	query.str("");

	DBInsert stmt(db);

	//learned spells
	stmt.setQuery("INSERT INTO `player_spells` (`player_id`, `name`) VALUES ");

	for(LearnedInstantSpellList::const_iterator it = player->learnedInstantSpellList.begin();
			it != player->learnedInstantSpellList.end(); ++it){
		query << player->getGUID() << ", " << db->escapeString(*it);

		if(!stmt.addRow(query)){
			return false;
		}
	}

	if(!stmt.execute()){
		return false;
	}

	ItemBlockList itemList;

	Item* item;
	for(int32_t slotId = 1; slotId <= 10; ++slotId){
		if((item = player->inventory[slotId])){
			itemList.push_back(itemBlock(slotId, item));
		}
	}

	//item saving
	stmt.setQuery("INSERT INTO `player_items` (`player_id` , `pid` , `sid` , `itemtype` , `count` , `attributes` ) VALUES ");

	if(!(saveItems(player, itemList, stmt) && stmt.execute())){
		return false;
	}

	itemList.clear();
	for(DepotMap::iterator it = player->depots.begin(); it != player->depots.end(); ++it){
		itemList.push_back(itemBlock(it->first, it->second));
	}

	//save depot items
	stmt.setQuery("INSERT INTO `player_depotitems` (`player_id` , `pid` , `sid` , `itemtype` , `count` , `attributes` ) VALUES ");

	if(!(saveItems(player, itemList, stmt) && stmt.execute())){
		return false;
	}

	stmt.setQuery("INSERT INTO `player_storage` (`player_id` , `key` , `value` ) VALUES ");

	player->genReservedStorageRange();
	for(StorageMap::const_iterator cit = player->getStorageIteratorBegin(); cit != player->getStorageIteratorEnd();cit++){
		query << player->getGUID() << ", " << cit->first << ", " << cit->second;

		if(!stmt.addRow(query)){
			return false;
		}
	}

	if(!stmt.execute()){
		return false;
	}

	//save vip list
	stmt.setQuery("INSERT INTO `player_viplist` (`player_id`, `vip_id`) VALUES ");

	for(VIPListSet::iterator it = player->VIPList.begin(); it != player->VIPList.end(); it++){
		query << player->getGUID() << ", " << *it;

		if(!stmt.addRow(query)){
			return false;
		}
	}

	if(!stmt.execute()){
		return false;
	}

	//End the transaction
	return transaction.commit();
}

bool IOPlayer::storeNameByGuid(Database &db, uint32_t guid)
{
	DBQuery query;
	DBResult* result;

	NameCacheMap::iterator it = nameCacheMap.find(guid);
	if(it != nameCacheMap.end())
		return true;

	query << "SELECT `name` FROM `players` WHERE `id` = " << guid;

	if(!(result = db.storeQuery(query.str())))
		return false;

	nameCacheMap[guid] = result->getDataString("name");
	db.freeResult(result);
	return true;
}

bool IOPlayer::getNameByGuid(uint32_t guid, std::string& name)
{
	NameCacheMap::iterator it = nameCacheMap.find(guid);
	if(it != nameCacheMap.end()){
		name = it->second;
		return true;
	}

	Database* db = Database::instance();
	DBQuery query;
	DBResult* result;

	query << "SELECT `name` FROM `players` WHERE `id` = " << guid;

	if(!(result = db->storeQuery(query.str())))
		return false;

	name = result->getDataString("name");
	nameCacheMap[guid] = name;
	db->freeResult(result);
	return true;
}

bool IOPlayer::getGuidByName(uint32_t &guid, std::string& name)
{
	GuidCacheMap::iterator it = guidCacheMap.find(name);
	if(it != guidCacheMap.end()){
		name = it->first;
		guid = it->second;
		return true;
	}

	Database* db = Database::instance();
	DBResult* result;
	DBQuery query;

	if(!(result = db->storeQuery("SELECT `name`, `id` FROM `players` WHERE `name` = " + db->escapeString(name))))
		return false;

	name = result->getDataString("name");
	guid = result->getDataInt("id");

	guidCacheMap[name] = guid;
	db->freeResult(result);
	return true;
}


bool IOPlayer::getGuidByNameEx(uint32_t &guid, bool &specialVip, std::string& name)
{
	Database* db = Database::instance();
	DBResult* result;
	DBQuery query;

	if(!(result = db->storeQuery("SELECT `name`, `id`, `group_id` FROM `players` WHERE `name`= " + db->escapeString(name))))
		return false;

	name = result->getDataString("name");
	guid = result->getDataInt("id");
	const PlayerGroup* group = getPlayerGroup(result->getDataInt("group_id"));

	if(group){
		specialVip = (0 != (group->m_flags & ((uint64_t)1 << PlayerFlag_SpecialVIP)));
	}
	else{
		specialVip = false;
	}

	db->freeResult(result);
	return true;
}

bool IOPlayer::getGuildIdByName(uint32_t &guildId, const std::string& guildName)
{
	Database* db = Database::instance();
	DBResult* result;
	DBQuery query;

	if(!(result = db->storeQuery("SELECT `id` FROM `guilds` WHERE `name` = " + db->escapeString(guildName))))
		return false;

	guildId = result->getDataInt("id");
	db->freeResult(result);
	return true;
}

bool IOPlayer::playerExists(std::string name)
{
	Database* db = Database::instance();
	DBResult* result;
	DBQuery query;

	if(!(result = db->storeQuery("SELECT `id` FROM `players` WHERE `name`= " + db->escapeString(name))))
		return false;

	db->freeResult(result);
	return true;
}

const PlayerGroup* IOPlayer::getPlayerGroup(uint32_t groupid)
{
	PlayerGroupMap::const_iterator it = playerGroupMap.find(groupid);

	if(it != playerGroupMap.end()){
		return it->second;
	}
	else{
		Database* db = Database::instance();
		DBQuery query;
		DBResult* result;

		query << "SELECT * FROM `groups` WHERE `id`= " << groupid;

		if((result = db->storeQuery(query.str()))){
			PlayerGroup* group = new PlayerGroup;

			group->m_name = result->getDataString("name");
			group->m_flags = result->getDataLong("flags");
			group->m_access = result->getDataInt("access");
			group->m_maxDepotItems = result->getDataInt("maxdepotitems");
			group->m_maxVip = result->getDataInt("maxviplist");

			playerGroupMap[groupid] = group;

			db->freeResult(result);
			return group;
		}
	}
	return NULL;
}

void IOPlayer::loadItems(ItemMap& itemMap, DBResult* result)
{
	do{
		int sid = result->getDataInt("sid");
		int pid = result->getDataInt("pid");
		int type = result->getDataInt("itemtype");
		int count = result->getDataInt("count");

		unsigned long attrSize = 0;
		const char* attr = result->getDataStream("attributes", attrSize);

		PropStream propStream;
		propStream.init(attr, attrSize);

		Item* item = Item::CreateItem(type, count);
		if(item){
			if(!item->unserializeAttr(propStream)){
				std::cout << "WARNING: Serialize error in IOPlayer::loadItems" << std::endl;
			}

			std::pair<Item*, int> pair(item, pid);
			itemMap[sid] = pair;
		}
	}while(result->next());
}

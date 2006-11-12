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
	m_host = g_config.getString(ConfigManager::SQL_HOST);
	m_user = g_config.getString(ConfigManager::SQL_USER);
	m_pass = g_config.getString(ConfigManager::SQL_PASS);
	m_db   = g_config.getString(ConfigManager::SQL_DB);
}

bool IOPlayerSQL::loadPlayer(Player* player, std::string name)
{
	Database* mysql = Database::instance();
	DBQuery query;
	DBResult result;
	
	if(!mysql->connect(m_db.c_str(), m_host.c_str(), m_user.c_str(), m_pass.c_str())){
		return false;
	}

	query << "SELECT * FROM players WHERE name='" << Database::escapeString(name) << "'";
	if(!mysql->storeQuery(query, result) || result.getNumRows() != 1)
		return false;

	int accno = result.getDataInt("account");
	if(accno < 1)
		return false;

	// Getting all player properties
	player->setGUID((int)result.getDataInt("id"));

	player->accountNumber = result.getDataInt("account");
	player->setSex((playersex_t)result.getDataInt("sex"));

	player->guildId   = result.getDataInt("guildid");
	player->guildRank = result.getDataString("guildrank");
	player->guildNick = result.getDataString("guildnick");

	player->setDirection((Direction)result.getDataInt("direction"));
	player->experience = result.getDataLong("experience");
	player->level = result.getDataInt("level");
	player->level_percent  = (unsigned char)(100*(player->experience-player->getExpForLv(player->level))/(1.*player->getExpForLv(player->level+1)-player->getExpForLv(player->level)));
	if(player->level_percent > 100)
		player->level_percent = 100;
	player->capacity = result.getDataInt("cap");
	player->maxDepotLimit = result.getDataInt("maxdepotitems");
	player->lastLoginSaved = result.getDataInt("lastlogin");
	#ifdef __SKULLSYSTEM__
	long redSkullSeconds = result.getDataInt("redskulltime") - std::time(NULL);
	if(redSkullSeconds > 0){
		//ensure that we round up the number of ticks
		player->redSkullTicks = (redSkullSeconds + 2)*1000;
		if(result.getDataInt("redskull") == 1){
			player->skull = SKULL_RED;
		}
	}
	#endif

	player->setVocation(result.getDataInt("vocation"));
	player->accessLevel = result.getDataInt("access");
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

	boost::char_separator<char> sep(";");

	//SPAWN
	std::string pos = result.getDataString("pos");
	//std::cout << pos << std::endl;
	tokenizer tokens(pos, sep);

	tokenizer::iterator spawnit = tokens.begin();
	player->loginPosition.x = atoi(spawnit->c_str()); spawnit++;
	player->loginPosition.y = atoi(spawnit->c_str()); spawnit++;
	player->loginPosition.z = atoi(spawnit->c_str());

	//there is no "fuck" in the sources, but every major programm got
	//one and i think here is a good place to add one

	//MASTERSPAWN
	std::string masterpos = result.getDataString("masterpos");
	tokenizer mastertokens(masterpos, sep);

	tokenizer::iterator mspawnit = mastertokens.begin();
	player->masterPos.x = atoi(mspawnit->c_str()); mspawnit++;
	player->masterPos.y = atoi(mspawnit->c_str()); mspawnit++;
	player->masterPos.z = atoi(mspawnit->c_str());


	//get password
	query << "SELECT * FROM accounts WHERE accno='" << accno << "'";
	if(!mysql->storeQuery(query, result))
		return false;

	if(result.getDataInt("accno") != accno)
		return false;

	player->password = result.getDataString("password");


	if(player->guildId){
		query << "SELECT guildname FROM guilds WHERE guildid='" << player->guildId << "'";
		if(mysql->storeQuery(query, result)){
			player->guildName = result.getDataString("guildname");
		}
	}

	// we need to find out our skills
	// so we query the skill table
	query << "SELECT * FROM skills WHERE player='" << player->getGUID() << "'";
	if(mysql->storeQuery(query, result)){
		//now iterate over the skills
		for(uint32_t i = 0; i < result.getNumRows(); ++i){
			int skillid = result.getDataInt("id",i);
			player->skills[skillid][SKILL_LEVEL] = result.getDataInt("skill",i);
			player->skills[skillid][SKILL_TRIES] = result.getDataInt("tries",i);
		}
	}

	//load the items
	OTSERV_HASH_MAP<int,std::pair<Item*,int> > itemmap;
	
	query << "SELECT * FROM items WHERE player='" << player->getGUID() << "' ORDER BY sid ASC";
	if(mysql->storeQuery(query, result) && (result.getNumRows() > 0)){		
		for(uint32_t i=0; i < result.getNumRows(); ++i){
			int type = result.getDataInt("type",i);
			int count = result.getDataInt("number",i);
			Item* myItem = Item::CreateItem(type, count);
			if(result.getDataInt("actionid",i) >= 100)
				myItem->setActionId(result.getDataInt("actionid",i));

			myItem->setText(result.getDataString("text",i));
			myItem->setSpecialDescription(result.getDataString("specialdesc",i));
			std::pair<Item*, int> myPair(myItem, result.getDataInt("pid",i));
			itemmap[result.getDataInt("sid",i)] = myPair;
			if(int slotid = result.getDataInt("slot",i)){
				if(slotid > 0 && slotid <= 10){
					player->__internalAddThing(slotid, myItem);
				}
				else{
					if(Container* container = myItem->getContainer()){
						if(Depot* depot = container->getDepot()){
							player->addDepot(depot, slotid - 100);
						}
						else{
							std::cout << "Error loading depot "<< slotid << " for player " <<
								player->getGUID() << std::endl;
							delete myItem;
						}
					}
					else{
						std::cout << "Error loading depot "<< slotid << " for player " <<
							player->getGUID() << std::endl;
						delete myItem;
					}
				}
			}
		}
	}

	OTSERV_HASH_MAP<int,std::pair<Item*,int> >::iterator it;
	
	for(int i = (int)itemmap.size(); i > 0; --i){
		it = itemmap.find(i);
		if(it == itemmap.end())
			continue;

		if(int p=(*it).second.second){
			if(Container* c = itemmap[p].first->getContainer()) {
				c->__internalAddThing((*it).second.first);
			}
		}
	}

	player->updateInventoryWeigth();
	player->updateItemsLight(true);
	player->setSkillsPercents();

	//load storage map
	query << "SELECT * FROM playerstorage WHERE player='" << player->getGUID() << "'";

	if(mysql->storeQuery(query,result)){
		for(uint32_t i=0; i < result.getNumRows(); ++i){
			unsigned long key = result.getDataInt("key",i);
			long value = result.getDataInt("value",i);
			player->addStorageValue(key,value);
		}
	}

	//load vip
	query << "SELECT vip_id FROM viplist WHERE player='" << player->getGUID() << "'";		
	if(mysql->storeQuery(query,result)){
		for(uint32_t i = 0; i < result.getNumRows(); ++i){
			unsigned long vip_id = result.getDataInt("vip_id",i);
			std::string dummy_str;
			if(storeNameByGuid(*mysql, vip_id)){
				player->addVIP(vip_id, dummy_str, false, true);
			}
		}
	}

	return true;
}

bool IOPlayerSQL::savePlayer(Player* player)
{
	player->preSave();

	Database* mysql = Database::instance();
	DBQuery query;
	DBResult result;
	
	if(!mysql->connect(m_db.c_str(), m_host.c_str(), m_user.c_str(), m_pass.c_str())){
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
	query << "`masterpos` = '" << player->masterPos.x<<";"<< player->masterPos.y<<";"<< player->masterPos.z << "', ";
	query << "`pos` = '" << player->getLoginPosition().x<<";"<< player->getLoginPosition().y<<";"<< player->getLoginPosition().z << "', ";
	query << "`speed` = " << player->baseSpeed << ", ";
	query << "`cap` = " << player->getCapacity() << ", ";
	query << "`sex` = " << player->sex << ", ";
	query << "`lastlogin` = " << player->lastlogin << ", ";
	query << "`lastip` = " << player->lastip << " ";

#ifdef __SKULLSYSTEM__
	long redSkullTime = 0;
	if(player->redSkullTicks > 0){
		redSkullTime = std::time(NULL) + player->redSkullTicks/1000;
	}

	query << ", `redskulltime` = " << redSkullTime << ", ";
	long redSkull = 0;
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

	// Saving Skills
	for(int i = 0; i <= 6; i++){
		query << "UPDATE `skills` SET `skill` = " << player->skills[i][SKILL_LEVEL] <<", `tries` = "<< player->skills[i][SKILL_TRIES] << " WHERE `player` = " << player->getGUID() << " AND  `id` = " << i;

		if(!mysql->executeQuery(query))
			return false;
	}

	//now item saving
	query << "DELETE FROM items WHERE player='"<< player->getGUID() << "'";

	if(!mysql->executeQuery(query))
		return false;

	DBSplitInsert query_insert(mysql);
	query_insert.setQuery("INSERT INTO `items` (`player` , `slot` , `sid` , `pid` , `type` , `number` , `actionid` , `text` , `specialdesc` ) VALUES ");
	
	int runningID = 0;

	typedef std::pair<Container*, int> containerStackPair;
	std::list<containerStackPair> stack;
	Item* item = NULL;
	Container* container = NULL;
	Container* topcontainer = NULL;

	int parentid = 0;
	std::stringstream streamitems;
	
	for(int slotid = 1; slotid <= 10; ++slotid){
		if(!player->inventory[slotid])
			continue;

		item = player->inventory[slotid];
		++runningID;

		streamitems << "(" << player->getGUID() <<"," << slotid << ","<< runningID <<","<< parentid <<"," << item->getID()<<","<< (int)item->getItemCountOrSubtype() << "," <<
			(int)item->getActionId()<<",'"<< Database::escapeString(item->getText()) <<"','" << Database::escapeString(item->getSpecialDescription()) <<"')";
		
		if(!query_insert.addRow(streamitems.str()))
			return false;
		
		streamitems.str("");
        
		topcontainer = item->getContainer();
		if(topcontainer) {
			stack.push_back(containerStackPair(topcontainer, runningID));
		}
	}

	while(stack.size() > 0) {
		
		containerStackPair csPair = stack.front();
		container = csPair.first;
		parentid = csPair.second;
		stack.pop_front();

		for(uint32_t i = 0; i < container->size(); i++){
			++runningID;
			Item* item = container->getItem(i);
			Container* container = item->getContainer();
			if(container){
				stack.push_back(containerStackPair(container, runningID));
			}
			
			streamitems << "(" << player->getGUID() <<"," << 0 /*slotid*/ << ","<< runningID <<","<< parentid <<"," << item->getID()<<","<< (int)item->getItemCountOrSubtype() << "," <<
				(int)item->getActionId()<<",'"<< Database::escapeString(item->getText()) <<"','" << Database::escapeString(item->getSpecialDescription()) <<"')";
			
			if(!query_insert.addRow(streamitems.str()))
				return false;
			
			streamitems.str("");	
		}
	}

	parentid = 0;
	//save depot items
	for(DepotMap::reverse_iterator dit = player->depots.rbegin(); dit !=player->depots.rend() ;++dit){
		item = dit->second;
		++runningID;

		streamitems << "(" << player->getGUID() <<"," << dit->first + 100 << ","<< runningID <<","<< parentid <<"," << item->getID()<<","<< (int)item->getItemCountOrSubtype() << "," <<
			(int)item->getActionId()<<",'"<< Database::escapeString(item->getText()) <<"','" << Database::escapeString(item->getSpecialDescription()) <<"')";

		if(!query_insert.addRow(streamitems.str()))
			return false;
		
		streamitems.str("");

		topcontainer = item->getContainer();
		if(topcontainer){
			stack.push_back(containerStackPair(topcontainer, runningID));
		}
	}

	while(stack.size() > 0) {

		containerStackPair csPair = stack.front();
		container = csPair.first;
		parentid = csPair.second;
		stack.pop_front();

		for(uint32_t i = 0; i < container->size(); i++){
			++runningID;
			Item* item = container->getItem(i);
			Container* container = item->getContainer();
			if(container) {
				stack.push_back(containerStackPair(container, runningID));
			}

			streamitems << "(" << player->getGUID() <<"," << 0 /*slotid*/ << ","<< runningID <<","<< parentid <<"," << item->getID()<<","<< (int)item->getItemCountOrSubtype() << "," <<
			(int)item->getActionId()<<",'"<< Database::escapeString(item->getText()) <<"','" << Database::escapeString(item->getSpecialDescription()) <<"')";

			if(!query_insert.addRow(streamitems.str()))
				return false;

            streamitems.str("");
		}
	}
	if(!query_insert.executeQuery())
		return false;

	//save storage map
	query.reset();
	query << "DELETE FROM playerstorage WHERE player='"<< player->getGUID() << "'";

	if(!mysql->executeQuery(query))
		return false;

	query_insert.setQuery("INSERT INTO `playerstorage` (`player` , `key` , `value` ) VALUES ");
	player->genReservedStorageRange();
	for(StorageMap::const_iterator cit = player->getStorageIteratorBegin(); cit != player->getStorageIteratorEnd();cit++){
		streamitems << "(" << player->getGUID() <<","<< cit->first <<","<< cit->second<<")";
		
		if(!query_insert.addRow(streamitems.str()))
			return false;
		
		streamitems.str("");
	}
	if(!query_insert.executeQuery())
		return false;
    
    
	//save vip list
	query.reset();
	query << "DELETE FROM `viplist` WHERE player='"<< player->getGUID() << "'";

	if(!mysql->executeQuery(query))
		return false;

	query_insert.setQuery("INSERT INTO `viplist` (`player` , `vip_id` ) VALUES ");
	for(VIPListSet::iterator it = player->VIPList.begin(); it != player->VIPList.end(); it++){
		streamitems << "(" << player->getGUID() <<","<< *it <<")";
		
		if(!query_insert.addRow(streamitems.str()))
			return false;
		
		streamitems.str("");
	}
	if(!query_insert.executeQuery())
		return false;
    
	//End the transaction
	return trans.success();
}

std::string IOPlayerSQL::getItems(Item* i, int &startid, int slot, int player,int parentid)
{
	++startid;
	std::stringstream ss;

	ss << "(" << player <<"," << slot << ","<< startid <<","<< parentid <<"," << i->getID()<<","<< (int)i->getItemCountOrSubtype() << "," <<
	(int)i->getActionId()<<",'"<< Database::escapeString(i->getText()) <<"','" << Database::escapeString(i->getSpecialDescription()) <<"'),";

	//std::cout << "i";
	if(Container* c = i->getContainer()){
		//std::cout << "c";
		int pid = startid;
		for(uint32_t i = 0; i < c->size(); i++){
			Item* item = c->getItem(i);
			//std::cout << "s";
			ss << getItems(item, startid, 0, player, pid);
			//std::cout << "r";
		}
	}

	return ss.str();
}

bool IOPlayerSQL::storeNameByGuid(Database &mysql, unsigned long guid)
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

bool IOPlayerSQL::getNameByGuid(unsigned long guid, std::string& name)
{
	NameCacheMap::iterator it = nameCacheMap.find(guid);
	if(it != nameCacheMap.end()){
		name = it->second;
		return true;
	}

	Database* mysql = Database::instance();
	DBQuery query;
	DBResult result;
	
	if(!mysql->connect(m_db.c_str(), m_host.c_str(), m_user.c_str(), m_pass.c_str())){
		return false;
	}

	query << "SELECT name FROM players WHERE id='" << guid << "'";
	if(!mysql->storeQuery(query, result) || result.getNumRows() != 1)
		return false;

	name = result.getDataString("name");
	nameCacheMap[guid] = name;

	return true;
}

bool IOPlayerSQL::getGuidByName(unsigned long &guid, std::string& name)
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
	
	if(!mysql->connect(m_db.c_str(), m_host.c_str(), m_user.c_str(), m_pass.c_str())){
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


bool IOPlayerSQL::getGuidByNameEx(unsigned long &guid, unsigned long &alvl, std::string& name)
{
	Database* mysql = Database::instance();
	DBQuery query;
	DBResult result;
	
	if(!mysql->connect(m_db.c_str(), m_host.c_str(), m_user.c_str(), m_pass.c_str())){
		return false;
	}

	query << "SELECT name,id,access FROM players WHERE name='" << Database::escapeString(name) << "'";
	if(!mysql->storeQuery(query, result) || result.getNumRows() != 1)
		return false;

	name = result.getDataString("name");
	guid = result.getDataInt("id");
	alvl = result.getDataInt("access");
	return true;
}

bool IOPlayerSQL::getGuildIdByName(unsigned long &guildId, const std::string& guildName)
{
	Database* mysql = Database::instance();
	DBQuery query;
	DBResult result;
	
	if(!mysql->connect(m_db.c_str(), m_host.c_str(), m_user.c_str(), m_pass.c_str())){
		return false;
	}

	query << "SELECT guildid FROM guilds WHERE guildname='" << Database::escapeString(guildName) << "'";
	if(!mysql->storeQuery(query, result) || result.getNumRows() != 1)
		return false;

	guildId = result.getDataInt("guildid");
	return true;
}

bool IOPlayerSQL::playerExists(std::string name)
{
	Database* mysql = Database::instance();
	DBQuery query;
	DBResult result;
	
	if(!mysql->connect(m_db.c_str(), m_host.c_str(), m_user.c_str(), m_pass.c_str())){
		return false;
	}

	query << "SELECT name FROM players WHERE name='" << Database::escapeString(name) << "'";
	if(!mysql->storeQuery(query, result) || result.getNumRows() != 1)
		return false;

	return true;
}

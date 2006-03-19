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


#include "ioplayer.h"
#include "ioplayersql.h"
#include "ioaccount.h"
#include "item.h"
#include "luascript.h"
#include "database.h"

#include <boost/tokenizer.hpp>
#include <iostream>
#include <iomanip>

// cross compatibility vc++ and gcc
#ifdef __GNUC__
#include <ext/hash_map>
#else
#include <hash_map>
#endif

extern LuaScript g_config;

typedef boost::tokenizer<boost::char_separator<char> > tokenizer;

#pragma warning( disable : 4005)
#pragma warning( disable : 4996)

IOPlayerSQL::IOPlayerSQL()
{
	m_host = g_config.getGlobalString("sql_host");
	m_user = g_config.getGlobalString("sql_user");
	m_pass = g_config.getGlobalString("sql_pass");
	m_db   = g_config.getGlobalString("sql_db");
}

bool IOPlayerSQL::loadPlayer(Player* player, std::string name)
{
	Database mysql;
	DBQuery query;
	DBResult result;

	mysql.connect(m_db.c_str(), m_host.c_str(), m_user.c_str(), m_pass.c_str());

	query << "SELECT * FROM players WHERE name='" << Database::escapeString(name) << "'";
	//std::cout << query.GetText() << std::endl;
	if(!mysql.storeQuery(query, result) || result.getNumRows() != 1)
		return false;
	
	int accno = result.getDataInt("account");
	if(accno < 1)
		return false;
	
	// Getting all player properties
	player->setGUID((int)result.getDataInt("id"));
	
	player->accountNumber = result.getDataInt("account");
	player->sex = (playersex_t)result.getDataInt("sex");
	
	player->guildId   = result.getDataInt("guildid");
	player->guildRank = result.getDataString("guildrank");
	player->guildNick = result.getDataString("guildnick");
	
	player->setDirection((Direction)result.getDataInt("direction"));
	player->experience = result.getDataLong("experience");
	player->level = result.getDataInt("level");
	player->level_percent  = (unsigned char)(100*(player->experience-player->getExpForLv(player->level))/(1.*player->getExpForLv(player->level+1)-player->getExpForLv(player->level)));
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

	player->vocation = (playervoc_t)result.getDataInt("vocation");
	player->access = result.getDataInt("access");
	player->setNormalSpeed();
	
	player->mana = result.getDataInt("mana");
	player->manamax = result.getDataInt("manamax");
	player->manaspent = result.getDataInt("manaspent");
	player->maglevel = result.getDataInt("maglevel");
	player->maglevel_percent  = (unsigned char)(100*(player->manaspent/(1.*player->getReqMana(player->maglevel+1, player->vocation))));

	player->health = result.getDataInt("health");
	if(player->health <= 0)
		player->health = 100;

	player->healthmax = result.getDataInt("healthmax");
	if(player->healthmax <= 0)
		player->healthmax = 100;

	player->food = result.getDataInt("food");

	player->looktype = result.getDataInt("looktype");
	player->lookmaster = player->looktype;
	player->lookhead = result.getDataInt("lookhead");
	player->lookbody = result.getDataInt("lookbody");
	player->looklegs = result.getDataInt("looklegs");
	player->lookfeet = result.getDataInt("lookfeet");

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
	if(!mysql.storeQuery(query, result))
		return false;
	
	if(result.getDataInt("accno") != accno)
		return false;
	
	player->password = result.getDataString("password");


	if(player->guildId){
		query << "SELECT guildname FROM guilds WHERE guildid='" << player->guildId << "'";
		if(mysql.storeQuery(query, result)){
			player->guildName = result.getDataString("guildname");
		}
	}	
	
	// we need to find out our skills
	// so we query the skill table
	query << "SELECT * FROM skills WHERE player='" << player->getGUID() << "'";
	if(mysql.storeQuery(query, result)){
		//now iterate over the skills
		for(int i=0; i < result.getNumRows(); ++i){
			int skillid = result.getDataInt("id",i);
			player->skills[skillid][SKILL_LEVEL] = result.getDataInt("skill",i);
			player->skills[skillid][SKILL_TRIES] = result.getDataInt("tries",i);
			player->skills[skillid][SKILL_PERCENT] = (unsigned int)(100*(player->skills[skillid][SKILL_TRIES])/(1.*player->getReqSkillTries(skillid, (player->skills[skillid][SKILL_LEVEL]+1), player->vocation)));
		}
	}
	
	//load the items
	
	// cross compatibility vc++ and gcc
#ifdef __GNUC__
	__gnu_cxx::hash_map<int,std::pair<Item*,int> > itemmap;
#else
	stdext::hash_map<int,std::pair<Item*,int> > itemmap;
#endif
	query << "SELECT * FROM items WHERE player='" << player->getGUID() << "' ORDER BY sid ASC";
	if(mysql.storeQuery(query, result) && (result.getNumRows() > 0)){		
		for(int i=0; i < result.getNumRows(); ++i){
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
	
	// cross compatibility vc++ and gcc
#ifdef __GNUC__
	__gnu_cxx::hash_map<int,std::pair<Item*,int> >::iterator it;
#else
	stdext::hash_map<int,std::pair<Item*,int> >::iterator it;
#endif
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

	//load storage map
	query << "SELECT * FROM playerstorage WHERE player='" << player->getGUID() << "'";
	
	if(mysql.storeQuery(query,result)){
		for(int i=0; i < result.getNumRows(); ++i){
			unsigned long key = result.getDataInt("key",i);
			long value = result.getDataInt("value",i);
			player->addStorageValue(key,value);
		}
	}
	
	//load vip
	query << "SELECT vip_id FROM viplist WHERE player='" << player->getGUID() << "'";		
	if(mysql.storeQuery(query,result)){
		for(int i=0; i < result.getNumRows(); ++i){
			unsigned long vip_id = result.getDataInt("vip_id",i);
			std::string dummy_str;
			if(storeNameByGuid(mysql, vip_id)){
				player->addVIP(vip_id, dummy_str, false, true);
			}
		}
	}

	return true;
}

bool IOPlayerSQL::savePlayer(Player* player)
{
	player->preSave();
		
	Database mysql;
	DBQuery query;
	DBResult result;
	
	mysql.connect(m_db.c_str(), m_host.c_str(), m_user.c_str(), m_pass.c_str());
	
	//check if the player have to be saved or not
	query << "SELECT save FROM players WHERE id='" << player->getGUID() << "'";
	if(!mysql.storeQuery(query,result) || (result.getNumRows() != 1) )
		return false;
	
	if(result.getDataInt("save") != 1) // If save var is not 1 don't save the player info
		return true;
	
	//Start the transaction	
	query << "BEGIN;";
	if(!mysql.executeQuery(query))
		return false;
	
	//First, an UPDATE query to write the player itself
	query << "UPDATE `players` SET ";
	query << "`level` = " << player->level << ", ";
	query << "`vocation` = " << (int)player->vocation << ", ";
	query << "`health` = " << player->health << ", ";
	query << "`healthmax` = " << player->healthmax << ", ";
	query << "`direction` = " << (int)player->getDirection() << ", ";
	query << "`experience` = " << player->experience << ", ";
	query << "`lookbody` = " << player->lookbody << ", ";
	query << "`lookfeet` = " << player->lookfeet << ", ";
	query << "`lookhead` = " << player->lookhead << ", ";
	query << "`looklegs` = " << player->looklegs << ", ";
	query << "`looktype` = " << player->looktype << ", ";
	query << "`maglevel` = " << player->maglevel << ", ";
	query << "`mana` = " << player->mana << ", ";
	query << "`manamax` = " << player->manamax << ", ";
	query << "`manaspent` = " << player->manaspent << ", ";
	query << "`masterpos` = '" << player->masterPos.x<<";"<< player->masterPos.y<<";"<< player->masterPos.z << "', ";
	query << "`pos` = '" << player->getLoginPosition().x<<";"<< player->getLoginPosition().y<<";"<< player->getLoginPosition().z << "', ";
	query << "`speed` = " << player->speed << ", ";
	query << "`cap` = " << player->getCapacity() << ", ";
	query << "`food` = " << player->food << ", ";
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

	query << " WHERE `id` = "<< player->getGUID() <<" LIMIT 1";
	
	if(!mysql.executeQuery(query))
		return false;
	
	// Saving Skills
	for(int i = 0; i <= 6; i++){
		query << "UPDATE `skills` SET `skill` = " << player->skills[i][SKILL_LEVEL] <<", `tries` = "<< player->skills[i][SKILL_TRIES] << " WHERE `player` = " << player->getGUID() << " AND  `id` = " << i;
	
		if(!mysql.executeQuery(query))
			return false;
	}
	
	//now item saving
	query << "DELETE FROM items WHERE player='"<< player->getGUID() << "'";
	
	if(!mysql.executeQuery(query))
		return false;
	
	query << "INSERT INTO `items` (`player` , `slot` , `sid` , `pid` , `type` , `number` , `actionid` , `text` , `specialdesc` ) VALUES"; 
	int runningID = 0;

	typedef std::pair<Container*, int> containerStackPair;
	std::list<containerStackPair> stack;
	Item* item = NULL;
	Container* container = NULL;
	Container* topcontainer = NULL;

	int parentid = 0;
	std::stringstream streamitems;
	std::string itemsstring;
	for(int slotid = 1; slotid <= 10; ++slotid){
		if(!player->items[slotid])
			continue;

		item = player->items[slotid];
		++runningID;

		streamitems << "(" << player->getGUID() <<"," << slotid << ","<< runningID <<","<< parentid <<"," << item->getID()<<","<< (int)item->getItemCountOrSubtype() << "," << 
			(int)item->getActionId()<<",'"<< Database::escapeString(item->getText()) <<"','" << Database::escapeString(item->getSpecialDescription()) <<"'),";

		topcontainer = item->getContainer();
		if(topcontainer) {
			stack.push_back(containerStackPair(topcontainer, runningID));
		}
	}
			
	while(stack.size() > 0) {
		//split into sub-queries
		if(streamitems.str().length() > 8192) {
			DBQuery subquery;
			subquery << query.str();

			itemsstring = streamitems.str();
			itemsstring.erase(itemsstring.length()-1);
			subquery << itemsstring;

			if(!mysql.executeQuery(subquery))
				return false;

			streamitems.str("");
			itemsstring = "";
		}

		containerStackPair csPair = stack.front();
		container = csPair.first;
		parentid = csPair.second;
		stack.pop_front();

		for(int i = 0; i < container->size(); i++){
			++runningID;
			Item* item = container->getItem(i);
			Container* container = item->getContainer();
			if(container){
				stack.push_back(containerStackPair(container, runningID));
			}
			streamitems << "(" << player->getGUID() <<"," << 0 /*slotid*/ << ","<< runningID <<","<< parentid <<"," << item->getID()<<","<< (int)item->getItemCountOrSubtype() << "," << 
			(int)item->getActionId()<<",'"<< Database::escapeString(item->getText()) <<"','" << Database::escapeString(item->getSpecialDescription()) <<"'),";
		}
	}

	parentid = 0;
	//save depot items
	for(DepotMap::reverse_iterator dit = player->depots.rbegin(); dit !=player->depots.rend() ;++dit){
		item = dit->second;
		++runningID;

		streamitems << "(" << player->getGUID() <<"," << dit->first + 100 << ","<< runningID <<","<< parentid <<"," << item->getID()<<","<< (int)item->getItemCountOrSubtype() << "," << 
			(int)item->getActionId()<<",'"<< Database::escapeString(item->getText()) <<"','" << Database::escapeString(item->getSpecialDescription()) <<"'),";

		topcontainer = item->getContainer();
		if(topcontainer){
			stack.push_back(containerStackPair(topcontainer, runningID));
		}
	}
			
	while(stack.size() > 0) {
		//split into sub-queries
		if(streamitems.str().length() > 8192) {
			DBQuery subquery;
			subquery << query.str();

			itemsstring = streamitems.str();
			itemsstring.erase(itemsstring.length()-1);
			subquery << itemsstring;

			if(!mysql.executeQuery(subquery))
				return false;

			streamitems.str("");
			itemsstring = "";
		}

		containerStackPair csPair = stack.front();
		container = csPair.first;
		parentid = csPair.second;
		stack.pop_front();

		for(int i = 0; i < container->size(); i++){
			++runningID;
			Item* item = container->getItem(i);	
			Container* container = item->getContainer();
			if(container) {
				stack.push_back(containerStackPair(container, runningID));
			}

			streamitems << "(" << player->getGUID() <<"," << 0 /*slotid*/ << ","<< runningID <<","<< parentid <<"," << item->getID()<<","<< (int)item->getItemCountOrSubtype() << "," << 
			(int)item->getActionId()<<",'"<< Database::escapeString(item->getText()) <<"','" << Database::escapeString(item->getSpecialDescription()) <<"'),";
		}
	}
	
	if(streamitems.str().length() > 0){
		itemsstring = streamitems.str();
		itemsstring.erase(itemsstring.length()-1);
		query << itemsstring;
		
		if(!mysql.executeQuery(query))
			return false;
		
		streamitems.str("");
		itemsstring = "";
	}
	
	//save storage map
	query.reset();
	query << "DELETE FROM playerstorage WHERE player='"<< player->getGUID() << "'";
	
	if(!mysql.executeQuery(query))
		return false;
	
	query << "INSERT INTO `playerstorage` (`player` , `key` , `value` ) VALUES"; 
	std::stringstream ss;
	for(StorageMap::const_iterator cit = player->getStorageIteratorBegin(); cit != player->getStorageIteratorEnd();cit++){
		ss << "(" << player->getGUID() <<","<< cit->first <<","<< cit->second<<"),";
	}

	std::string ststring = ss.str();
	if(ststring.length()){
		ststring.erase(ststring.length()-1);
		query << ststring;
		
		if(!mysql.executeQuery(query))
			return false;
	}
	
	//save vip list
	query.reset();
	query << "DELETE FROM `viplist` WHERE player='"<< player->getGUID() << "'";
	
	if(!mysql.executeQuery(query))
		return false;
	
	query << "INSERT INTO `viplist` (`player` , `vip_id` ) VALUES"; 
	std::stringstream ss2;
	for(VIPListSet::iterator it = player->VIPList.begin(); it != player->VIPList.end(); it++){
		ss2 << "(" << player->getGUID() <<","<< *it <<"),";
	}

	ststring = ss2.str();
	if(ststring.length()){
		ststring.erase(ststring.length()-1);
		query << ststring;
		
		if(!mysql.executeQuery(query))
			return false;
	}
	
	//End the transaction
	query.reset();
	query << "COMMIT;";
	
	if(!mysql.executeQuery(query))
		return false;

	return true;
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
		for(int i = 0; i < c->size(); i++){
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

bool IOPlayerSQL::getNameByGuid(unsigned long guid, std::string &name)
{
	NameCacheMap::iterator it = nameCacheMap.find(guid);
	if(it != nameCacheMap.end()){
		name = it->second;
		return true;
	}
	
	Database mysql;
	DBQuery query;
	DBResult result;
	
	mysql.connect(m_db.c_str(), m_host.c_str(), m_user.c_str(), m_pass.c_str());
	query << "SELECT name FROM players WHERE id='" << guid << "'";
	
	if(!mysql.storeQuery(query, result) || result.getNumRows() != 1)
		return false;
	
	name = result.getDataString("name");
	nameCacheMap[guid] = name;
	
	return true;
}

bool IOPlayerSQL::getGuidByName(unsigned long &guid, unsigned long &alvl, std::string &name)
{
	Database mysql;
	DBQuery query;
	DBResult result;
	
	mysql.connect(m_db.c_str(), m_host.c_str(), m_user.c_str(), m_pass.c_str());
	
	query << "SELECT name,id,access FROM players WHERE name='" << Database::escapeString(name) << "'";
	
	if(!mysql.storeQuery(query, result) || result.getNumRows() != 1)
		return false;
	
	name = result.getDataString("name");
	guid = result.getDataInt("id");
	alvl = result.getDataInt("access");
	return true;
}

bool IOPlayerSQL::getGuildIdByName(unsigned long &guildId, const std::string& guildName)
{
	Database mysql;
	DBQuery query;
	DBResult result;
	
	mysql.connect(m_db.c_str(), m_host.c_str(), m_user.c_str(), m_pass.c_str());
	
	query << "SELECT guildid FROM guilds WHERE guildname='" << Database::escapeString(guildName) << "'";
	if(!mysql.storeQuery(query, result) || result.getNumRows() != 1)
		return false;
		
	guildId = result.getDataInt("guildid");
	return true;
}

bool IOPlayerSQL::playerExists(std::string name)
{
	Database mysql;
	DBQuery query;
	DBResult result;
	
	mysql.connect(m_db.c_str(), m_host.c_str(), m_user.c_str(), m_pass.c_str());	
	
	query << "SELECT name FROM players WHERE name='" << Database::escapeString(name) << "'";
	
	if(!mysql.storeQuery(query, result) || result.getNumRows() != 1)
		return false;

	return true;
}

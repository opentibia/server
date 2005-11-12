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

#include <iostream>
#include <iomanip>

// cross compatibility vc++ and gcc
#ifdef __GNUC__
#include <ext/hash_map>
#else
#include <hash_map>
#endif


#include "database.h"

#include <boost/tokenizer.hpp>


extern LuaScript g_config;

typedef boost::tokenizer<boost::char_separator<char> > tokenizer;

#pragma warning( disable : 4005)
#pragma warning( disable : 4996)

IOPlayerSQL::IOPlayerSQL(){
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
		
		player->guildId   = result.getDataLong("guildid");
		player->guildRank = result.getDataString("guildrank");
		player->guildNick = result.getDataString("guildnick");
		
		player->setDirection((Direction)result.getDataInt("direction"));
		player->experience = result.getDataLong("experience");
		player->level = result.getDataInt("level");
		player->level_percent  = (unsigned char)(100*(player->experience-player->getExpForLv(player->level))/(1.*player->getExpForLv(player->level+1)-player->getExpForLv(player->level)));
		player->capacity = result.getDataInt("cap");
		player->max_depot_items = result.getDataInt("maxdepotitems");
		player->lastLoginSaved = result.getDataInt("lastlogin");
	
		player->vocation = (playervoc_t)result.getDataInt("vocation");
		player->access = result.getDataInt("access");
		player->setNormalSpeed();
		
		player->mana = result.getDataInt("mana");
		player->manamax = result.getDataInt("manamax");
		player->manaspent = result.getDataInt("manaspent");
		player->maglevel = result.getDataInt("maglevel");
		player->maglevel_percent  = (unsigned char)(100*(player->manaspent/(1.*player->getReqMana(player->maglevel+1, player->vocation))));

		player->health = result.getDataInt("health");
		player->healthmax = result.getDataInt("healthmax");
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
		player->pos.x = atoi(spawnit->c_str()); spawnit++;
		player->pos.y = atoi(spawnit->c_str()); spawnit++;
		player->pos.z = atoi(spawnit->c_str());
		
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
		if(mysql.storeQuery(query, result))
		{
			//now iterate over the skills
			for(int i=0; i < result.getNumRows(); ++i)
			{
				int skillid=result.getDataInt("id",i);
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
		if(mysql.storeQuery(query, result) && (result.getNumRows() > 0))
		{
			
			for(int i=0; i < result.getNumRows(); ++i)
			{
				int type = result.getDataInt("type",i);
				int count = result.getDataInt("number",i);
				Item* myItem = Item::CreateItem(type, count);
				if(result.getDataInt("actionid",i) >= 100)
					myItem->setActionId(result.getDataInt("actionid",i));
				myItem->setText(result.getDataString("text",i));	
				myItem->setSpecialDescription(result.getDataString("specialdesc",i));
				std::pair<Item*, int> myPair(myItem, result.getDataInt("pid",i));
				itemmap[result.getDataInt("sid",i)] = myPair;
				if(int slotid = result.getDataInt("slot",i))
				{
					if(slotid > 0 && slotid <= 10){
						player->addItemInventory(myItem, slotid, true);
					}
					else{
						if(dynamic_cast<Container*>(myItem)){
							player->addDepot(dynamic_cast<Container*>(myItem), slotid - 100);
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
		for(int i = (int)itemmap.size(); i > 0; --i) 
		{
			it = itemmap.find(i);
			if(it == itemmap.end())
				continue;
	
			if(int p=(*it).second.second) {
				if(Container* c = dynamic_cast<Container*>(itemmap[p].first)) {
					c->addItem((*it).second.first);
				}
			}
		}
		
		player->updateInventoryWeigth();

		//load storage map
		query << "SELECT * FROM playerstorage WHERE player='" << player->getGUID() << "'";
		
		if(mysql.storeQuery(query,result))
		{
			for(int i=0; i < result.getNumRows(); ++i)
			{
				unsigned long key = result.getDataInt("key",i);
				long value = result.getDataInt("value",i);
				player->addStorageValue(key,value);
			}
		}
		
		//load vip
		query << "SELECT vip_id FROM viplist WHERE player='" << player->getGUID() << "'";		
		if(mysql.storeQuery(query,result))
		{
			for(int i=0; i < result.getNumRows(); ++i)
			{
				unsigned long vip_id = result.getDataInt("vip_id",i);
				std::string dummy_str;
				if(storeNameByGuid(mysql, vip_id)){
					player->addVIP(vip_id, dummy_str, false, true);
				}
			}
		}
		
	/*}
	catch(DBError e)
	{
		switch(e.getType())
		{
		case DB_ERROR_QUERY:
		case DB_ERROR_STORE:
		case DB_ERROR_DATA_NOT_FOUND:
			std::cout << "DB WARNING: (" << e.getType() << ") " << e.getMsg() << std::endl;
			break;
		default:
			std::cout << "DB ERROR: (" << e.getType() << ") " << e.getMsg() << std::endl;
			return false;
		}
	}
	catch(...)
	{
		std::cout << "ERROR: Unknown exception raised.\n\tFile: " << __FILE__ << "\n\tLine: " << __LINE__ << std::endl;
		return false;
	}*/

	return true;
}

bool IOPlayerSQL::savePlayer(Player* player)
{
	//return true;
	player->preSave();
		
	//try
	//{
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
		query << "`pos` = '" << player->pos.x<<";"<< player->pos.y<<";"<< player->pos.z << "', ";
		query << "`speed` = " << player->speed << ", ";
		//query << "`cap` = " << player->cap << ", ";
		query << "`cap` = " << player->getCapacity() << ", ";
		query << "`food` = " << player->food << ", ";
		query << "`sex` = " << player->sex << ", ";
		query << "`lastlogin` = " << player->lastlogin << ", ";
		query << "`lastip` = " << player->lastip << " ";
		query << " WHERE `id` = "<< player->getGUID() <<" LIMIT 1";
		
		if(!mysql.executeQuery(query))
			return false;
		
		// Saving Skills
		for(int i = 0; i <= 6; i++)
		{
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
		ContainerList::const_iterator it;
		Item* item = NULL;
		Container* container = NULL;
		Container* topcontainer = NULL;

		int parentid = 0;
		std::stringstream streamitems;
		std::string itemsstring;
		for (int slotid = 1; slotid <= 10; ++slotid) {
			if(!player->items[slotid])
				continue;

			item = player->items[slotid];
			++runningID;

			streamitems << "(" << player->getGUID() <<"," << slotid << ","<< runningID <<","<< parentid <<"," << item->getID()<<","<< (int)item->getItemCountOrSubtype() << "," << 
				(int)item->getActionId()<<",'"<< Database::escapeString(item->getText()) <<"','" << Database::escapeString(item->getSpecialDescription()) <<"'),";

			topcontainer = dynamic_cast<Container*>(item);
			if(topcontainer) {
				stack.push_back(containerStackPair(topcontainer, runningID));
			}

			/*
			if(player->items[i])
				itemsstring += getItems(player->items[i],runningID,i,player->getGUID(),0);
			*/
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

			for (it = container->getItems(); it != container->getEnd(); ++it) {
				++runningID;
				Container *container = dynamic_cast<Container*>(*it);
				if(container) {
					stack.push_back(containerStackPair(container, runningID));
				}

				streamitems << "(" << player->getGUID() <<"," << 0 /*slotid*/ << ","<< runningID <<","<< parentid <<"," << (*it)->getID()<<","<< (int)(*it)->getItemCountOrSubtype() << "," << 
				(int)(*it)->getActionId()<<",'"<< Database::escapeString((*it)->getText()) <<"','" << Database::escapeString((*it)->getSpecialDescription()) <<"'),";
			}
		}

		parentid = 0;
		//save depot items
		for(DepotMap::reverse_iterator dit = player->depots.rbegin(); dit !=player->depots.rend() ;++dit)
		{
			item = dit->second;
			++runningID;

			streamitems << "(" << player->getGUID() <<"," << dit->first + 100 << ","<< runningID <<","<< parentid <<"," << item->getID()<<","<< (int)item->getItemCountOrSubtype() << "," << 
				(int)item->getActionId()<<",'"<< Database::escapeString(item->getText()) <<"','" << Database::escapeString(item->getSpecialDescription()) <<"'),";

			topcontainer = dynamic_cast<Container*>(item);
			if(topcontainer) {				
				stack.push_back(containerStackPair(topcontainer, runningID));
			}

			//itemsstring += getItems(it->second,runningID,it->first+100,player->getGUID(),0);
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

			for (it = container->getItems(); it != container->getEnd(); ++it) {
				++runningID;
				Container *container = dynamic_cast<Container*>(*it);
				if(container) {
					stack.push_back(containerStackPair(container, runningID));
				}

				streamitems << "(" << player->getGUID() <<"," << 0 /*slotid*/ << ","<< runningID <<","<< parentid <<"," << (*it)->getID()<<","<< (int)(*it)->getItemCountOrSubtype() << "," << 
				(int)(*it)->getActionId()<<",'"<< Database::escapeString((*it)->getText()) <<"','" << Database::escapeString((*it)->getSpecialDescription()) <<"'),";
			}
		}
		
		if(streamitems.str().length() > 0)
		{
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
		for(StorageMap::const_iterator cit = player->getStorageIteratorBegin(); cit != player->getStorageIteratorEnd();cit++)
		{
			ss << "(" << player->getGUID() <<","<< cit->first <<","<< cit->second<<"),";
		}
		std::string ststring = ss.str();
		if(ststring.length())
		{
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
		
	/*}
	catch(DBError e)
	{
		switch(e.getType())
		{
		case DB_ERROR_QUERY:
		case DB_ERROR_STORE:
		case DB_ERROR_DATA_NOT_FOUND:
			std::cout << "DB WARNING: (" << e.getType() << ") " << e.getMsg() << std::endl;
			break;
		default:
			std::cout << "DB ERROR: (" << e.getType() << ") " << e.getMsg() << std::endl;
			return false;
		}
	}
	catch(...)
	{
		std::cout << "ERROR: Unknown exception raised.\n\tFile: " << __FILE__ << "\n\tLine: " << __LINE__ << std::endl;
		return false;
	}*/
	return true;

}

std::string IOPlayerSQL::getItems(Item* i, int &startid, int slot, int player,int parentid)
{
	++startid;
	std::stringstream ss;

	ss << "(" << player <<"," << slot << ","<< startid <<","<< parentid <<"," << i->getID()<<","<< (int)i->getItemCountOrSubtype() << "," << 
	(int)i->getActionId()<<",'"<< Database::escapeString(i->getText()) <<"','" << Database::escapeString(i->getSpecialDescription()) <<"'),";

	//std::cout << "i";
	if(Container* c = dynamic_cast<Container*>(i)){
		//std::cout << "c";	
		int pid = startid;
		for(ContainerList::const_iterator it = c->getItems(); it != c->getEnd(); it++){
			//std::cout << "s";
			ss << getItems(*it, startid, 0, player, pid);
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
	return false;
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


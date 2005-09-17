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

#ifdef _OLD_MYSQL_
	#include <mysql++.h>
#elif USE_MYSQL
	#include "database.h"
	//#include <mysql++.h>
#endif
#include <boost/tokenizer.hpp>


extern LuaScript g_config;

typedef boost::tokenizer<boost::char_separator<char> > tokenizer;

#pragma warning( disable : 4005)
#pragma warning( disable : 4996)

bool IOPlayerSQL::loadPlayer(Player* player, std::string name){
	std::string host = g_config.getGlobalString("sql_host");
	std::string user = g_config.getGlobalString("sql_user");
	std::string pass = g_config.getGlobalString("sql_pass");
	std::string db   = g_config.getGlobalString("sql_db");

#ifndef _OLD_MYSQL_

	//try
	//{
		Database mysql;
		DBQuery query;
		DBResult result;
	
		mysql.connect(db.c_str(), host.c_str(), user.c_str(), pass.c_str());
		//mysql.Connect("otserv", "localhost", "root", "test");

		query << "SELECT * FROM players WHERE name='" << Database::escapeString(name) << "'";
		//std::cout << query.GetText() << std::endl;
		if(!mysql.storeQuery(query, result) || result.getNumRows() != 1)
			return false;
		
		int accno = result.getDataInt("account");
		if(accno < 1)
			return false;
		
		/*
		DBResult result_temp;
		query << "SELECT * FROM accounts WHERE id='" << accno << "'";
		if(!mysql.StoreQuery(query, result_temp) || result_temp.GetNumRows() != 1)
			return false;
		*/
		
		// Check the password. TODO: Change this to use only 1 connection, doing the query from here.
		Account a = IOAccount::instance()->loadAccount(accno);
			
		player->password = a.password;
		if (a.accnumber == 0 || a.accnumber != accno)
			return false;
		
		// Getting all player properties
		player->setGUID((int)result.getDataInt("id"));
		
		player->accountNumber = result.getDataInt("account");
		player->sex = (playersex_t)result.getDataInt("sex");
		player->guildName = result.getDataString("guildname");
		player->guildRank = result.getDataString("guildrank");
		
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
	
		//std::cout << "sql loading: " << player->pos << std::endl;
	
		//MASTERSPAWN
		std::string masterpos = result.getDataString("masterpos");
		tokenizer mastertokens(masterpos, sep);
	
		tokenizer::iterator mspawnit = mastertokens.begin();
		player->masterPos.x = atoi(mspawnit->c_str()); mspawnit++;
		player->masterPos.y = atoi(mspawnit->c_str()); mspawnit++;
		player->masterPos.z = atoi(mspawnit->c_str());
	
	
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
	
#else
	///////////////////////////////
	mysqlpp::Connection con;
	try{
		con.connect(db.c_str(), host.c_str(), user.c_str(), pass.c_str()); 
	}
	catch(mysqlpp::BadQuery e){
		std::cout << "MYSQL-ERROR: " << e.error << std::endl;
		return false;
	}

	mysqlpp::Result res;
	try{
		mysqlpp::Query query = con.query();
		query << "SELECT * FROM players WHERE name ='" << mysqlpp::escape << name << "'";
#ifdef __DEBUG__
		std::cout << query.preview() << std::endl;
#endif		
		res = query.store();
	}
	catch(mysqlpp::BadQuery e){
		std::cout << "MYSQL-ERROR: " << e.error << std::endl;
		return false;
	}

	std::cout << "found " << res.size() << " chars" << std::endl;

	//FIXME: why doesnt this work?
	if(res.size() != 1)
		return false;

	mysqlpp::Row row = *(res.begin());

	int accno = row.lookup_by_name("account");
	Account a = IOAccount::instance()->loadAccount(accno);
		
	player->password = a.password;
	if (a.accnumber == 0 || a.accnumber != accno) {
		  return false;
	}

	//player->setID(row.lookup_by_name("id"));
	player->setGUID(row.lookup_by_name("id"));
		
	player->accountNumber = row.lookup_by_name("account");
	player->sex=row.lookup_by_name("sex");
	
	player->setDirection((Direction)(int)row.lookup_by_name("direction"));
	player->experience=row.lookup_by_name("experience");
	player->level=row.lookup_by_name("level");
	player->level_percent  = (unsigned char)(100*(player->experience-player->getExpForLv(player->level))/(1.*player->getExpForLv(player->level+1)-player->getExpForLv(player->level)));
	player->capacity = row.lookup_by_name("cap");
	player->max_depot_items = row.lookup_by_name("maxdepotitems");

	player->voc=row.lookup_by_name("vocation");
	player->access=row.lookup_by_name("access");
	player->setNormalSpeed();

	player->mana=row.lookup_by_name("mana");
	player->manamax=row.lookup_by_name("manamax");
	player->manaspent=row.lookup_by_name("manaspent");
	player->maglevel=row.lookup_by_name("maglevel");
	player->maglevel_percent  = (unsigned char)(100*(player->manaspent/(1.*player->getReqMana(player->maglevel+1, player->vocation))));

	player->health=row.lookup_by_name("health");
	player->healthmax=row.lookup_by_name("healthmax");
	player->food=row.lookup_by_name("food");

	player->looktype=row.lookup_by_name("looktype");
	player->lookmaster = player->looktype;
	player->lookhead=row.lookup_by_name("lookhead");
	player->lookbody=row.lookup_by_name("lookbody");
	player->looklegs=row.lookup_by_name("looklegs");
	player->lookfeet=row.lookup_by_name("lookfeet");

	boost::char_separator<char> sep(";");

	//SPAWN
	std::string pos = std::string(row.lookup_by_name("pos"));
	//std::cout << pos << std::endl;
	tokenizer tokens(pos, sep);

	tokenizer::iterator spawnit = tokens.begin();
	player->pos.x=atoi(spawnit->c_str()); spawnit++;
	player->pos.y=atoi(spawnit->c_str()); spawnit++;
	player->pos.z=atoi(spawnit->c_str());

	//there is no "fuck" in the sources, but every major programm got
	//one and i think here is a good place to add one

	//std::cout << "sql loading: " << player->pos << std::endl;

	//MASTERSPAWN
	std::string masterpos = std::string(row.lookup_by_name("masterpos"));
	tokenizer mastertokens(masterpos, sep);

	tokenizer::iterator mspawnit = mastertokens.begin();
	player->masterPos.x=atoi(mspawnit->c_str()); mspawnit++;
	player->masterPos.y=atoi(mspawnit->c_str()); mspawnit++;
	player->masterPos.z=atoi(mspawnit->c_str());


	// we need to find out our skills
	// so we query the skill table

	try{
		mysqlpp::Query query = con.query();
		query << "SELECT * FROM skills WHERE player =" << player->getGUID();
#ifdef __DEBUG__
		std::cout << query.preview() << std::endl;
#endif		
		res = query.store();
	}
	catch(mysqlpp::BadQuery e){
		std::cout << "MYSQL-ERROR: " << e.error << std::endl;
		return false;
	}
	
	//now iterate over the skills
	for(mysqlpp::Result::iterator i = res.begin(); i != res.end(); i++){
		mysqlpp::Row r = *i;
		int skillid=r.lookup_by_name("id");
		player->skills[skillid][SKILL_LEVEL]=r.lookup_by_name("skill");
		player->skills[skillid][SKILL_TRIES]=r.lookup_by_name("tries");
		player->skills[skillid][SKILL_PERCENT] = (unsigned int)(100*(player->skills[skillid][SKILL_TRIES])/(1.*player->getReqSkilltries(skillid, (player->skills[skillid][SKILL_LEVEL]+1), player->voc)));
	}

	//load the items
	try{
		mysqlpp::Query query = con.query();
		query << "SELECT * FROM items WHERE player =" << player->getGUID() << " ORDER BY sid ASC";
#ifdef __DEBUG__
		std::cout << query.preview() << std::endl;
#endif
		res = query.store();
	}
	catch(mysqlpp::BadQuery e){
		std::cout << "MYSQL-ERROR: " << e.error << std::endl;
		return false;
	}

    // cross compatibility vc++ and gcc
    #ifdef __GNUC__
	__gnu_cxx::hash_map<int,std::pair<Item*,int> > itemmap;
	#else
	stdext::hash_map<int,std::pair<Item*,int> > itemmap;
	#endif
	try{
		for(mysqlpp::Result::iterator i = res.begin(); i != res.end(); i++){
			mysqlpp::Row r = *i;
			int type = r.lookup_by_name("type");
			int count = r.lookup_by_name("number");
			Item* myItem = Item::CreateItem(type, count);
			if((int)r.lookup_by_name("actionid") >= 100)
				myItem->setActionId((int)r.lookup_by_name("actionid"));
			myItem->setText(r.lookup_by_name("text").get_string());	
			myItem->setSpecialDescription(r.lookup_by_name("specialdesc").get_string());
			std::pair<Item*, int> myPair(myItem, r.lookup_by_name("pid"));
			itemmap[r.lookup_by_name("sid")] = myPair;
			if(int slotid = r.lookup_by_name("slot")){
				if(slotid > 0 && slotid <= 10){
					player->addItemInventory(myItem, slotid,true);
				}
				else{
					if(dynamic_cast<Container*>(myItem)){
						player->addDepot(dynamic_cast<Container*>(myItem), slotid - 100);
					}
					else{
						std::cout << "Error loading depot "<< slotid << " for player " << 
							player->getGUID() << std::endl;
					}
				}
			}
		}
	}
	catch(std::exception er){
		std::cout << "damn crap" << std::endl;
	}
	// cross compatibility vc++ and gcc
    #ifdef __GNUC__
	__gnu_cxx::hash_map<int,std::pair<Item*,int> >::iterator it;
	#else
	stdext::hash_map<int,std::pair<Item*,int> >::iterator it;
	#endif
	for(int i = (int)itemmap.size(); i > 0; --i) {
		it = itemmap.find(i);
		if(it == itemmap.end())
			continue;

		if(int p=(*it).second.second) {
			if(Container* c = dynamic_cast<Container*>(itemmap[p].first)) {
				c->addItem((*it).second.first);
			}
		}
	}
	//load storage map
	try{
		mysqlpp::Query query = con.query();
		query << "SELECT * FROM playerstorage WHERE player =" << player->getGUID();
#ifdef __DEBUG__
		std::cout << query.preview() << std::endl;
#endif
		res = query.store();
	}
	catch(mysqlpp::BadQuery e){
		std::cout << "MYSQL-ERROR: " << e.error << std::endl;
		return false;
	}
	for(mysqlpp::Result::iterator i = res.begin(); i != res.end(); i++){
		mysqlpp::Row r = *i;
		unsigned long key = r.lookup_by_name("key");
		long value = r.lookup_by_name("value");
		player->addStorageValue(key,value);
	}

#endif // _OLD_MYSQL_

	return true;
}

bool IOPlayerSQL::savePlayer(Player* player)
{
	//return true;
	std::string host = g_config.getGlobalString("sql_host");
	std::string user = g_config.getGlobalString("sql_user");
	std::string pass = g_config.getGlobalString("sql_pass");
	std::string db   = g_config.getGlobalString("sql_db");

	player->preSave();
	
#ifndef _OLD_MYSQL_
	
	//try
	//{
		Database mysql;
		DBQuery query;
		DBResult result;
		
		mysql.connect(db.c_str(), host.c_str(), user.c_str(), pass.c_str());
		
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
#else
	//////////////////////////////////////////////////////
	mysqlpp::Connection con;
	try{
		con.connect(db.c_str(), host.c_str(), user.c_str(), pass.c_str()); 
	}
	catch(mysqlpp::BadQuery e){
		std::cout << "MYSQL-ERROR: " << e.error << std::endl;
		return false;
	}
	catch(...){
		std::cout << "ERROR donde peta siempre." << std::endl;
		return false;
	}

	mysqlpp::Query query = con.query();
	
	//check if the player have to be saved or not
	query << "SELECT save FROM players WHERE `id` = " << player->getGUID();
#ifdef __DEBUG__
	std::cout << query.preview() << std::endl;
#endif
	int save = 0;
	try{
		mysqlpp::Result res = query.store();
		if(res.size() != 1)
			return false;
		mysqlpp::Row row = *(res.begin());
		save = row.lookup_by_name("save");
	}
	catch(mysqlpp::BadQuery e){
		std::cout << "MYSQL-ERROR: " << e.error << std::endl;
		return false;
	}
	
	if(save != 1)
		return true;
	
	//Start the transaction	
	query.reset();
	query << "BEGIN;";
	try{
		query.execute();
	}
	catch(mysqlpp::BadQuery e){
		std::cout << "MYSQL-ERROR: " << e.error << std::endl;
		return false;
	}

	//First, an UPDATE query to write the player itself

	query.reset();

	query << "UPDATE `players` SET ";
	query << "`level` = " << player->level << ", ";
	query << "`vocation` = " << player->voc << ", ";
	query << "`health` = " << player->health << ", ";
	query << "`healthmax` = " << player->healthmax << ", ";
	query << "`direction` = " << (int)player->getDirection() << ", ";
	query << "`experience` = " << player->experience << ", ";
	query << "`soul` = " << (unsigned short) player->soul << ", "; //probably needed for no number to char conversion
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
	query << "`lastlogin` = " << player->lastlogin << " ";
	query << " WHERE `id` = "<< player->getGUID() <<" LIMIT 1";
#ifdef __DEBUG__
	std::cout << query.preview() << std::endl;
#endif
	try{
		query.execute();
	}
	catch(mysqlpp::BadQuery e){
		std::cout << "MYSQL-ERROR: " << e.error << std::endl;
		return false;
	}


	//then we write the individual skills
	/*query.reset();
	query << "DELETE FROM skills WHERE player="<< player->getGUID();
#ifdef __DEBUG__
	std::cout << query.preview() << std::endl;
#endif
	query.execute();

	query.reset();
	query << "INSERT INTO `skills` ( `player` , `id`, `skill` , `tries` ) VALUES";
	for(int i = 0; i <= 6; i++){
		query << "("<<player->getGUID()<<","<<i<<","<<player->skills[i][SKILL_LEVEL]<<","<<player->skills[i][SKILL_TRIES]<<")";
		if(i!=6)
			query<<",";
	}
#ifdef __DEBUG__
	std::cout << query.preview() << std::endl;
#endif
	query.execute();
*/
	for(int i = 0; i <= 6; i++){
		query.reset();
		query << "UPDATE `skills` SET `skill` = " << player->skills[i][SKILL_LEVEL] <<", `tries` = "<<
		   player->skills[i][SKILL_TRIES] << " WHERE `player` = " << player->getGUID() << " AND  `id` = " << i;
#ifdef __DEBUG__
		std::cout << query.preview() << std::endl;
#endif
		try{
			query.execute();
		}
			catch(mysqlpp::BadQuery e){
			std::cout << "MYSQL-ERROR: " << e.error << std::endl;
			return false;
		}
	}

	//now item saving
	query.reset();
	query << "DELETE FROM items WHERE player="<< player->getGUID();
#ifdef __DEBUG__
	std::cout << query.preview() << std::endl;
#endif
	try{
		query.execute();
	}
	catch(mysqlpp::BadQuery e){
		std::cout << "MYSQL-ERROR: " << e.error << std::endl;
		return false;
	}
	query.reset();
	std::string itemsstring;
	query << "INSERT INTO `items` (`player` , `slot` , `sid` , `pid` , `type` , `number` , `actionid` , `text` , `specialdesc` ) VALUES"; 
	int runningID=0;
	for (int i = 1; i <= 10; i++){
		if(player->items[i])
			itemsstring += getItems(player->items[i],runningID,i,player->getGUID(),0);
	}
	//save depot items
	for(DepotMap::reverse_iterator it = player->depots.rbegin(); it !=player->depots.rend() ;++it){
    	itemsstring += getItems(it->second,runningID,it->first+100,player->getGUID(),0);
	}
      
	if(itemsstring.length()){
		itemsstring.erase(itemsstring.length()-1);
		query << itemsstring;
#ifdef __DEBUG__
		std::cout << query.preview() << std::endl;
#endif
		try{
			query.execute();
		}
		catch(mysqlpp::BadQuery e){
			std::cout << "MYSQL-ERROR: " << e.error << std::endl;
			return false;
		}
	}
	//save storage map
	query.reset();
	query << "DELETE FROM playerstorage WHERE player="<< player->getGUID();
#ifdef __DEBUG__
	std::cout << query.preview() << std::endl;
#endif
	try{
		query.execute();
	}
	catch(mysqlpp::BadQuery e){
		std::cout << "MYSQL-ERROR: " << e.error << std::endl;
		return false;
	}
	
	query.reset();
	query << "INSERT INTO `playerstorage` (`player` , `key` , `value` ) VALUES"; 
	std::stringstream ss;
	for(StorageMap::const_iterator cit = player->getStorageIteratorBegin(); cit != player->getStorageIteratorEnd();cit++){
    	ss << "(" << player->getGUID() <<","<< cit->first <<","<< cit->second<<"),";
	}
	std::string ststring = ss.str();
	if(ststring.length()){
		ststring.erase(ststring.length()-1);
		query << ststring;
#ifdef __DEBUG__
		std::cout << query.preview() << std::endl;
#endif
		try{
			query.execute();
		}
		catch(mysqlpp::BadQuery e){
			std::cout << "MYSQL-ERROR: " << e.error << std::endl;
			return false;
		}
	}
	
	//End the transaction
	query.reset();
	query << "COMMIT;";
#ifdef __DEBUG__
	std::cout << query.preview() << std::endl;
#endif
	try{
		query.execute();
	}
	catch(mysqlpp::BadQuery e){
		std::cout << "MYSQL-ERROR: " << e.error << std::endl;
		return false;
	}
#endif // _OLD_MYSQL_
	return true;

}

std::string IOPlayerSQL::getItems(Item* i, int &startid, int slot, int player,int parentid)
{
	++startid;
	std::stringstream ss;
#ifndef _OLD_MYSQL_
	ss << "(" << player <<"," << slot << ","<< startid <<","<< parentid <<"," << i->getID()<<","<< (int)i->getItemCountOrSubtype() << "," << 
	(int)i->getActionId()<<",'"<< Database::escapeString(i->getText()) <<"','" << Database::escapeString(i->getSpecialDescription()) <<"'),";
#else
	ss << "(" << player <<"," << slot << ","<< startid <<","<< parentid <<"," << i->getID()<<","<< (int)i->getItemCountOrSubtype() << "," << 
	(int)i->getActionId()<<",'"<< mysqlpp::escape << i->getText() <<"','" << mysqlpp::escape << i->getSpecialDescription() <<"'),";
#endif // _OLD_MYSQL_
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

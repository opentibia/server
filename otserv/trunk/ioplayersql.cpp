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

#include <mysql++.h>
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


	mysqlpp::Connection con;
	con.connect(db.c_str(), host.c_str(), user.c_str(), pass.c_str()); 
	mysqlpp::Result res;
	try{
		mysqlpp::Query query = con.query();
		query << "SELECT * FROM players WHERE name ='" << name << "'";
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
	player->maglevel=row.lookup_by_name("maglevel");
	

	player->voc=row.lookup_by_name("vocation");
	player->access=row.lookup_by_name("access");
	player->setNormalSpeed();

	player->mana=row.lookup_by_name("mana");
	player->manamax=row.lookup_by_name("manamax");
	player->manaspent=row.lookup_by_name("manaspent");

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
	std::cout << pos << std::endl;
	tokenizer tokens(pos, sep);

	tokenizer::iterator spawnit = tokens.begin();
	player->pos.x=atoi(spawnit->c_str()); spawnit++;
	player->pos.y=atoi(spawnit->c_str()); spawnit++;
	player->pos.z=atoi(spawnit->c_str());

	//there is no "fuck" in the sources, but every major programm got
	//one and i think here is a good place to add one

	std::cout << "sql loading: " << player->pos << std::endl;

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
		query << "SELECT * FROM skills WHERE player ='" << player->getGUID() << "'";
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
	}

	//load the items
	try{
		mysqlpp::Query query = con.query();
		query << "SELECT * FROM items WHERE player ='" << player->getGUID() << "'" << " ORDER BY sid ASC";
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
			std::pair<Item*, int> myPair(myItem, r.lookup_by_name("pid"));
			itemmap[r.lookup_by_name("sid")] = myPair;
			if(int slotid = r.lookup_by_name("slot"))
				player->addItemInventory(myItem, slotid,true);
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
	return true;
	
}

bool IOPlayerSQL::savePlayer(Player* player){
	std::string host = g_config.getGlobalString("sql_host");
	std::string user = g_config.getGlobalString("sql_user");
	std::string pass = g_config.getGlobalString("sql_pass");
	std::string db   = g_config.getGlobalString("sql_db");

	player->preSave();

	mysqlpp::Connection con;
	con.connect(db.c_str(), host.c_str(), user.c_str(), pass.c_str()); 
	mysqlpp::Result res;

	//Start the transaction
	mysqlpp::Query query = con.query();
	query << "BEGIN;";
	query.execute();

	//First, an UPDATE query to write the player itself

	query.reset();

	query << "UPDATE `players` SET ";
	query << "`level` = " << player->level << ", ";
	query << "`vocation` = " << player->voc << ", ";
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
	query << "`cap` = " << player->cap << ", ";
	query << "`food` = " << player->food << ", ";
	query << "`sex` = " << player->sex << " ";
	query << " WHERE `id` = "<< player->getGUID() <<" LIMIT 1";
#ifdef __DEBUG__
	std::cout << query.preview() << std::endl;
#endif
	query.execute();


	//then we write the individual skills
	query.reset();
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

	//now item saving
	query.reset();
	query << "DELETE FROM items WHERE player="<< player->getGUID();
#ifdef __DEBUG__
	std::cout << query.preview() << std::endl;
#endif
	query.execute();
	query.reset();
	std::string itemsstring;
	query << "INSERT INTO `items` (`player` , `slot` , `sid` , `pid` , `type` , `number` ) VALUES"; 
	int runningID=0;
	for (int i = 1; i <= 10; i++){
		if(player->items[i])
			itemsstring += getItems(player->items[i],runningID,i,player->getGUID(),0);
	}
	if(itemsstring.length()){
		itemsstring.erase(itemsstring.length()-1);
		query << itemsstring;
#ifdef __DEBUG__
		std::cout << query.preview() << std::endl;
#endif
		query.execute();
	}
	
	//End the transaction
	query.reset();
	query << "COMMIT;";
#ifdef __DEBUG__
	std::cout << query.preview() << std::endl;
#endif
	query.execute();

	return true;

}

std::string IOPlayerSQL::getItems(Item* i, int &startid, int slot, int player,int parentid){
	++startid;
	std::stringstream ss;
	ss << "(" << player <<"," << slot << ","<< startid <<","<< parentid <<"," << i->getID()<<","<< (int)i->getItemCountOrSubtype() <<"),";
	std::cout << "i";
	if(Container* c = dynamic_cast<Container*>(i)){
		std::cout << "c";	
		int pid = startid;
		for(ContainerList::const_iterator it = c->getItems(); it != c->getEnd(); it++){
			std::cout << "s";
			ss << getItems(*it, startid, 0, player, pid);
			std::cout << "r";
		}
	}
	return ss.str();
}

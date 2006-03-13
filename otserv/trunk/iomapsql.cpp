#include "iomapsql.h"

#include <iostream>
#include <algorithm>
#include <functional>
#include <sstream>

// cross compatibility vc++ and gcc
#ifdef __GNUC__
#include <ext/hash_map>
#else
#include <hash_map>
#endif

#include "luascript.h"
#include <boost/tokenizer.hpp>
typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
extern LuaScript g_config;

/*
bool IOMapSQL::loadMap(Map* map, std::string identifier){
  	std::string host = g_config.getGlobalString("map_host");
	std::string user = g_config.getGlobalString("map_user");
	std::string pass = g_config.getGlobalString("map_pass");
	std::string db   = g_config.getGlobalString("map_db");
#ifdef __DEBUG__
	std::cout << "host" << host << "user" << user << "pass" << pass << "db" << db << std::endl;
#endif     
    mysqlpp::Connection con;
try{
	con.connect(db.c_str(), host.c_str(), user.c_str(), pass.c_str()); 
}
catch(mysqlpp::BadQuery e){
	std::cout << "MYSQL-ERROR: " << e.error << std::endl;
	return false;
}
	mysqlpp::Result res;
	
//Try & Find the Map	
try{
     mysqlpp::Query query = con.query();
	 query << "SELECT * FROM maps WHERE map ='" << identifier << "'";
	 
#ifdef __DEBUG__
		std::cout << query.preview() << std::endl;
#endif	
	
	 res = query.store();
}//End Try
catch(mysqlpp::BadQuery e){
	std::cout << "MYSQL-ERROR: " << e.error << std::endl;
	return false;
}

	std::cout << ":: Found: " << res.size() << " map(s)" << std::endl;
	if(res.size() > 1){//More then 1 map with this name
       std::cout << "FATAL: Too Many Maps!" << std::endl;
	   exit(1);
    }
    else if(res.size() < 1){//No Map by this Name
       std::cout << "FATAL: No Map Found" << std::endl;
	   exit(1);
    }
//Load Map Info
try{
        mysqlpp::Query query = con.query();
		query << "SELECT * FROM maps WHERE map ='" << identifier << "'";
		mysqlpp::Row row = *res.begin();
		std::string name = std::string(row.lookup_by_name("map"));
		std::string author = std::string(row.lookup_by_name("author"));
	    std::string size = std::string(row.lookup_by_name("size"));
	    std::string temple = std::string(row.lookup_by_name("temple"));	    
	    //Display Map Info in Command Window
	    std::cout << ":: Loading: " << name << " table; Made by: " << author << " Size: " << size << std::endl;
	    std::cout << ":: Temple: " << temple << std::endl;
		
#ifdef __DEBUG__
		std::cout << query.preview() << std::endl;
#endif		
		res = query.store();
}//End Try
catch(mysqlpp::BadQuery e){
	std::cout << "MYSQL-ERROR: " << e.error << std::endl;
	return false;
}//End Catch

//Load Tiles
try{
        mysqlpp::Result tile;
        mysqlpp::Query query = con.query();
		query << "SELECT * FROM " << identifier << " WHERE tile != ''";
#ifdef __DEBUG__
		std::cout << query.preview() << std::endl;
#endif		
		res = query.store(); 
		//How Many Tiles?
        std::cout << ":: Found: " << res.size() << " tile(s)" << std::endl;
        int x = 0;
        int y = 0;
        int z = 0;
        int gid = 0;
        int pz = 0;
        int save = 0;
        for(int i=1; i <= res.size(); ++i){
          query.reset();
          query << "SELECT * FROM " << identifier << " WHERE tile = '" << i <<"' and tile != ''";
          tile = query.store();
          mysqlpp::Row row = *tile.begin();          
          //Get the Ground's Position on Map
          if(std::string(row.lookup_by_name("pos")) != ""){
          std::string pos = std::string(row.lookup_by_name("pos"));
          boost::char_separator<char> sep(";");
          tokenizer postokens(pos, sep);
          tokenizer::iterator posit = postokens.begin();
	      x=atoi(posit->c_str()); posit++;
	      y=atoi(posit->c_str()); posit++;
	      z=atoi(posit->c_str());
          }
          gid = row.lookup_by_name("id");
          pz = row.lookup_by_name("pz");
          save = row.lookup_by_name("save");
          if(std::string(row.lookup_by_name("house")) != ""){house = std::string(row.lookup_by_name("house"));}

//#ifdef __DEBUG__
//			if(gid == 0) {std::cout << "No ground tile! x: " << x << ", y: " << y << " z: " << z << std::endl;}
//#endif

          map->setTile(x,y,z,gid);
          Tile *t;
          t = map->getTile(x,y,z);

					//switch(save){
					//	case 1:
          //  t->setSave();
          //  break;
          //  
          //  default:
          //  break;
          //}//Switch
          
          switch(pz){
            case 1:
             t->setPz();
            break;
            
						//case 2:
            // t->setPzR();
            //break;
            
            default:
            break;
          }//Switch 
        }//End For Loop
}//End Try
catch(mysqlpp::BadQuery e){
	std::cout << "MYSQL-ERROR: " << e.error << std::endl;
	return false;
}//End Catch

//Load Items
try{
    Tile *t;
    mysqlpp::Result item;
    mysqlpp::Query query = con.query();
    query << "SELECT * FROM " << identifier << "_items";
    res = query.store();
    //How many Items?
    std::cout << ":: Found: " << res.size() << " item(s)" << std::endl;

    query.reset();
    query << "SELECT * FROM test_map_items ORDER BY sid ASC";

#ifdef __GNUC__
	__gnu_cxx::hash_map<int,std::pair<Item*,int> > itemmap;
#else
	stdext::hash_map<int,std::pair<Item*,int> > itemmap;
#endif
	for(mysqlpp::Result::iterator i = res.begin(); i != res.end(); i++){
	    mysqlpp::Row r = *i;
		int id = r.lookup_by_name("item");
	    int depot = r.lookup_by_name("depot");
		int count = r.lookup_by_name("number");
		int pid = r.lookup_by_name("pid");
		Item* myItem = Item::CreateItem(id, count);
		if((int)r.lookup_by_name("actionid") >= 100)
  	       myItem->setActionId((int)r.lookup_by_name("actionid"));
  	    if((int)r.lookup_by_name("uniqueid") >= 100)
  	       myItem->setActionId((int)r.lookup_by_name("uniqueid"));
  	    myItem->setText(r.lookup_by_name("text").get_string());
  	    myItem->setSpecialDescription(r.lookup_by_name("specialdesc").get_string());
		std::pair<Item*, int> myPair(myItem, r.lookup_by_name("pid"));
		itemmap[r.lookup_by_name("sid")] = myPair;
		std::string pos = std::string(r.lookup_by_name("pos"));
        int x;
        int y;
        int z;
        if(pos != "" && pid == 0){
           boost::char_separator<char> sep(";");
           tokenizer postokens(pos, sep);
           tokenizer::iterator posit = postokens.begin();
	       x=atoi(posit->c_str()); posit++;
	       y=atoi(posit->c_str()); posit++;
	       z=atoi(posit->c_str());
	       t = map->getTile(x,y,z);
	       if(t){
              Container *container = dynamic_cast<Container*>(myItem);
              if(depot != 0){
                 container->depotId = depot;
              }
              myItem->pos.x = x;
		      myItem->pos.y = y;
		      myItem->pos.z = z;
              if (myItem->isAlwaysOnTop())
		         t->topItems.push_back(myItem);
		      else
			     t->downItems.push_back(myItem);
           }//if(t)
        }//if(pos != "")
    }//End For
  	    
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
	}//End For
}//End Try
catch(mysqlpp::BadQuery e){
	std::cout << "MYSQL-ERROR: " << e.error << std::endl;
	return false;
}//End Catch

       return true;
}//Function End



//Save Maps
//For all tiles that: save = true
//Delete all from pos = save tiles pos
//Search by Sid and get # of Sid's
//Save Items starting SID @ 1 + SID Size
//Not Yet Implemented!
bool IOMapSQL::SaveMap(Map* map, std::string identifier, int x, int y, int z){
  Tile *t;
   t = map->getTile(x,y,z);
    std::stringstream pos;
       pos << x << ";" << y << ";" << z;
    std::string host = g_config.getGlobalString("map_host");
	std::string user = g_config.getGlobalString("map_user");
	std::string pass = g_config.getGlobalString("map_pass");
	std::string db   = g_config.getGlobalString("map_db");
#ifdef __DEBUG__
	std::cout "host" << host << "user" << user << "pass" << pass << "db" << db << std::endl;
#endif     
    mysqlpp::Connection con;
try{
	con.connect(db.c_str(), host.c_str(), user.c_str(), pass.c_str()); 
}
catch(mysqlpp::BadQuery e){
	std::cout << "MYSQL-ERROR: " << e.error << std::endl;
	return false;
}
	mysqlpp::Result res;
    mysqlpp::Query query = con.query();
    query << "DELETE FROM " << identifier << " WHERE pos= '"<< pos << "`";
#ifdef __DEBUG__
	std::cout << query.preview() << std::endl;
#endif
	query.execute();
	
    query.reset();
    
    query << "SELECT * FROM " << identifier << "_items";
    res = query.store();
   
   std::string itemsstring;
	query << "INSERT INTO `test_map_items` (`pos` , `item` , `depot` , `actionid` , `uniqueid` , `sid` , `pid` , `number` , `text` , `specialdesc` ) VALUES"; 
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
std::string IOMapSQL::getItems(Item* i, int &startid, int slot, int player,int parentid){
	++startid;
	std::stringstream ss;
	ss << "(" << player <<"," << slot << ","<< startid <<","<< parentid <<"," << i->getID()<<","<< (int)i->getItemCountOrSubtype() << "," << 
	(int)i->getActionId()<<",'"<< i->getText() <<"','" << i->getSpecialDescription() <<"'),";
	//std::cout << "i";
	if(Container* c = dynamic_cast<Container*>(i)){
		//std::cout << "c";	
		int pid = startid;
		for(ItemList::const_iterator it = c->getItems(); it != c->getEnd(); it++){
			//std::cout << "s";
			ss << getItems(*it, startid, 0, player, pid);
			//std::cout << "r";
		}
	}
	return ss.str();
	return 0;
}
*/

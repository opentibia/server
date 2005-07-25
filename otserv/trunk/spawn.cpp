//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
//
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

#include "spawn.h"
#include "game.h"
#include "player.h"
#include "npc.h"

#ifdef _SQLMAP_
#include <mysql++.h>
#include <boost/tokenizer.hpp>
typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
#endif

#include <libxml/xmlmemory.h>
#include <libxml/parser.h> 

extern LuaScript g_config;

SpawnManager* SpawnManager::_instance = NULL;
Game* SpawnManager::game = NULL;
std::vector<Spawn*> SpawnManager::spawns;

SpawnManager::SpawnManager()
{
	//
}

SpawnManager::~SpawnManager()
{
	for(std::vector<Spawn*>::iterator it = spawns.begin(); it != spawns.end(); ++it)
		delete *it;

	spawns.clear();
}

bool SpawnManager::initialize(Game *igame)
{
	game = igame;
	_instance = new SpawnManager();
	return (_instance != NULL);
}

SpawnManager* SpawnManager::instance() {
	return _instance;
}

bool SpawnManager::loadSpawnsXML(std::string filename)
{
	std::transform(filename.begin(), filename.end(), filename.begin(), tolower);
	xmlDocPtr doc = xmlParseFile(filename.c_str());

	if (doc){
		xmlNodePtr root, p;
		root = xmlDocGetRootElement(doc);
		
		root = xmlDocGetRootElement(doc);
		
		if (xmlStrcmp(root->name,(const xmlChar*) "spawns")){			
			xmlFreeDoc(doc);
			return false;
		}

		p = root->children;
            
		while (p) {
			const char* str = (char*)p->name;
			
			if (strcmp(str, "spawn") == 0) {
				Position centerpos;
				int radius;

				if((const char*)xmlGetProp(p, (const xmlChar *)"centerx"))
					centerpos.x = atoi((const char*)xmlGetProp(p, (const xmlChar *)"centerx"));
				else
					return false;

				if((const char*)xmlGetProp(p, (const xmlChar *)"centery"))
					centerpos.y = atoi((const char*)xmlGetProp(p, (const xmlChar *)"centery"));
				else
					return false;

				if((const char*)xmlGetProp(p, (const xmlChar *)"centerz"))
					centerpos.z = atoi((const char*)xmlGetProp(p, (const xmlChar *)"centerz"));
				else
					return false;

				if((const char*)xmlGetProp(p, (const xmlChar *)"radius"))
					radius = atoi((const char*)xmlGetProp(p, (const xmlChar *)"radius"));
				else
					return false;

				Spawn *spawn = new Spawn(game, centerpos, radius);
				spawns.push_back(spawn);

				std::string name;
				int x, y, spawntime;
				Direction direction = NORTH;
				int rawdir = 0; //NORTH

				xmlNodePtr tmp = p->children;
				while (tmp) {
					str = (char*)tmp->name;
					if (strcmp(str, "monster") == 0) {
						if((const char*)xmlGetProp(tmp, (const xmlChar *)"name")) {
							name = (const char*)xmlGetProp(tmp, (const xmlChar *)"name");
						}
						else {
							tmp = tmp->next;
							break;
						}

						if((const char*)xmlGetProp(tmp, (const xmlChar *)"direction"))
							rawdir = atoi((const char*)xmlGetProp(tmp, (const xmlChar *)"direction"));

						if((const char*)xmlGetProp(tmp, (const xmlChar *)"x"))
							x = atoi((const char*)xmlGetProp(tmp, (const xmlChar *)"x"));
						else {
							tmp = tmp->next;
							break;
						}

						if((const char*)xmlGetProp(tmp, (const xmlChar *)"y"))
							y = atoi((const char*)xmlGetProp(tmp, (const xmlChar *)"y"));
						else {
							tmp = tmp->next;
							break;
						}

						if((const char*)xmlGetProp(tmp, (const xmlChar *)"spawntime"))
							spawntime = atoi((const char*)xmlGetProp(tmp, (const xmlChar *)"spawntime"));
						else {
							tmp = tmp->next;
							break;
						}

						switch(rawdir) {
							case 0: direction = NORTH; break;
							case 1: direction = EAST; break;
							case 2: direction = SOUTH; break;
							case 3: direction = WEST; break;

							default:
								direction = NORTH;
								break;
						}
						spawn->addMonster(name, direction, x, y, spawntime * 1000);
					}

					tmp = tmp->next;
				}
			}

			p = p->next;
		}

		return true;
	}

	return false;
}

#ifdef _SQLMAP_
bool SpawnManager::loadSpawnsSQL(std::string identifier)
{
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
	
	//Monsters

	//Try & Find the Monter's	
	try{
     mysqlpp::Query query = con.query();
		query << "SELECT * FROM " << identifier << "_monsters WHERE name !=''";
	 
#ifdef __DEBUG__
	std::cout << query.preview() << std::endl;
#endif	
	
	 res = query.store();
	} //End Try
	catch(mysqlpp::BadQuery e){
		std::cout << "MYSQL-ERROR: " << e.error << std::endl;
		return false;
	}

	std::cout << ":: Found: " << res.size() << " Monsters(s)/Spawn(s)" << std::endl;
    if(res.size() < 1){//No Monsters
       std::cout << "No Monsters found" << std::endl;
       return false;
    }
		//if there are any monster spawns to load
    else{
       //Load Monsters
			try{
        mysqlpp::Result Monster;
        mysqlpp::Query query = con.query();

        for(int i=1; i <= res.size(); ++i){
          query.reset();
          query << "SELECT * FROM " << identifier << "_monsters WHERE id = '" << i <<"' and id != ''";
          Monster = query.store();
          mysqlpp::Row row = *Monster.begin();          
          //Get the Monster's Position on Map
          std::string pos = std::string(row.lookup_by_name("spawnpos"));
          boost::char_separator<char> sep(";");
          tokenizer spawnpostokens(pos, sep);
          tokenizer::iterator spawnposit = spawnpostokens.begin();
					Position spawnpos;
          spawnpos.x=atoi(spawnposit->c_str()); spawnposit++;
					spawnpos.y=atoi(spawnposit->c_str()); spawnposit++;
					spawnpos.z=atoi(spawnposit->c_str());
					std::string name;
          if(std::string(row.lookup_by_name("name")) != ""){name = std::string(row.lookup_by_name("name"));}
          int time = row.lookup_by_name("time");

          Spawn *spawn = new Spawn(game, spawnpos, 1);
					spawns.push_back(spawn);
          spawn->addMonster(name, NORTH, 0, 0, time * 1000);
        }//End For Loop
			}//End Try
			catch(mysqlpp::BadQuery e){
				std::cout << "MYSQL-ERROR: " << e.error << std::endl;
				return false;
			}//End Catch    
		}
	
	//NPC's	
	//Try & Find the NPC's	
	try{
		mysqlpp::Query query = con.query();
		query << "SELECT * FROM " << identifier << "_npcs WHERE name !=''";
	 
#ifdef __DEBUG__
		std::cout << query.preview() << std::endl;
#endif	
	
	 res = query.store();
	}//End Try
	catch(mysqlpp::BadQuery e){
		std::cout << "MYSQL-ERROR: " << e.error << std::endl;
		return false;
	}

	std::cout << ":: Found: " << res.size() << " NPC(s)" << std::endl;
    if(res.size() < 1){//No NPC's
       std::cout << "No NPC's found" << std::endl;
       return false;
		}
		//if there are any NPC's to load
    else{
       //Load Monsters
			try{
        mysqlpp::Result Monster;
        mysqlpp::Query query = con.query();

        for(int i=1; i <= res.size(); ++i){
          query.reset();
          query << "SELECT * FROM " << identifier << "_npcs WHERE id = '" << i <<"' and id != ''";
          Monster = query.store();
          mysqlpp::Row row = *Monster.begin();          
          //Get the NPC's Position on Map
          std::string pos = std::string(row.lookup_by_name("pos"));
          boost::char_separator<char> sep(";");
          tokenizer postokens(pos, sep);
          tokenizer::iterator posit = postokens.begin();
					Position npcpos;
          npcpos.x=atoi(posit->c_str()); posit++;
					npcpos.y=atoi(posit->c_str()); posit++;
					npcpos.z=atoi(posit->c_str());
					std::string name;
          if(std::string(row.lookup_by_name("name")) != ""){name = std::string(row.lookup_by_name("name"));}
          int dir = row.lookup_by_name("dir");
          Npc* npc = new Npc(name, game);
          
          npc->pos = npcpos;
          switch(dir){
             case 1:
                npc->direction=(NORTH);
                break;
             
             case 2:
                npc->direction=(SOUTH);
                break;
             
             case 3:
                npc->direction=(WEST);
                break;
             
             case 4:
                npc->direction=(EAST);
                break;
             
             default:
              //  std::cout << "Invalid direction for " << name << "  " <<x<<" "<<y<<" "<<z<<".";
                return false;
                break;
          }
					
					if(!game->placeCreature(npc->pos, npc)){
						delete npc;
					}
				}//End For Loop
        return true;
			}//End Try
			catch(mysqlpp::BadQuery e){
				std::cout << "MYSQL-ERROR: " << e.error << std::endl;
				return false;
			}//End Catch    
    
		}
    return true;
}
#endif

bool SpawnManager::startup()
{
	for(std::vector<Spawn*>::iterator it = spawns.begin(); it != spawns.end(); ++it) {
		(*it)->startup();
	}

	if(!spawns.empty()) {
		game->addEvent(makeTask(20000, std::bind2nd(std::mem_fun(&Game::checkSpawns), 20000)));
	}

	return true;
}

void SpawnManager::checkSpawns(int t)
{
	for(std::vector<Spawn*>::iterator it = spawns.begin(); it != spawns.end(); ++it) {
		(*it)->idle(t);
	}
}

Spawn::Spawn(Game *igame, Position pos, int _radius)
{
	game = igame;
	centerPos = pos;
	radius = _radius;
}

bool Spawn::startup()
{
	for(SpawnMap::iterator sit = spawnmap.begin(); sit != spawnmap.end(); ++sit) {
		respawn(sit->first, sit->second.pos, sit->second.name, sit->second.dir);
	}

	return true;
}

bool Spawn::addMonster(std::string name, Direction dir, int x, int y, int spawntime)
{
	Position tmpPos(centerPos.x + x, centerPos.y, centerPos.z);
	if(!isInSpawnRange(tmpPos)) {
// #ifdef __DEBUG__
		std::cout << "Monster is outside the spawn-area!" << std::endl;
// #endif
		return false;
	}

	struct spawninfo si;
	si.name = name;
	si.dir = dir;
	si.pos.x = centerPos.x + x;
	si.pos.y = centerPos.y + y;
	si.pos.z = centerPos.z;
	si.spawntime = spawntime;
	si.lastspawn = 0;

	unsigned long spawnid = (int)spawnmap.size() + 1;
	spawnmap[spawnid] = si;

	return true;
}

Monster* Spawn::respawn(unsigned long spawnid, Position &pos, std::string &name, Direction dir)
{
	Monster *monster = new Monster(name, game);
	if(monster && monster->isLoaded()) {
		monster->setDirection(dir);
		monster->masterPos = centerPos;

		if(game->placeCreature(pos, monster)) {
			monster->useThing();
			spawnedmap.insert(spawned_pair(spawnid, monster));
			spawnmap[spawnid].lastspawn = OTSYS_TIME();
		}
		else {
			delete monster;
			monster = NULL;
		}
	}

	return monster;
}

bool Spawn::isInSpawnRange(const Position &p)
{
	if ((p.x >= centerPos.x - radius) && (p.x <= centerPos.x + radius) &&
      (p.y >= centerPos.y - radius) && (p.y <= centerPos.y + radius))
    return true;

	return false;
}

void Spawn::idle(int t)
{
	SpawnedMap::iterator it;
	for(it = spawnedmap.begin(); it != spawnedmap.end();) {
		if (it->second->health <= 0) {
			if(it->first != 0) {
				spawnmap[it->first].lastspawn = OTSYS_TIME();
			}
			it->second->releaseThing();
			//delete it->second;
			spawnedmap.erase(it++);
		}
		else if(!isInSpawnRange(it->second->pos) && it->first != 0) {
			spawnedmap.insert(spawned_pair(0, it->second));
			spawnedmap.erase(it++);
		}
		else
			++it;
	}
	
	for(SpawnMap::iterator sit = spawnmap.begin(); sit != spawnmap.end(); ++sit) {

		if(spawnedmap.count(sit->first) == 0) {
			if((OTSYS_TIME() - sit->second.lastspawn) >= sit->second.spawntime) {

				std::vector<Creature*> list;
				game->getSpectators(Range(sit->second.pos, true), list);

				bool playerFound = false;
				for(std::vector<Creature*>::iterator it = list.begin(); it != list.end(); ++it) {
					Player *player = dynamic_cast<Player*>(*it);
					if(player && player->access == 0) {
						playerFound = true;
						break;
					}
				}
				
				if(playerFound) {
					sit->second.lastspawn = OTSYS_TIME();
					continue;
				}

				respawn(sit->first, sit->second.pos, sit->second.name, sit->second.dir);
			}
		}
	}
}

/*
void Spawn::initialize()
{
	for(int x = centerPos.x - radius; x < centerPos.x  + radius; ++x) {
		for(int y = centerPos.y - radius; y < centerPos.y  + radius; ++y) {
			Tile *t = game->getTile(x, y, centerPos.z);

			if(t) {
				t->RegisterTrigger(EVENT_CREATURE_ENTER, this);
				t->RegisterTrigger(EVENT_CREATURE_LEAVE, this);
			}
		}
	}
}
*/

/*
void Spawn::onCreatureEnter(const Creature *creature, const Position &pos)
{
	const Player *player = dynamic_cast<const Player*>(creature);
	if(player && player->access == 0) {
		for(SpawnMap::iterator sit = spawnmap.begin(); sit != spawnmap.end(); ++sit) {
			if(player->CanSee(sit->second.pos.x, sit->second.pos.y, sit->second.pos.z)) {
				sit->second.holdspawn = true;
				//sit->second.curtime = 0;
			}
		}
	}

	//std::cout << "Creature enter spawn: " << creature->getName() << " Pos: " << pos << std::endl;
};

void Spawn::onCreatureLeave(const Creature *creature, const Position &pos)
{
	const Player *player = dynamic_cast<const Player*>(creature);
	if(player && player->access == 0) {
		for(SpawnMap::iterator sit = spawnmap.begin(); sit != spawnmap.end(); ++sit) {
			if(sit->second.holdspawn && !player->CanSee(sit->second.pos.x, sit->second.pos.y, sit->second.pos.z)) {
				sit->second.holdspawn = false;
			}
		}
	}

	//std::cout << "Creature leave spawn: " << creature->getName() << " Pos: " << pos << std::endl;
};
*/

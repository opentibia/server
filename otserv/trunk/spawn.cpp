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

#include <libxml/xmlmemory.h>
#include <libxml/parser.h> 

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

bool SpawnManager::loadSpawns(std::string filename)
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

						spawn->addMonster(name, x, y, spawntime * 1000);
					}

					tmp = tmp->next;
				}
			}

			p = p->next;
		}
	}

	return true;
}

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
		respawn(sit->first, sit->second.pos, sit->second.name);
	}

	return true;
}

bool Spawn::addMonster(std::string name, int x, int y, int spawntime)
{
	struct spawninfo si;
	si.name = name;
	si.pos.x = x;
	si.pos.y = y;
	si.pos.z = centerPos.z;
	si.spawntime = spawntime;
	si.lastspawn = 0;

	unsigned long spawnid = (int)spawnmap.size() + 1;
	spawnmap[spawnid] = si;

	return true;
}

Monster* Spawn::respawn(unsigned long spawnid, Position &pos, std::string &name)
{
	Monster *monster = new Monster(name.c_str(), game);
	monster->masterPos = centerPos;

	if(monster && monster->isLoaded()) {

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

				respawn(sit->first, sit->second.pos, sit->second.name);
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

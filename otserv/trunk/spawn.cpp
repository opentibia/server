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
#include "otpch.h"

#include "spawn.h"
#include "game.h"
#include "player.h"
#include "npc.h"
#include "tools.h"
#include "configmanager.h"
#include "raids.h"

#include <libxml/xmlmemory.h>
#include <libxml/parser.h> 

extern ConfigManager g_config;
extern Monsters g_monsters;
extern Game g_game;

SpawnManager::SpawnManager()
{
	//
}

SpawnManager::~SpawnManager()
{
	for(spawnsList::iterator it = spawns.begin(); it != spawns.end(); ++it)
		delete *it;

	spawns.clear();
}

bool SpawnManager::addSpawn(Spawn* spawn)
{
	spawns.push_back(spawn);
	return true;
}

bool SpawnManager::loadSpawnsXML(std::string filename)
{
	xmlDocPtr doc = xmlParseFile(filename.c_str());

	if(doc){
		xmlNodePtr root, spawnNode;		
		root = xmlDocGetRootElement(doc);
		
		if(xmlStrcmp(root->name,(const xmlChar*)"spawns") != 0){
			xmlFreeDoc(doc);
			return false;
		}

		int intValue;
		std::string strValue;

		spawnNode = root->children;
		while(spawnNode){
			if(xmlStrcmp(spawnNode->name, (const xmlChar*)"spawn") == 0){
				Position centerPos;
				int radius;

				if(readXMLInteger(spawnNode, "centerx", intValue)){
					centerPos.x = intValue;
				}
				else{
					xmlFreeDoc(doc);
					return false;
				}

				if(readXMLInteger(spawnNode, "centery", intValue)){
					centerPos.y = intValue;
				}
				else{
					xmlFreeDoc(doc);
					return false;
				}

				if(readXMLInteger(spawnNode, "centerz", intValue)){
					centerPos.z = intValue;
				}
				else{
					xmlFreeDoc(doc);
					return false;
				}

				if(readXMLInteger(spawnNode, "radius", intValue)){
					radius = intValue;
				}
				else{
					xmlFreeDoc(doc);
					return false;
				}

				Spawn* spawn = new Spawn(centerPos, radius);
				spawns.push_back(spawn);

				std::string name;
				int x, y, spawntime;
				Direction dir = NORTH;
				int rawdir = 0; //NORTH

				xmlNodePtr tmpNode = spawnNode->children;
				while(tmpNode){
					if(xmlStrcmp(tmpNode->name, (const xmlChar*)"monster") == 0){

						if(readXMLString(tmpNode, "name", strValue)){
							name = strValue;
						}
						else{
							tmpNode = tmpNode->next;
							continue;
						}

						if(readXMLInteger(tmpNode, "direction", intValue)){
							rawdir = intValue;
						}

						if(readXMLInteger(tmpNode, "x", intValue)){
							x = intValue;
						}
						else{
							tmpNode = tmpNode->next;
							continue;
						}

						if(readXMLInteger(tmpNode, "y", intValue)){
							y = intValue;
						}
						else{
							tmpNode = tmpNode->next;
							continue;
						}

						if(readXMLInteger(tmpNode, "spawntime", intValue)){
							spawntime = intValue;
						}
						else{
							tmpNode = tmpNode->next;
							continue;
						}

						switch(rawdir){
							case 0: dir = NORTH; break;
							case 1: dir = EAST; break;
							case 2: dir = SOUTH; break;
							case 3: dir = WEST; break;

							default:
								dir = NORTH;
								break;
						}

						spawn->addMonster(name, dir, x, y, spawntime * 1000);
					}
					else if(xmlStrcmp(tmpNode->name, (const xmlChar*)"npc") == 0){

						if(readXMLString(tmpNode, "name", strValue)){
							name = strValue;
						}
						else{
							tmpNode = tmpNode->next;
							continue;
						}
						
						if(readXMLInteger(tmpNode, "direction", intValue)){
							rawdir = intValue;
						}

						if(readXMLInteger(tmpNode, "x", intValue)){
							x = intValue;
						}
						else{
							tmpNode = tmpNode->next;
							continue;
						}

						if(readXMLInteger(tmpNode, "y", intValue)){
							y = intValue;
						}
						else{
							tmpNode = tmpNode->next;
							continue;
						}

						switch(rawdir){
							case 0: dir = NORTH; break;
							case 1: dir = EAST; break;
							case 2: dir = SOUTH; break;
							case 3: dir = WEST; break;

							default:
								dir = NORTH;
								break;
						}
						
						Npc* npc = new Npc(name);
						if(!npc->isLoaded()){
							delete npc;

							tmpNode = tmpNode->next;
							continue;
						}
						
						npc->setDirection(dir);

						Position placePos = centerPos;
						placePos.x += x;
						placePos.y += y;

						// Place the npc
						if(!g_game.placeCreature(placePos, npc)){
							delete npc;

							tmpNode = tmpNode->next;
							continue;
						}
					}

					tmpNode = tmpNode->next;
				}
			}

			spawnNode = spawnNode->next;
		}

		xmlFreeDoc(doc);
		return true;
	}

	return false;
}

bool SpawnManager::startup()
{
	for(spawnsList::iterator it = spawns.begin(); it != spawns.end(); ++it) {
		(*it)->startup();
	}

	if(!spawns.empty()) {
		g_game.addEvent(makeTask(20000, std::bind2nd(std::mem_fun(&Game::checkSpawns), 20000)));
	}
	
	Raids *raids = Raids::getInstance();
	raids->loadFromXml(g_config.getString(ConfigManager::DATA_DIRECTORY) + "raids/raids.xml");
	if(raids->isLoaded()) {
		raids->startup();
	}
	
	return true;
}

void SpawnManager::checkSpawns(int t)
{
	for(spawnsList::iterator it = spawns.begin(); it != spawns.end(); ++it) {
		(*it)->idle(t);
	}
}

Spawn::Spawn(Position pos, int _radius)
{
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

	if(g_monsters.getIdByName(name) == 0){
		std::cout << "[Spawn::addMonster] Can not find " << name << std::endl;
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

Monster* Spawn::respawn(unsigned long spawnid, Position& pos, std::string& name, Direction dir)
{
	Monster* monster = Monster::createMonster(name);
	if(monster){
		monster->setDirection(dir);
		monster->setMasterPos(centerPos);

		if(g_game.placeCreature(pos, monster, true, true)){
			monster->useThing2();
			spawnedmap.insert(spawned_pair(spawnid, monster));
			spawnmap[spawnid].lastspawn = OTSYS_TIME();
			return monster;
		}

		//not loaded, or could not place it on the map
		delete monster;
		monster = NULL;
	}
	else{
		std::cout << "[Spawn::respawn] Can not create monster " << name << std::endl;
	}

	return NULL;
}

bool Spawn::isInSpawnRange(const Position& p)
{
	if((p.x >= centerPos.x - radius) && (p.x <= centerPos.x + radius) &&
      (p.y >= centerPos.y - radius) && (p.y <= centerPos.y + radius))
    return true;

	return false;
}

void Spawn::idle(int t)
{
	SpawnedMap::iterator it;
	for(it = spawnedmap.begin(); it != spawnedmap.end();) {
		if (it->second->isRemoved()) {
			if(it->first != 0) {
				spawnmap[it->first].lastspawn = OTSYS_TIME();
			}

			it->second->releaseThing2();
			spawnedmap.erase(it++);
		}
		else if(!isInSpawnRange(it->second->getPosition()) && it->first != 0) {
			spawnedmap.insert(spawned_pair(0, it->second));
			spawnedmap.erase(it++);
		}
		else
			++it;
	}
	
	for(SpawnMap::iterator sit = spawnmap.begin(); sit != spawnmap.end(); ++sit) {

		if(spawnedmap.count(sit->first) == 0) {
			if((OTSYS_TIME() - sit->second.lastspawn) >= sit->second.spawntime) {

				SpectatorVec list;
				SpectatorVec::iterator it;

				//g_game.getSpectators(Range(sit->second.pos, true), list);
				g_game.getSpectators(list, sit->second.pos, true);

				bool playerFound = false;
				Player* player = NULL;

				for(it = list.begin(); it != list.end(); ++it) {
					if((player = (*it)->getPlayer()) && player->getAccessLevel() == 0){
						playerFound = true;
						break;
					}
				}
				
				if(playerFound){
					sit->second.lastspawn = OTSYS_TIME();
					continue;
				}

				respawn(sit->first, sit->second.pos, sit->second.name, sit->second.dir);
			}
		}
	}
}

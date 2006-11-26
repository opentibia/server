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

#include "raids.h"

#include "game.h"
#include "player.h"
#include "configmanager.h"

#include <sstream>

extern Game g_game;
extern ConfigManager g_config;

Raids* Raids::_instance = NULL;

Raids::Raids()
{
	loaded = false;
	started = false;
	fileName = "";
	running = NULL;
	lastRaidEnd = 0;
}

Raids::~Raids()
{
	clear();
}

Raids* Raids::getInstance()
{
	if(_instance == NULL){
		_instance = new Raids();
	}
	return _instance;
}

bool Raids::loadFromXml(const std::string& fileName)
{
	if(isLoaded()){
		return true;
	}
	
	this->fileName = fileName;
	
	xmlDocPtr doc = xmlParseFile(fileName.c_str());
	
	if(doc){
		xmlNodePtr root, raidNode;
		root = xmlDocGetRootElement(doc);
		
		if(xmlStrcmp(root->name,(const xmlChar*)"raids") != 0){
			std::cout << "[Error] Raids: Wrong root node." << std::endl;
			xmlFreeDoc(doc);
			return false;
		}
		
		int intValue;
		std::string strValue;
		
		raidNode = root->children;
		while(raidNode){
			if(xmlStrcmp(raidNode->name, (const xmlChar*)"raid") == 0){
				std::string name, file;
				uint32_t interval, chance, margin;
				
				if(readXMLString(raidNode, "name", strValue)){
					name = strValue;
				}
				else{
					std::cout << "[Error] Raids: name tag missing for raid." << std::endl;
					raidNode = raidNode->next;
					continue;
				}
				
				if(readXMLString(raidNode, "file", strValue)){
					file = strValue;
				}
				else{
					std::stringstream ss;
					ss << "raids/" << name << ".xml";
					file = ss.str();
					std::cout << "[Warning] Raids: file tag missing for raid " << name << ". Using default: " << file << std::endl;
				}
				
				if(readXMLInteger(raidNode, "chance", intValue)){
					chance = intValue;
				}
				else{
					std::cout << "[Error] Raids: chance tag missing for raid " << name << std::endl;
					raidNode = raidNode->next;
					continue;
				}
				
				if(readXMLInteger(raidNode, "interval", intValue)){
					interval = intValue * 60 * 1000;
				}
				else{
					std::cout << "[Error] Raids: interval tag missing for raid " << name << std::endl;
					raidNode = raidNode->next;
					continue;
				}
				
				if(readXMLInteger(raidNode, "margin", intValue)){
					margin = intValue * 60 * 1000;
				}
				else{
					std::cout << "[Warning] Raids: margin tag missing for raid " << name << std::endl;
					margin = 0;
				}
				
				Raid* newRaid = new Raid(name, chance, interval, margin);
				if(!newRaid){
					xmlFreeDoc(doc);
					return false;
				}
				
				bool ret = newRaid->loadFromXml(g_config.getString(ConfigManager::DATA_DIRECTORY) + "raids/" + file);
				if(!ret){
					std::cout << "[Error] Raids: failed to load raid " << name << std::endl;
					delete newRaid;
				}
				else{
					raidList.push_back(newRaid);
				}				
			}

			raidNode = raidNode->next;
		}
				
		xmlFreeDoc(doc);
		
	}
	else{
		std::cout << "[Error] Raids: Could not load " << fileName << std::endl;
		return false;
	}
	
	loaded = true;
	return true;
}

void Raids::startup()
{	
	if(!isLoaded() || isStarted())
		return;
	
	setLastRaidEnd(OTSYS_TIME());
	
	RaidList::iterator it;
	for(it = raidList.begin(); it != raidList.end(); it++){
		uint32_t eventId = g_game.addEvent(makeTask((*it)->getInterval(), boost::bind(&Raid::checkRaid, (*it))));
		(*it)->setRaidCheckEvent(eventId);
	}
	started = true;
}

void Raids::clear()
{
	RaidList::iterator it;
	for(it = raidList.begin(); it != raidList.end(); ++it){
		delete (*it);
	}
	raidList.clear();
	
	loaded = false;
	started = false;
	fileName = "";
	running = NULL;
	lastRaidEnd = 0;
	
}

void Raids::reload()
{
	std::string file = fileName;
	clear();
	loadFromXml(file);
}

Raid* Raids::getRaidByName(const std::string& name)
{
	RaidList::iterator it;
	for(it = raidList.begin(); it != raidList.end(); it++){
		if(strcasecmp((*it)->getName().c_str(), name.c_str()) == 0){
			return (*it);
		}
	}

	return NULL;
}

Raid::Raid(const std::string& _name, uint32_t _chance, uint32_t _interval, uint32_t _marginTime)
{
	loaded = false;
	name = _name;
	chance = _chance;
	interval = _interval;
	nextEvent = 0;
	state = RAIDSTATE_IDLE;
	margin = _marginTime;
	checkRaidEvent = 0;
	nextEventEvent = 0;
}

Raid::~Raid()
{
	stopEvents();
	
	RaidEventVector::iterator it;
	for(it = raidEvents.begin(); it != raidEvents.end(); it++) {
		delete (*it);
	}
	raidEvents.clear();
}

bool Raid::loadFromXml(const std::string& fileName)
{	
	if(isLoaded()){
		return true;
	}
	
	RaidEventVector temporaryList;
	
	xmlDocPtr doc = xmlParseFile(fileName.c_str());
	
	if(doc){
		xmlNodePtr root, eventNode, monsterNode;
		root = xmlDocGetRootElement(doc);
		
		if(xmlStrcmp(root->name,(const xmlChar*)"raid") != 0){
			std::cout << "[Error] Raids: Wrong root node." << std::endl;
			xmlFreeDoc(doc);
			return false;
		}
		
		int intValue;
		std::string strValue;
		
		eventNode = root->children;
		while(eventNode){
			if(xmlStrcmp(eventNode->name, (const xmlChar*)"announce") == 0){
				std::string message;
				MessageClasses type;
				uint32_t delay;
				
				if(readXMLInteger(eventNode, "delay", intValue)){
					delay = intValue;
				}
				else{
					std::cout << "[Error] Raid(" << fileName << "): delay tag missing for announce event." << std::endl;
					eventNode = eventNode->next;
					continue;
				}
				
				if(readXMLString(eventNode, "message", strValue)){
					message = strValue;
				}
				else{
					std::cout << "[Error] Raid(" << fileName << "): message tag missing for announce event." << std::endl;
					eventNode = eventNode->next;
					continue;
				}
				
				if(readXMLInteger(eventNode, "type", intValue)){
					type = (MessageClasses)intValue;
				}
				else{
					type = MSG_EVENT_ADVANCE;
					std::cout << "[Notice] Raid(" << fileName << "): type tag missing for announce event. Using default: " << (int)type << std::endl;
				}
				
				AnnounceEvent* event = new AnnounceEvent(message, type, delay);
				temporaryList.push_back(event);
			}
			else if(xmlStrcmp(eventNode->name, (const xmlChar*)"singlespawn") == 0){
				std::string name;
				Position pos;
				uint32_t delay;
				
				if(readXMLInteger(eventNode, "delay", intValue)){
					delay = intValue;
				}
				else{
					std::cout << "[Error] Raid(" << fileName << "): delay tag missing for singlespawn event." << std::endl;
					eventNode = eventNode->next;
					continue;
				}
				
				if(readXMLString(eventNode, "name", strValue)){
					name = strValue;
				}
				else{
					std::cout << "[Error] Raid(" << fileName << "): name tag missing for singlespawn event." << std::endl;
					eventNode = eventNode->next;
					continue;
				}
				
				if(readXMLInteger(eventNode, "x", intValue)){
					pos.x = intValue;
				}
				else{
					std::cout << "[Error] Raid(" << fileName << "): x tag missing for singlespawn event." << std::endl;
					eventNode = eventNode->next;
					continue;
				}

				if(readXMLInteger(eventNode, "y", intValue)){
					pos.y = intValue;
				}
				else{
					std::cout << "[Error] Raid(" << fileName << "): y tag missing for singlespawn event." << std::endl;
					eventNode = eventNode->next;
					continue;
				}

				if(readXMLInteger(eventNode, "z", intValue)){
					pos.z = intValue;
				}
				else{
					std::cout << "[Error] Raid(" << fileName << "): z tag missing for singlespawn event." << std::endl;
					eventNode = eventNode->next;
					continue;
				}
				
				SingleSpawnEvent* event = new SingleSpawnEvent(name, pos, delay);
				temporaryList.push_back(event);
			}
			else if(xmlStrcmp(eventNode->name, (const xmlChar*)"areaspawn") == 0){
				Position fromPos, toPos;
				uint32_t delay;
				
				if(readXMLInteger(eventNode, "delay", intValue)){
					delay = intValue;
				}
				else{
					std::cout << "[Error] Raid(" << fileName << "): delay tag missing for areaspawn event." << std::endl;
					eventNode = eventNode->next;
					continue;
				}
				
				
				if(readXMLInteger(eventNode, "fromx", intValue)){
					fromPos.x = intValue;
				}
				else{
					std::cout << "[Error] Raid(" << fileName << "): fromx tag missing for areaspawn event." << std::endl;
					eventNode = eventNode->next;
					continue;
				}
				if(readXMLInteger(eventNode, "fromy", intValue)){
					fromPos.y = intValue;
				}
				else{
					std::cout << "[Error] Raid(" << fileName << "): fromy tag missing for areaspawn event." << std::endl;
					eventNode = eventNode->next;
					continue;
				}

				if(readXMLInteger(eventNode, "fromz", intValue)){
					fromPos.z = intValue;
				}
				else{
					std::cout << "[Error] Raid(" << fileName << "): fromz tag missing for areaspawn event." << std::endl;
					eventNode = eventNode->next;
					continue;
				}
				
				if(readXMLInteger(eventNode, "tox", intValue)){
					toPos.x = intValue;
				}
				else{
					std::cout << "[Error] Raid(" << fileName << "): tox tag missing for areaspawn event." << std::endl;
					eventNode = eventNode->next;
					continue;
				}

				if(readXMLInteger(eventNode, "toy", intValue)){
					toPos.y = intValue;
				}
				else{
					std::cout << "[Error] Raid(" << fileName << "): toy tag missing for areaspawn event." << std::endl;
					eventNode = eventNode->next;
					continue;
				}

				if(readXMLInteger(eventNode, "toz", intValue)){
					toPos.z = intValue;
				}
				else{
					std::cout << "[Error] Raid(" << fileName << "): toz tag missing for areaspawn event." << std::endl;
					eventNode = eventNode->next;
					continue;
				}
				
				AreaSpawnEvent* areaSpawn = new AreaSpawnEvent(fromPos, toPos, delay);
				bool success = true;
				monsterNode = eventNode->children;
				while(monsterNode){
					if(xmlStrcmp(monsterNode->name, (const xmlChar*)"monster") == 0){
						std::string name;
						int amount;
						
						if(readXMLString(monsterNode, "name", strValue)){
							name = strValue;
						}
						else{
							std::cout << "[Error] Raid(" << fileName << "): name tag missing for monster node." << std::endl;
							success = false;
							break;
						}
						
						if(readXMLInteger(monsterNode, "amount", intValue)){
							amount = intValue;
						}
						else{
							std::cout << "[Error] Raid(" << fileName << "): amount tag missing for monster node." << std::endl;
							success = false;
							break;
						}
						
						areaSpawn->addMonster(name, amount);						
					}

					monsterNode = monsterNode->next;
				}
				
				if(success){
					temporaryList.push_back(areaSpawn);
				}
				else{
					std::cout << "[Error] Raid(" << fileName << "): failed to load monster node for areaspawn event." << std::endl;
					delete areaSpawn;
				}				
			}

			eventNode = eventNode->next;
		}
		
		RaidEventVector::iterator it;
		RaidEvent* temp;
		while(temporaryList.begin() != temporaryList.end()){			
			temp = temporaryList.back();
			
			bool added = false;
			for(it = raidEvents.begin(); it != raidEvents.end(); it++){
				RaidEvent* temp2 = (*it);
				if(temp->getDelay() < temp2->getDelay()) {
					raidEvents.insert(it, temp);
					added = true;
					break;
				}
			}

			if(!added){
				raidEvents.push_back(temp);
			}
			
			temporaryList.pop_back();			
		}
		
		xmlFreeDoc(doc);	
	}
	else{
		std::cout << "[Error] Raid: Could not load " << fileName << "!" << std::endl;
		return false;
	}
	
	loaded = true;
	return true;
}

void Raid::checkRaid()
{
	if(state == RAIDSTATE_IDLE){
		if(!Raids::getInstance()->getRunning()){
			if((uint64_t)OTSYS_TIME() >= (Raids::getInstance()->getLastRaidEnd() + getMargin())){
				if(chance >= (uint32_t)random_range(0, 100)){
#ifdef __DEBUG_RAID__
					std::cout << "[Notice] Raids: Starting raid " << name << std::endl;
#endif
					
					Raids::getInstance()->setRunning(this);
					
					RaidEvent* raidEvent = getNextRaidEvent();
					if(raidEvent){
							state = RAIDSTATE_EXECUTING;
							nextEventEvent = g_game.addEvent(makeTask(raidEvent->getDelay(), boost::bind(&Raid::executeRaidEvent, this, raidEvent)));
					}
				}
			}
		}
	}

	checkRaidEvent = g_game.addEvent(makeTask(getInterval(), boost::bind(&Raid::checkRaid, this)));
}

void Raid::executeRaidEvent(RaidEvent* raidEvent)
{
	if(raidEvent->executeEvent()){
		nextEvent++;
		RaidEvent* newRaidEvent = getNextRaidEvent();
		if(newRaidEvent){
			nextEventEvent = g_game.addEvent(makeTask(newRaidEvent->getDelay()-raidEvent->getDelay(), boost::bind(&Raid::executeRaidEvent, this, newRaidEvent)));
		}
		else{
			resetRaid();
		}
	}
	else{
		resetRaid();
	}
}

void Raid::resetRaid()
{
#ifdef __DEBUG_RAID__
	std::cout << "[Notice] Raids: Resetting raid." << std::endl;
#endif

	nextEvent = 0;
	state = RAIDSTATE_IDLE;
	Raids::getInstance()->setRunning(NULL);
	Raids::getInstance()->setLastRaidEnd(OTSYS_TIME());
}

void Raid::stopEvents()
{
	if(checkRaidEvent != 0){
		g_game.stopEvent(checkRaidEvent);
		checkRaidEvent = 0;
	}
	if(nextEventEvent != 0){
		g_game.stopEvent(nextEventEvent);
		nextEventEvent = 0;
	}
}

RaidEvent* Raid::getNextRaidEvent()
{
	if(nextEvent < raidEvents.size())
		return raidEvents[nextEvent];
	else
		return NULL;
}

void Raid::addEvent(RaidEvent* event)
{
	raidEvents.push_back(event);
}

RaidEvent::RaidEvent(uint32_t _delay)
{
	delay = _delay;
}

AnnounceEvent::AnnounceEvent(const std::string& _message, MessageClasses _messageType, uint32_t _delay) : 
	RaidEvent(_delay)
{
	message = _message;
	messageType = _messageType;
}

bool AnnounceEvent::executeEvent()
{
	for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it){
		(*it).second->sendTextMessage(messageType, message);
	}
	return true;
}


SingleSpawnEvent::SingleSpawnEvent(const std::string& _monsterName, const Position& pos, uint32_t _delay) :
	RaidEvent(_delay)
{
	monsterName = _monsterName;
	position = pos;
}

bool SingleSpawnEvent::executeEvent()
{
	Monster* monster = Monster::createMonster(monsterName);
	if(!monster){
		return false;
	}

	if(!g_game.placeCreature(position, monster)){
		delete monster;
		return false;
	}

	return true;
}

AreaSpawnEvent::AreaSpawnEvent(const Position& _fromPos, const Position& _toPos, uint32_t _delay) :
	RaidEvent(_delay)
{
	fromPos = _fromPos;
	toPos = _toPos;
}

AreaSpawnEvent::~AreaSpawnEvent()
{
	MonsterSpawnList::iterator it;
	for(it = spawnList.begin(); it != spawnList.end(); it++){
		delete (*it);
	}

	spawnList.clear();
}

void AreaSpawnEvent::addMonster(MonsterSpawn* monsterSpawn)
{
	spawnList.push_back(monsterSpawn);
}

void AreaSpawnEvent::addMonster(const std::string& monsterName, uint32_t amount)
{
	MonsterSpawn* monsterSpawn = new MonsterSpawn();
	monsterSpawn->name = monsterName;
	monsterSpawn->amount = amount;
	addMonster(monsterSpawn);
}

bool AreaSpawnEvent::executeEvent()
{
	MonsterSpawnList::iterator it;
	for(it = spawnList.begin(); it != spawnList.end(); it++) {
		MonsterSpawn* spawn = (*it);

		for(unsigned int i = 0; i < spawn->amount; i++){
			Monster* monster = Monster::createMonster(spawn->name);
			if(!monster){
				return false;
			}

			bool success = false;
			for(int tries = 0; tries < MAXIMUM_TRIES_PER_MONSTER; tries++){
				Position pos;
				pos.x = random_range(fromPos.x, toPos.x);
				pos.y = random_range(fromPos.y, toPos.y);
				pos.z = random_range(fromPos.z, toPos.z);
				
				if(g_game.placeCreature(pos, monster)){
					success = true;
					break;
				}
			}

			if(!success){
				delete monster;
				//return false;
			}
		}
	}
	return true;
}

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


#include "definitions.h"
#include "tools.h"
#include "house.h"
#include "housetile.h"
#include "spells.h"

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include <sstream>

extern Game g_game;

Spells::Spells():
m_scriptInterface("Action Interface")
{
	m_scriptInterface.initState();
}

Spells::~Spells()
{
	clear();
}

void Spells::clear()
{
	RunesMap::iterator it;
	for(it = runes.begin(); it != runes.end(); ++it){
		delete it->second;
	}
	runes.clear();
	
	InstantsMap::iterator it2;
	for(it2 = instants.begin(); it2 != instants.end(); ++it2){
		delete it2->second;
	}
	instants.clear();
}	

LuaScriptInterface& Spells::getScriptInterface()
{
	return m_scriptInterface;	
}

std::string Spells::getScriptBaseName()
{
	return "spells";	
}

Event* Spells::getEvent(const std::string& nodeName)
{
	if(nodeName == "rune"){
		return new RuneSpell(&m_scriptInterface);
	}
	else if(nodeName == "instant"){
		return new InstantSpell(&m_scriptInterface);
	}
	else{
		return NULL;
	}
}

bool Spells::registerEvent(Event* event, xmlNodePtr p)
{
	InstantSpell* instant = dynamic_cast<InstantSpell*>(event);
	RuneSpell* rune = dynamic_cast<RuneSpell*>(event);
	if(!instant && !rune)
		return false;
	
	if(instant){
		instants[instant->getWords()] = instant;
	}
	else if(rune){
		runes[rune->getRuneItemId()] = rune;
	}
	else{
		return false;
	}
	
	return true;
}

RuneSpell* Spells::getRuneSpell(const Item* item)
{
	uint32_t id = item->getID();
	RunesMap::iterator it = runes.find(id);
	if(it != runes.end()){
		return it->second;
	}
	else{
		return NULL;
	}
}

//TODO
RuneSpell* Spells::getRuneSpell(const std::string& name)
{
	return NULL;
}
	
InstantSpell* Spells::getInstantSpell(const std::string& words)
{
	return NULL;
}

InstantSpell* Spells::getInstantSpellByName(const std::string& name)
{
	return NULL;
}
//

Spell::Spell()
{
	targetType = TARGET_NONE;
	level = 0;
	magLevel = 0;
	mana = 0;
	soul = 0;
	exhaustion = false;
	premium = false;
	enabled = true;
	vocationBits = 0;
}
	
bool Spell::configureSpell(xmlNodePtr p)
{
	int intValue;
	std::string str;
	if(readXMLString(p, "name", str)){
		name = str;
	}
	else{
		std::cout << "Error: [Spell::configureSpell] Spell without name." << std::endl;
		return false;
	}
	if(readXMLInteger(p, "lvl", intValue)){
	 	level = intValue;
	}
	if(readXMLInteger(p, "maglv", intValue)){
	 	magLevel = intValue;
	}
	if(readXMLInteger(p, "mana", intValue)){
	 	mana = intValue;
	}
	if(readXMLInteger(p, "soul", intValue)){
	 	soul = intValue;
	}
	if(readXMLInteger(p, "exhaustion", intValue)){
		if(intValue == 1)
	 		exhaustion = true;
	 	else
	 		exhaustion = false;
	}
	if(readXMLInteger(p, "prem", intValue)){
		if(intValue == 1)
	 		premium = true;
	 	else
	 		premium = false;
	}
	if(readXMLInteger(p, "enabled", intValue)){
		if(intValue == 1)
	 		enabled = true;
		else
			enabled = false;
	}
	vocationBits = 0xFFFFFFFF;
	//TODO
	//get vocations

	return true;
}
	
bool Spell::spellPlayerChecks(const Player* player)
{
	if(player->getAccessLevel() > 3)
		return true;
	
	if(!enabled)
		return false;
	
	//TODO changes messages
	if(player->getLevel() < level){
		player->sendTextMessage(MSG_STATUS_SMALL, "You do not have enough level.",player->getPosition(), NM_ME_PUFF);
		return false;
	}
	if(player->getMagicLevel() < magLevel){
		player->sendTextMessage(MSG_STATUS_SMALL, "You do not have enough magic level.",player->getPosition(), NM_ME_PUFF);
		return false;
	}
	if(player->getPlayerInfo(PLAYERINFO_MANA) < mana){
		player->sendTextMessage(MSG_STATUS_SMALL, "You do not have enough mana.",player->getPosition(), NM_ME_PUFF);
		return false;
	}
	if(player->getPlayerInfo(PLAYERINFO_SOUL) < soul){
		player->sendTextMessage(MSG_STATUS_SMALL, "You do not have enough soul.",player->getPosition(), NM_ME_PUFF);
		return false;
	}
	//TODO: check if the player knows the spell
	//TODO: check vocs
	/*
	if(premium && !player->getPremium()){
		return false;
	}
	*/
	return true;
}

void Spell::addSpellEffects(Player* player)
{
	if(mana > 0){
		player->changeMana(-((int32_t)mana));
	}
	//TODO
	/*
	if(soul > 0){
		player->changeSoul(-soul);
	}
	if(exhaustion){
		
	}
	*/
}

InstantSpell::InstantSpell(LuaScriptInterface* _interface) :
TalkAction(_interface)
{
	hasParam = false;
	function = NULL;
}

InstantSpell::~InstantSpell()
{
	//
}

std::string InstantSpell::getScriptEventName()
{
	return "onCastInstant";
}

bool InstantSpell::configureEvent(xmlNodePtr p)
{
	int intValue;
	
	if(readXMLInteger(p, "params", intValue)){
		if(intValue == 1)
	 		hasParam = true;
	}
	if(Spell::configureSpell(p) && TalkAction::configureEvent(p)){
		return true;
	}
	else{
		return false;
	}
}

bool InstantSpell::castInstant(Creature* creature, const std::string& words, const std::string& param)
{
	if(creature){
		bool success = true;
		Player* player = creature->getPlayer();
		if(player && !spellPlayerChecks(player)){
			return false;
		}
		//TODO: more checks?
		
		if(m_scripted){
			success =  executeCastInstant(creature, param);
		}
		else{
			if(function){
				success =  function(creature, words, param);
			}
			else{
				success = false;
			}
		}
		
		if(success && player){
			Spell::addSpellEffects(player);
		}
		return success;
	}
	return false;
}

bool InstantSpell::loadFunction(const std::string& functionName)
{
	if(functionName == "editHouseGuest"){
		function = HouseGuestList;
	}
	else if(functionName == "editHouseSubOwner"){
		function = HouseSubOwnerList;
	}
	else if(functionName == "editHouseDoor"){
		function = HouseDoorList;	
	}
	else if(functionName == "houseKick"){
		function = HouseKick;
	}
	else if(functionName == "searchPlayer"){
		function = SearchPlayer;
	}
	else{
		return false;
	}
	
	m_scripted = false;
	return true;
}

bool InstantSpell::executeCastInstant(Creature* creature, const std::string& param)
{
	//onCastInstant(...)
	ScriptEnviroment* env = m_scriptInterface->getScriptEnv();

	//debug only
	std::stringstream desc;
	desc << "onCastInstant";
	env->setEventDesc(desc.str());
	//
	
	env->setScriptId(m_scriptId, m_scriptInterface);
	env->setRealPos(creature->getPosition());
	
	lua_State* L = m_scriptInterface->getLuaState();
	int size0 = lua_gettop(L);
	
	//TODO:
	//	call script
	//
	
	if(size0 != lua_gettop(L)){
		LuaScriptInterface::reportError(NULL, "Stack size changed!");
	}
	
	return false;
}

House* InstantSpell::getHouseFromPos(Creature* creature)
{
	if(creature){
		Player* player = creature->getPlayer();
		if(player){
			HouseTile* houseTile = dynamic_cast<HouseTile*>(player->getTile());
			if(houseTile){
				House* house = houseTile->getHouse();
				if(house){
					return house;
				}
			}
		}
	}
	return NULL;
}

bool InstantSpell::HouseGuestList(Creature* creature, const std::string& words, const std::string& param)
{
	House* house = getHouseFromPos(creature);
	if(!house)
		return false;
	Player* player = creature->getPlayer();
	
	if(house->canEditAccessList(GUEST_LIST, player)){
		player->sendHouseWindow(house, GUEST_LIST);
		return true;
	}
	else{
		player->sendCancelMessage(RET_NOTPOSSIBLE);
		player->sendMagicEffect(player->getPosition(), NM_ME_PUFF);
	}
	
	return true;
}

bool InstantSpell::HouseSubOwnerList(Creature* creature, const std::string& words, const std::string& param)
{
	House* house = getHouseFromPos(creature);
	if(!house)
		return false;
	
	Player* player = creature->getPlayer();
	
	if(house->canEditAccessList(SUBOWNER_LIST, player)){
		player->sendHouseWindow(house, SUBOWNER_LIST);
		return true;
	}
	else{
		player->sendCancelMessage(RET_NOTPOSSIBLE);
		player->sendMagicEffect(player->getPosition(), NM_ME_PUFF);
	}
		
	return true;
}

bool InstantSpell::HouseDoorList(Creature* creature, const std::string& words, const std::string& param)
{
	House* house = getHouseFromPos(creature);
	if(!house)
		return false;
	
	Player* player = creature->getPlayer();
	Position pos = player->getPosition();

	switch(player->getDirection()){
	case NORTH:
		pos.y -= 1;
		break;
	case SOUTH:
		pos.y += 1;
		break;
	case WEST:
		pos.x -= 1;
		break;
	case EAST:
		pos.x += 1;
		break;
	default:
		break;
	}

	Door* door = house->getDoorByPosition(pos);
	if(door && house->canEditAccessList(door->getDoorId(), player)){
		player->sendHouseWindow(house, door->getDoorId());
		return true;
	}
	else{
		player->sendCancelMessage(RET_NOTPOSSIBLE);
		player->sendMagicEffect(player->getPosition(), NM_ME_PUFF);
	}
	
	return true;
}

bool InstantSpell::HouseKick(Creature* creature, const std::string& words, const std::string& param)
{
	House* house = getHouseFromPos(creature);
	if(!house)
		return false;
	
	Player* player = creature->getPlayer();
	house->kickPlayer(player, param);
	
	return true;
}

bool InstantSpell::SearchPlayer(Creature* creature, const std::string& words, const std::string& param)
{
	//a. From 1 to 4 sq's [Person] is standing next to you.
	//b. From 5 to 100 sq's [Person] is to the south, north, east, west.
	//c. From 101 to 274 sq's [Person] is far to the south, north, east, west.
	//d. From 275 to infinite sq's [Person] is very far to the south, north, east, west.
	//e. South-west, s-e, n-w, n-e (corner coordinates): this phrase appears if the player you're looking for has moved five squares in any direction from the south, north, east or west.
	//f. Lower level to the (direction): this phrase applies if the person you're looking for is from 1-25 squares up/down the actual floor you're in.
	//g. Higher level to the (direction): this phrase applies if the person you're looking for is from 1-25 squares up/down the actual floor you're in.

	Player* player = creature->getPlayer();
	if(!player){
		return false;
	}
	enum distance_t{
		DISTANCE_BESIDE,
		DISTANCE_CLOSE_1,
		DISTANCE_CLOSE_2,
		DISTANCE_FAR,
		DISTANCE_VERYFAR,
	};
	
	enum direction_t{
		DIR_N, DIR_S, DIR_E, DIR_W,
		DIR_NE, DIR_NW, DIR_SE, DIR_SW,
	};
	
	enum level_t{
		LEVEL_HIGHER,
		LEVEL_LOWER,
		LEVEL_SAME,
	};

	Player* playerExiva = g_game.getPlayerByName(param);
	if(playerExiva){
		const Position lookPos = player->getPosition();
		const Position searchPos = playerExiva->getPosition();
		
		long dx = lookPos.x - searchPos.x;
		long dy = lookPos.y - searchPos.y;
		long dz = lookPos.z - searchPos.z;
		
		distance_t distance;
		direction_t direction;
		level_t level;
		//getting floor
		if(dz > 0){
			level = LEVEL_HIGHER;
		}
		else if(dz < 0){
			level = LEVEL_LOWER;
		}
		else{
			level = LEVEL_SAME;
		}
		//getting distance
		if(std::abs(dx) < 4 && std::abs(dy) <4){
			distance = DISTANCE_BESIDE;
		}
		else{
			long distance2 = dx*dx + dy*dy;
			if(distance2 < 625){
				distance = DISTANCE_CLOSE_1;
			}
			else if(distance2 < 10000){
				distance = DISTANCE_CLOSE_2;
			}
			else if(distance2 < 75076){
				distance = DISTANCE_FAR;
			}
			else{
				distance = DISTANCE_VERYFAR;
			}
		}
		//getting direction
		float tan;
		if(dx != 0){
			tan = (float)dy/(float)dx;
		}
		else{
			tan = 10.;
		}
		if(std::abs(tan) < 0.4142){
			if(dx > 0){
				direction = DIR_W;
			}
			else{
				direction = DIR_E;
			}			
		}
		else if(std::abs(tan) < 2.4142){
			if(tan > 0){
				if(dy > 0){
					direction = DIR_NW;
				}
				else{
					direction = DIR_SE;
				}
			}
			else{ //tan < 0
				if(dx > 0){
					direction = DIR_SW;
				}
				else{
					direction = DIR_NE;
				}
			}
		}
		else{
			if(dy > 0){
				direction = DIR_N;
			}
			else{
				direction = DIR_S;
			}
		}
		
		std::stringstream ss;
		ss << playerExiva->getName() << " ";

		if(distance == DISTANCE_BESIDE){
			if(level == LEVEL_SAME)
				ss << "is standing next to you";
			else if(level == LEVEL_HIGHER)
				ss << "is above you";
			else if(level == LEVEL_LOWER)
				ss << "is below you";
			}
		else{
			switch(distance){
			case DISTANCE_CLOSE_1:
				if(level == LEVEL_SAME){
					ss << "is to the";
				}
				else if(level == LEVEL_HIGHER){
					ss << "is on a higher level to the";
				}
				else if(level == LEVEL_LOWER){
					ss << "is on a lower level to the";
				}
				break;
				
			case DISTANCE_CLOSE_2:
				ss << "is to the";
				break;

			case DISTANCE_FAR:
				ss << "is far to the";
				break;

			case DISTANCE_VERYFAR:
				ss << "is very far to the";
				break;
			
			default:
				break;
			}

			ss << " ";
			switch(direction){
				case DIR_N:
					ss << "north";
					break;

				case DIR_S:
					ss << "south";
					break;
				case DIR_E:

					ss << "east";
					break;

				case DIR_W:
					ss << "west";
					break;

				case DIR_NE:
					ss << "north-east";
					break;

				case DIR_NW:
					ss << "north-west";
					break;

				case DIR_SE:
					ss << "south-east";
					break;

				case DIR_SW:
					ss << "south-west";
					break;
			}
		}

		ss << ".";
		player->sendTextMessage(MSG_INFO_DESCR, ss.str().c_str());		
		return true;
	}

	return false;
}


RuneSpell::RuneSpell(LuaScriptInterface* _interface) :
Action(_interface)
{
	hasCharges = true;
	runeId = 0;
	function = NULL;
}

RuneSpell::~RuneSpell()
{
	//
}

std::string RuneSpell::getScriptEventName()
{
	return "onUseRune";
}
	
bool RuneSpell::configureEvent(xmlNodePtr p)
{
	int intValue;
	if(readXMLInteger(p, "id", intValue)){
		runeId = intValue;
	}
	else{
		std::cout << "Error: [RuneSpell::configureSpell] Rune spell without id." << std::endl;
		return false;
	}
	if(readXMLInteger(p, "hascharges", intValue)){
		if(intValue == 0)
	 		hasCharges = false;
	}
	if(Spell::configureSpell(p) && Action::configureEvent(p)){
		return true;
	}
	else{
		return false;
	}
}

bool RuneSpell::loadFunction(const std::string& function)
{
	return false;
}

bool RuneSpell::canExecuteAction(const Player* player, const Position& toPos)
{
	if(Action::canExecuteAction(player, toPos)){
		return spellPlayerChecks(player);
	}
	return false;
}

bool RuneSpell::useRune(Creature* creature, const Position& posFrom, const Position& posTo, Creature* target)
{
	//non-players
	bool isSuccess = false;

	if(m_scripted){
		if(executeUseRune(creature, NULL, posFrom, posTo, target) != 0){
			isSuccess = true;
		}
		else{
			isSuccess = false;
		}
	}
	else{
		//call hardcodedAction
		isSuccess = false;
	}

	return isSuccess;
}

bool RuneSpell::executeUse(Player* player, Item* item, const PositionEx& posFrom, const PositionEx& posTo)
{
	bool isSuccess = false;

	if(m_scripted){
		if(executeUseRune(player, item, posFrom, posTo, NULL) != 0){
			isSuccess = true;
		}
		else{
			isSuccess = false;
		}
	}
	else{
		//call hardcodedAction
		isSuccess = false;
	}

	if(player){
		Spell::addSpellEffects(player);
	}
	
	if(hasCharges && item){
		int32_t newCharge = std::max(0, item->getItemCharge() - 1);
		g_game.transformItem(item, item->getID(), newCharge);
	}

	return isSuccess;
}

long RuneSpell::executeUseRune(Creature* creature, Item* item, const Position& posFrom, const Position& posTo, Creature* target)
{
	//onUseRune(cid, pos, var)
	ScriptEnviroment* env = m_scriptInterface->getScriptEnv();
		
	//debug only
	std::stringstream desc;
	desc << "onUseRune";
	env->setEventDesc(desc.str());
		
	env->setScriptId(m_scriptId, m_scriptInterface);
	env->setRealPos(creature->getPosition());
		
	lua_State* L = m_scriptInterface->getLuaState();
		
	long cid = env->addThing(Creature);

	uint32_t targetcid = 0;

	if(target){
		targetcid = env->addThing(target);
	}

	m_scriptInterface->pushFunction(m_scriptId);
	lua_pushnumber(L, cid);
	LuaScriptInterface::pushPosition(L, posTo, 0);
	lua_pushnumber(L, targetcid);
	
	long ret;
	if(m_scriptInterface->callFunction(3, ret) == false){
		ret = 0;
	}
	
	return ret;
}


/*
bool RuneSpell::useRune(Creature* creature, Item* item, const Position& posFrom, const Position& posTo, Creature* target)
{
	Player* player = NULL;
	if(creature){
		player = creature->getPlayer();
		if(player && !spellPlayerChecks(player)){
			return false;
		}
		if(allowFarUse() == false){
			if(Actions::canUse(creature, posTo) == TOO_FAR){
				if(player)
					player->sendCancelMessage(RET_TOOFARAWAY);
				return false;
			}
		}
		else if(Actions::canUseFar(creature, posTo, blockWalls()) == TOO_FAR){
			if(player)
				player->sendCancelMessage(RET_TOOFARAWAY);
			return false;
		}
		else if(Actions::canUseFar(creature, posTo, blockWalls()) == CAN_NOT_THROW){
			if(player)
				player->sendCancelMessage(RET_CANNOTTHROW);
			return false;
		}
	}
	bool success;
	if(m_scripted){
		success = executeUseRune(creature, item, posFrom, posTo, target);
	}
	else{
		//call hardcodedAction
		success = false;
	}
	if(success){
		if(player){
			Spell::addSpellEffects(player);
		}
		if(hasCharges && item){
			int32_t newCharge = std::max(0, item->getItemCharge() - 1);
			g_game.transformItem(item, item->getID(), newCharge);
		}
	}
	return success;
}
*/

/*
#include <algorithm>
#include <functional>
#include <string>
#include <fstream>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h> 

#include <boost/config.hpp>
#include <boost/bind.hpp>

Spells::Spells(Game* igame): game(igame)
{
	//
}

bool Spells::loadFromXml(const std::string& _datadir)
{
	std::string name, words;
	bool enabled = false;
	int vocId, maglv = 0, mana = 0, id = 0, charges = 0;
	loaded = false;

	std::string filename = _datadir + "spells/spells.xml";

	xmlDocPtr doc = xmlParseFile(filename.c_str());
	if(doc){
		loaded = true;
		xmlNodePtr root, p;
		root = xmlDocGetRootElement(doc);
		
		if(xmlStrcmp(root->name,(const xmlChar*)"spells") != 0){
			//TODO: use exceptions here
			std::cerr << "Malformed XML" << std::endl;
		}
		
		int intValue;
		std::string strValue;

		if(readXMLInteger(root, "maxVoc", intValue)){
			maxVoc = intValue;
		}

		for(int i =0; i<= maxVoc; i++){
			std::map<std::string, Spell*> voc;
			vocationSpells.push_back(voc);
		}
		
		p = root->children;
            
		while(p){
			if(xmlStrcmp(p->name, (const xmlChar*)"spell") == 0){

				if(readXMLInteger(p, "enabled", intValue)){
					enabled = (intValue != 0);
				}

				if(enabled){

					if(readXMLString(p, "name", strValue)){
						name = strValue;
						toLowerCaseString(name);
					}

					if(readXMLString(p, "words", strValue)){
						words = strValue;
					}

					if(readXMLInteger(p, "maglv", intValue)){
						maglv = intValue;
					}

					if(readXMLInteger(p, "mana", intValue)){
						mana = intValue;
					}

					Spell* spell = new InstantSpell(_datadir, name, words, maglv, mana, game);
					
					xmlNodePtr tmpNode = p->children;
					while(tmpNode){
						if(xmlStrcmp(tmpNode->name, (const xmlChar*)"vocation") == 0){
							
							if(readXMLInteger(tmpNode, "id", intValue)){
								vocId = intValue;

								if(vocId <= maxVoc){
									(vocationSpells.at(vocId))[words] = spell;
								}
							}
						}
						
						tmpNode = tmpNode->next;
					}
					
					allSpells[words] = spell;
				}
			}
			if(xmlStrcmp(p->name, (const xmlChar*)"rune") == 0){
				
				if(readXMLInteger(p, "enabled", intValue)){
					enabled = (intValue != 0);
				}

				if(enabled){

					if(readXMLString(p, "name", strValue)){
						name = strValue;
						toLowerCaseString(name);
					}

					if(readXMLInteger(p, "id", intValue)){
						id = intValue;
					}

					if(readXMLInteger(p, "charges", intValue)){
						charges = intValue;
					}

					if(readXMLInteger(p, "maglv", intValue)){
						maglv = intValue;
					}
					
					if(readXMLInteger(p, "mana", intValue)){
						mana = intValue;
					}

					Spell* spell = new RuneSpell(_datadir, name, id, charges, maglv, mana, game);
					
					xmlNodePtr tmpNode = p->children;
					while(tmpNode){
						if(xmlStrcmp(tmpNode->name, (const xmlChar*)"vocation") == 0){

							if(readXMLInteger(tmpNode, "id", intValue)){
								vocId = intValue;

								if(vocId <= maxVoc){
									(vocationRuneSpells.at(vocId))[id] = spell;
								}
							}
						}
						
						tmpNode = tmpNode->next;
					}
					
					allRuneSpells[id] = spell;
				}
			}

			p = p->next;    
		}
		
		xmlFreeDoc(doc);
	}

  return loaded;
}

Spells::~Spells(){
	std::map<std::string, Spell*>::iterator it = allSpells.begin();

	while(it != allSpells.end()) {
		delete it->second;
		allSpells.erase(it);
		it = allSpells.begin();
	}
}

Spell::Spell(std::string iname, int imagLv, int imana, Game* igame)
:  game(igame), name(iname),magLv(imagLv) , mana(imana)
{
	this->loaded=false;
	this->script = NULL;
}

Spell::~Spell(){
	if(script) {
		delete script;
		script = NULL;
	}
}

InstantSpell::InstantSpell(const std::string &datadir, std::string iname, std::string iwords, int magLv, int mana, Game* game)
: Spell(iname, magLv, mana, game), words(iwords)
{
	this->script = new SpellScript(datadir, std::string(datadir + "spells/instant/")+(this->words)+std::string(".lua"), this);
	if(!this->script->isLoaded())
		this->loaded=false;
}


RuneSpell::RuneSpell(const std::string &datadir, std::string iname, unsigned short id, unsigned short charges, int magLv, int mana, Game* game)
: Spell(iname, magLv, mana, game)
{
	this->id = id;
	this->charges = charges;

	this->script = new SpellScript(datadir, std::string(datadir + "spells/runes/")+(this->name)+std::string(".lua"), this);
	if(!this->script->isLoaded())
		this->loaded=false;
}

                 
SpellScript::SpellScript(const std::string &datadir, std::string scriptname, Spell* spell){
	this->loaded = false;
	if(scriptname == "")
		return;
	luaState = lua_open();
	luaopen_loadlib(luaState);
	luaopen_base(luaState);
	luaopen_math(luaState);
	luaopen_string(luaState);
	luaopen_io(luaState);
    lua_dofile(luaState, std::string(datadir + "spells/lib/spells.lua").c_str());
	
	FILE* in=fopen(scriptname.c_str(), "r");
	if(!in)
		return;
	else
		fclose(in);
	lua_dofile(luaState, scriptname.c_str());
	this->loaded=true;
	this->spell=spell;
	this->setGlobalNumber("addressOfSpell", (int) spell);
	this->registerFunctions();
}

int SpellScript::registerFunctions(){

	lua_register(luaState, "doTargetMagic", SpellScript::luaActionDoTargetSpell);
	lua_register(luaState, "doTargetExMagic", SpellScript::luaActionDoTargetExSpell);
	lua_register(luaState, "doTargetGroundMagic", SpellScript::luaActionDoTargetGroundSpell);
	lua_register(luaState, "doAreaMagic", SpellScript::luaActionDoAreaSpell);
	lua_register(luaState, "doAreaExMagic", SpellScript::luaActionDoAreaExSpell);
	lua_register(luaState, "doAreaGroundMagic", SpellScript::luaActionDoAreaGroundSpell);

	lua_register(luaState, "changeOutfit", SpellScript::luaActionChangeOutfit);
	lua_register(luaState, "manaShield", SpellScript::luaActionManaShield);
	lua_register(luaState, "getPosition", SpellScript::luaActionGetPos);
	lua_register(luaState, "getSpeed", SpellScript::luaActionGetSpeed);
	lua_register(luaState, "changeSpeed", SpellScript::luaActionChangeSpeed);
	lua_register(luaState, "changeSpeedMonster", SpellScript::luaActionChangeSpeedMonster);
	lua_register(luaState, "makeRune", SpellScript::luaActionMakeRune);
	lua_register(luaState, "makeArrows", SpellScript::luaActionMakeArrows);
	lua_register(luaState, "makeFood", SpellScript::luaActionMakeFood);
	return true;
}

bool SpellScript::castSpell(Creature* creature, const Position& pos, std::string var)
{
	lua_pushstring(luaState, "onCast");
	lua_gettable(luaState, LUA_GLOBALSINDEX);
	lua_pushnumber(luaState, creature->getID());

	lua_newtable(luaState);
	setField("z", pos.z);
	setField("y", pos.y);
	setField("x", pos.x);

	lua_pushnumber(luaState, creature->level);
	lua_pushnumber(luaState, creature->maglevel);
	lua_pushstring(luaState, var.c_str());

	lua_pcall(luaState, 5, 1, 0);

	bool ret = (bool)(lua_toboolean(luaState, -1) > 0);
	lua_pop(luaState, 1);

	return ret;
}

Spell* SpellScript::getSpell(lua_State *L)
{
	lua_getglobal(L, "addressOfSpell");
	int val = (int)lua_tonumber(L, -1);
	lua_pop(L,1);

	Spell* myspell = (Spell*) val;
	if(!myspell){
		return 0;
	}
	return myspell;
}


void SpellScript::internalGetArea(lua_State *L, MagicEffectAreaClass &magicArea)
{
	std::vector<unsigned char> col;

	int i=0, j = 0;
	lua_pushnil(L);  /* first key //

	while (lua_next(L, -2) != 0) {
		lua_pushnil(L);
		col.clear();
    while (lua_next(L, -2) != 0) {
			col.push_back((unsigned char)lua_tonumber(L, -1));
			
			lua_pop(L, 1);  /* removes `value'; keeps `key' for next iteration //
			j++;
		}

		magicArea.areaVec.push_back(col);
		
		j=0;
		lua_pop(L, 1);  /* removes `value'; keeps `key' for next iteration //
		i++;
	}
	
	lua_pop(L, 1);

	magicArea.areaEffect = (char)lua_tonumber(L, -1);
	lua_pop(L,1);
}

void SpellScript::internalGetPosition(lua_State *L, Position& pos)
{
	lua_pushstring(L, "z");
	lua_gettable(L, -2);
	pos.z = (int)lua_tonumber(L, -1);
	lua_pop(L, 1);

	lua_pushstring(L, "y");
	lua_gettable(L, -2);
	pos.y = (int)lua_tonumber(L, -1);
	lua_pop(L, 1);

	lua_pushstring(L, "x");
	lua_gettable(L, -2);
	pos.x = (int)lua_tonumber(L, -1);
	lua_pop(L, 1);

  lua_pop(L, 1); //table
}

void SpellScript::internalGetMagicEffect(lua_State *L, MagicEffectClass& me)
{
	lua_pushnil(L);

	lua_next(L, -2);
	int attackType = (int)lua_tonumber(L, -1);
	lua_pop(L, 1);

	switch(attackType) {
		case 0: me.attackType = ATTACK_NONE; break;
		case 1: me.attackType = ATTACK_ENERGY; break;
		case 2: me.attackType = ATTACK_BURST; break;
		case 3: me.attackType = ATTACK_FIRE; break;
		case 4: me.attackType = ATTACK_PHYSICAL; break;
		case 5: me.attackType = ATTACK_POISON; break;
		case 6: me.attackType = ATTACK_PARALYZE; break;
		case 7: me.attackType = ATTACK_DRUNKNESS; break;

		default:
#ifdef __DEBUG__
			std::cerr << "WARNING: internalGetMagicEffect(), attackType out of range!" << std::endl;
#endif
			break;
	}
	
	lua_next(L, -2);
	me.animationEffect = (char)lua_tonumber(L, -1);
	lua_pop(L, 1);

	lua_next(L, -2);
	me.hitEffect = (char)lua_tonumber(L, -1);
	lua_pop(L, 1);

 	lua_next(L, -2);
	me.damageEffect = (char)lua_tonumber(L, -1);
	lua_pop(L, 1);

	lua_next(L, -2);
	me.animationColor = (char)lua_tonumber(L, -1);
	lua_pop(L, 1);

	lua_next(L, -2);
	me.offensive = (bool)(lua_toboolean(L, -1) > 0);
	lua_pop(L, 1);

	lua_next(L, -2);
	me.drawblood = (bool)(lua_toboolean(L, -1) > 0);
	lua_pop(L, 1);

	lua_next(L, -2);
	me.minDamage = (int)lua_tonumber(L, -1);
	lua_pop(L, 1);

	lua_next(L, -2);
	me.maxDamage = (int)lua_tonumber(L, -1);
	lua_pop(L, 1);

	lua_pop(L, 1); // last key

	lua_pop(L, 1); //end of table
}

int SpellScript::luaActionDoTargetSpell(lua_State *L)
{
	MagicEffectTargetClass magicTarget;

	internalGetMagicEffect(L, magicTarget);

	Position centerpos;
	internalGetPosition(L, centerpos);

	Spell* spell = getSpell(L);
	magicTarget.manaCost = spell->getMana();

	Creature* creature = spell->game->getCreatureByID((unsigned long)lua_tonumber(L, -1));
  	lua_pop(L,1);
	if(!creature){
		lua_pushboolean(L, false);
		return 1;
	}
	bool isSuccess = spell->game->creatureThrowRune(creature, centerpos, magicTarget);
	lua_pushboolean(L, isSuccess);
	return 1;
}

int SpellScript::luaActionDoTargetExSpell(lua_State *L)
{
	ConditionVec condvec;
	internalLoadDamageVec(L, condvec);

	MagicEffectTargetExClass magicTargetEx(/*md,// condvec);

	internalGetMagicEffect(L, magicTargetEx);

	Position centerpos;
	internalGetPosition(L, centerpos);

	Spell* spell = getSpell(L);
  magicTargetEx.manaCost = spell->getMana();

	Creature* creature = spell->game->getCreatureByID((unsigned long)lua_tonumber(L, -1));
  lua_pop(L,1);
	if(!creature){
		lua_pushboolean(L, false);
		return 1;
	}
	bool isSuccess = spell->game->creatureThrowRune(creature, centerpos, magicTargetEx);
	lua_pushboolean(L, isSuccess);
	return 1;
}

int SpellScript::luaActionDoTargetGroundSpell(lua_State *L)
{
	TransformMap transformMap;
	internalLoadTransformVec(L, transformMap);
	
	MagicEffectItem* fieldItem = new MagicEffectItem(transformMap);
	fieldItem->useThing2();
	MagicEffectTargetGroundClass magicGround(fieldItem);

	magicGround.offensive = (bool)(lua_toboolean(L, -1) > 0);
	lua_pop(L,1);
	
	magicGround.animationEffect = (char)lua_tonumber(L, -1);
	lua_pop(L,1);

	Position centerpos;
	internalGetPosition(L, centerpos);

	Spell* spell = getSpell(L);
 	magicGround.manaCost = spell->getMana();

	Creature* creature = spell->game->getCreatureByID((unsigned long)lua_tonumber(L, -1));
	if(!creature){
		lua_pushboolean(L, false);
		return 1;
	}
	bool isSuccess = spell->game->creatureThrowRune(creature, centerpos, magicGround);
	lua_pushboolean(L, isSuccess);
	return 1;
}

int SpellScript::luaActionDoAreaSpell(lua_State *L)
{
	MagicEffectAreaClass magicArea;

	internalGetMagicEffect(L, magicArea);

	internalGetArea(L, magicArea);

	bool needDirection = (bool)(lua_toboolean(L, -1) > 0);
	lua_pop(L,1);

	Position centerpos;
	internalGetPosition(L, centerpos);

	Spell* spell = getSpell(L);
  magicArea.manaCost = spell->getMana();
  
	Creature* creature = spell->game->getCreatureByID((unsigned long)lua_tonumber(L, -1));
	lua_pop(L,1);
	if(!creature){
		lua_pushboolean(L, false);
		return 1;
	}
	 
	if(needDirection){
		switch(creature->getDirection()) {
			case NORTH: magicArea.direction = 1; break;
			case WEST: magicArea.direction = 2; break;
			case EAST: magicArea.direction = 3; break;
			case SOUTH: magicArea.direction = 4; break;
		};
	}
  else {
		magicArea.direction = 1;
	}
    RuneSpell* runeSpell = dynamic_cast<RuneSpell*>(spell);
    bool isSuccess;
    if(runeSpell)
			isSuccess = spell->game->creatureThrowRune(creature, centerpos, magicArea);
    else
			isSuccess = spell->game->creatureMakeMagic(creature, centerpos, &magicArea);

	lua_pushboolean(L, isSuccess);
	return 1;
}

int SpellScript::luaActionDoAreaExSpell(lua_State *L)
{
	ConditionVec condvec;

	int count = (int)lua_tonumber(L, -1);
	lua_pop(L,1);

	for(int n = 0; n < count; ++n) {
		internalLoadDamageVec(L, condvec);
	}

	MagicEffectAreaExClass magicAreaEx(/*md,// condvec);

	internalGetMagicEffect(L, magicAreaEx);
    
	internalGetArea(L, magicAreaEx);

	bool needDirection = (bool)(lua_toboolean(L, -1) > 0);
	lua_pop(L,1);

	Position centerpos;
	internalGetPosition(L, centerpos);

	Spell* spell = getSpell(L);
  magicAreaEx.manaCost = spell->getMana();
  
	Creature* creature = spell->game->getCreatureByID((unsigned long)lua_tonumber(L, -1));
	lua_pop(L,1);
	if(!creature){
		lua_pushboolean(L, false);
		return 1;
	}
		 	
	if(needDirection){
		switch(creature->getDirection()) {
			case NORTH: magicAreaEx.direction = 1; break;
			case WEST: magicAreaEx.direction = 2; break;
			case EAST: magicAreaEx.direction = 3; break;
			case SOUTH: magicAreaEx.direction = 4; break;
		};
	}
  else {
		magicAreaEx.direction = 1;
	}
	
    RuneSpell* runeSpell = dynamic_cast<RuneSpell*>(spell);
    bool isSuccess;
    if(runeSpell)
			isSuccess = spell->game->creatureThrowRune(creature, centerpos, magicAreaEx);
    else
			isSuccess = spell->game->creatureMakeMagic(creature, centerpos, &magicAreaEx);
	
	lua_pushboolean(L, isSuccess);
	return 1;
}

int SpellScript::luaActionDoAreaGroundSpell(lua_State *L)
{
	TransformMap transformMap;
	internalLoadTransformVec(L, transformMap);

	MagicEffectItem* fieldItem = new MagicEffectItem(/*md,// transformMap);
	fieldItem->useThing2();
	MagicEffectAreaGroundClass magicGroundEx(fieldItem);

	internalGetMagicEffect(L, magicGroundEx);

	internalGetArea(L, magicGroundEx);

	bool needDirection = (bool)(lua_toboolean(L, -1) > 0);
	lua_pop(L,1);

	Position centerpos;
	internalGetPosition(L, centerpos);

	Spell* spell = getSpell(L);
  magicGroundEx.manaCost = spell->getMana();

	Creature* creature = spell->game->getCreatureByID((unsigned long)lua_tonumber(L, -1));
	lua_pop(L, 1);
	if(!creature){
		lua_pushboolean(L, false);
		return 1;
	}
	
	if(needDirection){
		switch(creature->getDirection()) {
			case NORTH: magicGroundEx.direction = 1; break;
			case WEST: magicGroundEx.direction = 2; break;
			case EAST: magicGroundEx.direction = 3; break;
			case SOUTH: magicGroundEx.direction = 4; break;
		};
	}
  else {
		magicGroundEx.direction = 1;
	}

	bool isSuccess = spell->game->creatureThrowRune(creature, centerpos, magicGroundEx);
	lua_pushboolean(L, isSuccess);
	return 1;
}

int SpellScript::luaActionChangeOutfit(lua_State *L)
{
	int looktype = (int)lua_tonumber(L, -1);
	lua_pop(L,1);
	long time = (long)lua_tonumber(L, -1)*1000;
	lua_pop(L,1);
	
	Spell* spell = getSpell(L);
	Creature* creature = spell->game->getCreatureByID((unsigned long)lua_tonumber(L, -1));
	lua_pop(L,1);
	
	creature->looktype = looktype;
	spell->game->internalCreatureChangeOutfit(creature);
	
  spell->game->changeOutfitAfter(creature->getID(), creature->lookmaster, time);
	return 0;
}

void SpellScript::internalLoadDamageVec(lua_State *L, ConditionVec& condvec)
{
	//cid
	unsigned long cid = (int)lua_tonumber(L, 1);

	MagicEffectTargetCreatureCondition magicTargetCondition(cid);
	
	internalGetMagicEffect(L, magicTargetCondition);

	//conditionTimeCount
	long condCount = (int)lua_tonumber(L, -1); //conditionCount
	lua_pop(L, 1);

	long ticks = (int)lua_tonumber(L, -1); //delayTicks
	lua_pop(L, 1);

	condvec.insert(condvec.begin(), CreatureCondition(ticks, condCount, magicTargetCondition));
}

void SpellScript::internalLoadTransformVec(lua_State *L, TransformMap& transformMap)
{
	TransformItem ti;
	ConditionVec condvec;

	int stateCount = (int)lua_tonumber(L, -1);
	lua_pop(L,1);

	for(int s = 0; s < stateCount; ++s) {
		condvec.clear();

		int id = (int)lua_tonumber(L, -1);
		lua_pop(L,1);
		
		ti.first = (int)lua_tonumber(L, -1);
		lua_pop(L,1);

		int count = (int)lua_tonumber(L, -1);
		lua_pop(L,1);

		for(int n = 0; n < count; ++n) {
			internalLoadDamageVec(L, condvec);
		}
		
		ti.second = condvec;
		transformMap[id] = ti;
	}
}

int SpellScript::luaActionManaShield(lua_State *L)
{
	long time = (long)lua_tonumber(L, -1)*1000;
	lua_pop(L,1);
	
	Spell* spell = getSpell(L);
	Creature* creature = spell->game->getCreatureByID((unsigned long)lua_tonumber(L, -1));
	lua_pop(L,1);
	creature->manaShieldTicks = time;
	
	if(Player* player = creature->getPlayer()){
		player->sendIcons();
	}

	return 0;
}

int SpellScript::luaActionChangeSpeed(lua_State *L)
{
	long time = (long)lua_tonumber(L, -1)*1000;
	lua_pop(L,1);
	
	int speed = (int)lua_tonumber(L, -1);
	lua_pop(L,1);
	
	Spell* spell = getSpell(L);
	Creature* creature = spell->game->getCreatureByID((unsigned long)lua_tonumber(L, -1));
	lua_pop(L,1);
	
	spell->game->addEvent(makeTask(time, boost::bind(&Game::changeSpeed, spell->game,creature->getID(), creature->getNormalSpeed()) ) );

	if(Player* player = creature->getPlayer()){
		spell->game->changeSpeed(creature->getID(), creature->getNormalSpeed() + speed);
		player->sendIcons();
	}
	
	creature->hasteTicks = time;
	return 0;
}

int SpellScript::luaActionChangeSpeedMonster(lua_State *L)
{
	long time = (long)lua_tonumber(L, -1)*1000;
	lua_pop(L,1);
	
	int speed = (int)lua_tonumber(L, -1);
	lua_pop(L,1);
	
	Spell* spell = getSpell(L);
	Creature* creature = spell->game->getCreatureByID((unsigned long)lua_tonumber(L, -1));
	lua_pop(L,1);
	
	if(creature){
		spell->game->addEvent(makeTask(time, boost::bind(&Game::changeSpeed, spell->game,creature->getID(), creature->getSpeed())));
		spell->game->changeSpeed(creature->getID(), speed);
		creature->hasteTicks = time;
    }
	return 0;
}

int SpellScript::luaActionGetSpeed(lua_State *L)
{
	Spell* spell = getSpell(L);
	Creature* creature = spell->game->getCreatureByID((unsigned long)lua_tonumber(L, -1));
	lua_pop(L,1);
	
	lua_pushnumber(L, creature->getNormalSpeed());
	return 1;
}

int SpellScript::luaActionGetPos(lua_State *L)
{
	const char* s = lua_tostring(L, -1);
	lua_pop(L,1);
	Spell* spell = getSpell(L);
	Creature* c = spell->game->getCreatureByName(std::string(s));
	Player* p = dynamic_cast<Player*>(c);
	if(!c || !p){
		lua_newtable(L);
		lua_pushstring(L, "x");
		lua_pushnil(L);
		lua_settable(L, -3);

		lua_pushstring(L, "y");
		lua_pushnil(L);
		lua_settable(L, -3);

		lua_pushstring(L, "z");
		lua_pushnil(L);
		lua_settable(L, -3);
	}
	else{
		lua_newtable(L);
		lua_pushstring(L, "x");
		lua_pushnumber(L, c->getPosition().x);
		lua_settable(L, -3);

		lua_pushstring(L, "y");
		lua_pushnumber(L, c->getPosition().y);
		lua_settable(L, -3);

		lua_pushstring(L, "z");
		lua_pushnumber(L, c->getPosition().z);
		lua_settable(L, -3);
	}

	return 1;
}

int SpellScript::luaActionMakeRune(lua_State *L)
{
	unsigned char charges = (unsigned char)lua_tonumber(L, -1);
	lua_pop(L,1);
	
	unsigned short type = (unsigned short)lua_tonumber(L, -1);
	lua_pop(L,1);
	
	Spell* spell = getSpell(L);
	Creature* creature = spell->game->getCreatureByID((unsigned long)lua_tonumber(L, -1));
	lua_pop(L,1);
		
	Player* player = creature->getPlayer();
	if(player){
		MagicEffectTargetClass magicTarget;

		magicTarget.offensive = false;
		magicTarget.drawblood = false;
		magicTarget.animationEffect = 0;
		magicTarget.attackType = ATTACK_NONE;		
		magicTarget.hitEffect = 255; //NM_ME_NONE
		magicTarget.animationColor = 19; //GREEN

		int a = -1;
		int b = -1;

		//try to create rune 1
		a = internalMakeRune(player,SLOT_RIGHT,spell,type,charges);
		if(a == 1){
			magicTarget.manaCost = spell->getMana();
		}

		//check if we got enough mana for the left hand
		if(player->getMana() - magicTarget.manaCost >= magicTarget.manaCost){
			//try to create rune 2
			b = internalMakeRune(player,SLOT_LEFT,spell,type,charges);
			if(b == 1) {
				magicTarget.manaCost += spell->getMana();
			}
		}

		if(a == -1 && b == -1){ //not enough mana
			magicTarget.damageEffect = 2; //NM_ME_PUFF
			magicTarget.manaCost = player->getPlayerInfo(PLAYERINFO_MAXMANA) + 1; //force not enough mana
		}
		else if( a == 0 && b == 0){ //not create any rune
			magicTarget.damageEffect = 2; //NM_ME_PUFF		
			magicTarget.manaCost = 0;
		}
		else if(a == 1 || b == 1) {
			magicTarget.damageEffect = 12; //NM_ME_MAGIC_ENERGIE = 12
		}

		/*else{
			magicTarget.damageEffect = 12; //NM_ME_MAGIC_ENERGIE = 12

			if(b==1){
				magicTarget.manaCost = spell->getMana();
			}
			else{	//only create 1 rune
				magicTarget.manaCost = 0; 
			}
		}//

		bool isSuccess = spell->game->creatureThrowRune(player, player->getPosition(), magicTarget);

		lua_pushnumber(L, 1);
		return 1;
	}
	lua_pushnumber(L, 0);
	return 1;
}

//create new runes and delete blank ones
int SpellScript::internalMakeRune(Player* player, unsigned short sl_id, Spell* spell,
	unsigned short id, unsigned char charges)
{
	//check mana
	if(player->mana < spell->getMana() || player->exhaustedTicks >= 1000)
		return -1;

	Item* item = player->getInventoryItem((slots_t)sl_id);
	if(item){
		if(item->getID() == ITEM_RUNE_BLANK){
			spell->game->transformItem(item, id, charges);
			return 1;
		}
		else{
			return 0;
		}
	}
	else{
		return 0;
	}
}

int SpellScript::luaActionMakeArrows(lua_State *L)
{
	unsigned char count = (unsigned char)lua_tonumber(L, -1);
	lua_pop(L,1);

	unsigned short id = (unsigned short)lua_tonumber(L, -1);
	lua_pop(L,1);

	Spell* spell = getSpell(L);
	Player* player = spell->game->getPlayerByID((unsigned long)lua_tonumber(L, -1));
	lua_pop(L,1);
 
	if(player){
 		MagicEffectTargetClass magicTarget;

		magicTarget.offensive = false;
 		magicTarget.drawblood = false;
 		magicTarget.animationEffect = 0;
 		magicTarget.attackType = ATTACK_NONE;  
 		magicTarget.hitEffect = 255; //NM_ME_NONE
 		magicTarget.animationColor = 19; //GREEN

		if(player->getMana() < spell->getMana()){
			magicTarget.damageEffect = 2; //NM_ME_PUFF  
  			magicTarget.manaCost = player->getPlayerInfo(PLAYERINFO_MAXMANA) + 1; //force not enough mana
		}
		else{			
			magicTarget.manaCost = spell->getMana();
			magicTarget.damageEffect = 12; //NM_ME_MAGIC_ENERGIE = 12
 		}
 		
		bool isSuccess = spell->game->creatureThrowRune(player, player->getPosition(), magicTarget);

		if(isSuccess){
			Item* newItem = Item::CreateItem(id, count);

			ReturnValue ret = spell->game->internalAddItem(player, newItem);
			if(ret != RET_NOERROR){
				Tile* tile = player->getTile();
				ret = spell->game->internalAddItem(tile, newItem);
				if(ret != RET_NOERROR){
					delete newItem;
				}
			}
		}
		
 		lua_pushnumber(L, 1);
 		return 1;
	}
	lua_pushnumber(L, 0);
	return 1;
}

int SpellScript::luaActionMakeFood(lua_State *L)
{
	unsigned char count = (unsigned char)lua_tonumber(L, -1);
	lua_pop(L,1);
	
	Spell* spell = getSpell(L);
	Creature* creature = spell->game->getCreatureByID((unsigned long)lua_tonumber(L, -1));
	lua_pop(L,1);
	
	Player* player = dynamic_cast<Player*>(creature);
	if(player){
  		MagicEffectTargetClass magicTarget;

		magicTarget.offensive = false;
		magicTarget.drawblood = false;
		magicTarget.animationEffect = 0;
		magicTarget.attackType = ATTACK_NONE;  
		magicTarget.hitEffect = 255; //NM_ME_NONE
		magicTarget.animationColor = 19; //GREEN

		if(player->getMana() < spell->getMana()){
  			magicTarget.damageEffect = 2; //NM_ME_PUFF  
    		magicTarget.manaCost = player->getPlayerInfo(PLAYERINFO_MAXMANA) + 1; //force not enough mana
  		}
  		else{  
  			magicTarget.manaCost = spell->getMana();
  			magicTarget.damageEffect = 12; //NM_ME_MAGIC_ENERGIE = 12    	
		}
  
		bool isSuccess = spell->game->creatureThrowRune(player, player->getPosition(), magicTarget);

		if(isSuccess) {
			int r,foodtype;
			r = rand() % 7;
			if(r == 0) foodtype = ITEM_MEAT;
			if(r == 1) foodtype = ITEM_HAM;
			if(r == 2) foodtype = ITEM_GRAPE;
			if(r == 3) foodtype = ITEM_APPLE;
			if(r == 4) foodtype = ITEM_BREAD;
			if(r == 5) foodtype = ITEM_CHEESE;
			if(r == 6) foodtype = ITEM_ROLL;
    	if(r == 7) foodtype = ITEM_BREAD;
  		
			Item* newItem = Item::CreateItem(foodtype, count);

			ReturnValue ret = spell->game->internalAddItem(player, newItem);
			if(ret != RET_NOERROR){
				Tile* tile = player->getTile();
				ret = spell->game->internalAddItem(tile, newItem);
				if(ret != RET_NOERROR){
					delete newItem;
				}
			}
		}
  
		lua_pushnumber(L, 1);
		return 1;
	}
	lua_pushnumber(L, 0);
	return 1;
}
*/

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
#include "combat.h"

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include <sstream>

extern Game g_game;
extern LuaScript g_config;

int32_t Spells::spellExhaustionTime = 0;
int32_t Spells::spellInFightTime = 0;

Spells::Spells():
m_scriptInterface("Spell Interface")
{
	spellExhaustionTime = g_config.getGlobalNumber("exhausted", 0);
	spellInFightTime = g_config.getGlobalNumber("pzlocked", 0);
	m_scriptInterface.initState();
}

Spells::~Spells()
{
	clear();
}

bool Spells::playerSaySpell(Player* player, SpeakClasses type, const std::string& words)
{
	std::string str_words;
	std::string str_param;
	unsigned int loc = (uint32_t)words.find( '"', 0 );
	if(loc != std::string::npos && loc >= 0){
		str_words = std::string(words, 0, loc);
		str_param = std::string(words, (loc+1), words.size()-loc-1);
	}
	else {
		str_words = words;
		str_param = std::string(""); 
	}
	
	trim_left(str_words, " ");
	trim_right(str_words, " ");

	InstantsMap::iterator it;
	for(it = instants.begin(); it != instants.end(); ++it){
		if(strcasecmp(it->first.c_str(), str_words.c_str()) == 0){
			InstantSpell* instantSpell = it->second;
			if(instantSpell->playerCastInstant(player, str_param)){
				return true;
			}
		}
	}

	return false;
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
	level = 0;
	magLevel = 0;
	mana = 0;
	soul = 0;
	exhaustion = false;
	needTarget = false;
	blocking = false;
	premium = false;
	enabled = true;
	isAggressive = true;
	vocationBits = 0;
}
	
bool Spell::configureSpell(xmlNodePtr p)
{
	int intValue;
	std::string strValue;
	if(readXMLString(p, "name", strValue)){
		name = strValue;
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
		exhaustion = (intValue == 1);
	}

	if(readXMLInteger(p, "prem", intValue)){
		premium = (intValue == 1);
	}

	if(readXMLInteger(p, "enabled", intValue)){
		enabled = (intValue == 1);
	}

	if(readXMLInteger(p, "needtarget", intValue)){
		needTarget = (intValue == 1);
	}

	if(readXMLInteger(p, "blocking", intValue)){
		blocking = (intValue == 1);
	}

	if(readXMLInteger(p, "aggressive", intValue)){
		isAggressive = (intValue == 1);
	}

	vocationBits = 0xFFFFFFFF;
	//TODO
	//get vocations

	return true;
}

bool Spell::playerSpellCheck(const Player* player)
{
	if(player->getAccessLevel() > 3)
		return true;
	
	if(!enabled)
		return false;
	
	if(isAggressive){
		if(player->getAccessLevel() < 2 && player->getTile()->isPz()){
			player->sendCancelMessage(RET_ACTIONNOTPERMITTEDINPROTECTIONZONE);
			return false;
		}
	}

	if(player->hasCondition(CONDITION_EXHAUSTED)){
		player->sendTextMessage(MSG_STATUS_SMALL, "You are exhausted.",player->getPosition(), NM_ME_PUFF);
		return false;
	}

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

bool Spell::playerInstantSpellCheck(const Player* player, const Position& toPos)
{
	bool result = playerSpellCheck(player);

	if(result){
		ReturnValue ret = RET_NOERROR;

		const Position& playerPos = player->getPosition();
		if(playerPos.z > toPos.z){
			ret = RET_FIRSTGOUPSTAIRS;
		}
		else if(playerPos.z < toPos.z){
			ret = RET_FIRSTGODOWNSTAIRS;
		}
		else{
			Tile* tile = g_game.getTile(toPos.x, toPos.y, toPos.z);

			if(!tile){
				ret = RET_NOTPOSSIBLE;
			}

			if(ret == RET_NOERROR){
				ret = Combat::canDoCombat(player, tile, isAggressive);
			}

			if(ret == RET_NOERROR && blocking){
				if(!tile->creatures.empty() || tile->hasProperty(BLOCKSOLID)){
					ret = RET_NOTENOUGHROOM;
				}
			}
		}

		if(ret != RET_NOERROR){
			player->sendCancelMessage(ret);
			player->sendMagicEffect(player->getPosition(), NM_ME_PUFF);
			return false;
		}
	}

	return result;
}

bool Spell::playerRuneSpellCheck(const Player* player, const Position& toPos)
{
	bool result = playerSpellCheck(player);

	if(result){
		ReturnValue ret = RET_NOERROR;

		const Position& playerPos = player->getPosition();
		if(playerPos.z > toPos.z){
			ret = RET_FIRSTGOUPSTAIRS;
		}
		else if(playerPos.z < toPos.z){
			ret = RET_FIRSTGODOWNSTAIRS;
		}
		else{
			Tile* tile = g_game.getTile(toPos.x, toPos.y, toPos.z);

			if(!tile){
				ret = RET_NOTPOSSIBLE;
			}

			if(ret == RET_NOERROR){
				ret = Combat::canDoCombat(player, tile, isAggressive);
			}

			if(ret == RET_NOERROR && blocking){
				if(!tile->creatures.empty() || tile->hasProperty(BLOCKSOLID)){
					ret = RET_NOTENOUGHROOM;
				}
			}
			
			if(ret == RET_NOERROR && needTarget && tile->creatures.empty()){
				ret = RET_CANONLYUSETHISRUNEONCREATURES;
			}
		}

		if(ret != RET_NOERROR){
			player->sendCancelMessage(ret);
			player->sendMagicEffect(player->getPosition(), NM_ME_PUFF);
			return false;
		}
	}

	return result;
}

void Spell::postCastSpell(Player* player)
{
	if(player->getAccessLevel() > 0){
		return;
	}

	if(mana > 0){
		player->changeMana(-((int32_t)mana));
	}

	if(exhaustion){
		Condition* condition = Condition::createCondition(CONDITION_EXHAUSTED, Spells::spellExhaustionTime, 0);
		if(!player->addCondition(condition)){
			delete condition;
		}
	}
	
	if(isAggressive && Spells::spellInFightTime != 0){
		Condition* condition = Condition::createCondition(CONDITION_INFIGHT, Spells::spellInFightTime, 0);
		if(!player->addCondition(condition)){
			delete condition;
		}
	}

	//TODO
	/*
	if(soul > 0){
		player->changeSoul(-soul);
	}
	*/
}

InstantSpell::InstantSpell(LuaScriptInterface* _interface) :
TalkAction(_interface)
{
	needDirection = false;
	hasParam = false;
	function = NULL;
}

InstantSpell::~InstantSpell()
{
	//
}

std::string InstantSpell::getScriptEventName()
{
	return "onCastSpell";
}

bool InstantSpell::configureEvent(xmlNodePtr p)
{
	int intValue;
	
	if(readXMLInteger(p, "params", intValue)){
		if(intValue == 1)
	 		hasParam = true;
	}

	if(readXMLInteger(p, "direction", intValue)){
		needDirection = (intValue == 1);
	}

	if(Spell::configureSpell(p) && TalkAction::configureEvent(p)){
		return true;
	}
	else{
		return false;
	}
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


bool InstantSpell::playerCastInstant(Player* player, const std::string& param)
{
	if(!playerSpellCheck(player)){
		return false;
	}

	LuaVariant var;

	if(needTarget){
		Player* target = g_game.getPlayerByName(param);
		if(!target){
			player->sendTextMessage(MSG_STATUS_SMALL, "A player with this name is not online.", player->getPosition(), NM_ME_PUFF);
			return false;
		}

		if(!g_game.canThrowObjectTo(player->getPosition(), target->getPosition())){
			player->sendTextMessage(MSG_STATUS_SMALL, "Player is not reachable.", player->getPosition(), NM_ME_PUFF);
			return false;
		}

		var.type = VARIANT_NUMBER;
		var.number = target->getID();
	}
	else if(hasParam){
		var.type = VARIANT_STRING;
		var.text = param;
	}
	else{
		var.type = VARIANT_POSITION;
		var.pos = getCasterPosition(player);

		if(!playerInstantSpellCheck(player, var.pos)){
			return false;
		}
	}

	bool result = castInstant(player, var);

	if(result){
		Spell::postCastSpell(player);
	}

	return result;
}

Position InstantSpell::getCasterPosition(Creature* creature)
{
	Position pos = creature->getPosition();

	if(needDirection){
		switch(creature->getDirection()){
			case NORTH:
				pos.y -= 1;
				break;

			case SOUTH:
				pos.y += 1;
				break;

			case EAST:
				pos.x += 1;
				break;

			case WEST:
				pos.x -= 1;
				break;
			
			default:
				break;
		}
	}

	return pos;
}

bool InstantSpell::castInstant(Creature* creature)
{
	LuaVariant var;
	var.type = VARIANT_POSITION;
	var.pos = getCasterPosition(creature);

	return castInstant(creature, var);
}

bool InstantSpell::castInstant(Creature* creature, Creature* target)
{
	LuaVariant var;
	var.type = VARIANT_NUMBER;
	var.number = target->getID();

	return castInstant(creature, var);
}

bool InstantSpell::castInstant(Creature* creature, const LuaVariant& var)
{
	bool result = false;
	
	if(m_scripted){
		result =  executeCastInstant(creature, var);
	}
	else{
		if(function){
			result = function(creature, var.text);
		}
	}

	return result;
}

bool InstantSpell::executeCastInstant(Creature* creature, const LuaVariant& var)
{
	//onCastSpell(cid, var)
	ScriptEnviroment* env = m_scriptInterface->getScriptEnv();

	#ifdef __DEBUG_LUASCRIPTS__
	std::stringstream desc;
	desc << "onCastSpell - " << creature->getName();
	env->setEventDesc(desc.str());
	#endif
	
	env->setScriptId(m_scriptId, m_scriptInterface);
	env->setRealPos(creature->getPosition());
	
	lua_State* L = m_scriptInterface->getLuaState();

	uint32_t cid = env->addThing(creature);

	LuaVariant* pVar = new LuaVariant(var);
	uint32_t variant = env->addVariant(pVar);

	m_scriptInterface->pushFunction(m_scriptId);
	lua_pushnumber(L, cid);
	lua_pushnumber(L, variant);

	bool isSuccess = true;

	long result;
	if(m_scriptInterface->callFunction(2, result) == false){
		isSuccess = false;
	}
	
	return isSuccess;
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

bool InstantSpell::HouseGuestList(Creature* creature, const std::string& param)
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

bool InstantSpell::HouseSubOwnerList(Creature* creature, const std::string& param)
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

bool InstantSpell::HouseDoorList(Creature* creature, const std::string& param)
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

bool InstantSpell::HouseKick(Creature* creature, const std::string& param)
{
	House* house = getHouseFromPos(creature);
	if(!house)
		return false;
	
	Player* player = creature->getPlayer();
	house->kickPlayer(player, param);
	
	return true;
}

bool InstantSpell::SearchPlayer(Creature* creature, const std::string& param)
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
	return "onCastSpell";
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
	if(!Action::canExecuteAction(player, toPos)){
		return false;
	}

	/*
	if(!playerSpellCheck(player)){
		return false;
	}
	*/

	return true;
}

bool RuneSpell::executeUse(Player* player, Item* item, const PositionEx& posFrom, const PositionEx& posTo)
{
	if(!playerRuneSpellCheck(player, posTo)){
		return false;
	}

	LuaVariant var;
	var.type = VARIANT_POSITION;
	var.pos = posTo;

	bool result = castRune(player, var);

	if(result){
		Spell::postCastSpell(player);
		
		if(hasCharges && item){
			int32_t newCharge = std::max(0, item->getItemCharge() - 1);
			g_game.transformItem(item, item->getID(), newCharge);
		}
	}

	return result;
}

bool RuneSpell::castRune(Creature* creature, Creature* target)
{
	LuaVariant var;
	var.type = VARIANT_NUMBER;
	var.number = target->getID();

	return castRune(creature, var);
}

bool RuneSpell::castRune(Creature* creature, const Position& pos)
{
	LuaVariant var;
	var.type = VARIANT_POSITION;
	var.pos = pos;

	return castRune(creature, var);
}

bool RuneSpell::castRune(Creature* creature, const LuaVariant& var)
{
	bool result = false;
	
	if(m_scripted){
		result = executeCastRune(creature, var);
	}
	else{
		//call hardcodedAction
		result = false;
	}

	return result;
}

bool RuneSpell::executeCastRune(Creature* creature, const LuaVariant& var)
{
	//onCastSpell(cid, var)

	ScriptEnviroment* env = m_scriptInterface->getScriptEnv();
		
	#ifdef __DEBUG_LUASCRIPTS__
	std::stringstream desc;
	desc << "onCastSpell - " << creature->getName();
	env->setEventDesc(desc.str());
	#endif
		
	env->setScriptId(m_scriptId, m_scriptInterface);
	env->setRealPos(creature->getPosition());
		
	lua_State* L = m_scriptInterface->getLuaState();
	int size0 = lua_gettop(L);
		
	uint32_t cid = env->addThing(creature);

	LuaVariant* pVar = new LuaVariant(var);
	uint32_t variant = env->addVariant(pVar);

	m_scriptInterface->pushFunction(m_scriptId);
	lua_pushnumber(L, cid);
	lua_pushnumber(L, variant);
	
	bool isSuccess = true;

	long result;
	if(m_scriptInterface->callFunction(2, result) == false){
		isSuccess = false;
	}
	
	if(size0 != lua_gettop(L)){
		LuaScriptInterface::reportError(NULL, "Stack size changed!");
	}

	return isSuccess;
}

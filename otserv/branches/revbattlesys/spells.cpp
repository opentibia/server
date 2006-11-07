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
#include "commands.h"
#include "monsters.h"

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include <sstream>

extern Game g_game;
extern Monsters g_monsters;
extern Vocations g_vocations;
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
	else if(nodeName == "conjure"){
		return new ConjureSpell(&m_scriptInterface);
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

Spell* Spells::getSpellByName(const std::string& name)
{
	Spell* spell;

	if(spell = getRuneSpellByName(name)){
		return spell;
	}

	if(spell = getInstantSpellByName(name)){
		return spell;
	}

	return NULL;
}

RuneSpell* Spells::getRuneSpell(uint32_t id)
{
	RunesMap::iterator it = runes.find(id);
	if(it != runes.end()){
		return it->second;
	}

	return NULL;
}

RuneSpell* Spells::getRuneSpellByName(const std::string& name)
{
	for(RunesMap::iterator it = runes.begin(); it != runes.end(); ++it){
		if(strcasecmp(it->second->getName().c_str(), name.c_str()) == 0){
			return it->second;
		}
	}

	return NULL;
}
	
InstantSpell* Spells::getInstantSpell(const std::string& words)
{
	for(InstantsMap::iterator it = instants.begin(); it != instants.end(); ++it){
		if(strcasecmp(it->second->getWords().c_str(), words.c_str()) == 0){
			return it->second;
		}
	}

	return NULL;
}

InstantSpell* Spells::getInstantSpellByName(const std::string& name)
{
	for(InstantsMap::iterator it = instants.begin(); it != instants.end(); ++it){
		if(strcasecmp(it->second->getName().c_str(), name.c_str()) == 0){
			return it->second;
		}
	}

	return NULL;
}

Spell::Spell()
{
	level = 0;
	magLevel = 0;
	mana = 0;
	manaPercent = 0;
	soul = 0;
	exhaustion = false;
	needTarget = false;
	selfTarget = false;
	blocking = false;
	premium = false;
	enabled = true;
	isAggressive = true;
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

	if(readXMLInteger(p, "manaPercent", intValue)){
	 	manaPercent = intValue;
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

	if(readXMLInteger(p, "selftarget", intValue)){
		selfTarget = (intValue == 1);
	}

	if(readXMLInteger(p, "blocking", intValue)){
		blocking = (intValue == 1);
	}

	if(readXMLInteger(p, "aggressive", intValue)){
		isAggressive = (intValue == 1);
	}

	xmlNodePtr vocationNode = p->children;
	while(vocationNode){
		if(xmlStrcmp(vocationNode->name,(const xmlChar*)"vocation") == 0){
			if(readXMLString(vocationNode, "name", strValue)){
				int32_t vocationId = g_vocations.getVocationId(strValue);

				if(vocationId != -1){
					vocSpellMap[vocationId] = true;
				}
			}
		}
		
		vocationNode = vocationNode->next;
	}

	return true;
}

bool Spell::playerSpellCheck(const Player* player) const
{
	if(player->getAccessLevel() > 0){
		return true;
	}
	
	if(!enabled){
		return false;
	}
	
	if(isAggressive){
		if(player->getAccessLevel() == 0 && player->getTile()->isPz()){
			player->sendCancelMessage(RET_ACTIONNOTPERMITTEDINPROTECTIONZONE);
			return false;
		}
	}

	if(player->hasCondition(CONDITION_EXHAUSTED)){
		player->sendCancelMessage(RET_YOUAREEXHAUSTED);
		g_game.addMagicEffect(player->getPosition(), NM_ME_PUFF);
		return false;
	}

	if(player->getLevel() < level){
		player->sendCancelMessage(RET_NOTENOUGHLEVEL);
		g_game.addMagicEffect(player->getPosition(), NM_ME_PUFF);
		return false;
	}

	if(player->getMagicLevel() < magLevel){
		player->sendCancelMessage(RET_NOTENOUGHMAGICLEVEL);
		g_game.addMagicEffect(player->getPosition(), NM_ME_PUFF);
		return false;
	}

	if(player->getMana() < getManaCost(player)){
		player->sendCancelMessage(RET_NOTENOUGHMANA);
		g_game.addMagicEffect(player->getPosition(), NM_ME_PUFF);
		return false;
	}

	if(player->getPlayerInfo(PLAYERINFO_SOUL) < soul){		
		player->sendCancelMessage(RET_NOTENOUGHSOUL);
		g_game.addMagicEffect(player->getPosition(), NM_ME_PUFF);
		return false;
	}

	if(!vocSpellMap.empty()){
		if(vocSpellMap.find(player->getVocationId()) == vocSpellMap.end()){
			player->sendCancel("Your vocation cannot use this spell.");
			g_game.addMagicEffect(player->getPosition(), NM_ME_PUFF);
			return false;
		}
	}

	//TODO: check if the player knows the spell
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
			g_game.addMagicEffect(player->getPosition(), NM_ME_PUFF);
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
			g_game.addMagicEffect(player->getPosition(), NM_ME_PUFF);
			return false;
		}
	}

	return result;
}

void Spell::postCastSpell(Player* player) const
{
	if(player->getAccessLevel() > 0){
		return;
	}

	if(exhaustion){
		Condition* condition = Condition::createCondition(CONDITIONID_DEFAULT, CONDITION_EXHAUSTED, Spells::spellExhaustionTime, 0);
		player->addCondition(condition);
	}
	
	if(isAggressive && Spells::spellInFightTime != 0){
		Condition* condition = Condition::createCondition(CONDITIONID_DEFAULT, CONDITION_INFIGHT, Spells::spellInFightTime, 0);
		player->addCondition(condition);
	}

	int32_t manaCost = getManaCost(player);
	int32_t soulCost = 0;
	postCastSpell(player, manaCost, soulCost);
}

void Spell::postCastSpell(Player* player, uint32_t manaCost, uint32_t soulCost) const
{
	if(player->getAccessLevel() > 0){
		return;
	}

	if(manaCost > 0){
		player->changeMana(-manaCost);
		player->addManaSpent(manaCost);
		player->sendStats();
	}

	//TODO: reduce soul
	/*
	if(soulCoust > 0){
		player->changeSoul(-soul);
	}
	*/
}


int32_t Spell::getManaCost(const Player* player) const
{
	if(mana != 0){
		return mana;
	}

	if(manaPercent != 0){
		int32_t currentMana = player->getMana();
		int32_t manaCost = currentMana * (((double)manaPercent) / 100);
		return manaCost;
	}

	return 0;
}

int32_t Spell::getSoulCost(const Player* player) const
{
	if(soul != 0){
		return soul;
	}

	return 0;
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
	if(!Spell::configureSpell(p)){
		return false;
	}

	if(!TalkAction::configureEvent(p)){
		return false;
	}

	int intValue;
	
	if(readXMLInteger(p, "params", intValue)){
		if(intValue == 1)
	 		hasParam = true;
	}

	if(readXMLInteger(p, "direction", intValue)){
		needDirection = (intValue == 1);
	}

	return true;
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
	else if(functionName == "summonMonster"){
		function = SummonMonster;
	}
	else{
		return false;
	}
	
	m_scripted = false;
	return true;
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

bool InstantSpell::playerCastInstant(Player* player, const std::string& param)
{
	if(!playerSpellCheck(player)){
		return false;
	}

	LuaVariant var;

	if(selfTarget){
		var.type = VARIANT_NUMBER;
		var.number = player->getID();
	}
	else if(needTarget){
		Player* target = g_game.getPlayerByName(param);
		if(!target){
			player->sendCancelMessage(RET_PLAYERWITHTHISNAMEISNOTONLINE);
			g_game.addMagicEffect(player->getPosition(), NM_ME_PUFF);
			return false;
		}

		if(!g_game.canThrowObjectTo(player->getPosition(), target->getPosition())){
			player->sendCancelMessage(RET_PLAYERISNOTREACHABLE);
			g_game.addMagicEffect(player->getPosition(), NM_ME_PUFF);
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

	bool result = internalCastSpell(player, var);

	if(result){
		Spell::postCastSpell(player);
	}

	return result;
}

bool InstantSpell::castSpell(Creature* creature)
{
	LuaVariant var;
	var.type = VARIANT_POSITION;
	var.pos = getCasterPosition(creature);

	return internalCastSpell(creature, var);
}

bool InstantSpell::castSpell(Creature* creature, Creature* target)
{
	if(needTarget){
		LuaVariant var;
		var.type = VARIANT_NUMBER;
		var.number = target->getID();

		return internalCastSpell(creature, var);
	}
	else{
		return castSpell(creature);
	}
}

bool InstantSpell::internalCastSpell(Creature* creature, const LuaVariant& var)
{
	bool result = false;
	
	if(m_scripted){
		result =  executeCastSpell(creature, var);
	}
	else{
		if(function){
			result = function(this, creature, var.text);
		}
	}

	return result;
}

bool InstantSpell::executeCastSpell(Creature* creature, const LuaVariant& var)
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

	int32_t result;
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

bool InstantSpell::HouseGuestList(const InstantSpell* spell, Creature* creature, const std::string& param)
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
		g_game.addMagicEffect(player->getPosition(), NM_ME_PUFF);
	}
	
	return true;
}

bool InstantSpell::HouseSubOwnerList(const InstantSpell* spell, Creature* creature, const std::string& param)
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
		g_game.addMagicEffect(player->getPosition(), NM_ME_PUFF);
	}
		
	return true;
}

bool InstantSpell::HouseDoorList(const InstantSpell* spell, Creature* creature, const std::string& param)
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
		g_game.addMagicEffect(player->getPosition(), NM_ME_PUFF);
	}
	
	return true;
}

bool InstantSpell::HouseKick(const InstantSpell* spell, Creature* creature, const std::string& param)
{
	House* house = getHouseFromPos(creature);
	if(!house)
		return false;
	
	Player* player = creature->getPlayer();
	house->kickPlayer(player, param);
	
	return true;
}

bool InstantSpell::SearchPlayer(const InstantSpell* spell, Creature* creature, const std::string& param)
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
		g_game.addMagicEffect(player->getPosition(), NM_ME_MAGIC_ENERGIE);
		return true;
	}
	else{
		player->sendCancelMessage(RET_PLAYERWITHTHISNAMEISNOTONLINE);
		g_game.addMagicEffect(player->getPosition(), NM_ME_PUFF);
	}

	return false;
}

bool InstantSpell::SummonMonster(const InstantSpell* spell, Creature* creature, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(!player){
		return false;
	}

	uint32_t mId = g_monsters.getIdByName(param);
	if(!mId){
		player->sendCancelMessage(RET_NOTPOSSIBLE);
		g_game.addMagicEffect(player->getPosition(), NM_ME_PUFF);
		return false;
	}

	MonsterType* mType = g_monsters.getMonsterType(mId);
	if(!mType){
		player->sendCancelMessage(RET_NOTPOSSIBLE);
		g_game.addMagicEffect(player->getPosition(), NM_ME_PUFF);
		return false;
	}

	if(player->getAccessLevel() == 0 && !mType->isSummonable){
		player->sendCancelMessage(RET_NOTPOSSIBLE);
		g_game.addMagicEffect(player->getPosition(), NM_ME_PUFF);
		return false;
	}

	int32_t manaCost = mType->manaSummonCost;
	if(player->getMana() < manaCost){
		player->sendCancelMessage(RET_NOTENOUGHMANA);
		g_game.addMagicEffect(player->getPosition(), NM_ME_PUFF);
		return false;
	}

	if(player->getSummonCount() >= 2){
		player->sendCancel("You cannot summon more creatures.");
		g_game.addMagicEffect(player->getPosition(), NM_ME_PUFF);
		return false;
	}

	ReturnValue ret = Commands::placeSummon(creature, param);

	if(ret == RET_NOERROR){
		spell->postCastSpell(player, manaCost, spell->getSoulCost(player));
		g_game.addMagicEffect(player->getPosition(), NM_ME_MAGIC_POISEN);
	}
	else{
		player->sendCancelMessage(ret);
		g_game.addMagicEffect(player->getPosition(), NM_ME_PUFF);
	}

	return (ret == RET_NOERROR);
}

ConjureSpell::ConjureSpell(LuaScriptInterface* _interface) :
InstantSpell(_interface)
{
	conjureId = 0;
	conjureCount = 1;
	conjureReagentId = 0;
}

ConjureSpell::~ConjureSpell()
{
	//
}

std::string ConjureSpell::getScriptEventName()
{
	return "onCastSpell";
}

bool ConjureSpell::configureEvent(xmlNodePtr p)
{
	if(!InstantSpell::configureEvent(p)){
		return false;
	}

	/*
	if(!Spell::configureSpell(p)){
		return false;
	}

	if(!TalkAction::configureEvent(p)){
		return false;
	}
	*/

	int intValue;
	
	if(readXMLInteger(p, "conjureId", intValue)){
		conjureId = intValue;
	}

	if(readXMLInteger(p, "conjureCount", intValue)){
		conjureCount = intValue;
	}

	if(readXMLInteger(p, "reagentId", intValue)){
		conjureReagentId = intValue;
	}	

	return true;
}

bool ConjureSpell::loadFunction(const std::string& functionName)
{
	if(functionName == "conjureItem"){
		function = ConjureItem;
	}
	else if(functionName == "conjureRune"){
		function = ConjureItem;
	}
	else if(functionName == "conjureFood"){
		function = ConjureFood;
	}
	else{
		return false;
	}
	
	m_scripted = false;
	return true;
}

bool ConjureSpell::internalConjureItem(Player* player, uint32_t conjureId, uint32_t conjureCount)
{
	Item* newItem = Item::CreateItem(conjureId, conjureCount);
	if(!newItem){
		return false;
	}

	ReturnValue ret = g_game.internalPlayerAddItem(player, newItem);
	return (ret == RET_NOERROR);
}

bool ConjureSpell::internalConjureItem(Player* player, uint32_t conjureId, uint32_t conjureCount, uint32_t reagentId, slots_t slot)
{
	bool result = false;
	if(reagentId != 0){
		Item* item;

		item = player->getInventoryItem(slot);
		if(item && item->getID() == reagentId){
			
			g_game.transformItem(item, conjureId, conjureCount);
			result = true;
		}
	}

	return result;
}

bool ConjureSpell::ConjureItem(const ConjureSpell* spell, Creature* creature, const std::string& param)
{
	Player* player = creature->getPlayer();

	if(!player){
		return false;
	}
	
	bool result = false;

	if(spell->getReagentId() != 0){
		
		if(!spell->playerSpellCheck(player)){
			return false;
		}

		if(internalConjureItem(player, spell->getConjureId(), spell->getConjureCount(),
		spell->getReagentId(), SLOT_LEFT)){
			spell->postCastSpell(player);
			result = true;
		}

		if(!spell->playerSpellCheck(player)){
			return false;
		}

		if(internalConjureItem(player, spell->getConjureId(), spell->getConjureCount(),
		spell->getReagentId(), SLOT_RIGHT)){
			spell->postCastSpell(player);
			result = true;
		}
	}
	else{
		if(internalConjureItem(player, spell->getConjureId(), spell->getConjureId())){
			spell->postCastSpell(player);
			result = true;
		}
	}

	if(result){
		g_game.addMagicEffect(player->getPosition(), NM_ME_MAGIC_BLOOD);
	}
	else if(spell->getReagentId() != 0){
		player->sendCancel("You need a magic item to cast this spell.");
		g_game.addMagicEffect(player->getPosition(), NM_ME_PUFF);
	}

	return result;
}

bool ConjureSpell::ConjureFood(const ConjureSpell* spell, Creature* creature, const std::string& param)
{
	Player* player = creature->getPlayer();

	if(!player){
		return false;
	}

	uint32_t foodType = 0;
	switch(rand() % 7){
		case 0: foodType = ITEM_MEAT; break;
		case 1: foodType = ITEM_HAM; break;
		case 2: foodType = ITEM_GRAPE; break;
		case 3: foodType = ITEM_APPLE; break;
		case 4: foodType = ITEM_BREAD; break;
		case 5: foodType = ITEM_CHEESE; break;
		case 6: foodType = ITEM_ROLL; break;
		case 7: foodType = ITEM_BREAD; break;
	}

	bool result = internalConjureItem(player, foodType, 1);

	if(result){
		g_game.addMagicEffect(player->getPosition(), NM_ME_MAGIC_POISEN);
	}

	return result;
}

bool ConjureSpell::playerCastInstant(Player* player, const std::string& param)
{
	if(!playerSpellCheck(player)){
		return false;
	}

	bool result = false;
	
	if(m_scripted){
		LuaVariant var;
		var.type = VARIANT_STRING;
		var.text = param;
		result =  executeCastSpell(player, var);
	}
	else{
		if(function){
			result = function(this, player, param);
		}
	}

	return result;
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
	if(!Spell::configureSpell(p)){
		return false;
	}
	
	if(!Action::configureEvent(p)){
		return false;
	}

	int intValue;
	if(readXMLInteger(p, "id", intValue)){
		runeId = intValue;
	}
	else{
		std::cout << "Error: [RuneSpell::configureSpell] Rune spell without id." << std::endl;
		return false;
	}

	uint32_t charges = 0;
	if(readXMLInteger(p, "charges", intValue)){
		charges = intValue;
	}

	hasCharges = (charges > 0);

	if(magLevel != 0){
		//Change magic level in the ItemType to get accurate description
		ItemType& iType = Item::items.getItemType(runeId);
		iType.runeMagLevel = magLevel;
		iType.charges = charges;
	}

	return true;
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

bool RuneSpell::executeUse(Player* player, Item* item, const PositionEx& posFrom, const PositionEx& posTo, bool extendedUse)
{
	if(!playerRuneSpellCheck(player, posTo)){
		return false;
	}

	LuaVariant var;
	var.type = VARIANT_POSITION;
	var.pos = posTo;

	bool result = internalCastSpell(player, var);

	if(result){
		Spell::postCastSpell(player);
		
		if(hasCharges && item){
			int32_t newCharge = std::max(0, item->getItemCharge() - 1);
			g_game.transformItem(item, item->getID(), newCharge);
		}
	}

	return result;
}

bool RuneSpell::castSpell(Creature* creature)
{
	LuaVariant var;
	var.type = VARIANT_NUMBER;
	var.number = creature->getID();

	return internalCastSpell(creature, var);
}

bool RuneSpell::castSpell(Creature* creature, Creature* target)
{
	LuaVariant var;
	var.type = VARIANT_NUMBER;
	var.number = target->getID();

	return internalCastSpell(creature, var);
}

bool RuneSpell::internalCastSpell(Creature* creature, const LuaVariant& var)
{
	bool result = false;
	
	if(m_scripted){
		result = executeCastSpell(creature, var);
	}
	else{
		//call hardcodedAction
		result = false;
	}

	return result;
}

bool RuneSpell::executeCastSpell(Creature* creature, const LuaVariant& var)
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

	int32_t result;
	if(m_scriptInterface->callFunction(2, result) == false){
		isSuccess = false;
	}
	
	if(size0 != lua_gettop(L)){
		LuaScriptInterface::reportError(NULL, "Stack size changed!");
	}

	return isSuccess;
}

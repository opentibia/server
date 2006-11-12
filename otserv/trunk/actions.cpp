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

#include "definitions.h"
#include "const78.h"
#include "player.h"
#include "monster.h"
#include "npc.h"
#include "game.h"
#include "item.h"
#include "container.h"
#include "combat.h"
#include "depot.h"
#include "house.h"
#include "tasks.h"
#include "tools.h"
#include "spells.h"

#include <libxml/xmlmemory.h>
#include <libxml/parser.h> 

//#include <boost/bind.hpp>
#include <sstream>

#include "actions.h"

extern Game g_game;
extern Spells* g_spells;

Actions::Actions() :
m_scriptInterface("Action Interface")
{
	m_scriptInterface.initState();
}

Actions::~Actions()
{
	clear();
}

inline void Actions::clearMap(ActionUseMap& map)
{
	ActionUseMap::iterator it = map.begin();
	while(it != map.end()){
		delete it->second;
		map.erase(it);
		it = map.begin();
	}
}

void Actions::clear()
{
	clearMap(useItemMap);
	clearMap(uniqueItemMap);
	clearMap(actionItemMap);
	
	m_scriptInterface.reInitState();
	m_loaded = false;
}

LuaScriptInterface& Actions::getScriptInterface()
{
	return m_scriptInterface;	
}

std::string Actions::getScriptBaseName()
{
	return "actions";	
}

Event* Actions::getEvent(const std::string& nodeName)
{
	if(nodeName == "action"){
		return new Action(&m_scriptInterface);
	}
	else{
		return NULL;
	}
}

bool Actions::registerEvent(Event* event, xmlNodePtr p)
{
	Action* action = dynamic_cast<Action*>(event);
	if(!action)
		return false;
	
	int value;
	if(readXMLInteger(p,"itemid",value)){
		useItemMap[value] = action;
	}
	else if(readXMLInteger(p,"uniqueid",value)){
		uniqueItemMap[value] = action;
	}
	else if(readXMLInteger(p,"actionid",value)){
		actionItemMap[value] = action;
	}
	else{
		return false;
	}
	
	return true;
}

ReturnValue Actions::canUse(const Creature* creature, const Position& pos)
{
	const Position& creaturePos = creature->getPosition();

	if(pos.x != 0xFFFF){
		if(creaturePos.z > pos.z){
			return RET_FIRSTGOUPSTAIRS;
		}
		else if(creaturePos.z < pos.z){
			return RET_FIRSTGODOWNSTAIRS;
		}
		else if(!Position::areInRange<1,1,0>(creaturePos, pos)){
			return RET_TOOFARAWAY;
		}
	}

	return RET_NOERROR;
}

ReturnValue Actions::canUseFar(const Creature* creature, const Position& toPos, const bool blockWalls)
{
	if(toPos.x == 0xFFFF){
		return RET_NOERROR;
	}

	const Position& creaturePos = creature->getPosition();

	if(creaturePos.z > toPos.z){
		return RET_FIRSTGOUPSTAIRS;
	}
	else if(creaturePos.z < toPos.z){
		return RET_FIRSTGODOWNSTAIRS;
	}
	else if(!Position::areInRange<7,5,0>(toPos, creaturePos)){
		return RET_TOOFARAWAY;
	}
	
	if(blockWalls && canUse(creature, toPos) == RET_TOOFARAWAY && 
		!g_game.map->canThrowObjectTo(creaturePos, toPos)){
			return RET_CANNOTTHROW;
	}

	return RET_NOERROR;
}

Action *Actions::getAction(const Item* item)
{
	if(item->getUniqueId() != 0){
		ActionUseMap::iterator it = uniqueItemMap.find(item->getUniqueId());
    	if (it != uniqueItemMap.end()){
			return it->second;
		}
	}
	if(item->getActionId() != 0){
		ActionUseMap::iterator it = actionItemMap.find(item->getActionId());
    	if (it != actionItemMap.end()){
	    	return it->second;
		}
	}
	ActionUseMap::iterator it = useItemMap.find(item->getID());
    if (it != useItemMap.end()){
	   	return it->second;
	}

	//rune items
	Action* runeSpell = g_spells->getRuneSpell(item->getID());
	if(runeSpell){
		return runeSpell;
	}
	
	return NULL;
}

bool Actions::useItem(Player* player, const Position& pos, uint8_t index, Item* item)
{	
	//check if it is a house door
	if(Door* door = item->getDoor()){
		if(door->canUse(player) == false){
			player->sendCancelMessage(RET_CANNOTUSETHISOBJECT);
			return false;
		}
	}
	
	//look for the item in action maps	
	Action* action = getAction(item);
	
	//if found execute it
	if(action){
		int32_t stack = item->getParent()->__getIndexOfThing(item);
		PositionEx posEx(pos, stack);
		if(action->executeUse(player, item, posEx, posEx, false)){
			return true;
		}
	}
	
	//can we read it?
	int maxlen;
	int RWInfo = item->getRWInfo(maxlen);
	if(RWInfo & CAN_BE_READ){
		if(RWInfo & CAN_BE_WRITTEN){
			player->sendTextWindow(item, maxlen, true);
		}
		else{
			player->sendTextWindow(item, 0, false);
		}
		return true;
	}
	
	//if it is a container try to open it
	if(Container* container = item->getContainer()){
		if(openContainer(player, container, index))
			return true;
	}
    
	//we dont know what to do with this item
	player->sendCancelMessage(RET_CANNOTUSETHISOBJECT);
	return false;	
}

bool Actions::openContainer(Player* player, Container* container, const unsigned char index)
{
	Container* openContainer = NULL;

	//depot container
	if(Depot* depot = container->getDepot()){
		Depot* myDepot = player->getDepot(depot->getDepotId(), true);
		myDepot->setParent(depot->getParent());
		openContainer = myDepot;
	}
	else{
		openContainer = container;
	}

	//open/close container
	int32_t oldcid = player->getContainerID(openContainer);
	if(oldcid != -1){
		player->onCloseContainer(openContainer);
		player->closeContainer(oldcid);
	}
	else{
		player->addContainer(index, openContainer);
		player->onSendContainer(openContainer);
	}

	return true;
}

bool Actions::useItemEx(Player* player, const Position& from_pos,
	const Position& to_pos, const unsigned char to_stack, Item* item)
{
	Action* action = getAction(item);
	
	if(action){
		bool ret = action->canExecuteAction(player, to_pos);
		if(ret){
			int32_t from_stack = item->getParent()->__getIndexOfThing(item);
			PositionEx posFromEx(from_pos, from_stack);
			PositionEx posToEx(to_pos, to_stack);
			if(action->executeUse(player, item, posFromEx, posToEx, true)){
				return true;
			}
		}

		return ret;
	}
	else{
		//not found
		player->sendCancelMessage(RET_CANNOTUSETHISOBJECT);
		return false;
	}
}

Action::Action(LuaScriptInterface* _interface) :
Event(_interface)
{
	allowfaruse = false;
	blockwalls = true;
}

Action::~Action()
{
	//
}

bool Action::configureEvent(xmlNodePtr p)
{
	int intValue;
	if(readXMLInteger(p, "allowfaruse", intValue)){
		if(intValue != 0){
			setAllowFarUse(true);
		}
	}
	if(readXMLInteger(p, "blockwalls", intValue)){
		if(intValue == 0){
			setBlockWalls(false);
		}
	}
	
	return true;
}

std::string Action::getScriptEventName()
{
	return "onUse";
}

bool Action::canExecuteAction(const Player* player, const Position& toPos)
{
	ReturnValue ret = RET_NOERROR;

	if(allowFarUse() == false){
		if((ret = Actions::canUse(player, toPos)) != RET_NOERROR){
			player->sendCancelMessage(ret);
			return false;
		}
	}
	else{
		ret = Actions::canUseFar(player, toPos, blockWalls());
		if(ret != RET_NOERROR){
			player->sendCancelMessage(ret);
			return false;
		}
	}
	return true;
}

bool Action::executeUse(Player* player, Item* item, const PositionEx& posFrom, const PositionEx& posTo, bool extendedUse)
{
	//onUse(cid, item1, position1, item2, position2)
	ScriptEnviroment* env = m_scriptInterface->getScriptEnv();
	
	#ifdef __DEBUG_LUASCRIPTS__
	std::stringstream desc;
	desc << player->getName() << " - " << item->getID() << " " << posFrom << "|" << posTo;
	env->setEventDesc(desc.str());
	#endif
	
	env->setScriptId(m_scriptId, m_scriptInterface);
	env->setRealPos(player->getPosition());
	
	long cid = env->addThing(player);
	long itemid1 = env->addThing(item);
	
	lua_State* L = m_scriptInterface->getLuaState();
	
	m_scriptInterface->pushFunction(m_scriptId);
	lua_pushnumber(L, cid);
	LuaScriptInterface::pushThing(L, item, itemid1);
	LuaScriptInterface::pushPosition(L, posFrom, posFrom.stackpos);
	//std::cout << "posTo" <<  (Position)posTo << " stack" << (int)posTo.stackpos <<std::endl;
	Thing* thing = g_game.internalGetThing(player, posTo, posTo.stackpos);
	if(thing && (!extendedUse || thing != item)){
		long thingId2 = env->addThing(thing);
		LuaScriptInterface::pushThing(L, thing, thingId2);
		LuaScriptInterface::pushPosition(L, posTo, posTo.stackpos);
	}
	else{
		LuaScriptInterface::pushThing(L, NULL, 0);
		Position posEx;
		LuaScriptInterface::pushPosition(L, posEx, 0);
	}
	
	int32_t ret;
	if(m_scriptInterface->callFunction(5, ret) == false){
		ret = 0;
	}
	
	return (ret != 0);
}

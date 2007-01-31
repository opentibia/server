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
#include "const79.h"
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
#include "configmanager.h"

#include <libxml/xmlmemory.h>
#include <libxml/parser.h> 

//#include <boost/bind.hpp>
#include <sstream>

#include "actions.h"

extern Game g_game;
extern Spells* g_spells;
extern ConfigManager g_config;

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

Action* Actions::getAction(const Item* item)
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

ReturnValue Actions::internalUseItem(Player* player, const Position& pos, uint8_t index, Item* item)
{	
	//check if it is a house door
	if(Door* door = item->getDoor()){
		if(!door->canUse(player)){
			return RET_CANNOTUSETHISOBJECT;
		}
	}
	
	//look for the item in action maps	
	Action* action = getAction(item);
	
	//if found execute it
	if(action){
		int32_t stack = item->getParent()->__getIndexOfThing(item);
		PositionEx posEx(pos, stack);
		if(action->executeUse(player, item, posEx, posEx, false)){
			return RET_NOERROR;
		}
	}
	
	if(item->isReadable()){
		if(item->canWriteText()){
			player->sendTextWindow(item, item->getMaxWriteLength(), true);
		}
		else{
			player->sendTextWindow(item, 0, false);
		}

		return RET_NOERROR;
	}
	
	//if it is a container try to open it
	if(Container* container = item->getContainer()){
		if(openContainer(player, container, index)){
			return RET_NOERROR;
		}
	}
    
	//we dont know what to do with this item
	return RET_CANNOTUSETHISOBJECT;
}

bool Actions::useItem(Player* player, const Position& pos, uint8_t index, Item* item, bool isHotkey)
{	
	if(OTSYS_TIME() - player->getLastAction() < g_config.getNumber(ConfigManager::MIN_ACTIONTIME)){
		return false;
	}
	
	ReturnValue ret;
	if(isHotkey){
		uint32_t itemCount = item->getParent()->__getItemTypeCount(item->getID(), item->getSubType(), false);
	
		ret = internalUseItem(player, pos, index, item);
		
		if(ret == RET_NOERROR){
			std::stringstream ss;
			if(itemCount == 1){
				ss << "Using the last " << item->getName() << "...";
			}
			else{
				ss << "Using one of " << itemCount << " " << item->getName() << "s..."; 
			}
			player->sendTextMessage(MSG_INFO_DESCR, ss.str());
		}
	}
	else{
		ret = internalUseItem(player, pos, index, item);
	}
	
	if(ret == RET_NOERROR){
		player->setLastAction(OTSYS_TIME());
		return true;
	}
	else{
		player->sendCancelMessage(ret);
		return false;
	}
}

bool Actions::useItemEx(Player* player, const Position& fromPos, const Position& toPos,
	const unsigned char toStackPos, Item* item, bool isHotkey)
{
	if(OTSYS_TIME() - player->getLastAction() < g_config.getNumber(ConfigManager::MIN_ACTIONTIME)){
		return false;
	}

	Action* action = getAction(item);
	
	if(!action){
		player->sendCancelMessage(RET_CANNOTUSETHISOBJECT);
		return false;
	}

	ReturnValue ret = action->canExecuteAction(player, toPos);
	if(ret != RET_NOERROR){
		player->sendCancelMessage(ret);
		return false;
	}

	uint32_t itemCount = item->getParent()->__getItemTypeCount(item->getID(), item->getSubType(), false);
	int32_t fromStackPos = item->getParent()->__getIndexOfThing(item);
	PositionEx fromPosEx(fromPos, fromStackPos);
	PositionEx toPosEx(toPos, toStackPos);

	if(!action->executeUse(player, item, fromPosEx, toPosEx, true)){
		player->sendCancelMessage(RET_CANNOTUSETHISOBJECT);
		return false;
	}

	if(isHotkey){
		std::stringstream ss;
		if(itemCount == 1){
			ss << "Using the last " << item->getName() << "...";
		}
		else{
			ss << "Using one of " << itemCount << " " << item->getName() << "s..."; 
		}
		
		player->sendTextMessage(MSG_INFO_DESCR, ss.str());
	}

	player->setLastAction(OTSYS_TIME());
	return true;
}

bool Actions::useItemEx(Player* player, Item* item, Creature* creature, bool isHotkey)
{
	if(OTSYS_TIME() - player->getLastAction() < g_config.getNumber(ConfigManager::MIN_ACTIONTIME)){
		return false;
	}

	Action* action = getAction(item);
	
	if(!action){
		player->sendCancelMessage(RET_CANNOTUSETHISOBJECT);
		return false;
	}

	uint32_t itemCount = item->getParent()->__getItemTypeCount(item->getID(), item->getSubType(), false);
	PositionEx fromPosEx(item->getPosition(), item->getParent()->__getIndexOfThing(item));
	PositionEx toPosEx(creature->getPosition(), creature->getParent()->__getIndexOfThing(creature));

	if(!action->executeUse(player, item, fromPosEx, toPosEx, true)){
		player->sendCancelMessage(RET_CANNOTUSETHISOBJECT);
		return false;
	}

	if(isHotkey){
		std::stringstream ss;
		if(itemCount == 1){
			ss << "Using the last " << item->getName() << "...";
		}
		else{
			ss << "Using one of " << itemCount << " " << item->getName() << "s..."; 
		}
		
		player->sendTextMessage(MSG_INFO_DESCR, ss.str());
	}

	player->setLastAction(OTSYS_TIME());
	return true;
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

ReturnValue Action::canExecuteAction(const Player* player, const Position& toPos)
{
	ReturnValue ret = RET_NOERROR;

	if(!allowFarUse()){
		ret = Actions::canUse(player, toPos);
		if(ret != RET_NOERROR){
			return ret;
		}
	}
	else{
		ret = Actions::canUseFar(player, toPos, blockWalls());
		if(ret != RET_NOERROR){
			return ret;
		}
	}

	return RET_NOERROR;
}

bool Action::executeUse(Player* player, Item* item,
	const PositionEx& fromPos, const PositionEx& toPos, bool extendedUse)
{
	//onUse(cid, item1, position1, item2, position2)
	if(m_scriptInterface->reserveScriptEnv()){
		ScriptEnviroment* env = m_scriptInterface->getScriptEnv();
	
		#ifdef __DEBUG_LUASCRIPTS__
		std::stringstream desc;
		desc << player->getName() << " - " << item->getID() << " " << fromPos << "|" << toPos;
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
		LuaScriptInterface::pushPosition(L, fromPos, fromPos.stackpos);
		//std::cout << "posTo" <<  (Position)posTo << " stack" << (int)posTo.stackpos <<std::endl;
		Thing* thing = g_game.internalGetThing(player, toPos, toPos.stackpos);
		if(thing && (!extendedUse || thing != item)){
			long thingId2 = env->addThing(thing);
			LuaScriptInterface::pushThing(L, thing, thingId2);
			LuaScriptInterface::pushPosition(L, toPos, toPos.stackpos);
		}
		else{
			LuaScriptInterface::pushThing(L, NULL, 0);
			Position posEx;
			LuaScriptInterface::pushPosition(L, posEx, 0);
		}
	
		int32_t result = m_scriptInterface->callFunction(5);
		m_scriptInterface->releaseScriptEnv();
		
		return (result == LUA_TRUE);
	}
	else{
		std::cout << "[Error] Call stack overflow. Action::executeUse" << std::endl;
		return false;
	}
}

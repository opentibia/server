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


#include "game.h"
#include "creature.h"
#include "player.h"
#include "tile.h"
#include <sstream>
#include "tools.h"
#include "combat.h"

#include <libxml/xmlmemory.h>
#include <libxml/parser.h> 

#include "movement.h"

extern Game g_game;

MoveEvents::MoveEvents() :
m_scriptInterface("MoveEvents Interface")
{
	m_scriptInterface.initState();
}

MoveEvents::~MoveEvents()
{
	clear();
}

void MoveEvents::clear()
{
	MoveListMap::iterator it = m_itemIdMap.begin();
	while(it != m_itemIdMap.end()){
		for(int i = 0; i < MOVE_EVENT_LAST; ++i){
			delete it->second.event[i];
		}
		m_itemIdMap.erase(it);
		it = m_itemIdMap.begin();
	}
	
	it = m_actionIdMap.begin();
	while(it != m_actionIdMap.end()){
		for(int i = 0; i < MOVE_EVENT_LAST; ++i){
			delete it->second.event[i];
		}
		m_actionIdMap.erase(it);
		it = m_actionIdMap.begin();
	}
	
	it = m_uniqueIdMap.begin();
	while(it != m_uniqueIdMap.end()){
		for(int i = 0; i < MOVE_EVENT_LAST; ++i){
			delete it->second.event[i];
		}
		m_uniqueIdMap.erase(it);
		it = m_uniqueIdMap.begin();
	}
	
	m_scriptInterface.reInitState();
}


LuaScriptInterface& MoveEvents::getScriptInterface()
{
	return m_scriptInterface;	
}
	
std::string MoveEvents::getScriptBaseName()
{
	return "movements";	
}

Event* MoveEvents::getEvent(const std::string& nodeName)
{
	if(nodeName == "movevent"){
		return new MoveEvent(&m_scriptInterface);
	}
	else{
		return NULL;
	}
}
	
bool MoveEvents::registerEvent(Event* event, xmlNodePtr p)
{
	MoveEvent* moveEvent = dynamic_cast<MoveEvent*>(event);
	if(!moveEvent)
		return false;
		
	bool success = true;
	int id;
	std::string str;
	
	MoveEvent_t eventType = moveEvent->getEventType();
	if(eventType == MOVE_EVENT_ADD_ITEM || eventType == MOVE_EVENT_REMOVE_ITEM){
		if(readXMLInteger(p,"tileitem",id) && id == 1){
			switch(eventType){
			case MOVE_EVENT_ADD_ITEM:
				moveEvent->setEventType(MOVE_EVENT_ADD_ITEM_ITEMTILE);
				break;
			case MOVE_EVENT_REMOVE_ITEM:
				moveEvent->setEventType(MOVE_EVENT_REMOVE_ITEM_ITEMTILE);
				break;
			default:
				break;
			}			
		}
	}
	
	if(readXMLInteger(p,"itemid",id)){
		addEvent(moveEvent, id, m_itemIdMap);
	}
	else if(readXMLInteger(p,"uniqueid",id)){
		addEvent(moveEvent, id, m_uniqueIdMap);
	}
	else if(readXMLInteger(p,"actionid",id)){
		addEvent(moveEvent, id, m_actionIdMap);
	}
	else if(readXMLString(p,"position",str)){
		//TODO
		success = false;
	}
	else{
		success = false;
	}
	return success;
}
	
void MoveEvents::addEvent(MoveEvent* event, long id, MoveListMap& map)
{
	MoveListMap::iterator it = map.find(id);
	if(it == map.end()){
		MoveEventList moveEventList;
		moveEventList.event[event->getEventType()] = event;
		map[id] = moveEventList;
	}
	else{
		it->second.event[event->getEventType()] = event;
	}
	return;
}

MoveEvent* MoveEvents::getEvent(Item* item, MoveEvent_t eventType)
{
	MoveListMap::iterator it;
	if(item->getUniqueId() != 0){
		it = m_uniqueIdMap.find(item->getUniqueId());
    	if(it != m_uniqueIdMap.end()){
			return it->second.event[eventType];
		}
	}
	if(item->getActionId() != 0){
		it = m_actionIdMap.find(item->getActionId());
    	if(it != m_actionIdMap.end()){
	    	return it->second.event[eventType];
		}
	}
	
	it = m_itemIdMap.find(item->getID());
    if(it != m_itemIdMap.end()){
	   	return it->second.event[eventType];
	}
	
	return NULL;
}

long MoveEvents::onCreatureMove(Creature* creature, Tile* tile, bool isIn)
{
	MoveEvent_t eventType;
	if(isIn){
		eventType = MOVE_EVENT_STEP_IN;
	}
	else{
		eventType = MOVE_EVENT_STEP_OUT;
	}
	
	long ret = 1;
	long j = tile->__getLastIndex();
	Item* tileItem = NULL;
	for(long i = tile->__getFirstIndex(); i < j; ++i){
		Thing* thing = tile->__getThing(i);
		if(thing && (tileItem = thing->getItem())){
			MoveEvent* event = getEvent(tileItem, eventType);
			if(event){
				ret = ret & event->fireStepEvent(creature, tileItem, tile->getPosition());
			}
		}
	}
	return ret;
}

long MoveEvents::onPlayerEquip(Player* player, Item* item, long slot, bool isEquip)
{
	MoveEvent_t eventType;
	if(isEquip){
		eventType = MOVE_EVENT_EQUIP;
	}
	else{
		eventType = MOVE_EVENT_DEEQUIP;
	}
	
	MoveEvent* event = getEvent(item, eventType);
	if(event){
		return event->executeEquip(player, item, slot);
	}
	return 1;
}

long MoveEvents::onItemMove(Item* item, Tile* tile, bool isAdd)
{
	MoveEvent_t eventType1;
	MoveEvent_t eventType2;
	if(isAdd){
		eventType1 = MOVE_EVENT_ADD_ITEM;
		eventType2 = MOVE_EVENT_ADD_ITEM_ITEMTILE;
	}
	else{
		eventType1 = MOVE_EVENT_REMOVE_ITEM;
		eventType2 = MOVE_EVENT_REMOVE_ITEM_ITEMTILE;
	}
	
	long ret = 1;
	MoveEvent* event = getEvent(item, eventType1);
	if(event){
		ret = ret & event->fireAddRemItem(item, NULL, tile->getPosition());
	}
	
	long j = tile->__getLastIndex();
	Item* tileItem = NULL;
	for(long i = tile->__getFirstIndex(); i < j; ++i){
		Thing* thing = tile->__getThing(i);
		if(thing && (tileItem = thing->getItem()) && (tileItem != item)){
			MoveEvent* event = getEvent(tileItem, eventType2);
			if(event){
				ret = ret & event->fireAddRemItem(item, tileItem, tile->getPosition());
			}
		}
	}
	
	return ret;
}


MoveEvent::MoveEvent(LuaScriptInterface* _interface) :
Event(_interface)
{
	m_eventType = MOVE_EVENT_NONE;
	stepFunction = NULL;
	moveFunction = NULL;
}

MoveEvent::~MoveEvent()
{
	//
}

std::string MoveEvent::getScriptEventName()
{
	switch(m_eventType){
	case MOVE_EVENT_STEP_IN:
		return "onStepIn";
		break;
	case MOVE_EVENT_STEP_OUT:
		return "onStepOut";
		break;
	case MOVE_EVENT_EQUIP:
		return "onEquip";
		break;
	case MOVE_EVENT_DEEQUIP:
		return "onDeEquip";
		break;
	case MOVE_EVENT_ADD_ITEM:
		return "onAddItem";
		break;
	case MOVE_EVENT_REMOVE_ITEM:
		return "onRemoveItem";
		break;
	default:
		std::cout << "Error: [MoveEvent::getScriptEventName()] No valid event type." <<  std::endl;
		return "";
		break;
	};
}

bool MoveEvent::configureEvent(xmlNodePtr p)
{
	std::string str;
	if(readXMLString(p, "event", str)){
		if(str == "StepIn"){
			m_eventType = MOVE_EVENT_STEP_IN;
		}
		else if(str == "StepOut"){
			m_eventType = MOVE_EVENT_STEP_OUT;
		}
		else if(str == "Equip"){
			m_eventType = MOVE_EVENT_EQUIP;
		}
		else if(str == "DeEquip"){
			m_eventType = MOVE_EVENT_DEEQUIP;
		}
		else if(str == "AddItem"){
			m_eventType = MOVE_EVENT_ADD_ITEM;
		}
		else if(str == "RemoveItem"){
			m_eventType = MOVE_EVENT_REMOVE_ITEM;
		}
		else{
			std::cout << "Error: [MoveEvent::configureMoveEvent] No valid event name " << str << std::endl;
			return false;
		}
	}
	else{
		std::cout << "Error: [MoveEvent::configureMoveEvent] No event found." << std::endl;
		return false;
	}
	return true;
}

bool MoveEvent::loadFunction(const std::string& functionName)
{
	if(functionName == "onStepInField"){
		stepFunction = StepInField;
	}
	else if(functionName == "onStepOutField"){
		stepFunction = StepOutField;
	}
	else if(functionName == "onAddField"){
		moveFunction = AddItemField;
	}
	else if(functionName == "onRemoveField"){
		moveFunction = RemoveItemField;
	}
	else{
		return false;
	}
	
	m_scripted = false;
	return true;
}

MoveEvent_t MoveEvent::getEventType() const
{
	if(m_eventType == MOVE_EVENT_NONE){
		std::cout << "Error: [MoveEvent::getEventType()] MOVE_EVENT_NONE" << std::endl;
		return (MoveEvent_t)0;
	}
	return m_eventType;
}

void MoveEvent::setEventType(MoveEvent_t type)
{
	m_eventType = type;
}

long MoveEvent::StepInField(Creature* creature, Item* item, const Position& pos)
{
	MagicField* field = item->getMagicField();

	if(field){
		field->onStepInField(creature);
		return 1;
	}

	return LUA_ERROR_ITEM_NOT_FOUND;
}

long MoveEvent::StepOutField(Creature* creature, Item* item, const Position& pos)
{
	return 1;
}

long MoveEvent::AddItemField(Item* item, Item* tileItem, const Position& pos)
{
	if(MagicField* field = item->getMagicField()){
		Tile* tile = item->getTile();
		for(CreatureVector::iterator cit = tile->creatures.begin(); cit != tile->creatures.end(); ++cit){
			field->onStepInField(*cit);
		}

		return 1;
	}

	return LUA_ERROR_ITEM_NOT_FOUND;
}

long MoveEvent::RemoveItemField(Item* item, Item* tileItem, const Position& pos)
{
	return 1;
}

long MoveEvent::fireStepEvent(Creature* creature, Item* item, const Position& pos)
{
	if(m_scripted){
		return executeStep(creature, item, pos);
	}
	else{
		return stepFunction(creature, item, pos);
	}
}

long MoveEvent::executeStep(Creature* creature, Item* item, const Position& pos)
{
	//onStepIn(cid, item, pos)
	//onStepOut(cid, item, pos)
	ScriptEnviroment* env = m_scriptInterface->getScriptEnv();
	
	#ifdef __DEBUG_LUASCRIPTS__
	std::stringstream desc;
	desc << creature->getName() << " itemid: " << item->getID() << " - " << pos;
	env->setEventDesc(desc.str());
	#endif
	
	env->setScriptId(m_scriptId, m_scriptInterface);
	env->setRealPos(pos);
	
	long cid = env->addThing((Thing*)creature);
	long itemid = env->addThing(item);
	
	lua_State* L = m_scriptInterface->getLuaState();
	
	m_scriptInterface->pushFunction(m_scriptId);
	lua_pushnumber(L, cid);
	LuaScriptInterface::pushThing(L, item, itemid);
	LuaScriptInterface::pushPosition(L, pos, 0);
	
	long ret;
	if(m_scriptInterface->callFunction(3, ret) == false){
		ret = 0;
	}
	
	return ret;
}

long MoveEvent::executeEquip(Player* player, Item* item, long slot)
{
	//onEquip(cid, item, slot)
	//onDeEquip(cid, item, slot)
	ScriptEnviroment* env = m_scriptInterface->getScriptEnv();
	
	#ifdef __DEBUG_LUASCRIPTS__
	std::stringstream desc;
	desc << player->getName() << " itemid:" << item->getID() << " slot:" << slot;
	env->setEventDesc(desc.str());
	#endif
	
	env->setScriptId(m_scriptId, m_scriptInterface);
	env->setRealPos(player->getPosition());
	
	long cid = env->addThing((Thing*)player);
	long itemid = env->addThing(item);
	
	lua_State* L = m_scriptInterface->getLuaState();
	
	m_scriptInterface->pushFunction(m_scriptId);
	lua_pushnumber(L, cid);
	LuaScriptInterface::pushThing(L, item, itemid);
	lua_pushnumber(L, slot);
	
	long ret;
	if(m_scriptInterface->callFunction(3, ret) == false){
		ret = 0;
	}
	
	return ret;
}

long MoveEvent::fireAddRemItem(Item* item, Item* tileItem, const Position& pos)
{
	if(m_scripted){
		return executeAddRemItem(item, tileItem, pos);
	}
	else{
		return moveFunction(item, tileItem, pos);
	}
}

long MoveEvent::executeAddRemItem(Item* item, Item* tileItem, const Position& pos)
{
	//onAddItem(moveitem, tileitem, pos)
	//onRemoveItem(moveitem, tileitem, pos)
	ScriptEnviroment* env = m_scriptInterface->getScriptEnv();
	
	#ifdef __DEBUG_LUASCRIPTS__
	std::stringstream desc;
	if(tileItem){
		desc << "tileid: " << tileItem->getID();
	}
	desc << " itemid: " << item->getID() << " - " << pos;
	env->setEventDesc(desc.str());
	#endif
	
	env->setScriptId(m_scriptId, m_scriptInterface);
	env->setRealPos(pos);
	
	long itemidMoved = env->addThing(item);
	long itemidTile = env->addThing(tileItem);
	
	lua_State* L = m_scriptInterface->getLuaState();
	
	m_scriptInterface->pushFunction(m_scriptId);
	LuaScriptInterface::pushThing(L, item, itemidMoved);
	LuaScriptInterface::pushThing(L, tileItem, itemidTile);
	LuaScriptInterface::pushPosition(L, pos, 0);
	
	long ret;
	if(m_scriptInterface->callFunction(3, ret) == false){
		ret = 0;
	}
	
	return ret;
}

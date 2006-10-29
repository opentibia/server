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

long MoveEvents::onPlayerEquip(Player* player, Item* item, slots_t slot, bool isEquip)
{
	MoveEvent_t eventType;
	if(isEquip){
		eventType = MOVE_EVENT_EQUIP;
	}
	else{
		eventType = MOVE_EVENT_DEEQUIP;
	}
	
	MoveEvent* event = getEvent(item, eventType);
	if(event && slot == event->getSlot()){
		return event->fireEquip(player, item, slot);
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
	equipFunction = NULL;
	slot = SLOT_WHEREEVER;
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
	
		if(m_eventType == MOVE_EVENT_EQUIP || m_eventType == MOVE_EVENT_DEEQUIP){
			if(readXMLString(p, "slot", str)){
				if(strcasecmp(str.c_str(), "head") == 0){
					slot = SLOT_HEAD;
				}
				else if(strcasecmp(str.c_str(), "necklace") == 0){
					slot = SLOT_NECKLACE;
				}
				else if(strcasecmp(str.c_str(), "backpack") == 0){
					slot = SLOT_BACKPACK;
				}
				else if(strcasecmp(str.c_str(), "armor") == 0){
					slot = SLOT_ARMOR;
				}
				else if(strcasecmp(str.c_str(), "right-hand") == 0){
					slot = SLOT_RIGHT;
				}
				else if(strcasecmp(str.c_str(), "left-hand") == 0){
					slot = SLOT_LEFT;
				}
				else if(strcasecmp(str.c_str(), "legs") == 0){
					slot = SLOT_LEGS;
				}
				else if(strcasecmp(str.c_str(), "feet") == 0){
					slot = SLOT_FEET;
				}
				else if(strcasecmp(str.c_str(), "ring") == 0){
					slot = SLOT_RING;
				}
			}
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
	else if(functionName == "onEquipItem"){
		equipFunction = EquipItem;
	}
	else if(functionName == "onDeEquipItem"){
		equipFunction = DeEquipItem;
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

long MoveEvent::EquipItem(Player* player, Item* item, slots_t slot)
{
	player->setItemAbility(slot, true);

	const ItemType& it = Item::items[item->getID()];
	
	if(it.transformEquipTo != 0){
		g_game.transformItem(item, it.transformEquipTo);
		g_game.startDecay(item);
	}

	if(it.abilities.invisible){
		Condition* condition = Condition::createCondition((ConditionId_t)slot, CONDITION_INVISIBLE, -1, 0);
		player->addCondition(condition);
	}

	if(it.abilities.manaShield){
		Condition* condition = Condition::createCondition((ConditionId_t)slot, CONDITION_MANASHIELD, -1, 0);
		player->addCondition(condition);
	}

	if(it.abilities.speed != 0){
		g_game.changeSpeed(player, it.abilities.speed);
	}

	if(it.abilities.conditionSuppressions != 0){
		player->setConditionSuppressions(it.abilities.conditionSuppressions, false);
		player->sendIcons();
	}

	if(it.abilities.regeneration){
		Condition* condition = Condition::createCondition((ConditionId_t)slot, CONDITION_REGENERATION, -1, 0);
		if(it.abilities.healthGain != 0){
			condition->setParam(CONDITIONPARAM_HEALTHGAIN, it.abilities.healthGain);
		}
		
		if(it.abilities.healthTicks != 0){
			condition->setParam(CONDITIONPARAM_HEALTHTICKS, it.abilities.healthTicks);
		}

		if(it.abilities.manaGain != 0){
			condition->setParam(CONDITIONPARAM_MANAGAIN, it.abilities.manaGain);
		}

		if(it.abilities.manaTicks != 0){
			condition->setParam(CONDITIONPARAM_MANATICKS, it.abilities.manaTicks);
		}

		player->addCondition(condition);
	}

	//skill modifiers
	for(int32_t i = SKILL_FIRST; i <= SKILL_LAST; ++i){
		player->setVarSkill((skills_t)i, it.abilities.skills[i]);
	}
	player->sendSkills();

	return 1;
}

long MoveEvent::DeEquipItem(Player* player, Item* item, slots_t slot)
{
	player->setItemAbility(slot, false);

	const ItemType& it = Item::items[item->getID()];

	if(it.transformDeEquipTo != 0){
		g_game.transformItem(item, it.transformDeEquipTo);
		g_game.startDecay(item);
	}

	if(it.abilities.invisible){
		player->removeCondition(CONDITION_INVISIBLE, (ConditionId_t)slot);
	}

	if(it.abilities.manaShield){
		player->removeCondition(CONDITION_MANASHIELD, (ConditionId_t)slot);
	}

	if(it.abilities.speed != 0){
		g_game.changeSpeed(player, -it.abilities.speed);
	}

	if(it.abilities.conditionSuppressions != 0){
		player->setConditionSuppressions(it.abilities.conditionSuppressions, true);
		player->sendIcons();
	}

	if(it.abilities.regeneration){
		player->removeCondition(CONDITION_REGENERATION, (ConditionId_t)slot);
	}

	//skill modifiers
	for(int32_t i = SKILL_FIRST; i <= SKILL_LAST; ++i){
		player->setVarSkill((skills_t)i, -it.abilities.skills[i]);
	}
	player->sendSkills();

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
	
	int32_t ret;
	if(m_scriptInterface->callFunction(3, ret) == false){
		ret = 0;
	}
	
	return ret;
}

long MoveEvent::fireEquip(Player* player, Item* item, slots_t slot)
{
	if(m_scripted){
		return executeEquip(player, item, slot);
	}
	else{
		return equipFunction(player, item, slot);
	}
}

long MoveEvent::executeEquip(Player* player, Item* item, slots_t slot)
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
	
	int32_t ret;
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
	
	int32_t ret;
	if(m_scriptInterface->callFunction(3, ret) == false){
		ret = 0;
	}
	
	return ret;
}

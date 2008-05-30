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
			delete it->second.moveEvent[i];
		}
		m_itemIdMap.erase(it);
		it = m_itemIdMap.begin();
	}

	it = m_actionIdMap.begin();
	while(it != m_actionIdMap.end()){
		for(int i = 0; i < MOVE_EVENT_LAST; ++i){
			delete it->second.moveEvent[i];
		}
		m_actionIdMap.erase(it);
		it = m_actionIdMap.begin();
	}

	it = m_uniqueIdMap.begin();
	while(it != m_uniqueIdMap.end()){
		for(int i = 0; i < MOVE_EVENT_LAST; ++i){
			delete it->second.moveEvent[i];
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
	else if(readXMLString(p,"pos",str)){
		std::vector<std::string> posList = explodeString(str, ";");
		if(posList.size() < 3){
			success = false;
		}
		else{
			Position pos;
			pos.x = atoi(posList[0].c_str());
			pos.y = atoi(posList[1].c_str());
			pos.z = atoi(posList[2].c_str());
			addEvent(moveEvent, pos, m_positionMap);
		}
	}
	else{
		success = false;
	}
	return success;
}

void MoveEvents::addEvent(MoveEvent* moveEvent, int32_t id, MoveListMap& map)
{
	MoveListMap::iterator it = map.find(id);
	if(it == map.end()){
		MoveEventList moveEventList;
		moveEventList.moveEvent[moveEvent->getEventType()] = moveEvent;
		map[id] = moveEventList;
	}
	else{
		it->second.moveEvent[moveEvent->getEventType()] = moveEvent;
	}
}

MoveEvent* MoveEvents::getEvent(Item* item, MoveEvent_t eventType)
{
	MoveListMap::iterator it;
	if(item->getUniqueId() != 0){
		it = m_uniqueIdMap.find(item->getUniqueId());
		if(it != m_uniqueIdMap.end()){
			return it->second.moveEvent[eventType];
		}
	}

	if(item->getActionId() != 0){
		it = m_actionIdMap.find(item->getActionId());
		if(it != m_actionIdMap.end()){
			return it->second.moveEvent[eventType];
		}
	}

	it = m_itemIdMap.find(item->getID());
	if(it != m_itemIdMap.end()){
	   	return it->second.moveEvent[eventType];
	}

	return NULL;
}

void MoveEvents::addEvent(MoveEvent* moveEvent, Position pos, MovePosListMap& map)
{
	MovePosListMap::iterator it = map.find(pos);
	if(it == map.end()){
		MoveEventList moveEventList;
		moveEventList.moveEvent[moveEvent->getEventType()] = moveEvent;
		map[pos] = moveEventList;
	}
	else{
		it->second.moveEvent[moveEvent->getEventType()] = moveEvent;
	}
}

MoveEvent* MoveEvents::getEvent(Tile* tile, MoveEvent_t eventType)
{
	MovePosListMap::iterator it = m_positionMap.find(tile->getPosition());
	if(it != m_positionMap.end()){
		return it->second.moveEvent[eventType];
	}
	
	return NULL;
}

uint32_t MoveEvents::onCreatureMove(Creature* creature, Tile* tile, bool isIn)
{
	MoveEvent_t eventType;
	if(isIn){
		eventType = MOVE_EVENT_STEP_IN;
	}
	else{
		eventType = MOVE_EVENT_STEP_OUT;
	}

	uint32_t ret = 1;
	MoveEvent* moveEvent = getEvent(tile, eventType);
	if(moveEvent){
		ret = ret & moveEvent->fireStepEvent(creature, NULL, tile->getPosition());
	}

	int32_t j = tile->__getLastIndex();
	Item* tileItem = NULL;
	for(int32_t i = tile->__getFirstIndex(); i < j; ++i){
		Thing* thing = tile->__getThing(i);
		if(thing && (tileItem = thing->getItem())){
			moveEvent = getEvent(tileItem, eventType);
			if(moveEvent){
				ret = ret & moveEvent->fireStepEvent(creature, tileItem, tile->getPosition());
			}
		}
	}
	return ret;
}

uint32_t MoveEvents::onPlayerEquip(Player* player, Item* item, slots_t slot)
{
	MoveEvent* moveEvent = getEvent(item, MOVE_EVENT_EQUIP);
	if(moveEvent && slot == moveEvent->getSlot()){
		return moveEvent->fireEquip(player, item, slot, false);
	}
	return 1;
}

uint32_t MoveEvents::onPlayerDeEquip(Player* player, Item* item, slots_t slot, bool isRemoval)
{
	MoveEvent* moveEvent = getEvent(item, MOVE_EVENT_DEEQUIP);
	if(moveEvent && slot == moveEvent->getSlot()){
		return moveEvent->fireEquip(player, item, slot, isRemoval);
	}
	return 1;
}

uint32_t MoveEvents::onItemMove(Item* item, Tile* tile, bool isAdd)
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

	uint32_t ret = 1;

	MoveEvent* moveEvent = getEvent(tile, eventType1);
	if(moveEvent){
		ret = ret & moveEvent->fireAddRemItem(item, NULL, tile->getPosition());
	}

	moveEvent = getEvent(item, eventType1);
	if(moveEvent){
		ret = ret & moveEvent->fireAddRemItem(item, NULL, tile->getPosition());
	}

	int32_t j = tile->__getLastIndex();
	Item* tileItem = NULL;
	for(int32_t i = tile->__getFirstIndex(); i < j; ++i){
		Thing* thing = tile->__getThing(i);
		if(thing && (tileItem = thing->getItem()) && (tileItem != item)){
			moveEvent = getEvent(tileItem, eventType2);
			if(moveEvent){
				ret = ret & moveEvent->fireAddRemItem(item, tileItem, tile->getPosition());
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
				else if(strcasecmp(str.c_str(), "ammo") == 0){
					slot = SLOT_AMMO;
				}
				else{
					std::cout << "Warning: [MoveEvent::configureMoveEvent] " << "Unknown slot type " << str << std::endl;
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

uint32_t MoveEvent::StepInField(Creature* creature, Item* item, const Position& pos)
{
	MagicField* field = item->getMagicField();

	if(field){
		field->onStepInField(creature);
		return 1;
	}

	return LUA_ERROR_ITEM_NOT_FOUND;
}

uint32_t MoveEvent::StepOutField(Creature* creature, Item* item, const Position& pos)
{
	return 1;
}

uint32_t MoveEvent::AddItemField(Item* item, Item* tileItem, const Position& pos)
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

uint32_t MoveEvent::RemoveItemField(Item* item, Item* tileItem, const Position& pos)
{
	return 1;
}

uint32_t MoveEvent::EquipItem(Player* player, Item* item, slots_t slot, bool transform)
{
	if(player->isItemAbilityEnabled(slot)){
		return 1;
	}

	const ItemType& it = Item::items[item->getID()];

	if(it.transformEquipTo != 0){
		Item* newItem = g_game.transformItem(item, it.transformEquipTo);
		g_game.startDecay(newItem);
	}
	else{
		player->setItemAbility(slot, true);
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
	bool needUpdateSkills = false;
	for(int32_t i = SKILL_FIRST; i <= SKILL_LAST; ++i){
		if(it.abilities.skills[i]){
			needUpdateSkills = true;
			player->setVarSkill((skills_t)i, it.abilities.skills[i]);
		}
	}
	if(needUpdateSkills){
		player->sendSkills();
	}

	//stat modifiers
	bool needUpdateStats = false;
	for(int32_t s = STAT_FIRST; s <= STAT_LAST; ++s){
		if(it.abilities.stats[s]){
			needUpdateStats = true;
			player->setVarStats((stats_t)s, it.abilities.stats[s]);
		}
		if(it.abilities.statsPercent[s]){
			needUpdateStats = true;
			player->setVarStats((stats_t)s, (int32_t)(player->getDefaultStats((stats_t)s) * ((it.abilities.statsPercent[s] - 100) / 100.f)));
		}
	}
	if(needUpdateStats){
		player->sendStats();
	}

	return 1;
}

uint32_t MoveEvent::DeEquipItem(Player* player, Item* item, slots_t slot, bool isRemoval)
{
	if(!player->isItemAbilityEnabled(slot)){
		return 1;
	}

	player->setItemAbility(slot, false);

	const ItemType& it = Item::items[item->getID()];

	if(isRemoval && it.transformDeEquipTo != 0){
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
	bool needUpdateSkills = false;
	for(int32_t i = SKILL_FIRST; i <= SKILL_LAST; ++i){
		if(it.abilities.skills[i] != 0){
			needUpdateSkills = true;
			player->setVarSkill((skills_t)i, -it.abilities.skills[i]);
		}
	}
	if(needUpdateSkills){
		player->sendSkills();
	}

	//stat modifiers
	bool needUpdateStats = false;
	for(int32_t s = STAT_FIRST; s <= STAT_LAST; ++s){
		if(it.abilities.stats[s]){
			needUpdateStats = true;
			player->setVarStats((stats_t)s, -it.abilities.stats[s]);
		}
		if(it.abilities.statsPercent[s]){
			needUpdateStats = true;
			player->setVarStats((stats_t)s, -(int32_t)(player->getDefaultStats((stats_t)s) * ((it.abilities.statsPercent[s] - 100) / 100.f)));
		}
	}
	if(needUpdateStats){
		player->sendStats();
	}

	return 1;
}

uint32_t MoveEvent::fireStepEvent(Creature* creature, Item* item, const Position& pos)
{
	if(m_scripted){
		return executeStep(creature, item, pos);
	}
	else{
		return stepFunction(creature, item, pos);
	}
}

uint32_t MoveEvent::executeStep(Creature* creature, Item* item, const Position& pos)
{
	//onStepIn(cid, item, pos)
	//onStepOut(cid, item, pos)
	if(m_scriptInterface->reserveScriptEnv()){
		ScriptEnviroment* env = m_scriptInterface->getScriptEnv();

		#ifdef __DEBUG_LUASCRIPTS__
		std::stringstream desc;
		desc << creature->getName() << " itemid: " << item->getID() << " - " << pos;
		env->setEventDesc(desc.str());
		#endif

		env->setScriptId(m_scriptId, m_scriptInterface);
		env->setRealPos(pos);

		uint32_t cid = env->addThing(creature);
		uint32_t itemid = env->addThing(item);

		lua_State* L = m_scriptInterface->getLuaState();

		m_scriptInterface->pushFunction(m_scriptId);
		lua_pushnumber(L, cid);
		LuaScriptInterface::pushThing(L, item, itemid);
		LuaScriptInterface::pushPosition(L, pos, 0);

		int32_t result = m_scriptInterface->callFunction(3);
		m_scriptInterface->releaseScriptEnv();

		return (result != LUA_FALSE);
	}
	else{
		std::cout << "[Error] Call stack overflow. MoveEvent::executeStep" << std::endl;
		return 0;
	}
}

uint32_t MoveEvent::fireEquip(Player* player, Item* item, slots_t slot, bool isRemoval)
{
	if(m_scripted){
		return executeEquip(player, item, slot);
	}
	else{
		return equipFunction(player, item, slot, isRemoval);
	}
}

uint32_t MoveEvent::executeEquip(Player* player, Item* item, slots_t slot)
{
	//onEquip(cid, item, slot)
	//onDeEquip(cid, item, slot)
	if(m_scriptInterface->reserveScriptEnv()){
		ScriptEnviroment* env = m_scriptInterface->getScriptEnv();

		#ifdef __DEBUG_LUASCRIPTS__
		std::stringstream desc;
		desc << player->getName() << " itemid:" << item->getID() << " slot:" << slot;
		env->setEventDesc(desc.str());
		#endif

		env->setScriptId(m_scriptId, m_scriptInterface);
		env->setRealPos(player->getPosition());

		uint32_t cid = env->addThing(player);
		uint32_t itemid = env->addThing(item);

		lua_State* L = m_scriptInterface->getLuaState();

		m_scriptInterface->pushFunction(m_scriptId);
		lua_pushnumber(L, cid);
		LuaScriptInterface::pushThing(L, item, itemid);
		lua_pushnumber(L, slot);

		int32_t result = m_scriptInterface->callFunction(3);
		m_scriptInterface->releaseScriptEnv();

		return (result != LUA_FALSE);
	}
	else{
		std::cout << "[Error] Call stack overflow. MoveEvent::executeEquip" << std::endl;
		return 0;
	}
}

uint32_t MoveEvent::fireAddRemItem(Item* item, Item* tileItem, const Position& pos)
{
	if(m_scripted){
		return executeAddRemItem(item, tileItem, pos);
	}
	else{
		return moveFunction(item, tileItem, pos);
	}
}

uint32_t MoveEvent::executeAddRemItem(Item* item, Item* tileItem, const Position& pos)
{
	//onAddItem(moveitem, tileitem, pos)
	//onRemoveItem(moveitem, tileitem, pos)
	if(m_scriptInterface->reserveScriptEnv()){
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

		uint32_t itemidMoved = env->addThing(item);
		uint32_t itemidTile = env->addThing(tileItem);

		lua_State* L = m_scriptInterface->getLuaState();

		m_scriptInterface->pushFunction(m_scriptId);
		LuaScriptInterface::pushThing(L, item, itemidMoved);
		LuaScriptInterface::pushThing(L, tileItem, itemidTile);
		LuaScriptInterface::pushPosition(L, pos, 0);

		int32_t result = m_scriptInterface->callFunction(3);
		m_scriptInterface->releaseScriptEnv();

		return (result != LUA_FALSE);
	}
	else{
		std::cout << "[Error] Call stack overflow. MoveEvent::executeAddRemItem" << std::endl;
		return 0;
	}
}

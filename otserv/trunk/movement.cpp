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

#include "movement.h"
#include "game.h"
#include "player.h"
#include "tile.h"
#include <sstream>
#include "tools.h"
#include "combat.h"
#include "vocation.h"
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

extern Game g_game;
extern Vocations g_vocations;
extern MoveEvents* g_moveEvents;

MoveEvents::MoveEvents() :
	m_scriptInterface("MoveEvents Interface"),
	m_lastCacheTile(NULL)
{
	m_scriptInterface.initState();
}

MoveEvents::~MoveEvents()
{
	clear();
}

void MoveEvents::clear()
{
	m_lastCacheTile = NULL;
	m_lastCacheItemVector.clear();

	MoveListMap::iterator it = m_itemIdMap.begin();
	while(it != m_itemIdMap.end()){
		for(int i = 0; i < MOVE_EVENT_LAST; ++i){
			std::list<MoveEvent*>& moveEventList = it->second.moveEvent[i];
			for(std::list<MoveEvent*>::iterator it = moveEventList.begin(); it != moveEventList.end(); ++it){
				delete (*it);
			}
		}
		m_itemIdMap.erase(it);
		it = m_itemIdMap.begin();
	}

	it = m_actionIdMap.begin();
	while(it != m_actionIdMap.end()){
		for(int i = 0; i < MOVE_EVENT_LAST; ++i){
			std::list<MoveEvent*>& moveEventList = it->second.moveEvent[i];
			for(std::list<MoveEvent*>::iterator it = moveEventList.begin(); it != moveEventList.end(); ++it){
				delete (*it);
			}
		}

		m_actionIdMap.erase(it);
		it = m_actionIdMap.begin();
	}

	it = m_uniqueIdMap.begin();
	while(it != m_uniqueIdMap.end()){
		for(int i = 0; i < MOVE_EVENT_LAST; ++i){
			std::list<MoveEvent*>& moveEventList = it->second.moveEvent[i];
			for(std::list<MoveEvent*>::iterator it = moveEventList.begin(); it != moveEventList.end(); ++it){
				delete (*it);
			}
		}
		m_uniqueIdMap.erase(it);
		it = m_uniqueIdMap.begin();
	}

	MovePosListMap::iterator posIter = m_positionMap.begin();
	while(posIter != m_positionMap.end()){
		for(int i = 0; i < MOVE_EVENT_LAST; ++i){
			std::list<MoveEvent*>& moveEventList = posIter->second.moveEvent[i];
			for(std::list<MoveEvent*>::iterator it = moveEventList.begin(); it != moveEventList.end(); ++it){
				delete (*it);
			}
		}
		m_positionMap.erase(posIter);
		posIter = m_positionMap.begin();
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
	if(asLowerCaseString(nodeName) == "movevent" || asLowerCaseString(nodeName) == "moveevent"){
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
	int32_t id, toId;
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

	if(readXMLInteger(p,"itemid",id) || readXMLInteger(p,"fromitemid",id) || readXMLInteger(p,"fromid",id)){
		if(!readXMLInteger(p,"toitemid",toId) && !readXMLInteger(p,"toid",toId))
			toId = id;
		for(; toId >= id; --toId){
			if(moveEvent->getEventType() == MOVE_EVENT_EQUIP){
				ItemType& it = Item::items.getItemType(toId);
				it.wieldInfo = moveEvent->getWieldInfo();
				it.minReqLevel = moveEvent->getReqLevel();
				it.minReqMagicLevel = moveEvent->getReqMagLv();
				it.vocationString = moveEvent->getVocationString();
			}

			addEvent(moveEvent->clone(), toId, m_itemIdMap);
		}
	}
	else if(readXMLInteger(p,"uniqueid",id) || readXMLInteger(p,"fromuniqueid",id) || readXMLInteger(p,"fromuid",id)){
		if(!readXMLInteger(p,"touniqueid",toId) && !readXMLInteger(p,"touid",toId))
			toId = id;
		for(; toId >= id; --toId){
			addEvent(moveEvent->clone(), toId, m_uniqueIdMap);
		}
	}
	else if(readXMLInteger(p,"actionid",id) || readXMLInteger(p,"fromactionid",id) || readXMLInteger(p,"fromaid",id)){
		if(!readXMLInteger(p,"toactionid",toId) && !readXMLInteger(p,"toaid",toId))
			toId = id;
		for(; toId >= id; --toId){
			addEvent(moveEvent->clone(), toId, m_actionIdMap);
		}
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
			addEvent(moveEvent->clone(), pos, m_positionMap);
		}
	}
	else{
		success = false;
	}

	delete event;
	return success;
}

void MoveEvents::addEvent(MoveEvent* moveEvent, int32_t id, MoveListMap& map)
{
	MoveListMap::iterator it = map.find(id);
	if(it == map.end()){
		MoveEventList moveEventList;
		moveEventList.moveEvent[moveEvent->getEventType()].push_back(moveEvent);
		map[id] = moveEventList;
	}
	else{
		std::list<MoveEvent*>& moveEventList = it->second.moveEvent[moveEvent->getEventType()];
		for(std::list<MoveEvent*>::iterator it = moveEventList.begin(); it != moveEventList.end(); ++it){
			if((*it)->getSlot() == moveEvent->getSlot()){
				std::cout << "Warning: [MoveEvents::addEvent] Duplicate move event found: " << id << std::endl;
			}
		}
		moveEventList.push_back(moveEvent);
	}
}

MoveEvent* MoveEvents::getEvent(Item* item, MoveEvent_t eventType, slots_t slot)
{
	uint32_t slotp = 0;
	switch(slot){
		case SLOT_HEAD: slotp = SLOTP_HEAD; break;
		case SLOT_NECKLACE: slotp = SLOTP_NECKLACE; break;
		case SLOT_BACKPACK: slotp = SLOTP_BACKPACK; break;
		case SLOT_ARMOR: slotp = SLOTP_ARMOR; break;
		case SLOT_RIGHT: slotp = SLOTP_RIGHT; break;
		case SLOT_LEFT: slotp = SLOTP_LEFT; break;
		case SLOT_LEGS: slotp = SLOTP_LEGS; break;
		case SLOT_FEET: slotp = SLOTP_FEET; break;
		case SLOT_AMMO: slotp = SLOTP_AMMO; break;
		case SLOT_RING: slotp = SLOTP_RING; break;
		default: slotp = 0; break;
	}
	MoveListMap::iterator it = m_itemIdMap.find(item->getID());
	if(it != m_itemIdMap.end()){
		std::list<MoveEvent*>& moveEventList = it->second.moveEvent[eventType];
		for(std::list<MoveEvent*>::iterator it = moveEventList.begin(); it != moveEventList.end(); ++it){
			if(((*it)->getSlot() & slotp) != 0){
				return *it;
			}
		}
	}

	return NULL;
}

MoveEvent* MoveEvents::getEvent(Item* item, MoveEvent_t eventType)
{
	MoveListMap::iterator it;
	if(item->getUniqueId() != 0){
		it = m_uniqueIdMap.find(item->getUniqueId());
		if(it != m_uniqueIdMap.end()){
			std::list<MoveEvent*>& moveEventList = it->second.moveEvent[eventType];
			if(!moveEventList.empty()){
				return *moveEventList.begin();
			}
		}
	}

	if(item->getActionId() != 0){
		it = m_actionIdMap.find(item->getActionId());
		if(it != m_actionIdMap.end()){
			std::list<MoveEvent*>& moveEventList = it->second.moveEvent[eventType];
			if(!moveEventList.empty()){
				return *moveEventList.begin();
			}
		}
	}

	it = m_itemIdMap.find(item->getID());
	if(it != m_itemIdMap.end()){
		std::list<MoveEvent*>& moveEventList = it->second.moveEvent[eventType];
		if(!moveEventList.empty()){
			return *moveEventList.begin();
		}
	}

	return NULL;
}

void MoveEvents::addEvent(MoveEvent* moveEvent, Position pos, MovePosListMap& map)
{
	MovePosListMap::iterator it = map.find(pos);
	if(it == map.end()){
		MoveEventList moveEventList;
		moveEventList.moveEvent[moveEvent->getEventType()].push_back(moveEvent);
		map[pos] = moveEventList;
	}
	else{
		std::list<MoveEvent*>& moveEventList = it->second.moveEvent[moveEvent->getEventType()];
		if(!moveEventList.empty()){
			std::cout << "Warning: [MoveEvents::addEvent] Duplicate move event found: " << pos << std::endl;
		}

		moveEventList.push_back(moveEvent);
	}
}

MoveEvent* MoveEvents::getEvent(const Tile* tile, MoveEvent_t eventType)
{
	MovePosListMap::iterator it = m_positionMap.find(tile->getPosition());
	if(it != m_positionMap.end()){
		std::list<MoveEvent*>& moveEventList = it->second.moveEvent[eventType];
		if(!moveEventList.empty()){
			return *moveEventList.begin();
		}
	}

	return NULL;
}

uint32_t MoveEvents::onCreatureMove(Creature* creature, const Tile* fromTile, const Tile* toTile, bool isIn)
{
	MoveEvent_t eventType;
	const Tile* tile = NULL;
	if(isIn){
		tile = toTile;
		eventType = MOVE_EVENT_STEP_IN;
	}
	else{
		tile = fromTile;
		eventType = MOVE_EVENT_STEP_OUT;
	}

	Position fromPos(0, 0, 0);
	if(fromTile){
		fromPos = fromTile->getPosition();
	}

	Position toPos(0, 0, 0);
	if(toTile){
		toPos = toTile->getPosition();
	}

	uint32_t ret = 1;
	MoveEvent* moveEvent = getEvent(tile, eventType);
	if(moveEvent){
		ret = ret & moveEvent->fireStepEvent(creature, NULL, fromPos, toPos);
	}

	Item* tileItem = NULL;
	if(m_lastCacheTile == tile){
		if(m_lastCacheItemVector.empty()){
			return ret;
		}

		//We can not use iterators here since the scripts can invalidate the iterator
		for(uint32_t i = 0; i < m_lastCacheItemVector.size(); ++i){
			tileItem = m_lastCacheItemVector[i];
			if(tileItem){
				moveEvent = getEvent(tileItem, eventType);
				if(moveEvent){
					ret = ret & moveEvent->fireStepEvent(creature, tileItem, fromPos, toPos);
				}
			}
		}
		return ret;
	}

	m_lastCacheTile = tile;
	m_lastCacheItemVector.clear();

	//We can not use iterators here since the scripts can invalidate the iterator
	int32_t j = tile->__getLastIndex();
	for(int32_t i = tile->__getFirstIndex(); i < j; ++i){
		Thing* thing = tile->__getThing(i);
		if(thing && (tileItem = thing->getItem())){
			moveEvent = getEvent(tileItem, eventType);
			if(moveEvent){
				m_lastCacheItemVector.push_back(tileItem);
				ret = ret & moveEvent->fireStepEvent(creature, tileItem, fromPos, toPos);
			}
			else if(hasTileEvent(tileItem)){
				m_lastCacheItemVector.push_back(tileItem);
			}
		}
	}

	return ret;
}

uint32_t MoveEvents::onPlayerEquip(Player* player, Item* item, slots_t slot)
{
	MoveEvent* moveEvent = getEvent(item, MOVE_EVENT_EQUIP, slot);
	if(moveEvent){
		return moveEvent->fireEquip(player, item, slot, false);
	}
	return 1;
}

uint32_t MoveEvents::onPlayerDeEquip(Player* player, Item* item, slots_t slot, bool isRemoval)
{
	MoveEvent* moveEvent = getEvent(item, MOVE_EVENT_DEEQUIP, slot);
	if(moveEvent){
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
		ret &= moveEvent->fireAddRemItem(item, NULL, tile->getPosition());
	}

	moveEvent = getEvent(item, eventType1);
	if(moveEvent){
		ret &= moveEvent->fireAddRemItem(item, NULL, tile->getPosition());
	}

	Item* tileItem = NULL;
	if(m_lastCacheTile == tile){
		if(m_lastCacheItemVector.empty()){
			return false;
		}

		//We can not use iterators here since the scripts can invalidate the iterator
		for(uint32_t i = 0; i < m_lastCacheItemVector.size(); ++i){
			tileItem = m_lastCacheItemVector[i];
			if(tileItem && tileItem != item){
				moveEvent = getEvent(tileItem, eventType2);
				if(moveEvent){
					ret &= moveEvent->fireAddRemItem(item, tileItem, tile->getPosition());
				}
			}
		}

		return ret;
	}

	m_lastCacheTile = tile;
	m_lastCacheItemVector.clear();

	//We can not use iterators here since the scripts can invalidate the iterator
	int32_t j = tile->__getLastIndex();
	for(int32_t i = tile->__getFirstIndex(); i < j; ++i){
		Thing* thing = tile->__getThing(i);
		if(thing && (tileItem = thing->getItem()) && (tileItem != item)){
			moveEvent = getEvent(tileItem, eventType2);
			if(moveEvent){
				m_lastCacheItemVector.push_back(tileItem);
				ret &= moveEvent->fireAddRemItem(item, tileItem, tile->getPosition());
			}
			else if(hasTileEvent(tileItem)){
				m_lastCacheItemVector.push_back(tileItem);
			}
		}
	}

	return ret;
}

bool MoveEvents::hasTileEvent(Item* item)
{
	return( getEvent(item, MOVE_EVENT_ADD_ITEM_ITEMTILE) ||
			getEvent(item, MOVE_EVENT_REMOVE_ITEM_ITEMTILE) ||
			getEvent(item, MOVE_EVENT_STEP_IN) ||
			getEvent(item, MOVE_EVENT_STEP_OUT));
}

void MoveEvents::onAddTileItem(const Tile* tile, Item* item)
{
	if(m_lastCacheTile == tile){
		std::vector<Item*>::iterator it = std::find(m_lastCacheItemVector.begin(), m_lastCacheItemVector.end(), item);
		if(it == m_lastCacheItemVector.end()){
			if(hasTileEvent(item)){
				m_lastCacheItemVector.push_back(item);
			}
		}
	}
}

void MoveEvents::onRemoveTileItem(const Tile* tile, Item* item)
{
	if(m_lastCacheTile == tile){
		for(uint32_t i = 0; i < m_lastCacheItemVector.size(); ++i){
			if(m_lastCacheItemVector[i] == item){
				m_lastCacheItemVector[i] = NULL;
				break;
			}
		}
	}
}

ReturnValue MoveEvents::canPlayerWearEquip(Player* player, Item* item, slots_t slot)
{
	MoveEvent* moveEvent = getEvent(item, MOVE_EVENT_EQUIP, slot);
	if(moveEvent){
		return moveEvent->canPlayerWearEquip(player, slot);
	}
	return RET_NOERROR;
}

MoveEvent::MoveEvent(LuaScriptInterface* _interface) :
Event(_interface)
{
	m_eventType = MOVE_EVENT_NONE;
	stepFunction = NULL;
	moveFunction = NULL;
	equipFunction = NULL;
	slot = 0xFFFFFFFF;
	reqLevel = 0;
	reqMagLevel = 0;
	premium = false;
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
	case MOVE_EVENT_ADD_ITEM_ITEMTILE:
		return "onAddItem";
		break;
	case MOVE_EVENT_REMOVE_ITEM:
	case MOVE_EVENT_REMOVE_ITEM_ITEMTILE:
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
	int intValue;
	if(readXMLString(p, "event", str)){
		if(asLowerCaseString(str) == "stepin"){
			m_eventType = MOVE_EVENT_STEP_IN;
		}
		else if(asLowerCaseString(str) == "stepout"){
			m_eventType = MOVE_EVENT_STEP_OUT;
		}
		else if(asLowerCaseString(str) == "equip"){
			m_eventType = MOVE_EVENT_EQUIP;
		}
		else if(asLowerCaseString(str) == "deequip"){
			m_eventType = MOVE_EVENT_DEEQUIP;
		}
		else if(asLowerCaseString(str) == "additem"){
			m_eventType = MOVE_EVENT_ADD_ITEM;
		}
		else if(asLowerCaseString(str) == "removeitem"){
			m_eventType = MOVE_EVENT_REMOVE_ITEM;
		}
		else{
			std::cout << "Error: [MoveEvent::configureMoveEvent] No valid event name " << str << std::endl;
			return false;
		}

		if(m_eventType == MOVE_EVENT_EQUIP || m_eventType == MOVE_EVENT_DEEQUIP){
			if(readXMLString(p, "slot", str)){
				if(asLowerCaseString(str) == "head"){
					slot = SLOTP_HEAD;
				}
				else if(asLowerCaseString(str) == "necklace"){
					slot = SLOTP_NECKLACE;
				}
				else if(asLowerCaseString(str) == "backpack"){
					slot = SLOTP_BACKPACK;
				}
				else if(asLowerCaseString(str) == "armor"){
					slot = SLOTP_ARMOR;
				}
				else if(asLowerCaseString(str) == "right-hand"){
					slot = SLOTP_RIGHT;
				}
				else if(asLowerCaseString(str) == "left-hand"){
					slot = SLOTP_LEFT;
				}
				else if(asLowerCaseString(str) == "hand"){
					slot = SLOTP_RIGHT | SLOTP_LEFT;
				}
				else if(asLowerCaseString(str) == "legs"){
					slot = SLOTP_LEGS;
				}
				else if(asLowerCaseString(str) == "feet"){
					slot = SLOTP_FEET;
				}
				else if(asLowerCaseString(str) == "ring"){
					slot = SLOTP_RING;
				}
				else if(asLowerCaseString(str) == "ammo"){
					slot = SLOTP_AMMO;
				}
				else{
					std::cout << "Warning: [MoveEvent::configureMoveEvent] " << "Unknown slot type " << str << std::endl;
				}
			}

			wieldInfo = 0;
			if(readXMLInteger(p, "lvl", intValue) || readXMLInteger(p, "level", intValue)){
	 			reqLevel = intValue;
				if(reqLevel > 0){
					wieldInfo |= WIELDINFO_LEVEL;
				}
			}
			if(readXMLInteger(p, "maglv", intValue) || readXMLInteger(p, "maglevel", intValue)){
	 			reqMagLevel = intValue;
				if(reqMagLevel > 0){
					wieldInfo |= WIELDINFO_MAGLV;
				}
			}
			if(readXMLInteger(p, "prem", intValue) || readXMLInteger(p, "premium", intValue)){
				premium = (intValue != 0);
				if(premium){
					wieldInfo |= WIELDINFO_PREMIUM;
				}
			}

			//Gather vocation information
			typedef std::list<std::string> STRING_LIST;
			STRING_LIST vocStringList;
			xmlNodePtr vocationNode = p->children;
			while(vocationNode){
				if(xmlStrcmp(vocationNode->name,(const xmlChar*)"vocation") == 0){
					if(readXMLString(vocationNode, "name", str)){
						int32_t vocationId = 0;
						if(g_vocations.getVocationId(str, vocationId)){
							vocEquipMap[vocationId] = true;
							intValue = 1;
							readXMLInteger(vocationNode, "showInDescription", intValue);
							if(intValue != 0){
								toLowerCaseString(str);
								vocStringList.push_back(str);
							}
						}
					}
				}

				vocationNode = vocationNode->next;
			}

			if(!vocStringList.empty()){
				for(STRING_LIST::iterator it = vocStringList.begin(); it != vocStringList.end(); ++it){
					if(*it != vocStringList.front()){
						if(*it != vocStringList.back()){
							vocationString += ", ";
						}
						else{
							vocationString += " and ";
						}
					}
					vocationString += *it;
					vocationString += "s";
				}
				wieldInfo |= WIELDINFO_VOCREQ;
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
	if(asLowerCaseString(functionName) == "onstepinfield"){
		stepFunction = StepInField;
	}
	else if(asLowerCaseString(functionName) == "onstepoutfield"){
		stepFunction = StepOutField;
	}
	else if(asLowerCaseString(functionName) == "onaddfield"){
		moveFunction = AddItemField;
	}
	else if(asLowerCaseString(functionName) == "onremovefield"){
		moveFunction = RemoveItemField;
	}
	else if(asLowerCaseString(functionName) == "onequipitem"){
		equipFunction = EquipItem;
	}
	else if(asLowerCaseString(functionName) == "ondeequipitem"){
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

uint32_t MoveEvent::StepInField(Creature* creature, Item* item, const Position& fromPos, const Position& toPos)
{
	MagicField* field = item->getMagicField();

	if(field){
		bool purposeful = false;
		if(creature->getPlayer())
			purposeful = true;

		field->onStepInField(creature, purposeful);

		return 1;
	}

	return LUA_ERROR_ITEM_NOT_FOUND;
}

uint32_t MoveEvent::StepOutField(Creature* creature, Item* item, const Position& fromPos, const Position& toPos)
{
	return 1;
}

uint32_t MoveEvent::AddItemField(Item* item, Item* tileItem, const Position& pos)
{
	if(MagicField* field = item->getMagicField()){
		Tile* tile = item->getTile();
		if(CreatureVector* creatures = tile->getCreatures()){
			for(CreatureVector::iterator cit = creatures->begin(); cit != creatures->end(); ++cit){
				//totally invisible GMs shouldn't call for onStep events
				if (!(*cit)->getPlayer() || !(*cit)->getPlayer()->hasSomeInvisibilityFlag()){
					field->onStepInField(*cit);
				}
			}
		}
		return 1;
	}

	return LUA_ERROR_ITEM_NOT_FOUND;
}

uint32_t MoveEvent::RemoveItemField(Item* item, Item* tileItem, const Position& pos)
{
	return 1;
}

uint32_t MoveEvent::EquipItem(MoveEvent* moveEvent, Player* player, Item* item, slots_t slot, bool isRemoval)
{
	if(player->isItemAbilityEnabled(slot)){
		return 1;
	}
	//Enable item only when requirements are complete
	//This includes item transforming
	if(!player->hasFlag(PlayerFlag_IgnoreWeaponCheck) && moveEvent->getWieldInfo() != 0){
		if(player->getLevel() < moveEvent->getReqLevel()
			|| player->getMagicLevel() < moveEvent->getReqMagLv() ||
			(!player->isPremium() && moveEvent->isPremium())){
			return 1;
		}
		const VocEquipMap vocMap = moveEvent->getVocEquipMap();
		if(!vocMap.empty()){
			if(vocMap.find(player->getVocationId()) == vocMap.end()){
				return 1;
			}
		}
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
		if(it.abilities.skill.upgrades[i]){
			needUpdateSkills = true;
			player->setVarSkill((skills_t)i, it.abilities.skill.upgrades[i]);
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

uint32_t MoveEvent::DeEquipItem(MoveEvent* moveEvent, Player* player, Item* item, slots_t slot, bool isRemoval)
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
		if(it.abilities.skill.upgrades[i] != 0){
			needUpdateSkills = true;
			player->setVarSkill((skills_t)i, -it.abilities.skill.upgrades[i]);
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

uint32_t MoveEvent::fireStepEvent(Creature* creature, Item* item, const Position& fromPos, const Position& toPos)
{

	if(m_scripted){
		return executeStep(creature, item, fromPos, toPos);
	}
	else{
		return stepFunction(creature, item, fromPos, toPos);
	}
}

uint32_t MoveEvent::executeStep(Creature* creature, Item* item, const Position& fromPos, const Position& toPos)
{
	//onStepIn(cid, item, topos, frompos)
	//onStepOut(cid, item, topos, frompos)
	if(m_scriptInterface->reserveScriptEnv()){
		ScriptEnviroment* env = m_scriptInterface->getScriptEnv();

		#ifdef __DEBUG_LUASCRIPTS__
		std::stringstream desc;
		desc << creature->getName() << " itemid: " << item->getID() << " - " << toPos;
		env->setEventDesc(desc.str());
		#endif

		env->setScriptId(m_scriptId, m_scriptInterface);
		env->setRealPos(creature->getPosition());

		uint32_t cid = env->addThing(creature);
		uint32_t itemid = env->addThing(item);

		lua_State* L = m_scriptInterface->getLuaState();

		m_scriptInterface->pushFunction(m_scriptId);
		lua_pushnumber(L, cid);
		LuaScriptInterface::pushThing(L, item, itemid);
		LuaScriptInterface::pushPosition(L, toPos, 0);
		LuaScriptInterface::pushPosition(L, fromPos, 0);

		bool result = m_scriptInterface->callFunction(4);
		m_scriptInterface->releaseScriptEnv();

		return result;
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
		return equipFunction(this, player, item, slot, isRemoval);
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

		bool result = m_scriptInterface->callFunction(3);
		m_scriptInterface->releaseScriptEnv();

		return result;
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

		bool result = m_scriptInterface->callFunction(3);
		m_scriptInterface->releaseScriptEnv();

		return result;
	}
	else{
		std::cout << "[Error] Call stack overflow. MoveEvent::executeAddRemItem" << std::endl;
		return 0;
	}
}

ReturnValue MoveEvent::canPlayerWearEquip(Player* player, slots_t slot)
{
	//check if we need to continue
	if(player->isItemAbilityEnabled(slot) || player->hasFlag(PlayerFlag_IgnoreWeaponCheck) || getWieldInfo() == 0){
		return RET_NOERROR;
	}

	//check all required values
	const VocEquipMap vocMap = getVocEquipMap();
	if(!vocMap.empty() && vocMap.find(player->getVocationId()) == vocMap.end()){
		return RET_NOTREQUIREDPROFESSION;
	}
	if(player->getLevel() < getReqLevel()){
		return RET_NOTREQUIREDLEVEL;
	}
	if(player->getMagicLevel() < getReqMagLv()){
		return RET_NOTENOUGHMAGICLEVEL;
	}
	if(!player->isPremium() && isPremium()){
		return RET_NEEDPREMIUMTOEQUIPITEM;
	}

	return RET_NOERROR;
}

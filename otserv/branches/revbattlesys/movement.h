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

#ifndef __MOVEMENT_H__
#define __MOVEMENT_H__


#include "luascript.h"
#include "baseevents.h"
#include <map>

enum MoveEvent_t{
	MOVE_EVENT_STEP_IN = 0,
	MOVE_EVENT_STEP_OUT,
	MOVE_EVENT_EQUIP,
	MOVE_EVENT_DEEQUIP,
	MOVE_EVENT_ADD_ITEM,
	MOVE_EVENT_REMOVE_ITEM,
	MOVE_EVENT_ADD_ITEM_ITEMTILE,
	MOVE_EVENT_REMOVE_ITEM_ITEMTILE,
	MOVE_EVENT_LAST,
	MOVE_EVENT_NONE,
};

class MoveEvent;

struct MoveEventList{
	MoveEvent* event[MOVE_EVENT_LAST];
MoveEventList(){
	for(int i=0; i < MOVE_EVENT_LAST; ++i){
		event[i] = NULL;	
	}
};	
};

class MoveEvents : public BaseEvents
{
public:
	MoveEvents();
	virtual ~MoveEvents();
	
	long onCreatureMove(Creature* creature, Tile* tile, bool isIn);
	long onPlayerEquip(Player* player, Item* item, slots_t slot, bool isEquip);
	long onItemMove(Item* item, Tile* tile, bool isAdd);
	
protected:
	typedef std::map<long , MoveEventList> MoveListMap;
	virtual void clear();
	virtual LuaScriptInterface& getScriptInterface();
	virtual std::string getScriptBaseName();
	virtual Event* getEvent(const std::string& nodeName);
	virtual bool registerEvent(Event* event, xmlNodePtr p);
	
	void addEvent(MoveEvent* event, long id, MoveListMap& map);
	
	MoveEvent* getEvent(Item* item, MoveEvent_t eventType);
	
	MoveListMap m_uniqueIdMap;
	MoveListMap m_actionIdMap;
	MoveListMap m_itemIdMap;
	
	LuaScriptInterface m_scriptInterface;
};

typedef long (StepFunction)(Creature* creature, Item* item, const Position& pos);
typedef long (MoveFunction)(Item* item, Item* tileItem, const Position& pos);
typedef long (EquipFunction)(Player* player, Item* item, slots_t slot);

class MoveEvent : public Event
{
public:
	MoveEvent(LuaScriptInterface* _interface);
	virtual ~MoveEvent();

	MoveEvent_t getEventType() const;
	void setEventType(MoveEvent_t type);
	
	virtual bool configureEvent(xmlNodePtr p);
	virtual bool loadFunction(const std::string& functionName);

	long fireStepEvent(Creature* creature, Item* item, const Position& pos);
	long fireAddRemItem(Item* item, Item* tileItem, const Position& pos);
	long fireEquip(Player* player, Item* item, slots_t slot);

	slots_t getSlot() const {return slot;}

	//scripting
	long executeStep(Creature* creature, Item* item, const Position& pos);
	long executeEquip(Player* player, Item* item, slots_t slot);
	long executeAddRemItem(Item* item, Item* tileItem, const Position& pos);
	//
	
protected:
	virtual std::string getScriptEventName();
	
	static StepFunction StepInField;
	static StepFunction StepOutField;

	static MoveFunction AddItemField;
	static MoveFunction RemoveItemField;
	static EquipFunction EquipItem;
	static EquipFunction DeEquipItem;

	MoveEvent_t m_eventType;
	StepFunction* stepFunction;
	MoveFunction* moveFunction;
	EquipFunction* equipFunction;
	slots_t slot;
	
};


#endif

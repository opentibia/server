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


#ifndef __OTSERV_ACTIONS_H__
#define __OTSERV_ACTIONS_H__

#include "position.h"

#include <map>
#include "luascript.h"
#include "baseevents.h"
#include "thing.h"

class Action;
class Container;
class ItemType;

enum ActionType_t{
	ACTION_ANY,
	ACTION_UNIQUEID,
	ACTION_ACTIONID,
	ACTION_ITEMID,
	ACTION_RUNEID,
};

class Actions : public BaseEvents
{
public:
	Actions();
	virtual ~Actions();

	bool useItem(Player* player, const Position& pos, uint8_t index, Item* item, bool isHotkey);
	bool useItemEx(Player* player, const Position& fromPos, const Position& toPos,
		uint8_t toStackPos, Item* item, bool isHotkey, uint32_t creatureId = 0);

	ReturnValue canUse(const Player* player, const Position& pos);
	ReturnValue canUse(const Player* player, const Position& pos, const Item* item);
	ReturnValue canUseFar(const Creature* creature, const Position& toPos, bool checkLineOfSight);
	bool hasAction(const Item* item) const;

protected:
	bool executeUse(Action* action, Player* player, Item* item, const PositionEx& posEx, uint32_t creatureId);
	ReturnValue internalUseItem(Player* player, const Position& pos,
		uint8_t index, Item* item, uint32_t creatureId);

	bool executeUseEx(Action* action, Player* player, Item* item, const PositionEx& fromPosEx,
		const PositionEx& toPosEx, bool isHotkey, uint32_t creatureId);
	ReturnValue internalUseItemEx(Player* player, const PositionEx& fromPosEx, const PositionEx& toPosEx,
		Item* item, bool isHotkey, uint32_t creatureId, bool& isSuccess);

	virtual void clear();
	virtual LuaScriptInterface& getScriptInterface();
	virtual std::string getScriptBaseName();
	virtual Event* getEvent(const std::string& nodeName);
	virtual bool registerEvent(Event* event, xmlNodePtr p);

	typedef std::map<unsigned short, Action*> ActionUseMap;
	ActionUseMap useItemMap;
	ActionUseMap uniqueItemMap;
	ActionUseMap actionItemMap;

	Action* getAction(const Item* item, ActionType_t type = ACTION_ANY) const;
	void clearMap(ActionUseMap& map);

	LuaScriptInterface m_scriptInterface;
};

class Action : public Event
{
public:
	Action(LuaScriptInterface* _interface);
	virtual ~Action();

	virtual bool configureEvent(xmlNodePtr p);

	//scripting
	virtual bool executeUse(Player* player, Item* item, const PositionEx& posFrom,
		const PositionEx& posTo, bool extendedUse, uint32_t creatureId);
	//

	bool getAllowFarUse() const {return allowFarUse;};
	void setAllowFarUse(bool v){allowFarUse = v;};

	bool getCheckLineOfSight() const {return checkLineOfSight;};
	void setCheckLineOfSight(bool v){checkLineOfSight = v;};

	virtual ReturnValue canExecuteAction(const Player* player, const Position& toPos);
	virtual bool hasOwnErrorHandler() {return false;}

protected:
	virtual std::string getScriptEventName();

	bool allowFarUse;
	bool checkLineOfSight;
};

#endif

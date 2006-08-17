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


#ifndef __ACTIONS_H__
#define __ACTIONS_H__

#include "position.h"

#include <map>
#include "luascript.h"
#include "baseevents.h"

class Action;
class Container;

enum tCanUseRet{
	CAN_USE,
	TOO_FAR,
	CAN_NOT_THROW,
};

class Actions : public BaseEvents
{
public:
	Actions();
	virtual ~Actions();


	bool useItem(Player* player, const Position& pos, uint8_t index, Item* item);
	bool useItemEx(Player* player, const Position& from_pos,
		const Position& to_pos, const unsigned char to_stack, Item* item);

	bool openContainer(Player* player,Container* container, const unsigned char index);

	static int canUse(const Creature* creature ,const Position& pos);
	static int canUseFar(const Creature* creature ,const Position& to_pos, const bool blockWalls);

protected:
	virtual void clear();
	virtual LuaScriptInterface& getScriptInterface();
	virtual std::string getScriptBaseName();
	virtual Event* getEvent(const std::string& nodeName);
	virtual bool registerEvent(Event* event, xmlNodePtr p);

	typedef std::map<unsigned short, Action*> ActionUseMap;
	ActionUseMap useItemMap;
	ActionUseMap uniqueItemMap;
	ActionUseMap actionItemMap;

	Action *getAction(const Item* item);
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
	virtual bool executeUse(Player* player, Item* item, const PositionEx& posFrom, const PositionEx& posTo);
	//

	bool allowFarUse() const {return allowfaruse;};
	bool blockWalls() const {return blockwalls;};

	void setAllowFarUse(bool v){allowfaruse = v;};
	void setBlockWalls(bool v){blockwalls = v;};

	virtual bool canExecuteAction(const Player* player, const Position& toPos);

protected:
	virtual std::string getScriptEventName();

	bool allowfaruse;
	bool blockwalls;
};

#endif

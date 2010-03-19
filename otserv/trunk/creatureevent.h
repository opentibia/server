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

#ifndef __OTSERV_CREATUREEVENT_H__
#define __OTSERV_CREATUREEVENT_H__

#include "enums.h"
#include "luascript.h"
#include "baseevents.h"

enum CreatureEventType_t{
	CREATURE_EVENT_NONE,
	CREATURE_EVENT_LOGIN,
	CREATURE_EVENT_LOGOUT,
	CREATURE_EVENT_DIE,
	CREATURE_EVENT_KILL,
	CREATURE_EVENT_ADVANCE,
	CREATURE_EVENT_LOOK
};

class CreatureEvent;

class CreatureEvents : public BaseEvents
{
public:
	CreatureEvents();
	virtual ~CreatureEvents();

	// global events
	uint32_t playerLogIn(Player* player);
	uint32_t playerLogOut(Player* player);
	bool executeLookAtEvent(Player* player, Thing* target, uint16_t itemidPar);

	CreatureEvent* getEventByName(const std::string& name, bool forceLoaded = true);

protected:

	virtual LuaScriptInterface& getScriptInterface();
	virtual std::string getScriptBaseName();
	virtual Event* getEvent(const std::string& nodeName);
	virtual bool registerEvent(Event* event, xmlNodePtr p);
	virtual void clear();

	//events
	typedef std::map<std::string, CreatureEvent*> CreatureEventList;
	CreatureEventList m_creatureEvents;

	LuaScriptInterface m_scriptInterface;
};

class CreatureEvent : public Event
{
public:
	CreatureEvent(LuaScriptInterface* _interface);
	virtual ~CreatureEvent();

	virtual bool configureEvent(xmlNodePtr p);

	CreatureEventType_t getEventType() const { return m_type; }
	const std::string& getName() const { return m_eventName; }
	bool isLoaded() const { return m_isLoaded; }

	void clearEvent();
	void copyEvent(CreatureEvent* creatureEvent);

	//scripting
	uint32_t executeOnLogin(Player* player);
	uint32_t executeOnLogout(Player* player);
	uint32_t executeOnDie(Creature* creature, Item* corpse);
	uint32_t executeOnKill(Creature* creature, Creature* target, bool lastHit);
	uint32_t executeOnAdvance(Player* player, levelTypes_t type, uint32_t oldLevel, uint32_t newLevel);
	uint32_t executeOnLook(Creature* creature, Thing* target, uint16_t itemIdPar);
	//

protected:
	virtual std::string getScriptEventName();

	std::string m_eventName;
	CreatureEventType_t m_type;
	bool m_isLoaded;
};


#endif // __OTSERV_CREATUREEVENT_H__

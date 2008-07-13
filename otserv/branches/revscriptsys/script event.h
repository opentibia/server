//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
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

class ScriptManager;
class ScriptEnviroment;

#include <string>
#include "boost/any.hpp"
#include "shared_ptr.h"

class Creature;

///////////////////////////////////////////////////////////////////////////////
// Event Listener

class EventListener {
public:
	EventListener(const std::string& name);
	~EventListener();

	std::string getLuaTag() const;

	boost::any data;
protected:
	static uint32_t ID_counter;
	uint32_t ID;
	std::string name;
};

typedef boost::shared_ptr<EventListener> EventListener_ptr;
typedef boost::weak_ptr<EventListener> EventListener_wptr;

///////////////////////////////////////////////////////////////////////////////
// Event template

class ScriptEvent {
public:
	ScriptEvent();
	virtual ~ScriptEvent();

	virtual void dispatch(ScriptManager& script_system, ScriptEnviroment& enviroment) = 0;
protected:
	uint32_t eventID;
	static uint32_t eventID_counter;
};

///////////////////////////////////////////////////////////////////////////////
// OnSay event
// Triggered when a creature talks

class ScriptEvent_OnSay : public ScriptEvent {
public:
	ScriptEvent_OnSay(Creature* speaker, SpeakClass type, std::string receiver, std::string text);
	~ScriptEvent_OnSay();
	
	void dispatch(ScriptManager& script_system, ScriptEnviroment& enviroment);
protected:
	void call(ScriptManager& script_system, ScriptEnviroment& enviroment, EventListener_ptr listener);

	Creature* speaker;
	SpeakClass type;
	std::string text;
	std::string receiver;
};

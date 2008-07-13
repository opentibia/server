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

#include "otpch.h"

#include "script event.h"

uint32_t ScriptEvent::eventID_counter = 0;

ScriptEvent::ScriptEvent() : eventID(++eventID_counter) {
}

ScriptEvent::~ScriptEvent() {
}

///////////////////////////////////////////////////////////////////////////////

ScriptEvent_OnSay::ScriptEvent_OnSay(Creature* _speaker, SpeakClass _type, std::string _receiver, std::string _text) {
	speaker = _speaker;
	type = _type;
	text = _text;
	receiver = _receiver;
}

ScripEvent_OnSay::~ScriptEvent_OnSay() {
}

ScriptEvent_OnSay::dispatch(ScriptManager& script_system, ScriptEnviroment& enviroment) {
	SpecificCreatureEventMap& specific_map = enviroment.Specific.OnSay;
	SpecificCreatureEventMap::iterator creatures_found = specific_map.equal_range(speaker);

	if(creatures_found != specific_map.end()) {
		GenericCreatureEventList& specific_list = *creatures_found;
		for(GenericCreatureEventList::iterator event_iter = specific_list.first;
				event_iter != specific_list.second;
				++event_iter)
		{
			// Call handler
			call(script_system, enviroment, *event_iter);
		}
	}
}

ScriptEvent_OnSay::call(ScriptManager& script_system, ScriptEnviroment& enviroment, EventListener_ptr listener) {	
	EventListener_ptr listener = *event_iter;
	LuaThread_ptr thread = script_system->newEventThread(this, "OnSay");
	thread->pushEventCallback(listener);
	thread->pushThing(speaker);
	thread->pushInteger(type);
	thread->pushString(receiver);
	thread->pushString(text);
	thread->run();
}

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

#ifndef __OTSERV_SCRIPT_EVENT__
#define __OTSERV_SCRIPT_EVENT__

#include <string>
#include "boost/any.hpp"
#include "boost_common.h"

#include "script_enviroment.h"

#include "const.h"

// Forward declarations
class Creature;

class LuaState;
class LuaThread;
typedef boost::shared_ptr<LuaThread> LuaThread_ptr;

namespace Script {

	class Manager;
	class Enviroment;
	class Listener;
	typedef boost::shared_ptr<Listener> Listener_ptr;
	typedef boost::weak_ptr<Listener> Listener_wptr;
	typedef std::vector<Listener_ptr> ListenerList;
}


template<class T, class ScriptInformation>
bool dispatchEvent(T* e, Script::Manager& state, Script::Enviroment& enviroment, Script::ListenerList& specific_list);


namespace Script {
	///////////////////////////////////////////////////////////////////////////////
	// Event template

	class Event {
	public:
		Event();
		virtual ~Event();

		virtual std::string getName() const = 0;

		// Runs the event (ie. triggers all concerned listeners)
		virtual bool dispatch(Manager& state, Script::Enviroment& enviroment) = 0;
		
		// Lua stack manipulation, push
		virtual void push_instance(LuaState& state, Script::Enviroment& enviroment) = 0;
		// update, peek at top table and fill this event with values from it)
		virtual void update_instance(Manager& state, Script::Enviroment& enviroment, LuaThread_ptr thread) = 0;
	protected:
		bool call(Manager& stae, Enviroment& enviroment, Listener_ptr listener);

		uint32_t eventID;
		static uint32_t eventID_counter;
		std::string lua_tag;
		
		template<class T, class ScriptInformation> friend
			bool ::dispatchEvent(T* e, 
				Script::Manager& state, 
				Script::Enviroment& enviroment, 
				Script::ListenerList& specific_list);

	};

	///////////////////////////////////////////////////////////////////////////////
	// OnSay event
	// Triggered when a creature talks

	namespace OnSay {
		enum FilterType {
			FILTER_ALL,
			FILTER_SUBSTRING,
			FILTER_MATCH_BEGINNING,
			FILTER_EXACT,
		};

		struct ScriptInformation {
			std::string filter;
			FilterType method;
			bool case_sensitive;
		};

		class Event : public Script::Event {
		public:
			Event(Creature* speaker, SpeakClass& speak_class, std::string& receiver, std::string& text);
			~Event();

			std::string getName() const {return "OnSay";}
			
			// Runs the event
			bool dispatch(Manager& state, Enviroment& enviroment);
			
			// This checks if the script information matches this events prerequiste (data members)
			bool check_match(const ScriptInformation& info);

			// Lua stack manipulation
			void push_instance(LuaState& state, Enviroment& enviroment);
			void update_instance(Manager& state, Script::Enviroment& enviroment, LuaThread_ptr thread);

		protected:
			Creature* speaker;
			SpeakClass& speak_class;
			std::string& text;
			std::string& receiver;
		};
	}
	
	/* To add a new event
	 * 1. Create the event class
	 * 2. Expose a registerListener function to lua
	 * 3. Add the class to Enviroment::stopListener
	 * 4. Add callback from an arbitrary location in otserv source
	 */
}


///////////////////////////////////////////////////////////////////////////////
// Implementation details

template<class T, class ScriptInformation>
bool dispatchEvent(T* e, Script::Manager& state, Script::Enviroment& enviroment, Script::ListenerList& specific_list) {
	if(specific_list.size() == 0) {
		return false;
	}
	for(Script::ListenerList::iterator event_iter = specific_list.begin();
		event_iter != specific_list.end();
		++event_iter)
	{
		Listener_ptr listener = *event_iter;
		if(listener->isActive() == false) continue;
		const ScriptInformation& info = boost::any_cast<const ScriptInformation>(listener->getData());

		// Call handler
		if(e->check_match(info)) {
			if(e->call(state, enviroment, listener) == true) {
				// Handled
				return true;
			}
		}
	}
	return false;
}

#endif // __OTSERV_SCRIPT_EVENT__

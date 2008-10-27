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

class Creature;

class LuaState;

namespace Script {

	class Manager;

	class Listener;
	typedef boost::shared_ptr<Listener> Listener_ptr;
	typedef boost::weak_ptr<Listener> Listener_wptr;

	///////////////////////////////////////////////////////////////////////////////
	// Event template

	class Event {
	public:
		Event();
		virtual ~Event();

		virtual bool dispatch(Manager& state, Script::Enviroment& enviroment) = 0;
		virtual void push_instance(LuaState& state, Script::Enviroment& enviroment) = 0;
	protected:

		uint32_t eventID;
		static uint32_t eventID_counter;
		std::string lua_tag;
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
			Event(Creature* speaker, SpeakClass& type, std::string& receiver, std::string& text);
			~Event();
			
			bool dispatch(Manager& state, Enviroment& enviroment);
			void push_instance(LuaState& state, Enviroment& enviroment);
		protected:
			bool dispatch(Manager& state, Enviroment& enviroment, ListenerList& specific_list);
			bool check_match(const ScriptInformation& info);
			bool call(Manager& state, Enviroment& enviroment, Listener_ptr listener);

			Creature* speaker;
			SpeakClass& type;
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

#endif // __OTSERV_SCRIPT_EVENT__

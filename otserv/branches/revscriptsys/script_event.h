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
#include "thing.h"

// Forward declarations
class Creature;
class PositionEx;

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
template<class T>
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
		bool propagate_by_default;
		
		template<class T, class ScriptInformation> friend
			bool ::dispatchEvent(T* e, 
				Script::Manager& state, 
				Script::Enviroment& enviroment, 
				Script::ListenerList& specific_list);
		
		template<class T> friend
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
			Event(Creature* speaker, SpeakClass& speak_class, ChatChannel* channel, std::string& text);
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
			ChatChannel* channel;
			std::string& text;
		};
	}


	////////////////////////////////////////////////////////////////
	// Use event
	// Triggered when a player uses an item

	namespace OnUseItem {
		enum FilterType {
			FILTER_ITEMID,
			FILTER_UNIQUEID,
			FILTER_ACTIONID,
		};

		struct ScriptInformation {
			FilterType method;
			uint16_t id;
		};

		class Event : public Script::Event {
		public:
			Event(Player* user, Item* item, const PositionEx* toPos, ReturnValue& retval);
			~Event();

			std::string getName() const {return "OnUseItem";}
			
			// Runs the event
			bool dispatch(Manager& state, Enviroment& enviroment);
			
			// This checks if the script information matches this events prerequiste (data members)
			bool check_match(const ScriptInformation& info);

			// Lua stack manipulation
			void push_instance(LuaState& state, Enviroment& enviroment);
			void update_instance(Manager& state, Script::Enviroment& enviroment, LuaThread_ptr thread);

		protected:
			Player* user;
			Item* item;
			const PositionEx* targetPos;
			ReturnValue& retval;
		};
	}

	///////////////////////////////////////////////////////////////////////////////
	// OnJoinChannel event
	// Triggered when a player opens a new chat channel

	namespace OnJoinChannel {
		class Event : public Script::Event {
		public:
			Event(Player* chatter, ChatChannel* chat);
			~Event();

			std::string getName() const {return "OnJoinChannel";}
			
			// Runs the event
			bool dispatch(Manager& state, Enviroment& enviroment);

			// Lua stack manipulation
			void push_instance(LuaState& state, Enviroment& enviroment);
			void update_instance(Manager& state, Script::Enviroment& enviroment, LuaThread_ptr thread);

		protected:
			Player* chatter;
			ChatChannel* channel;
		};
	}

	///////////////////////////////////////////////////////////////////////////////
	// OnLeaveChannel event
	// Triggered when a player closes an existing chat channel

	namespace OnLeaveChannel {
		class Event : public Script::Event {
		public:
			Event(Player* chatter, ChatChannel* chat);
			~Event();

			std::string getName() const {return "OnLeaveChannel";}
			
			// Runs the event
			bool dispatch(Manager& state, Enviroment& enviroment);

			// Lua stack manipulation
			void push_instance(LuaState& state, Enviroment& enviroment);
			void update_instance(Manager& state, Script::Enviroment& enviroment, LuaThread_ptr thread);

		protected:
			Player* chatter;
			ChatChannel* channel;
		};
	}

	///////////////////////////////////////////////////////////////////////////////
	// OnLogin event
	// Triggered when a player enters the server

	namespace OnLogin {
		class Event : public Script::Event {
		public:
			Event(Player* player);
			~Event();

			std::string getName() const {return "OnLogin";}
			
			// Runs the event
			bool dispatch(Manager& state, Enviroment& enviroment);

			// Lua stack manipulation
			void push_instance(LuaState& state, Enviroment& enviroment);
			void update_instance(Manager& state, Script::Enviroment& enviroment, LuaThread_ptr thread);

		protected:
			Player* player;
		};
	}

	///////////////////////////////////////////////////////////////////////////////
	// OnLogout event
	// Triggered when a player leaves the server

	namespace OnLogout {
		class Event : public Script::Event {
		public:
			Event(Player* player, bool forced, bool timeout);
			~Event();

			std::string getName() const {return "OnLogout";}
			
			// Runs the event
			bool dispatch(Manager& state, Enviroment& enviroment);

			// Lua stack manipulation
			void push_instance(LuaState& state, Enviroment& enviroment);
			void update_instance(Manager& state, Script::Enviroment& enviroment, LuaThread_ptr thread);

		protected:
			Player* player;
			bool forced;
			bool timeout;
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

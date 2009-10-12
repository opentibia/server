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

#include "classes.h"
#include "script_environment.h"
#include "script_listener.h"
#include "condition_attributes.h"
#include "chat.h"
#include "const.h"
#include "boost/any.hpp"
#include "boost_common.h"

// Forward declarations
class LuaState;
class LuaThread;
typedef boost::shared_ptr<LuaThread> LuaThread_ptr;

// These are actually defined in the .cpp file, so you CAN ONLY USE THEM IN script_event.cpp
template<class T, class ScriptInformation>
	bool dispatchEvent(T* e, Script::Manager& state, Script::Environment& environment, Script::ListenerList& specific_list);
template<class T>
	bool dispatchEvent(T* e, Script::Manager& state, Script::Environment& environment, Script::ListenerList& specific_list);

namespace Script {

	/****************** GUIDE READ THIS TO ADD AN EVENT! *****************&*/
	/* To add a new event
	 * 1. Create the event class, with all it's members
	 *    easiest is to copy an existing event that's similar.
	 * 2. Add the listener type to enums.h
	 * 3. Expose a registerListener function to lua (or many)
	 * 4. Add the class to Environment::stopListener
	 * 5. Add callback from an arbitrary location in otserv source
	 */

	///////////////////////////////////////////////////////////////////////////////
	// Event template

	class Event {
	public:
		Event();
		virtual ~Event();

		virtual std::string getName() const = 0;

		// Runs the event (ie. triggers all concerned listeners)
		virtual bool dispatch(Manager& state, Script::Environment& environment) = 0;

		// Lua stack manipulation, push
		virtual void push_instance(LuaState& state, Script::Environment& environment) = 0;
		// update, peek at top table and fill this event with values from it)
		virtual void update_instance(Manager& state, Script::Environment& environment, LuaThread_ptr thread) = 0;

		// Should not be called directly (calling functions
		// can't be made fiend due to compiler limitations)
		bool call(Manager& stae, Environment& environment, Listener_ptr listener);
	protected:

		uint32_t eventID;
		static uint32_t eventID_counter;
		bool propagate_by_default;

	};

	////////////////////////////////////////////////////////////////
	// OnServerLoad event
	// Triggered when the server has finished loading (after map),
	// or when scripts are reloaded

	namespace OnServerLoad {
		class Event : public Script::Event {
		public:
			Event(bool real_startup);
			~Event();

			std::string getName() const {return "OnServerLoad";}

			// Runs the event
			bool dispatch(Manager& state, Environment& environment);

			// Lua stack manipulation
			void push_instance(LuaState& state, Environment& environment);
			void update_instance(Manager& state, Script::Environment& environment, LuaThread_ptr thread);

		protected:
			bool real_startup;
		};
	}

	////////////////////////////////////////////////////////////////
	// OnServerUnload event
	// Triggered when server is shutdown, or right before a /reload

	namespace OnServerUnload {
		class Event : public Script::Event {
		public:
			Event(bool real_shutdown);
			~Event();

			std::string getName() const {return "OnServerUnload";}

			// Runs the event
			bool dispatch(Manager& state, Environment& environment);

			// Lua stack manipulation
			void push_instance(LuaState& state, Environment& environment);
			void update_instance(Manager& state, Script::Environment& environment, LuaThread_ptr thread);

		protected:
			bool real_shutdown;
		};
	}

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
			bool dispatch(Manager& state, Environment& environment);

			// This checks if the script information matches this events prerequiste (data members)
			bool check_match(const ScriptInformation& info);

			// Lua stack manipulation
			void push_instance(LuaState& state, Environment& environment);
			void update_instance(Manager& state, Script::Environment& environment, LuaThread_ptr thread);

		protected:
			Creature* speaker;
			SpeakClass& speak_class;
			ChatChannel* channel;
			std::string& text;
		};
	}


	///////////////////////////////////////////////////////////////////////////////
	// OnHear event
	// Triggered when a creature hears another creature speak

	namespace OnHear {
		struct ScriptInformation {
		};

		class Event : public Script::Event {
		public:
			Event(Creature* creature, Creature* talking_creature, const std::string& message, const SpeakClass& speak_class);
			~Event();

			std::string getName() const {return "OnHear";}

			// Runs the event
			bool dispatch(Manager& state, Environment& environment);

			// Lua stack manipulation
			void push_instance(LuaState& state, Environment& environment);
			void update_instance(Manager& state, Script::Environment& environment, LuaThread_ptr thread);

		protected:
			Creature* creature;
			Creature* talking_creature;
			const std::string& message;
			const SpeakClass& speak_class;
		};
	}

	////////////////////////////////////////////////////////////////
	// OnUseItem event
	// Triggered when a player uses an item

	namespace OnUseItem {
		enum FilterType {
			FILTER_ITEMID,
			FILTER_ACTIONID,
		};

		struct ScriptInformation {
			FilterType method;
			int32_t id;
		};

		class Event : public Script::Event {
		public:
			Event(Player* user, Item* item, const PositionEx* toPos, ReturnValue& retval);
			~Event();

			std::string getName() const {return "OnUseItem";}

			// Runs the event
			bool dispatch(Manager& state, Environment& environment);

			// This checks if the script information matches this events prerequiste (data members)
			bool check_match(const ScriptInformation& info);

			// Lua stack manipulation
			void push_instance(LuaState& state, Environment& environment);
			void update_instance(Manager& state, Script::Environment& environment, LuaThread_ptr thread);

		protected:
			Player* user;
			Item* item;
			const PositionEx* targetPos;
			ReturnValue& retval;
		};
	}

	////////////////////////////////////////////////////////////////
	// OnEquipItem event
	// Triggered when a player equip an item

	namespace OnEquipItem {
		enum FilterType {
			FILTER_ITEMID,
			FILTER_ACTIONID,
		};

		struct ScriptInformation {
			FilterType method;
			int32_t id;
			SlotPosition slot;
			bool equip;
		};

		class Event : public Script::Event {
		public:
			Event(Player* user, Item* item, SlotType slot, bool equip);
			~Event();

			std::string getName() const {return "OnEquipItem";}

			// Runs the event
			bool dispatch(Manager& state, Environment& environment);

			// This checks if the script information matches this events prerequiste (data members)
			bool check_match(const ScriptInformation& info);

			// Lua stack manipulation
			void push_instance(LuaState& state, Environment& environment);
			void update_instance(Manager& state, Script::Environment& environment, LuaThread_ptr thread);

		protected:
			Player* user;
			Item* item;
			SlotPosition equipslot;
			bool equip;
		};
	}

	////////////////////////////////////////////////////////////////
	// OnMoveCreature event
	// Triggered when a creature moves

	namespace OnMoveCreature {
		enum FilterType {
			FILTER_ITEMID,
			FILTER_ACTIONID,
			FILTER_NONE,
		};

		enum MoveType {
			TYPE_NONE,
			TYPE_MOVE,
			TYPE_STEPIN,
			TYPE_STEPOUT,
		};

		struct ScriptInformation {
			FilterType method;
			int32_t id;
			uint32_t slot;
			MoveType moveType;
		};

		class Event : public Script::Event {
		public:
			Event(Creature* actor, Creature* moving_creature, Tile* fromTile, Tile* toTile);
			~Event();

			std::string getName() const {return "OnMoveCreature";}

			// Runs the event
			bool dispatch(Manager& state, Environment& environment);

			// This checks if the script information matches this events prerequiste (data members)
			bool check_match(const ScriptInformation& info);

			// Lua stack manipulation
			void push_instance(LuaState& state, Environment& environment);
			void update_instance(Manager& state, Script::Environment& environment, LuaThread_ptr thread);

		protected:
			bool isMatch(const ScriptInformation& info, Tile* tile);

			Creature* actor;
			Creature* moving_creature;
			Tile* fromTile;
			Tile* toTile;
			MoveType moveType;
		};
	}

	////////////////////////////////////////////////////////////////
	// OnTurn event
	// Triggered when a creature turns

	namespace OnTurn {
		class Event : public Script::Event {
		public:
			Event(Creature* creature, Direction direction);
			~Event();

			std::string getName() const {return "OnTurn";}

			// Runs the event
			bool dispatch(Manager& state, Environment& environment);

			// Lua stack manipulation
			void push_instance(LuaState& state, Environment& environment);
			void update_instance(Manager& state, Script::Environment& environment, LuaThread_ptr thread);

		protected:
			Creature* creature;
			Direction direction;
		};
	}

	////////////////////////////////////////////////////////////////
	// OnMoveItem event
	// Triggered when an item is moved

	namespace OnMoveItem {
		enum FilterType {
			FILTER_ITEMID,
			FILTER_ACTIONID,
		};

		struct ScriptInformation {
			FilterType method;
			int32_t id;
			bool addItem;
			bool isItemOnTile;
		};

		class Event : public Script::Event {
		public:
			Event(Creature* actor, Item* item, Tile* tile, bool addItem);
			~Event();

			std::string getName() const {return "OnMoveItem";}

			// Runs the event
			bool dispatch(Manager& state, Environment& environment);

			// This checks if the script information matches this events prerequiste (data members)
			bool check_match(const ScriptInformation& info);

			// Lua stack manipulation
			void push_instance(LuaState& state, Environment& environment);
			void update_instance(Manager& state, Script::Environment& environment, LuaThread_ptr thread);

		protected:
			Creature* actor;
			Item* item;
			Tile* tile;
			bool addItem;
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
			bool dispatch(Manager& state, Environment& environment);

			// Lua stack manipulation
			void push_instance(LuaState& state, Environment& environment);
			void update_instance(Manager& state, Script::Environment& environment, LuaThread_ptr thread);

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
			bool dispatch(Manager& state, Environment& environment);

			// Lua stack manipulation
			void push_instance(LuaState& state, Environment& environment);
			void update_instance(Manager& state, Script::Environment& environment, LuaThread_ptr thread);

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
			bool dispatch(Manager& state, Environment& environment);

			// Lua stack manipulation
			void push_instance(LuaState& state, Environment& environment);
			void update_instance(Manager& state, Script::Environment& environment, LuaThread_ptr thread);

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
			bool dispatch(Manager& state, Environment& environment);

			// Lua stack manipulation
			void push_instance(LuaState& state, Environment& environment);
			void update_instance(Manager& state, Script::Environment& environment, LuaThread_ptr thread);

		protected:
			Player* player;
			bool forced;
			bool timeout;
		};
	}

	///////////////////////////////////////////////////////////////////////////////
	// OnChangeOut event
	// Triggered when a player request the outfit dialog

	namespace OnChangeOutfit {
		class Event : public Script::Event {
		public:
			Event(Player* player, std::list<Outfit>& outfitList);
			~Event();

			std::string getName() const {return "OnChangeOutfit";}

			// Runs the event
			bool dispatch(Manager& state, Environment& environment);

			// Lua stack manipulation
			void push_instance(LuaState& state, Environment& environment);
			void update_instance(Manager& state, Script::Environment& environment, LuaThread_ptr thread);

		protected:
			Player* player;
			std::list<Outfit>& outfitList;
		};
	}

	///////////////////////////////////////////////////////////////////////////////
	// OnLook event
	// Triggered when a looks at an object

	namespace OnLook {
		enum FilterType {
			FILTER_NONE,
			FILTER_ITEMID,
			FILTER_ACTIONID
		};

		struct ScriptInformation {
			FilterType method;
			int32_t id;
		};

		class Event : public Script::Event {
		public:
			Event(Player* player, std::string& desc, Thing* object);
			~Event();

			std::string getName() const {return "OnLook";}

			// Runs the event
			bool dispatch(Manager& state, Environment& environment);

			// This checks if the script information matches this events prerequiste (data members)
			bool check_match(const ScriptInformation& info);

			// Lua stack manipulation
			void push_instance(LuaState& state, Environment& environment);
			void update_instance(Manager& state, Script::Environment& environment, LuaThread_ptr thread);

		protected:
			Player* player;
			std::string& desc;
			Thing* object;
		};
	}

	///////////////////////////////////////////////////////////////////////////////
	// OnSpotCreature event
	// Triggered when a creature spots another creature (OnCreatureAppear)

	namespace OnSpotCreature {
		class Event : public Script::Event {
		public:
			Event(Creature* creature, Creature* spotted_creature);
			~Event();

			std::string getName() const {return "OnSpotCreature";}

			// Runs the event
			bool dispatch(Manager& state, Environment& environment);

			// Lua stack manipulation
			void push_instance(LuaState& state, Environment& environment);
			void update_instance(Manager& state, Script::Environment& environment, LuaThread_ptr thread);

		protected:
			Creature* creature;
			Creature* spotted_creature;
		};
	}

	///////////////////////////////////////////////////////////////////////////////
	// OnLoseCreature event
	// Triggered when a creature loses a creature (OnCreatureDisappear)

	namespace OnLoseCreature {
		class Event : public Script::Event {
		public:
			Event(Creature* creature, Creature* lose_creature);
			~Event();

			std::string getName() const {return "OnLoseCreature";}

			// Runs the event
			bool dispatch(Manager& state, Environment& environment);

			// Lua stack manipulation
			void push_instance(LuaState& state, Environment& environment);
			void update_instance(Manager& state, Script::Environment& environment, LuaThread_ptr thread);

		protected:
			Creature* creature;
			Creature* lose_creature;
		};
	}

	///////////////////////////////////////////////////////////////////////////////
	// OnSpawn event
	// Triggered when a creature is spawned

	namespace OnSpawn {
		class Event : public Script::Event {
		public:
			Event(Actor* actor, bool reloading = false);
			~Event();

			std::string getName() const {return "OnSpawn";}

			// Runs the event
			bool dispatch(Manager& state, Environment& environment);

			// Lua stack manipulation
			void push_instance(LuaState& state, Environment& environment);
			void update_instance(Manager& state, Script::Environment& environment, LuaThread_ptr thread);

		protected:
			Actor* actor;
			bool reloading;
		};
	}

	///////////////////////////////////////////////////////////////////////////////
	// OnThink event
	// Triggered when a creature thinks (ie. all the time)

	namespace OnThink {
		class Event : public Script::Event {
		public:
			Event(Creature* creature, int32_t interval);
			~Event();

			std::string getName() const {return "OnThink";}

			// Runs the event
			bool dispatch(Manager& state, Environment& environment);

			// Lua stack manipulation
			void push_instance(LuaState& state, Environment& environment);
			void update_instance(Manager& state, Script::Environment& environment, LuaThread_ptr thread);

		protected:
			Creature* creature;
			int32_t interval;
		};
	}

	///////////////////////////////////////////////////////////////////////////////
	// OnAdvance event
	// Triggered when a player advances in a skill

	namespace OnAdvance {
		enum FilterType {
			FILTER_ALL,
			FILTER_SKILL
		};

		struct ScriptInformation {
			FilterType method;
			LevelType skill;
		};

		class Event : public Script::Event {
		public:
			Event(Player* player, LevelType skill, uint32_t oldskilllevel, uint32_t newskilllevel);
			~Event();

			std::string getName() const {return "OnAdvance";}

			// Runs the event
			bool dispatch(Manager& state, Environment& environment);

			// This checks if the script information matches this events prerequiste (data members)
			bool check_match(const ScriptInformation& info);

			// Lua stack manipulation
			void push_instance(LuaState& state, Environment& environment);
			void update_instance(Manager& state, Script::Environment& environment, LuaThread_ptr thread);

		protected:
			Player* player;
			LevelType skill;
			uint32_t oldSkillLevel;
			uint32_t newSkillLevel;
		};
	}

	///////////////////////////////////////////////////////////////////////////////
	// OnCondition event
	// Triggered when a condition is added/removed or tick

	namespace OnCondition {
		enum FilterType {
			FILTER_BEGIN,
			FILTER_END,
			FILTER_TICK,
		};

		struct ScriptInformation {
			FilterType method;
			std::string name;
		};

		class Event : public Script::Event {
		public:
			Event(Creature* creature, Condition* condition);
			Event(Creature* creature, Condition* condition, ConditionEnd reason);
			Event(Creature* creature, Condition* condition, uint32_t ticks);
			~Event();

			std::string getName() const {return "OnCondition";}

			// Runs the event
			bool dispatch(Manager& state, Environment& environment);

			// This checks if the script information matches this events prerequiste (data members)
			bool check_match(const ScriptInformation& info);

			// Lua stack manipulation
			void push_instance(LuaState& state, Environment& environment);
			void update_instance(Manager& state, Script::Environment& environment, LuaThread_ptr thread);

		protected:
			Creature* creature;
			Condition* condition;
			ConditionEnd reason;
			uint32_t ticks;

			enum EventType{
				EVENT_BEGIN,
				EVENT_END,
				EVENT_TICK
			} eventType;

		};
	}

	///////////////////////////////////////////////////////////////////////////////
	// OnAttack event
	// Triggered when a creature attacks another creature (usually every 2 seconds)

	namespace OnAttack {
		enum FilterType {
			FILTER_ALL,
			FILTER_NAME,
			FILTER_PLAYER,
			FILTER_ATTACKED_NAME,
			FILTER_ATTACKED_PLAYER,
			FILTER_ATTACKED_ACTOR,
			FILTER_PLAYER_ATTACK_ACTOR,
			FILTER_PLAYER_ATTACK_PLAYER,
			FILTER_ACTOR_ATTACK_ACTOR,
			FILTER_ACTOR_ATTACK_PLAYER
		};

		struct ScriptInformation {
			FilterType method;
			std::string name;
		};

		class Event : public Script::Event {
		public:
			Event(Creature* creature, Creature* attacked);
			~Event();

			std::string getName() const {return "OnAttack";}

			// Runs the event
			bool dispatch(Manager& state, Environment& environment);

			// This checks if the script information matches this events prerequiste (data members)
			bool check_match(const ScriptInformation& info);

			// Lua stack manipulation
			void push_instance(LuaState& state, Environment& environment);
			void update_instance(Manager& state, Script::Environment& environment, LuaThread_ptr thread);

		protected:
			Creature* creature;
			Creature* attacked;
		};
	}

	///////////////////////////////////////////////////////////////////////////////
	// OnDamage event
	// Triggered when a creature is taking damage (or heals)

	namespace OnDamage {
		enum FilterType {
			FILTER_ALL,
			FILTER_NAME,
			FILTER_PLAYER,
			FILTER_ATTACKER_NAME,
			FILTER_ATTACKER_PLAYER,
			FILTER_ATTACKER_ACTOR,
			FILTER_PLAYER_DAMAGE_PLAYER,
			FILTER_PLAYER_DAMAGE_ACTOR,
			FILTER_ACTOR_DAMAGE_ACTOR,
			FILTER_ACTOR_DAMAGE_PLAYER,
			FILTER_TYPE
		};

		struct ScriptInformation {
			FilterType method;
			std::string name;
			CombatType combatType;
		};

		class Event : public Script::Event {
		public:
			Event(CombatType& combatType, CombatSource& combatSource, Creature* creature, int32_t& value);
			~Event();

			std::string getName() const {return "OnDamage";}

			// Runs the event
			bool dispatch(Manager& state, Environment& environment);

			// This checks if the script information matches this events prerequiste (data members)
			bool check_match(const ScriptInformation& info);

			// Lua stack manipulation
			void push_instance(LuaState& state, Environment& environment);
			void update_instance(Manager& state, Script::Environment& environment, LuaThread_ptr thread);

		protected:
			CombatType& combatType;
			CombatSource& combatSource;
			Creature* creature;
			int32_t& value;
		};
	}

	///////////////////////////////////////////////////////////////////////////////
	// OnKill event
	// Triggered when a creature reaches 0 health

	namespace OnKill {
		enum FilterType {
			FILTER_ALL,
			FILTER_NAME,
			FILTER_PLAYER,
			FILTER_KILLER_NAME,
			FILTER_KILLER_PLAYER,
			FILTER_KILLER_ACTOR,
			FILTER_PLAYER_KILL_PLAYER,
			FILTER_PLAYER_KILL_ACTOR,
			FILTER_ACTOR_KILL_ACTOR,
			FILTER_ACTOR_KILL_PLAYER
		};

		struct ScriptInformation {
			FilterType method;
			std::string name;
		};

		class Event : public Script::Event {
		public:
			Event(Creature* creature, CombatSource& combatSource);
			~Event();

			std::string getName() const {return "OnKill";}

			// Runs the event
			bool dispatch(Manager& state, Environment& environment);

			// This checks if the script information matches this events prerequiste (data members)
			bool check_match(const ScriptInformation& info);

			// Lua stack manipulation
			void push_instance(LuaState& state, Environment& environment);
			void update_instance(Manager& state, Script::Environment& environment, LuaThread_ptr thread);

		protected:
			Creature* creature;
			CombatSource& combatSource;
		};
	}

	///////////////////////////////////////////////////////////////////////////////
	// OnDeath event
	// Triggered when a creature dies (after death-delay)

	namespace OnDeath {
		enum FilterType {
			FILTER_ALL,
			FILTER_NAME,
			FILTER_PLAYER,
			FILTER_KILLER_NAME,
			FILTER_KILLER_PLAYER,
			FILTER_KILLER_ACTOR,
			FILTER_PLAYER_DEATH_BY_PLAYER,
			FILTER_PLAYER_DEATH_BY_ACTOR,
			FILTER_ACTOR_DEATH_BY_ACTOR,
			FILTER_ACTOR_DEATH_BY_PLAYER
		};

		struct ScriptInformation {
			FilterType method;
			std::string name;
		};

		class Event : public Script::Event {
		public:
			Event(Creature* creature, Item* corpse, Creature* killer);
			~Event();

			std::string getName() const {return "OnDeath";}

			// Runs the event
			bool dispatch(Manager& state, Environment& environment);

			// This checks if the script information matches this events prerequiste (data members)
			bool check_match(const ScriptInformation& info);

			// Lua stack manipulation
			void push_instance(LuaState& state, Environment& environment);
			void update_instance(Manager& state, Script::Environment& environment, LuaThread_ptr thread);

		protected:
			Creature* creature;
			Item* corpse;
			Creature* killer;
		};
	}
}

///////////////////////////////////////////////////////////////////////////////
// Implementation details

template<class T, class ScriptInformation>
bool dispatchEvent(T* e, Script::Manager& state, Script::Environment& environment, Script::Listener_ptr listener)
{
	if(listener->isActive() == false)
		return false;

	const ScriptInformation& info = boost::any_cast<const ScriptInformation>(listener->getData());

	// Call handler
	if(e->check_match(info)) {
		if(e->call(state, environment, listener) == true) {
			// Handled
			return true;
		}
	}
	return false;
}

template<class T>
bool dispatchEvent(T* e, Script::Manager& state, Script::Environment& environment, Script::Listener_ptr listener)
{
	if(listener->isActive() == false)
		return false;

	// Call handler
	if(e->call(state, environment, listener) == true) {
		// Handled
		return true;
	}
	return false;
}

template<class T, class ScriptInformation>
bool dispatchEvent(T* e, Script::Manager& state, Script::Environment& environment, Script::ListenerList& specific_list)
{
	if(specific_list.size() == 0)
		return false;

	for(Script::ListenerList::iterator event_iter = specific_list.begin();
		event_iter != specific_list.end();
		++event_iter)
	{
		if(dispatchEvent<T, ScriptInformation>(e, state, environment, *event_iter))
			return true;
	}
	return false;
}

template<class T> // No script information!
bool dispatchEvent(T* e, Script::Manager& state, Script::Environment& environment, Script::ListenerList& specific_list)
{
	if(specific_list.size() == 0)
		return false;

	for(Script::ListenerList::iterator event_iter = specific_list.begin();
		event_iter != specific_list.end();
		++event_iter)
	{
		if(dispatchEvent<T>(e, state, environment, *event_iter))
			return true;
	}
	return false;
}

#endif // __OTSERV_SCRIPT_EVENT__

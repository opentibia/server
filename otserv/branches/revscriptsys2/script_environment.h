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

#ifndef __OTSERV_SCRIPT_ENVIRONMENT__
#define __OTSERV_SCRIPT_ENVIRONMENT__

#include "classes.h"
#include <boost/bimap.hpp>
#include "enums.h"

namespace Script {
	class Listener;
	typedef boost::shared_ptr<Listener> Listener_ptr;
	typedef boost::weak_ptr<Listener> Listener_wptr;
	//typedef boost::shared_ptr<const Listener> Listener_cptr;

	typedef std::vector<Listener_ptr> ListenerList;
	typedef std::map<uint32_t, ListenerList> ListenerMap;
	typedef struct{
		ListenerMap ItemId;
		ListenerMap ActionId;
	} ListenerItemMap;
	typedef std::map<uint32_t, Listener_ptr> SpecificListenerMap;
	typedef std::map<std::string, ListenerList> ListenerStringMap;

	typedef uint64_t ObjectID;

	class Environment {
	public:
		Environment();
		~Environment();

		void cleanup();

		typedef boost::bimap<ObjectID, void*> ObjectMap;

		// Make sure to use a pointer to the superclass of the object
		ObjectID addThing(Thing* thing);
		ObjectID addObject(void* obj);
		bool reassignThing(Thing* oldthing, Thing* newthing);
		// Returns NULL if there is no object with that ID
		void* getObject(ObjectID obj);
		Thing* getThing(ObjectID id);

		// Removes an object from the scripted object list
		// called when a Thing is destroyed, for example
		bool removeObject(void* obj);
		bool removeObject(ObjectID id);
		bool removeThing(Thing* thing);

	public: // Event maps

		// Listener API
		// Cleanup must be called when no scripts are running as it will invalidate the listener maps
		void cleanupUnusedListeners();

		// This is just so it can be found & stopped effectively
		void registerSpecificListener(Listener_ptr listener);

		// Stop a listener!
		bool stopListener(Listener_ptr listener);
		bool stopListener(ListenerType type, uint32_t id);

		struct {
			ListenerList OnSay;
			ListenerItemMap OnUseItem;
			ListenerList OnEquipItem;
			ListenerList OnMoveCreature;
			ListenerList OnJoinChannel;
			ListenerList OnLeaveChannel;
			ListenerList OnAccountLogin;
			ListenerList OnLogin;
			ListenerList OnLogout;
			ListenerList OnChangeOutfit;
			ListenerItemMap OnLook;
			ListenerList OnLookAtCreature;
			ListenerList OnTurn;
			ListenerList OnLoad;
			ListenerList OnUnload;
			ListenerStringMap OnSpawn;
			ListenerList OnAdvance;
			ListenerList OnTrade;
			ListenerList OnShopPurchase;
			ListenerList OnShopSell;
			ListenerList OnShopClose;
			ListenerItemMap OnMoveItem;
			ListenerList OnMoveItemOnItem;
			ListenerList OnCondition;
			ListenerList OnAttack;
			ListenerList OnDamage;
			ListenerList OnKilled;
			ListenerList OnKill;
			ListenerList OnDeathBy;
			ListenerList OnDeath;
		} Generic;
		SpecificListenerMap specific_listeners;

	protected:
		bool stopListener(ListenerList& list, uint32_t id);
		bool stopListener(ListenerItemMap& item_map, uint32_t id);
		bool stopListener(ListenerStringMap& list, uint32_t id);

		void cleanupUnusedListeners(ListenerList& list);
		void cleanupUnusedListeners(ListenerStringMap& list);
		void cleanupUnusedListeners(ListenerItemMap& list);
		void cleanupUnusedListeners(ListenerMap& list);

		void debugOutput() const; // noop unless passed preprocessor directive

		ObjectID objectID_counter;
		ObjectMap object_map;

		ObjectID getFreeID() {
			return ++objectID_counter;
		}
	};

	inline ObjectID Environment::addObject(void* obj) {
		ObjectMap::right_iterator thing_iter = object_map.right.find(obj);
		if(thing_iter == object_map.right.end()) {
			ObjectID id = getFreeID();
			object_map.left.insert(std::make_pair(id, obj));
#ifdef __DEBUG_SCRIPT_ENVIRONMENT_OBJECTMAP__
			std::cout << "Added object " << obj << " with id " << id << std::endl;
			debugOutput();
#endif
			return id;
		}
		return thing_iter->second;
	}

	inline ObjectID Environment::addThing(Thing* thing) {
		ObjectMap::right_iterator thing_iter = object_map.right.find(thing);
		if(thing_iter == object_map.right.end()) {
			ObjectID id = getFreeID();
			object_map.left.insert(std::make_pair(id, reinterpret_cast<void*>(thing)));
#ifdef __DEBUG_SCRIPT_ENVIRONMENT_OBJECTMAP__
			std::cout << "Added thing " << thing << " with id " << id << std::endl;
			debugOutput();
#endif
			return id;
		}
		return thing_iter->second;
	}

	inline bool Environment::reassignThing(Thing* oldthing, Thing* newthing) {
		ObjectMap::right_iterator obj_iter = object_map.right.find(reinterpret_cast<void*>(oldthing));
		if(obj_iter == object_map.right.end()) {
			return false;
		}
		// Store the ID
		ObjectID id = obj_iter->second;

		// Remove old
		object_map.right.erase(obj_iter);

		// Add the new one back in
		object_map.left.insert(std::make_pair(id, reinterpret_cast<void*>(newthing)));
#ifdef __DEBUG_SCRIPT_ENVIRONMENT_OBJECTMAP__
		std::cout << "Reassigned thing " << oldthing << " with new ref " << newthing << " using id " << id << std::endl;
		debugOutput();
#endif
		return true;
	}

	inline void* Environment::getObject(ObjectID id) {
		ObjectMap::left_iterator id_iter = object_map.left.find(id);
		if(id_iter == object_map.left.end()) {
			return NULL;
		}
		return id_iter->second;
	}

	inline Thing* Environment::getThing(ObjectID id) {
		ObjectMap::left_iterator id_iter = object_map.left.find(id);
		if(id_iter == object_map.left.end()) {
			return NULL;
		}
		return reinterpret_cast<Thing*>(id_iter->second);
	}

	inline bool Environment::removeObject(ObjectID id) {
		ObjectMap::left_iterator id_iter = object_map.left.find(id);
		if(id_iter == object_map.left.end()) {
			return false;
		}
		object_map.left.erase(id_iter);
#ifdef __DEBUG_SCRIPT_ENVIRONMENT_OBJECTMAP__
		std::cout << "Removed object with ID " << id << std::endl;
		debugOutput();
#endif
		return true;
	}

	inline bool Environment::removeObject(void* obj) {
		ObjectMap::right_iterator obj_iter = object_map.right.find(obj);
		if(obj_iter == object_map.right.end()) {
			return false;
		}
		object_map.right.erase(obj_iter);
#ifdef __DEBUG_SCRIPT_ENVIRONMENT_OBJECTMAP__
		std::cout << "Removed Object " << obj << std::endl;
		debugOutput();
#endif
		return true;
	}

	inline bool Environment::removeThing(Thing* thing) {
		ObjectMap::right_iterator thing_iter = object_map.right.find(thing);
		if(thing_iter == object_map.right.end()) {
			return false;
		}
		object_map.right.erase(thing_iter);
#ifdef __DEBUG_SCRIPT_ENVIRONMENT_OBJECTMAP__
		std::cout << "Removed Thing " << thing << std::endl;
		debugOutput();
#endif
		return true;
	}

	inline void Environment::debugOutput() const {
#ifdef __DEBUG_SCRIPT_ENVIRONMENT_OBJECTMAP__
		ObjectMap::left_const_iterator debug_iter = object_map.left.begin();
		std::cout << "Object map, counter is at " << objectID_counter << " :" << std::endl;
		while(debug_iter != object_map.left.end()) {
			std::cout << "\tID: " << debug_iter->first << " ptr: " << debug_iter->second << std::endl;
			++debug_iter;
		}
#endif
	}
}

#endif

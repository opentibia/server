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

#ifndef __OTSERV_SCRIPT_ENVIROMENT__
#define __OTSERV_SCRIPT_ENVIROMENT__

#include <boost/bimap.hpp>
#include <string>
#include "boost_common.h"
#include "enums.h"

class Combat;
class Creature;
class Thing;

namespace Script {
	class Listener;
	typedef boost::shared_ptr<Listener> Listener_ptr;
	typedef boost::weak_ptr<Listener> Listener_wptr;
	//typedef boost::shared_ptr<const Listener> Listener_cptr;

	typedef std::vector<Listener_ptr> ListenerList;
	typedef std::map<uint32_t, Listener_ptr> ListenerMap;

	typedef uint64_t ObjectID;

	class Enviroment {
	public:
		Enviroment();
		~Enviroment();

		void cleanup();

		typedef boost::bimap<ObjectID, void*> ObjectMap;

		// Make sure to use a pointer to the superclass of the object
		ObjectID addObject(Thing* thing);
		ObjectID addObject(Combat* combat);
		// If the object is not scripted already, it will be added
		Thing* getThing(ObjectID id);
		// Removes an object from the scripted object list
		// called when a Thing is destroyed, for example
		bool removeObject(ObjectID id);
		bool removeThing(Thing* thing);

	public: // Event maps

		// Listener API
		// Cleanup must be called when no scripts are running as it will invalidate the listener maps
		void cleanupUnusedListeners();

		// This is just so it can be found & stopped effectively
		void registerSpecificListener(Listener_ptr listener);

		// Stop a listener!
		bool stopListener(Listener* listener);
		bool stopListener(ListenerType type, uint32_t id);

		struct {
			ListenerList OnSay;
		} Generic;
		ListenerMap specific_listeners;

	protected:
		bool stopListener(ListenerList& list, uint32_t id);
		void cleanupUnusedListeners(ListenerList& list);

		void debugOutput() const; // noop unless passed preprocessor directive

		ObjectID objectID_counter;
		ObjectMap object_map;

		ObjectID getFreeID() {
			return ++objectID_counter;
		}
	};

	inline ObjectID Enviroment::addObject(Thing* thing) {
		ObjectMap::right_iterator thing_iter = object_map.right.find(thing);
		if(thing_iter == object_map.right.end()) {
			ObjectID id = getFreeID();
			object_map.left.insert(std::make_pair(id, reinterpret_cast<void*>(thing)));
#ifdef __DEBUG_SCRIPT_ENVIROMENT_OBJECTMAP__
		std::cout << "Added thing " << thing << " with id " << id << std::endl;
		debugOutput();
#endif
			return id;
		}
		return thing_iter->second;
	}

	inline Thing* Enviroment::getThing(ObjectID id) {
		ObjectMap::left_iterator id_iter = object_map.left.find(id);
		if(id_iter == object_map.left.end()) {
			return NULL;
		}
		return reinterpret_cast<Thing*>(id_iter->second);
	}

	inline bool Enviroment::removeObject(ObjectID id) {
		ObjectMap::left_iterator id_iter = object_map.left.find(id);
		if(id_iter == object_map.left.end()) {
			return false;
		}
		object_map.left.erase(id_iter);
#ifdef __DEBUG_SCRIPT_ENVIROMENT_OBJECTMAP__
		std::cout << "Removed object with ID " << id << std::endl;
		debugOutput();
#endif
		return true;
	}

	inline bool Enviroment::removeThing(Thing* thing) {
		ObjectMap::right_iterator thing_iter = object_map.right.find(thing);
		if(thing_iter == object_map.right.end()) {
			return false;
		}
		object_map.right.erase(thing_iter);
#ifdef __DEBUG_SCRIPT_ENVIROMENT_OBJECTMAP__
		std::cout << "Removed Thing " << thing << std::endl;
		debugOutput();
#endif
		return true;
	}

	inline void Enviroment::debugOutput() const {
		ObjectMap::left_const_iterator debug_iter = object_map.left.begin();
		std::cout << "Object map, counter is at " << objectID_counter << " :" << std::endl;
		while(debug_iter != object_map.left.end()) {
			std::cout << "\tID: " << debug_iter->first << " ptr: " << debug_iter->second << std::endl;
			++debug_iter;
		}
	}
}

#endif

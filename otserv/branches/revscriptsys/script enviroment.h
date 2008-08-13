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

class Combat;
class Thing;

namespace Script {
	class Listener;
	typedef boost::shared_ptr<Listener> Listener_ptr;
	typedef boost::weak_ptr<Listener> Listener_wptr;

	typedef std::vector<Listener_ptr> GenericCreatureEventList;
	typedef std::map<Creature*, GenericCreatureEventList> SpecificCreatureEventMap;

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
		bool removeObject(ObjectID id);
		bool removeThing(Thing* thing);

	public: // Event maps

		// Listener API
		// Cleanup must be called when no scripts are running as it will invalidate the listener maps
		void cleanupUnusedListeners();

		bool stopListener(Listener* listener);
		bool stopListener(const std::string& tag);

		struct {
			SpecificCreatureEventMap OnSay;
		} Specific;
		struct {
			GenericCreatureEventList OnSay;
		} Generic;
	protected:
		bool stopListener(GenericCreatureEventList& list, const std::string& tag);
		bool stopListener(SpecificCreatureEventMap& map, const std::string& tag);
		void cleanupUnusedListeners(GenericCreatureEventList& list);
		void cleanupUnusedListeners(SpecificCreatureEventMap& map);

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
		return true;
	}

	inline bool Enviroment::removeThing(Thing* thing) {
		ObjectMap::right_iterator thing_iter = object_map.right.find(thing);
		if(thing_iter == object_map.right.end()) {
			return false;
		}
		object_map.right.erase(thing_iter);
		return true;
	}
}

#endif

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

#include "otpch.h"

#include <string>
#include "shared_ptr.h"

class EventListener;
typedef boost::shared_ptr<EventListener> EventListener_ptr;
typedef boost::weak_ptr<EventListener> EventListener_wptr;

typedef std::vector<EventListener_ptr> GenericCreatureEventList;
typedef std::multimap<Creature*, GenericCreatureEventList> SpecificCreatureEventMap;

typedef ptrdiff_t ScriptObjectID;

class ScriptEnviroment {
public:
	ScriptEnviroment();
	~ScriptEnviroment();

	struct ScriptObjectBlock {
		// Only instansiate with pointers
		template<T> ScriptObjectBlock(const T& t) : type(typeid(T)), id(reinterpret_cast<ScripbObjectID>(t)) {}
		const std::type_info& type;
		const ScriptObjectID id;

		bool operator<(const ScriptObjectBlock& rhs) {
			return id < rhs.id;
		}
	};

	typedef std::set<ScriptObjectBlock> ObjectMap;

	// Make sure to use a pointer to the superclass of the object
	template<typename T> ScriptObjectID addObject(T* p);
	// If the object is not scripted already, it will be added
	template<typename T> T* getObject(ScriptObjectID p);
	template<typename T> bool removeObject(T* p);

public: // Event maps
	struct {
		SpecificCreatureEventMap OnSay;
	} Specific;
	struct {
		GenericCreatureEventList OnSay;
	} Generic;

protected:
	ObjectMap object_map;
};

inline template<typename T>
ScriptObjectID ScriptEnviroment::addObject(T* p) {
	ScriptObjectBlock(p) b;
	ObjectMap::iterator iter = object_map.find(b);
	if(iter == object_map.end()) {
		object_map.insert(b);
	} else if(typeid(T*) != iter->type) {
		// This guarantees a limited form of type safety
		return 0;
	}
	return b.id;
}

inline template<typename T>
T* getObject(ScriptObjectID p) {
	ScriptObjectBlock(p) b;
	ObjectMap::iterator iter = object_map.find(b);
	if(iter == object_map.end()) {
		return NULL;
	} else if(typeid(T*) != iter->type) {
		// This guarantees a limited form of type safety
		return NULL;
	}
	return b.id;
}

inline template<typename T>
bool removeObject(T* p) {
	ScriptObjectBlock(p) b;
	ObjectMap::iterator iter = object_map.find(b);
	if(iter == object_map.end()) {
		return false;
	} else if(typeid(T*) != iter->type) {
		// This guarantees a limited form of type safety
		return false;
	}
	object_map.erase(iter);
	return true;
}

#endif

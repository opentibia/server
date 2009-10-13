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

#ifndef __OTSERV_SCRIPT_MANAGER__
#define __OTSERV_SCRIPT_MANAGER__

#include "lua_manager.h"
#include "boost_common.h"
#include <boost/any.hpp>
#include <set>

namespace Script {
	class Event;
	class Environment;
	class LuaClassType;
	typedef shared_ptr<LuaClassType> LuaClassType_ptr;

	class Manager : public LuaStateManager {
	public:
		Manager(Environment& e);
		virtual ~Manager();

		// Event handling
		bool dispatchEvent(Event& event);
	protected:
		// This actually registers functions!
		// Defined in script functions.cpp
		void registerFunctions();
		void registerClasses();
		void registerMetaMethods();

	protected:
		// Internal representation of functions
		typedef int (LuaState::*CallbackFunctionType)();

		struct ComposedTypeDeclaration {
			std::string name;
			std::string type_name;
			std::vector<std::string> types;
			boost::any default_value;
			int32_t optional_level;
		};

		struct ComposedCallback {
			CallbackFunctionType func;
			std::string name;
			std::vector<ComposedTypeDeclaration> parameters;
		};

		typedef shared_ptr<ComposedCallback> ComposedCallback_ptr;
		typedef std::map<uint32_t, ComposedCallback_ptr> FunctionMap;

		std::map<std::string, LuaClassType_ptr> class_list;
		FunctionMap function_map;
		uint32_t function_id_counter;
		int32_t event_nested_level;

		// Expose functions/classes to lua
		void registerClass(const std::string& cname);
		void registerClass(const std::string& cname, const std::string& parent_class);

		void registerGlobalFunction(const std::string& fdecl, CallbackFunctionType cfunc);
		void registerMemberFunction(const std::string& cname, const std::string& fdecl, CallbackFunctionType cfunc);

		template <class ET>
		void registerEnum();

		// Callback from lua
		static int luaFunctionCallback(lua_State* L);
		static int luaCompareClassInstances(lua_State* L);
		static int luaGetClassInstanceID(lua_State* L);
		static int luaCreateEnum(lua_State* L);

		// Parse arguments
		ComposedCallback_ptr parseFunctionDeclaration(std::string s);
		void parseWhitespace(std::string& str);
		std::string parseIdentifier(std::string& s);
		ComposedTypeDeclaration parseTypeDeclaration(std::string& s);
		boost::any parseDefaultDefinition(std::string& s);

		friend class LuaClassType;
	};


	class LuaClassType {
	public:
		LuaClassType(Manager& manager, std::string name, std::string parent_name = "");

		bool isType(const std::string& type) const;
	protected:
		Manager& manager;
		std::string name;
		std::vector<std::string> parent_classes;
	};
}

// Must be done here since it's a templated function
template<class ET>
inline void Script::Manager::registerEnum()
{
	registerClass(ET::name(), "Enum");

	// Push Enum table
	getGlobal(ET::name()); // index 1

	//std::cout << "Global " << ET::name() << " table is " << lua_topointer(state, 1) << std::endl;
	// Expose members
	int n = 1;
	for(typename ET::iterator ei = ET::begin(); ei != ET::end(); ++ei, ++n){

		// Push class instance
		pushClassTableInstance(ET::name()); // index 2

		// Table to hold string values
		newTable(); // index 3
		// Should iterate over all possible values here
		std::vector<std::string> stringValues = ei->toStrings();
		int in = 1;
		for(std::vector<std::string>::const_iterator str = stringValues.begin(); str != stringValues.end(); ++str, ++in)
			setField(3, in, *str);
		
		// Save the table
		setField(2, "__strValues");
		
		// set integer converted value
		setField(2, "__intValue", ei->value());
		
		// Save first pointer to class instance as a global
		duplicate(2);
		setGlobal(stringValues.front());

		//std::cout << "set field of " << lua_topointer(state, 1) << " to " << lua_topointer(state, -1) << " " << typeName(-1) << std::endl;
		// Other should go into the table for this Enum
		setField(1, n);

		//std::cout << "stack is " << lua_gettop(state) << std::endl;
	}

	// We actually hide the class here in favor of pretty construction of it
	// Another added bonus is that it's impossible to create new Enum values of this
	// type from inside lua
	lua_pushcclosure(state, luaCreateEnum, 1);
	lua_setglobal(state, ET::name().c_str());
}

#endif

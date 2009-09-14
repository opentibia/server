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
			int optional_level;
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
		int event_nested_level;

		// Expose functions/classes to lua
		void registerClass(const std::string& cname);
		void registerClass(const std::string& cname, const std::string& parent_class);

		void registerGlobalFunction(const std::string& fdecl, CallbackFunctionType cfunc);
		void registerMemberFunction(const std::string& cname, const std::string& fdecl, CallbackFunctionType cfunc);

		// Callback from lua
		static int luaFunctionCallback(lua_State* L);
		static int luaCompareClassInstances(lua_State* L);
		static int luaGetClassInstanceID(lua_State* L);

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

#endif

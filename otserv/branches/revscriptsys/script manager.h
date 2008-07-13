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

#include "lua manager.h"

class ScriptManager : public LuaStateManager {
public:
	ScriptManager(ScriptEnviroment& e);
	~ScriptManager();

	// Event handling
	void dispatchEvent(ScriptEvent& event);

protected:
	typedef (ScriptManager::*CallbackFunctionType)();
	struct ComposedTypeDeclaration {
		std::string name;
		std::vector<std::string> types;
		int optional_level;
	};
	struct ComposedCallback {
		CallbackFunctionType func;
		std::string name;
		std::vector<ComposedTypeDeclaration> parameters;
	};
	typedef shared_ptr<ComposedCallback> ComposedCallback_ptr;
	typedef std::map<uint32_t, ComposedCallback_ptr> function_map;
	uint32_t function_id_counter;

	// Single
	void registerGlobalFunction(const std::string& fdecl, CallbackFunctionType cfunc);
	void registerMemberFunction(const std::string& cname, const std::string& fdecl, CallbackFunctionType cfunc);
	// Hidden implantation
	static int luaFunctionCallback(lua_State* L);

	// Parse arguments
	ComposedCallback_ptr parseFunctionDeclaration(std::string s);
	std::string parseIdentifier(std::string& s);
	ComposedTypeDeclaration parseTypeDeclaration(std::string& s);


	// This actually registers functions!
	void registerFunctions();
	void registerClasses();

	// And finally lua functions that are available for scripts
	// Global
	// - Utility
	int lua_wait(CallbackFunction& w);
	// - Register Events
	int lua_registerGenericEvent_OnSay();
	int lua_registerSpecificEvent_OnSay();
	// Classes
	// - Thing
	int lua_Thing_getPosition();
	int lua_Thing_moveToXYZ();
	// - - Creature
	int lua_Creature_getHealth();
	int lua_Creature_getHealthMax();
};

#endif

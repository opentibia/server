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

#include "otpch.h"

#include "script manager.h"

ScriptManager::ScriptManager(ScriptEnviroment& e) : LuaStateManager(e) {

}

ScriptManager::~ScriptManager() {
}

void ScriptManager::handleEvent(ScriptEvent& event) {
	event->dispatch(*this, enviroment);
}


///////////////////////////////////////////////////////////////////////////////
// Register functions

void ScriptManager::luaFunctionCallback(lua_State* L) {
	uint32_t callbackID = uint32_t(lua_tonumber(L, lua_upvalueindex(1)));
	ScriptManager* interface = (ScriptManager*)(lua_touserdata(L, lua_upvalueindex(2)));
	
	try {
		ComposedCallback_ptr cc = function_map[callback];
		
		int argument_count = getStackSize();
		if(argument_count > cc->parameters.size()) {
			throw LuaScriptError("Too many arguments passed to function " + cc->name);
		}
		int parsed_argument_count = 0;
		int last_optional_level = 0;
		for(std::vector<ComposedTypeDeclaration>::const_iterator ctditer = cc->parameters.begin();
				ctditer != cc->parameters.end();
				++ctditer)
		{
			const ComposedTypeDeclaration& ctd = *ctditer;
			parsed_argument_count += 1;

			if(ctd.optional_level == 0) {
				// Required argument
				if(argument_count < parsed_argument_count) {
					throw LuaScriptError("Too few arguments passed to function " + cc->name);
				}
			} else {
				// Optional argument
				if(argument_count < parsed_argument_count) {
					// Optional argument!
					// Perhaps a default argument should be pushed?
					break; // We don't need to parse more
				}
				last_optional_level = ctd.optional_level; 
			}
		}

		parsed_argument_count = 0;
		for(std::vector<ComposedTypeDeclaration>::const_iterator ctditer = cc->parameters.begin();
				ctditer != cc->parameters.end();
				++ctditer)
		{
			const ComposedTypeDeclaration& ctd = *ctditer;
			parsed_argument_count += 1;

			for(std::vector<std::string>::const_iterator type_iter = ctd.types.begin();
					type_iter != ctd.types.end();
					++type_iter)
			{
				if(*type_iter == "boolean" && !isBoolean(argument_count - parsed_argument_count + 1)) {
					throw LuaScriptError("Expected boolean for argument '" + ctd.name + "'");
				} else if(*type_iter == "number" && !isNumber(argument_count - parsed_argument_count + 1)) {
					throw LuaScriptError("Expected number for argument '" + ctd.name + "'");
				} else if(*type_iter == "string" && !isString(argument_count - parsed_argument_count + 1)) {
					throw LuaScriptError("Expected string for argument '" + ctd.name + "'");
				} else if(*type_iter == "function" && !isFunction(argument_count - parsed_argument_count + 1)) {
					throw LuaScriptError("Expected function for argument '" + ctd.name + "'");
				} else if(*type_iter == "userdata" && !isUserdata(argument_count - parsed_argument_count + 1)) {
					throw LuaScriptError("Expected userdata for argument '" + ctd.name + "'");
				} else if(*type_iter == "thread" && !isThread(argument_count - parsed_argument_count + 1)) {
					throw LuaScriptError("Expected thread for argument '" + ctd.name + "'");
				} else if(*type_iter == "table" && !isTable(argument_count - parsed_argument_count + 1)) {
					throw LuaScriptError("Expected table for argument '" + ctd.name + "'");
				}
			}
		}
		// All arguments checked out, call the function!
		return (interface)->*(cc->func);
	} catch(LuaScriptError& err) {
		// We can't use lua_error in the C++ function as it doesn't call destructors properly.
		lua_pushstring(state, err.what());
		lua_error(state);
	}
}

void ScriptManager::registerGlobalFunction(const std::string& fdecl, CallbackFunctionType cfunc) {
	ComposedCallback_ptr func = parseFunctionDeclaration(fdecl);
	func.func = cfunc;
	
	uint32_t function_id = ++function_id_counter;
	function_map[function_id] = func;

	lua_pushlightuserdata(state, (ScriptManager*)this);
	lua_pushnumber(state, function_id);
	lua_pushcclosure(state, luaFunctionCallback, 2);

	lua_setfield(state, LUA_GLOBALSINDEX, func->name.c_str());
}

void ScriptManager::registerMemberFunction(const std::string& cname, const std::string& fdecl, CallbackFunctionType cfunc) {
	ComposedCallback func = parseFunctionDeclaration(fdecl);
	
	// Add the hidden "self" parameter
	ComposedTypeDeclaration ctd;
	ctd.name = "thing";
	ctd.optional_level = 0;
	ctd.types.push_back("userdata");
	func->parameters.insert(func->parameters.begin(), ctd);

	func->name = cname + ":" + func->name;
	func->func = cfunc;
	
	uint32_t function_id = ++function_id_counter;
	function_map[function_id] = func;

	lua_getfield(state, LUA_REGISTRYINDEX, ("OTClass_" + cname).c_str());
	
	lua_pushlightuserdata(state, (ScriptManager*)this);
	lua_pushnumber(state, function_id);
	lua_pushcclosure(state, luaFunctionCallback, 2);

	lua_setfield(state, 2, func->name.c_str());
}

void ScriptManager::registerClasses() {
	registerClass("Thing");
	registerMemberFunction("Thing", "getPosition()", lua_Thing_getPosition);
	registerMemberFunction("Thing", "moveTo(int x, int y, int z)", lua_Thing_moveToXYZ);

	registerClass("Creature", "Thing");
	registerMemberFunction("Creature", "getHealth()", lua_Creature_getHealth);
	registerMemberFunction("Creature", "getHealthMax()", lua_Creature_getHealthMax);

	registerClass("Monster", "Creature");

	registerClass("Player", "Creature");

	registerClass("Item", "Thing");

	registerClass("Teleport", "Item");

	registerClass("Container", "Item");

	registerClass("Tile");
}

void ScriptManager::registerFreeFunctions() {
	registerGlobalFunction("wait(int delay)"), lua_wait);

	registerGlobalFunction("registerGenericOnSayListener(string method, string filter, function callback)"), lua_registerGenericEvent_OnSay);
}

///////////////////////////////////////////////////////////////////////////////
// Internal parsing of arguments

ComposedCallback_ptr ScriptManager::parseFunctionDeclaration(std::string s) {
	ComposedCallback_ptr cc(new ComposedCallback);
	// Parse name
	parseWhitespace(s);
	cc->name = parseIdentifier(s);
	parseWhitespace(s);
	if(s.size() == 0) {
		// No parameters
		return cc;
	}
	assert(s[0] == '(');
	s.erase(s.begin());
	bool optional_level = 0;
	
	parseWhitespace(s);
	// Size must at least be 1 so there can be a closing )
	assert(s.size() > 0);
	if(s[0] == ')') {
		// Do nothing
	} else {
		if(s[0] == '[') {
			optional_level += 1;
			s.erase(s.begin());
		}

		while(true) {
			parseWhitespace(s);
			assert(s.size() > 0);

			ComposedTypeDeclaration type = parseTypeDeclaration(s);
			type.optional_level = optional_level;
			cc->parameters.push_back(type);

			parseWhitespace(s);

			assert(s.size() > 0);
			assert(s[0] == ',' || s[0] == '[' || s[0] == ')' || s[0] == ']');

			if(s[0] == ',') {
				s.erase(s.begin());
			} else if(s[0] == '[')  {
				s.erase(s.begin();
				parseWhitespace(s);
				assert(s.size() > 0);
				assert(s[0]  == ',');
				optional_level += 1;
			} else if(s[0] == ']') {
				assert(optional_level > 0);
				while(optional_level > 0) {
					// After a ']' only another ']' can follow
					assert(s[0] == ']')
					parseWhitespace(s);
					optional_level -= 1;
				}
				// There must be an equal amount of [ and ]
				assert(optional_level == 0);
				// After ]]]] the declaration must end
				assert(s[0] == ')')
				break;
			} else if(s[0] == ')') {
				// There must be an equal amount of [ and ]
				assert(optional_level == 0);
				break;
			}
		}
	}
	return cc;
}

void ScriptManager::parseWhitespace(std::string& s) {
	std::string::iterator iter = s.begin();
	while(*iter == ' ' || *iter == '\t' || *iter == '\n') {
		++iter;
	}
	s.erase(s.begin(), iter);
}

std::string ScriptManager::parseIdentifier(std::string &s) {
	std::string::iterator iter = s.begin();
	assert(
		(*iter >= 'a' && *iter <= 'z') || 
		(*iter >= 'A' && *iter <= 'Z') ||
		(*iter == '_'));
	++iter;
	while(
		(*iter >= 'a' && *iter <= 'z') || 
		(*iter >= 'A' && *iter <= 'Z') ||
		(*iter == '_') ||
		(*iter >= '0' && *iter <= '9'))
	{
		++iter;
	}
	std::string r(s.begin(), iter);
	s.erase(s.begin(), iter);
	return r;
}

ComposedTypeDeclaration ScriptManager::parseTypeDeclaration(std::string& s) {
	ComposedTypeDeclaration ctd;
	while(true) {
		std::string type = parseIdentifier(s);
		if(type == "int") type = "number";
		// Argument must be a valid lua type
		assert(
			type == "boolean" ||
			type == "number" ||
			type == "string" ||
			type == "function" ||
			type == "userdata" ||
			type == "thread" ||
			type == "table"
			);
		ctd.types.push_back(type);
		parseWhitespace(s);
		if(s[0] != '|') {
			break;
		}
	}
	ctd.name = parseIdentifier(s);
}

///////////////////////////////////////////////////////////////////////////////
// Lua functions!


///////////////////////////////////////////////////////////////////////////////
// Utility functions

int ScriptManager::lua_wait() {
	checkStackSize(1);
	int32_t ms = popInteger();
	pushInteger(ms);
	return lua_yield(state, 1);
}

///////////////////////////////////////////////////////////////////////////////
// Register Events
int ScriptManager::lua_registerGenericEvent_OnSay() {
	return lua_yield(state, 1);
}


///////////////////////////////////////////////////////////////////////////////
// Member functions

int ScriptManager::lua_Thing_getPosition() {
	Thing* thing = popThing();
	pushPosition(thing->getPosition());
	return 1;
}

int ScriptManager::lua_Thing_moveToXYZ() {
	Thing* thing = popThing();
	int x, y, z;
	z = popInteger();
	y = popInteger();
	x = popInteger();
	
	

	return 1;
}

int ScriptManager::lua_Creature_getHealth() {
	Creature* creature = popCreature();
	pushInteger(creature->getHealth());
	return 1;
}

int ScriptManager::lua_Creature_getHealthMax() {
	Creature* creature = popCreature();
	pushInteger(creature->getMaxHealth());
	return 1;
}

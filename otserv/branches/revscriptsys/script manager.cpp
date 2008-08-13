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
#include "script event.h"
#include "script enviroment.h"

using namespace Script;

Manager::Manager(Script::Enviroment& e) : LuaStateManager(e), function_id_counter(1) {
	registerFunctions();
	registerClasses();
}

Manager::~Manager() {
}

void Manager::dispatchEvent(Script::Event& event) {
	event.dispatch(*this, enviroment);
}

Manager* Manager::getManager() {
	return this;
}

///////////////////////////////////////////////////////////////////////////////
// Register functions

int Manager::luaFunctionCallback(lua_State* L) {
	uint32_t callbackID = uint32_t(lua_tonumber(L, lua_upvalueindex(2)));
	Manager* manager = (Manager*)(lua_touserdata(L, lua_upvalueindex(1)));
	LuaState* interface = NULL;
	if(L == manager->state) {
		interface = manager;
	}
	else {
		LuaThread_ptr p = manager->threads[L];
		interface = p.get();
	}
	
	assert(interface);

	try {
		ComposedCallback_ptr cc = manager->function_map[callbackID];
		
		int argument_count = interface->getStackTop();
		if(argument_count > cc->parameters.size()) {
			throw Script::Error("Too many arguments passed to function " + cc->name);
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
					throw Script::Error("Too few arguments passed to function " + cc->name);
				}
			}
			else {
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

			std::string expected_type = "";

			for(std::vector<std::string>::const_iterator type_iter = ctd.types.begin();
					type_iter != ctd.types.end();
					++type_iter)
			{
				if(*type_iter == "boolean") {
					if(interface->isBoolean(parsed_argument_count)) {
						expected_type = "";
						break;
					}
					expected_type = "boolean";
				}
				else if(*type_iter == "number") {
					if(interface->isNumber(parsed_argument_count)) {
						expected_type = "";
						break;
					}
					expected_type = "number";
				}
				else if(*type_iter == "string") {
					if(interface->isString(parsed_argument_count)) {
						expected_type = "";
						break;
					}
					expected_type = "string";
				}
				else if(*type_iter == "function") {
					if(interface->isFunction(parsed_argument_count)) {
						expected_type = "";
						break;
					}
					expected_type = "function";
				}
				else if(*type_iter == "userdata") {
					if(interface->isUserdata(parsed_argument_count)) {
						expected_type = "";
						break;
					}
					expected_type = "userdata";
				}
				else if(*type_iter == "thread") {
					if(interface->isThread(parsed_argument_count)) {
						expected_type = "";
						break;
					}
					expected_type = "thread";
				}
				else if(*type_iter == "table") {
					if(interface->isTable(parsed_argument_count)) {
						expected_type = "";
						break;
					}
					expected_type = "table";
				}
			}
			if(expected_type != "") {
				std::ostringstream os;
				os <<
					"When calling function " <<
					"'" << cc->name << "'" <<
					" - Expected '" << expected_type << "' for argument " <<
					"'" << ctd.name << "'" << 
					" type was " << 
					"'" << interface->typeOf(parsed_argument_count) << "'"; 
				throw Error(os.str());
			}
		}
		// All arguments checked out, call the function!
		return (interface->*(cc->func))();
	} catch(Script::Error& err) {
		// We can't use lua_error in the C++ function as it doesn't call destructors properly.
		std::cout << "top:" << interface->getStackTop() << "\n";
		interface->clearStack();
		interface->pushString(err.what());
		return lua_error(interface->state);
	}
}

///////////////////////////////////////////////////////////////////////////////
// Internal parsing of arguments

Manager::ComposedCallback_ptr Manager::parseFunctionDeclaration(std::string s) {
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
	int optional_level = 0;

	parseWhitespace(s);
	// Size must at least be 1 so there can be a closing )
	assert(s.size() > 0);
	if(s[0] == ')') {
		// Do nothing
	}
	else {
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
			}
			else if(s[0] == '[')  {
				s.erase(s.begin());
				parseWhitespace(s);
				assert(s.size() > 0);
				assert(s[0]  == ',');
				optional_level += 1;
			}
			else if(s[0] == ']') {
				assert(optional_level > 0);
				while(optional_level > 0) {
					// After a ']' only another ']' can follow
					assert(s[0] == ']');
					parseWhitespace(s);
					optional_level -= 1;
				}
				// There must be an equal amount of [ and ]
				assert(optional_level == 0);
				// After ]]]] the declaration must end
				assert(s[0] == ')');
				break;
			}
			else if(s[0] == ')') {
				// There must be an equal amount of [ and ]
				assert(optional_level == 0);
				break;
			}
		}
	}
	return cc;
}

void Manager::parseWhitespace(std::string& s) {
	std::string::iterator iter = s.begin();
	while(*iter == ' ' || *iter == '\t' || *iter == '\n') {
		++iter;
	}
	s.erase(s.begin(), iter);
}

std::string Manager::parseIdentifier(std::string &s) {
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

Manager::ComposedTypeDeclaration Manager::parseTypeDeclaration(std::string& s) {
	ComposedTypeDeclaration ctd;
	while(true) {
		std::string type = parseIdentifier(s);
		ctd.type_name = type;
		// Argument must be a valid lua type

		// Adjust custom types
		if(type == "int" || type == "float") {
			type = "number";
		}
		if(type == "position") {
			type = "table";
		}

		// Checked on server start
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

	return ctd;
}


///////////////////////////////////////////////////////////////////////////////
// Register Lua Classes

void Manager::registerClass(const std::string& cname) {
	lua_newtable(state); // Metatable
	lua_pushvalue(state, -1); // Another reference to the table
	lua_setfield(state, LUA_REGISTRYINDEX, ("OTClass_" + cname).c_str());

	lua_newtable(state); // Class table
	lua_pushvalue(state, -1); // Another reference to the table
	lua_setfield(state, LUA_GLOBALSINDEX, cname.c_str());

	lua_setfield(state, -2, "__index"); // Set the index metamethod for the class metatable to the class table

	lua_pop(state, 1); // Pop the class metatable
}


void Manager::registerClass(const std::string& cname, const std::string& parent_class) {
	lua_newtable(state); // Metatable
	lua_pushvalue(state, -1); // Another reference to the table
	lua_setfield(state, LUA_REGISTRYINDEX, ("OTClass_" + cname).c_str());
	
	lua_newtable(state); // Class table
	lua_pushvalue(state, -1); // Another reference to the table
	lua_setfield(state, LUA_GLOBALSINDEX, cname.c_str());

	lua_setfield(state, -2, "__index"); // Set the index metamethod for the class metatable to the class table

	lua_pop(state, 1); // Pop the class metatable

	lua_getfield(state, LUA_GLOBALSINDEX, cname.c_str()); // Add the derived class table to the top of the stack
	
	lua_newtable(state); // Create a small redirect table
	lua_getfield(state, LUA_GLOBALSINDEX, parent_class.c_str()); // Get the parent table
	lua_setfield(state, -2, "__index"); // Set the index metamethod for the redirect table to the base class table

	lua_setmetatable(state, -2); // Set the metatable of the derived class table to the redirect table
	lua_pop(state, 1); // Pop the derived class table
}

///////////////////////////////////////////////////////////////////////////////
// Register handling functions

void Manager::registerGlobalFunction(const std::string& fdecl, CallbackFunctionType cfunc) {
	ComposedCallback_ptr func = parseFunctionDeclaration(fdecl);
	func->func = cfunc;
	
	uint32_t function_id = ++function_id_counter;
	function_map[function_id] = func;

	lua_pushlightuserdata(state, (Manager*)this);
	lua_pushnumber(state, function_id);
	lua_pushcclosure(state, luaFunctionCallback, 2);

	lua_setfield(state, LUA_GLOBALSINDEX, func->name.c_str());
}

void Manager::registerMemberFunction(const std::string& cname, const std::string& fdecl, CallbackFunctionType cfunc) {
	ComposedCallback_ptr func = parseFunctionDeclaration(fdecl);
	
	// Add the hidden "self" parameter
	ComposedTypeDeclaration ctd;
	ctd.name = cname;
	ctd.optional_level = 0;
	ctd.types.push_back("userdata");
	func->parameters.insert(func->parameters.begin(), ctd);

	std::string funcname = func->name;
	func->name = cname + ":" + funcname;
	func->func = cfunc;
	
	uint32_t function_id = ++function_id_counter;
	function_map[function_id] = func;

	lua_getfield(state, LUA_GLOBALSINDEX, cname.c_str());
	lua_pushlightuserdata(state, (Manager*)this);
	lua_pushnumber(state, function_id);
	lua_pushcclosure(state, luaFunctionCallback, 2);

	lua_setfield(state, -2, funcname.c_str());

	lua_pop(state, 1);
}

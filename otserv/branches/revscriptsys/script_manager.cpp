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

#include <sstream>

#include "script_manager.h"
#include "script_event.h"
#include "script_enviroment.h"

using namespace Script;

Manager::Manager(Script::Enviroment& e) : LuaStateManager(e), function_id_counter(1) {
	registerClasses();
	registerFunctions();
}

Manager::~Manager() {
}

bool Manager::dispatchEvent(Script::Event& event) {
	return event.dispatch(*this, enviroment);
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
		if((unsigned int)argument_count > cc->parameters.size()) {
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
		
		int passed_argument_count = parsed_argument_count;

		parsed_argument_count = 0;
		for(std::vector<ComposedTypeDeclaration>::const_iterator ctditer = cc->parameters.begin();
				ctditer != cc->parameters.end();
				++ctditer)
		{
			const ComposedTypeDeclaration& ctd = *ctditer;
			
			if(parsed_argument_count < passed_argument_count) {
				break;
			}
			parsed_argument_count += 1;
			
			std::string expected_type = "";

			for(std::vector<std::string>::const_iterator type_iter = ctd.types.begin();
					type_iter != ctd.types.end();
					++type_iter)
			{
				if(*type_iter == "mixed") {
						expected_type = "";
					break;
				}
				else if(*type_iter == "boolean") {
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
				s.erase(s.begin());
				parseWhitespace(s);
				optional_level += 1;
			}
			else if(s[0] == ']') {
				s.erase(s.begin());
				parseWhitespace(s);
				assert(optional_level > 0);
				optional_level -= 1;

				while(optional_level > 1) {
					// After a ']' only another ']' can follow
					assert(s[0] == ']');
					s.erase(s.begin());
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
		if(type == "int" || type == "integer" || type == "float") {
			type = "number";
		}
		if(type == "position") {
			type = "table";
		}

		// Checked on server start
		assert(
			type == "mixed" ||
			type == "boolean" ||
			type == "number" ||
			type == "string" ||
			type == "function" ||
			type == "userdata" ||
			type == "thread" ||
			type == "table" ||
			class_list.find(type) != class_list.end()
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
// Lua class type

LuaClassType::LuaClassType(Manager& manager, std::string name, std::string parent_name) :
	manager(manager),
	name(name)
{
	if(parent_name == "") {
		return;
	}
	std::map<std::string, LuaClassType_ptr>::iterator iter = manager.class_list.find(parent_name);
	// If this fails you have a class inheriting from a non-existant class
	assert(iter != manager.class_list.end());

	LuaClassType_ptr pc = iter->second;
	parent_classes.push_back(iter->first);
	std::copy(pc->parent_classes.begin(), pc->parent_classes.end(), parent_classes.begin());
}

bool LuaClassType::isType(const std::string& typestring) const {
	if(typestring == name) {
		return true;
	}
	if(std::find(parent_classes.begin(), parent_classes.end(), typestring) == parent_classes.end()) {
		return false;
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////
// Register Lua Classes

void Manager::registerClass(const std::string& cname) {
	lua_newtable(state); // Metatable
	lua_pushvalue(state, -1); // Another reference to the table
	lua_setfield(state, LUA_REGISTRYINDEX, ("OTClass_" + cname).c_str());

	// Create the meta-relations
	lua_newtable(state); // Class table
	lua_pushvalue(state, -1); // Another reference to the table
	lua_setfield(state, LUA_GLOBALSINDEX, cname.c_str());
	
	// Set the index metamethod for the class metatable to the class table
	lua_setfield(state, -2, "__index"); 

	// Pop the class metatable
	lua_pop(state, 1); 

	// Insert into the C++ class list for future reference
	class_list[cname] = LuaClassType_ptr(new LuaClassType(*this, cname));
}


void Manager::registerClass(const std::string& cname, const std::string& parent_class) {
	lua_newtable(state); // Metatable
	lua_pushvalue(state, -1); // Another reference to the table
	lua_setfield(state, LUA_REGISTRYINDEX, ("OTClass_" + cname).c_str());
	
	lua_newtable(state); // Class table
	lua_pushvalue(state, -1); // Another reference to the table
	lua_setfield(state, LUA_GLOBALSINDEX, cname.c_str());

	// Set the index metamethod for the class metatable to the class table
	lua_setfield(state, -2, "__index"); 

	lua_pop(state, 1); // Pop the class metatable

	// Add the derived class table to the top of the stack
	lua_getfield(state, LUA_GLOBALSINDEX, cname.c_str()); 
	
	// Create the meta-relations
	lua_newtable(state); // Create a small redirect table
	lua_getfield(state, LUA_GLOBALSINDEX, parent_class.c_str()); // Get the parent table
	lua_setfield(state, -2, "__index"); // Set the index metamethod for the redirect table to the base class table

	// Set the metatable of the derived class table to the redirect table
	lua_setmetatable(state, -2); 
	// Pop the derived class table
	lua_pop(state, 1);

	// Insert into the C++ class list for future reference
	class_list[cname] = LuaClassType_ptr(new LuaClassType(*this, cname, parent_class));
}

///////////////////////////////////////////////////////////////////////////////
// Register handling functions

void Manager::registerGlobalFunction(const std::string& fdecl, CallbackFunctionType cfunc) {
	// Create the composed callback containing all the type info etc.
	ComposedCallback_ptr func = parseFunctionDeclaration(fdecl);
	// Store the C callback in the composed callback
	func->func = cfunc;
	
	// Generate unique function ID, and associate it with the composed function
	uint32_t function_id = ++function_id_counter;
	function_map[function_id] = func;

	// Push this manager (pointer cast is important!)
	lua_pushlightuserdata(state, (Manager*)this);
	// Push the function ID
	lua_pushnumber(state, function_id);
	// Create the function, with the manager & id stored in the closure
	// the callback extracts the function and manager and calls it.
	lua_pushcclosure(state, luaFunctionCallback, 2);

	// Store the function in the global lua table
	lua_setfield(state, LUA_GLOBALSINDEX, func->name.c_str());
}

void Manager::registerMemberFunction(const std::string& cname, const std::string& fdecl, CallbackFunctionType cfunc) {
	// Create the composed callback containing all type info etc.
	ComposedCallback_ptr func = parseFunctionDeclaration(fdecl);
	
	// Add the hidden "self" parameter
	ComposedTypeDeclaration ctd;
	ctd.name = cname;
	ctd.optional_level = 0;
	ctd.types.push_back(cname); // Class name is first parameter type
	func->parameters.insert(func->parameters.begin(), ctd);

	// Construct function name, for debug purposes
	std::string funcname = func->name;
	func->name = cname + ":" + funcname;
	func->func = cfunc;
	
	// Create a unique ID for the function, and associate it with the composed callback
	uint32_t function_id = ++function_id_counter;
	function_map[function_id] = func;

	// Push the class table
	lua_getfield(state, LUA_GLOBALSINDEX, cname.c_str());
	// Push this manager (pointer cast is important!)
	lua_pushlightuserdata(state, (Manager*)this);
	// Push the function ID
	lua_pushnumber(state, function_id);
	// Create the function, with the manager & id stored in the closure
	// the callback extracts the function and manager and calls it.
	lua_pushcclosure(state, luaFunctionCallback, 2);
	
	// Store the function at the class field funcname
	lua_setfield(state, -2, funcname.c_str());

	// Remove the class table
	lua_pop(state, 1);
}

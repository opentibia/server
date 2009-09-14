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

#include "script_manager.h"
#include "script_event.h"
#include "script_environment.h"
#include "configmanager.h"

extern ConfigManager g_config;

using namespace Script;

Manager::Manager(Script::Environment& e) : LuaStateManager(NULL),
	function_id_counter(1),
	event_nested_level(0)
{
	manager = this;
	environment = &e;

	registerClasses();
	registerFunctions();
}

Manager::~Manager() {
}

bool Manager::dispatchEvent(Script::Event& event) {
	if(event_nested_level > g_config.getNumber(ConfigManager::MAXIMUM_SCRIPT_RECURSION_DEPTH))
		// Event not handled
		return false;

	event_nested_level++;
	bool s = event.dispatch(*this, *environment);
	event_nested_level--;
	return s;
}

///////////////////////////////////////////////////////////////////////////////
// Register functions

int Manager::luaCompareClassInstances(lua_State* L)
{
	/*
	std::cout << "Compare" << std::endl;
	int n = lua_gettop(L);
	while(--n >= 0)
		std::cout << luaL_typename(L, -n-1) << std::endl;
	*/

	//Manager* manager = (Manager*)(lua_touserdata(L, lua_upvalueindex(1)));
	//Environment& e = manager->environment;

	// 2 class instances are ontop of the stack
	Script::ObjectID* objid1 = (Script::ObjectID*)lua_touserdata(L, -1);
	Script::ObjectID* objid2 = (Script::ObjectID*)lua_touserdata(L, -2);

	lua_pop(L, 2);

	if(*objid1 == *objid2)
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);

	return 1;
}

int Manager::luaGetClassInstanceID(lua_State* L)
{
	/*
	std::cout << "GetID" << std::endl;
	int n = lua_gettop(L);
	while(--n >= 0)
		std::cout << luaL_typename(L, -n-1) << std::endl;
	*/

	// Phantom instance (always NIL)
	lua_pop(L, 1);

	// A class instances are ontop of the stack
	Script::ObjectID* objid = (Script::ObjectID*)lua_touserdata(L, -1);
	lua_pop(L, 1);

	if(objid)
		lua_pushnumber(L, (double)*objid);
	else
		lua_pushnil(L);

	return 1;
}

int Manager::luaFunctionCallback(lua_State* L) {
	// Do NOT allocate complex types here, lua_error is called, which causes a longjump,
	// so complex destructors won't be called.
	uint32_t callbackID = uint32_t(lua_tonumber(L, lua_upvalueindex(2)));
	Manager* manager = (Manager*)(lua_touserdata(L, lua_upvalueindex(1)));
	LuaState* state = NULL;

	// We must allocate manually, since lua_error is called no destructors will work
	unsigned char threadmem[sizeof(LuaThread)];
	LuaThread* private_thread = NULL;
	// REMEMBER TO EXPLICITLY CALL private_thread->~LuaThread

	if(L == manager->state) {
		state = manager;
	}
	else {
		ThreadMap::iterator finder = manager->threads.find(L);
		if(finder != manager->threads.end())
			state = (finder->second).get();
		else{
			private_thread = new(threadmem) LuaThread(manager, L);
			state = private_thread;
		}
	}

	// If the script failed
	try {
		// In here it's safe to allocate complex objects
		ComposedCallback_ptr cc = manager->function_map[callbackID];

		int argument_count = state->getStackSize();
		if((unsigned int)argument_count > cc->parameters.size()) {
			throw Script::Error("Too many arguments passed to function " + cc->name);
		}

		// This loop counts how many argument required arguments we need, at least
		// This could probably be done at register time out
		int required_arguments = 0;

		for(std::vector<ComposedTypeDeclaration>::const_iterator ctditer = cc->parameters.begin();
				ctditer != cc->parameters.end();
				++ctditer)
		{
			if(ctditer->optional_level == 0)
				required_arguments += 1;
		}
		
		if(argument_count < required_arguments) {
			throw Script::Error("Too few arguments passed to function " + cc->name);
		}

		// parsed_argument_count is how many arguments we have parsed in the loop so far
		int parsed_argument_count = 0;
		for(std::vector<ComposedTypeDeclaration>::const_iterator ctditer = cc->parameters.begin();
				ctditer != cc->parameters.end();
				++ctditer)
		{
			const ComposedTypeDeclaration& ctd = *ctditer;

			bool ignoreTypeCheck = false;

			if(required_arguments - parsed_argument_count > 0 && !ctd.default_value.empty() && ctd.optional_level > 0) {
				// We got an optional argument, and one is missing on this spot!
				// Push our default argument onto the stack
				if(ctd.default_value.type() == typeid(std::string))
					state->push(boost::any_cast<std::string>(ctd));
				else if(ctd.default_value.type() == typeid(int))
					state->push(boost::any_cast<std::string>(ctd));
				else if(ctd.default_value.type() == typeid(void*))
					state->pushNil();
				else
					throw Error("Cannot deduce type of optional argument default value (function " + ctd.name + ") (source error)");
				// inject it to the correct position
				state->insert(parsed_argument_count+1);
				// We now have one more argument passed
				required_arguments += 1;

				ignoreTypeCheck = true;
			}

			if(parsed_argument_count >= required_arguments) {
				// We have already parsed all passed arguments
				break;
			}
			parsed_argument_count += 1;

			if(ignoreTypeCheck)
				continue;

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
					if(state->isBoolean(parsed_argument_count)) {
						expected_type = "";
						break;
					}
					expected_type = "boolean";
				}
				else if(*type_iter == "number") {
					if(state->isNumber(parsed_argument_count)) {
						expected_type = "";
						break;
					}
					expected_type = "number";
				}
				else if(*type_iter == "string") {
					if(state->isString(parsed_argument_count)) {
						expected_type = "";
						break;
					}
					expected_type = "string";
				}
				else if(*type_iter == "function") {
					if(state->isFunction(parsed_argument_count)) {
						expected_type = "";
						break;
					}
					expected_type = "function";
				}
				else if(*type_iter == "userdata") {
					if(state->isUserdata(parsed_argument_count)) {
						expected_type = "";
						break;
					}
					expected_type = "userdata";
				}
				else if(*type_iter == "thread") {
					if(state->isThread(parsed_argument_count)) {
						expected_type = "";
						break;
					}
					expected_type = "thread";
				}
				else if(*type_iter == "table") {
					if(state->isTable(parsed_argument_count)) {
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
					"'" << state->typeOf(parsed_argument_count) << "'";
				throw Error(os.str());
			}
		}
		// All arguments checked out, call the function!
		int ret = (state->*(cc->func))();
		if(private_thread)
			private_thread->~LuaThread();
		return ret;
	} catch(Script::Error& err) {
		// We can't use lua_error in the C++ function as it doesn't call destructors properly.
		state->clearStack();
		state->pushString(err.what());
	}

	if(private_thread)
		private_thread->~LuaThread();

	// Can't be done in handler, since then the destructor of Script::Error
	// won't be called, which in turn won't call the destructor of the
	// std::string object it owns
	return lua_error(state->state);
}

///////////////////////////////////////////////////////////////////////////////
// Internal parsing of arguments

Manager::ComposedCallback_ptr Manager::parseFunctionDeclaration(std::string s)
{
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
	// We can't have optional arguments in two places in the argument list
	bool already_optional = false;

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

			parseWhitespace(s);
			assert(s.size() > 0);

			if(s[0] == '='){
				// Parse default value
				s.erase(s.begin());

				parseWhitespace(s);
				assert(s.size() > 0);

				type.default_value = parseDefaultDefinition(s);
				
				parseWhitespace(s);
			}

			// Modifications to type will be ignored after this line
			cc->parameters.push_back(type);

			assert(s.size() > 0);
			assert(s[0] == ',' || s[0] == '[' || s[0] == ')' || s[0] == ']');

			if(s[0] == ',') {
				s.erase(s.begin());
			}
			else if(s[0] == '[')  {
				// We can't have two optional groups in the same argument list!
				assert(already_optional == false);

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

				while(true) {
					if(s[0] == ','){
						// Declaration continues after this, but we decreased optional_level already
						s.erase(s.begin());
						parseWhitespace(s);
						already_optional = true;
						break;
					}
					else{
						if(optional_level == 0)
							break;

						// If declaration does not continue, after a ']' only another ']' can follow
						assert(s[0] == ']');
						s.erase(s.begin());
						parseWhitespace(s);
						optional_level -= 1;
					}
				}
				// We have parsed an optional group inside the argument list
				if(already_optional)
					continue;
				break;
			}
			else if(s[0] == ')') {
				// There must be an equal amount of [ and ]
				assert(optional_level == 0);
				break;
			}
		}
		
		// There must be an equal amount of [ and ]
		assert(optional_level == 0);
	}
	return cc;
}

void Manager::parseWhitespace(std::string& s)
{
	std::string::iterator iter = s.begin();
	while(*iter == ' ' || *iter == '\t' || *iter == '\n') {
		++iter;
	}
	s.erase(s.begin(), iter);
}

std::string Manager::parseIdentifier(std::string &s)
{
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

Manager::ComposedTypeDeclaration Manager::parseTypeDeclaration(std::string& s)
{
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
		if(type == "bool") {
			type = "boolean";
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

boost::any Manager::parseDefaultDefinition(std::string& s)
{
	if(s[0] == '"'){
		// string
		std::string::iterator si = std::find(s.begin() + 1, s.end(), '"');
		assert(si != s.end());
		
		std::string name(s.begin() + 1, si);

		s.erase(s.begin(), si);

		return boost::any(s);
	}
	else if(s[0] >= '0' && s[0] <= '9'){
		// integer
		std::string::iterator si = s.begin() + s.find_first_not_of("0123456789");
		assert(si != s.end());
		
		std::string name(s.begin(), si);

		s.erase(s.begin(), si);

		// Create the integer
		int tmpi = 0;
		std::istringstream is(s);
		is >> tmpi;

		return boost::any(tmpi);
	}
	else if(s.size() > 3 && s.substr(0, 3) == "nil"){
		s.erase(s.begin(), s.begin() + 3);
		return boost::any((void*)1);
	}
	// Must be of one of the above types
	assert(false);

	// To avoid compiler warnings
	return boost::any();
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

void Manager::registerMetaMethods()
{
	// Push this manager (pointer cast is important!)
	lua_pushlightuserdata(state, (Manager*)this);
	// Create the function, with the manager stored in the closure
	lua_pushcclosure(state, &Manager::luaCompareClassInstances, 1);
	lua_setfield(state, -2, "__eq");

	// The # operator, which is "get ID", not length
	// Push this manager (pointer cast is important!)
	lua_pushlightuserdata(state, (Manager*)this);
	// Create the function, with the manager stored in the closure
	lua_pushcclosure(state, &Manager::luaGetClassInstanceID, 1);
	lua_setfield(state, -2, "__len");
}

void Manager::registerClass(const std::string& cname)
{
	lua_newtable(state); // Metatable
	registerMetaMethods();

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


void Manager::registerClass(const std::string& cname, const std::string& parent_class)
{
	lua_newtable(state); // Metatable
	registerMetaMethods();

	// Another reference to the table
	lua_pushvalue(state, -1);
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

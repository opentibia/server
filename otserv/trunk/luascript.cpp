//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// class which takes care of all data which must get saved in the player
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


extern "C"
{
#include <lua.h>
#include <lauxlib.h>
}

#include <string>
#include <iostream>

using namespace std;

#include "luascript.h"



LuaScript::LuaScript()
{
  luaState = NULL;
}


LuaScript::~LuaScript()
{
  if (luaState)
	  lua_close(luaState);
}


int LuaScript::OpenFile(const char *filename)
{
	luaState = lua_open();

	if (lua_dofile(luaState, filename))
		return false;

  return true;
}


string LuaScript::getGlobalString(string var, const string &defString)
{
	lua_getglobal(luaState, var.c_str());

  if(!lua_isstring(luaState, -1))
  	  return defString;

	int len = lua_strlen(luaState, -1);
	string ret(lua_tostring(luaState, -1), len);
	lua_pop(luaState,1);

	return ret;
}


int LuaScript::setGlobalString(string var, string val)
{
	return false;
}

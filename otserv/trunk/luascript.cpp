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
#include "otpch.h"

#include <string>
#include <iostream>

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


std::string LuaScript::getGlobalString(std::string var, const std::string &defString)
{
	lua_getglobal(luaState, var.c_str());

  if(!lua_isstring(luaState, -1))
  	  return defString;

	int len = (int)lua_strlen(luaState, -1);
	std::string ret(lua_tostring(luaState, -1), len);
	lua_pop(luaState,1);

	return ret;
}

int LuaScript::getGlobalNumber(std::string var, const int defNum)
{
	lua_getglobal(luaState, var.c_str());

  if(!lua_isnumber(luaState, -1))
  	  return defNum;

	int val = (int)lua_tonumber(luaState, -1);
	lua_pop(luaState,1);

	return val;
}


int LuaScript::setGlobalString(std::string var, std::string val)
{
	return false;
}

int LuaScript::setGlobalNumber(std::string var, int val){
	lua_pushnumber(luaState, val);
	lua_setglobal(luaState, var.c_str());
	return true;
}

std::string LuaScript::getGlobalStringField (std::string var, const int key, const std::string &defString) {
      lua_getglobal(luaState, var.c_str());

      lua_pushnumber(luaState, key);
      lua_gettable(luaState, -2);  /* get table[key] */
      if(!lua_isstring(luaState, -1))
  	  return defString;
      std::string result = lua_tostring(luaState, -1);
      lua_pop(luaState, 2);  /* remove number and key*/
      return result;
}

int LuaScript::getField (const char *key) {
      int result;
      lua_pushstring(luaState, key);
      lua_gettable(luaState, -2);  /* get table[key] */
      result = (int)lua_tonumber(luaState, -1);
      lua_pop(luaState, 1);  /* remove number and key*/
      return result;
}

void LuaScript::setField (const char *index, int val) {
      lua_pushstring(luaState, index);
      lua_pushnumber(luaState, (double)val);
      lua_settable(luaState, -3);
    }


int LuaScript::getField (lua_State *L , const char *key) {
      int result;
      lua_pushstring(L, key);
      lua_gettable(L, -2);  /* get table[key] */
      result = (int)lua_tonumber(L, -1);
      lua_pop(L, 1);  /* remove number and key*/
      return result;
}

void LuaScript::setField (lua_State *L, const char *index, int val) {
      lua_pushstring(L, index);
      lua_pushnumber(L, (double)val);
      lua_settable(L, -3);
    }

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
// $Id$
//////////////////////////////////////////////////////////////////////
// $Log$
// Revision 1.2  2003/10/19 21:32:19  tliffrag
// Reworked the Tile class; stackable items now working
//
// Revision 1.1  2003/10/17 22:25:02  tliffrag
// Addes SorryNotPossible; added configfile; basic lua support
//
//////////////////////////////////////////////////////////////////////

#include "luascript.h"
#include <iostream>


LuaScript::LuaScript(std::string file){
	l=lua_open();
	if(lua_dofile(l, file.c_str()))	  
	  std::cout << "Error loading " << file << std::endl;
}

LuaScript::~LuaScript(){
	lua_close(l);
}

std::string LuaScript::getGlobalString(std::string var){
	lua_getglobal(l, var.c_str());
	if(!lua_isstring(l, -1)){
	  std::cout << "code " << lua_type(l, -1) <<std::endl; 
	  return "no string";
	}
	int len = lua_strlen(l, -1);
	std::string ret(lua_tostring(l,-1), len);
	lua_pop(l,1);
	return ret;
}

int LuaScript::setGlobalString(std::string var, std::string val){
	return true;
}

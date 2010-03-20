//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Lua script interface
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


#ifndef __OTSERV_PARAMETERSTYPE_H__
#define __OTSERV_PARAMETERSTYPE_H__

#include <string>
#include <map>
#include <list>
#include <vector>

extern "C"
{
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include "position.h"
#include "definitions.h"
#include "creature.h"

typedef std::map<std::string, std::string> StringParameters_t;
typedef std::map<std::string, int32_t> IntParameters_t;

struct Parameters_t {
    StringParameters_t stringParameters;
    IntParameters_t intParameters;
    void set(const std::string key, const std::string val);
    void set(const std::string key, const int32_t val);
    bool get(const std::string key, std::string &ret) const;
    bool get(const std::string key, int32_t &ret) const;
    bool isEmpty() const;
    void clear() { stringParameters.clear();
                    intParameters.clear(); };
    void pushValueToLua(lua_State *L, const std::string key) const;
    bool readXMLParameters(const xmlNodePtr node);
    };

struct ParametersVector_t {
    std::map<std::string, Parameters_t*> parametersVector;
    void set(const std::string owner, const std::string key, const std::string val);
    void set(const std::string owner, const std::string key, const int32_t val);
    bool pushLuaExtraParametersTable(const std::string owner, LuaScriptInterface *env) const;
    bool get(const std::string owner, const std::string key, std::string &ret) const;
    bool get(const std::string owner, const std::string key, int32_t &ret) const;
    bool readXMLParameters(const xmlNodePtr node, const std::string ownerName);
    void reset();

    };
#endif

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


#ifndef __LUASCRIPT_H__
#define __LUASCRIPT_H__

#include <string>
#include <map>

extern "C"
{
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

class Thing;
class Creature;
class Player;
class Item;

class LuaScript
{
public:
	LuaScript();
	~LuaScript();
	int openFile(const char* file);
	std::string getGlobalString(std::string var, const std::string& defString = "");
	int getGlobalNumber(std::string var, const int defNum = 0);
	std::string getGlobalStringField (std::string var, const int key, const std::string& defString = "");
	// set a var to a val
	int setGlobalString(std::string var, std::string val);
	int setGlobalNumber(std::string var, int val);
	
protected:
	int getField (const char *key);
	void setField (const char *index, int value);
	//static version
	static int getField (lua_State *L , const char *key);
	static void setField (lua_State *L, const char *index, int val);	
	
protected:
	std::string luaFile;   // the file we represent
	lua_State*  luaState;  // our lua state
};

class LuaScriptInterface;

class ScriptEnviroment
{
public:
	ScriptEnviroment();
	~ScriptEnviroment();
	
	void resetEnv();
	
	void setScriptId(long scriptId, LuaScriptInterface* scriptInterface);
	void setEventDesc(const std::string& desc);
	
	void getEventInfo(long& scriptId, std::string& desc, LuaScriptInterface* scriptInterface);
	
	static void addUniqueThing(Thing* thing);
	long addThing(Thing* thing);
	
	Thing* getThingByUID(long uid);
	Item* getItemByUID(long uid);
	Creature* getCreatureByUID(long uid);
	Player* getPlayerByUID(long uid);
	
private:
	typedef std::map<long, Thing*> ThingMap;
	//typedef std::map<long, Matrix*> AreaMap;
	//script file id
	long m_scriptId;
	LuaScriptInterface* m_interface;
	//script event desc
	std::string m_eventdesc;
	
	//unique id map
	static ThingMap m_globalMap;
	//item/creature map
	ThingMap m_localMap;
	long m_lastUID;
	//area map
	//static AreaMap areaMap;
};

class LuaScriptInterface
{
public:
	LuaScriptInterface();
	virtual ~LuaScriptInterface();
	
	bool reInitState();
	
	long loadFile(const std::string& file, const std::string& eventName);
	const std::string& getFileById(long scriptId);
	
	static ScriptEnviroment* getScriptEnv();
	
	static void reportError(const std::string& error_desc);
	
	virtual std::string getInterfaceName();
	long getLastLuaError();
	void dumpLuaStack();
	
	
	bool pushFunction(long functionId);
	//push/pop common structures
	
protected:
	bool initState();
	bool closeState();
	
	void registerFunctions();
	
	//lua functions
	
	//
	
	lua_State*  m_luaState;
	long m_lastLuaError;
private:
	
	static ScriptEnviroment m_scriptEnv;
	
	long m_runningEventId;
	
	//script file cache
	typedef std::map<long , std::string> ScriptsCache;
	ScriptsCache m_cacheFiles;
};

#endif  // #ifndef __LUASCRIPT_H__

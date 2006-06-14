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
#include "position.h"

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
class Game;
	
class ScriptEnviroment
{
public:
	ScriptEnviroment();
	~ScriptEnviroment();
	
	void resetEnv();
	
	void setScriptId(long scriptId, LuaScriptInterface* scriptInterface);
	void setEventDesc(const std::string& desc);
	
	void getEventInfo(long& scriptId, std::string& desc, LuaScriptInterface*& scriptInterface);
	
	static void addUniqueThing(Thing* thing);
	long addThing(Thing* thing);
	
	void setRealPos(const Position& realPos);
	Position getRealPos();
	
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
	Position m_realPos;
	//area map
	//static AreaMap areaMap;
};

class Position;

enum PlayerInfo_t{
	PlayerInfoFood,
	PlayerInfoAccess,
	PlayerInfoLevel,
	PlayerInfoMagLevel,
	PlayerInfoMana,
	PlayerInfoHealth,
	PlayerInfoName,
	PlayerInfoPosition,
	PlayerInfoVocation,
	PlayerInfoMasterPos,
	PlayerInfoGuildId,
};

class LuaScriptInterface
{
public:
	LuaScriptInterface::LuaScriptInterface(std::string interfaceName);
	virtual ~LuaScriptInterface();
	
	bool reInitState();
	
	long loadFile(const std::string& file);
	const std::string& getFileById(long scriptId);
	
	long getEvent(const std::string& eventName);
	
	static ScriptEnviroment* getScriptEnv();
	
	static void reportError(const char* function, const std::string& error_desc);
	
	std::string getInterfaceName();
	long getLastLuaError();
	void dumpLuaStack();
	
	lua_State* getLuaState();
	
	bool pushFunction(long functionId);
	//push/pop common structures
	static void pushThing(lua_State *L, Thing* thing, long thingid);
	static void pushPosition(lua_State *L, const Position& position, long stackpos);
	
	static void popPosition(lua_State *L, Position& position, long& stackpos);
	static long popNumber(lua_State *L);
	static const char* popString(lua_State *L);
	
	static long getField(lua_State *L, const char *key);
	static void setField(lua_State *L, const char *index, long val);
	
protected:
	bool initState();
	bool closeState();
	
	void registerFunctions();
	
	//lua functions
	static int luaDoRemoveItem(lua_State *L);
	static int luaDoFeedPlayer(lua_State *L);	
	static int luaDoSendCancel(lua_State *L);
	static int luaDoTeleportThing(lua_State *L);
	static int luaDoTransformItem(lua_State *L);
	static int luaDoPlayerSay(lua_State *L);
	static int luaDoSendMagicEffect(lua_State *L);
	static int luaDoChangeTypeItem(lua_State *L);
	static int luaDoSendAnimatedText(lua_State *L);
	
	static int luaDoPlayerAddSkillTry(lua_State *L);
	static int luaDoPlayerAddHealth(lua_State *L);
	static int luaDoPlayerAddMana(lua_State *L);
	static int luaDoPlayerAddItem(lua_State *L);
	static int luaDoPlayerSendTextMessage(lua_State *L);
	static int luaDoShowTextWindow(lua_State *L);
	static int luaDoDecayItem(lua_State *L);
	static int luaDoCreateItem(lua_State *L);
	static int luaDoSummonCreature(lua_State *L);
	static int luaDoPlayerRemoveMoney(lua_State *L);
	static int luaDoPlayerSetMasterPos(lua_State *L);
	static int luaDoPlayerSetVocation(lua_State *L);
	static int luaDoPlayerRemoveItem(lua_State *L);
	
	//get item info
	static int luaGetItemRWInfo(lua_State *L);
	static int luaGetThingfromPos(lua_State *L);
	//set item
	static int luaDoSetItemActionId(lua_State *L);
	static int luaDoSetItemText(lua_State *L);
	static int luaDoSetItemSpecialDescription(lua_State *L);
	
	//get tile info
	static int luaGetTilePzInfo(lua_State *L);
	static int luaGetTileHouseInfo(lua_State *L);
	
	//get player info functions
	static int luaGetPlayerFood(lua_State *L);
	static int luaGetPlayerAccess(lua_State *L);
	static int luaGetPlayerLevel(lua_State *L);
	static int luaGetPlayerMagLevel(lua_State *L);
	static int luaGetPlayerMana(lua_State *L);
	static int luaGetPlayerHealth(lua_State *L);
	static int luaGetPlayerName(lua_State *L);
	static int luaGetPlayerPosition(lua_State *L);	
	static int luaGetPlayerSkill(lua_State *L);
	static int luaGetPlayerVocation(lua_State *L);
	static int luaGetPlayerMasterPos(lua_State *L);
	static int luaGetPlayerGuildId(lua_State *L);
	
	static int luaGetPlayerStorageValue(lua_State *L);
	static int luaSetPlayerStorageValue(lua_State *L);
	//
	
	static int internalGetPlayerInfo(lua_State *L, PlayerInfo_t info);
	
	lua_State* m_luaState;
	long m_lastLuaError;
private:
	
	static ScriptEnviroment m_scriptEnv;
	
	long m_runningEventId;
	std::string m_loadingFile;
	
	//script file cache
	typedef std::map<long , std::string> ScriptsCache;
	ScriptsCache m_cacheFiles;
	
	
	std::string m_interfaceName;
};

#endif  // #ifndef __LUASCRIPT_H__

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


#ifndef __LUASCRIPT_H__
#define __LUASCRIPT_H__

#include <string>
#include <map>
#include <list>

extern "C"
{
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include "position.h"
#include "definitions.h"

class Thing;
class Creature;
class Player;
class Item;
class Container;
class AreaCombat;
class Combat;
class Condition;

enum LuaVariantType_t{	
	VARIANT_NONE = 0,
	VARIANT_NUMBER,
	VARIANT_POSITION,
	VARIANT_TARGETPOSITION,
	VARIANT_STRING,
};

struct LuaVariant{
	LuaVariant()
	{
		type = VARIANT_NONE;
		text = "";
		pos.x = 0;
		pos.y = 0;
		pos.z = 0;
		number = 0;
	}

	LuaVariantType_t type;
	std::string text;
	Position pos;
	uint32_t number;
};

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
	bool setCallbackId(long callbackId);
	void setEventDesc(const std::string& desc);
	
	std::string getEventDesc();
	long getScriptId();
	LuaScriptInterface* getScriptInterface();

	void getEventInfo(long& scriptId, std::string& desc, LuaScriptInterface*& scriptInterface, long& callbackId);

	static void addUniqueThing(Thing* thing);
	long addThing(Thing* thing);
	uint32_t addVariant(const LuaVariant* variant);
	const LuaVariant* getVariant(uint32_t index);
	
	void addGlobalStorageValue(const unsigned long key, const long value);
	bool getGlobalStorageValue(const unsigned long key, long &value) const;

	void setRealPos(const Position& realPos);
	Position getRealPos();

	Thing* getThingByUID(long uid);
	Item* getItemByUID(long uid);
	Container* getContainerByUID(long uid);
	Creature* getCreatureByUID(long uid);
	Player* getPlayerByUID(long uid);

	uint32_t addCombatArea(AreaCombat* area);
	AreaCombat* getCombatArea(uint32_t areaId) const;

	uint32_t addCombatObject(Combat* combat);
	const Combat* getCombatObject(uint32_t combatId) const;
	Combat* getCombatObject(uint32_t combatId);

	uint32_t addConditionObject(Condition* condition);
	const Condition* getConditionObject(uint32_t conditionId) const;
	Condition* getConditionObject(uint32_t conditionId);

private:
	typedef std::map<long, Thing*> ThingMap;
	typedef std::vector<const LuaVariant*> VariantVector;
	typedef std::map<unsigned long,long> StorageMap;
	typedef std::map<uint32_t, AreaCombat*> AreaMap;
	typedef std::map<uint32_t, Combat*> CombatMap;
	typedef std::map<uint32_t, Condition*> ConditionMap;

	//script file id
	long m_scriptId;
	long m_callbackId;
	LuaScriptInterface* m_interface;
	//script event desc
	std::string m_eventdesc;

	static StorageMap m_globalStorageMap;
	//unique id map
	static ThingMap m_globalMap;
	
	Position m_realPos;
	
	//item/creature map
	long m_lastUID;
	ThingMap m_localMap;
	
	//variant vector
	VariantVector m_variants;
	
	//area map
	uint32_t m_lastAreaId;
	static AreaMap m_areaMap;

	//combat map
	uint32_t m_lastCombatId;
	static CombatMap m_combatMap;

	//condition map
	uint32_t m_lastConditionId;
	static ConditionMap m_conditionMap;
	
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
	PlayerInfoSoul,
	PlayerInfoFreeCap,
	PlayerInfoGuildId,
	PlayerInfoGuildName,
	PlayerInfoGuildRank,
	PlayerInfoGuildNick,
};

#define reportErrorFunc(a)  reportError(__FUNCTION__, a)

enum ErrorCode_t{
	LUA_ERROR_PLAYER_NOT_FOUND,
	LUA_ERROR_CREATURE_NOT_FOUND,
	LUA_ERROR_ITEM_NOT_FOUND,
	LUA_ERROR_THING_NOT_FOUND,
	LUA_ERROR_TILE_NOT_FOUND,
	LUA_ERROR_HOUSE_NOT_FOUND,
	LUA_ERROR_COMBAT_NOT_FOUND,
	LUA_ERROR_CONDITION_NOT_FOUND,
	LUA_ERROR_AREA_NOT_FOUND,
	LUA_ERROR_CONTAINER_NOT_FOUND,
	LUA_ERROR_VARIANT_NOT_FOUND,
	LUA_ERROR_VARIANT_UNKNOWN
};


class LuaScriptInterface
{
public:
	LuaScriptInterface(std::string interfaceName);
	virtual ~LuaScriptInterface();

	virtual bool initState();
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

	bool callFunction(long nParams, long &result);
	bool callFunction(long nParams);

	//push/pop common structures
	static void pushThing(lua_State *L, Thing* thing, long thingid);
	static void pushPosition(lua_State *L, const Position& position, long stackpos);

	static void popPosition(lua_State *L, Position& position, long& stackpos);
	static long popNumber(lua_State *L);
	static double popFloatNumber(lua_State *L);
	static const char* popString(lua_State *L);

	static long getField(lua_State *L, const char *key);
	static void setField(lua_State *L, const char *index, long val);

protected:
	virtual bool closeState();

	virtual void registerFunctions();

	static std::string getErrorDesc(ErrorCode_t code);
	static bool getArea(lua_State *L, std::list<uint32_t>& list, uint32_t& rows);

	//lua functions
	static int luaDoRemoveItem(lua_State *L);
	static int luaDoFeedPlayer(lua_State *L);
	static int luaDoSendCancel(lua_State *L);
	static int luaDoTeleportThing(lua_State *L);
	static int luaDoTransformItem(lua_State *L);
	static int luaDoSendMagicEffect(lua_State *L);
	static int luaDoChangeTypeItem(lua_State *L);
	static int luaDoSendAnimatedText(lua_State *L);
	static int luaDoShowTextWindow(lua_State *L);
	static int luaDoDecayItem(lua_State *L);
	static int luaDoCreateItem(lua_State *L);
	static int luaDoSummonCreature(lua_State *L);
	static int luaDoMoveCreature(lua_State *L);

	static int luaDoPlayerSay(lua_State *L);
	static int luaDoPlayerAddSkillTry(lua_State *L);
	static int luaDoPlayerAddHealth(lua_State *L);
	static int luaDoPlayerAddMana(lua_State *L);
	static int luaDoPlayerSoul(lua_State *L);
	static int luaDoPlayerAddItem(lua_State *L);
	static int luaDoPlayerSendTextMessage(lua_State *L);
	static int luaDoPlayerRemoveMoney(lua_State *L);
	static int luaDoPlayerSetMasterPos(lua_State *L);
	static int luaDoPlayerSetVocation(lua_State *L);
	static int luaDoPlayerRemoveItem(lua_State *L);
	static int luaDoPlayerAddSoul(lua_State *L);
	static int luaDoPlayerAddExp(lua_State *L);
	//static int luaDoPlayerSetGuildId(lua_State *L);
	static int luaDoPlayerSetGuildRank(lua_State *L);
	static int luaDoPlayerSetGuildNick(lua_State *L);

	//get item info
	static int luaGetItemRWInfo(lua_State *L);
	static int luaGetThingfromPos(lua_State *L);
	static int luaGetThing(lua_State *L);
	static int luaGetThingPos(lua_State *L);
	//set item
	static int luaDoSetItemActionId(lua_State *L);
	static int luaDoSetItemText(lua_State *L);
	static int luaDoSetItemSpecialDescription(lua_State *L);

	//get tile info
	static int luaGetTilePzInfo(lua_State *L);
	static int luaGetTileHouseInfo(lua_State *L);
	//houses
	static int luaGetHouseOwner(lua_State *L);
	static int luaSetHouseOwner(lua_State *L);

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
	static int luaGetPlayerItemCount(lua_State *L);
	static int luaGetPlayerSoul(lua_State *L);
	static int luaGetPlayerFreeCap(lua_State *L);
	static int luaGetPlayerLight(lua_State *L);
	static int luaGetPlayerSlotItem(lua_State *L);
	static int luaGetPlayerDepotItems(lua_State *L);
	static int luaGetPlayerGuildId(lua_State *L);
	static int luaGetPlayerGuildName(lua_State *L);
	static int luaGetPlayerGuildRank(lua_State *L);
	static int luaGetPlayerGuildNick(lua_State *L);

	static int luaGetPlayerStorageValue(lua_State *L);
	static int luaSetPlayerStorageValue(lua_State *L);
	
	static int luaGetGlobalStorageValue(lua_State *L);
	static int luaSetGlobalStorageValue(lua_State *L);

	static int luaGetWorldType(lua_State *L);
	static int luaGetWorldTime(lua_State *L);
	static int luaGetWorldLight(lua_State *L);
	static int luaGetWorldCreatures(lua_State *L);
	static int luaGetWorldUpTime(lua_State *L);
	static int luaGetGuildId(lua_State *L);

	//type validation
	static int luaIsPlayer(lua_State *L);
	static int luaIsContainer(lua_State *L);
	
	static int luaGetPlayerByName(lua_State *L);
	
	//container
	static int luaGetContainerSize(lua_State *L);
	static int luaGetContainerCap(lua_State *L);
	static int luaGetContainerItem(lua_State *L);
	static int luaDoAddContainerItem(lua_State *L);

	//
	static int luaCreateCombatObject(lua_State *L);
	static int luaCreateCombatArea(lua_State *L);
	static int luaSetCombatArea(lua_State *L);
	static int luaSetCombatCondition(lua_State *L);
	static int luaSetCombatParam(lua_State *L);
	static int luaCreateConditionObject(lua_State *L);
	static int luaSetConditionParam(lua_State *L);
	static int luaAddDamageCondition(lua_State *L);
	static int luaAddOutfitCondition(lua_State *L);

	static int luaSetCombatCallBack(lua_State *L);
	static int luaSetCombatFormula(lua_State *L);
	static int luaDoCombat(lua_State *L);

	static int luaDoAreaCombatHealth(lua_State *L);
	static int luaDoTargetCombatHealth(lua_State *L);

	//
	static int luaDoAreaCombatMana(lua_State *L);
	static int luaDoTargetCombatMana(lua_State *L);

	static int luaDoAreaCombatCondition(lua_State *L);
	static int luaDoTargetCombatCondition(lua_State *L);

	static int luaDebugPrint(lua_State *L);
	static int luaIsInArray(lua_State *L);
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

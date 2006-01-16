//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// 
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


#ifndef __ACTIONS_H__
#define __ACTIONS_H__

#include "position.h"

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include <map>

#include "luascript.h"

extern "C"
{
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

class Player;
class Npc;
class Monster;
class Thing;
class Item;
class Container;
class Depot;
class Game;
class ActionScript;
class Action;

enum tCanUseRet{
	CAN_USE,
	TOO_FAR,
	CAN_NOT_THROW,
};

class Actions
{
public:
	Actions(){};
	Actions(Game* igame);
	bool loadFromXml(const std::string &datadir);
	virtual ~Actions();
	void clear();
	
	bool UseItem(Player* player, const Position &pos,const unsigned char stack, 
		const unsigned short itemid, const unsigned char index);
	bool UseItemEx(Player* player, const Position &from_pos,
		const unsigned char from_stack,const Position &to_pos,
		const unsigned char to_stack,const unsigned short itemid);
	
	bool openContainer(Player *player,Container *container, const unsigned char index);
	
	Game* game;
	bool loaded;

	bool isLoaded(){return loaded;}	
	bool reload();
  
protected:
	std::string datadir;
	typedef std::map<unsigned short, Action*> ActionUseMap;
	ActionUseMap useItemMap;
	ActionUseMap uniqueItemMap;
	ActionUseMap actionItemMap;
	int canUse(const Player *player,const Position &pos) const;
	int canUseFar(const Player *player,const Position &to_pos, const bool blockWalls) const;
	Action *getAction(const Item *item);
	Action *loadAction(xmlNodePtr xmlaction);
};

enum tThingType{
	thingTypeItem,
	thingTypePlayer,
	thingTypeMonster,
	thingTypeNpc,
	thingTypeUnknown,
};

enum ePlayerInfo{
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

class Action
{
public:
	Action(Game* igame,const std::string &datadir, const std::string &scriptname);
	virtual ~Action();
	bool isLoaded() const {return loaded;}
	bool allowFarUse() const {return allowfaruse;};
	bool blockWalls() const {return blockwalls;};
	void setAllowFarUse(bool v){allowfaruse = v;};
	void setBlockWalls(bool v){blockwalls = v;};
	bool executeUse(Player *player,Item* item, PositionEx &posFrom, PositionEx &posTo);
	
protected:
	ActionScript *script;
	bool loaded;
	bool allowfaruse;
	bool blockwalls;
};

class ActionScript : protected LuaScript{
public:
	ActionScript(Game* igame,const std::string &datadir, const std::string &scriptname);
	virtual ~ActionScript();
	bool isLoaded()const {return loaded;}
	
	lua_State* getLuaState(){return luaState;}
	
	void ClearMap();
	static void AddThingToMapUnique(Thing *thing);
	unsigned int AddThingToMap(Thing *thing);
	Thing* GetThingByUID(int uid);
	Item* GetItemByUID(int uid);
	Creature* GetCreatureByUID(int uid);
	Player* GetPlayerByUID(int uid);
	
	//lua functions
	static int luaActionDoRemoveItem(lua_State *L);
	static int luaActionDoFeedPlayer(lua_State *L);	
	static int luaActionDoSendCancel(lua_State *L);
	static int luaActionDoTeleportThing(lua_State *L);
	static int luaActionDoTransformItem(lua_State *L);
	static int luaActionDoPlayerSay(lua_State *L);
	static int luaActionDoSendMagicEffect(lua_State *L);
	static int luaActionDoChangeTypeItem(lua_State *L);
	static int luaActionDoSendAnimatedText(lua_State *L);
	
	static int luaActionDoPlayerAddSkillTry(lua_State *L);
	static int luaActionDoPlayerAddHealth(lua_State *L);
	static int luaActionDoPlayerAddMana(lua_State *L);
	static int luaActionDoPlayerAddItem(lua_State *L);
	static int luaActionDoPlayerSendTextMessage(lua_State *L);
	static int luaActionDoShowTextWindow(lua_State *L);
	static int luaActionDoDecayItem(lua_State *L);
	static int luaActionDoCreateItem(lua_State *L);
	static int luaActionDoSummonCreature(lua_State *L);
	static int luaActionDoPlayerRemoveMoney(lua_State *L);
	static int luaActionDoPlayerSetMasterPos(lua_State *L);
	static int luaActionDoPlayerSetVocation(lua_State *L);
	static int luaActionDoPlayerRemoveItem(lua_State *L);
	
	//get item info
	static int luaActionGetItemRWInfo(lua_State *L);
	static int luaActionGetThingfromPos(lua_State *L);
	//set item
	static int luaActionDoSetItemActionId(lua_State *L);
	static int luaActionDoSetItemText(lua_State *L);
	static int luaActionDoSetItemSpecialDescription(lua_State *L);
	
	//get tile info
	static int luaActionGetTilePzInfo(lua_State *L);
	
	//get player info functions
	static int luaActionGetPlayerFood(lua_State *L);
	static int luaActionGetPlayerAccess(lua_State *L);
	static int luaActionGetPlayerLevel(lua_State *L);
	static int luaActionGetPlayerMagLevel(lua_State *L);
	static int luaActionGetPlayerMana(lua_State *L);
	static int luaActionGetPlayerHealth(lua_State *L);
	static int luaActionGetPlayerName(lua_State *L);
	static int luaActionGetPlayerPosition(lua_State *L);	
	static int luaActionGetPlayerSkill(lua_State *L);
	static int luaActionGetPlayerVocation(lua_State *L);
	static int luaActionGetPlayerMasterPos(lua_State *L);
	static int luaActionGetPlayerGuildId(lua_State *L);
	
	static int luaActionGetPlayerStorageValue(lua_State *L);
	static int luaActionSetPlayerStorageValue(lua_State *L);
	
protected:			
	
	Game *game;
	Player *_player;
	unsigned int lastuid;
	
	friend class Action;
	
	std::map<unsigned int,Thing*> ThingMap;
	static std::map<unsigned int,Thing*> uniqueIdMap;
	
	//lua related functions
	int registerFunctions();
	bool loaded;
	//lua interface helpers
	static ActionScript* getActionScript(lua_State *L);
	static void internalAddPositionEx(lua_State *L, const PositionEx& pos);
	static void internalGetPositionEx(lua_State *L, PositionEx& pos);
	static unsigned long internalGetNumber(lua_State *L);
	static const char* internalGetString(lua_State *L);
	static void internalAddThing(lua_State *L, const Thing *thing, const unsigned int thingid);
	
	static const Position& internalGetRealPosition(ActionScript *action, Player *player, const Position &pos);
	static int internalGetPlayerInfo(lua_State *L, ePlayerInfo info);
	
};

#endif

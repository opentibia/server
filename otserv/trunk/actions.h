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


#ifndef __actions_h_
#define __actions_h_

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
class Item;
class Game;
class ActionScript;
class Action;

class Actions
{
public:
	Actions(){};
	Actions(Game* igame);
	bool loadFromXml();
	virtual ~Actions();
	
	void UseItem(Player* player, const Position &pos,const unsigned char stack, 
		const unsigned short itemid, const unsigned char index);
	void UseItemEx(Player* player, const Position &from_pos,
		const unsigned char from_stack,const Position &to_pos,
		const unsigned char to_stack,const unsigned short itemid);
	
	bool openContainer(Player *player,Container *container, const unsigned char index);
	
	Game* game;
	bool loaded;

	bool isLoaded(){return loaded;}	
	//bool reload();
  
protected:
	typedef std::map<unsigned short, Action*> ActionUseMap;
	ActionUseMap useItemMap;
	ActionUseMap uniqueItemMap;
	int canUse(const Player *player,const Position &pos) const;
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
};

struct KnownThing{
	Thing *thing;
	tThingType type;
	PositionEx pos;	
};

class Action
{
public:
	Action(Game* igame,std::string scriptname);
	virtual ~Action();
	bool isLoaded() const {return loaded;}
	bool allowFarUse() const {return allowfaruse;};
	void setAllowFarUse(bool v){allowfaruse = v;};
	ActionScript *getScript(){return script;};
	
	void ClearMap();
	static void AddThingToMapUnique(Thing *thing);
	unsigned int AddThingToMap(Thing *thing,PositionEx &pos);
	const KnownThing* GetThingByUID(int uid);
	const KnownThing* GetItemByUID(int uid);
	const KnownThing* GetCreatureByUID(int uid);
	const KnownThing* GetPlayerByUID(int uid);
	
	Game *game;
	Player *_player;
	
protected:
	std::map<unsigned int,KnownThing*> ThingMap;
	static std::map<unsigned int,KnownThing*> uniqueIdMap;
	unsigned int lastuid;
	ActionScript *script;
	bool loaded;
	bool allowfaruse;
};

class ActionScript : protected LuaScript{
public:
	ActionScript(Action* iaction,std::string scriptname);
	virtual ~ActionScript(){}
	bool executeUse(Player *player,Item* item, PositionEx &posFrom, PositionEx &posTo);
	bool isLoaded()const {return loaded;}
	static Action* getAction(lua_State *L);
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
	
	//get item info
	static int luaActionGetItemRWInfo(lua_State *L);
	static int luaActionGetThingfromPos(lua_State *L);
	
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
	
protected:			
	
	Action *_action;	
	int registerFunctions();
	bool loaded;	
	
	static void internalAddPositionEx(lua_State *L, const PositionEx& pos);
	static void internalGetPositionEx(lua_State *L, PositionEx& pos);
	static unsigned long internalGetNumber(lua_State *L);
	static const char* internalGetString(lua_State *L);
	static void internalAddThing(lua_State *L, const Thing *thing, const unsigned int thingid);
	
	static Position internalGetRealPosition(Player *player, const Position &pos);
	static int internalGetPlayerInfo(lua_State *L, ePlayerInfo info);
	
};

#endif // __actions_h_

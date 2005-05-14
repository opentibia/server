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
		unsigned short itemid);
	void UseItemEx(Player* player, const Position &from_pos,
		const unsigned char from_stack,const Position &to_pos,
		const unsigned char to_stack,const unsigned short itemid);
	
	Game* game;
	bool loaded;

	bool isLoaded(){return loaded;}	
  
protected:
	//use map
	typedef std::map<unsigned short, Action*> ActionUseMap;
	ActionUseMap useItemMap;
};

enum tThingType{
	thingTypeItem,
	thingTypePlayer,
	thingTypeMonster,
	thingTypeNpc,
	thingTypeUnknown,
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
	bool isLoaded(){return loaded;}
	ActionScript *getScript(){return script;};
	
	void ClearMap();
	int AddThingToMap(Thing *thing,PositionEx &pos);
	const KnownThing* GetThingByUID(int uid);
	const KnownThing* GetItemByUID(int uid);
	const KnownThing* GetCreatureByUID(int uid);
	const KnownThing* GetPlayerByUID(int uid);
	
	Game *game;
	Player *_player;
	
protected:
	std::map<unsigned int,KnownThing*> ThingMap;
	unsigned int lastuid;
	ActionScript *script;
	bool loaded;
};

class ActionScript : protected LuaScript{
public:
	ActionScript(Action* iaction,std::string scriptname);
	virtual ~ActionScript(){}
	bool execute(Player *player,Item* item, PositionEx &posFrom, PositionEx &posTo);
	bool isLoaded(){return loaded;}
	static Action* getAction(lua_State *L);
		
	
	//lua functions
	static int luaActionDoRemoveItem(lua_State *L);
	static int luaActionDoFeedPlayer(lua_State *L);
	static int luaActionGetPlayerFood(lua_State *L);
	static int luaActionDoSendCancel(lua_State *L);
	static int luaActionDoTeleportThing(lua_State *L);
	static int luaActionDoTransformItem(lua_State *L);
protected:		
	Action *_action;	
	int registerFunctions();
	bool loaded; 
private:
	void internalAddPositionEx(lua_State *L, PositionEx& pos);
	static void internalGetPositionEx(lua_State *L, PositionEx& pos);
	void internalAddItem(lua_State *L, Item *item, unsigned int itemid);
};

#endif // __actions_h_

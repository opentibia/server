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


#ifndef __npc_h_
#define __npc_h_


#include "creature.h"
#include "game.h"
#include "luascript.h"

extern "C"
{
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}


//////////////////////////////////////////////////////////////////////
// Defines an NPC...
class Npc;
class NpcScript : protected LuaScript{
public:
	NpcScript(std::string name, Npc* npc);
	virtual ~NpcScript(){}
//	virtual void onThingMove(const Player *player, const Thing *thing, const Position *oldPos,
//	unsigned char oldstackpos, unsigned char oldcount, unsigned char count);
	virtual void onCreatureAppear(unsigned long cid);
	virtual void onCreatureDisappear(int cid);
//	virtual void onCreatureTurn(const Creature *creature, unsigned char stackpos);
	virtual void onCreatureSay(int cid, unsigned char type, const std::string &text);
	virtual void onThink();
//	virtual void onCreatureChangeOutfit(const Creature* creature);
	static Npc* getNpc(lua_State *L);
	static int luaActionSay(lua_State *L);
	static int luaActionMove(lua_State *L);
	static int luaActionMoveTo(lua_State *L);
	static int luaCreatureGetName(lua_State *L);
	static int luaCreatureGetName2(lua_State *L);
	static int luaActionAttackCreature(lua_State *L);
	static int luaCreatureGetPos(lua_State *L);
	static int luaSelfGetPos(lua_State *L);

  bool isLoaded(){return loaded;}
protected:
	int registerFunctions();
	Npc* npc;
	bool loaded;
};

class Npc : protected AutoID<Npc>, public AutoList<Npc>, public Creature
{
public:
  Npc(const char *name, Game* game);
  virtual ~Npc();

	static const unsigned long min_id = 10000U;
	static const unsigned long max_id = 65536U;

  void speak(const std::string &text){};
  std::string getName(){return name;};
  fight_t getFightType(){return fighttype;};

  int mana, manamax;

	//damage per hit
  int damage;

  fight_t fighttype;

	Game* game;

  void doSay(std::string msg);
  void doMove(int dir);
  void doMoveTo(Position pos);
  void doAttack(int id);
  bool isLoaded(){return loaded;}

protected:
  virtual void onThingMove(const Player *player, const Thing *thing, const Position *oldPos,
		unsigned char oldstackpos, unsigned char oldcount, unsigned char count);
  virtual void onCreatureAppear(const Creature *creature);
  virtual void onCreatureDisappear(const Creature *creature, unsigned char stackPos, bool tele);
  virtual void onCreatureTurn(const Creature *creature, unsigned char stackpos);
  virtual void onCreatureSay(const Creature *creature, unsigned char type, const std::string &text);
  virtual void onCreatureChangeOutfit(const Creature* creature);
  virtual int onThink(int& newThinkTicks);
  virtual void setAttackedCreature(unsigned long id);
  virtual std::string getDescription(bool self = false) const;

	virtual bool isAttackable() const { return false; };
  virtual bool isPushable() const { return true; };

	std::string name;
  std::string scriptname;
  NpcScript* script;
  std::list<Position> route;
	bool loaded;
};


#endif // __npc_h_

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

#ifndef __OTSERV_NPC_H__
#define __OTSERV_NPC_H__

#include "creature.h"
#include "luascript.h"
#include "templates.h"

//////////////////////////////////////////////////////////////////////
// Defines an NPC...

/*
class Npc;
class NpcScript : protected LuaScript{
public:
	NpcScript(std::string name, Npc* npc);
	virtual ~NpcScript(){}
	virtual void onCreatureAppear(unsigned long cid);
	virtual void onCreatureDisappear(int cid);

	virtual void onCreatureSay(int cid, SpeakClasses, const std::string& text);
	virtual void onThink();
	static Npc* getNpc(lua_State *L);
	static int luaActionSay(lua_State *L);
	static int luaActionMove(lua_State *L);
	static int luaActionMoveTo(lua_State *L);
	static int luaCreatureGetName(lua_State *L);
	static int luaCreatureGetName2(lua_State *L);
	static int luaCreatureGetPos(lua_State *L);
	static int luaSelfGetPos(lua_State *L);
	
	bool isLoaded(){return loaded;}

protected:
	int registerFunctions();
	Npc* npc;
	bool loaded;
};
*/

class Npc;

class NpcScriptInterface : public LuaScriptInterface
{
public:
	NpcScriptInterface();
	virtual ~NpcScriptInterface();

	static void setNpc(Npc* npc);

	bool loadNpcLib(std::string file);

protected:
	
	static Npc* getNpc();
	
	virtual void registerFunctions();
	
	static int luaActionSay(lua_State *L);
	static int luaActionMove(lua_State *L);
	static int luaActionMoveTo(lua_State *L);
	static int luaCreatureGetName(lua_State *L);
	static int luaCreatureGetName2(lua_State *L);
	static int luaCreatureGetPos(lua_State *L);
	static int luaSelfGetPos(lua_State *L);
	static int luagetDistanceTo(lua_State *L);
	
private:
	virtual bool initState();
	virtual bool closeState();
	
	bool m_libLoaded;
	static Npc* m_curNpc;
};

class NpcScript{
public:
	NpcScript(std::string file, Npc* npc);
	~NpcScript();
	
	void onCreatureAppear(const Creature* creature);
	void onCreatureDisappear(const Creature* creature);

	void onCreatureSay(const Creature* creature, SpeakClasses, const std::string& text);
	void onThink();
	
	bool isLoaded();
	
private:
	
	bool m_loaded;
	Npc* m_npc;
	NpcScriptInterface* m_scriptInterface;
	
	long m_onCreatureAppear;
	long m_onCreatureDisappear;
	long m_onCreatureSay;
	long m_onThink;
	
};

class Npc : public Creature
{
public:
	Npc(const std::string& name);
	virtual ~Npc();

	virtual Npc* getNpc() {return this;};
	virtual const Npc* getNpc() const {return this;};

	virtual bool isPushable() const { return true;};
	
	virtual unsigned long idRange(){ return 0x30000000;}
	static AutoList<Npc> listNpc;
	void removeList() {listNpc.removeList(getID());}
	void addList() {listNpc.addList(this);}
	
	bool canSee(const Position& pos) const;
	
	void speak(const std::string& text){};
	virtual const std::string& getName() const {return name;};
	virtual const std::string& getNameDescription() const {return name;};
	
	void doSay(std::string msg);
	void doMove(Direction dir);
	void doMoveTo(Position pos);
	bool isLoaded(){return loaded;}
	
	NpcScriptInterface* getScriptInterface();
	
protected:
	virtual void onAddTileItem(const Position& pos, const Item* item);
	virtual void onUpdateTileItem(const Position& pos, uint32_t stackpos, const Item* oldItem, const Item* newItem);
	virtual void onRemoveTileItem(const Position& pos, uint32_t stackpos, const Item* item);
	virtual void onUpdateTile(const Position& pos);

	virtual void onCreatureAppear(const Creature* creature, bool isLogin);
	virtual void onCreatureDisappear(const Creature* creature, uint32_t stackpos, bool isLogout);
	virtual void onCreatureMove(const Creature* creature, const Position& oldPos, uint32_t oldStackPos, bool teleport);

	virtual void onCreatureTurn(const Creature* creature, uint32_t stackpos);
	virtual void onCreatureSay(const Creature* creature, SpeakClasses type, const std::string& text);
	virtual void onCreatureChangeOutfit(const Creature* creature);
	virtual void onThink(uint32_t interval);
	virtual std::string getDescription(int32_t lookDistance) const;
	
	virtual bool isAttackable() const { return false; };
	
	std::string name;
	
	static NpcScriptInterface m_scriptInterface;
	NpcScript* m_npcScript;
	std::list<Position> route;
	bool loaded;
};

#endif

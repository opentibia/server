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


#ifndef __spells_h_
#define __spells_h_


#include "game.h"
#include "player.h"
#include "luascript.h"

extern "C"
{
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}


typedef std::vector< std::vector<unsigned char> > AreaVector;

//////////////////////////////////////////////////////////////////////
// Defines a Spell...
class Spell;
class SpellScript;
class MagicSpell;

class Spells
{
public:
  Spells(Game* game);
  bool loadFromXml(const std::string&);
  virtual ~Spells();

	Game* game;

  bool isLoaded(){return loaded;}
  std::map<std::string, Spell*>* getVocSpells(int voc){
		if(voc>maxVoc || voc<0){
			return 0;
		}
		
		return &(vocationSpells.at(voc));
	}
  
	std::map<std::string, Spell*>* getAllSpells(){
		return &allSpells;
	}
	
	//////////////////
  std::map<unsigned short, Spell*>* getVocRuneSpells(int voc){
		if(voc>maxVoc || voc<0){
			return 0;
		}
		
		return &(vocationRuneSpells.at(voc));
	}
  
	std::map<unsigned short, Spell*>* getAllRuneSpells(){
		return &allRuneSpells;
	}
	//////////////////

protected:
  std::map<std::string, Spell*> allSpells;
  std::vector<std::map<std::string, Spell*> > vocationSpells;

	std::map<unsigned short, Spell*> allRuneSpells;
	std::vector<std::map<unsigned short, Spell*> > vocationRuneSpells;
  bool loaded;
  int maxVoc;
};

enum manausage_t {
	MANAUSAGE_FIXED,
	MANAUSAGE_PERCENT
};

class Spell
{
public:
  Spell(std::string name, int magLv, int mana, Game* game);
  virtual ~Spell();

	bool isLoaded(){return loaded;}
	virtual bool castSpell(Creature* creature, const Position& pos, const std::string& var) const;
	virtual bool castSpell(Creature* creature, Creature* targetCreature) const;
	std::string getName() const {return name;};
	int getMana() const {return mana;};
	int getMagLv() const {return magLv;};

protected:
	Game* game;
	MagicSpell *magicspell;
	SpellScript* script;
	bool offensive;
	std::string name;
	manausage_t manausage;
  int magLv, mana;
  bool loaded;

	bool internalCastSpell(Creature* creature, const Position& pos) const;

	friend class SpellScript;
	friend class ConjureItemSpell;
	friend class ChangeSpeedSpell;
	friend class LightSpell;
	friend class MagicAreaSpell;
	friend class MagicTargetSpell;
};

class InstantSpell : public Spell
{
public:
	InstantSpell(const std::string &, std::string name, std::string words, int magLv, int mana, Game* game);
	virtual bool castSpell(Creature* creature, const Position& pos, const std::string& var) const;
	virtual bool castSpell(Creature* creature, Creature* targetCreature) const {return false;};
	std::string getWords(){return words;};

protected:
	std::string words;
};

class RuneSpell : public Spell
{
public:
	RuneSpell(const std::string& ,std::string name, unsigned short id, unsigned short charges, int magLv, int mana, Game* game);
	virtual bool castSpell(Creature* creature, const Position& pos, const std::string& var) const;
	virtual bool castSpell(Creature* creature, Creature* targetCreature) const;
protected:
  unsigned short id;
	unsigned short charges;
};

class SpellScript : protected LuaScript
{
public:
	SpellScript(const std::string&, std::string filename, Spell* spell);
	virtual ~SpellScript(){}
  bool isLoaded(){return loaded;}
	int onUse(Creature* spellCastCreature, Creature *target, const std::string& var);

	static int luaActionCreateConjureItemSpell(lua_State *L);
	static int luaActionCreateChangeSpeedSpell(lua_State *L);
	static int luaActionCreateLightSpell(lua_State *L);
	static int luaActionCreateAreaAttackSpell(lua_State *L);
	static int luaActionCreateTargetSpell(lua_State *L);

	//help functions
	static int luaActionGetPlayerLevel(lua_State *L);
	static int luaActionGetPlayerMagLevel(lua_State *L);

protected:
	std::string scriptname;
	bool loaded;

	static void internalGetArea(lua_State *L, AreaVector& vec);
	static attacktype_t internalGetAttackType(lua_State *L);
  static Spell* getSpell(lua_State *L);

	int registerFunctions();
};
#endif // __spells_h_

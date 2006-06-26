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
#include "luascript.h"
#include "player.h"
#include "actions.h"
#include "talkaction.h"

class RuneSpell;
class InstantSpell;

class Spells
{
public:
	Spells();
	~Spells();
	
	bool reload();
	void clear();
	
	bool loadFromXml(const std::string& datadir);
	
	RuneSpell* getRuneSpell(Item* item);
	RuneSpell* getRuneSpell(const std::string& name);
	
	InstantSpell* getInstantSpell(const std::string& words);
	InstantSpell* getInstantSpellByName(const std::string& name);
	
protected:
	bool loaded;
	std::string datadir;
	typedef std::map<uint32_t, RuneSpell*> RunesMap;
	typedef std::map<std::string, InstantSpell*> InstantsMap ;
	RunesMap runes;
	InstantsMap instants;
	
	LuaScriptInterface m_scriptInterface;
};


enum TargetType_t{
	TARGET_NONE,
	TARGET_CREATURE,
	TARGET_POSITION,
};

typedef bool (InstantSpellFunction)(Creature* creature, const std::string& words, const std::string& param);
typedef bool (RuneSpellFunction)(Creature* creature, Item* item, const Position& posFrom, const Position& posTo, Creature* target);

class Spell
{
public:
	Spell();
	virtual ~Spell(){};
	
	bool configureSpell(xmlNodePtr xmlspell);
	
	virtual bool loadScriptSpell(const std::string& script) = 0;
	virtual bool loadFunctionSpell(const std::string& function) = 0;
	
protected:
	bool spellPlayerChecks(Player* player);
	bool causeExhaustion(){return exhaustion;};
	
	void addSpellEffects(Player* player);
	
	TargetType_t targetType;
	
private:
	std::string name;
	
	bool enabled;
	bool premium;
	uint32_t level;
	uint32_t magLevel;
	uint32_t vocationBits;
	
	uint32_t mana;
	uint32_t soul;
	bool exhaustion;
};

class InstantSpell : public TalkAction, public Spell
{
public:
	InstantSpell(LuaScriptInterface* _interface);
	virtual ~InstantSpell();
	
	bool configureSpell(xmlNodePtr xmlspell);
	
	bool castInstant(Creature* creature, const std::string& words, const std::string& param);
	
	virtual bool loadFunctionSpell(const std::string& function);
	//scripting
	virtual bool loadScriptSpell(const std::string& script);
	bool executeCastInstant(Creature* creature, const std::string& param);
	//
	
protected:
	
	static InstantSpellFunction HouseGuestList;
	static InstantSpellFunction HouseSubOwnerList;
	static InstantSpellFunction HouseDoorList;
	static InstantSpellFunction HouseKick;
	static InstantSpellFunction SearchPlayer;
	
	static House* getHouseFromPos(Creature* creature);
	
	bool hasParam;
	
	bool scripted;
	InstantSpellFunction* function;
};

class RuneSpell : public Action, public Spell
{
public:
	RuneSpell(LuaScriptInterface* _interface);
	virtual ~RuneSpell();
	
	bool configureSpell(xmlNodePtr xmlspell);
	
	bool useRune(Creature* creature, Item* item, const Position& posFrom, const Position& posTo, Creature* target);
	
	virtual bool loadFunctionSpell(const std::string& function);
	//sciprting
	virtual bool loadScriptSpell(const std::string& script);
	bool executeUseRune(Creature* creature, Item* item, const Position& posFrom, const Position& posTo, Creature* target);
	//
	
	uint32_t getRuneItemId(){return runeId;};
	
protected:
	bool hasCharges;
	uint32_t runeId;
	
	bool scripted;
	RuneSpellFunction* function;
};

//////////////////////////////////////////////////////////////////////
// Defines a Spell...
/*class Spell;
class SpellScript;
class Spells
{
public:
  Spells(Game* game);
  bool loadFromXml(const std::string&);
  virtual ~Spells();

	Game* game;

  bool isLoaded(){return loaded;}
  std::map<std::string, Spell*>* getVocSpells(Vocation_t voc)
	{
		if((int)voc > maxVoc || voc < 0){
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
*/
/*
class Spell
{
public:
  Spell(std::string name, int magLv, int mana, Game* game);
  virtual ~Spell();

	Game* game;

	bool isLoaded(){return loaded;}
	SpellScript* getSpellScript(){return script;};
	std::string getName() const {return name;};
	int getMana(){return mana;};
	int getMagLv(){
  return magLv;};

protected:
	std::string name;
  int magLv, mana;
  bool loaded;
	SpellScript* script;
};

class InstantSpell : public Spell
{
public:
	InstantSpell(const std::string &, std::string name, std::string words, int magLv, int mana, Game* game);
	std::string getWords(){return words;};

protected:
	std::string words;
};

class RuneSpell : public Spell
{
public:
	RuneSpell(const std::string& ,std::string name, unsigned short id, unsigned short charges, int magLv, int mana, Game* game);

protected:
  unsigned short id;
	unsigned short charges;
};

class SpellScript : protected LuaScript{
public:
	SpellScript(const std::string&, std::string scriptname, Spell* spell);
	virtual ~SpellScript(){}
  bool castSpell(Creature* creature, const Position& pos, std::string var);
  bool isLoaded(){return loaded;}
  static Spell* getSpell(lua_State *L);

	static int luaActionDoTargetSpell(lua_State *L);
	static int luaActionDoTargetExSpell(lua_State *L);
	static int luaActionDoTargetGroundSpell(lua_State *L);
	static int luaActionDoAreaSpell(lua_State *L);
	static int luaActionDoAreaExSpell(lua_State *L);
	static int luaActionDoAreaGroundSpell(lua_State *L);

	//static int luaActionDoSpell(lua_State *L);
  static int luaActionGetPos(lua_State *L);
  static int luaActionChangeOutfit(lua_State *L);
  static int luaActionManaShield(lua_State *L);
  static int luaActionChangeSpeed(lua_State *L);
  static int luaActionChangeSpeedMonster(lua_State *L);
  static int luaActionGetSpeed(lua_State *L);
  static int luaActionMakeRune(lua_State *L);
  static int luaActionMakeArrows(lua_State *L);
  static int luaActionMakeFood(lua_State *L);
protected:
	static void internalGetArea(lua_State *L, MagicEffectAreaClass &magicArea);
	static void internalGetPosition(lua_State *L, Position& pos);
	static void internalGetMagicEffect(lua_State *L, MagicEffectClass &me);
	static void internalLoadDamageVec(lua_State *L, ConditionVec& condvec);
	static void internalLoadTransformVec(lua_State *L, TransformMap& transformMap);
	static int  internalMakeRune(Player *p,unsigned short sl_id,Spell *S,unsigned short id, unsigned char charges);
	int registerFunctions();
	Spell* spell;
	bool loaded;
};
*/
#endif // __spells_h_

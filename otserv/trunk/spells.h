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


#include "creature.h"
#include "map.h"
#include "luascript.h"

extern "C"
{
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}


//////////////////////////////////////////////////////////////////////
// Defines an NPC...
class Spell;
class SpellScript;

class Spells
{
public:
  Spells(Map* map);
  bool loadFromXml();
  virtual ~Spells();

	Map* map;

  bool isLoaded(){return loaded;}
  std::map<std::string, Spell*>* getVocSpells(int voc){
                        if(voc>maxVoc || voc<0){
                                      return 0;
                                      }
                        return &(vocationSpells.at(voc));
                        } 
  std::vector<Spell*>* getAllSpells(){
                        return &allSpells;
                        }
protected:

  std::vector<Spell*> allSpells;
  std::vector<std::map<std::string, Spell*> > vocationSpells;
  bool loaded;
  int maxVoc;
};


class Spell
{
public:
  Spell(std::string name, std::string words, bool var, int magLv, int mana, Map* map);
  virtual ~Spell();

	Map* map;

  bool isLoaded(){return loaded;}
SpellScript* getSpellScript(){return script;};
int getMana(){return mana;};
std::string getWords(){
            return words;};
int getMagLv(){
            return magLv;};
protected:

  std::string name, words;
  bool var;
  int magLv, mana;
  bool loaded;
  SpellScript* script;
};

class SpellScript : protected LuaScript{
public:
	SpellScript(std::string scriptname, Spell* spell);
	virtual ~SpellScript(){}
  void castSpell(Creature* creature, std::string var);
  bool isLoaded(){return loaded;}
  static Spell* SpellScript::getSpell(lua_State *L);
  static int SpellScript::luaActionDoSpell(lua_State *L);
protected:
    int registerFunctions();
	Spell* spell;
	bool loaded;      
};
#endif // __npc_h_

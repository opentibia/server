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


#include "definitions.h"

#include <algorithm>
#include <functional>
#include <string>
#include <fstream>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h> 

#include <boost/config.hpp>
#include <boost/bind.hpp>

#include "spells.h"
Spells::Spells(Map* imap): map(imap){
                   
                   }

bool Spells::loadFromXml()
{
                    std::string name, words;
                    bool enabled;
                    int vocId, maglv, mana;
                    this->loaded = false;
                    xmlDocPtr doc = xmlParseFile(std::string("data/spells/spells.xml").c_str());
                    if (doc){
                             this->loaded=true;
		                     xmlNodePtr root, p, tmp;
		                     root = xmlDocGetRootElement(doc);
		                     
		                     if (xmlStrcmp(root->name,(const xmlChar*) "spells")){
		                        //TODO: use exceptions here
		                        std::cerr << "Malformed XML" << std::endl;
		                        }
                             if ((const char*)xmlGetProp(root, (const xmlChar *)"maxVoc")) {
			                    this->maxVoc = atoi((const char*)xmlGetProp(root, (const xmlChar *)"maxVoc"));
		                     }
		                     for(int i =0; i<=this->maxVoc; i++){
                                     std::map<std::string, Spell*> voc;
                                     vocationSpells.push_back(voc);
                                     }
                             p = root->children;
                             
                             while (p)
		                     {
                             const char* str = (char*)p->name;
			                 if (strcmp(str, "spell") == 0){
                                             if ((const char*)xmlGetProp(p, (const xmlChar *)"enabled")) {
			                                    enabled = (bool)atoi((const char*)xmlGetProp(p, (const xmlChar *)"enabled"));
		                                     }
		                                     if (enabled){
                                             if ((const char*)xmlGetProp(p, (const xmlChar *)"name")) {
			                                    name = (const char*)xmlGetProp(p, (const xmlChar *)"name");
		                                     }
		                                     if ((const char*)xmlGetProp(p, (const xmlChar *)"words")) {
			                                    words = (const char*)xmlGetProp(p, (const xmlChar *)"words");
		                                     }
		                                     if ((const char*)xmlGetProp(p, (const xmlChar *)"maglv")) {
			                                    maglv = atoi((const char*)xmlGetProp(p, (const xmlChar *)"maglv"));
		                                     }
		                                     if ((const char*)xmlGetProp(p, (const xmlChar *)"mana")) {
			                                    mana = atoi((const char*)xmlGetProp(p, (const xmlChar *)"mana"));
		                                     }
		                                     Spell* spell = new Spell(name, words, maglv, mana, map);
				                             tmp=p->children;
				                             while (tmp){
                                                   if (strcmp((const char*)tmp->name, "vocation") == 0){
                                                                     if ((const char*)xmlGetProp(tmp, (const xmlChar *)"id")) {
			                                                            vocId = atoi((const char*)xmlGetProp(tmp, (const xmlChar *)"id"));
			                                                            if (vocId<=this->maxVoc){                                                                           
                                                                           (vocationSpells.at(vocId))[words] = spell;
                                                                        }
		                                                                }
                                                                     }
                                                   tmp = tmp->next;
                                                   }
                                             allSpells[words] = spell;           
                                             }
				             
			                 }
			                 
                               p = p->next;    
                              }
                    xmlFreeDoc(doc);
                    }
                    return this->loaded;
}
Spells::~Spells(){
                  std::map<std::string, Spell*>::iterator it;
                  for(it = allSpells.begin(); it != allSpells.end(); it++){
                                           delete it->second;;
                                           allSpells.erase(it);
                                           }
                 }

Spell::Spell(std::string iname, std::string iwords, int imagLv, int imana, Map* imap) : name(iname), words(iwords), magLv(imagLv), mana(imana), map(imap){
                        //now try to load the script
	                this->script = new SpellScript(std::string("data/spells/")+(this->words)+std::string(".lua"), this);
	                if(!this->script->isLoaded())
		            this->loaded=false;
                        }
Spell::~Spell(){
                 delete script;
                 }
                 
SpellScript::SpellScript(std::string scriptname, Spell* spell){
	this->loaded = false;
	if(scriptname == "")
		return;
	luaState = lua_open();
	luaopen_loadlib(luaState);
	luaopen_base(luaState);
	luaopen_math(luaState);
	luaopen_string(luaState);
	luaopen_io(luaState);
    lua_dofile(luaState, "data/spells/lib/spells.lua");
	
	FILE* in=fopen(scriptname.c_str(), "r");
	if(!in)
		return;
	else
		fclose(in);
	lua_dofile(luaState, scriptname.c_str());
	this->loaded=true;
	this->spell=spell;
	this->setGlobalNumber("addressOfSpell", (int) spell);
	this->registerFunctions();
}

int SpellScript::registerFunctions(){
	lua_register(luaState, "doMagic", SpellScript::luaActionDoSpell);
	lua_register(luaState, "changeOutfit", SpellScript::luaActionChangeOutfit);
	lua_register(luaState, "manaShield", SpellScript::luaActionManaShield);
	lua_register(luaState, "getPosition", SpellScript::luaActionGetPos);
	lua_register(luaState, "getSpeed", SpellScript::luaActionGetSpeed);
	lua_register(luaState, "changeSpeed", SpellScript::luaActionChangeSpeed);
	return true;
}

void SpellScript::castSpell(Creature* creature, std::string var){
     lua_pushstring(luaState, "onCast");
     lua_gettable(luaState, LUA_GLOBALSINDEX);
     lua_pushnumber(luaState, creature->getID());
		
      lua_newtable(luaState);
      setField("z", creature->pos.z);
      setField("y", creature->pos.y);
      setField("x", creature->pos.x);

      lua_pushnumber(luaState, creature->level);
      lua_pushnumber(luaState, creature->maglevel);
      lua_pushstring(luaState, var.c_str());
		
      lua_call(luaState, 5,0);
}

Spell* SpellScript::getSpell(lua_State *L){
	lua_getglobal(L, "addressOfSpell");
	int val = (int)lua_tonumber(L, -1);
	lua_pop(L,1);

	Spell* myspell = (Spell*) val;
	if(!myspell){
		return 0;
	}
	return myspell;
}

int SpellScript::luaActionDoSpell(lua_State *L){
    int cx,cy,cz;
    bool needDirection;
    EffectInfo ei;
    ei.animationEffect = 0;
    ei.needtarget = false;
    
    needDirection = (bool)lua_toboolean(L, -1);
	lua_pop(L,1);
    
    ei.maxDamage = (int)lua_tonumber(L, -1);
	lua_pop(L,1);
	
	ei.minDamage = (int)lua_tonumber(L, -1);
	lua_pop(L,1);
    
	ei.offensive = lua_toboolean(L, -1);
	lua_pop(L,1);
	
	ei.animationcolor = (char)lua_tonumber(L, -1);
	lua_pop(L,1);
	
	ei.areaEffect = (char)lua_tonumber(L, -1);
	lua_pop(L,1);
	
	ei.damageEffect = (char)lua_tonumber(L, -1);
	lua_pop(L,1);
	
	lua_pushstring(L, "z");
    lua_gettable(L, -2);
    cz = (int)lua_tonumber(L, -1);
    lua_pop(L, 1);
    
    lua_pushstring(L, "y");
    lua_gettable(L, -2);
    cy = (int)lua_tonumber(L, -1);
    lua_pop(L, 1);
    
    lua_pushstring(L, "x");
    lua_gettable(L, -2);
    cx = (int)lua_tonumber(L, -1);
    lua_pop(L, 1);
     
    
    lua_pop(L, 1); //table
    
	unsigned char area[14][18]={};
	int i=0, j = 0;
	lua_pushnil(L);  /* first key */
       while (lua_next(L, -2) != 0) {
           lua_pushnil(L);
           while (lua_next(L, -2) != 0) {
                 if(i<14 && j <18){
                 area [i][j] = (unsigned char)lua_tonumber(L, -1);
                 }
                 lua_pop(L, 1);  /* removes `value'; keeps `key' for next iteration */
                 j++;
                 }
         j=0;
         lua_pop(L, 1);  /* removes `value'; keeps `key' for next iteration */
         i++;
       }
       lua_pop(L, 1);
   memcpy(&ei.area, area, sizeof(area));	
   Spell* spell = getSpell(L);
   ei.manaCost = spell->getMana();
   
   Creature* creature = spell->map->getCreatureByID((unsigned long)lua_tonumber(L, -1));
   lua_pop(L,1);
   
   if(needDirection){
                    switch(creature->getDirection()) {
			                                    case NORTH: ei.direction = 1; break;
			                                    case WEST: ei.direction = 2; break;
			                                    case EAST: ei.direction = 3; break;
			                                    case SOUTH: ei.direction = 4; break;
		                                     };
                    }
   else {
        ei.direction = 1;
        }
   Position pos = Position(cx, cy, cz);
   ei.centerpos = pos;
   spell->map->creatureCastSpell(creature, ei);
   return 0;
}
int SpellScript::luaActionChangeOutfit(lua_State *L){
    int looktype = (int)lua_tonumber(L, -1);
	lua_pop(L,1);
	long time = (long)lua_tonumber(L, -1)*1000;
	lua_pop(L,1);
	
	Spell* spell = getSpell(L);
	Creature* creature = spell->map->getCreatureByID((unsigned long)lua_tonumber(L, -1));
	lua_pop(L,1);
	
	creature->looktype = looktype;
	spell->map->creatureChangeOutfit(creature);
	
    spell->map->changeOutfitAfter(creature->getID(), creature->lookmaster, time);
    return 0;
}

int SpellScript::luaActionManaShield(lua_State *L){
	long time = (long)lua_tonumber(L, -1)*1000;
	lua_pop(L,1);
	
	Spell* spell = getSpell(L);
	Creature* creature = spell->map->getCreatureByID((unsigned long)lua_tonumber(L, -1));
	lua_pop(L,1);
	creature->manaShieldTicks = time;
	
	Player* p = dynamic_cast<Player*>(creature);
	if(p)
	     p->sendIcons();
	return 0;
}

int SpellScript::luaActionChangeSpeed(lua_State *L){
	long time = (long)lua_tonumber(L, -1)*1000;
	lua_pop(L,1);
	
	int speed = (int)lua_tonumber(L, -1);
	lua_pop(L,1);
	
	Spell* spell = getSpell(L);
	Creature* creature = spell->map->getCreatureByID((unsigned long)lua_tonumber(L, -1));
	lua_pop(L,1);
	
	spell->map->addEvent(makeTask(time, boost::bind(&Map::changeSpeed, spell->map,creature->getID(), creature->speed) ) );
	Player* p = dynamic_cast<Player*>(creature);
	if(p){
         spell->map->changeSpeed(creature->getID(), creature->getNormalSpeed()+speed); 
	     p->sendIcons();
      }
    creature->hasteTicks = time;  
	return 0;
}

int SpellScript::luaActionGetSpeed(lua_State *L){
	Spell* spell = getSpell(L);
	Creature* creature = spell->map->getCreatureByID((unsigned long)lua_tonumber(L, -1));
	lua_pop(L,1);
	
	lua_pushnumber(L, creature->getNormalSpeed());
	return 1;
}

int SpellScript::luaActionGetPos(lua_State *L){
	const char* s = lua_tostring(L, -1);
	lua_pop(L,1);
	Spell* spell = getSpell(L);
	Creature* c = spell->map->getCreatureByName(s);
	Player* p = dynamic_cast<Player*>(c);
	if(!c || !p){
      lua_newtable(L);
      lua_pushstring(L, "x");
      lua_pushnil(L);
      lua_settable(L, -3);
      
      lua_pushstring(L, "y");
      lua_pushnil(L);
      lua_settable(L, -3);
      
      lua_pushstring(L, "z");
      lua_pushnil(L);
      lua_settable(L, -3);
	}
	else{
         lua_newtable(L);
      lua_pushstring(L, "x");
      lua_pushnumber(L, c->pos.x);
      lua_settable(L, -3);
      
      lua_pushstring(L, "y");
      lua_pushnumber(L, c->pos.y);
      lua_settable(L, -3);
      
      lua_pushstring(L, "z");
      lua_pushnumber(L, c->pos.z);
      lua_settable(L, -3);
	}
	return 1;
}

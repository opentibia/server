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

Spells::Spells(Game* igame): game(igame){
                   
                   }

bool Spells::loadFromXml()
{
	std::string name, words;
  bool enabled = false;
  int vocId, maglv = 0, mana = 0, id = 0, charges = 0;
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

					Spell* spell = new InstantSpell(name, words, maglv, mana, game);
					
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
			else if (strcmp(str, "rune") == 0){
				if ((const char*)xmlGetProp(p, (const xmlChar *)"enabled")) {
					enabled = (bool)atoi((const char*)xmlGetProp(p, (const xmlChar *)"enabled"));
				}
				
				if (enabled){
					if ((const char*)xmlGetProp(p, (const xmlChar *)"name")) {
						name = (const char*)xmlGetProp(p, (const xmlChar *)"name");
					}

					if ((const char*)xmlGetProp(p, (const xmlChar *)"id")) {
						id = atoi((const char*)xmlGetProp(p, (const xmlChar *)"id"));
					}

					if ((const char*)xmlGetProp(p, (const xmlChar *)"charges")) {
						charges = atoi((const char*)xmlGetProp(p, (const xmlChar *)"charges"));
					}

					if ((const char*)xmlGetProp(p, (const xmlChar *)"maglv")) {
						maglv = atoi((const char*)xmlGetProp(p, (const xmlChar *)"maglv"));
					}

					if ((const char*)xmlGetProp(p, (const xmlChar *)"mana")) {
						mana = atoi((const char*)xmlGetProp(p, (const xmlChar *)"mana"));
					}

					Spell* spell = new RuneSpell(name, id, charges, maglv, mana, game);
					
					tmp=p->children;
					while (tmp){
						if (strcmp((const char*)tmp->name, "vocation") == 0){
							if ((const char*)xmlGetProp(tmp, (const xmlChar *)"id")) {
								vocId = atoi((const char*)xmlGetProp(tmp, (const xmlChar *)"id"));
								if (vocId<=this->maxVoc){                                                                           
									(vocationRuneSpells.at(vocId))[id] = spell;
								}
							}
						}
						
						tmp = tmp->next;
					}
					
					allRuneSpells[id] = spell;
				}
			}

			p = p->next;    
		}
		
		xmlFreeDoc(doc);
	}

  return this->loaded;
}
Spells::~Spells(){
	std::map<std::string, Spell*>::iterator it = allSpells.begin();

	while(it != allSpells.end()) {
		delete it->second;
		allSpells.erase(it);
		it = allSpells.begin();
	}
}

Spell::Spell(std::string iname, int imagLv, int imana, Game* igame)
:  game(igame), name(iname),magLv(imagLv) , mana(imana)
{
	this->loaded=false;
	this->script = NULL;
}

Spell::~Spell(){
	if(script) {
		delete script;
		script = NULL;
	}
}

InstantSpell::InstantSpell(std::string iname, std::string iwords, int magLv, int mana, Game* game)
: Spell(iname, magLv, mana, game), words(iwords)
{
	this->script = new SpellScript(std::string("data/spells/instant/")+(this->words)+std::string(".lua"), this);
	if(!this->script->isLoaded())
		this->loaded=false;
}


RuneSpell::RuneSpell(std::string iname, unsigned short id, unsigned short charges, int magLv, int mana, Game* game)
: Spell(iname, magLv, mana, game)
{
	this->id = id;
	this->charges = charges;

	this->script = new SpellScript(std::string("data/spells/runes/")+(this->name)+std::string(".lua"), this);
	if(!this->script->isLoaded())
		this->loaded=false;
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

	lua_register(luaState, "doTargetMagic", SpellScript::luaActionDoTargetSpell);
	lua_register(luaState, "doTargetExMagic", SpellScript::luaActionDoTargetExSpell);
	lua_register(luaState, "doTargetGroundMagic", SpellScript::luaActionDoTargetGroundSpell);
	lua_register(luaState, "doAreaMagic", SpellScript::luaActionDoAreaSpell);
	lua_register(luaState, "doAreaExMagic", SpellScript::luaActionDoAreaExSpell);
	lua_register(luaState, "doAreaGroundMagic", SpellScript::luaActionDoAreaGroundSpell);

	//lua_register(luaState, "doMagic", SpellScript::luaActionDoSpell);
	lua_register(luaState, "changeOutfit", SpellScript::luaActionChangeOutfit);
	lua_register(luaState, "manaShield", SpellScript::luaActionManaShield);
	lua_register(luaState, "getPosition", SpellScript::luaActionGetPos);
	lua_register(luaState, "getSpeed", SpellScript::luaActionGetSpeed);
	lua_register(luaState, "changeSpeed", SpellScript::luaActionChangeSpeed);
	return true;
}

bool SpellScript::castSpell(Creature* creature, const Position& pos, std::string var){
	lua_pushstring(luaState, "onCast");
	lua_gettable(luaState, LUA_GLOBALSINDEX);
	lua_pushnumber(luaState, creature->getID());

	lua_newtable(luaState);
	setField("z", pos.z);
	setField("y", pos.y);
	setField("x", pos.x);

	lua_pushnumber(luaState, creature->level);
	lua_pushnumber(luaState, creature->maglevel);
	lua_pushstring(luaState, var.c_str());

	lua_call(luaState, 5, 1);

	bool ret = (bool)lua_toboolean(luaState, -1);
	lua_pop(luaState, 1);

	return ret;
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


void SpellScript::internalGetArea(lua_State *L, MagicEffectAreaClass &magicArea)
{
	//unsigned char area[14][18]={};
	
	std::vector<unsigned char> col;

	int i=0, j = 0;
	lua_pushnil(L);  /* first key */

	while (lua_next(L, -2) != 0) {
		lua_pushnil(L);
		col.clear();
    while (lua_next(L, -2) != 0) {
			//if(i< rows /*14*/ && j < cols /*18*/){
				//area[i][j] = (unsigned char)lua_tonumber(L, -1);
				col.push_back((unsigned char)lua_tonumber(L, -1));
			//}
			
			lua_pop(L, 1);  /* removes `value'; keeps `key' for next iteration */
			j++;
		}

		magicArea.areaVec.push_back(col);
		
		j=0;
		lua_pop(L, 1);  /* removes `value'; keeps `key' for next iteration */
		i++;
	}
	
	lua_pop(L, 1);

	magicArea.areaEffect = (char)lua_tonumber(L, -1);
	lua_pop(L,1);

	//memcpy(&magicArea.area, area, sizeof(area));	
}

void SpellScript::internalGetPosition(lua_State *L, Position& pos)
{
	lua_pushstring(L, "z");
	lua_gettable(L, -2);
	pos.z = (int)lua_tonumber(L, -1);
	lua_pop(L, 1);

	lua_pushstring(L, "y");
	lua_gettable(L, -2);
	pos.y = (int)lua_tonumber(L, -1);
	lua_pop(L, 1);

	lua_pushstring(L, "x");
	lua_gettable(L, -2);
	pos.x = (int)lua_tonumber(L, -1);
	lua_pop(L, 1);

  lua_pop(L, 1); //table
}

void SpellScript::internalGetMagicEffect(lua_State *L, MagicEffectClass& me)
{
	/*
  lua_pushnil(L);
  while (lua_next(L, 2) != 0) {
    printf("%s - %s\n", lua_typename(L, lua_type(L, -2)), lua_typename(L, lua_type(L, -1)));
    lua_pop(L, 1);
  }

	lua_pushnil(L); //start of table
	*/

	lua_pushnil(L);

	lua_next(L, -2);
	me.animationEffect = (char)lua_tonumber(L, -1);
	lua_pop(L, 1);

	lua_next(L, -2);
	me.damageEffect = (char)lua_tonumber(L, -1);
	lua_pop(L, 1);

	lua_next(L, -2);
	me.animationColor = (char)lua_tonumber(L, -1);
	lua_pop(L, 1);

	lua_next(L, -2);
	me.offensive = lua_toboolean(L, -1);
	lua_pop(L, 1);

	lua_next(L, -2);
	me.physical = lua_toboolean(L, -1);
	lua_pop(L, 1);

	lua_next(L, -2);
	me.minDamage = (int)lua_tonumber(L, -1);
	lua_pop(L, 1);

	lua_next(L, -2);
	me.maxDamage = (int)lua_tonumber(L, -1);
	lua_pop(L, 1);

	lua_pop(L, 1); // last key

	lua_pop(L, 1); //end of table
}

int SpellScript::luaActionDoTargetSpell(lua_State *L)
{
	MagicEffectTargetClass magicTarget;

	internalGetMagicEffect(L, magicTarget);

	Position centerpos;
	internalGetPosition(L, centerpos);

	Spell* spell = getSpell(L);
	magicTarget.manaCost = spell->getMana();

	Creature* creature = spell->game->getCreatureByID((unsigned long)lua_tonumber(L, -1));
  lua_pop(L,1);
		
	bool isSuccess = spell->game->creatureThrowRune(creature, centerpos, magicTarget);
	lua_pushboolean(L, isSuccess);
	return 1;
}

int SpellScript::luaActionDoTargetExSpell(lua_State *L)
{
	MagicDamageVec dmgvec;
	internalLoadDamageVec(L, dmgvec);

	MagicDamageType md = (MagicDamageType)(int)lua_tonumber(L, -1);
	lua_pop(L,1);

	MagicEffectTargetEx magicTargetEx(md, dmgvec);

	internalGetMagicEffect(L, magicTargetEx);

	Position centerpos;
	internalGetPosition(L, centerpos);

	Spell* spell = getSpell(L);
  magicTargetEx.manaCost = spell->getMana();

	Creature* creature = spell->game->getCreatureByID((unsigned long)lua_tonumber(L, -1));
  lua_pop(L,1);
	
	bool isSuccess = spell->game->creatureThrowRune(creature, centerpos, magicTargetEx);
	lua_pushboolean(L, isSuccess);
	return 1;
}

int SpellScript::luaActionDoTargetGroundSpell(lua_State *L)
{
	damageMapClass dmgMap;
	internalLoadTransformVec(L, dmgMap);
	
	MagicEffectItem* fieldItem = new MagicEffectItem(magicNone, dmgMap);
	MagicEffectTargetGroundClass magicGround(fieldItem);

	magicGround.offensive = lua_toboolean(L, -1);
	lua_pop(L,1);
	
	magicGround.animationEffect = (char)lua_tonumber(L, -1);
	lua_pop(L,1);

	Position centerpos;
	internalGetPosition(L, centerpos);

	Spell* spell = getSpell(L);
  magicGround.manaCost = spell->getMana();

	Creature* creature = spell->game->getCreatureByID((unsigned long)lua_tonumber(L, -1));
	
	bool isSuccess = spell->game->creatureThrowRune(creature, centerpos, magicGround);
	lua_pushboolean(L, isSuccess);
	return 1;
}

int SpellScript::luaActionDoAreaSpell(lua_State *L)
{
	MagicEffectAreaClass magicArea;
	internalGetMagicEffect(L, magicArea);

	internalGetArea(L, magicArea);

	bool needDirection = (bool)lua_toboolean(L, -1);
	lua_pop(L,1);

	Position centerpos;
	internalGetPosition(L, centerpos);

	Spell* spell = getSpell(L);
  magicArea.manaCost = spell->getMana();
  
	Creature* creature = spell->game->getCreatureByID((unsigned long)lua_tonumber(L, -1));
	lua_pop(L,1);
	 
	if(needDirection){
		switch(creature->getDirection()) {
			case NORTH: magicArea.direction = 1; break;
			case WEST: magicArea.direction = 2; break;
			case EAST: magicArea.direction = 3; break;
			case SOUTH: magicArea.direction = 4; break;
		};
	}
  else {
		magicArea.direction = 1;
	}

	bool isSuccess = spell->game->creatureCastSpell(creature, centerpos, magicArea);
	lua_pushboolean(L, isSuccess);
	return 1;
}

int SpellScript::luaActionDoAreaExSpell(lua_State *L)
{
	MagicDamageVec dmgvec;

	int count = (int)lua_tonumber(L, -1);
	lua_pop(L,1);

	for(int n = 0; n < count; ++n) {
		internalLoadDamageVec(L, dmgvec);
	}

	MagicDamageType md = (MagicDamageType)(int)lua_tonumber(L, -1);
	lua_pop(L,1);

	MagicEffectAreaExClass magicAreaEx(md, dmgvec);

	internalGetMagicEffect(L, magicAreaEx);
    
	internalGetArea(L, magicAreaEx);

	bool needDirection = (bool)lua_toboolean(L, -1);
	lua_pop(L,1);

	Position centerpos;
	internalGetPosition(L, centerpos);

	Spell* spell = getSpell(L);
  magicAreaEx.manaCost = spell->getMana();
  
	Creature* creature = spell->game->getCreatureByID((unsigned long)lua_tonumber(L, -1));
	lua_pop(L,1);
		 	
	if(needDirection){
		switch(creature->getDirection()) {
			case NORTH: magicAreaEx.direction = 1; break;
			case WEST: magicAreaEx.direction = 2; break;
			case EAST: magicAreaEx.direction = 3; break;
			case SOUTH: magicAreaEx.direction = 4; break;
		};
	}
  else {
		magicAreaEx.direction = 1;
	}

	bool isSuccess = spell->game->creatureCastSpell(creature, centerpos, magicAreaEx);
	lua_pushboolean(L, isSuccess);
	return 1;
}

int SpellScript::luaActionDoAreaGroundSpell(lua_State *L)
{
	damageMapClass dmgMap;
	internalLoadTransformVec(L, dmgMap);
	
	MagicDamageType md = (MagicDamageType)(int)lua_tonumber(L, -1);
	lua_pop(L,1);

	MagicEffectItem* fieldItem = new MagicEffectItem(md, dmgMap);
	MagicEffectGroundAreaClass magicGroundEx(fieldItem);

	internalGetMagicEffect(L, magicGroundEx);

	internalGetArea(L, magicGroundEx);

	bool needDirection = (bool)lua_toboolean(L, -1);
	lua_pop(L,1);

	Position centerpos;
	internalGetPosition(L, centerpos);

	Spell* spell = getSpell(L);
  magicGroundEx.manaCost = spell->getMana();

	Creature* creature = spell->game->getCreatureByID((unsigned long)lua_tonumber(L, -1));
	lua_pop(L, 1);
	
	if(needDirection){
		switch(creature->getDirection()) {
			case NORTH: magicGroundEx.direction = 1; break;
			case WEST: magicGroundEx.direction = 2; break;
			case EAST: magicGroundEx.direction = 3; break;
			case SOUTH: magicGroundEx.direction = 4; break;
		};
	}
  else {
		magicGroundEx.direction = 1;
	}

	bool isSuccess = spell->game->creatureThrowRune(creature, centerpos, magicGroundEx);
	lua_pushboolean(L, isSuccess);
	return 1;
}

/*
int SpellScript::luaActionDoSpell(lua_State *L){

	MagicEffectAreaClass magicInstant;
    
  needDirection = (bool)lua_toboolean(L, -1);
	lua_pop(L,1);

	magicInstant.maxDamage = (int)lua_tonumber(L, -1);
	lua_pop(L,1);
	
	magicInstant.minDamage = (int)lua_tonumber(L, -1);
	lua_pop(L,1);
    
  magicInstant.physical = (bool)lua_toboolean(L, -1);
	lua_pop(L,1);

	magicInstant.offensive = (bool)lua_toboolean(L, -1);
	lua_pop(L,1);
	
	magicInstant.animationColor = (char)lua_tonumber(L, -1);
	lua_pop(L,1);

	magicInstant.areaEffect = (char)lua_tonumber(L, -1);
	lua_pop(L,1);
	
	magicInstant.damageEffect = (char)lua_tonumber(L, -1);
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
	lua_pushnil(L);
  while (lua_next(L, -2) != 0) {
		lua_pushnil(L);
		while (lua_next(L, -2) != 0) {
			if(i<14 && j <18){
				area [i][j] = (unsigned char)lua_tonumber(L, -1);
			}
			
			lua_pop(L, 1);
			j++;
		}
    j=0;
    lua_pop(L, 1);
    i++;
	}

	lua_pop(L, 1);
	memcpy(&magicInstant.area, area, sizeof(area));	

	Spell* spell = getSpell(L);
  magicInstant.manaCost = spell->getMana();
   
  Creature* creature = spell->game->getCreatureByID((unsigned long)lua_tonumber(L, -1));
  lua_pop(L,1);
	 
	if(needDirection){
		switch(creature->getDirection()) {
			case NORTH: magicInstant.direction = 1; break;
			case WEST: magicInstant.direction = 2; break;
			case EAST: magicInstant.direction = 3; break;
			case SOUTH: magicInstant.direction = 4; break;
		 };
	}
	else {
		magicInstant.direction = 1;
	}

	bool isSuccess = spell->game->creatureCastSpell(creature, centerpos, magicInstant);
	lua_pushboolean(L, isSuccess);
	return 1;
}
*/

int SpellScript::luaActionChangeOutfit(lua_State *L){
    int looktype = (int)lua_tonumber(L, -1);
	lua_pop(L,1);
	long time = (long)lua_tonumber(L, -1)*1000;
	lua_pop(L,1);
	
	Spell* spell = getSpell(L);
	Creature* creature = spell->game->getCreatureByID((unsigned long)lua_tonumber(L, -1));
	lua_pop(L,1);
	
	creature->looktype = looktype;
	spell->game->creatureChangeOutfit(creature);
	
  spell->game->changeOutfitAfter(creature->getID(), creature->lookmaster, time);
	return 0;
}

void SpellScript::internalLoadDamageVec(lua_State *L, MagicDamageVec& dmgvec)
{
	damageTimeCount dt;

	//cid
	unsigned long cid = (int)lua_tonumber(L, 1);

	MagicEffectTargetMagicDamageClass tmd(cid);
	
	internalGetMagicEffect(L, tmd);

	//damageTimeCount
	dt.second = (int)lua_tonumber(L, -1); //damageCount
	lua_pop(L, 1);

	dt.first = (int)lua_tonumber(L, -1); //delayTicks
	lua_pop(L, 1);

	dmgvec.insert(dmgvec.begin(), damageInfo(dt, tmd));
}

void SpellScript::internalLoadTransformVec(lua_State *L, damageMapClass& dmgMap)
{
	transformInfo ti;
	MagicDamageVec dmgvec;

	int stateCount = (int)lua_tonumber(L, -1);
	lua_pop(L,1);

	for(int s = 0; s < stateCount; ++s) {
		int id = (int)lua_tonumber(L, -1);
		lua_pop(L,1);
		
		ti.first = (int)lua_tonumber(L, -1);
		lua_pop(L,1);

		int count = (int)lua_tonumber(L, -1);
		lua_pop(L,1);

		for(int n = 0; n < count; ++n) {
			internalLoadDamageVec(L, dmgvec);
		}
		
		ti.second = dmgvec;
		dmgMap[id] = ti;
	}
}

int SpellScript::luaActionManaShield(lua_State *L){
	long time = (long)lua_tonumber(L, -1)*1000;
	lua_pop(L,1);
	
	Spell* spell = getSpell(L);
	Creature* creature = spell->game->getCreatureByID((unsigned long)lua_tonumber(L, -1));
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
	Creature* creature = spell->game->getCreatureByID((unsigned long)lua_tonumber(L, -1));
	lua_pop(L,1);
	
	spell->game->addEvent(makeTask(time, boost::bind(&Game::changeSpeed, spell->game,creature->getID(), creature->getNormalSpeed()) ) );
	Player* p = dynamic_cast<Player*>(creature);
	if(p){
         spell->game->changeSpeed(creature->getID(), creature->getNormalSpeed()+speed); 
	     p->sendIcons();
      }
    creature->hasteTicks = time;  
	return 0;
}

int SpellScript::luaActionGetSpeed(lua_State *L){
	Spell* spell = getSpell(L);
	Creature* creature = spell->game->getCreatureByID((unsigned long)lua_tonumber(L, -1));
	lua_pop(L,1);
	
	lua_pushnumber(L, creature->getNormalSpeed());
	return 1;
}

int SpellScript::luaActionGetPos(lua_State *L){
	const char* s = lua_tostring(L, -1);
	lua_pop(L,1);
	Spell* spell = getSpell(L);
	Creature* c = spell->game->getCreatureByName(s);
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

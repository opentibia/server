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

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>



#include "npc.h"


Npc::Npc(const char *name, Map* map) : Creature(name)
{
	std::string filename = "data/npc/" + std::string(name) + ".xml";
	std::transform(filename.begin(), filename.end(), filename.begin(), tolower);
	xmlDocPtr doc = xmlParseFile(filename.c_str());
	if (doc){
		xmlNodePtr root, p;
		root = xmlDocGetRootElement(doc);

		if (xmlStrcmp(root->name,(const xmlChar*) "npc")){
		//TODO: use exceptions here
		std::cerr << "Malformed XML" << std::endl;
		}

		p = root->children;

		this->scriptname = (const char*)xmlGetProp(root, (const xmlChar *)"script");

		if ((const char*)xmlGetProp(root, (const xmlChar *)"name")) {
			this->name = (const char*)xmlGetProp(root, (const xmlChar *)"name");
		}
		if ((const char*)xmlGetProp(root, (const xmlChar *)"access")) {
			access = atoi((const char*)xmlGetProp(root, (const xmlChar *)"access"));
		}
		if ((const char*)xmlGetProp(root, (const xmlChar *)"level")) {
			level = atoi((const char*)xmlGetProp(root, (const xmlChar *)"level"));
			std::cout << level << std::endl;
			maglevel = atoi((const char*)xmlGetProp(root, (const xmlChar *)"maglevel"));
			std::cout << maglevel << std::endl;
		}

		while (p)
		{
			const char* str = (char*)p->name;
			if (strcmp(str, "mana") == 0){
				this->mana = atoi((const char*)xmlGetProp(p, (const xmlChar *)"now"));
				this->manamax = atoi((const char*)xmlGetProp(p, (const xmlChar *)"max"));
			}
			if (strcmp(str, "health") == 0){
				this->health = atoi((const char*)xmlGetProp(p, (const xmlChar *)"now"));
				this->healthmax = atoi((const char*)xmlGetProp(p, (const xmlChar *)"max"));
			}
			if (strcmp(str, "look") == 0){
				this->looktype = atoi((const char*)xmlGetProp(p, (const xmlChar *)"type"));
				this->lookhead = atoi((const char*)xmlGetProp(p, (const xmlChar *)"head"));
				this->lookbody = atoi((const char*)xmlGetProp(p, (const xmlChar *)"body"));
				this->looklegs = atoi((const char*)xmlGetProp(p, (const xmlChar *)"legs"));
				this->lookfeet = atoi((const char*)xmlGetProp(p, (const xmlChar *)"feet"));
				this->lookcorpse = atoi((const char*)xmlGetProp(p, (const xmlChar *)"corpse"));
			}
			if (strcmp(str, "attack") == 0){
				std::string attacktype = (const char*)xmlGetProp(p, (const xmlChar *)"type");
				if(attacktype == "melee")
					this->fighttype = FIGHT_MELEE;
				this->damage = atoi((const char*)xmlGetProp(p, (const xmlChar *)"damage"));
			}
			if (strcmp(str, "loot") == 0){
				//TODO implement loot
			}
				
			p = p->next;
		}

		xmlFreeDoc(doc);
	}
	//now try to load the script
	this->script = new NpcScript(this->scriptname, this);
	this->map=map;
}


Npc::~Npc()
{
}

// The following things should be turned over to lua scripts which will allow more
// complex reactions of NPCs to player actions
void Npc::onThingMove(const Player *player, const Thing *thing, const Position *oldPos, unsigned char oldstackpos){
}

void Npc::onCreatureAppear(const Creature *creature){
	//if we arent attacking anybody AND the creature coming in range is enemy, attack it

	//else ignore
}

void Npc::onCreatureDisappear(const Creature *creature, unsigned char stackPos){
	//if we were attacking the creature, find a new enemy

	//else ignore
}

void Npc::onCreatureTurn(const Creature *creature, unsigned char stackpos){
	//we dont care about turning, we are an evil bad monster!
}

void Npc::onCreatureSay(const Creature *creature, unsigned char type, const std::string &text){
	if(creature->getID() == this->getID())
		return;
	this->script->onCreatureSay(creature->getID(), type, text);
}

void Npc::onCreatureChangeOutfit(const Creature* creature){
	#ifdef __DEBUG_NPC__
		std::cout << "Npc::onCreatureChangeOutfit" << std::endl;
	#endif
	//we dont care about filthy player changing his ugly clothes
}

void Npc::onThink(){
	this->script->onThink();
}


void Npc::doSay(std::string msg){
	map->creatureCastSpell(this, msg);
	this->map->creatureSay(this, 1, msg);
}

void Npc::doAttack(int id){
	attackedCreature = id;
}

void Npc::doMove(int direction){
	switch(direction){
		case 0:
			this->map->thingMove(this, this,this->pos.x, this->pos.y+1, this->pos.z);
		break;
		case 1:
			this->map->thingMove(this, this,this->pos.x+1, this->pos.y, this->pos.z);
		break;
		case 2:
			this->map->thingMove(this, this,this->pos.x, this->pos.y-1, this->pos.z);
		break;
		case 3:
			this->map->thingMove(this, this,this->pos.x-1, this->pos.y, this->pos.z);
		break;
	}
}

NpcScript::NpcScript(std::string scriptname, Npc* npc){
	luaState = lua_open();
	luaopen_loadlib(luaState);
	luaopen_base(luaState);
	luaopen_math(luaState);
	luaopen_string(luaState);
	luaopen_io(luaState);
	lua_dofile(luaState, "data/npc/scripts/lib/npc.lua");
	lua_dofile(luaState, scriptname.c_str());
	this->npc=npc;
	this->setGlobalNumber("addressOfNpc", (int) npc);
	this->registerFunctions();
}

void NpcScript::onThink(){
	lua_pushstring(luaState, "onThink");
	lua_gettable(luaState, LUA_GLOBALSINDEX);
	lua_call(luaState, 0,0);
}

void NpcScript::onCreatureSay(int cid, unsigned char type, const std::string &text){
	//now we need to call the function
	lua_pushstring(luaState, "onCreatureSay");
	lua_gettable(luaState, LUA_GLOBALSINDEX);
	lua_pushnumber(luaState, cid);
	lua_pushnumber(luaState, type);
	lua_pushstring(luaState, text.c_str());
	lua_call(luaState, 3,0);
}

int NpcScript::registerFunctions(){
	lua_register(luaState, "selfSay", NpcScript::luaActionSay);
	lua_register(luaState, "selfMove", NpcScript::luaActionMove);
	lua_register(luaState, "selfGetPosition", NpcScript::luaSelfGetPos);
	lua_register(luaState, "selfAttackCreature", NpcScript::luaActionAttackCreature);
	lua_register(luaState, "creatureGetName", NpcScript::luaCreatureGetName);
	lua_register(luaState, "creatureGetPosition", NpcScript::luaCreatureGetPos);
	lua_register(luaState, "selfGetPosition", NpcScript::luaSelfGetPos);
	
	return true;
}

Npc* NpcScript::getNpc(lua_State *L){
	lua_getglobal(L, "addressOfNpc");
	int val = (int)lua_tonumber(L, -1);
	lua_pop(L,1);

	Npc* mynpc = (Npc*) val;
	if(!mynpc){
		return 0;
	}
	return mynpc;
}

int NpcScript::luaCreatureGetName(lua_State *L){
	int id = (int)lua_tonumber(L, -1);
	lua_pop(L,1);
	Npc* mynpc = getNpc(L);
	lua_pushstring(L, mynpc->map->getCreatureByID(id)->getName().c_str());
	return 1;
}

int NpcScript::luaCreatureGetPos(lua_State *L){
	int id = (int)lua_tonumber(L, -1);
	lua_pop(L,1);
	Npc* mynpc = getNpc(L);
	Creature* c = mynpc->map->getCreatureByID(id);
	
	if(!c){
		lua_pushnil(L);
		lua_pushnil(L);
		lua_pushnil(L);
	}
	else{
		lua_pushnumber(L, c->pos.x);
		lua_pushnumber(L, c->pos.y);
		lua_pushnumber(L, c->pos.z);
	}
	return 3;
}

int NpcScript::luaSelfGetPos(lua_State *L){
	lua_pop(L,1);
	Npc* mynpc = getNpc(L);
	lua_pushnumber(L, mynpc->pos.x);
	lua_pushnumber(L, mynpc->pos.y);
	lua_pushnumber(L, mynpc->pos.z);
	return 3;
}

int NpcScript::luaActionSay(lua_State* L){
	int len = lua_strlen(L, -1);
	std::string msg(lua_tostring(L, -1), len);
	lua_pop(L,1);
	//now, we got the message, we now have to find out
	//what npc this belongs to

	Npc* mynpc=getNpc(L);
	if(mynpc)
		mynpc->doSay(msg);
	return 0;
}

int NpcScript::luaActionMove(lua_State* L){
	int dir=(int)lua_tonumber(L, -1);
	lua_pop(L,1);
	Npc* mynpc=getNpc(L);
	if(mynpc)
		mynpc->doMove(dir);
	return 0;
}

int NpcScript::luaActionAttackCreature(lua_State *L){
	int id=(int)lua_tonumber(L, -1);
	lua_pop(L,1);
	Npc* mynpc=getNpc(L);
	if(mynpc)
		mynpc->doAttack(id);
	return 0;
}

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
#include <sstream>
#include <fstream>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include "npc.h"
#include "game.h"

#include "luascript.h"

extern LuaScript g_config;

AutoList<Npc> Npc::listNpc;

Npc::Npc(const std::string& name, Game* game) :
 Creature()
{
	char *tmp;
	this->loaded = false;
	this->name = name;
	std::string datadir = g_config.getGlobalString("datadir");
	std::string filename = datadir + "npc/" + std::string(name) + ".xml";
	std::transform(filename.begin(), filename.end(), filename.begin(), tolower);
	xmlDocPtr doc = xmlParseFile(filename.c_str());
	if(doc){
		this->loaded=true;
		xmlNodePtr root, p;
		root = xmlDocGetRootElement(doc);

		if (xmlStrcmp(root->name,(const xmlChar*) "npc")){
		//TODO: use exceptions here
		std::cerr << "Malformed XML" << std::endl;
		}

		p = root->children;
		
		tmp = (char*)xmlGetProp(root, (const xmlChar *)"script");
		if(tmp){
			this->scriptname = tmp;
			xmlFreeOTSERV(tmp);
		}
		else{
			this->scriptname = "";
		}
		
		if(tmp = (char*)xmlGetProp(root, (const xmlChar *)"name")) {
			this->name = tmp;
			xmlFreeOTSERV(tmp);
		}
		else{
			this->name = "";
		}
		
		if(tmp = (char*)xmlGetProp(root, (const xmlChar *)"access")) {
			access = atoi(tmp);
			xmlFreeOTSERV(tmp);
		}
		else{
			access = 0;
		}
		
		if(tmp = (char*)xmlGetProp(root, (const xmlChar *)"level")) {
			level = atoi(tmp);
			xmlFreeOTSERV(tmp);
			setNormalSpeed();
			//std::cout << level << std::endl;
		}
		else{
			level = 1;
		}
		
		if(tmp = (char*)xmlGetProp(root, (const xmlChar *)"maglevel")) {
			maglevel = atoi(tmp);
			xmlFreeOTSERV(tmp);
			//std::cout << maglevel << std::endl;
		}
		else{
			maglevel = 1;
		}
		
		while (p)
		{
			const char* str = (char*)p->name;
			if(strcmp(str, "health") == 0){
				if(tmp = (char*)xmlGetProp(p, (const xmlChar *)"now")) {
					this->health = atoi(tmp);
					xmlFreeOTSERV(tmp);
				}
				else{
					this->health = 100;
				}
				if(tmp = (char*)xmlGetProp(p, (const xmlChar *)"max")) {
					this->healthmax = atoi(tmp);
					xmlFreeOTSERV(tmp);
				}
				else{
					this->healthmax = 100;
				}
			}
			if(strcmp(str, "look") == 0){
				if(tmp = (char*)xmlGetProp(p, (const xmlChar *)"type")) {
					this->looktype = atoi(tmp);
					xmlFreeOTSERV(tmp);
				}
				else{
					this->looktype = 20;
				}
				this->lookmaster = this->looktype;
				if(tmp = (char*)xmlGetProp(p, (const xmlChar *)"head")) {
					this->lookhead = atoi(tmp);
					xmlFreeOTSERV(tmp);
				}
				else{
					this->lookhead = 10;
				}
				
				if(tmp = (char*)xmlGetProp(p, (const xmlChar *)"body")) {
					this->lookbody = atoi(tmp);
					xmlFreeOTSERV(tmp);
				}
				else{
					this->lookbody = 20;
				}
				
				if(tmp = (char*)xmlGetProp(p, (const xmlChar *)"legs")) {
					this->looklegs = atoi(tmp);
					xmlFreeOTSERV(tmp);
				}
				else{
					this->looklegs = 30;
				}
				
				if(tmp = (char*)xmlGetProp(p, (const xmlChar *)"feet")) {
					this->lookfeet = atoi(tmp);
					xmlFreeOTSERV(tmp);
				}
				else{
					this->lookfeet = 40;
				}
				if(tmp = (char*)xmlGetProp(p, (const xmlChar *)"corpse")) {
					this->lookcorpse = atoi(tmp);
					xmlFreeOTSERV(tmp);
				}
				else{
					this->lookcorpse = 100;
				}
				
			}
			/*
			if(strcmp(str, "attack") == 0){
				if(tmp = (char*)xmlGetProp(p, (const xmlChar *)"type")){
					std::string attacktype = tmp;
					xmlFreeOTSERV(tmp);
					if(attacktype == "melee")
						this->fighttype = FIGHT_MELEE;
					tmp = (char*)xmlGetProp(p, (const xmlChar *)"damage");
					if(tmp){
						this->damage = atoi(tmp);
						xmlFreeOTSERV(tmp);
					}
					else{
						this->damage = 5;
					}
				}
				else{
					this->fighttype = FIGHT_MELEE;
					this->damage = 0;
				}
			}
			if(strcmp(str, "loot") == 0){
				//TODO implement loot
			}
			*/
				
			p = p->next;
		}

		xmlFreeDoc(doc);
	}
	//now try to load the script
	this->script = new NpcScript(this->scriptname, this);
	if(!this->script->isLoaded())
		this->loaded=false;
	this->game=game;
}


Npc::~Npc()
{
	delete this->script;
}

std::string Npc::getDescription(int32_t lookDistance) const
{
	std::stringstream s;
	s << name << ".";
	return s.str();
}

void Npc::onAddTileItem(const Position& pos, const Item* item)
{
	//not implemented yet
}

void Npc::onUpdateTileItem(const Position& pos, uint32_t stackpos, const Item* oldItem, const Item* newItem)
{
	//not implemented yet
}

void Npc::onRemoveTileItem(const Position& pos, uint32_t stackpos, const Item* item)
{
	//not implemented yet
}

void Npc::onUpdateTile(const Position& pos)
{
	//not implemented yet
}

void Npc::onCreatureAppear(const Creature* creature, bool isLogin)
{
	script->onCreatureAppear(creature->getID());
}

void Npc::onCreatureDisappear(const Creature* creature, uint32_t stackpos, bool isLogout)
{
	script->onCreatureDisappear(creature->getID());
}

void Npc::onCreatureMove(const Creature* creature, const Position& oldPos, uint32_t oldStackPos)
{
	//not implemented yet
}

void Npc::onTeleport(const Creature* creature, const Position& oldPos, uint32_t oldStackPos)
{
	//not implemented yet
}

void Npc::onCreatureTurn(const Creature* creature, uint32_t stackpos)
{
	//not implemented yet, do we need it?
}

void Npc::onCreatureSay(const Creature *creature, SpeakClasses type, const std::string &text)
{
	if(creature->getID() == this->getID())
		return;
	this->script->onCreatureSay(creature->getID(), type, text);
}

void Npc::onCreatureChangeOutfit(const Creature* creature)
{
	#ifdef __DEBUG_NPC__
		std::cout << "Npc::onCreatureChangeOutfit" << std::endl;
	#endif
	//we dont care about filthy player changing his ugly clothes
}

int Npc::onThink(int& newThinkTicks)
{
	this->script->onThink();
	return Creature::onThink(newThinkTicks);
}


void Npc::doSay(std::string msg)
{
	if(!game->creatureSaySpell(this, msg))
		this->game->creatureSay(this, SPEAK_SAY, msg);
}

void Npc::doMove(int direction)
{
	switch(direction){
		case 0:
			game->moveCreature(this, SOUTH);
		break;
		case 1:
			game->moveCreature(this, EAST);
		break;
		case 2:
			game->moveCreature(this, NORTH);
		break;
		case 3:
			game->moveCreature(this, WEST);
		break;
	}
}

void Npc::doMoveTo(Position target)
{
	if(route.size() == 0 || route.back() != target || route.front() != getPosition()){
		route = this->game->getPathTo(this, getPosition(), target);
	}
	if(route.size()==0){
		//still no route, means there is none
		return;
	}
	else route.pop_front();
	Position nextStep = route.front();
	route.pop_front();
	int dx = nextStep.x - getPosition().x;
	int dy = nextStep.y - getPosition().y;

	//this->game->thingMove(this, this, getPosition().x + dx, getPosition().y + dy, getPosition().z, 1);
	//game->moveCreature(this, 
}

NpcScript::NpcScript(std::string scriptname, Npc* npc)
{
	this->loaded = false;
	if(scriptname == "")
		return;
	luaState = lua_open();
	luaopen_loadlib(luaState);
	luaopen_base(luaState);
	luaopen_math(luaState);
	luaopen_string(luaState);
	luaopen_io(luaState);
	
	std::string datadir = g_config.getGlobalString("datadir");
    lua_dofile(luaState, std::string(datadir + "npc/scripts/lib/npc.lua").c_str());
	
	FILE* in=fopen(scriptname.c_str(), "r");
	if(!in)
		return;
	else
		fclose(in);
	lua_dofile(luaState, scriptname.c_str());
	this->loaded=true;
	this->npc=npc;
	this->setGlobalNumber("addressOfNpc", (int) npc);
	this->registerFunctions();
}

void NpcScript::onThink()
{
	lua_pushstring(luaState, "onThink");
	lua_gettable(luaState, LUA_GLOBALSINDEX);
	if(lua_pcall(luaState, 0, 0, 0)){
		std::cerr << "Lua error: " << lua_tostring(luaState, -1) << std::endl;
		lua_pop(luaState,1);
		std::cerr << "Backtrace: " << std::endl;
		lua_Debug* d = NULL;
		int i = 0;
		while(lua_getstack(luaState, i++, d)){
			std::cerr << "    " << d->name << " @ " << d->currentline << std::endl;
		}
	}
}

void NpcScript::onCreatureAppear(unsigned long cid)
{
	if(npc->getID() != cid){
		lua_pushstring(luaState, "onCreatureAppear");
		lua_gettable(luaState, LUA_GLOBALSINDEX);
		lua_pushnumber(luaState, cid);
		if(lua_pcall(luaState, 1, 0, 0)){
			std::cerr << "Lua error: " << lua_tostring(luaState, -1) << std::endl;
			lua_pop(luaState,1);
			std::cerr << "Backtrace: " << std::endl;
			lua_Debug* d = NULL;
			int i = 0;
			while(lua_getstack(luaState, i++, d)){
				std::cerr << "    " << d->name << " @ " << d->currentline << std::endl;
			}
		}
	}
}

void NpcScript::onCreatureDisappear(int cid)
{
	lua_pushstring(luaState, "onCreatureDisappear");
	lua_gettable(luaState, LUA_GLOBALSINDEX);
	lua_pushnumber(luaState, cid);
	if(lua_pcall(luaState, 1, 0, 0)){
		std::cerr << "Lua error: " << lua_tostring(luaState, -1) << std::endl;
		lua_pop(luaState,1);
		std::cerr << "Backtrace: " << std::endl;
		lua_Debug* d = NULL;
		int i = 0;
		while(lua_getstack(luaState, i++, d)){
			std::cerr << "    " << d->name << " @ " << d->currentline << std::endl;
		}
	}
}

void NpcScript::onCreatureSay(int cid, SpeakClasses type, const std::string &text)
{
	//now we need to call the function
	lua_pushstring(luaState, "onCreatureSay");
	lua_gettable(luaState, LUA_GLOBALSINDEX);
	lua_pushnumber(luaState, cid);
	lua_pushnumber(luaState, type);
	lua_pushstring(luaState, text.c_str());
	if(lua_pcall(luaState, 3, 0, 0)){
		std::cerr << "Lua error: " << lua_tostring(luaState, -1) << std::endl;
		lua_pop(luaState,1);
		std::cerr << "Backtrace: " << std::endl;
		lua_Debug* d = NULL;
		int i = 0;
		while(lua_getstack(luaState, i++, d)){
			std::cerr << "    " << d->name << " @ " << d->currentline << std::endl;
		}
	}
}

int NpcScript::registerFunctions()
{
	lua_register(luaState, "selfSay", NpcScript::luaActionSay);
	lua_register(luaState, "selfMove", NpcScript::luaActionMove);
	lua_register(luaState, "selfMoveTo", NpcScript::luaActionMoveTo);
	lua_register(luaState, "selfGetPosition", NpcScript::luaSelfGetPos);
	lua_register(luaState, "creatureGetName", NpcScript::luaCreatureGetName);
	lua_register(luaState, "creatureGetName2", NpcScript::luaCreatureGetName2);
	lua_register(luaState, "creatureGetPosition", NpcScript::luaCreatureGetPos);
	lua_register(luaState, "selfGetPosition", NpcScript::luaSelfGetPos);
	
	return true;
}

Npc* NpcScript::getNpc(lua_State *L)
{
	lua_getglobal(L, "addressOfNpc");
	int val = (int)lua_tonumber(L, -1);
	lua_pop(L,1);

	Npc* mynpc = (Npc*) val;
	if(!mynpc){
		return 0;
	}
	return mynpc;
}

int NpcScript::luaCreatureGetName2(lua_State *L)
{
	const char* s = lua_tostring(L, -1);
	lua_pop(L,1);
	Npc* mynpc = getNpc(L);
	Creature *c = mynpc->game->getCreatureByName(std::string(s));
	
	if(c && c->access == 0) {
		lua_pushnumber(L, c->getID());
	}
	else
		lua_pushnumber(L, 0);

	return 1;
}

int NpcScript::luaCreatureGetName(lua_State *L)
{
	int id = (int)lua_tonumber(L, -1);
	lua_pop(L,1);
	Npc* mynpc = getNpc(L);
	lua_pushstring(L, mynpc->game->getCreatureByID(id)->getName().c_str());
	return 1;
}

int NpcScript::luaCreatureGetPos(lua_State *L)
{
	int id = (int)lua_tonumber(L, -1);
	lua_pop(L,1);
	Npc* mynpc = getNpc(L);
	Creature* c = mynpc->game->getCreatureByID(id);
	
	if(!c){
		lua_pushnil(L);
		lua_pushnil(L);
		lua_pushnil(L);
	}
	else{
		lua_pushnumber(L, c->getPosition().x);
		lua_pushnumber(L, c->getPosition().y);
		lua_pushnumber(L, c->getPosition().z);
	}
	return 3;
}

int NpcScript::luaSelfGetPos(lua_State *L)
{
	lua_pop(L,1);
	Npc* mynpc = getNpc(L);
	lua_pushnumber(L, mynpc->getPosition().x);
	lua_pushnumber(L, mynpc->getPosition().y);
	lua_pushnumber(L, mynpc->getPosition().z);
	return 3;
}

int NpcScript::luaActionSay(lua_State* L)
{
	int len = (uint32_t)lua_strlen(L, -1);
	std::string msg(lua_tostring(L, -1), len);
	lua_pop(L,1);
	//now, we got the message, we now have to find out
	//what npc this belongs to

	Npc* mynpc=getNpc(L);
	if(mynpc)
		mynpc->doSay(msg);
	return 0;
}

int NpcScript::luaActionMove(lua_State* L)
{
	int dir=(int)lua_tonumber(L, -1);
	lua_pop(L,1);
	Npc* mynpc=getNpc(L);
	if(mynpc)
		mynpc->doMove(dir);
	return 0;
}

int NpcScript::luaActionMoveTo(lua_State* L)
{
	Position target;
	target.z=(int)lua_tonumber(L, -1);
	lua_pop(L,1);
	target.y=(int)lua_tonumber(L, -1);
	lua_pop(L,1);
	target.x=(int)lua_tonumber(L, -1);
	lua_pop(L,1);
	Npc* mynpc=getNpc(L);
	if(mynpc)
		mynpc->doMoveTo(target);
	return 0;
}

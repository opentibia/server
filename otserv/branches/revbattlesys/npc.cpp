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
#include "npc.h"
#include "game.h"
#include "tools.h"

#include <algorithm>
#include <functional>
#include <string>
#include <sstream>
#include <fstream>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include "luascript.h"

extern LuaScript g_config;
extern Game g_game;

AutoList<Npc> Npc::listNpc;

NpcScriptInterface* Npc::m_scriptInterface = NULL;

Npc::Npc(const std::string& _name) :
Creature()
{
	if(!m_scriptInterface){
		 m_scriptInterface = new NpcScriptInterface();
	}
	loaded = false;
	name = _name;

	std::string datadir = g_config.getGlobalString("datadir");
	std::string filename = datadir + "npc/" + std::string(name) + ".xml";
	std::string scriptname;

	xmlDocPtr doc = xmlParseFile(filename.c_str());

	if(doc){
		loaded = true;
		xmlNodePtr root, p;
		root = xmlDocGetRootElement(doc);

		if(xmlStrcmp(root->name,(const xmlChar*)"npc") != 0){
			//TODO: use exceptions here
			std::cerr << "Malformed XML" << std::endl;
		}

		int intValue;
		std::string strValue;

		p = root->children;
		
		if(readXMLString(root, "script", strValue)){
			scriptname = strValue;
		}
		else
			scriptname = "";
		
		if(readXMLString(root, "name", strValue)){
			name = strValue;
		}
		else
			name = "";
		
		if(readXMLInteger(root, "speed", intValue)){
			baseSpeed = intValue;
		}
		else
			baseSpeed = 220;
		
		varSpeed = baseSpeed;

		while(p){
			if(xmlStrcmp(p->name, (const xmlChar*)"health") == 0){

				if(readXMLInteger(p, "now", intValue)){
					health = intValue;
				}
				else
					health = 100;

				if(readXMLInteger(p, "max", intValue)){
					healthMax = intValue;
				}
				else
					healthMax = 100;
			}

			if(xmlStrcmp(p->name, (const xmlChar*)"look") == 0){

				if(readXMLInteger(p, "type", intValue)){
					currentOutfit.lookType = intValue;
					defaultOutfit.lookType = intValue;
				}

				if(readXMLInteger(p, "head", intValue)){
					currentOutfit.lookHead = intValue;
					defaultOutfit.lookHead = intValue;
				}

				if(readXMLInteger(p, "body", intValue)){
					currentOutfit.lookBody = intValue;
					defaultOutfit.lookBody = intValue;
				}

				if(readXMLInteger(p, "legs", intValue)){
					currentOutfit.lookLegs = intValue;
					defaultOutfit.lookLegs = intValue;
				}
				
				if(readXMLInteger(p, "feet", intValue)){
					currentOutfit.lookFeet = intValue;
					defaultOutfit.lookFeet = intValue;
				}
			}

			p = p->next;
		}

		xmlFreeDoc(doc);
	}

	//now try to load the script
	if(scriptname != ""){
		m_npcEventHandler = new NpcScript(scriptname, this);
	}
	else{ //default npcs
	}

	if(!m_npcEventHandler->isLoaded())
		loaded = false;
}


Npc::~Npc()
{
	delete m_npcEventHandler;
}

bool Npc::canSee(const Position& pos) const
{
	const Position& myPos = getPosition();

	if(pos.z != myPos.z){
		return false;
	}

	return (std::abs(myPos.x - pos.x) <= Map::maxViewportX &&
					std::abs(myPos.y - pos.y) <= Map::maxViewportY);
}

std::string Npc::getDescription(int32_t lookDistance) const
{
	std::stringstream s;
	s << name << ".";
	return s.str();
}

void Npc::onAddTileItem(const Position& pos, const Item* item)
{
	Creature::onAddTileItem(pos, item);
}

void Npc::onUpdateTileItem(const Position& pos, uint32_t stackpos, const Item* oldItem, const Item* newItem)
{
	Creature::onUpdateTileItem(pos, stackpos, oldItem, newItem);
}

void Npc::onRemoveTileItem(const Position& pos, uint32_t stackpos, const Item* item)
{
	Creature::onRemoveTileItem(pos, stackpos, item);
}

void Npc::onUpdateTile(const Position& pos)
{
	Creature::onUpdateTile(pos);
}

void Npc::onCreatureAppear(const Creature* creature, bool isLogin)
{
	Creature::onCreatureAppear(creature, isLogin);
	m_npcEventHandler->onCreatureAppear(creature);
}

void Npc::onCreatureDisappear(const Creature* creature, uint32_t stackpos, bool isLogout)
{
	Creature::onCreatureDisappear(creature, stackpos, isLogout);
	m_npcEventHandler->onCreatureDisappear(creature);
}

void Npc::onCreatureMove(const Creature* creature, const Position& newPos, const Position& oldPos,
	uint32_t oldStackPos, bool teleport)
{
	Creature::onCreatureMove(creature, newPos, oldPos, oldStackPos, teleport);
}

void Npc::onCreatureTurn(const Creature* creature, uint32_t stackpos)
{
	Creature::onCreatureTurn(creature, stackpos);
}

void Npc::onCreatureSay(const Creature* creature, SpeakClasses type, const std::string& text)
{
	if(creature->getID() == this->getID())
		return;

	m_npcEventHandler->onCreatureSay(creature, type, text);
}

void Npc::onCreatureChangeOutfit(const Creature* creature)
{
	#ifdef __DEBUG_NPC__
		std::cout << "Npc::onCreatureChangeOutfit" << std::endl;
	#endif
	//we dont care about filthy player changing his ugly clothes
}

void Npc::onThink(uint32_t interval)
{
	Creature::onThink(interval);
	m_npcEventHandler->onThink();
}

void Npc::doSay(std::string msg)
{
	g_game.internalCreatureSay(this, SPEAK_SAY, msg);
}

void Npc::doMove(Direction dir)
{
	g_game.internalMoveCreature(this, dir);
}

void Npc::doMoveTo(Position target)
{
	std::list<Direction> listDir;
	if(!g_game.getPathToEx(this, target, 1, 1, true, listDir)){
		return;
	}

	startAutoWalk(listDir);
	
	/*
	if(route.size() == 0 || route.back() != target || route.front() != getPosition()){
		route = g_game.map->getPathTo(this, getPosition(), target);
	}

	if(route.size() == 0){
		//still no route, means there is none
		return;
	}
	else
		route.pop_front();

	Position nextStep = route.front();
	route.pop_front();
	int dx = nextStep.x - getPosition().x;
	int dy = nextStep.y - getPosition().y;

	Direction dir;

	if(dx == -1 && dy == -1)
		dir = NORTHWEST;
	else if(dx == 1 && dy == -1)
		dir = NORTHEAST;
	else if(dx == -1 && dy == 1)
		dir = SOUTHWEST;
	else if(dx == 1 && dy == 1)
		dir = SOUTHEAST;
	else if(dx == -1)
		dir = WEST;
	else if(dx == 1)
		dir = EAST;
	else if(dy == -1)
		dir = NORTH;
	else
		dir = SOUTH;

	g_game.internalMoveCreature(this, dir);
	*/
}

NpcScriptInterface* Npc::getScriptInterface()
{
	return m_scriptInterface;	
}

Npc* NpcScriptInterface::m_curNpc = NULL;

NpcScriptInterface::NpcScriptInterface() :
LuaScriptInterface("Npc interface")
{
	m_libLoaded = false;
	initState();
}


NpcScriptInterface::~NpcScriptInterface()
{
	//
}

bool NpcScriptInterface::initState()
{
	return LuaScriptInterface::initState();
}

bool NpcScriptInterface::closeState()
{
	m_libLoaded = false;
	return LuaScriptInterface::closeState();
}

bool NpcScriptInterface::loadNpcLib(std::string file)
{
	if(m_libLoaded)
		return true;
		
	if(loadFile(file) == -1){
		std::cout << "Warning: [NpcScriptInterface::loadNpcLib] Can not load actions " << file << std::endl;
		return false;
	}
		
	m_libLoaded = true;
	return true;
}

Npc* NpcScriptInterface::getNpc()
{
	return m_curNpc;
}

void NpcScriptInterface::setNpc(Npc* npc)
{
	m_curNpc = npc;
}

void NpcScriptInterface::registerFunctions()
{
	LuaScriptInterface::registerFunctions();
	
	//npc exclusive functions
	lua_register(m_luaState, "selfSay", NpcScriptInterface::luaActionSay);
	lua_register(m_luaState, "selfMove", NpcScriptInterface::luaActionMove);
	lua_register(m_luaState, "selfMoveTo", NpcScriptInterface::luaActionMoveTo);
	lua_register(m_luaState, "selfGetPosition", NpcScriptInterface::luaSelfGetPos);
	lua_register(m_luaState, "creatureGetName", NpcScriptInterface::luaCreatureGetName);
	lua_register(m_luaState, "creatureGetName2", NpcScriptInterface::luaCreatureGetName2);
	lua_register(m_luaState, "creatureGetPosition", NpcScriptInterface::luaCreatureGetPos);
	lua_register(m_luaState, "getDistanceTo", NpcScriptInterface::luagetDistanceTo);
}


int NpcScriptInterface::luaCreatureGetName2(lua_State *L)
{
	const char* s = popString(L);

	Creature* creature = g_game.getCreatureByName(std::string(s));	
	if(creature){
		lua_pushnumber(L, creature->getID());
	}
	else{
		lua_pushnumber(L, 0);
	}

	return 1;
}

int NpcScriptInterface::luaCreatureGetName(lua_State *L)
{
	long uid = (long)popNumber(L);
	ScriptEnviroment* env = getScriptEnv();
	Creature* creature = env->getCreatureByUID(uid);
	
	if(creature){
		lua_pushstring(L, creature->getName().c_str());
	}
	else{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushstring(L, "");
	}
	
	return 1;
}

int NpcScriptInterface::luaCreatureGetPos(lua_State *L)
{
	long uid = (long)popNumber(L);
	ScriptEnviroment* env = getScriptEnv();
	Creature* creature = env->getCreatureByUID(uid);
	
	if(creature){
		Position pos = creature->getPosition();
		lua_pushnumber(L, pos.x);
		lua_pushnumber(L, pos.y);
		lua_pushnumber(L, pos.z);
	}
	else{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushnil(L);
		lua_pushnil(L);
		lua_pushnil(L);
	}
	return 3;
}

int NpcScriptInterface::luaSelfGetPos(lua_State *L)
{
	Npc* mynpc = getNpc();
	if(mynpc){
		Position pos = mynpc->getPosition();
		lua_pushnumber(L, pos.x);
		lua_pushnumber(L, pos.y);
		lua_pushnumber(L, pos.z);
	}
	else{
		lua_pushnil(L);
		lua_pushnil(L);
		lua_pushnil(L);
	}
	
	return 3;
}

int NpcScriptInterface::luaActionSay(lua_State* L)
{
	std::string msg(popString(L));

	Npc* mynpc = getNpc();
	if(mynpc)
		mynpc->doSay(msg);
	return 0;
}

int NpcScriptInterface::luaActionMove(lua_State* L)
{
	Direction dir = (Direction)(int)popNumber(L);

	Npc* mynpc = getNpc();
	if(mynpc)
		mynpc->doMove(dir);

	return 0;
}

int NpcScriptInterface::luaActionMoveTo(lua_State* L)
{
	Position target;
	target.z = (int)popNumber(L);
	target.y = (int)popNumber(L);
	target.x = (int)popNumber(L);
	Npc* mynpc = getNpc();
	if(mynpc)
		mynpc->doMoveTo(target);
	return 0;
}

int NpcScriptInterface::luagetDistanceTo(lua_State *L)
{
	long uid = (long)popNumber(L);
	Npc* mynpc = getNpc();
	
	ScriptEnviroment* env = getScriptEnv();
	
	Thing* thing = env->getThingByUID(uid);
	if(thing && mynpc){
		Position thing_pos = thing->getPosition();
		Position npc_pos = mynpc->getPosition();
		if(npc_pos.z != thing_pos.z){
			lua_pushnumber(L, -1);
		}
		else{
			long dist = std::max(std::abs(npc_pos.x - thing_pos.x), std::abs(npc_pos.y - thing_pos.y));
			lua_pushnumber(L, dist);
		}	
	}
	else{
		reportErrorFunc(getErrorDesc(LUA_ERROR_THING_NOT_FOUND));
		lua_pushnil(L);
		return 1;
	}
	
	return 1;
}

NpcEventsHandler::NpcEventsHandler(Npc* npc)
{
	m_npc = npc;
	m_loaded = false;
}

NpcEventsHandler::~NpcEventsHandler()
{
	//
}

bool NpcEventsHandler::isLoaded()
{
	return m_loaded;
}


NpcScript::NpcScript(std::string file, Npc* npc) :
NpcEventsHandler(npc)
{
	m_scriptInterface = npc->getScriptInterface();
	//load npc libs
	std::string datadir = g_config.getGlobalString("datadir");
	if(m_scriptInterface->loadFile(std::string(datadir + "npc/scripts/lib/npc.lua")) == -1){
		std::cout << "Warning: [NpcScript::NpcScript] Can not load npc/scripts/lib/npc.lua" << std::endl;
	}
	
	if(m_scriptInterface->loadFile(file) == -1){
		std::cout << "Warning: [NpcScript::NpcScript] Can not load script. " << file << std::endl;
		m_loaded = false;
		return;
	}
	
	m_onCreatureSay = m_scriptInterface->getEvent("onCreatureSay");
	m_onCreatureDisappear = m_scriptInterface->getEvent("onCreatureDisappear");
	m_onCreatureAppear = m_scriptInterface->getEvent("onCreatureAppear");
	m_onThink = m_scriptInterface->getEvent("onThink");
	m_loaded = true;
}

NpcScript::~NpcScript()
{
	//
}
	
void NpcScript::onCreatureAppear(const Creature* creature)
{
	if(m_onCreatureAppear == -1){
		return;	
	}
	
	ScriptEnviroment* env = m_scriptInterface->getScriptEnv();
	
	#ifdef __DEBUG_LUASCRIPTS__
	std::stringstream desc;
	desc << "npc " << m_npc->getName();
	env->setEventDesc(desc.str());
	#endif
	
	lua_State* L = m_scriptInterface->getLuaState();
	
	env->setScriptId(m_onCreatureAppear, m_scriptInterface);
	env->setRealPos(m_npc->getPosition());
	m_scriptInterface->setNpc(m_npc);
	
	long cid = env->addThing(const_cast<Creature*>(creature));
	
	m_scriptInterface->pushFunction(m_onCreatureAppear);
	lua_pushnumber(L, cid);	
	m_scriptInterface->callFunction(1);
}

void NpcScript::onCreatureDisappear(const Creature* creature)
{
	if(m_onCreatureDisappear == -1){
		return;	
	}
	
	ScriptEnviroment* env = m_scriptInterface->getScriptEnv();
	
	#ifdef __DEBUG_LUASCRIPTS__
	std::stringstream desc;
	desc << "npc " << m_npc->getName();
	env->setEventDesc(desc.str());
	#endif
	
	lua_State* L = m_scriptInterface->getLuaState();
	
	env->setScriptId(m_onCreatureDisappear, m_scriptInterface);
	env->setRealPos(m_npc->getPosition());
	m_scriptInterface->setNpc(m_npc);
	
	long cid = env->addThing(const_cast<Creature*>(creature));
	
	m_scriptInterface->pushFunction(m_onCreatureDisappear);
	lua_pushnumber(L, cid);	
	m_scriptInterface->callFunction(1);
}

void NpcScript::onCreatureSay(const Creature* creature, SpeakClasses type, const std::string& text)
{
	if(m_onCreatureSay == -1){
		return;	
	}
	
	ScriptEnviroment* env = m_scriptInterface->getScriptEnv();
	
	#ifdef __DEBUG_LUASCRIPTS__
	std::stringstream desc;
	desc << "npc " << m_npc->getName();
	env->setEventDesc(desc.str());
	#endif
	
	env->setScriptId(m_onCreatureSay, m_scriptInterface);
	env->setRealPos(m_npc->getPosition());
	m_scriptInterface->setNpc(m_npc);
	
	long cid = env->addThing(const_cast<Creature*>(creature));
	
	lua_State* L = m_scriptInterface->getLuaState();
	m_scriptInterface->pushFunction(m_onCreatureSay);
	lua_pushnumber(L, cid);
	lua_pushnumber(L, type);
	lua_pushstring(L, text.c_str());
	m_scriptInterface->callFunction(3);
}

void NpcScript::onThink()
{
	if(m_onThink == -1){
		return;	
	}
	
	ScriptEnviroment* env = m_scriptInterface->getScriptEnv();
	
	#ifdef __DEBUG_LUASCRIPTS__
	std::stringstream desc;
	desc << "npc " << m_npc->getName();
	env->setEventDesc(desc.str());
	#endif
	
	env->setScriptId(m_onThink, m_scriptInterface);
	env->setRealPos(m_npc->getPosition());
	m_scriptInterface->setNpc(m_npc);
	
	m_scriptInterface->pushFunction(m_onThink);
	m_scriptInterface->callFunction(0);
}

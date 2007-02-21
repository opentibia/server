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
#include "otpch.h"

#include "definitions.h"
#include "npc.h"
#include "game.h"
#include "tools.h"
#include "configmanager.h"
#include "position.h"

#include <algorithm>
#include <functional>
#include <string>
#include <sstream>
#include <fstream>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include "luascript.h"

extern ConfigManager g_config;
extern Game g_game;

AutoList<Npc> Npc::listNpc;

NpcScriptInterface* Npc::m_scriptInterface = NULL;

Npc::Npc(const std::string& _name) :
Creature()
{
	std::string datadir = g_config.getString(ConfigManager::DATA_DIRECTORY);
	if(!m_scriptInterface){
		m_scriptInterface = new NpcScriptInterface();
		m_scriptInterface->loadNpcLib(std::string(datadir + "npc/scripts/lib/npc.lua"));
	}
	
	m_npcEventHandler = NULL;
	loaded = true;
	name = _name;
	autoWalk = false;
	floorChange = false;
	focusCreature = 0;

	std::string filename = datadir + "npc/" + std::string(name) + ".xml";
	std::string scriptname;

	xmlDocPtr doc = xmlParseFile(filename.c_str());

	if(doc){
		xmlNodePtr root, p, q;
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

		if(readXMLInteger(root, "autowalk", intValue)){
			autoWalk = intValue != 0;
		}

		if(readXMLInteger(root, "floorchange", intValue)){
			floorChange = intValue != 0;
		}		

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
			else if(xmlStrcmp(p->name, (const xmlChar*)"look") == 0){

				if(readXMLInteger(p, "type", intValue)){
					defaultOutfit.lookType = intValue;

					if(readXMLInteger(p, "head", intValue)){
						defaultOutfit.lookHead = intValue;
					}

					if(readXMLInteger(p, "body", intValue)){
						defaultOutfit.lookBody = intValue;
					}

					if(readXMLInteger(p, "legs", intValue)){
						defaultOutfit.lookLegs = intValue;
					}

					if(readXMLInteger(p, "feet", intValue)){
						defaultOutfit.lookFeet = intValue;
					}
					
					if(readXMLInteger(p, "addons", intValue)){
						defaultOutfit.lookAddons = intValue;
					}					
				}
				else if(readXMLInteger(p, "typeex", intValue)){
					defaultOutfit.lookTypeEx = intValue;
				}

				currentOutfit = defaultOutfit;
			}
			else if(xmlStrcmp(p->name, (const xmlChar*)"parameters") == 0){
					
				for(q = p->children; q != NULL; q = q->next){
					if(xmlStrcmp(q->name, (const xmlChar*)"parameter") == 0){
						std::string paramKey;
						std::string paramValue;
						if(!readXMLString(q, "key", paramKey)){
							continue;
						}
						if(!readXMLString(q, "value", paramValue)){
							continue;
						}
						m_parameters[paramKey] = paramValue;
					}
				}
				
			}
			
			p = p->next;
		}

		xmlFreeDoc(doc);
		
		//now try to load the script
		if(scriptname != ""){
			m_npcEventHandler = new NpcScript(scriptname, this);
			if(!m_npcEventHandler->isLoaded()){
				loaded = false;
			}
		}
		else{ //default npcs
			//TODO
			loaded = false;
		}	
	}
	else{
		loaded = false;
	}
}


Npc::~Npc()
{
	delete m_npcEventHandler;
}

bool Npc::canSee(const Position& pos) const
{
	return Creature::canSee(pos);
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

	if(creature == this && autoWalk){
		addEventWalk();
	}
	
	//only players for script events
	if(creature->getPlayer()){
		m_npcEventHandler->onCreatureAppear(creature);
	}
}

void Npc::onCreatureDisappear(const Creature* creature, uint32_t stackpos, bool isLogout)
{
	Creature::onCreatureDisappear(creature, stackpos, isLogout);
	
	//only players for script events
	if(creature->getPlayer()){
		m_npcEventHandler->onCreatureDisappear(creature);
	}
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
	
	//only players for script events
	if(creature->getPlayer()){
		m_npcEventHandler->onCreatureSay(creature, type, text);
	}
}

void Npc::onCreatureChangeOutfit(const Creature* creature, const Outfit_t& outfit)
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

void Npc::doTurn(Direction dir)
{
	g_game.internalCreatureTurn(this, dir);
}

bool Npc::getNextStep(Direction& dir)
{
	if(Creature::getNextStep(dir)){
		return true;
	}

	if(autoWalk && focusCreature == 0){
		return getRandomStep(dir);
	}

	return false;
}

bool Npc::canWalkTo(const Position& fromPos, Direction dir)
{
	if(masterRadius == -1){
		//no restrictions
		return true;
	}

	Position toPos = fromPos;

	switch(dir){
		case NORTH:
			toPos.y -= 1;
		break;

		case SOUTH:
			toPos.y += 1;
		break;

		case WEST:
			toPos.x -= 1;
		break;

		case EAST:
			toPos.x += 1;
		break;

		default:
			break;
	}

	bool result = Spawns::getInstance()->isInZone(masterPos, masterRadius, toPos);
	if(!result){
		return false;
	}

	Tile* tile = g_game.getTile(toPos.x, toPos.y, toPos.z);
	if(!tile || tile->__queryAdd(0, this, 1, 0) != RET_NOERROR){
		return false;
	}

	if(!floorChange && (tile->floorChange() || tile->getTeleportItem())){
		return false;
	}

	return true;
}

bool Npc::getRandomStep(Direction& dir)
{
	std::vector<Direction> dirList;
	const Position& creaturePos = getPosition();

	if(canWalkTo(creaturePos, NORTH)){
		dirList.push_back(NORTH);
	}

	if(canWalkTo(creaturePos, SOUTH)){
		dirList.push_back(SOUTH);
	}

	if(canWalkTo(creaturePos, EAST)){
		dirList.push_back(EAST);
	}

	if(canWalkTo(creaturePos, WEST)){
		dirList.push_back(WEST);
	}

	if(!dirList.empty()){
		std::random_shuffle(dirList.begin(), dirList.end());
		dir = dirList[random_range(0, dirList.size() - 1)];
		return true;
	}

	return false;
}

void Npc::doMoveTo(Position target)
{
	std::list<Direction> listDir;
	if(!g_game.getPathToEx(this, target, 1, 1, true, true, listDir)){
		return;
	}

	startAutoWalk(listDir);
}

void Npc::setCreatureFocus(Creature* creature)
{
	if(creature){
		focusCreature = creature->getID();

		const Position& creaturePos = creature->getPosition();
		const Position& myPos = getPosition();
		int32_t dx = myPos.x - creaturePos.x;
		int32_t dy = myPos.y - creaturePos.y;

		Direction dir = SOUTH;
		float tan = 0;

		if(dx != 0){
			tan = dy/dx;
		}
		else{
			tan = 10;
		}

		if(std::abs(tan) < 1){
			if(dx > 0){
				dir = WEST;
			}
			else{
				dir = EAST;
			}
		}
		else{
			if(dy > 0){
				dir = NORTH;
			}
			else{
				dir = SOUTH;
			}
		}

		g_game.internalCreatureTurn(this, dir);
	}
	else{
		focusCreature = 0;
	}
}

NpcScriptInterface* Npc::getScriptInterface()
{
	return m_scriptInterface;	
}

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
		std::cout << "Warning: [NpcScriptInterface::loadNpcLib] Can not load " << file  << std::endl;
		return false;
	}
		
	m_libLoaded = true;
	return true;
}

void NpcScriptInterface::registerFunctions()
{
	LuaScriptInterface::registerFunctions();
	
	//npc exclusive functions
	lua_register(m_luaState, "selfSay", NpcScriptInterface::luaActionSay);
	lua_register(m_luaState, "selfMove", NpcScriptInterface::luaActionMove);
	lua_register(m_luaState, "selfMoveTo", NpcScriptInterface::luaActionMoveTo);
	lua_register(m_luaState, "selfTurn", NpcScriptInterface::luaActionTurn);
	lua_register(m_luaState, "selfGetPosition", NpcScriptInterface::luaSelfGetPos);
	lua_register(m_luaState, "creatureGetName", NpcScriptInterface::luaCreatureGetName);
	lua_register(m_luaState, "creatureGetName2", NpcScriptInterface::luaCreatureGetName2);
	lua_register(m_luaState, "creatureGetPosition", NpcScriptInterface::luaCreatureGetPos);
	lua_register(m_luaState, "getDistanceTo", NpcScriptInterface::luagetDistanceTo);
	lua_register(m_luaState, "doNpcSetCreatureFocus", NpcScriptInterface::luaSetNpcFocus);
	lua_register(m_luaState, "getNpcCid", NpcScriptInterface::luaGetNpcCid);
	lua_register(m_luaState, "getNpcName", NpcScriptInterface::luaGetNpcName);
	lua_register(m_luaState, "getNpcParameter", NpcScriptInterface::luaGetNpcParameter);
}


int NpcScriptInterface::luaCreatureGetName2(lua_State *L)
{
	//creatureGetName2(name) - returns creature id
	popString(L);
	reportErrorFunc("Deprecated function.");
	lua_pushnil(L);
	return 1;
}

int NpcScriptInterface::luaCreatureGetName(lua_State *L)
{
	//creatureGetName(cid)
	popNumber(L);
	reportErrorFunc("Deprecated function. Use getCreatureName");
	lua_pushstring(L, "");
	return 1;
}

int NpcScriptInterface::luaCreatureGetPos(lua_State *L)
{
	//creatureGetPosition(cid)
	popNumber(L);
	reportErrorFunc("Deprecated function. Use getCreaturePosition");
	lua_pushnil(L);
	lua_pushnil(L);
	lua_pushnil(L);
	return 3;
}

int NpcScriptInterface::luaSelfGetPos(lua_State *L)
{
	//selfGetPosition()
	ScriptEnviroment* env = getScriptEnv();
	Npc* mynpc = env->getNpc();
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
	//selfSay(words)
	std::string msg(popString(L));
	ScriptEnviroment* env = getScriptEnv();
	Npc* mynpc = env->getNpc();
	if(mynpc)
		mynpc->doSay(msg);
		
	return 0;
}

int NpcScriptInterface::luaActionMove(lua_State* L)
{
	//selfMove(direction)
	Direction dir = (Direction)popNumber(L);
	ScriptEnviroment* env = getScriptEnv();
	Npc* mynpc = env->getNpc();
	if(mynpc)
		mynpc->doMove(dir);

	return 0;
}

int NpcScriptInterface::luaActionMoveTo(lua_State* L)
{
	//selfMoveTo(x,y,z)
	Position target;
	target.z = (int)popNumber(L);
	target.y = (int)popNumber(L);
	target.x = (int)popNumber(L);
	
	ScriptEnviroment* env = getScriptEnv();
	Npc* mynpc = env->getNpc();
	if(mynpc)
		mynpc->doMoveTo(target);
	
	return 0;
}

int NpcScriptInterface::luaActionTurn(lua_State* L)
{
	//selfTurn(direction)
	Direction dir = (Direction)popNumber(L);
	
	ScriptEnviroment* env = getScriptEnv();
	Npc* mynpc = env->getNpc();
	if(mynpc)
		mynpc->doTurn(dir);

	return 0;
}

int NpcScriptInterface::luagetDistanceTo(lua_State *L)
{
	//getDistanceTo(uid)
	uint32_t uid = popNumber(L);
	
	ScriptEnviroment* env = getScriptEnv();
	Npc* mynpc = env->getNpc();
	
	Thing* thing = env->getThingByUID(uid);
	if(thing && mynpc){
		Position thing_pos = thing->getPosition();
		Position npc_pos = mynpc->getPosition();
		if(npc_pos.z != thing_pos.z){
			lua_pushnumber(L, -1);
		}
		else{
			int32_t dist = std::max(std::abs(npc_pos.x - thing_pos.x), std::abs(npc_pos.y - thing_pos.y));
			lua_pushnumber(L, dist);
		}	
	}
	else{
		reportErrorFunc(getErrorDesc(LUA_ERROR_THING_NOT_FOUND));
		lua_pushnil(L);
	}
	
	return 1;
}

int NpcScriptInterface::luaSetNpcFocus(lua_State *L)
{
	//doNpcSetCreatureFocus(cid)
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();
	
	Npc* mynpc = env->getNpc();
	if(mynpc){
		Creature* creature = env->getCreatureByUID(cid);
		mynpc->setCreatureFocus(creature);
	}
	return 0;
}

int NpcScriptInterface::luaGetNpcCid(lua_State* L)
{
	//getNpcCid()
	ScriptEnviroment* env = getScriptEnv();
	
	Npc* mynpc = env->getNpc();
	if(mynpc){
		uint32_t cid = env->addThing(mynpc);
		lua_pushnumber(L, cid);
	}
	else{
		lua_pushnil(L);
	}
	
	return 1;
}

int NpcScriptInterface::luaGetNpcName(lua_State* L)
{
	//getNpcName()
	ScriptEnviroment* env = getScriptEnv();
	
	Npc* mynpc = env->getNpc();
	if(mynpc){
		lua_pushstring(L, mynpc->getName().c_str());
	}
	else{
		lua_pushstring(L, "");
	}
	
	return 1;
}

int NpcScriptInterface::luaGetNpcParameter(lua_State *L)
{
	//getNpcParameter(paramKey)
	std::string paramKey = popString(L);
	
	ScriptEnviroment* env = getScriptEnv();
	
	Npc* mynpc = env->getNpc();
	if(mynpc){
		Npc::ParametersMap::iterator it = mynpc->m_parameters.find(paramKey);
		if(it != mynpc->m_parameters.end()){
			lua_pushstring(L, it->second.c_str());
		}
		else{
			lua_pushnil(L);
		}
	}
	else{
		lua_pushnil(L);
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
	
	if(m_scriptInterface->loadFile(file, npc) == -1){
		std::cout << "Warning: [NpcScript::NpcScript] Can not load script. " << file << std::endl;
		std::cout << m_scriptInterface->getLastLuaError() << std::endl;
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
	//onCreatureAppear(creature)
	if(m_scriptInterface->reserveScriptEnv()){
		ScriptEnviroment* env = m_scriptInterface->getScriptEnv();
	
		#ifdef __DEBUG_LUASCRIPTS__
		std::stringstream desc;
		desc << "npc " << m_npc->getName();
		env->setEventDesc(desc.str());
		#endif
	
		lua_State* L = m_scriptInterface->getLuaState();
	
		env->setScriptId(m_onCreatureAppear, m_scriptInterface);
		env->setRealPos(m_npc->getPosition());
		env->setNpc(m_npc);
	
		uint32_t cid = env->addThing(const_cast<Creature*>(creature));
	
		m_scriptInterface->pushFunction(m_onCreatureAppear);
		lua_pushnumber(L, cid);	
		m_scriptInterface->callFunction(1);
		m_scriptInterface->releaseScriptEnv();
	}
	else{
		std::cout << "[Error] Call stack overflow. NpcScript::onCreatureAppear" << std::endl;
	}
}

void NpcScript::onCreatureDisappear(const Creature* creature)
{
	if(m_onCreatureDisappear == -1){
		return;	
	}
	//onCreatureDisappear(id)
	if(m_scriptInterface->reserveScriptEnv()){
		ScriptEnviroment* env = m_scriptInterface->getScriptEnv();
	
		#ifdef __DEBUG_LUASCRIPTS__
		std::stringstream desc;
		desc << "npc " << m_npc->getName();
		env->setEventDesc(desc.str());
		#endif
	
		lua_State* L = m_scriptInterface->getLuaState();
	
		env->setScriptId(m_onCreatureDisappear, m_scriptInterface);
		env->setRealPos(m_npc->getPosition());
		env->setNpc(m_npc);
	
		uint32_t cid = env->addThing(const_cast<Creature*>(creature));
	
		m_scriptInterface->pushFunction(m_onCreatureDisappear);
		lua_pushnumber(L, cid);	
		m_scriptInterface->callFunction(1);
		m_scriptInterface->releaseScriptEnv();
	}
	else{
		std::cout << "[Error] Call stack overflow. NpcScript::onCreatureDisappear" << std::endl;
	}
}

void NpcScript::onCreatureSay(const Creature* creature, SpeakClasses type, const std::string& text)
{
	if(m_onCreatureSay == -1){
		return;	
	}
	//onCreatureSay(cid, type, msg)
	if(m_scriptInterface->reserveScriptEnv()){
		ScriptEnviroment* env = m_scriptInterface->getScriptEnv();
	
		#ifdef __DEBUG_LUASCRIPTS__
		std::stringstream desc;
		desc << "npc " << m_npc->getName();
		env->setEventDesc(desc.str());
		#endif
	
		env->setScriptId(m_onCreatureSay, m_scriptInterface);
		env->setRealPos(m_npc->getPosition());
		env->setNpc(m_npc);
	
		uint32_t cid = env->addThing(const_cast<Creature*>(creature));
	
		lua_State* L = m_scriptInterface->getLuaState();
		m_scriptInterface->pushFunction(m_onCreatureSay);
		lua_pushnumber(L, cid);
		lua_pushnumber(L, type);
		lua_pushstring(L, text.c_str());
		m_scriptInterface->callFunction(3);
		m_scriptInterface->releaseScriptEnv();
	}
	else{
		std::cout << "[Error] Call stack overflow. NpcScript::onCreatureSay" << std::endl;
	}
}

void NpcScript::onThink()
{
	if(m_onThink == -1){
		return;	
	}
	//onThink()
	if(m_scriptInterface->reserveScriptEnv()){
		ScriptEnviroment* env = m_scriptInterface->getScriptEnv();
	
		#ifdef __DEBUG_LUASCRIPTS__
		std::stringstream desc;
		desc << "npc " << m_npc->getName();
		env->setEventDesc(desc.str());
		#endif
	
		env->setScriptId(m_onThink, m_scriptInterface);
		env->setRealPos(m_npc->getPosition());
		env->setNpc(m_npc);
	
		m_scriptInterface->pushFunction(m_onThink);
		m_scriptInterface->callFunction(0);
		m_scriptInterface->releaseScriptEnv();
	}
	else{
		std::cout << "[Error] Call stack overflow. NpcScript::onThink" << std::endl;
	}
}

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
#include "const76.h"
#include "player.h"
#include "monster.h"
#include "npc.h"
#include "game.h"
#include "item.h"
#include "container.h"
#include "combat.h"
#include "depot.h"
#include "house.h"
#include "tasks.h"
#include "tools.h"

#include <libxml/xmlmemory.h>
#include <libxml/parser.h> 

//#include <boost/bind.hpp>
#include <sstream>

#include "actions.h"

extern Game g_game;

Actions::Actions() :
m_scriptInterface("Action Interface")
{
	m_loaded = false;
	m_scriptInterface.initState();
}

Actions::~Actions()
{
	clear();
}

void Actions::clear()
{
	ActionUseMap::iterator it = useItemMap.begin();
	while(it != useItemMap.end()){
		delete it->second;
		useItemMap.erase(it);
		it = useItemMap.begin();
	}

	it = uniqueItemMap.begin();
	while(it != uniqueItemMap.end()){
		delete it->second;
		uniqueItemMap.erase(it);
		it = uniqueItemMap.begin();
	}

	it = actionItemMap.begin();
	while(it != actionItemMap.end()){
		delete it->second;
		actionItemMap.erase(it);
		it = actionItemMap.begin();
	}
	
	m_scriptInterface.reInitState();
}

bool Actions::reload(){
	m_loaded = false;
	//unload
	clear();
	//load
	return loadFromXml(m_datadir);
}

bool Actions::loadFromXml(const std::string& _datadir)
{
	if(m_loaded){
		std::cout << "Error: [Actions::loadFromXml] loaded == true" << std::endl;
		return false;
	}
	m_datadir = _datadir;
	Action* action = NULL;
	//load actions lib in script interface
	if(m_scriptInterface.loadFile(std::string(m_datadir + "actions/lib/actions.lua")) == -1){
		std::cout << "Warning: [Actions::loadFromXml] Can not load actions lib/actions.lua" << std::endl;
	}
	
	std::string filename = m_datadir + "actions/actions.xml";
	xmlDocPtr doc = xmlParseFile(filename.c_str());
	
	if(doc){
		m_loaded = true;
		xmlNodePtr root, p;
		root = xmlDocGetRootElement(doc);
		
		if(xmlStrcmp(root->name,(const xmlChar*)"actions") != 0){
			xmlFreeDoc(doc);
			return false;
		}

		p = root->children;
		while(p){
			if(xmlStrcmp(p->name, (const xmlChar*)"action") == 0){
				int itemid,uniqueid,actionid;
				action = new Action(&m_scriptInterface);
				if(action->configureAction(p)){
					bool success = true;
					std::string scriptfile;
					if(readXMLString(p, "script", scriptfile)){
						if(!action->loadScriptUse(m_datadir + std::string("actions/scripts/") + scriptfile)){
							success = false;
						}
					}
					else{
						//TODO? hardcoded actions ....
						success = false;
					}
					
					if(success){
						if(readXMLInteger(p,"itemid",itemid)){
							useItemMap[itemid] = action;
							action = NULL;
						}
						else if(readXMLInteger(p,"uniqueid",uniqueid)){
							uniqueItemMap[uniqueid] = action;
							action = NULL;
						}
						else if(readXMLInteger(p,"actionid",actionid)){
							actionItemMap[actionid] = action;
							action = NULL;
						}
						else{
							std::cout << "Warning: [Actions::loadFromXml] Missing item id" << std::endl;
							success = false;
						}
					}
					
					if(!success){
						delete action;
					}
				}
				else{
					std::cout << "Warning: [Actions::loadFromXml] Can not configure action" << std::endl;
					delete action;
				}
			}
			p = p->next;
		}
		
		xmlFreeDoc(doc);
	}
	else{
		std::cout << "Warning: [Actions::loadFromXml] Can not open actions.xml" << std::endl;
	}
	return m_loaded;
}

int Actions::canUse(const Creature* creature, const Position& pos)
{
	if(pos.x != 0xFFFF){
		if(!Position::areInRange<1,1,0>(pos, creature->getPosition())){
			return TOO_FAR;
		}
	}
	return CAN_USE;
}

int Actions::canUseFar(const Creature* creature, const Position& to_pos, const bool blockWalls)
{
	if(to_pos.x == 0xFFFF){
		return CAN_USE;
	}
	Position creature_pos = creature->getPosition();
	if(!Position::areInRange<7,5,0>(to_pos, creature_pos)){
		return TOO_FAR;
	}
	
	if(blockWalls && canUse(creature, to_pos) == TOO_FAR && 
		!g_game.map->canThrowObjectTo(creature_pos, to_pos)){
		return CAN_NOT_THROW;
	}

	return CAN_USE;
}

Action *Actions::getAction(const Item* item)
{
	if(item->getUniqueId() != 0){
		ActionUseMap::iterator it = uniqueItemMap.find(item->getUniqueId());
    	if (it != uniqueItemMap.end()){
			return it->second;
		}
	}
	if(item->getActionId() != 0){
		ActionUseMap::iterator it = actionItemMap.find(item->getActionId());
    	if (it != actionItemMap.end()){
	    	return it->second;
		}
	}
	ActionUseMap::iterator it = useItemMap.find(item->getID());
    if (it != useItemMap.end()){
	   	return it->second;
	}
	
	return NULL;
}

bool Actions::useItem(Player* player, const Position& pos, uint8_t index, Item* item)
{	
	//check if it is a house door
	if(Door* door = item->getDoor()){
		if(door->canUse(player) == false){
			player->sendCancelMessage(RET_CANNOTUSETHISOBJECT);
			return false;
		}
	}
	
	//look for the item in action maps	
	Action* action = getAction(item);
	
	//if found execute it
	if(action){
		long stack = item->getParent()->__getIndexOfThing(item);
		PositionEx posEx(pos, stack);
		if(action->executeUse(player, item, posEx, posEx)){
			return true;
		}
	}
	
	//can we read it?
	int maxlen;
	int RWInfo = item->getRWInfo(maxlen);
	if(RWInfo & CAN_BE_READ){
		if(RWInfo & CAN_BE_WRITTEN){
			player->sendTextWindow(item, maxlen, true);
		}
		else{
			player->sendTextWindow(item, 0, false);
		}
		return true;
	}
	
	//if it is a container try to open it
	if(Container* container = item->getContainer()){
		if(openContainer(player, container, index))
			return true;
	}
    
	//we dont know what to do with this item
	player->sendCancelMessage(RET_CANNOTUSETHISOBJECT);
	return false;	
}

bool Actions::openContainer(Player* player, Container* container, const unsigned char index)
{
	Container* openContainer = NULL;

	//depot container
	if(Depot* depot = container->getDepot()){
		Depot* myDepot = player->getDepot(depot->getDepotId(), true);
		myDepot->setParent(depot->getParent());
		openContainer = myDepot;
	}
	else{
		openContainer = container;
	}

	//open/close container
	int32_t oldcid = player->getContainerID(openContainer);
	if(oldcid != -1){
		player->onCloseContainer(openContainer);
		player->closeContainer(oldcid);
	}
	else{
		player->addContainer(index, openContainer);
		player->onSendContainer(openContainer);
	}

	return true;
}

bool Actions::useItemEx(Player* player, const Position& from_pos,
	const Position& to_pos, const unsigned char to_stack, Item* item)
{
	Action* action = getAction(item);
	
	if(action){
		if(action->allowFarUse() == false){
			if(canUse(player,to_pos) == TOO_FAR){
				player->sendCancelMessage(RET_TOOFARAWAY);
				return false;
			}
		}
		else if(canUseFar(player, to_pos, action->blockWalls()) == TOO_FAR){
			player->sendCancelMessage(RET_TOOFARAWAY);
			return false;
		}
		else if(canUseFar(player, to_pos, action->blockWalls()) == CAN_NOT_THROW){
			player->sendCancelMessage(RET_CANNOTTHROW);
			return false;
		}
	
		long from_stack = item->getParent()->__getIndexOfThing(item);
		PositionEx posFromEx(from_pos, from_stack);
		PositionEx posToEx(to_pos, to_stack);
		if(action->executeUse(player, item, posFromEx, posToEx))
			return true;
	}
	
	//not found
	player->sendCancelMessage(RET_CANNOTUSETHISOBJECT);
	return false;
}

Action::Action(LuaScriptInterface* _interface)
{
	m_scriptInterface = _interface;
	m_scriptId = 0;
	allowfaruse = false;
	blockwalls = true;
}

Action::~Action()
{

}

bool Action::configureAction(xmlNodePtr p)
{
	int intValue;
	if(readXMLInteger(p, "allowfaruse", intValue)){
		if(intValue != 0){
			setAllowFarUse(true);
		}
	}
	if(readXMLInteger(p, "blockwalls", intValue)){
		if(intValue == 0){
			setBlockWalls(false);
		}
	}
	
	return true;
}

bool Action::loadScriptUse(const std::string& script)
{
	if(!m_scriptInterface || m_scriptId != 0){
		std::cout << "Failure: [Action::loadScript] m_scriptInterface == NULL. scriptid = " << m_scriptId << std::endl;
		return false;
	}
	
	if(m_scriptInterface->loadFile(script) == -1){
		std::cout << "Warning: [Action::loadScript] Can not load script. " << script << std::endl;
		return false;
	}
	long id = m_scriptInterface->getEvent("onUse");
	if(id == -1){
		std::cout << "Warning: [Action::loadScript] Event onUse not found. " << script << std::endl;
		return false;
		
	}
	m_scriptId = id;
	return true;
}

bool Action::executeUse(Player* player, Item* item, const PositionEx& posFrom, const PositionEx& posTo)
{
	//onUse(cid, item1, position1, item2, position2)
	ScriptEnviroment* env = m_scriptInterface->getScriptEnv();
	
	//debug only
	std::stringstream desc;
	desc << player->getName() << " - " << item->getID() << " " << posFrom << "|" << posTo;
	env->setEventDesc(desc.str());
	//
	
	env->setScriptId(m_scriptId, m_scriptInterface);
	env->setRealPos(player->getPosition());
	
	long cid = env->addThing(player);
	long itemid1 = env->addThing(item);
	
	lua_State* L = m_scriptInterface->getLuaState();
	int size0 = lua_gettop(L);
	
	m_scriptInterface->pushFunction(m_scriptId);
	lua_pushnumber(L, cid);
	LuaScriptInterface::pushThing(L, item, itemid1);
	LuaScriptInterface::pushPosition(L, posFrom, posFrom.stackpos);
	//std::cout << "posTo" <<  (Position)posTo << " stack" << (int)posTo.stackpos <<std::endl;
	Thing* thing = g_game.internalGetThing(player, posTo, posTo.stackpos);
	if(thing && posFrom != posTo){
		long thingId2 = env->addThing(thing);
		LuaScriptInterface::pushThing(L, thing, thingId2);
		LuaScriptInterface::pushPosition(L, posTo, posTo.stackpos);
	}
	else{
		LuaScriptInterface::pushThing(L, NULL, 0);
		Position posEx;
		LuaScriptInterface::pushPosition(L, posEx, 0);
	}
	
	long ret;
	if(m_scriptInterface->callFunction(5, ret) == false){
		ret = 0;
	}
	/*
	if(lua_pcall(L, 5, 1, 0) != 0){
		LuaScriptInterface::reportError(NULL, std::string(LuaScriptInterface::popString(L)));
		ret = false;
	}
	else{
		ret = (LuaScriptInterface::popNumber(L) != 0);
	}
	*/
	if(size0 != lua_gettop(L)){
		LuaScriptInterface::reportError(NULL, "Stack size changed!");
	}
	
	return (ret != 0);
}

/*
void Action::setScriptInterface(LuaScriptInterface* _interface)
{
	if(m_scriptInterface != NULL && m_scriptId != 0){
		std::cout << "Warning: [Action::setScriptInterface] m_scriptInterface != NULL && scriptId != 0" << std::endl;
	}
	m_scriptInterface = _interface;
}
*/

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
#include "depot.h"
#include "house.h"

#include <libxml/xmlmemory.h>
#include <libxml/parser.h> 

#include "actions.h"

bool readXMLInteger(xmlNodePtr p, const char *tag, int &value);

Actions::Actions(Game* igame)
:game(igame)
{
	//                   
}

Actions::~Actions()
{
	clear();
}

void Actions::clear()
{
	ActionUseMap::iterator it = useItemMap.begin();
	while(it != useItemMap.end()) {
		delete it->second;
		useItemMap.erase(it);
		it = useItemMap.begin();
	}
	it = uniqueItemMap.begin();
	while(it != uniqueItemMap.end()) {
		delete it->second;
		uniqueItemMap.erase(it);
		it = uniqueItemMap.begin();
	}
	it = actionItemMap.begin();
	while(it != actionItemMap.end()) {
		delete it->second;
		actionItemMap.erase(it);
		it = actionItemMap.begin();
	}
}

bool Actions::reload(){
	this->loaded = false;
	//unload
	clear();
	//load
	return loadFromXml(datadir);
}

bool Actions::loadFromXml(const std::string &_datadir)
{
	this->loaded = false;
	Action *action = NULL;
	
	datadir = _datadir;
	
	std::string filename = datadir + "actions/actions.xml";
	std::transform(filename.begin(), filename.end(), filename.begin(), tolower);
	xmlDocPtr doc = xmlParseFile(filename.c_str());

	if(doc){
		this->loaded=true;
		xmlNodePtr root, p;
		root = xmlDocGetRootElement(doc);
		
		if(xmlStrcmp(root->name,(const xmlChar*) "actions") != 0){
			xmlFreeDoc(doc);
			return false;
		}

		p = root->children;
        
		while (p){
			if(xmlStrcmp(p->name, (const xmlChar*) "action") == 0){
				int itemid,uniqueid,actionid;
				if(readXMLInteger(p,"itemid",itemid)){
					action = loadAction(p);
					useItemMap[itemid] = action;
					action = NULL;
				}
				else if(readXMLInteger(p,"uniqueid",uniqueid)){
					action = loadAction(p);
					uniqueItemMap[uniqueid] = action;
					action = NULL;
				}
				else if(readXMLInteger(p,"actionid",actionid)){
					action = loadAction(p);
					actionItemMap[actionid] = action;
					action = NULL;
				}
				else{
					std::cout << "missing action id." << std::endl;
				}
			}
			p = p->next;
		}
		
		xmlFreeDoc(doc);
	}
	return this->loaded;
}

Action *Actions::loadAction(xmlNodePtr xmlaction){
	Action *action = NULL;
	char* scriptfile = (char*)xmlGetProp(xmlaction,(xmlChar*)"script");
	if(scriptfile){
		action = new Action(game,datadir, datadir + std::string("actions/scripts/") + scriptfile);
		if(action->isLoaded()){
			char* sallow = (char*)xmlGetProp(xmlaction,(xmlChar*)"allowfaruse");
			if(sallow){
				if(strcmp(sallow,"1") == 0){
					action->setAllowFarUse(true);
				}
				xmlFreeOTSERV(sallow);
			}
			sallow = (char*)xmlGetProp(xmlaction,(xmlChar*)"blockwalls");
			if(sallow){
				if(strcmp(sallow,"0") == 0){
					action->setBlockWalls(false);
				}
				xmlFreeOTSERV(sallow);
			}
		}
		else{
			delete action;
			action = NULL;
		}
		xmlFreeOTSERV(scriptfile);
	}
	else{
		std::cout << "Missing script tag."  << std::endl;
	}
	return action;
}

int Actions::canUse(const Player *player,const Position &pos) const
{
	if(pos.x != 0xFFFF){
		if(!Position::areInRange<1,1,0>(pos, player->getPosition())){
			return TOO_FAR;
		}
		//int dist_x = std::abs(pos.x - player->getPosition().x);
		//int dist_y = std::abs(pos.y - player->getPosition().y);
		//if(dist_x > 1 || dist_y > 1 || (pos.z != player->getPosition().z)){
		//	return TOO_FAR;
		//}
	}
	return CAN_USE;
}

int Actions::canUseFar(const Player *player,const Position &to_pos, const bool blockWalls) const
{
	if(to_pos.x == 0xFFFF){
		return CAN_USE;
	}
	if(!Position::areInRange<7,5,0>(to_pos, player->getPosition())){
		//std::abs(player->getPosition().x - to_pos.x) > 7 || std::abs(player->getPosition().y - to_pos.y) > 5 ||
		//player->getPosition().z != to_pos.z)
		return TOO_FAR;
	}
	
	if(canUse(player,to_pos) == TOO_FAR){
		if(blockWalls && (!game->map->canThrowObjectTo(player->getPosition(), to_pos))){
			return CAN_NOT_THROW;
		}
	}

	return CAN_USE;
}

Action *Actions::getAction(const Item *item)
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

bool Actions::UseItem(Player* player, const Position& pos, const unsigned char stack, 
	const unsigned short itemid, const unsigned char index)
{	
	if(canUse(player,pos)== TOO_FAR){
		/*
		bool Game::playerUseItem(Player* player, const Position& pos, uint8_t stackpos, uint8_t index, uint16_t itemId)

		boost::function1<void, Game*> _f = boost::bind(&Game::playerUseItem, game,
			player, pos, stack, index, itemid);
		*/

		player->sendCancelMessage(RET_TOOFARAWAY);
		return false;
	}
	
	Thing* thing = game->internalGetThing(player, pos, stack);
	if(!thing){
		player->sendCancelMessage(RET_NOTPOSSIBLE);
		return false;
	}

	Item *item = thing->getItem();

	if(!item){
		#ifdef __DEBUG__
		std::cout << "no item" << std::endl;
		#endif
		player->sendCancelMessage(RET_CANNOTUSETHISOBJECT);
		return false;
	}
	
	if(item->getID() != itemid){
		#ifdef __DEBUG__
		std::cout << "no id" << std::endl;
		#endif
		player->sendCancelMessage(RET_CANNOTUSETHISOBJECT);
		return false;
	} 
	
	//check if it is a house door
	if(Door* door = item->getDoor()){
		if(door->canUse(player) == false){
			player->sendCancelMessage(RET_CANNOTUSETHISOBJECT);
			return false;
		}
	}
	
	//look for the item in action maps	
	Action *action = getAction(item);
	
	//if found execute it
	if(action){
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

bool Actions::UseItemEx(Player* player, const Position &from_pos,
	const unsigned char from_stack,const Position &to_pos,
	const unsigned char to_stack,const unsigned short itemid)
{
	if(canUse(player,from_pos) == TOO_FAR){
		/*
		bool Game::playerUseItemEx(Player* player, const Position& fromPos, uint8_t fromStackPos, uint16_t fromItemId,
			const Position& toPos, uint8_t toStackPos, uint16_t toItemId)

		boost::function1<void, Game*> _f = boost::bind(&Game::playerUseItemEx, this,
			player, fromPos, fromStackPos, fromItemId, toPos, toStackPos, toItemId);
		*/

		player->sendCancelMessage(RET_TOOFARAWAY);
		return false;
	}
	
	Thing* thing = game->internalGetThing(player, from_pos, from_stack);
	if(!thing)
		return false;

	Item* item = thing->getItem();
	if(!item)
		return false;
	
	if(item->getID() != itemid)
		return false;
		
	if(!item->isUseable())
		return false;
	
	Action *action = getAction(item);
	
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
		
		PositionEx posFromEx(from_pos, from_stack);
		PositionEx posToEx(to_pos, to_stack);
		if(action->executeUse(player,item, posFromEx, posToEx))
			return true;
	}
	
	//not found
	player->sendCancelMessage(RET_CANNOTUSETHISOBJECT);
	return false;
}

//

bool readXMLInteger(xmlNodePtr p, const char *tag, int &value)
{
	char* sinteger = (char*)xmlGetProp(p, (xmlChar*)tag);
	if(!sinteger)
		return false;
	else{
		unsigned short integer = atoi(sinteger);
		value = integer;
		xmlFreeOTSERV(sinteger);
		return true;
	}
}


Action::Action(Game* igame,const std::string &datadir, const std::string &scriptname)
{
	loaded = false;
	allowfaruse = false;
	blockwalls = true;
	script = new ActionScript(igame,datadir,scriptname);
	if(script->isLoaded())
		loaded = true;
}

Action::~Action()
{
	if(script) {
		delete script;
		script = NULL;
	}
}


bool Action::executeUse(Player *player,Item* item, PositionEx &posFrom, PositionEx &posTo)
{
	//onUse(uidplayer, item1,position1,item2,position2)
	script->ClearMap();
	script->_player = player;
	PositionEx playerpos = player->getPosition();
	unsigned int cid = script->AddThingToMap((Thing*)player);
	unsigned int itemid1 = script->AddThingToMap(item);
	lua_State*  luaState = script->getLuaState();
	
	lua_pushstring(luaState, "onUse");
	lua_gettable(luaState, LUA_GLOBALSINDEX);
	
	lua_pushnumber(luaState, cid);
	script->internalAddThing(luaState,item,itemid1);
	script->internalAddPositionEx(luaState,posFrom);
	//std::cout << "posTo" <<  (Position)posTo << " stack" << (int)posTo.stackpos <<std::endl;
	Thing* thing = script->game->internalGetThing(player, posTo, posTo.stackpos);
	if(thing && posFrom != posTo){
		int thingId2 = script->AddThingToMap(thing);
		script->internalAddThing(luaState,thing,thingId2);
		script->internalAddPositionEx(luaState,posTo);
	}
	else{
		script->internalAddThing(luaState,NULL,0);
		PositionEx posEx;
		script->internalAddPositionEx(luaState,posEx);
	}
	
	lua_pcall(luaState, 5, 1, 0);
	
	bool ret = (script->internalGetNumber(luaState) != 0);
	
	return ret;
}


//std::map<unsigned int,KnownThing*> ActionScript::uniqueIdMap;
std::map<unsigned int,Thing*> ActionScript::uniqueIdMap;

ActionScript::ActionScript(Game *igame,const std::string &datadir,const std::string &scriptname):
game(igame),
_player(NULL)
{
	this->loaded = false;
	lastuid = 0;
	if(scriptname == "")
		return;
	luaState = lua_open();
	luaopen_loadlib(luaState);
	luaopen_base(luaState);
	luaopen_math(luaState);
	luaopen_string(luaState);
	luaopen_io(luaState);
    lua_dofile(luaState, std::string(datadir + "actions/lib/actions.lua").c_str());
    
	FILE* in=fopen(scriptname.c_str(), "r");
	if(!in){
		std::cout << "Error: Can not open " << scriptname.c_str() << std::endl;
		return;
	}
	else
		fclose(in);
	lua_dofile(luaState, scriptname.c_str());
	this->setGlobalNumber("addressOfActionScript", (int)this);
	this->loaded = true;
	this->registerFunctions();
}

ActionScript::~ActionScript()
{
	//std::map<unsigned int,KnownThing*>::iterator it;
	std::map<unsigned int,Thing*>::iterator it;
	/*for(it = ThingMap.begin(); it != ThingMap.end();it++ ){
		delete it->second;
	}*/

	ThingMap.clear();
	/*
	for(it = uniqueIdMap.begin(); it != uniqueIdMap.end();it++ ){
		delete it->second;
	}
	
	uniqueIdMap.clear();
	*/
}

void ActionScript::ClearMap()
{
	//std::map<unsigned int,KnownThing*>::iterator it;
	std::map<unsigned int,Thing*>::iterator it;
	/*for(it = ThingMap.begin(); it != ThingMap.end();it++ ){
		//delete it->second;
		it->second = NULL;
	}*/
	ThingMap.clear();
	lastuid = 0;
}

void ActionScript::AddThingToMapUnique(Thing *thing)
{
	Item *item = thing->getItem();
	if(item && item->getUniqueId() != 0 ){
		unsigned short uid = item->getUniqueId();
		//KnownThing *tmp = uniqueIdMap[uid];
		Thing *tmp = uniqueIdMap[uid];
		if(!tmp){
			//KnownThing *tmp = new KnownThing;
			//tmp->thing = thing;
			//tmp->type = thingTypeItem;
			//uniqueIdMap[uid] = tmp;
			uniqueIdMap[uid] = thing;
		}
		else{
			std::cout << "Duplicate uniqueId " <<  uid << std::endl;
		}
	}
}
/*
void ActionScript::UpdateThingPos(int uid, PositionEx &pos){
	//KnownThing *tmp = ThingMap[uid];
	Thing *tmp = ThingMap[uid];
	if(tmp){
		tmp->pos = pos;
	}		
}
*/
unsigned int ActionScript::AddThingToMap(Thing *thing)
{
	Item *item = thing->getItem();
	//if(item && item->pos.x != 0xFFFF && item->getUniqueId()){
	if(item && item->getTile() == item->getParent() && item->getUniqueId()){
		unsigned short uid = item->getUniqueId();
		//KnownThing *tmp = uniqueIdMap[uid];
		Thing *tmp = uniqueIdMap[uid];
		if(!tmp){
			std::cout << "Item with unique id not included in the map!." << std::endl;
		}
		//KnownThing *newKT = new KnownThing;
		//newKT->thing = thing;
		//newKT->type = thingTypeItem;
		//newKT->pos = pos;
		//ThingMap[uid] = newKT;
		ThingMap[uid] = thing;
		return uid;
	}
	
	//std::map<unsigned int,KnownThing*>::iterator it;
	std::map<unsigned int,Thing*>::iterator it;
	for(it = ThingMap.begin(); it != ThingMap.end();it++ ){
		if(it->second == thing){
			return it->first;
		}
	}
	/*
	KnownThing *tmp = new KnownThing;
	tmp->thing = thing;
	tmp->pos = pos;
	
	if(thing->getItem())
		tmp->type = thingTypeItem;
	else if(Creature *c = thing->getCreature()){
		if(c->getPlayer())
			tmp->type = thingTypePlayer;
		else if(c->getMonster())
			tmp->type = thingTypeMonster;
		else if(c->getNpc())
			tmp->type = thingTypeNpc;	
	}
	else
		tmp->type = thingTypeUnknown;
	*/
	lastuid++;
	while(ThingMap[lastuid]){
		lastuid++;
	}
	//ThingMap[lastuid] = tmp;
	ThingMap[lastuid] = thing;
	return lastuid;
}

//const KnownThing* ActionScript::GetThingByUID(int uid)
Thing* ActionScript::GetThingByUID(int uid)
{
	//KnownThing *tmp = ThingMap[uid];
	Thing *tmp = ThingMap[uid];
	if(tmp)
		return tmp;
	
	tmp = uniqueIdMap[uid];
	if(tmp /*&& thing->getParent() == thing->getTile()*/){
		/*KnownThing *newKT = new KnownThing;
		newKT->thing = tmp->thing;
		newKT->type = tmp->type;
		newKT->pos = tmp->thing->getPosition();
		ThingMap[uid] = newKT;*/
		ThingMap[uid] = tmp;
		return tmp;
	}
	return NULL;
}

//const KnownThing* ActionScript::GetItemByUID(int uid)
Item* ActionScript::GetItemByUID(int uid)
{
	//const KnownThing *tmp = GetThingByUID(uid);
	Thing* tmp = GetThingByUID(uid);
	if(tmp && !tmp->isRemoved()){
		if(Item *item = tmp->getItem())
			return item;
	}

	return NULL;
}

//const KnownThing* ActionScript::GetCreatureByUID(int uid)
Creature* ActionScript::GetCreatureByUID(int uid)
{
	//const KnownThing *tmp = GetThingByUID(uid);
	Thing* tmp = GetThingByUID(uid);
	if(tmp && !tmp->isRemoved()){
		if(Creature* creature = tmp->getCreature()) /*type == thingTypePlayer || tmp->type == thingTypeMonster
			|| tmp->type == thingTypeNpc )*/
			return creature;
	}
	return NULL;
}

//const KnownThing* ActionScript::GetPlayerByUID(int uid)
Player* ActionScript::GetPlayerByUID(int uid)
{
	Thing* tmp = GetThingByUID(uid);
	if(tmp){
		if(Creature* creature = tmp->getCreature()){ 
			if(Player* player = creature->getPlayer()/* == thingTypePlayer*/){
				return player;
			}
		}
	}
	return NULL;
}


int ActionScript::registerFunctions()
{
	//getPlayerFood(uid)
	lua_register(luaState, "getPlayerFood", ActionScript::luaActionGetPlayerFood);
	//getPlayerHealth(uid)	
	lua_register(luaState, "getPlayerHealth", ActionScript::luaActionGetPlayerHealth);
	//getPlayerMana(uid)
	lua_register(luaState, "getPlayerMana", ActionScript::luaActionGetPlayerMana);
	//getPlayerLevel(uid)
	lua_register(luaState, "getPlayerLevel", ActionScript::luaActionGetPlayerLevel);
	//getPlayerMagLevel(uid)
	lua_register(luaState, "getPlayerMagLevel", ActionScript::luaActionGetPlayerMagLevel);
	//getPlayerName(uid)	
	lua_register(luaState, "getPlayerName", ActionScript::luaActionGetPlayerName);
	//getPlayerAccess(uid)	
	lua_register(luaState, "getPlayerAccess", ActionScript::luaActionGetPlayerAccess);
	//getPlayerPosition(uid)
	lua_register(luaState, "getPlayerPosition", ActionScript::luaActionGetPlayerPosition);
	//getPlayerSkill(uid,skillid)
	lua_register(luaState, "getPlayerSkill", ActionScript::luaActionGetPlayerSkill);
	//getPlayerMasterPos(cid)
	lua_register(luaState, "getPlayerMasterPos", ActionScript::luaActionGetPlayerMasterPos);
	//getPlayerVocation(cid)
	lua_register(luaState, "getPlayerVocation", ActionScript::luaActionGetPlayerVocation);
	//getPlayerGuildId(cid)
	lua_register(luaState, "getPlayerGuildId", ActionScript::luaActionGetPlayerGuildId);
	//getPlayerItemCount(uid,itemid)
	//getPlayerItem(uid,itemid)
	
	
	//getPlayerStorageValue(uid,valueid)
	lua_register(luaState, "getPlayerStorageValue", ActionScript::luaActionGetPlayerStorageValue);
	//setPlayerStorageValue(uid,valueid, newvalue)
	lua_register(luaState, "setPlayerStorageValue", ActionScript::luaActionSetPlayerStorageValue);
	
	//getTilePzInfo(pos) 1 is pz. 0 no pz.
	lua_register(luaState, "getTilePzInfo", ActionScript::luaActionGetTilePzInfo);
	
	//getItemRWInfo(uid)
	lua_register(luaState, "getItemRWInfo", ActionScript::luaActionGetItemRWInfo);
	//getThingfromPos(pos)
	lua_register(luaState, "getThingfromPos", ActionScript::luaActionGetThingfromPos);
	//getThingPos(uid)
	
	//doRemoveItem(uid,n)
	lua_register(luaState, "doRemoveItem", ActionScript::luaActionDoRemoveItem);
	//doPlayerFeed(uid,food)
	lua_register(luaState, "doPlayerFeed", ActionScript::luaActionDoFeedPlayer);	
	//doPlayerSendCancel(uid,text)
	lua_register(luaState, "doPlayerSendCancel", ActionScript::luaActionDoSendCancel);
	//doTeleportThing(uid,newpos)
	lua_register(luaState, "doTeleportThing", ActionScript::luaActionDoTeleportThing);
	//doTransformItem(uid,toitemid)	
	lua_register(luaState, "doTransformItem", ActionScript::luaActionDoTransformItem);
	//doPlayerSay(uid,text,type)
	lua_register(luaState, "doPlayerSay", ActionScript::luaActionDoPlayerSay);
	//doSendMagicEffect(uid,position,type)
	lua_register(luaState, "doSendMagicEffect", ActionScript::luaActionDoSendMagicEffect);
	//doChangeTypeItem(uid,new_type)	
	lua_register(luaState, "doChangeTypeItem", ActionScript::luaActionDoChangeTypeItem);
	//doSetItemActionId(uid,actionid)
	lua_register(luaState, "doSetItemActionId", ActionScript::luaActionDoSetItemActionId);
	//doSetItemText(uid,text)
	lua_register(luaState, "doSetItemText", ActionScript::luaActionDoSetItemText);
	//doSetItemSpecialDescription(uid,desc)
	lua_register(luaState, "doSetItemSpecialDescription", ActionScript::luaActionDoSetItemSpecialDescription);
	//doSendAnimatedText(position,text,color)
	lua_register(luaState, "doSendAnimatedText", ActionScript::luaActionDoSendAnimatedText);
	//doPlayerAddSkillTry(uid,skillid,n)
	lua_register(luaState, "doPlayerAddSkillTry", ActionScript::luaActionDoPlayerAddSkillTry);
	//doPlayerAddHealth(uid,health)
	lua_register(luaState, "doPlayerAddHealth", ActionScript::luaActionDoPlayerAddHealth);
	//doPlayerAddMana(uid,mana)
	lua_register(luaState, "doPlayerAddMana", ActionScript::luaActionDoPlayerAddMana);
	//doPlayerAddItem(uid,itemid,count or type) . returns uid of the created item
	lua_register(luaState, "doPlayerAddItem", ActionScript::luaActionDoPlayerAddItem);
	//doPlayerSendTextMessage(uid,MessageClasses,message)
	lua_register(luaState, "doPlayerSendTextMessage", ActionScript::luaActionDoPlayerSendTextMessage);
	//doPlayerRemoveMoney(uid,money)
	lua_register(luaState, "doPlayerRemoveMoney", ActionScript::luaActionDoPlayerRemoveMoney);
	//doShowTextWindow(uid,maxlen,canWrite)	
	lua_register(luaState, "doShowTextWindow", ActionScript::luaActionDoShowTextWindow);	
	//doDecayItem(uid)
	lua_register(luaState, "doDecayItem", ActionScript::luaActionDoDecayItem);
	//doCreateItem(itemid,type or count,position) .only working on ground. returns uid of the created item
	lua_register(luaState, "doCreateItem", ActionScript::luaActionDoCreateItem);
	//doSummonCreature(name, position)
	lua_register(luaState, "doSummonCreature", ActionScript::luaActionDoSummonCreature);
	//doPlayerSetMasterPos(cid,pos)
	lua_register(luaState, "doPlayerSetMasterPos", ActionScript::luaActionDoPlayerSetMasterPos);
	//doPlayerSetVocation(cid,voc)
	lua_register(luaState, "doPlayerSetVocation", ActionScript::luaActionDoPlayerSetVocation);
	//doPlayerRemoveItem(cid,itemid,count)
	lua_register(luaState, "doPlayerRemoveItem", ActionScript::luaActionDoPlayerRemoveItem);
	
	//doMoveItem(uid,toPos)
	//doMovePlayer(cid,direction)
	
	//doPlayerAddCondition(....)
	
	
	return true;
}

ActionScript* ActionScript::getActionScript(lua_State *L)
{
	lua_getglobal(L, "addressOfActionScript");
	int val = (int)internalGetNumber(L);

	ActionScript* myaction = (ActionScript*) val;
	if(!myaction){
		return 0;
	}
	return myaction;
}

const Position& ActionScript::internalGetRealPosition(ActionScript *action, Player *player, const Position &pos)
{
	if(action)
		return action->game->internalGetPosition(player, pos);
	else {
		static const Position dummyPos(0,0,0);
		return dummyPos;
	}
}

void ActionScript::internalAddThing(lua_State *L, const Thing* thing, const unsigned int thingid)
{	
	lua_newtable(L);
	if(thing && thing->getItem()){	
		const Item *item = thing->getItem();
		setField(L,"uid", thingid);
		setField(L,"itemid", item->getID());

		if(item->hasSubType())
			setField(L,"type", item->getItemCountOrSubtype());
		else
			setField(L,"type", 0);

		setField(L,"actionid", item->getActionId());
	}
	else if(thing && thing->getCreature()){
		const Creature* c = thing->getCreature();
		setField(L,"uid", thingid);
		setField(L,"itemid", 1);
		char type;
		if(c->getPlayer()){
			type = 1;
		}
		else if(c->getMonster()){
			type = 2;
		}
		else{//npc
			type = 3;
		}	
		setField(L,"type", type);
		setField(L,"actionid", 0);
	}	
	else{
		setField(L,"uid", 0);
		setField(L,"itemid", 0);
		setField(L,"type", 0);
		setField(L,"actionid", 0);
	}
}

void ActionScript::internalAddPositionEx(lua_State *L, const PositionEx& pos)
{
	lua_newtable(L);
	setField(L,"z", pos.z);
	setField(L,"y", pos.y);
	setField(L,"x", pos.x);
	setField(L,"stackpos",pos.stackpos);
}

void ActionScript::internalGetPositionEx(lua_State *L, PositionEx& pos)
{	
	pos.z = (int)getField(L,"z");
	pos.y = (int)getField(L,"y");
	pos.x = (int)getField(L,"x");
	pos.stackpos = (int)getField(L,"stackpos");
	lua_pop(L, 1); //table
}

unsigned long ActionScript::internalGetNumber(lua_State *L)
{
	lua_pop(L,1);
	return (unsigned long)lua_tonumber(L, 0);
}
const char* ActionScript::internalGetString(lua_State *L)
{	
	lua_pop(L,1);		
	return lua_tostring(L, 0);
}

int ActionScript::internalGetPlayerInfo(lua_State *L, ePlayerInfo info)
{
	unsigned int cid = (unsigned int)internalGetNumber(L);
	ActionScript *action = getActionScript(L);
	int value;	
	
	//const KnownThing* tmp = action->GetPlayerByUID(cid);
	const Player* player = action->GetPlayerByUID(cid);
	if(player){
		PositionEx pos;
		const Tile *tile;
		//Player *player = dynamic_cast<Player*>(tmp->thing);
		switch(info){
		case PlayerInfoAccess:
			value = player->access;
			break;		
		case PlayerInfoLevel:
			value = player->level;
			break;		
		case PlayerInfoMagLevel:
			value = player->maglevel;
			break;
		case PlayerInfoMana:
			value = player->mana;
			break;
		case PlayerInfoHealth:
			value = player->health;
			break;
		case PlayerInfoName:
			lua_pushstring(L, player->name.c_str());
			return 1;
			break;
		case PlayerInfoPosition:			
			pos = player->getPosition();
			tile = player->getTile();
			if(tile)
				pos.stackpos = player->getParent()->__getIndexOfThing(player);
			internalAddPositionEx(L,pos);
			return 1;
			break;
		case PlayerInfoMasterPos:
			pos = player->masterPos;
			internalAddPositionEx(L,pos);
			return 1;
			break;
		case PlayerInfoFood:
			value = player->food/1000;
			break;
		case PlayerInfoVocation:
			value = player->vocation;
			break;
		case PlayerInfoGuildId:
			value = player->guildId;
			break;
		default:
			std::cout << "GetPlayerInfo: Unknown player info " << info << std::endl;
			value = 0;
			break;		
		}
		lua_pushnumber(L,value);
		return 1;
	}
	else{
		lua_pushnumber(L, -1);
		std::cout << "GetPlayerInfo(" << info << "): player not found" << std::endl;
		return 1;
	}		
	
	lua_pushnumber(L, 0);
	return 1;
}
//getPlayer[Info](uid)
int ActionScript::luaActionGetPlayerFood(lua_State *L){	
	return internalGetPlayerInfo(L,PlayerInfoFood);}
	
int ActionScript::luaActionGetPlayerAccess(lua_State *L){
	return internalGetPlayerInfo(L,PlayerInfoAccess);}
	
int ActionScript::luaActionGetPlayerLevel(lua_State *L){
	return internalGetPlayerInfo(L,PlayerInfoLevel);}
	
int ActionScript::luaActionGetPlayerMagLevel(lua_State *L){
	return internalGetPlayerInfo(L,PlayerInfoMagLevel);}
	
int ActionScript::luaActionGetPlayerMana(lua_State *L){
	return internalGetPlayerInfo(L,PlayerInfoMana);}

int ActionScript::luaActionGetPlayerHealth(lua_State *L){
	return internalGetPlayerInfo(L,PlayerInfoHealth);}
	
int ActionScript::luaActionGetPlayerName(lua_State *L){
	return internalGetPlayerInfo(L,PlayerInfoName);}
	
int ActionScript::luaActionGetPlayerPosition(lua_State *L){	
	return internalGetPlayerInfo(L,PlayerInfoPosition);}
	
int ActionScript::luaActionGetPlayerVocation(lua_State *L){
	return internalGetPlayerInfo(L,PlayerInfoVocation);}

int ActionScript::luaActionGetPlayerMasterPos(lua_State *L){
	return internalGetPlayerInfo(L,PlayerInfoMasterPos);}

int ActionScript::luaActionGetPlayerGuildId(lua_State *L){
	return internalGetPlayerInfo(L,PlayerInfoGuildId);}
//

int ActionScript::luaActionDoRemoveItem(lua_State *L)
{	
	//doRemoveItem(uid,n)
	char n = (unsigned char)internalGetNumber(L);	
	unsigned short itemid = (unsigned short)internalGetNumber(L);
						
	ActionScript *action = getActionScript(L);
	
	//const KnownThing* tmp = action->GetItemByUID(itemid);
	Item* item = action->GetItemByUID(itemid);
	if(item){
		action->game->internalRemoveItem(item, n);
		
		lua_pushnumber(L, 0);
		return 1;
	}
	else{
#ifdef __DEBUG__
		std::cout << "luaDoRemoveItem: item not found" << std::endl;
#endif

		lua_pushnumber(L, -1);
		return 1;
	}

	//Item *tmpitem = NULL;
	/*PositionEx tmppos;
	if(item){
		//tmpitem = dynamic_cast<Item*>(tmp->thing);
		tmppos = item->getPosition();//tmp->pos;
		if(item->isSplash()){
			lua_pushnumber(L, -1);
			std::cout << "luaDoRemoveItem: can not remove a splash" << std::endl;
			return 1;
		}
	}
	else{
		lua_pushnumber(L, -1);
		std::cout << "luaDoRemoveItem: item not found" << std::endl;
		return 1;
	}*/
}

int ActionScript::luaActionDoPlayerRemoveItem(lua_State *L)
{
	//doPlayerRemoveItem(cid,itemid,count)
	long count = (unsigned char)internalGetNumber(L);	
	unsigned short itemId = (unsigned short)internalGetNumber(L);
	unsigned int cid = (unsigned int)internalGetNumber(L);						
	
	ActionScript *action = getActionScript(L);
	
	//const KnownThing* tmp = action->GetPlayerByUID(cid);
	Player* player = action->GetPlayerByUID(cid);
	if(player){
		//Player *player = dynamic_cast<Player*>(tmp->thing);
		if(player->removeItemTypeCount(itemId, count)){
			lua_pushnumber(L, 1);
		}
		else{
			lua_pushnumber(L, 0);
		}
	}
	else{
		lua_pushnumber(L, -1);
		std::cout << "luaDoPlayerRemoveItem: player not found" << std::endl;
		return 1;
	}	

	return 1;
}

int ActionScript::luaActionDoFeedPlayer(lua_State *L)
{	
	//doFeedPlayer(uid,food)
	int food = (int)internalGetNumber(L);
	unsigned int cid = (unsigned int)internalGetNumber(L);
	
	ActionScript *action = getActionScript(L);
	
	//const KnownThing* tmp = action->GetPlayerByUID(cid);
	Player* player = action->GetPlayerByUID(cid);
	if(player){
		//Player *player = dynamic_cast<Player*>(tmp->thing);
		player->food += food*1000;
	}
	else{
		lua_pushnumber(L, -1);
		std::cout << "luaDoFeedPlayer: player not found" << std::endl;
		return 1;
	}
	
	lua_pushnumber(L, 0);
	return 1;
}

int ActionScript::luaActionDoSendCancel(lua_State *L)
{	
	//doSendCancel(uid,text)
	const char * text = internalGetString(L);
	unsigned int cid = (unsigned int)internalGetNumber(L);	
	
	ActionScript *action = getActionScript(L);
	
	//const KnownThing* tmp = action->GetPlayerByUID(cid);
	const Player* player = action->GetPlayerByUID(cid);
	if(player){
		//Player *player = dynamic_cast<Player*>(tmp->thing);
		player->sendCancel(text);
	}
	else{
		lua_pushnumber(L, -1);
		std::cout << "luaSendCancel: player not found" << std::endl;
		return 1;
	}		
	lua_pushnumber(L, 0);
	return 1;
}


int ActionScript::luaActionDoTeleportThing(lua_State *L)
{
	//doTeleportThing(uid,newpos)
	PositionEx pos;
	internalGetPositionEx(L,pos);
	unsigned int id = (unsigned int)internalGetNumber(L);	
	
	ActionScript *action = getActionScript(L);
	//Thing *tmpthing;
	
	//const KnownThing* tmp = action->GetThingByUID(id);
	Thing* tmp = action->GetThingByUID(id);
	if(!tmp){
		lua_pushnumber(L, -1);
		std::cout << "luaTeleport: thing not found" << std::endl;
		return 1;
	}

	/*
	//std::cout << "new pos: " << (Position&)pos << std::endl;
	if(Item* item = tmp->getItem()){
		//avoid teleport notMoveable items
		//if((dynamic_cast<Item*>(tmp->thing))->isNotMoveable()){
		if(item->isNotMoveable()){
			lua_pushnumber(L, -1);
			std::cout << "luaTeleport: item is not moveable" << std::endl;
			return 1;
		}
	}
	*/
	
	if(action->game->internalTeleport(tmp,(Position&)pos) == RET_NOERROR){
		lua_pushnumber(L, 0);
		return 1;
	}
	
	std::cout << "luaTeleport: item is not moveable" << std::endl;
	lua_pushnumber(L, -1);
	return 1;
}

	
int ActionScript::luaActionDoTransformItem(lua_State *L)
{
	//doTransformItem(uid,toitemid)	
	unsigned int toId = (unsigned int)internalGetNumber(L);	
	unsigned int itemid = (unsigned int)internalGetNumber(L);	
	
	ActionScript *action = getActionScript(L);
	
	//const KnownThing* tmp = action->GetItemByUID(itemid);	
	Item* item = action->GetItemByUID(itemid);
	if(item){
		action->game->transformItem(item, toId);
		
		lua_pushnumber(L, 0);
		return 1;
	}
	else{
#ifdef __DEBUG__
		std::cout << "luaDoTransform: Item not found" << std::endl;
#endif

		lua_pushnumber(L, -1);
		return 1;
	}
}

int ActionScript::luaActionDoPlayerSay(lua_State *L)
{
	//doPlayerSay(uid,text,type)
	int type = (int)internalGetNumber(L);	
	const char * text = internalGetString(L);
	unsigned int cid = (unsigned int)internalGetNumber(L);	
					
	ActionScript *action = getActionScript(L);
	
	//const KnownThing* tmp = action->GetPlayerByUID(cid);
	Player* player = action->GetPlayerByUID(cid);
	if(player){
		//Player *player = dynamic_cast<Player*>(tmp->thing);
		action->game->internalCreatureSay(player,(SpeakClasses)type,std::string(text));
	}
	else{
		lua_pushnumber(L, -1);
		std::cout << "luaDoPlayerSay: player not found" << std::endl;
		return 1;
	}		
		
	lua_pushnumber(L, 0);
	return 1;
}

int ActionScript::luaActionDoSendMagicEffect(lua_State *L)
{
	//doSendMagicEffect(position,type)
	int type = (int)internalGetNumber(L);	
	PositionEx pos;
	internalGetPositionEx(L,pos);
	
	ActionScript *action = getActionScript(L);
	
	const Position& cylinderMapPos = internalGetRealPosition(action, action->_player,(Position&)pos);
	SpectatorVec list;
	SpectatorVec::iterator it;

	action->game->getSpectators(Range(cylinderMapPos, true), list);

	for(it = list.begin(); it != list.end(); ++it) {
		Player *p = (*it)->getPlayer();
		if(p)
			p->sendMagicEffect(cylinderMapPos,type);
	}	
	
	lua_pushnumber(L, 0);
	return 1;
}

int ActionScript::luaActionDoChangeTypeItem(lua_State *L)
{
	//doChangeTypeItem(uid,new_type)
	unsigned int subtype = (unsigned int)internalGetNumber(L);	
	unsigned int itemid = (unsigned int)internalGetNumber(L);
	
	ActionScript *action = getActionScript(L);
	
	//const KnownThing* tmp = action->GetItemByUID(itemid);
	Item* item = action->GetItemByUID(itemid);
	if(item){
		action->game->transformItem(item, item->getID(), subtype);
		
		lua_pushnumber(L, 0);
		return 1;		
	}
	else{
#ifdef __DEBUG__
		std::cout << "luaDoChangeTypeItem: Item not found" << std::endl;
#endif

		lua_pushnumber(L, -1);
		return 1;
	}
}


int ActionScript::luaActionDoPlayerAddSkillTry(lua_State *L)
{
	//doPlayerAddSkillTry(uid,skillid,n)
	int n = (int)internalGetNumber(L);
	int skillid = (int)internalGetNumber(L);
	unsigned int cid = (unsigned int)internalGetNumber(L);	
					
	ActionScript *action = getActionScript(L);
	
	Player* player = action->GetPlayerByUID(cid);
	if(player){
		//Player *player = dynamic_cast<Player*>(tmp->thing);
		player->addSkillTryInternal(n,skillid);
	}
	else{
		lua_pushnumber(L, -1);
		std::cout << "luaDoPlayerAddSkillTry: player not found" << std::endl;
		return 1;
	}		
		
	lua_pushnumber(L, 0);
	return 1;
}


int ActionScript::luaActionDoPlayerAddHealth(lua_State *L)
{
	//doPlayerAddHealth(uid,health)
	int addhealth = (int)internalGetNumber(L);
	unsigned int cid = (unsigned int)internalGetNumber(L);	
					
	ActionScript *action = getActionScript(L);
	
	//const KnownThing* tmp = action->GetPlayerByUID(cid);
	Player* player = action->GetPlayerByUID(cid);
	if(player){
		//Player *player = dynamic_cast<Player*>(tmp->thing);
		int tmp = player->health + addhealth;
		if(tmp <= 0){
			player->health = 1;
		}
		else if(tmp > player->healthmax){
			player->health = player->healthmax;
		}
		else{
			player->health = tmp;
		}
		player->sendStats();
		
		SpectatorVec list;
		SpectatorVec::iterator it;

		action->game->getSpectators(Range(player->getPosition(), true), list);
		for(it = list.begin(); it != list.end(); ++it) {
			Player* p = (*it)->getPlayer();
			if(p)
				p->sendCreatureHealth(player);
		}
	}
	else{
		lua_pushnumber(L, -1);
		std::cout << "luaDoPlayerAddHealth: player not found" << std::endl;
		return 1;
	}		
		
	lua_pushnumber(L, 0);
	return 1;
}

int ActionScript::luaActionDoPlayerAddMana(lua_State *L)
{
	//doPlayerAddMana(uid,mana)
	int addmana = (int)internalGetNumber(L);
	unsigned int cid = (unsigned int)internalGetNumber(L);	
					
	ActionScript *action = getActionScript(L);
	
	//const KnownThing* tmp = action->GetPlayerByUID(cid);
	Player* player = action->GetPlayerByUID(cid);
	if(player){
		//Player *player = dynamic_cast<Player*>(tmp->thing);
		player->mana = std::min(player->manamax,player->mana+addmana);
		player->sendStats();
	}
	else{
		lua_pushnumber(L, -1);
		std::cout << "luaDoPlayerAddMana: player not found" << std::endl;
		return 1;
	}		
		
	lua_pushnumber(L, 0);
	return 1;
}

int ActionScript::luaActionDoPlayerAddItem(lua_State *L)
{
	//doPlayerAddItem(uid,itemid,count or type)
	int type = (int)internalGetNumber(L);
	int itemid = (int)internalGetNumber(L);
	unsigned int cid = (unsigned int)internalGetNumber(L);	
	
	ActionScript* action = getActionScript(L);
	unsigned int uid;
	Player* player = action->GetPlayerByUID(cid);
	if(player){
		Item* newItem = Item::CreateItem(itemid, type);

		ReturnValue ret = action->game->internalAddItem(player, newItem);
		if(ret != RET_NOERROR){
			Tile* tile = player->getTile();
			ret = action->game->internalAddItem(tile, newItem);
			if(ret != RET_NOERROR){
				delete newItem;

				lua_pushnumber(L, -1);
#ifdef __DEBUG__
				std::cout << "luaDoPlayerAddItem: could not add item" << std::endl;
#endif
				return 1;
			}
		}
		
		if(newItem->getParent()){
			uid = action->AddThingToMap((Thing*)newItem);
		}
		else{
			//stackable item stacked with existing object, newItem will be released
			lua_pushnumber(L, -1);
			return 1;
		}
	}
	else{
		lua_pushnumber(L, -1);
		std::cout << "luaDoPlayerAddItem: player not found" << std::endl;
		return 1;
	}		

	lua_pushnumber(L, uid);
	return 1;
}


int ActionScript::luaActionDoPlayerSendTextMessage(lua_State *L)
{	
	//doPlayerSendTextMessage(uid,MessageClasses,message)
	const char * text = internalGetString(L);
	unsigned char messageClass = (unsigned char)internalGetNumber(L);	
	unsigned int cid = (unsigned int)internalGetNumber(L);	
	
	ActionScript *action = getActionScript(L);
	
	//const KnownThing* tmp = action->GetPlayerByUID(cid);
	Player* player = action->GetPlayerByUID(cid);
	if(player){
		//Player *player = dynamic_cast<Player*>(tmp->thing);
		player->sendTextMessage((MessageClasses)messageClass,text);;
	}
	else{
		lua_pushnumber(L, -1);
		std::cout << "luaSendTextMessage: player not found" << std::endl;
		return 1;
	}		
	lua_pushnumber(L, 0);
	return 1;
}

int ActionScript::luaActionDoSendAnimatedText(lua_State *L)
{	
	//doSendAnimatedText(position,text,color)
	int color = (int)internalGetNumber(L);
	const char * text = internalGetString(L);
	PositionEx pos;
	internalGetPositionEx(L,pos);
	
	ActionScript *action = getActionScript(L);
	
	const Position& cylinderMapPos = internalGetRealPosition(action, action->_player,(Position&)pos);
	SpectatorVec list;
	SpectatorVec::iterator it;

	action->game->getSpectators(Range(cylinderMapPos, true), list);

	for(it = list.begin(); it != list.end(); ++it) {
		Player *p = (*it)->getPlayer();
		if(p)
			p->sendAnimatedText(cylinderMapPos, color, text);
	}
	
	lua_pushnumber(L, 0);
	return 1;
}

int ActionScript::luaActionGetPlayerSkill(lua_State *L)
{
	//getPlayerSkill(uid,skillid)
	unsigned char skillid = (unsigned int)internalGetNumber(L);
	unsigned int cid = (unsigned int)internalGetNumber(L);
	
	ActionScript *action = getActionScript(L);
	
	//const KnownThing* tmp = action->GetPlayerByUID(cid);
	//if(tmp){
	Player* player = action->GetPlayerByUID(cid);
	if(player){
		if(skillid > 6){
			lua_pushnumber(L, -1);
			std::cout << "GetPlayerSkill: invalid skillid" << std::endl;
			return 1;
		}
		//Player *player = dynamic_cast<Player*>(tmp->thing);
		int value = player->skills[skillid][SKILL_LEVEL];
		lua_pushnumber(L,value);
		return 1;
	}
	else{
		lua_pushnumber(L, -1);
		std::cout << "GetPlayerSkill: player not found" << std::endl;
		return 1;
	}
}

int ActionScript::luaActionDoShowTextWindow(lua_State *L)
{
	//doShowTextWindow(uid,maxlen,canWrite)
	bool canWrite = (internalGetNumber(L) != 0);
	unsigned short maxlen = (unsigned short)internalGetNumber(L);
	unsigned int uid = (unsigned int)internalGetNumber(L);
	
	ActionScript *action = getActionScript(L);
	
	//const KnownThing* tmp = action->GetItemByUID(uid);
	Item* item = action->GetItemByUID(uid);
	if(item){
		action->_player->sendTextWindow(item, maxlen, canWrite);
		
		lua_pushnumber(L, 0);
		return 1;
	}
	else{
#ifdef __DEBUG__
		std::cout << "luadoShowTextWindow: Item not found" << std::endl;
#endif

		lua_pushnumber(L, -1);
		return 1;
	}
}

int ActionScript::luaActionGetItemRWInfo(lua_State *L)
{
	//getItemRWInfo(uid)
	unsigned int uid = (unsigned int)internalGetNumber(L);
	
	ActionScript *action = getActionScript(L);
	
	//const KnownThing* tmp = action->GetItemByUID(uid);
	Item* item = action->GetItemByUID(uid);
	//Item *tmpitem = NULL;
	if(item){
		int maxlen;
		lua_pushnumber(L, (int)(item->getRWInfo(maxlen)));
		return 1;
	}
	else{
#ifdef __DEBUG__
		std::cout << "luagetItemRWInfo: Item not found" << std::endl;
#endif

		lua_pushnumber(L, -1);
		return 1;
	}
}

int ActionScript::luaActionDoDecayItem(lua_State *L)
{
	//doDecayItem(uid)
	//Note: to stop decay set decayTo = 0 in items.xml
	unsigned int uid = (unsigned int)internalGetNumber(L);	
	
	ActionScript *action = getActionScript(L);
	
	//const KnownThing* tmp = action->GetItemByUID(uid);
	Item* item = action->GetItemByUID(uid);
	if(item){
		action->game->startDecay(item);
		
		lua_pushnumber(L, 0);
		return 1;
	}
	else{
#ifdef __DEBUG__
		std::cout << "luadoDecayItem: Item not found" << std::endl;
#endif

		lua_pushnumber(L, -1);
		return 1;
	}
}

int ActionScript::luaActionGetThingfromPos(lua_State *L)
{
	//getThingfromPos(pos)
	//Note: 
	//	stackpos = 255. Get the top thing(item moveable or creature).
	//	stackpos = 254. Get MagicFieldtItem
	//	stackpos = 253. Get Creature
	
	PositionEx pos;
	internalGetPositionEx(L,pos);
	
	ActionScript *action = getActionScript(L);
	
	//Tile *tile = action->game->getTile(pos.x, pos.y, pos.z);
	Tile *tile = action->game->map->getTile(pos);
	
	Thing *thing = NULL;
	
	if(tile){
		if(pos.stackpos == 255){
			//thing = tile->getTopMoveableThing();
			thing = tile->getTopCreature();
			
			if(thing == NULL){
				Item* item = tile->getTopDownItem();
				if(item && !item->isNotMoveable())
					thing = item;
			}
		}
		else if(pos.stackpos == 254){
			thing = tile->getFieldItem();
		}
		else if(pos.stackpos == 253){
			thing = tile->getTopCreature();
		}
		else{
			thing = tile->__getThing(pos.stackpos);
		}
		
		if(thing){
			/*if(pos.stackpos > 250){
				pos.stackpos = tile->__getIndexOfThing(thing);
			}*/
			unsigned int thingid = action->AddThingToMap(thing);
			internalAddThing(L,thing,thingid);
		}
		else{
			internalAddThing(L,NULL,0);	
		}
		return 1;
		
	}//if(tile)
	else{
		std::cout << "luagetItemfromPos: Tile not found" << std::endl;
		internalAddThing(L,NULL,0);
		return 1;
	}
}

int ActionScript::luaActionDoCreateItem(lua_State *L)
{
	//doCreateItem(itemid,type or count,position) .only working on ground. returns uid of the created item
	PositionEx pos;
	internalGetPositionEx(L,pos);
	int type = (int)internalGetNumber(L);
	int itemid = (int)internalGetNumber(L);
	
	ActionScript *action = getActionScript(L);
	
	Tile* tile = action->game->map->getTile(pos);
	if(!tile){
		lua_pushnumber(L, 0);
		return 0;
	}
	
	Item* newItem = Item::CreateItem(itemid, type);
	
	ReturnValue ret = action->game->internalAddItem(tile, newItem);
	if(ret != RET_NOERROR){
		delete newItem;
		lua_pushnumber(L, 0);
		return 0;
	}
	
	if(newItem->getParent()){
		unsigned int uid = action->AddThingToMap((Thing*)newItem);
		lua_pushnumber(L, uid);
		return 1;	
	}
	else{
		//stackable item stacked with existing object, newItem will be released
		lua_pushnumber(L, -1);
		return 1;
	}
}

int ActionScript::luaActionGetPlayerStorageValue(lua_State *L)
{
	//getPlayerStorageValue(cid,valueid)
	unsigned long key = (unsigned int)internalGetNumber(L);
	unsigned int cid = (unsigned int)internalGetNumber(L);
	
	ActionScript *action = getActionScript(L);
	
	//const KnownThing* tmp = action->GetPlayerByUID(cid);
	Player* player = action->GetPlayerByUID(cid);
	if(player){
		//Player *player = dynamic_cast<Player*>(tmp->thing);
		long value;
		if(player->getStorageValue(key,value)){
			lua_pushnumber(L,value);
		}
		else{
			lua_pushnumber(L,-1);
		}
		return 1;
	}
	else{
		lua_pushnumber(L, -1);
		std::cout << "GetPlayerStorageValue: player not found" << std::endl;
		return 1;
	}
}

int ActionScript::luaActionSetPlayerStorageValue(lua_State *L)
{
	//setPlayerStorageValue(cid,valueid, newvalue)
	long value = (unsigned int)internalGetNumber(L);
	unsigned long key = (unsigned int)internalGetNumber(L);
	unsigned int cid = (unsigned int)internalGetNumber(L);
	
	ActionScript *action = getActionScript(L);
	
	//const KnownThing* tmp = action->GetPlayerByUID(cid);
	Player* player = action->GetPlayerByUID(cid);
	if(player){
		//Player *player = dynamic_cast<Player*>(tmp->thing);
		player->addStorageValue(key,value);
	}
	else{
		lua_pushnumber(L, -1);
		std::cout << "SetPlayerStorageValue: player not found" << std::endl;
		return 1;
	}
	lua_pushnumber(L,0);
	return 1;
}

int ActionScript::luaActionDoSetItemActionId(lua_State *L)
{
	//doSetItemActionId(uid,actionid)
	unsigned int actionid = (unsigned int)internalGetNumber(L);	
	unsigned int itemid = (unsigned int)internalGetNumber(L);
	
	ActionScript *action = getActionScript(L);
	
	//const KnownThing* tmp = action->GetItemByUID(itemid);	
	Item* item = action->GetItemByUID(itemid);	
	//Item *tmpitem = NULL;
	if(item){
		item->setActionId(actionid);
		
		lua_pushnumber(L, 0);
		return 1;
	}
	else{
#ifdef __DEBUG__
		std::cout << "luaDoSetActionId: Item not found" << std::endl;
#endif

		lua_pushnumber(L, -1);
		return 1;
	}
}

int ActionScript::luaActionDoSetItemText(lua_State *L)
{
	//doSetItemText(uid,text)
	const char *text = internalGetString(L);
	unsigned int itemid = (unsigned int)internalGetNumber(L);	
	
	ActionScript *action = getActionScript(L);
	
	//const KnownThing* tmp = action->GetItemByUID(itemid);
	//Item *tmpitem = NULL;
	Item* item = action->GetItemByUID(itemid);
	if(item){
		std::string str(text);
		item->setText(str);
		
		lua_pushnumber(L, 0);
		return 1;
	}
	else{
#ifdef __DEBUG__
		std::cout << "luaDoSetText: Item not found" << std::endl;
#endif

		lua_pushnumber(L, -1);
		return 1;
	}
}

int ActionScript::luaActionDoSetItemSpecialDescription(lua_State *L)
{
	//doSetItemSpecialDescription(uid,desc)
	const char *desc = internalGetString(L);
	unsigned int itemid = (unsigned int)internalGetNumber(L);
	
	ActionScript *action = getActionScript(L);
	
	//const KnownThing* tmp = action->GetItemByUID(itemid);
	Item* item = action->GetItemByUID(itemid);
	if(item){
		std::string str(desc);
		item->setSpecialDescription(str);
		
		lua_pushnumber(L, 0);
		return 1;
	}
	else{
#ifdef __DEBUG__
		std::cout << "luaDoSetSpecialDescription: Item not found" << std::endl;
#endif

		lua_pushnumber(L, -1);
		return 1;
	}
}

int ActionScript::luaActionGetTilePzInfo(lua_State *L)
{
	//getTilePzInfo(pos)
	PositionEx pos;
	internalGetPositionEx(L,pos);
	
	ActionScript *action = getActionScript(L);
	
	//Tile *tile = action->game->getTile(pos.x, pos.y, pos.z);
	Tile *tile = action->game->map->getTile(pos);
	
	if(tile){
		if(tile->isPz()){
			lua_pushnumber(L, 1);
		}
		else{
			lua_pushnumber(L, 0);
		}
	}//if(tile)
	else{
		std::cout << "luagetTilePzInfo: Tile not found" << std::endl;
		lua_pushnumber(L, -1);
	}
	return 1;
}

int ActionScript::luaActionDoSummonCreature(lua_State *L){
	//doSummonCreature(name, position)
	PositionEx pos;
	internalGetPositionEx(L,pos);
	const char *name = internalGetString(L);
	
	ActionScript *action = getActionScript(L);
	
	Monster* monster = Monster::createMonster(name);

	if(!monster){
		lua_pushnumber(L, 0);
		std::cout << "luadoSummonCreature: Monster not found" << std::endl;
		return 1;
	}
	
	if(!action->game->placeCreature((Position&)pos, monster)){
		delete monster;
		lua_pushnumber(L, 0);
		std::cout << "luadoSummonCreature: Can not place the monster" << std::endl;
		return 1;
	}
	
	unsigned int cid = action->AddThingToMap((Thing*)monster/*,pos*/);
	
	lua_pushnumber(L, cid);
	return 1;	
}


int ActionScript::luaActionDoPlayerRemoveMoney(lua_State *L)
{
	//doPlayerRemoveMoney(uid,money)
	int money = (int)internalGetNumber(L);
	unsigned int cid = (unsigned int)internalGetNumber(L);	
					
	ActionScript *action = getActionScript(L);
	
	Player* player = action->GetPlayerByUID(cid);
	if(player){
		//Player *player = tmp->thing->getCreature()->getPlayer();
		if(player->substractMoney(money)){
			lua_pushnumber(L, 1);
		}
		else{
			lua_pushnumber(L, 0);
		}
	}
	else{
		lua_pushnumber(L, -1);
		std::cout << "doPlayerRemoveMoney: player not found" << std::endl;
		return 1;
	}

	return 1;
}

int ActionScript::luaActionDoPlayerSetMasterPos(lua_State *L)
{
	//doPlayerSetMasterPos(cid,pos)
	PositionEx pos;
	internalGetPositionEx(L,pos);
	unsigned int cid = (unsigned int)internalGetNumber(L);	
	
	ActionScript *action = getActionScript(L);
	
	//const KnownThing* tmp = action->GetPlayerByUID(cid);
	Player* player = action->GetPlayerByUID(cid);
	if(player){
		//Player *player = dynamic_cast<Player*>(tmp->thing);
		player->masterPos = pos;
	}
	else{
		lua_pushnumber(L, -1);
		std::cout << "doPlayerSetMasterPos: player not found" << std::endl;
		return 1;
	}
	lua_pushnumber(L, 0);
	return 1;
}

int ActionScript::luaActionDoPlayerSetVocation(lua_State *L)
{
	//doPlayerSetVocation(cid,voc)
	int voc = (int)internalGetNumber(L);
	unsigned int cid = (unsigned int)internalGetNumber(L);	
					
	ActionScript *action = getActionScript(L);
	
	//const KnownThing* tmp = action->GetPlayerByUID(cid);
	Player* player = action->GetPlayerByUID(cid);
	if(player){
		//Player *player = dynamic_cast<Player*>(tmp->thing);
		player->vocation = (playervoc_t)voc;
	}
	else{
		lua_pushnumber(L, -1);
		std::cout << "doPlayerSetVocation: player not found" << std::endl;
		return 1;
	}
	
	lua_pushnumber(L, 0);
	return 1;
}


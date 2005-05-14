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
#include "const74.h"
#include "player.h"
#include "monster.h"
#include "npc.h"
#include "game.h"
#include "item.h"

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
	ActionUseMap::iterator it = useItemMap.begin();
	while(it != useItemMap.end()) {
		delete it->second;
		useItemMap.erase(it);
		it = useItemMap.begin();
	}	
}


bool Actions::loadFromXml()
{
	this->loaded = false;
	Action *action = NULL;
	
	std::string filename = "data/actions/actions.xml";
	std::transform(filename.begin(), filename.end(), filename.begin(), tolower);
	xmlDocPtr doc = xmlParseFile(filename.c_str());

	if (doc){
		this->loaded=true;
		xmlNodePtr root, p, tmp;
		root = xmlDocGetRootElement(doc);
		
		if (xmlStrcmp(root->name,(const xmlChar*) "actions")){			
			xmlFreeDoc(doc);
			return -1;
		}		
		p = root->children;
            
		while (p)
		{
			const char* str = (char*)p->name;
			
			if (strcmp(str, "action") == 0){
				int itemid;
				if(readXMLInteger(p,"itemid",itemid)){
					const char* scriptfile = (const char*)xmlGetProp(p,(xmlChar*)"script");					
					action = new Action(game,std::string("data/actions/scripts/") + scriptfile);
					if(action->isLoaded()){
						useItemMap[itemid] = action;
					}
					else{						
						delete action;
					}
					action = NULL;
				}
				else{
					//
				}
			}			
			p = p->next;    
		}
		
		xmlFreeDoc(doc);
	}  
	return this->loaded;
}

void Actions::UseItem(Player* player, const Position &pos,const unsigned char stack, 
	unsigned short itemid)
{	
	Item *item = dynamic_cast<Item*>(game->getThing(pos,stack,player));
	if(!item)
		return;
	
	if(item->getID() != itemid)
		return;
	
	ActionUseMap::iterator it = useItemMap.find(itemid);
	
    if (it != useItemMap.end()){
		PositionEx posEx(pos,stack);
    	if(!it->second->getScript()->execute(player,item,posEx,posEx)){
			player->sendCancel("You can not use this object.");
    		return;
		}
	}
    else{
		//not found
		player->sendCancel("You can not use this object.");
    	return;
	}
    
    return;
	
}

void Actions::UseItemEx(Player* player, const Position &from_pos,
	const unsigned char from_stack,const Position &to_pos,
	const unsigned char to_stack,const unsigned short itemid)
{
	Item *item = dynamic_cast<Item*>(game->getThing(from_pos,from_stack,player));
	if(!item)
		return;
	
	if(item->getID() != itemid)
		return;
	
	ActionUseMap::iterator it = useItemMap.find(itemid);
	
    if (it != useItemMap.end()){
		PositionEx posFromEx(from_pos,from_stack);
		PositionEx posToEx(to_pos,to_stack);    	
    	if(!it->second->getScript()->execute(player,item,posFromEx,posToEx)){
			player->sendCancel("You can not use this object.");
    		return;
		}
	}
    else{
		//not found
		player->sendCancel("You can not use this object.");
    	return;
	}
}

//

bool readXMLInteger(xmlNodePtr p, const char *tag, int &value)
{
	const char* sinteger = (const char*)xmlGetProp(p, (xmlChar*)tag);	
	if(!sinteger)
		return false;
	else{
		unsigned short integer = atoi(sinteger);		
		value = integer;
		return true;		
	}
}

Action::Action(Game* igame,std::string scriptname):
game(igame)
{
	loaded = false;
	lastuid = 0;
	script = new ActionScript(this,scriptname);
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

void Action::ClearMap()
{
	std::map<unsigned int,KnownThing*>::iterator it;
	for(it = ThingMap.begin(); it != ThingMap.end();it++ ){
		delete it->second;
		it->second == NULL;
	}
	ThingMap.clear();
}

int Action::AddThingToMap(Thing *thing,PositionEx &pos)
{
	std::map<unsigned int,KnownThing*>::iterator it;
	for(it = ThingMap.begin(); it != ThingMap.end();it++ ){
		if(it->second->thing == thing){
			return it->first;
		}
	}
	lastuid++;
	KnownThing *tmp = new KnownThing;	
	tmp->thing = thing;
	tmp->pos = pos;
	if(dynamic_cast<Item*>(thing))
		tmp->type = thingTypeItem;	
	else if(dynamic_cast<Player*>(thing))
		tmp->type = thingTypePlayer;	
	else if(dynamic_cast<Monster*>(thing))
		tmp->type = thingTypeMonster;	
	else if(dynamic_cast<Npc*>(thing))
		tmp->type = thingTypeNpc;	
	else
		tmp->type = thingTypeUnknown;	
	
	ThingMap[lastuid] = tmp;
	return lastuid;
}

const KnownThing* Action::GetThingByUID(int uid)
{
	KnownThing *tmp = ThingMap[uid];
	if(tmp)
		return tmp;	
	return NULL;
}

const KnownThing* Action::GetItemByUID(int uid)
{
	KnownThing *tmp = ThingMap[uid];
	if(tmp){
		if(tmp->type == thingTypeItem)
			return tmp;
	}		
	return NULL;
}

const KnownThing* Action::GetCreatureByUID(int uid)
{
	KnownThing *tmp = ThingMap[uid];
	if(tmp){
		if(tmp->type == thingTypePlayer || tmp->type == thingTypeMonster
			|| tmp->type == thingTypeNpc )
			return tmp;
	}		
	return NULL;
}

const KnownThing* Action::GetPlayerByUID(int uid)
{
	KnownThing *tmp = ThingMap[uid];
	if(tmp){
		if(tmp->type == thingTypePlayer)
			return tmp;
	}		
	return NULL;
}

ActionScript::ActionScript(Action *iaction,std::string scriptname)
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
    lua_dofile(luaState, "data/actions/lib/actions.lua");
    
	FILE* in=fopen(scriptname.c_str(), "r");
	if(!in){
		std::cout << "Error: Can not open " << scriptname.c_str() << std::endl;
		return;
	}
	else
		fclose(in);
	lua_dofile(luaState, scriptname.c_str());
	this->_action = iaction;
	this->setGlobalNumber("addressOfAction", (int)iaction);
	this->loaded = true;
	this->registerFunctions();
}

int ActionScript::registerFunctions()
{
	//doRemoveItem(uid,n)
	lua_register(luaState, "doRemoveItem", ActionScript::luaActionDoRemoveItem);
	//doPlayerFeed(uid,food)
	lua_register(luaState, "doPlayerFeed", ActionScript::luaActionDoFeedPlayer);
	//getPlayerFood(uid)
	lua_register(luaState, "getPlayerFood", ActionScript::luaActionGetPlayerFood);
	//doPlayerSendCancel(uid,text)
	lua_register(luaState, "doPlayerSendCancel", ActionScript::luaActionDoSendCancel);
	//doTeleportThing(uid,newpos)
	lua_register(luaState, "doTeleportThing", ActionScript::luaActionDoTeleportThing);
	//doTransformItem(uid,toitemid)	
	lua_register(luaState, "doTransformItem", ActionScript::luaActionDoTransformItem);
	//doPlayerAddItem(uid,itemid)
	//doPlayerAddSkillTry(uid,skillid)
	//doPlayerHealth(uid,health)
	//doPlayerMana(uid,mana)
	//getItemIDfromPos(pos,stack)
	//getPlayerPosition(uid)
	//doPlayerSendTextMessage(cid,MessageClasses,message)
	//doPlayerSendMagicEffect(uid,type)	
	//doPlayerSendAnimatedText(uid,text)
	//doPlayerSay(uid,text)
	//doDecayItem(uid,toitemid,time)
	//doCreateItem(item{itemid,count}, position)
	//doSummonCreature(name, position)	
	//doMoveItem(uid,toPos)
	//getPlayerHealth(cid)	
	//getPlayerMana(cid)
	//doPlayerAddCondition(....)	
	//getPlayerLevel(cid)
	//getPlayerMagLevel(cid)
	//getPlayerName(cid)
	//getPlayerItemCount(itemid)
	//doPlayerRemoveItem(itemid,count)
	
	return true;
}

Action* ActionScript::getAction(lua_State *L){
	lua_getglobal(L, "addressOfAction");
	int val = (int)lua_tonumber(L, -1);
	lua_pop(L,1);

	Action* myaction = (Action*) val;
	if(!myaction){
		return 0;
	}
	return myaction;
}

bool ActionScript::execute(Player *player,Item* item, PositionEx &posFrom, PositionEx &posTo)
{
	//onUse(uidplayer, item1,position1,item2,position2)
	_action->ClearMap();
	_action->_player = player;
	PositionEx playerpos = player->pos;
	unsigned int cid = _action->AddThingToMap((Thing*)player,playerpos);
	unsigned int itemid1 = _action->AddThingToMap(item,posFrom);
	
	lua_pushstring(luaState, "onUse");
	lua_gettable(luaState, LUA_GLOBALSINDEX);
	
	lua_pushnumber(luaState, cid);
	internalAddItem(luaState,item,itemid1);
	internalAddPositionEx(luaState,posFrom);	
	//std::cout << "posTo" <<  (Position)posTo << " stack" << (int)posTo.stackpos <<std::endl;
	Thing *thing = _action->game->getThing((Position)posTo,posTo.stackpos,player);
	Item *item2 = NULL;
	if(thing)
		item2 = dynamic_cast<Item*>(thing);	
	if(posFrom != posTo && item2){		
		int itemid2 = _action->AddThingToMap(item2,posTo);
		internalAddItem(luaState,item2,itemid2);
		internalAddPositionEx(luaState,posTo);	
	}
	else{		
		internalAddItem(luaState,NULL,0);
		PositionEx posEx;
		internalAddPositionEx(luaState,posEx);		
	}
	
	lua_pcall(luaState, 5, 1, 0);
	
	bool ret = (bool)lua_tonumber(luaState, -1);
	lua_pop(luaState, 1);

	return ret;
}

void ActionScript::internalAddItem(lua_State *L, Item* item, unsigned int itemid)
{
	lua_newtable(luaState);
	if(item){		
		setField("uid", itemid);
		setField("itemid", item->getID());
		setField("type", item->getItemCountOrSubtype());
	}
	else{
		setField("uid", 0);
		setField("itemid", 0);
		setField("type", 0);
	}
}

void ActionScript::internalAddPositionEx(lua_State *L, PositionEx& pos)
{
	lua_newtable(L);
	setField("z", pos.z);
	setField("y", pos.y);
	setField("x", pos.x);
	setField("stackpos",pos.stackpos);
}

void ActionScript::internalGetPositionEx(lua_State *L, PositionEx& pos)
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

	lua_pushstring(L, "stackpos");
	lua_gettable(L, -2);
	pos.stackpos = (int)lua_tonumber(L, -1);
	lua_pop(L, 1);

	lua_pop(L, 1); //table
}


int ActionScript::luaActionDoRemoveItem(lua_State *L)
{	
	//doRemoveItem(uid,n)
	unsigned char n = (unsigned char)lua_tonumber(L, -1);
	lua_pop(L,1);
	
	unsigned short itemid = (unsigned short)lua_tonumber(L, -1);
	lua_pop(L,1);
						
	Action *action = getAction(L);
	
	const KnownThing* tmp = action->GetItemByUID(itemid);	
	Item *tmpitem = NULL;
	PositionEx tmppos;
	if(tmp){
		tmpitem = (Item*)tmp->thing;
		tmppos = tmp->pos;
	}
	else{
		lua_pushnumber(L, -1);
		std::cout << "luaDoRemove: item not found" << std::endl;
		return 1;
	}
	
	if(tmpitem->isStackable() && n < tmpitem->getItemCountOrSubtype()){
		tmpitem->setItemCountOrSubtype(tmpitem->getItemCountOrSubtype() - n);
		action->game->sendUpdateThing(action->_player,(Position&)tmppos,tmpitem,tmppos.stackpos);
	}
	else{
		action->game->sendRemoveThing(action->_player,(Position&)tmppos,tmpitem,tmppos.stackpos);
		action->game->removeThing(action->_player,(Position&)tmppos,(Thing*)tmpitem);
		action->game->FreeThing(tmpitem);
	}	
	
	lua_pushnumber(L, 0);
	return 1;
}


int ActionScript::luaActionDoFeedPlayer(lua_State *L)
{	
	//doFeedPlayer(uid,food)
	int food = (int)lua_tonumber(L, -1);
	lua_pop(L,1);
	
	unsigned int cid = (unsigned int)lua_tonumber(L, -1);
	lua_pop(L,1);
					
	Action *action = getAction(L);
	
	const KnownThing* tmp = action->GetPlayerByUID(cid);
	if(tmp){
		Player *player = (Player*)(tmp->thing);
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

int ActionScript::luaActionGetPlayerFood(lua_State *L)
{	
	//getPlayerFood(uid)	
	unsigned int cid = (unsigned int)lua_tonumber(L, -1);
	lua_pop(L,1);
					
	Action *action = getAction(L);
	
	const KnownThing* tmp = action->GetPlayerByUID(cid);
	if(tmp){
		Player *player = (Player*)(tmp->thing);
		lua_pushnumber(L, (player->food)/1000);
		return 1;
	}
	else{
		lua_pushnumber(L, -1);
		std::cout << "luaGetPlayerFood: player not found" << std::endl;
		return 1;
	}		
}

int ActionScript::luaActionDoSendCancel(lua_State *L)
{	
	//doSendCancel(uid,text)
	const char * text = lua_tostring(L, -1);
	lua_pop(L,1);
	
	unsigned int cid = (unsigned int)lua_tonumber(L, -1);
	lua_pop(L,1);	
	
	Action *action = getAction(L);
	
	const KnownThing* tmp = action->GetPlayerByUID(cid);
	if(tmp){
		Player *player = (Player*)(tmp->thing);
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
	
	unsigned int id = (unsigned int)lua_tonumber(L, -1);
	lua_pop(L,1);
	
	Action *action = getAction(L);
	
	Thing *tmpthing;
	
	const KnownThing* tmp = action->GetThingByUID(id);
	if(tmp){
		tmpthing = tmp->thing;		
	}
	else{
		lua_pushnumber(L, -1);
		std::cout << "luaTeleport: thing not found" << std::endl;
		return 1;
	}
	
	action->game->teleport(tmpthing,(Position&)pos);
	
	lua_pushnumber(L, 0);
	return 1;
}

	
int ActionScript::luaActionDoTransformItem(lua_State *L)
{
	//doTransformItem(uid,toitemid)	
	unsigned int toid = (unsigned int)lua_tonumber(L, -1);
	lua_pop(L,1);
	
	unsigned int itemid = (unsigned int)lua_tonumber(L, -1);
	lua_pop(L,1);
	
	Action *action = getAction(L);
	
	const KnownThing* tmp = action->GetItemByUID(itemid);	
	Item *tmpitem = NULL;
	PositionEx tmppos;
	if(tmp){
		tmpitem = (Item*)tmp->thing;
		tmppos = tmp->pos;		
	}
	else{
		lua_pushnumber(L, -1);
		std::cout << "luaDoTransform: Item not found" << std::endl;
		return 1;
	}
	
	tmpitem->setID(toid);
	
	action->game->sendUpdateThing(action->_player,(Position&)tmppos,tmpitem,tmppos.stackpos);
	
	lua_pushnumber(L, 0);
	return 1;
}

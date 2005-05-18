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
		xmlNodePtr root, p;
		root = xmlDocGetRootElement(doc);
		
		if (xmlStrcmp(root->name,(const xmlChar*) "actions")){			
			xmlFreeDoc(doc);
			return false;
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
					//error message
				}
			}
			p = p->next;    
		}
		
		xmlFreeDoc(doc);
	}  
	return this->loaded;
}

int Actions::canUse(const Player *player,const Position &pos) const
{
	if(pos.x != 0xFFFF){
		int dist_x = std::abs(pos.x - player->pos.x);
		int dist_y = std::abs(pos.y - player->pos.y);
		if(dist_x > 1 || dist_y > 1 || (pos.z != player->pos.z)){
			return 1;
		}
	}
	return 0;
}

void Actions::UseItem(Player* player, const Position &pos,const unsigned char stack, 
	unsigned short itemid)
{	
	if(canUse(player,pos)== 1){
		player->sendCancel("Too far away.");
		return;
	}
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
	if(canUse(player,from_pos)== 1){
		player->sendCancel("Too far away.");
		return;
	}
	
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
		it->second = NULL;
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
	//getPlayerPosition(uid)
	lua_register(luaState, "getPlayerPosition", ActionScript::luaActionGetPlayerPosition);
	//getPlayerSkill(uid,skillid)
	lua_register(luaState, "getPlayerSkill", ActionScript::luaActionGetPlayerSkill);
	//getPlayerItemCount(uid,itemid)
	
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
	//doPlayerSendMagicEffect(uid,position,type)
	lua_register(luaState, "doSendMagicEffect", ActionScript::luaActionDoSendMagicEffect);
	//doChangeTypeItem(uid,new_type)
	lua_register(luaState, "doChangeTypeItem", ActionScript::luaActionDoChangeTypeItem);

	//doSendAnimatedText(position,text,color)
	lua_register(luaState, "doSendAnimatedText", ActionScript::luaActionDoSendAnimatedText);
	//doPlayerAddSkillTry(uid,skillid,n)
	lua_register(luaState, "doPlayerAddSkillTry", ActionScript::luaActionDoPlayerAddSkillTry);
	//doPlayerAddHealth(uid,health)
	lua_register(luaState, "doPlayerAddHealth", ActionScript::luaActionDoPlayerAddHealth);
	//doPlayerAddMana(uid,mana)
	lua_register(luaState, "doPlayerAddMana", ActionScript::luaActionDoPlayerAddMana);
	//doPlayerAddItem(uid,itemid,count or type)
	lua_register(luaState, "doPlayerAddItem", ActionScript::luaActionDoPlayerAddItem);
	//doPlayerSendTextMessage(uid,MessageClasses,message)
	lua_register(luaState, "doPlayerSendTextMessage", ActionScript::luaActionDoPlayerSendTextMessage);
		
	//getItemIDfromPos(pos,stack)	
	//doDecayItem(uid,toitemid,time)
	//doCreateItem(item{itemid,count}, position)
	//doSummonCreature(name, position)	
	//doMoveItem(uid,toPos)
	//doShowTextWindow(uid,canbemod)	
	//doPlayerAddCondition(....)		
	
	//doPlayerRemoveItem(itemid,count)
	
	return true;
}

Action* ActionScript::getAction(lua_State *L){
	lua_getglobal(L, "addressOfAction");
	int val = (int)internalGetNumber(L);

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
	internalAddThing(luaState,item,itemid1);
	internalAddPositionEx(luaState,posFrom);	
	//std::cout << "posTo" <<  (Position)posTo << " stack" << (int)posTo.stackpos <<std::endl;
	Thing *thing = _action->game->getThing((Position)posTo,posTo.stackpos,player);	
	if(thing){		
		if(posFrom != posTo){
			int thingId2 = _action->AddThingToMap(thing,posTo);
			internalAddThing(luaState,thing,thingId2);
			internalAddPositionEx(luaState,posTo);	
		}
	}
	else{		
		internalAddThing(luaState,NULL,0);
		PositionEx posEx;
		internalAddPositionEx(luaState,posEx);		
	}
	
	lua_pcall(luaState, 5, 1, 0);
	
	bool ret = (bool)lua_tonumber(luaState, -1);
	lua_pop(luaState, 1);

	return ret;
}

Position ActionScript::internalGetRealPosition(Player *player, const Position &pos)
{
	if(pos.x == 0xFFFF){
		Position dummyPos(0,0,0);
		if(!player)
			return dummyPos;
		if(pos.y & 0x40) { //from container						
			unsigned char containerid = pos.y & 0x0F;
			const Container* container = player->getContainer(containerid);
			if(!container){
				return dummyPos;
			}			
			while(container->getParent() != NULL) {				
				container = container->getParent();				
			}			
			if(container->pos.x == 0xFFFF)				
				return player->pos;			
			else
				return container->pos;
		}
		else //from inventory
		{
			return player->pos;
		}
	}
	else{
		return pos;
	}
}

void ActionScript::internalAddThing(lua_State *L, const Thing* thing, const unsigned int thingid)
{	
	lua_newtable(L);
	if(dynamic_cast<const Item*>(thing)){	
		const Item *item = dynamic_cast<const Item*>(thing);
		setField(L,"uid", thingid);
		setField(L,"itemid", item->getID());
		setField(L,"type", item->getItemCountOrSubtype());
	}
	else if(dynamic_cast<const Creature*>(thing)){
		setField(L,"uid", thingid);
		setField(L,"itemid", 1);
		setField(L,"type", 0);	//can be used to specify monster,npc or player?
	}	
	else{
		setField(L,"uid", 0);
		setField(L,"itemid", 0);
		setField(L,"type", 0);
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
	Action *action = getAction(L);
	int value;	
	
	const KnownThing* tmp = action->GetPlayerByUID(cid);
	if(tmp){
		PositionEx pos;
		Tile *tile;
		Player *player = (Player*)(tmp->thing);
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
			pos = player->pos;
			tile = action->game->map->getTile(player->pos.x, player->pos.y, player->pos.z);
			if(tile)
				pos.stackpos = tile->getCreatureStackPos(player);
			internalAddPositionEx(L,pos);
			return 1;
			break;
		case PlayerInfoFood:
			value = player->food/1000;
			break;
		default:
			std::cout << "GetPlayerInfo: Unkown player info " << info << std::endl;
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
	
//

int ActionScript::luaActionDoRemoveItem(lua_State *L)
{	
	//doRemoveItem(uid,n)
	char n = (unsigned char)internalGetNumber(L);	
	unsigned short itemid = (unsigned short)internalGetNumber(L);
						
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
		std::cout << "luaDoRemoveItem: item not found" << std::endl;
		return 1;
	}
	
	if(tmpitem->isStackable() && (tmpitem->getItemCountOrSubtype() - n) > 0){
		tmpitem->setItemCountOrSubtype(tmpitem->getItemCountOrSubtype() - n);
		action->game->sendUpdateThing(action->_player,(Position&)tmppos,tmpitem,tmppos.stackpos);
	}
	else{
		action->game->sendRemoveThing(action->_player,(Position&)tmppos,tmpitem,tmppos.stackpos);
		action->game->removeThing(action->_player,(Position&)tmppos,tmpitem);
		action->game->FreeThing(tmpitem);
	}	
	
	lua_pushnumber(L, 0);
	return 1;
}


int ActionScript::luaActionDoFeedPlayer(lua_State *L)
{	
	//doFeedPlayer(uid,food)
	int food = (int)internalGetNumber(L);
	unsigned int cid = (unsigned int)internalGetNumber(L);
					
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

int ActionScript::luaActionDoSendCancel(lua_State *L)
{	
	//doSendCancel(uid,text)
	const char * text = internalGetString(L);
	unsigned int cid = (unsigned int)internalGetNumber(L);	
	
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
	unsigned int id = (unsigned int)internalGetNumber(L);	
	
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
	unsigned int toid = (unsigned int)internalGetNumber(L);	
	unsigned int itemid = (unsigned int)internalGetNumber(L);	
	
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

int ActionScript::luaActionDoPlayerSay(lua_State *L)
{
	//doPlayerSay(uid,text,type)
	int type = (int)internalGetNumber(L);	
	const char * text = internalGetString(L);
	unsigned int cid = (unsigned int)internalGetNumber(L);	
					
	Action *action = getAction(L);
	
	const KnownThing* tmp = action->GetPlayerByUID(cid);
	if(tmp){
		Player *player = (Player*)(tmp->thing);
		action->game->creatureSay(player,(SpeakClasses)type,std::string(text));
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
	
	Action *action = getAction(L);	
	
	Position realpos = internalGetRealPosition(action->_player,(Position&)pos);
	std::vector<Creature*> list;
	action->game->getSpectators(Range(realpos, true), list);
	for(unsigned int i = 0; i < list.size(); ++i){
		Player *p = dynamic_cast<Player*>(list[i]);
		if(p)
			p->sendMagicEffect(realpos,type);
	}	
	
	lua_pushnumber(L, 0);
	return 1;
}

int ActionScript::luaActionDoChangeTypeItem(lua_State *L)
{
	//doChangeTypeItem(uid,new_type)
	unsigned int new_type = (unsigned int)internalGetNumber(L);	
	unsigned int itemid = (unsigned int)internalGetNumber(L);
	
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
		std::cout << "luaDoChangeTypeItem: Item not found" << std::endl;
		return 1;
	}
	
	tmpitem->setItemCountOrSubtype(new_type);
	
	action->game->sendUpdateThing(action->_player,(Position&)tmppos,tmpitem,tmppos.stackpos);
	
	lua_pushnumber(L, 0);
	return 1;		
}


int ActionScript::luaActionDoPlayerAddSkillTry(lua_State *L)
{
	//doPlayerAddSkillTry(uid,skillid,n)
	int n = (int)internalGetNumber(L);
	int skillid = (int)internalGetNumber(L);
	unsigned int cid = (unsigned int)internalGetNumber(L);	
					
	Action *action = getAction(L);
	
	const KnownThing* tmp = action->GetPlayerByUID(cid);
	if(tmp){
		Player *player = (Player*)(tmp->thing);
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
					
	Action *action = getAction(L);
	
	const KnownThing* tmp = action->GetPlayerByUID(cid);
	if(tmp){
		Player *player = (Player*)(tmp->thing);
		player->health = std::min(player->healthmax,player->health+addhealth);
		player->sendStats();
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
					
	Action *action = getAction(L);
	
	const KnownThing* tmp = action->GetPlayerByUID(cid);
	if(tmp){
		Player *player = (Player*)(tmp->thing);
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
	
	Action *action = getAction(L);
	
	const KnownThing* tmp = action->GetPlayerByUID(cid);
	if(tmp){
		Player *player = (Player*)(tmp->thing);
		Item *newitem = Item::CreateItem(itemid,type);
		if(!player->addItem(newitem)){
			//add item on the ground
			action->game->addThing(NULL,action->_player->pos,newitem);
			newitem->pos = action->_player->pos;			
			action->game->sendAddThing(NULL,action->_player->pos,newitem);			
		}
	}
	else{
		lua_pushnumber(L, -1);
		std::cout << "luaDoPlayerAddItem: player not found" << std::endl;
		return 1;
	}		
		
	lua_pushnumber(L, 0);
	return 1;
}


int ActionScript::luaActionDoPlayerSendTextMessage(lua_State *L)
{	
	//doPlayerSendTextMessage(uid,MessageClasses,message)
	const char * text = internalGetString(L);
	unsigned char messageClass = (unsigned char)internalGetNumber(L);	
	unsigned int cid = (unsigned int)internalGetNumber(L);	
	
	Action *action = getAction(L);
	
	const KnownThing* tmp = action->GetPlayerByUID(cid);
	if(tmp){
		Player *player = (Player*)(tmp->thing);
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
	
	Action *action = getAction(L);	
	
	Position realpos = internalGetRealPosition(action->_player,(Position&)pos);
	std::vector<Creature*> list;
	action->game->getSpectators(Range(realpos, true), list);
	for(unsigned int i = 0; i < list.size(); ++i){
		Player *p = dynamic_cast<Player*>(list[i]);
		if(p)
			p->sendAnimatedText(realpos, color, text);
	}	
	
	lua_pushnumber(L, 0);
	return 1;
}

int ActionScript::luaActionGetPlayerSkill(lua_State *L)
{
	//getPlayerSkill(uid,skillid)
	unsigned char skillid = (unsigned int)internalGetNumber(L);
	unsigned int cid = (unsigned int)internalGetNumber(L);
	
	Action *action = getAction(L);
	
	const KnownThing* tmp = action->GetPlayerByUID(cid);
	if(tmp){
		if(skillid > 6){
			lua_pushnumber(L, -1);
			std::cout << "GetPlayerSkill: invalid skillid" << std::endl;
			return 1;
		}
		Player *player = (Player*)(tmp->thing);
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

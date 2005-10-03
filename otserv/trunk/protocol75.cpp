//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Implementation of tibia v7.5 protocoll
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

#include <string>
#include <iostream>
#include <sstream>
#include <time.h>
#include <list>

#include "networkmessage.h"
#include "protocol75.h"

#include "items.h"

#include "tile.h"
#include "creature.h"
#include "player.h"
#include "status.h"
#include "chat.h"

#include <stdio.h>

#include "luascript.h"

#include "otsystem.h"
#include "actions.h"
#include "game.h"

extern LuaScript g_config;
extern Actions actions;
Chat g_chat;

Protocol75::Protocol75(SOCKET s)
{
	OTSYS_THREAD_LOCKVARINIT(bufferLock);
	windowTextID = 0;
	readItem = NULL;
	this->s = s;
}


Protocol75::~Protocol75()
{
	OTSYS_THREAD_LOCKVARRELEASE(bufferLock);	
}

void Protocol75::reinitializeProtocol()
{
	windowTextID = 0;
	readItem = NULL;
	OutputBuffer.Reset();
	knownPlayers.clear();
}

bool Protocol75::ConnectPlayer()
{	
	Status* stat = Status::instance();
	if(!stat->hasSlot() && player->access == 0)
		return false;
	else                    
		return game->placeCreature(player->pos, player);
}


void Protocol75::ReceiveLoop()
{
	NetworkMessage msg;
	do{
		while(pendingLogout == false && msg.ReadFromSocket(s)){
			parsePacket(msg);
		}
		
		if(s){
			closesocket(s);
			s = 0;
		}
		// logout by disconnect?  -> kick
		if(pendingLogout == false){
			game->playerSetAttackedCreature(player, 0);
			//player->setAttackedCreature(0);
			while(player->inFightTicks >= 1000 && player->isRemoved == false && s == 0){
				OTSYS_SLEEP(250);
			}
			OTSYS_THREAD_LOCK(game->gameLock, "Protocol75::ReceiveLoop()")
				if(s == 0 && player->isRemoved == false){
					game->removeCreature(player);
				}
				OTSYS_THREAD_UNLOCK(game->gameLock, "Protocol75::ReceiveLoop()")
		}
	}while(s != 0 && player->isRemoved == false);
}


void Protocol75::parsePacket(NetworkMessage &msg)
{
	if(msg.getMessageLength() <= 0)
		return;
	
	uint8_t recvbyte = msg.GetByte();
	//a dead player can not performs actions
	if (player->isRemoved == true && recvbyte != 0x14) {
		OTSYS_SLEEP(10);
		return;
	}	
    
	switch(recvbyte)
	{
    case 0x14: // logout
		parseLogout(msg);
		break;
		
	case 0x64: // client moving with steps
		parseMoveByMouse(msg);
		break;
		
    case 0x65: // move north
		parseMoveNorth(msg);
		break;
		
    case 0x66: // move east
		parseMoveEast(msg);
		break;
		
    case 0x67: // move south
		parseMoveSouth(msg);
		break;
		
    case 0x68: // move west
		parseMoveWest(msg);
		break;
		
    case 0x6A:
		parseMoveNorthEast(msg);
		break;
		
    case 0x6B:
		parseMoveSouthEast(msg);
		break;
		
    case 0x6C:
		parseMoveSouthWest(msg);
		break;
		
    case 0x6D:
		parseMoveNorthWest(msg);
		break;
		
	case 0x6F: // turn north
		parseTurnNorth(msg);
		break;
		
	case 0x70: // turn east
		parseTurnEast(msg);
		break;
		
    case 0x71: // turn south
		parseTurnSouth(msg);
		break;
		
    case 0x72: // turn west
		parseTurnWest(msg);
		break;
		
	case 0x7D: // Request trade
		parseRequestTrade(msg);
		break;
		
	case 0x7E: // Look at an item in trade
		parseLookInTrade(msg);
		break;
		
	case 0x7F: // Accept trade
		parseAcceptTrade(msg);
		break;
		
	case 0x80: // Close/cancel trade
		parseCloseTrade();
		break;
		
    case 0x78: // throw item
		parseThrow(msg);
		break;
		
    case 0x82: // use item
		parseUseItem(msg);
		break;
		
    case 0x83: // use item
		parseUseItemEx(msg);
		break;
		
    case 0x85:	//rotate item
		//parseRotateItem(msg);
		break;
		
    case 0x87: // close container
		parseCloseContainer(msg);
		break;
		
	case 0x88: //"up-arrow" - container
		parseUpArrowContainer(msg);	
		break;
		
	case 0x89:
		parseTextWindow(msg);
		break;
		
    case 0x8C: // throw item
		parseLookAt(msg);
		break;
		
    case 0x96:  // say something
		parseSay(msg);
		break;
		
    case 0xA1: // attack
		parseAttack(msg);
		break;
		
    case 0xD2: // request Outfit
		parseRequestOutfit(msg);
		break;
		
    case 0xD3: // set outfit
		parseSetOutfit(msg);
		break;
		
    case 0x97: // request Channels
		parseGetChannels(msg);
		break;
		
    case 0x98: // open Channel
		parseOpenChannel(msg);
		break;
		
    case 0x99: // close Channel
		parseCloseChannel(msg);
		break;
		
    case 0x9A: // open priv
		parseOpenPriv(msg);
		break;
		
    case 0xBE: // cancel move
		parseCancelMove(msg);
		break;
		
    case 0xA0: // set attack and follow mode
		parseModes(msg);
		break;
		
    case 0x69: // client quit without logout <- wrong
		if(game->stopEvent(player->eventAutoWalk)) {
			sendCancelWalk();
		}
		break;
		
    case 0x1E: // keep alive / ping response
		player->receivePing();
		break;
		
    case 0xC9: // change position
		// update position 
		break;
		
    default:
#ifdef __DEBUG__
		printf("unknown packet header: %x \n", recvbyte);
		parseDebug(msg);
#endif
		break;
	}
	game->flushSendBuffers();
}

void Protocol75::GetTileDescription(const Tile* tile, NetworkMessage &msg)
{
	if (tile)
	{
		int count = 0;
		if(tile->ground) {
			msg.AddItem(tile->ground);
			count++;
		}
		
		if (tile->splash)
		{
			msg.AddItem(tile->splash);
			count++;
		}
		
		ItemVector::const_iterator it;
		for (it = tile->topItems.begin(); ((it !=tile->topItems.end()) && (count < 10)); ++it)
		{		
			msg.AddItem(*it);
			count++;
		}
		
		CreatureVector::const_iterator itc;
		for (itc = tile->creatures.begin(); ((itc !=tile->creatures.end()) && (count < 10)); ++itc)
		{
			bool known;
			unsigned long removedKnown;
			checkCreatureAsKnown((*itc)->getID(), known, removedKnown);
			AddCreature(msg,*itc, known, removedKnown);
			count++;
		}
		
		
		for (it = tile->downItems.begin(); ((it !=tile->downItems.end()) && (count < 10)); ++it)
		{			
			msg.AddItem(*it);
			count++;			
		}
	}
}

void Protocol75::GetMapDescription(unsigned short x, unsigned short y, unsigned char z,
                                   unsigned short width, unsigned short height,
                                   NetworkMessage &msg)
{
	Tile* tile;
	int skip = -1;
	int startz, endz, offset, zstep, cc = 0;
	if (z > 7) {
		startz = z - 2;
		endz = std::min(MAP_LAYER - 1, z + 2);
		zstep = 1;
	}
	else {
		startz = 7;
		endz = 0;
		
		zstep = -1;
	}
	
	for(int nz = startz; nz != endz + zstep; nz += zstep) {
		offset = z - nz;
		
		for (int nx = 0; nx < width; nx++)
			for (int ny = 0; ny < height; ny++) {
				tile = game->getTile(x + nx + offset, y + ny + offset, nz);
				if (tile) {
					if (skip >= 0) {
						msg.AddByte(skip);
						msg.AddByte(0xFF);
						cc +=skip;
					}   
					skip = 0;
					
					GetTileDescription(tile, msg);
					cc++;
					
				}
				else {
				/*
				if(skip == -1) 
				skip = 0;
					*/
					
					skip++;
					if (skip == 0xFF) {
						msg.AddByte(0xFF);
						msg.AddByte(0xFF);
						cc += skip;
						skip = -1;
					}
				}
			}
	}
	
	if (skip >= 0) {
		msg.AddByte(skip);
		msg.AddByte(0xFF);
		cc += skip;
	}
	
#ifdef __DEBUG__
	printf("tiles in total: %d \n", cc);
#endif
}


void Protocol75::checkCreatureAsKnown(unsigned long id, bool &known, unsigned long &removedKnown)
{
	// loop through the known player and check if the given player is in
	std::list<unsigned long>::iterator i;
	for(i = knownPlayers.begin(); i != knownPlayers.end(); ++i)
	{
		if((*i) == id)
		{
			// know... make the creature even more known...
			knownPlayers.erase(i);
			knownPlayers.push_back(id);
			
			known = true;
			return;
		}
	}
	
	// ok, he is unknown...
	known = false;
	
	// ... but not in future
	knownPlayers.push_back(id);
	
	// to many known players?
	if(knownPlayers.size() > 150) //150 for 7.4 clients, changed for 7.5?
	{
		// lets try to remove one from the end of the list
		for (int n = 0; n < 150; n++)
		{
			removedKnown = knownPlayers.front();
			
			Creature *c = game->getCreatureByID(removedKnown);
			if ((!c) || (!CanSee(c)))
				break;
			
			// this creature we can't remove, still in sight, so back to the end
			knownPlayers.pop_front();
			knownPlayers.push_back(removedKnown);
		}
		
		// hopefully we found someone to remove :S, we got only 150 tries
		// if not... lets kick some players with debug errors :)
		knownPlayers.pop_front();
	}
	else
	{
		// we can cache without problems :)
		removedKnown = 0;
	}
}


// Parse methods
void Protocol75::parseLogout(NetworkMessage &msg)
{
	if(player->inFightTicks >=1000 && player->isRemoved == false){
		sendCancel("You may not logout during or immediately after a fight!");
		return;
	}
	else{
		logout();
	}
}

void Protocol75::logout(){
	// we ask the game to remove us
	if(player->isRemoved == false){
		if(game->removeCreature(player))
			pendingLogout = true;
	}
	else{
		pendingLogout = true;
	}
}

void Protocol75::parseGetChannels(NetworkMessage &msg){
	sendChannelsDialog();
}

void Protocol75::parseOpenChannel(NetworkMessage &msg){
	unsigned short channelId = msg.GetU16();
	OTSYS_THREAD_LOCK_CLASS lockClass(game->gameLock, "Protocol75::parseOpenChannel()");
	if(g_chat.addUserToChannel(player, channelId))
		sendChannel(channelId, g_chat.getChannelName(player, channelId));
}

void Protocol75::parseCloseChannel(NetworkMessage &msg){
	unsigned short channelId = msg.GetU16();
	OTSYS_THREAD_LOCK_CLASS lockClass(game->gameLock, "Protocol75::parseCloseChannel()");
	g_chat.removeUserFromChannel(player, channelId);
}

void Protocol75::parseOpenPriv(NetworkMessage &msg){
	std::string receiver; 
	receiver = msg.GetString();
	OTSYS_THREAD_LOCK_CLASS lockClass(game->gameLock, "Protocol75::parseOpenPriv()");
	Player* player = game->getPlayerByName(receiver);
	if(player)
		sendOpenPriv(receiver);
}

void Protocol75::sendOpenPriv(std::string &receiver){
	NetworkMessage newmsg; 
	newmsg.AddByte(0xAD); 
	newmsg.AddString(receiver);      
	WriteBuffer(newmsg);
}     

void Protocol75::parseCancelMove(NetworkMessage &msg)
{
	game->playerSetAttackedCreature(player, 0);
	
	game->stopEvent(player->eventAutoWalk);
	player->sendCancelWalk();
}

void Protocol75::parseModes(NetworkMessage &msg)
{
	//player->fightMode = msg.GetByte();
	//player->followMode = msg.GetByte();
}

void Protocol75::parseDebug(NetworkMessage &msg)
{
	int dataLength = msg.getMessageLength()-3;
	if(dataLength!=0){
		printf("data: ");
		size_t data = msg.GetByte();
		while( dataLength > 0){
			printf("%d ", data);
			if(--dataLength >0)
				data = msg.GetByte();
		}
		printf("\n");
	}
}

void Protocol75::parseMoveByMouse(NetworkMessage &msg)
{
	// first we get all directions...
	std::list<Direction> path;
	size_t numdirs = msg.GetByte();
	for (size_t i = 0; i < numdirs; ++i) {
		unsigned char rawdir = msg.GetByte();
		Direction dir = SOUTH;
		
		switch(rawdir) {
		case 1: dir = EAST; break;
		case 2: dir = NORTHEAST; break;
		case 3: dir = NORTH; break;
		case 4: dir = NORTHWEST; break;
		case 5: dir = WEST; break;
		case 6: dir = SOUTHWEST; break;
		case 7: dir = SOUTH; break;
		case 8: dir = SOUTHEAST; break;
			
		default:
			continue;
		};
		
		/*
		#ifdef __DEBUG__
		std::cout << "Walk by mouse: Direction: " << dir << std::endl;
		#endif
		*/
		path.push_back(dir);
	}
	
	game->playerAutoWalk(player, path);
}

void Protocol75::parseMoveNorth(NetworkMessage &msg)
{
	if(game->stopEvent(player->eventAutoWalk)) {
		player->sendCancelWalk();
	}
	
	this->sleepTillMove();
	
	game->thingMove(player, player,
		player->pos.x, player->pos.y-1, player->pos.z, 1);
}

void Protocol75::parseMoveEast(NetworkMessage &msg)
{
	if(game->stopEvent(player->eventAutoWalk)) {
		player->sendCancelWalk();
	}
	
	this->sleepTillMove();
	
	game->thingMove(player, player,
		player->pos.x+1, player->pos.y, player->pos.z, 1);
}


void Protocol75::parseMoveSouth(NetworkMessage &msg)
{
	if(game->stopEvent(player->eventAutoWalk)) {
		player->sendCancelWalk();
	}
	
	this->sleepTillMove();
	
	game->thingMove(player, player,
		player->pos.x, player->pos.y+1, player->pos.z, 1);
}


void Protocol75::parseMoveWest(NetworkMessage &msg)
{
	if(game->stopEvent(player->eventAutoWalk)) {
		player->sendCancelWalk();
	}
	
	this->sleepTillMove();
	
	game->thingMove(player, player,
		player->pos.x-1, player->pos.y, player->pos.z, 1);
}

void Protocol75::parseMoveNorthEast(NetworkMessage &msg)
{
	if(game->stopEvent(player->eventAutoWalk)) {
		player->sendCancelWalk();
	}
	
	this->sleepTillMove();
	this->sleepTillMove();
	
	game->thingMove(player, player,
		(player->pos.x+1), (player->pos.y-1), player->pos.z, 1);
}

void Protocol75::parseMoveSouthEast(NetworkMessage &msg)
{
	if(game->stopEvent(player->eventAutoWalk)) {
		player->sendCancelWalk();
	}
	
	this->sleepTillMove();
	this->sleepTillMove();
	
	game->thingMove(player, player,
		(player->pos.x+1), (player->pos.y+1), player->pos.z, 1);
}

void Protocol75::parseMoveSouthWest(NetworkMessage &msg)
{
	if(game->stopEvent(player->eventAutoWalk)) {
		player->sendCancelWalk();
	}
	
	this->sleepTillMove();
	this->sleepTillMove();
	
	game->thingMove(player, player,
		(player->pos.x-1), (player->pos.y+1), player->pos.z, 1);
}

void Protocol75::parseMoveNorthWest(NetworkMessage &msg)
{
	if(game->stopEvent(player->eventAutoWalk)) {
		player->sendCancelWalk();
	}
	
	this->sleepTillMove();
	this->sleepTillMove();
	
	game->thingMove(player, player,
		(player->pos.x-1), (player->pos.y-1), player->pos.z, 1);   
}


void Protocol75::parseTurnNorth(NetworkMessage &msg)
{
	game->creatureTurn(player, NORTH);
}


void Protocol75::parseTurnEast(NetworkMessage &msg)
{
	game->creatureTurn(player, EAST);
}


void Protocol75::parseTurnSouth(NetworkMessage &msg)
{
	game->creatureTurn(player, SOUTH);
}


void Protocol75::parseTurnWest(NetworkMessage &msg)
{
	game->creatureTurn(player, WEST);
	
}

void Protocol75::parseRequestOutfit(NetworkMessage &msg)
{
	msg.Reset();
	
	msg.AddByte(0xC8);
	msg.AddByte(player->looktype);
	msg.AddByte(player->lookhead);
	msg.AddByte(player->lookbody);
	msg.AddByte(player->looklegs);
	msg.AddByte(player->lookfeet);
	switch (player->getSex()) {
	case PLAYERSEX_FEMALE:
		msg.AddByte(PLAYER_FEMALE_1);
		msg.AddByte(PLAYER_FEMALE_7);
		break;
	case PLAYERSEX_MALE:
		msg.AddByte(PLAYER_MALE_1);
		msg.AddByte(PLAYER_MALE_7);
		break;
	case PLAYERSEX_OLDMALE:
		msg.AddByte(160);
		msg.AddByte(160);
		break;
	default:
		msg.AddByte(PLAYER_MALE_1);
		msg.AddByte(PLAYER_MALE_7);
	}
	
	WriteBuffer(msg);
}

void Protocol75::sendSetOutfit(const Creature* creature) {
	if (CanSee(creature)) {
		NetworkMessage newmsg;
		newmsg.AddByte(0x8E);
		newmsg.AddU32(creature->getID());
		newmsg.AddByte(creature->looktype);
		newmsg.AddByte(creature->lookhead);
		newmsg.AddByte(creature->lookbody);
		newmsg.AddByte(creature->looklegs);
		newmsg.AddByte(creature->lookfeet);
		WriteBuffer(newmsg);
	}
}

void Protocol75::sendInventory(unsigned char sl_id){	
	NetworkMessage msg;
	AddPlayerInventoryItem(msg,player,sl_id);
	WriteBuffer(msg);
}

void Protocol75::sendStats(){
	NetworkMessage msg;
	AddPlayerStats(msg,player);
	WriteBuffer(msg);
}

void Protocol75::sendTextMessage(MessageClasses mclass, const char* message){
	NetworkMessage msg;
	AddTextMessage(msg,mclass, message);
	WriteBuffer(msg);
}

void Protocol75::sendTextMessage(MessageClasses mclass, const char* message,const Position &pos, unsigned char type){
	NetworkMessage msg;
	AddMagicEffect(msg,pos,type);
	AddTextMessage(msg,mclass, message);
	WriteBuffer(msg);
}

void Protocol75::sendChannelsDialog(){
	NetworkMessage newmsg;
	ChannelList list;
	
	list = g_chat.getChannelList(player);
	
	newmsg.AddByte(0xAB);
	
	newmsg.AddByte(list.size()); //how many
	
	while(list.size())
	{
		ChatChannel *channel;
		channel = list.front();
		list.pop_front();
		
		newmsg.AddU16(channel->getId());
		newmsg.AddString(channel->getName());
	}
	WriteBuffer(newmsg);
}

void Protocol75::sendChannel(unsigned short channelId, std::string channelName){
	NetworkMessage newmsg;
	
	newmsg.AddByte(0xAC);
	newmsg.AddU16(channelId);
	newmsg.AddString(channelName);
	
	WriteBuffer(newmsg); 
}

void Protocol75::sendIcons(int icons){
	NetworkMessage newmsg;
	newmsg.AddByte(0xA2);
	newmsg.AddByte(icons);
	WriteBuffer(newmsg);
}

void Protocol75::parseSetOutfit(NetworkMessage &msg)
{
	int temp = msg.GetByte();
	if ( (player->getSex() == PLAYERSEX_FEMALE && temp >= PLAYER_FEMALE_1 && temp <= PLAYER_FEMALE_7) ||
		(player->getSex() == PLAYERSEX_MALE && temp >= PLAYER_MALE_1 && temp <= PLAYER_MALE_7))
	{
		player->looktype= temp;
		player->lookmaster = player->looktype;
		player->lookhead=msg.GetByte();
		player->lookbody=msg.GetByte();
		player->looklegs=msg.GetByte();
		player->lookfeet=msg.GetByte();
		
		game->creatureChangeOutfit(player);
	}
}


void Protocol75::parseUseItemEx(NetworkMessage &msg)
{
	Position pos_from = msg.GetPosition();
	unsigned short itemid = msg.GetItemId();
	unsigned char from_stackpos = msg.GetByte();
	Position pos_to = msg.GetPosition();
	/*unsigned short tile_id = */msg.GetU16();
	unsigned char to_stackpos = msg.GetByte();
	
	game->playerUseItemEx(player,pos_from,from_stackpos, pos_to, to_stackpos, itemid);
	
}

void Protocol75::sendContainer(unsigned char index, Container *container)
{
	if(!container)
		return;
	
	NetworkMessage msg;
	
	player->addContainer(index, container);
	
	msg.AddByte(0x6E);
	msg.AddByte(index);
	
	//msg.AddU16(container->getID());
	msg.AddItemId(container);
	msg.AddString(container->getName());
	msg.AddByte(container->capacity());
	if(container->getParent() != NULL)
		msg.AddByte(0x01); // container up ID (can go up) 
	else
		msg.AddByte(0x00);
	
	msg.AddByte(container->size());
	
	ContainerList::const_iterator cit;
	for (cit = container->getItems(); cit != container->getEnd(); ++cit) {
		msg.AddItem(*cit);
	}
	WriteBuffer(msg);
}

void Protocol75::sendTradeItemRequest(const Player* player, const Item* item, bool ack)
{
	NetworkMessage msg;
	if(ack) {
		msg.AddByte(0x7D);
	}
	else {
		msg.AddByte(0x7E);
	}
	
	msg.AddString(player->getName());
	
	const Container *tradeContainer = dynamic_cast<const Container*>(item);
	if(tradeContainer) {
		
		std::list<const Container*> stack;
		stack.push_back(tradeContainer);
		
		std::list<const Item*> itemstack;
		itemstack.push_back(tradeContainer);
		
		ContainerList::const_iterator it;
		
		while(stack.size() > 0) {
			const Container *container = stack.front();
			stack.pop_front();
			
			for (it = container->getItems(); it != container->getEnd(); ++it) {
				Container *container = dynamic_cast<Container*>(*it);
				if(container) {
					stack.push_back(container);
				}
				
				itemstack.push_back(*it);
			}
		}
		
		msg.AddByte(itemstack.size());
		while(itemstack.size() > 0) {
			const Item* item = itemstack.front();
			itemstack.pop_front();
			msg.AddItem(item);
		}
	}
	else {
		msg.AddByte(1);
		msg.AddItem(item);
	}
	
	WriteBuffer(msg);
}

void Protocol75::sendCloseTrade()
{
	NetworkMessage msg;
	msg.AddByte(0x7F);
	
	WriteBuffer(msg);
}

void Protocol75::sendCloseContainer(unsigned char containerid)
{
	NetworkMessage msg;
	
	msg.AddByte(0x6F);
	msg.AddByte(containerid);
	WriteBuffer(msg);
}

void Protocol75::parseUseItem(NetworkMessage &msg)
{
	Position pos = msg.GetPosition();
	unsigned short itemid = msg.GetItemId();
	unsigned char stack = msg.GetByte();
	unsigned char index = msg.GetByte();
	
#ifdef __DEBUG__
	std::cout << "parseUseItem: " << "x: " << pos.x << ", y: " << (int)pos.y <<  ", z: " << (int)pos.z << ", item: " << (int)item << ", stack: " << (int)stack << ", index: " << (int)index << std::endl;
#endif
	
	game->playerUseItem(player, pos, stack, itemid, index);
}

void Protocol75::parseCloseContainer(NetworkMessage &msg)
{
	unsigned char containerid = msg.GetByte();
	player->closeContainer(containerid);
	sendCloseContainer(containerid);
}

void Protocol75::parseUpArrowContainer(NetworkMessage &msg)
{
	unsigned char containerid = msg.GetByte();
	OTSYS_THREAD_LOCK_CLASS lockClass(game->gameLock, "Protocol75::parseUpArrowContainer()");
	Container *container = player->getContainer(containerid);
	if(!container)
		return;
	
	Container *parentcontainer = container->getParent();
	if(parentcontainer) {
		sendContainer(containerid, parentcontainer);
	}
}

void Protocol75::parseThrow(NetworkMessage &msg)
{
	unsigned short from_x     = msg.GetU16();
	unsigned short from_y     = msg.GetU16(); 
	unsigned char  from_z     = msg.GetByte();
	unsigned short itemid     = msg.GetItemId();
	unsigned char  from_stack = msg.GetByte();
	unsigned short to_x       = msg.GetU16();
	unsigned short to_y       = msg.GetU16(); 
	unsigned char  to_z       = msg.GetByte();
	unsigned char count       = msg.GetByte();
	/*
	std::cout << "parseThrow: " << "from_x: " << (int)from_x << ", from_y: " << (int)from_y
	<<  ", from_z: " << (int)from_z << ", item: " << (int)itemid << ", from_stack: " 
	<< (int)from_stack << " to_x: " << (int)to_x << ", to_y: " << (int)to_y
	<<  ", to_z: " << (int)to_z  
	<< ", count: " << (int)count << std::endl;*/
	bool toInventory = false;
	bool fromInventory = false;
	
	if(from_x == to_x && from_y == to_y && from_z == to_z)
		return;
	
	//container/inventory to container/inventory
	if(from_x == 0xFFFF && to_x == 0xFFFF) {
		unsigned char from_cid;
		unsigned char to_cid;
		
		if(from_y & 0x40) {
			from_cid = from_y & 0x0F;
		}
		else {
			fromInventory = true;
			from_cid = static_cast<unsigned char>(from_y);
		}
		
		if(to_y & 0x40) {
			to_cid = static_cast<unsigned char>(to_y & 0x0F);
		}
		else {
			toInventory = true;
			to_cid = static_cast<unsigned char>(to_y);
		}
		
		game->thingMove(player, from_cid, from_z, itemid,fromInventory, to_cid, to_z, toInventory, count);
	}
	//container/inventory to ground
	else if(from_x == 0xFFFF && to_x != 0xFFFF) {
		unsigned char from_cid;
		
		if(0x40 & from_y) {
			from_cid = static_cast<unsigned char>(from_y & 0x0F);
		}
		else {
			fromInventory = true;
			from_cid = static_cast<unsigned char>(from_y);
		}
		
		game->thingMove(player, from_cid, from_z, itemid, fromInventory, Position(to_x, to_y, to_z), count);
	}
	//ground to container/inventory
	else if(from_x != 0xFFFF && to_x == 0xFFFF) {
		unsigned char to_cid;
		
		if(0x40 & to_y) {
			to_cid = static_cast<unsigned char>(to_y & 0x0F);
		}
		else {
			toInventory = true;
			to_cid = static_cast<unsigned char>(to_y);
			
			if(to_cid > 11) {
				return;
			}
			
			if(to_cid == 0) {
				//Special condition, player drops an item to the "capacity window" when the inventory is minimized,
				//we should add this item to the most appropriate slot/container
				return;
			}
		}
		
		game->thingMove(player, Position(from_x, from_y, from_z), from_stack, itemid, to_cid, to_z, toInventory, count);
	}
	//ground to ground
	else {
		Tile *fromTile = game->getTile(from_x, from_y, from_z);
		if(!fromTile)
			return;
		Creature *movingCreature = dynamic_cast<Creature*>(fromTile->getThingByStackPos(from_stack));
		
		if(movingCreature) {
			Player *movingPlayer = dynamic_cast<Player*>(movingCreature);
			if(player == movingPlayer) {
				this->sleepTillMove();
			}
		}
		
		game->thingMove(player, from_x, from_y, from_z, from_stack, itemid, to_x, to_y, to_z, count);
	}
}


void Protocol75::parseLookAt(NetworkMessage &msg){
	Position LookPos = msg.GetPosition();
	unsigned short ItemNum = msg.GetU16();
	unsigned char stackpos = msg.GetByte();
	
#ifdef __DEBUG__
	std::cout << "look at: " << LookPos << std::endl;
	std::cout << "itemnum: " << ItemNum << " stackpos: " << (long)stackpos<< std::endl;
#endif
	
	NetworkMessage newmsg;
	std::stringstream ss;
	
	/*
	#ifdef __DEBUG__
	ss << "You look at " << LookPos << " and see Item # " << ItemNum << ".";
	AddTextMessage(newmsg,MSG_INFO, ss.str().c_str());
	#else
	*/
	Item *item = NULL;
	Creature *creature = NULL;
	OTSYS_THREAD_LOCK_CLASS lockClass(game->gameLock, "Protocol75::parseLookAt()");
	
	if(LookPos.x != 0xFFFF) {
		Tile* tile = game->getTile(LookPos.x, LookPos.y, LookPos.z);
		if(tile){
			item = dynamic_cast<Item*>(tile->getTopThing());
			creature = dynamic_cast<Creature*>(tile->getTopThing());
		}
	}
	else {
		//from container/inventory
		if(LookPos.y & 0x40) {
			unsigned char from_cid = LookPos.y & 0x0F;
			unsigned char slot = LookPos.z;
			
			Container *parentcontainer = player->getContainer(from_cid);
			if(!parentcontainer)
				return;
			
			item = parentcontainer->getItem(slot);
		}
		else {
			unsigned char from_cid = static_cast<unsigned char>(LookPos.y);
			item = player->getItem(from_cid);
		}
	}
	
	if(item) {
		bool fullDescription = false;
		if(LookPos.x == 0xFFFF) {
			fullDescription = true;
		}
		else if(std::abs(player->pos.x - LookPos.x) <= 1 && std::abs(player->pos.y - LookPos.y) <= 1 &&
			LookPos.z == player->pos.z) {
			fullDescription = true;
		}
		
		std::stringstream ss;
		ss << "You see " << item->getDescription(fullDescription);
		AddTextMessage(newmsg,MSG_INFO, ss.str().c_str());
	}
	else if(creature) {
		if(player == creature) {
			std::stringstream ss;
			ss << "You see " << creature->getDescription(true);
			AddTextMessage(newmsg,MSG_INFO, ss.str().c_str());
		}
		else {
			std::stringstream ss;
			ss << "You see " << creature->getDescription().c_str();
			AddTextMessage(newmsg,MSG_INFO, ss.str().c_str());
		}
	}
	//#endif
	
	sendNetworkMessage(&newmsg);
}



void Protocol75::parseSay(NetworkMessage &msg)
{
	SpeakClasses type = (SpeakClasses)msg.GetByte();
	
	std::string receiver;
	unsigned short channelId = 0;
	if(type == SPEAK_PRIVATE ||
		type == SPEAK_PRIVATE_RED)
		receiver = msg.GetString();
	if(type == SPEAK_CHANNEL_Y ||
		type == SPEAK_CHANNEL_R1 ||
		type == SPEAK_CHANNEL_R2)
		channelId = msg.GetU16();
	std::string text = msg.GetString();
	
	if(game->creatureSaySpell(player, text))
		type = SPEAK_SAY;
	
	switch (type)
	{
	case SPEAK_SAY:
		game->creatureSay(player, type, text);
		break;
	case SPEAK_WHISPER:
		game->creatureWhisper(player, text);
		break;
	case SPEAK_YELL:
		game->creatureYell(player, text);
		break;
	case SPEAK_PRIVATE:
	case SPEAK_PRIVATE_RED:
		game->creatureSpeakTo(player, type, receiver, text);
		break;
	case SPEAK_CHANNEL_Y:
	case SPEAK_CHANNEL_R1:
	case SPEAK_CHANNEL_R2:
		g_chat.talkToChannel(player, type, text, channelId);
		break;
	case SPEAK_BROADCAST:
		game->creatureBroadcastMessage(player, text);
		break;
	}
}

void Protocol75::parseAttack(NetworkMessage &msg)
{
	unsigned long creatureid = msg.GetU32();
	game->playerSetAttackedCreature(player, creatureid);
}

void Protocol75::parseTextWindow(NetworkMessage &msg)
{
	unsigned long id = msg.GetU32();
	std::string new_text = msg.GetString();
	if(readItem && windowTextID == id){	
		sendTextMessage(MSG_SMALLINFO, "Write not working yet.");
		//move to Game, and use gameLock
		/*//TODO: check that the item is in
		//an accesible place for the player
		unsigned short itemid = readItem->getID();
		readItem->setText(new_text);
		if(readItem->getID() != id){
		//TODO:update the item in the clients. 
		//Can be done when find a method to get 
		// items position its pointer.
	}*/
		readItem->releaseThing();
		readItem = NULL;
	}
}

void Protocol75::parseRequestTrade(NetworkMessage &msg)
{
	Position pos = msg.GetPosition();
	unsigned short itemid = msg.GetItemId();
	unsigned char stack = msg.GetByte();
	unsigned long playerid = msg.GetU32();
	
	game->playerRequestTrade(player, pos, stack, itemid, playerid);
}

void Protocol75::parseAcceptTrade(NetworkMessage &msg)
{
	game->playerAcceptTrade(player);
}

void Protocol75::parseLookInTrade(NetworkMessage &msg)
{
	bool counterOffer = (msg.GetByte() == 0x01);
	int index = msg.GetByte();
	
	game->playerLookInTrade(player, counterOffer, index);
}

void Protocol75::parseCloseTrade()
{
	game->playerCloseTrade(player);
}

bool Protocol75::CanSee(int x, int y, int z) const
{
#ifdef __DEBUG__
	if(z < 0 || z >= MAP_LAYER) {
		std::cout << "WARNING! Protocol75::CanSee() Z-value is out of range!" << std::endl;
	}
#endif
	
	/*underground 8->15*/
	if(player->pos.z > 7 && z < 6 /*8 - 2*/) {
		return false;
	}
	/*ground level and above 7->0*/
	else if(player->pos.z <= 7 && z > 7){
		return false;
	}
	
	//negative offset means that the action taken place is on a lower floor than ourself
	int offsetz = player->pos.z - z;
	
	if ((x >= player->pos.x - 8 + offsetz) && (x <= player->pos.x + 9 + offsetz) &&
		(y >= player->pos.y - 6 + offsetz) && (y <= player->pos.y + 7 + offsetz))
		return true;
	
	return false;
}

bool Protocol75::CanSee(const Creature *c) const
{
	if(c->isRemoved == true)
		return false;
	
	return CanSee(c->pos.x, c->pos.y, c->pos.z);
}

void Protocol75::sendNetworkMessage(NetworkMessage *msg)
{
	WriteBuffer(*msg);
}

void Protocol75::AddTileUpdated(NetworkMessage &msg, const Position &pos)
{
#ifdef __DEBUG__
	std::cout << "Pop-up item from below..." << std::endl;
#endif

	if (CanSee(pos.x, pos.y, pos.z))
	{		
		msg.AddByte(0x69);
		msg.AddPosition(pos);
		
		Tile* tile = game->getTile(pos.x, pos.y, pos.z);
		if(tile) {
			GetTileDescription(tile, msg);
			msg.AddByte(0);
			msg.AddByte(0xFF);
		}
		else {
			msg.AddByte(0x01);
			msg.AddByte(0xFF);
		}
	}
}

void Protocol75::sendTileUpdated(const Position &pos)
{
	NetworkMessage msg;
	
	AddTileUpdated(msg, pos);
	WriteBuffer(msg);
}

//container to container
void Protocol75::sendThingMove(const Creature *creature, const Container *fromContainer, unsigned char from_slotid,
							   const Item* fromItem, int oldFromCount, Container *toContainer, unsigned char to_slotid, const Item *toItem, int oldToCount, int count)
{
	if(player->NeedUpdateStats()) {
		player->sendStats();
	}
	
	NetworkMessage msg;
	
	//Auto-close container's
	const Container* moveContainer = dynamic_cast<const Container*>(fromItem);
	if(moveContainer){
		bool hasContainerOpen = false;
		for(containerLayout::const_iterator cit = player->getContainers(); cit != player->getEndContainer(); ++cit) {
			if(cit->second == toContainer || cit->second->getTopParent()->isHoldingItem(toContainer)){
				hasContainerOpen = true;
				break;
			}
		}
		
		if(!hasContainerOpen && !player->isHoldingContainer(toContainer)) {
			autoCloseContainers(moveContainer, msg);
		}
	}
	
	Item *container = NULL;
	for(containerLayout::const_iterator cit = player->getContainers(); cit != player->getEndContainer(); ++cit) {
		container  = cit->second;
		unsigned char cid = cit->first;
		
		if(container && container == fromContainer) {
			if(toContainer == fromContainer) {
				if(fromItem->isStackable()) {
					if(toItem && fromItem->getID() == toItem->getID() && toItem->getItemCountOrSubtype() != oldToCount) {
						//update count						
						TransformItemContainer(msg,cid,to_slotid,toItem);
						
						if(fromItem->getItemCountOrSubtype() > 0 && count != oldFromCount) {
							//update count							
							TransformItemContainer(msg,cid,from_slotid,fromItem);
						}
						else {
							//remove item							
							RemoveItemContainer(msg,cid,from_slotid);
						}
						
						//surplus items
						if(oldToCount + count > 100) {
							//add item														
							AddItemContainer(msg,cid,fromItem,oldToCount + count - 100);
						}
					}
					else {
						if(count == oldFromCount) {
							//remove item							
							RemoveItemContainer(msg,cid,from_slotid);
						}
						else {
							//update count							
							TransformItemContainer(msg,cid,from_slotid,fromItem);
						}
						
						//add item						
						AddItemContainer(msg,cid,fromItem,count);
					}
				}
				else {
					//remove item					
					RemoveItemContainer(msg,cid,from_slotid);
					
					//add item					
					AddItemContainer(msg,cid,fromItem);
				}
			}
			else {
				if(fromItem->isStackable() && fromItem->getItemCountOrSubtype() > 0 && count != oldFromCount) {
					//update count					
					TransformItemContainer(msg,cid,from_slotid,fromItem);
				}
				else {
					//remove item					
					RemoveItemContainer(msg,cid,from_slotid);
				}
			}
		}
		
		if(container && container == toContainer && toContainer != fromContainer) {
			if(fromItem->isStackable()) {
				if(toItem && fromItem->getID() == toItem->getID() && toItem->getItemCountOrSubtype() != oldToCount) {
					//update count					
					TransformItemContainer(msg,cid,to_slotid,toItem);
					
					//surplus items
					if(oldToCount + count > 100) {
						//add item						
						AddItemContainer(msg,cid,fromItem,oldToCount + count - 100);
					}
				}
				else {
					//add item					
					AddItemContainer(msg,cid,fromItem,count);
				}
			}
			else {
				//add item				
				AddItemContainer(msg,cid,fromItem);
			}
		}
	}
	
	WriteBuffer(msg);
}

//inventory to container
void Protocol75::sendThingMove(const Creature *creature, slots_t fromSlot, const Item* fromItem,
							   int oldFromCount, const Container *toContainer, unsigned char to_slotid, const Item *toItem, int oldToCount, int count)
{
	if(player->NeedUpdateStats()) {
		player->sendStats();
	}
	
	NetworkMessage msg;
	
	for(containerLayout::const_iterator cit = player->getContainers(); cit != player->getEndContainer(); ++cit) {
		if(cit->second == toContainer) {
			unsigned char cid = cit->first;
			
			if(fromItem->isStackable()) {
				if(toItem && fromItem->getID() == toItem->getID() && toItem->getItemCountOrSubtype() != oldToCount) {
					//update count					
					TransformItemContainer(msg,cid,to_slotid,toItem);
					
					//surplus items
					if(oldToCount + count > 100) {
						//add item						
						AddItemContainer(msg,cid,fromItem,oldToCount + count - 100);
					}
				}
				else {
					//add item					
					AddItemContainer(msg,cid,fromItem,count);
				}
			}
			else {
				//add item				
				AddItemContainer(msg,cid,fromItem);
			}
		}
	}
	
	if(creature == player) {
		AddPlayerInventoryItem(msg,player, fromSlot);

		//Update up-arrow
		const Container* moveContainer = dynamic_cast<const Container*>(fromItem);
		if(moveContainer) {
			for(containerLayout::const_iterator cit = player->getContainers(); cit != player->getEndContainer(); ++cit) {
				if(cit->second == moveContainer) {
					sendContainer(cit->first, cit->second);
				}
			}
		}
		//
	}
	
	WriteBuffer(msg);
}

//inventory to inventory
void Protocol75::sendThingMove(const Creature *creature, slots_t fromSlot, const Item* fromItem,
							   int oldFromCount, slots_t toSlot, const Item* toItem, int oldToCount, int count)
{
	NetworkMessage msg;
	if(creature == player) {
		if(player->NeedUpdateStats()) {
			player->sendStats();
		}
		
		AddPlayerInventoryItem(msg, player, fromSlot);
		AddPlayerInventoryItem(msg, player, toSlot);
	}
	
	WriteBuffer(msg);
}

//container to inventory
void Protocol75::sendThingMove(const Creature *creature, const Container *fromContainer,
							   unsigned char from_slotid, const Item* fromItem, int oldFromCount, slots_t toSlot, const Item *toItem, int oldToCount, int count)
{
	if(player->NeedUpdateStats()) {
		player->sendStats();
	}
	
	NetworkMessage msg;
	
	for(containerLayout::const_iterator cit = player->getContainers(); cit != player->getEndContainer(); ++cit) {
		if(cit->second == fromContainer) {
			unsigned char cid = cit->first;
			
			if(!fromItem->isStackable() || (oldFromCount == count && oldToCount + count <= 100)) {
				//remove item				
				RemoveItemContainer(msg,cid,from_slotid);
			}
			else {
				//update count				
				TransformItemContainer(msg,cid,from_slotid,fromItem);
			}
			
			if(toItem && toItem->getID() != fromItem->getID()) {
				//add item				
				AddItemContainer(msg,cid,toItem);
			}
		}
	}
	
	if(creature == player) {
		AddPlayerInventoryItem(msg,player, toSlot);

		//Update up-arrow
		const Container* moveContainer = dynamic_cast<const Container*>(fromItem);
		if(moveContainer) {
			for(containerLayout::const_iterator cit = player->getContainers(); cit != player->getEndContainer(); ++cit) {
				if(cit->second == moveContainer) {
					sendContainer(cit->first, cit->second);
				}
			}
		}
		//
	}
		
	WriteBuffer(msg);
}

//container to ground
void Protocol75::sendThingMove(const Creature *creature, const Container *fromContainer, unsigned char from_slotid,
							   const Item* fromItem, int oldFromCount, const Position &toPos, const Item *toItem, int oldToCount, int count)
{
	if(player->NeedUpdateStats()) {
		player->sendStats();
	}
	
	NetworkMessage msg;

	//Auto-close container's
	const Container* moveContainer = dynamic_cast<const Container*>(fromItem);
	bool updateContainerArrow = false;

	if(moveContainer) {
		//Auto-close container's
		if(std::abs(player->pos.x - toPos.x) > 1 || std::abs(player->pos.y - toPos.y) > 1) {
			autoCloseContainers(moveContainer, msg);			
		}
		else
			updateContainerArrow = true;
	}

	if(CanSee(toPos.x, toPos.y, toPos.z)) {
		if(toItem && toItem->getID() == fromItem->getID() && fromItem->isStackable() && toItem->getItemCountOrSubtype() != oldToCount) {
			AddTileUpdated(msg, toItem->pos);
		}
		else {
			AddAppearThing(msg, toPos);
			if(fromItem->isStackable()) {
				msg.AddItem(fromItem->getID(), count);
			}
			else
				msg.AddItem(fromItem);
		}
	}
	
	for(containerLayout::const_iterator cit = player->getContainers(); cit != player->getEndContainer(); ++cit) {
		unsigned char cid = cit->first;
		if(cit->second == fromContainer) {
			
			if(!fromItem->isStackable() || fromItem->getItemCountOrSubtype() == 0 || count == oldFromCount) {
				//remove item				
				RemoveItemContainer(msg,cid,from_slotid);
			}
			else {
				//update count				
				TransformItemContainer(msg,cid,from_slotid,fromItem);
			}
		}
	}
	
	//Update up-arrow
	if(updateContainerArrow) {
		const Container* moveContainer = dynamic_cast<const Container*>(fromItem);
		if(moveContainer) {
			for(containerLayout::const_iterator cit = player->getContainers(); cit != player->getEndContainer(); ++cit) {
				if(cit->second == moveContainer) {
					sendContainer(cit->first, cit->second);
				}
			}
		}
	}
	//
	
	WriteBuffer(msg);
}

//inventory to ground
void Protocol75::sendThingMove(const Creature *creature, slots_t fromSlot,
							   const Item* fromItem, int oldFromCount, const Position &toPos, const Item *toItem, int oldToCount, int count)
{
	NetworkMessage msg;
	
	if(creature == player) {
		if(player->NeedUpdateStats()) {
			player->sendStats();
		}
		
		//Auto-closing containers
		const Container* moveContainer = dynamic_cast<const Container*>(fromItem);
		if(moveContainer) {
			//Auto-close container's
			if(std::abs(player->pos.x - toPos.x) > 1 || std::abs(player->pos.y - toPos.y) > 1) {
				autoCloseContainers(moveContainer, msg);			
			}
		}
	}
	
	if(CanSee(toPos.x, toPos.y, toPos.z)) {
		if(toItem && toItem->getID() == fromItem->getID() && fromItem->isStackable() && toItem->getItemCountOrSubtype() != oldToCount) {
			AddTileUpdated(msg, toItem->pos);
		}
		else {
			AddAppearThing(msg, toPos);
			if(fromItem->isStackable()) {
				msg.AddItem(fromItem->getID(), count);
			}
			else
				msg.AddItem(fromItem);
		}
	}
	
	if(creature == player) {
		AddPlayerInventoryItem(msg, player, fromSlot);
	}
	
	WriteBuffer(msg);
}

//ground to container
void Protocol75::sendThingMove(const Creature *creature, const Position &fromPos, int stackpos, const Item* fromItem,
							   int oldFromCount, const Container *toContainer, unsigned char to_slotid, const Item *toItem, int oldToCount, int count)
{
	if(player->NeedUpdateStats()) {
		player->sendStats();
	}
	
	NetworkMessage msg;
	
	//Auto-closing containers
	const Container* moveContainer = dynamic_cast<const Container*>(fromItem);
	bool updateContainerArrow = false;
	if(moveContainer) {
		bool hasContainerOpen = false;
		
		for(containerLayout::const_iterator cit = player->getContainers(); cit != player->getEndContainer(); ++cit){
			if(cit->second == toContainer || cit->second->getTopParent()->isHoldingItem(toContainer)){
				hasContainerOpen = true;
				break;
			}
		}
		
		if(!hasContainerOpen && !player->isHoldingContainer(toContainer)){
			autoCloseContainers(moveContainer, msg);
		}
		else
			updateContainerArrow = true;
	}
	//
	
	if(CanSee(fromPos.x, fromPos.y, fromPos.z)) {
		if(!fromItem->isStackable() || (oldFromCount == count && oldToCount + count <= 100)) {
			AddRemoveThing(msg, fromPos, stackpos);
		}
		else
			AddTileUpdated(msg, fromPos);
	}
	
	Container *container = NULL;
	for(containerLayout::const_iterator cit = player->getContainers(); cit != player->getEndContainer(); ++cit) {
		container = cit->second;
		if(container == toContainer) {
			unsigned char cid = cit->first;
			
			if(fromItem->isStackable()) {
				if(toItem && fromItem->getID() == toItem->getID() && toItem->getItemCountOrSubtype() != oldToCount) {
					//update count					
					TransformItemContainer(msg,cid,to_slotid,toItem);
					
					//surplus items
					if(oldToCount + count > 100) {
						//add item						
						AddItemContainer(msg,cid,fromItem,oldToCount + count - 100);
					}
				}
				else {
					//add item					
					AddItemContainer(msg,cid,fromItem,count);
				}
			}
			else {
				//add item				
				AddItemContainer(msg,cid,fromItem);
			}
		}
	}
	
	if(updateContainerArrow) {
		const Container* moveContainer = dynamic_cast<const Container*>(fromItem);
		if(moveContainer) {
			for(containerLayout::const_iterator cit = player->getContainers(); cit != player->getEndContainer(); ++cit) {
				
				if(cit->second == moveContainer) {
					sendContainer(cit->first, cit->second);
				}
			}
		}
	}
	
	WriteBuffer(msg);
}

//ground to inventory
void Protocol75::sendThingMove(const Creature *creature, const Position &fromPos, int stackpos, const Item* fromItem,
							   int oldFromCount, slots_t toSlot, const Item *toItem, int oldToCount, int count)
{
	if(player->NeedUpdateStats()) {
		player->sendStats();
	}
	
	NetworkMessage msg;
	
	if(creature == player) {
		AddPlayerInventoryItem(msg, player, toSlot);
	}
	else {
		//Auto-closing containers
		const Container* moveContainer = dynamic_cast<const Container*>(fromItem);
		if(moveContainer) {
			autoCloseContainers(moveContainer, msg);						
		}
	}
	
	if(CanSee(fromPos.x, fromPos.y, fromPos.z)) {
		if(!fromItem->isStackable() || (oldFromCount == count && oldToCount + count <= 100)) {
			AddRemoveThing(msg, fromPos, stackpos);
			
			if(toItem && (!toItem->isStackable() || toItem->getID() != fromItem->getID())) {
				AddAppearThing(msg, fromPos);
				msg.AddItem(toItem);
			}
		}
		else
			AddTileUpdated(msg, fromPos);
	}
	
	WriteBuffer(msg);
}

//ground to ground
void Protocol75::sendThingMove(const Creature *creature, const Thing *thing,
							   const Position *oldPos, unsigned char oldStackPos, unsigned char oldcount, unsigned char count, bool tele)
{
	NetworkMessage msg;
	
	const Creature* c = dynamic_cast<const Creature*>(thing);
	if(c && c->isRemoved)
		return;
	
	if (!tele && c && (CanSee(oldPos->x, oldPos->y, oldPos->z)) && (CanSee(thing->pos.x, thing->pos.y, thing->pos.z)))
	{
		msg.AddByte(0x6D);
		msg.AddPosition(*oldPos);
		msg.AddByte(oldStackPos);
		msg.AddPosition(thing->pos);
		
		Tile *fromTile = game->getTile(oldPos->x, oldPos->y, oldPos->z);
		if(fromTile && fromTile->getThingCount() > 8) {
			//We need to pop up this item
			Thing *newthing = fromTile->getThingByStackPos(9);
			
			if(newthing != NULL) {
				AddTileUpdated(msg, *oldPos);
			}
		}
	}
	else
	{
		if (!tele && CanSee(oldPos->x, oldPos->y, oldPos->z))
		{
			AddRemoveThing(msg,*oldPos,oldStackPos);
		}
		
		if (!(tele && thing == this->player) && CanSee(thing->pos.x, thing->pos.y, thing->pos.z))
		{
			AddAppearThing(msg,thing->pos);      
			if (c) {
				bool known;
				unsigned long removedKnown;
				checkCreatureAsKnown(((Creature*)thing)->getID(), known, removedKnown);
				AddCreature(msg,(Creature*)thing, known, removedKnown);
			}
			else {
				msg.AddItem((Item*)thing);
				
				//Auto-close container's
				const Container* moveContainer = dynamic_cast<const Container*>(thing);
				if(moveContainer){
					if(std::abs(player->pos.x - thing->pos.x) > 1 || std::abs(player->pos.y - thing->pos.y) > 1 || player->pos.z != thing->pos.z ){
						autoCloseContainers(moveContainer, msg);
					}
				}
			}
		}
	}
	
	if (thing == this->player) {
		if(tele){
			msg.AddByte(0x64); 
			msg.AddPosition(player->pos); 
			GetMapDescription(player->pos.x-8, player->pos.y-6, player->pos.z, 18, 14, msg); 
		}
		else{               
			if (oldPos->y > thing->pos.y) { // north, for old x
				msg.AddByte(0x65);
				GetMapDescription(oldPos->x - 8, thing->pos.y - 6, thing->pos.z, 18, 1, msg);
			} else if (oldPos->y < thing->pos.y) { // south, for old x
				msg.AddByte(0x67);
				GetMapDescription(oldPos->x - 8, thing->pos.y + 7, thing->pos.z, 18, 1, msg);
			}
			if (oldPos->x < thing->pos.x) { // east, [with new y]
				msg.AddByte(0x66);
				GetMapDescription(thing->pos.x + 9, thing->pos.y - 6, thing->pos.z, 1, 14, msg);
			} else if (oldPos->x > thing->pos.x) { // west, [with new y]
				msg.AddByte(0x68);
				GetMapDescription(thing->pos.x - 8, thing->pos.y - 6, thing->pos.z, 1, 14, msg);
			}
		}
		
		//Auto-close container's
		std::vector<Container *> containers;
		for(containerLayout::const_iterator cit = player->getContainers(); cit != player->getEndContainer(); ++cit) {
			Container *container = cit->second;
			
			while(container->getParent()) {
				container = container->getParent();
			}
			
			//Only add those we need to close
			if(container->pos.x != 0xFFFF) {				
				if(std::abs(player->pos.x - container->pos.x) > 1 || std::abs(player->pos.y - container->pos.y) > 1 || player->pos.z != container->pos.z) {
					containers.push_back(cit->second);
				}
			}
		}
		
		for(std::vector<Container *>::const_iterator it = containers.begin(); it != containers.end(); ++it) {
			autoCloseContainers(*it, msg);
		}
	}	
	
	WriteBuffer(msg);
}


//close container and its child containers
void Protocol75::autoCloseContainers(const Container *container, NetworkMessage &msg)
{
	std::vector<unsigned char> containerlist;
	for(containerLayout::const_iterator cit = player->getContainers(); cit != player->getEndContainer(); ++cit) {
		Container *tmpcontainer = cit->second;
		while(tmpcontainer != NULL) {
			if(tmpcontainer == container) {
				containerlist.push_back(cit->first);
				break;
			}
			
			tmpcontainer = tmpcontainer->getParent();
		}
	}
	
	for(std::vector<unsigned char>::iterator it = containerlist.begin(); it != containerlist.end(); ++it) {
		player->closeContainer(*it);
		msg.AddByte(0x6F);
		msg.AddByte(*it);
	}						
}

void Protocol75::sendCreatureTurn(const Creature *creature, unsigned char stackPos)
{
	if (CanSee(creature))
	{
		NetworkMessage msg;
		
		msg.AddByte(0x6B);
		msg.AddPosition(creature->pos);
		msg.AddByte(stackPos); 
		
		msg.AddByte(0x63);
		msg.AddByte(0x00);
		msg.AddU32(creature->getID());
		msg.AddByte(creature->getDirection());
		WriteBuffer(msg);
	}
}


void Protocol75::sendCreatureSay(const Creature *creature, SpeakClasses type, const std::string &text)
{
	NetworkMessage msg;
	AddCreatureSpeak(msg,creature, type, text, 0);
	WriteBuffer(msg);
}

void Protocol75::sendToChannel(const Creature * creature, SpeakClasses type, const std::string &text, unsigned short channelId){
	NetworkMessage msg;
	AddCreatureSpeak(msg,creature, type, text, channelId);
	WriteBuffer(msg);
}

void Protocol75::sendCancel(const char *msg)
{
	NetworkMessage netmsg;
	AddTextMessage(netmsg,MSG_SMALLINFO, msg);
	WriteBuffer(netmsg);
}

void Protocol75::sendCancelAttacking()
{
	NetworkMessage netmsg;
	netmsg.AddByte(0xa3);
	WriteBuffer(netmsg);
}

void Protocol75::sendChangeSpeed(const Creature *creature)
{
	NetworkMessage netmsg;
	netmsg.AddByte(0x8F);
	
	netmsg.AddU32(creature->getID());
	netmsg.AddU16(creature->getSpeed());
	WriteBuffer(netmsg);
}

void Protocol75::sendCancelWalk()
{
	NetworkMessage netmsg;
	netmsg.AddByte(0xB5);
	netmsg.AddByte(player->getDirection()); // direction
	WriteBuffer(netmsg);
}

void Protocol75::sendThingDisappear(const Thing *thing, unsigned char stackPos, bool tele)
{
	NetworkMessage msg;
	const Creature* creature = dynamic_cast<const Creature*>(thing);
	//Auto-close trade
	if(player->getTradeItem() && dynamic_cast<const Item*>(thing) == player->getTradeItem()) {
		game->playerCloseTrade(player);
	}
	
	if(creature && creature->isRemoved)
		return;
	
	if(!tele) {		
		if(creature && creature->health > 0){
			const Player* remove_player = dynamic_cast<const Player*>(creature);
			if(remove_player == player)
				return;
			
			if(CanSee(creature)){
				AddMagicEffect(msg,thing->pos, NM_ME_PUFF);
			}
		}
	}
	
	if(CanSee(thing->pos.x, thing->pos.y, thing->pos.z)) {
		AddRemoveThing(msg,thing->pos, stackPos);
		if(creature && stackPos > 9){
			AddCreatureHealth(msg,creature);
		}
		Tile *tile = game->getTile(thing->pos.x, thing->pos.y, thing->pos.z);
		if(tile && tile->getThingCount() > 8) {
			//We need to pop up this item
			Thing *newthing = tile->getThingByStackPos(9);
			if(newthing != NULL) {
				AddTileUpdated(msg, thing->pos);
			}
		}
	}
	
	WriteBuffer(msg);
}

void Protocol75::sendThingAppear(const Thing *thing){
	NetworkMessage msg;
	const Creature* creature = dynamic_cast<const Creature*>(thing);
	if(creature){
		const Player* add_player = dynamic_cast<const Player*>(creature);
		if(add_player == player){
			msg.AddByte(0x0A);
			msg.AddU32(player->getID());
			
			msg.AddByte(0x32);
			msg.AddByte(0x00);
			
			msg.AddByte(0x00); //can report bugs 0,1
			
			/*msg.AddByte(0x0B);//TODO?. GM actions
			  msg.AddByte(0xFF);msg.AddByte(0xFF);msg.AddByte(0xFF);msg.AddByte(0xFF);
			  msg.AddByte(0xFF);msg.AddByte(0xFF);msg.AddByte(0xFF);msg.AddByte(0xFF);
			  msg.AddByte(0xFF);msg.AddByte(0xFF);msg.AddByte(0xFF);msg.AddByte(0xFF);
			  msg.AddByte(0xFF);msg.AddByte(0xFF);msg.AddByte(0xFF);msg.AddByte(0xFF);
			  msg.AddByte(0xFF);msg.AddByte(0xFF);msg.AddByte(0xFF);msg.AddByte(0xFF);
			  msg.AddByte(0xFF);msg.AddByte(0xFF);msg.AddByte(0xFF);msg.AddByte(0xFF);
			  msg.AddByte(0xFF);msg.AddByte(0xFF);msg.AddByte(0xFF);msg.AddByte(0xFF);
			  msg.AddByte(0xFF);msg.AddByte(0xFF);msg.AddByte(0xFF);msg.AddByte(0xFF);
			*/
			
			msg.AddByte(0x64);
			msg.AddPosition(player->pos);
			GetMapDescription(player->pos.x-8, player->pos.y-6, player->pos.z, 18, 14, msg);
			
			AddMagicEffect(msg,player->pos, NM_ME_ENERGY_AREA);
			
			AddPlayerStats(msg,player);	
			
			msg.AddByte(0x82);
			msg.AddByte(0x6F); //LIGHT LEVEL
			msg.AddByte(0xd7);//light? (seems constant)
			
			/*msg.AddByte(0x8d);//8d
			  msg.AddU32(player->getID());
			  msg.AddByte(0x03);//00
			  msg.AddByte(0xd7);//d7
			*/
			
			AddPlayerSkills(msg,player);
			
			AddPlayerInventoryItem(msg,player, 1);
			AddPlayerInventoryItem(msg,player, 2);
			AddPlayerInventoryItem(msg,player, 3);
			AddPlayerInventoryItem(msg,player, 4);
			AddPlayerInventoryItem(msg,player, 5);
			AddPlayerInventoryItem(msg,player, 6);
			AddPlayerInventoryItem(msg,player, 7);
			AddPlayerInventoryItem(msg,player, 8);
			AddPlayerInventoryItem(msg,player, 9);
			AddPlayerInventoryItem(msg,player, 10);
			
			AddTextMessage(msg,MSG_EVENT, g_config.getGlobalString("loginmsg", "Welcome.").c_str());
			std::string tempstring;
			tempstring = "Your last visit was on ";
			time_t lastlogin = player->getLastLoginSaved();
			tempstring += ctime(&lastlogin);
			tempstring.erase(tempstring.length() -1);
			tempstring += ".";
			AddTextMessage(msg,MSG_EVENT, tempstring.c_str());
			WriteBuffer(msg);
			//force flush
			flushOutputBuffer();
			return;
		}
		else if(CanSee(creature)){
			bool known;
			unsigned long removedKnown;
			checkCreatureAsKnown(creature->getID(), known, removedKnown);
			AddAppearThing(msg,creature->pos);
			AddCreature(msg,creature, known, removedKnown);		
			// login bubble
			AddMagicEffect(msg,creature->pos, NM_ME_ENERGY_AREA);
		}
	}
	else if(CanSee(thing->pos.x, thing->pos.y, thing->pos.z))
	{
		const Item *item = dynamic_cast<const Item*>(thing);
		if(item){
			Tile *tile = game->getTile(item->pos.x,item->pos.y,item->pos.z);
			if(tile->getThingCount() > 8){
				AddTileUpdated(msg,item->pos);
			}
			else{
				AddAppearThing(msg,item->pos);
				msg.AddItem(item);
			}
		}
	}
	
	WriteBuffer(msg);
}

void Protocol75::sendThingTransform(const Thing* thing,int stackpos){
	if(CanSee(thing->pos.x, thing->pos.y, thing->pos.z)) {
		const Item *item = dynamic_cast<const Item*>(thing);
		if(item){
			NetworkMessage msg;
			if(stackpos == 0){
				AddTileUpdated(msg,thing->pos);
			}
			else if(stackpos < 10){
				msg.AddByte(0x6B);
				msg.AddPosition(thing->pos);
				msg.AddByte(stackpos);
				msg.AddItem(item);
			}
			else{
				return;
			}
			WriteBuffer(msg);
		}
		//update container icon
		if(dynamic_cast<const Container*>(item)){
			const Container *updateContainer = dynamic_cast<const Container*>(item);
			for(containerLayout::const_iterator cit = player->getContainers(); cit != player->getEndContainer(); ++cit) {
				Container *container = cit->second;
				if(container == updateContainer) {
					sendContainer(cit->first, container);
				}
			}
		}
	}
}

void Protocol75::sendSkills()
{
	NetworkMessage msg;
	AddPlayerSkills(msg,player);
	WriteBuffer(msg);
}

void Protocol75::sendPing()
{
	NetworkMessage msg;
	msg.AddByte(0x1E);
	WriteBuffer(msg);
}

void Protocol75::sendThingRemove(const Thing *thing){
	//Auto-close trade
	if(player->getTradeItem() && dynamic_cast<const Item*>(thing) == player->getTradeItem()) {
		game->playerCloseTrade(player);
	}
	
	NetworkMessage msg;

	//Auto-close container's
	const Container* moveContainer = dynamic_cast<const Container *>(thing);
	if(moveContainer) {
		autoCloseContainers(moveContainer, msg);		
	}

	WriteBuffer(msg);
}

void Protocol75::sendDistanceShoot(const Position &from, const Position &to, unsigned char type){
	NetworkMessage msg;
	AddDistanceShoot(msg,from, to,type );
	WriteBuffer(msg);
}
void Protocol75::sendMagicEffect(const Position &pos, unsigned char type){
	NetworkMessage msg;
	AddMagicEffect(msg,pos,type );
	WriteBuffer(msg);
}
void Protocol75::sendAnimatedText(const Position &pos, unsigned char color, std::string text){
	NetworkMessage msg;
	AddAnimatedText(msg,pos,color,text);
	WriteBuffer(msg);
}
void Protocol75::sendCreatureHealth(const Creature *creature){
	NetworkMessage msg;
	AddCreatureHealth(msg,creature);
	WriteBuffer(msg);
}

void Protocol75::sendItemAddContainer(const Container *container, const Item *item){
	NetworkMessage msg;
	unsigned char cid = player->getContainerID(container);
	if(cid != 0xFF){
		AddItemContainer(msg,cid,item);
		WriteBuffer(msg);
	}
}

void Protocol75::sendItemRemoveContainer(const Container *container, const unsigned char slot)   {
	NetworkMessage msg;
	unsigned char cid = player->getContainerID(container);
	if(cid != 0xFF){
		RemoveItemContainer(msg,cid,slot);
		WriteBuffer(msg);
	}
}

void Protocol75::sendItemUpdateContainer(const Container *container, const Item* item,const unsigned char slot){
	NetworkMessage msg;
	unsigned char cid = player->getContainerID(container);
	if(cid != 0xFF){
		TransformItemContainer(msg,cid,slot,item);
		WriteBuffer(msg);
	}
}

void Protocol75::sendTextWindow(Item* item,const unsigned short maxlen, const bool canWrite){
	NetworkMessage msg;				
	msg.AddByte(0x96);
	windowTextID++;
	msg.AddU32(windowTextID);
	//msg.AddU16(item->getID());
	msg.AddItemId(item);
	if(canWrite){
		msg.AddU16(maxlen);		
		msg.AddString(item->getText());		
		item->useThing();
		readItem = item;
	}
	else{		
		msg.AddU16(item->getText().size());
		msg.AddString(item->getText());									
		readItem = NULL;		
	}
	
	WriteBuffer(msg);
}

////////////// Add common messages
void Protocol75::AddTextMessage(NetworkMessage &msg,MessageClasses mclass, const char* message)
{
	msg.AddByte(0xB4);
	msg.AddByte(mclass);
	msg.AddString(message);
}

void Protocol75::AddAnimatedText(NetworkMessage &msg,const Position &pos, unsigned char color, std::string text)
{
#ifdef __DEBUG__
	if(text.length() == 0) {
		std::cout << "Warning: 0-Length string in AddAnimatedText()" << std::endl;
	}
#endif
	
	msg.AddByte(0x84); 
	msg.AddPosition(pos);
	msg.AddByte(color);
	msg.AddString(text);
}


void Protocol75::AddMagicEffect(NetworkMessage &msg,const Position &pos, unsigned char type)
{
	msg.AddByte(0x83);
	msg.AddPosition(pos);
	msg.AddByte(type);
}


void Protocol75::AddDistanceShoot(NetworkMessage &msg,const Position &from, const Position &to, unsigned char type)
{
	msg.AddByte(0x85); 
	msg.AddPosition(from);
	msg.AddPosition(to);
	msg.AddByte(type);
}


void Protocol75::AddCreature(NetworkMessage &msg,const Creature *creature, bool known, unsigned int remove)
{
	if (known)
	{
		msg.AddByte(0x62);
		msg.AddByte(0x00);
		msg.AddU32(creature->getID());
	}
	else
	{
		msg.AddByte(0x61);
		msg.AddByte(0x00);
		//AddU32(0);
		msg.AddU32(remove);
		msg.AddU32(creature->getID());
		msg.AddString(creature->getName());
	}
	
	msg.AddByte(creature->health*100/creature->healthmax);
	msg.AddByte((unsigned char)creature->getDirection());
	
	msg.AddByte(creature->looktype);
	msg.AddByte(creature->lookhead);
	msg.AddByte(creature->lookbody);
	msg.AddByte(creature->looklegs);
	msg.AddByte(creature->lookfeet);
	
	msg.AddByte(0x00); // light level
	msg.AddByte(0xDC); // light color
	
	msg.AddU16(creature->getSpeed());
	
	msg.AddByte(0x00); // skull
	msg.AddByte(0x00); // shield
}


void Protocol75::AddPlayerStats(NetworkMessage &msg,const Player *player)
{
	msg.AddByte(0xA0);
	msg.AddU16(player->getHealth());
	msg.AddU16(player->getPlayerInfo(PLAYERINFO_MAXHEALTH));
	msg.AddU16((unsigned short)std::floor(player->getFreeCapacity()));
	msg.AddU32(player->getExperience());
	msg.AddByte(player->getPlayerInfo(PLAYERINFO_LEVEL));
	msg.AddByte(player->getPlayerInfo(PLAYERINFO_LEVELPERCENT));
	msg.AddU16(player->getMana());
	msg.AddU16(player->getPlayerInfo(PLAYERINFO_MAXMANA));
	msg.AddByte(player->getMagicLevel());
	msg.AddByte(player->getPlayerInfo(PLAYERINFO_MAGICLEVELPERCENT));
	msg.AddByte(player->getPlayerInfo(PLAYERINFO_SOUL));
}

void Protocol75::AddPlayerSkills(NetworkMessage &msg,const Player *player)
{
	msg.AddByte(0xA1);
	
	msg.AddByte(player->getSkill(SKILL_FIST,   SKILL_LEVEL));
	msg.AddByte(player->getSkill(SKILL_FIST,   SKILL_PERCENT));
	msg.AddByte(player->getSkill(SKILL_CLUB,   SKILL_LEVEL));
	msg.AddByte(player->getSkill(SKILL_CLUB,   SKILL_PERCENT));
	msg.AddByte(player->getSkill(SKILL_SWORD,  SKILL_LEVEL));
	msg.AddByte(player->getSkill(SKILL_SWORD,  SKILL_PERCENT));
	msg.AddByte(player->getSkill(SKILL_AXE,    SKILL_LEVEL));
	msg.AddByte(player->getSkill(SKILL_AXE,    SKILL_PERCENT));
	msg.AddByte(player->getSkill(SKILL_DIST,   SKILL_LEVEL));
	msg.AddByte(player->getSkill(SKILL_DIST,   SKILL_PERCENT));
	msg.AddByte(player->getSkill(SKILL_SHIELD, SKILL_LEVEL));
	msg.AddByte(player->getSkill(SKILL_SHIELD, SKILL_PERCENT));
	msg.AddByte(player->getSkill(SKILL_FISH,   SKILL_LEVEL));
	msg.AddByte(player->getSkill(SKILL_FISH,   SKILL_PERCENT));
}


void Protocol75::AddPlayerInventoryItem(NetworkMessage &msg,const Player *player, int item)
{
	if (player->getItem(item) == NULL)
	{
		msg.AddByte(0x79);
		msg.AddByte(item);
	}
	else
	{
		msg.AddByte(0x78);
		msg.AddByte(item);
		msg.AddItem(player->getItem(item));
	}
}


void Protocol75::AddCreatureSpeak(NetworkMessage &msg,const Creature *creature, SpeakClasses  type, std::string text, unsigned short channelId)
{
	msg.AddByte(0xAA);
	msg.AddString(creature->getName());
	msg.AddByte(type);
	switch(type){
	case SPEAK_SAY:
	case SPEAK_WHISPER:
	case SPEAK_YELL:
	case SPEAK_MONSTER1:
	case SPEAK_MONSTER2:
		msg.AddPosition(creature->pos);
		break;	
	case SPEAK_CHANNEL_Y:
	case SPEAK_CHANNEL_R1:
	case SPEAK_CHANNEL_R2:
		msg.AddU16(channelId);
		break;	
	}
	msg.AddString(text);
}

void Protocol75::AddCreatureHealth(NetworkMessage &msg,const Creature *creature)
{
	msg.AddByte(0x8C);
	msg.AddU32(creature->getID());
	msg.AddByte(std::max(creature->health,0)*100/creature->healthmax);
}

void Protocol75::AddRemoveThing(NetworkMessage &msg, const Position &pos,unsigned char stackpos){
	if(stackpos < 10) {
		if(CanSee(pos.x, pos.y, pos.z)) {
			msg.AddByte(0x6C);
			msg.AddPosition(pos);
			msg.AddByte(stackpos);
		}
	}
	else {
		//This will cause some problem, we remove an item (example: a player gets removed due to death) from the map, but the client cant see it
		//(above the 9 limit), real tibia has the same problem so I don't think there is a way to fix this.
		//Problem: The client won't be informed that the player has been killed
		//and will show the player as alive (0 hp).
		//Solution: re-log.
	}
	
	Tile *fromTile = game->getTile(pos.x, pos.y, pos.z);
	if(fromTile && fromTile->getThingCount() > 8) {
		//We need to pop up this item
		Thing *newthing = fromTile->getThingByStackPos(9);
		
		if(newthing != NULL) {
			AddTileUpdated(msg, pos);
		}
	}
}

void Protocol75::AddAppearThing(NetworkMessage &msg, const Position &pos){
	if(CanSee(pos.x, pos.y, pos.z)) {		
		msg.AddByte(0x6A);
		msg.AddPosition(pos);	
	}
}

void Protocol75::AddItemContainer(NetworkMessage &msg,unsigned char cid, const Item *item){
	msg.AddByte(0x70);
	msg.AddByte(cid);
	msg.AddItem(item);
}

void Protocol75::AddItemContainer(NetworkMessage &msg,unsigned char cid, const Item *item,unsigned char count){
	msg.AddByte(0x70);
	msg.AddByte(cid);
	msg.AddItem(item->getID(), count);
}

void Protocol75::TransformItemContainer(NetworkMessage &msg,unsigned char cid,unsigned char slot,const  Item *item){
	msg.AddByte(0x71);
	msg.AddByte(cid);
	msg.AddByte(slot);
	msg.AddItem(item);
}

void Protocol75::RemoveItemContainer(NetworkMessage &msg,unsigned char cid,unsigned char slot){
	msg.AddByte(0x72);
	msg.AddByte(cid);
	msg.AddByte(slot);
}

//////////////////////////

void Protocol75::flushOutputBuffer(){
	OTSYS_THREAD_LOCK_CLASS lockClass(bufferLock, "Protocol75::flushOutputBuffer()");
	//force writetosocket	
	OutputBuffer.WriteToSocket(s);
	OutputBuffer.Reset();
	
	return;
}

void Protocol75::WriteBuffer(NetworkMessage &add){
	
	game->addPlayerBuffer(player);	
	
	OTSYS_THREAD_LOCK(bufferLock, "Protocol75::WriteBuffer")
		if(OutputBuffer.getMessageLength() + add.getMessageLength() >= NETWORKMESSAGE_MAXSIZE - 16){
			this->flushOutputBuffer();
		}	
		OutputBuffer.JoinMessages(add);	
		OTSYS_THREAD_UNLOCK(bufferLock, "Protocol75::WriteBuffer")	
			return;
}



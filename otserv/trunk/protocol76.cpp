//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Implementation of tibia v7.6 protocoll
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
#include "protocol76.h"

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
#include "ioplayer.h"
#include "house.h"
#include "waitlist.h"

extern LuaScript g_config;
extern Actions actions;
Chat g_chat;

Protocol76::Protocol76(SOCKET s)
{
	OTSYS_THREAD_LOCKVARINIT(bufferLock);
	windowTextID = 0;
	readItem = NULL;
	maxTextLenght = 0;
	this->s = s;
}


Protocol76::~Protocol76()
{
	OTSYS_THREAD_LOCKVARRELEASE(bufferLock);	
}

void Protocol76::reinitializeProtocol()
{
	windowTextID = 0;
	readItem = NULL;
	maxTextLenght = 0;
	OutputBuffer.Reset();
	knownPlayers.clear();
}

connectResult_t Protocol76::ConnectPlayer()
{	
	Waitlist* wait = Waitlist::instance();

	if(player->access == 0 && !wait->clientLogin(player->getAccount(), player->getIP())){	
		return CONNECT_TOMANYPLAYERS;
	}
	else{
		//last login position
		if(game->placeCreature(player->getLoginPosition(), player)){
			return CONNECT_SUCCESS;
		}
		//temple
		else if(game->placeCreature(player->masterPos, player, true)){
			return CONNECT_SUCCESS;
		}
		else
			return CONNECT_MASTERPOSERROR;
	}

	return CONNECT_INTERNALERROR;
}


void Protocol76::ReceiveLoop()
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
			while(player->inFightTicks >= 1000 && !player->isRemoved() && s == 0){
				OTSYS_SLEEP(250);
			}

			OTSYS_THREAD_LOCK(game->gameLock, "Protocol76::ReceiveLoop()")

			if(s == 0 && !player->isRemoved()){
				game->removeCreature(player);
			}
			
			OTSYS_THREAD_UNLOCK(game->gameLock, "Protocol76::ReceiveLoop()")
		}
	}while(s != 0 && !player->isRemoved());
}


void Protocol76::parsePacket(NetworkMessage &msg)
{
	if(msg.getMessageLength() <= 0)
		return;
	
	uint8_t recvbyte = msg.GetByte();
	//a dead player can not performs actions
	if(player->isRemoved() && recvbyte != 0x14){
		OTSYS_SLEEP(10);
		return;
	}	
    
	switch(recvbyte){
	case 0x14: // logout
		parseLogout(msg);
		break;
		
	case 0x1E: // keep alive / ping response
		player->receivePing();
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
	
	case 0x69: // client quit without logout <- wrong
		if(game->stopEvent(player->eventAutoWalk)){
			sendCancelWalk();
		}
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
		
	case 0x78: // throw item
		parseThrow(msg);
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
		
	case 0x82: // use item
		parseUseItem(msg);
		break;
		
	case 0x83: // use item
		parseUseItemEx(msg);
		break;
		
	case 0x84: // battle window
		parseBattleWindow(msg);
		break;
	
	case 0x85:	//rotate item
		parseRotateItem(msg);
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
	
	case 0x8A:
		parseHouseWindow(msg);
		break;
	
	case 0x8C: // throw item
		parseLookAt(msg);
		break;
		
	case 0x96:  // say something
		parseSay(msg);
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
	
	case 0xA0: // set attack and follow mode
		parseModes(msg);
		break;	
	
	case 0xA1: // attack
		parseAttack(msg);
		break;
	
	case 0xA2: //follow
		parseFollow(msg);
		break;
	
	case 0xBE: // cancel move
		parseCancelMove(msg);
		break;
	
	case 0xD2: // request Outfit
		parseRequestOutfit(msg);
		break;
		
	case 0xD3: // set outfit
		parseSetOutfit(msg);
		break;
	
	case 0xDC:
		parseAddVip(msg);
		break;
		
	case 0xDD:
		parseRemVip(msg);
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

void Protocol76::GetTileDescription(const Tile* tile, NetworkMessage &msg)
{
	if(tile){
		int count = 0;
		if(tile->ground){
			msg.AddItem(tile->ground);
			count++;
		}
		
		ItemVector::const_iterator it;
		for(it = tile->topItems.begin(); ((it != tile->topItems.end()) && (count < 10)); ++it){
			msg.AddItem(*it);
			count++;
		}
		
		CreatureVector::const_iterator itc;
		for(itc = tile->creatures.begin(); ((itc != tile->creatures.end()) && (count < 10)); ++itc){
			bool known;
			unsigned long removedKnown;
			checkCreatureAsKnown((*itc)->getID(), known, removedKnown);
			AddCreature(msg,*itc, known, removedKnown);
			count++;
		}
		
		for(it = tile->downItems.begin(); ((it != tile->downItems.end()) && (count < 10)); ++it){
			msg.AddItem(*it);
			count++;
		}
	}
}

void Protocol76::GetMapDescription(unsigned short x, unsigned short y, unsigned char z,
	unsigned short width, unsigned short height, NetworkMessage &msg)
{
	int skip = -1;
	int startz, endz, zstep = 0;

	if (z > 7) {
		startz = z - 2;
		endz = std::min(MAP_MAX_LAYERS - 1, z + 2);
		zstep = 1;
	}
	else {
		startz = 7;
		endz = 0;
		
		zstep = -1;
	}
	
	for(int nz = startz; nz != endz + zstep; nz += zstep){
		GetFloorDescription(msg, x, y, nz, width, height, z - nz, skip);
	}
	
	if(skip >= 0){
		msg.AddByte(skip);
		msg.AddByte(0xFF);
		//cc += skip;
	}
	
#ifdef __DEBUG__
	//printf("tiles in total: %d \n", cc);
#endif
}

void Protocol76::GetFloorDescription(NetworkMessage& msg, int x, int y, int z, int width, int height, int offset, int& skip)
{
	Tile* tile;

	for(int nx = 0; nx < width; nx++){
		for(int ny = 0; ny < height; ny++){
			tile = game->getTile(x + nx + offset, y + ny + offset, z);
			if(tile){
				if(skip >= 0){
					msg.AddByte(skip);
					msg.AddByte(0xFF);
				}   
				skip = 0;
				
				GetTileDescription(tile, msg);
			}
			else {
				skip++;
				if(skip == 0xFF){
					msg.AddByte(0xFF);
					msg.AddByte(0xFF);
					skip = -1;
				}
			}
		}
	}
}

void Protocol76::checkCreatureAsKnown(unsigned long id, bool &known, unsigned long &removedKnown)
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

bool Protocol76::CanSee(const Position& pos) const
{
	return CanSee(pos.x, pos.y, pos.z);
}

bool Protocol76::CanSee(int x, int y, int z) const
{
#ifdef __DEBUG__
	if(z < 0 || z >= MAP_MAX_LAYERS) {
		std::cout << "WARNING! Protocol76::CanSee() Z-value is out of range!" << std::endl;
	}
#endif

	const Position& myPos = player->getPosition();

	//we are on ground level or above (7 -> 0)
	if(myPos.z <= 7){
		//view is from 7 -> 0
		if(z > 7){
			return false;
		}
	}
	//we are underground (8 -> 15)
	else if(myPos.z >= 8){
		//view is +/- 2 from the floor we stand on
		if(std::abs(myPos.z - z) > 2){
			return false;
		}
	}

	/*
	//underground 8->15
	if(myPos.z > 7 && z < 6){
		return false;
	}
	//ground level and above 7->0
	else if(myPos.z <= 7 && z > 7){
		return false;
	}
	*/
	
	//negative offset means that the action taken place is on a lower floor than ourself
	int offsetz = myPos.z - z;
	
	if ((x >= myPos.x - 8 + offsetz) && (x <= myPos.x + 9 + offsetz) &&
		(y >= myPos.y - 6 + offsetz) && (y <= myPos.y + 7 + offsetz))
		return true;
	
	return false;
}

bool Protocol76::CanSee(const Creature* c) const
{
	if(c->isRemoved())
		return false;
	
	Position pos = c->getPosition();
	return CanSee(pos.x, pos.y, pos.z);
}


void Protocol76::logout()
{
	// we ask the game to remove us
	if(!player->isRemoved()){
		if(game->removeCreature(player))
			pendingLogout = true;
	}
	else{
		pendingLogout = true;
	}
}

// Parse methods
void Protocol76::parseLogout(NetworkMessage& msg)
{
	if(player->inFightTicks >=1000 && !player->isRemoved()){
		player->sendCancelMessage(RET_YOUMAYNOTLOGOUTDURINGAFIGHT);
		return;
	}
	else{
		logout();
	}
}



void Protocol76::parseGetChannels(NetworkMessage& msg)
{
	sendChannelsDialog();
}

void Protocol76::parseOpenChannel(NetworkMessage& msg)
{
	unsigned short channelId = msg.GetU16();
	OTSYS_THREAD_LOCK_CLASS lockClass(game->gameLock, "Protocol76::parseOpenChannel()");
	if(g_chat.addUserToChannel(player, channelId))
		sendChannel(channelId, g_chat.getChannelName(player, channelId));
}

void Protocol76::parseCloseChannel(NetworkMessage &msg)
{
	unsigned short channelId = msg.GetU16();
	OTSYS_THREAD_LOCK_CLASS lockClass(game->gameLock, "Protocol76::parseCloseChannel()");
	g_chat.removeUserFromChannel(player, channelId);
}

void Protocol76::parseOpenPriv(NetworkMessage& msg)
{
	std::string receiver; 
	receiver = msg.GetString();
	OTSYS_THREAD_LOCK_CLASS lockClass(game->gameLock, "Protocol76::parseOpenPriv()");
	Player* player = game->getPlayerByName(receiver);
	if(player)
		sendOpenPriv(player->getName());
}

void Protocol76::parseCancelMove(NetworkMessage& msg)
{
	game->playerSetAttackedCreature(player, 0);
	
	game->stopEvent(player->eventAutoWalk);
	player->sendCancelWalk();
}

void Protocol76::parseModes(NetworkMessage& msg)
{
	long fightMode = msg.GetByte();
	long followMode = msg.GetByte();
	//std::cout << "fight " << fithgMode << ". follow " << followMode << std::endl;
	//player->fightMode = msg.GetByte();
	//player->followMode = msg.GetByte();
}

void Protocol76::parseDebug(NetworkMessage& msg)
{
	int dataLength = msg.getMessageLength() - 3;
	if(dataLength != 0){
		printf("data: ");
		size_t data = msg.GetByte();
		while(dataLength > 0){
			printf("%d ", data);
			if(--dataLength > 0)
				data = msg.GetByte();
		}
		printf("\n");
	}
}

void Protocol76::parseMoveByMouse(NetworkMessage& msg)
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

void Protocol76::parseMoveNorth(NetworkMessage& msg)
{
	if(game->stopEvent(player->eventAutoWalk)) {
		player->sendCancelWalk();
	}
	
	this->sleepTillMove();
	
	game->moveCreature(player, NORTH);
}

void Protocol76::parseMoveEast(NetworkMessage& msg)
{
	if(game->stopEvent(player->eventAutoWalk)) {
		player->sendCancelWalk();
	}
	
	this->sleepTillMove();
	
	game->moveCreature(player, EAST);
}

void Protocol76::parseMoveSouth(NetworkMessage& msg)
{
	if(game->stopEvent(player->eventAutoWalk)) {
		player->sendCancelWalk();
	}
	
	this->sleepTillMove();
	
	game->moveCreature(player, SOUTH);
}

void Protocol76::parseMoveWest(NetworkMessage& msg)
{
	if(game->stopEvent(player->eventAutoWalk)) {
		player->sendCancelWalk();
	}
	
	this->sleepTillMove();

	game->moveCreature(player, WEST);
}

void Protocol76::parseMoveNorthEast(NetworkMessage& msg)
{
	if(game->stopEvent(player->eventAutoWalk)) {
		player->sendCancelWalk();
	}
	
	this->sleepTillMove();
	this->sleepTillMove();
	
	game->moveCreature(player, NORTHEAST);
}

void Protocol76::parseMoveSouthEast(NetworkMessage& msg)
{
	if(game->stopEvent(player->eventAutoWalk)) {
		player->sendCancelWalk();
	}
	
	this->sleepTillMove();
	this->sleepTillMove();
	
	game->moveCreature(player, SOUTHEAST);
}

void Protocol76::parseMoveSouthWest(NetworkMessage& msg)
{
	if(game->stopEvent(player->eventAutoWalk)) {
		player->sendCancelWalk();
	}
	
	this->sleepTillMove();
	this->sleepTillMove();
	
	game->moveCreature(player, SOUTHWEST);
}

void Protocol76::parseMoveNorthWest(NetworkMessage& msg)
{
	if(game->stopEvent(player->eventAutoWalk)) {
		player->sendCancelWalk();
	}
	
	this->sleepTillMove();
	this->sleepTillMove();
	
	game->moveCreature(player, NORTHWEST);
}

void Protocol76::parseTurnNorth(NetworkMessage& msg)
{
	game->playerTurn(player, NORTH);
}

void Protocol76::parseTurnEast(NetworkMessage& msg)
{
	game->playerTurn(player, EAST);
}

void Protocol76::parseTurnSouth(NetworkMessage& msg)
{
	game->playerTurn(player, SOUTH);
}

void Protocol76::parseTurnWest(NetworkMessage& msg)
{
	game->playerTurn(player, WEST);
}

void Protocol76::parseRequestOutfit(NetworkMessage& msg)
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

void Protocol76::parseSetOutfit(NetworkMessage& msg)
{
	int temp = msg.GetByte();
	if ( (player->getSex() == PLAYERSEX_FEMALE && temp >= PLAYER_FEMALE_1 && temp <= PLAYER_FEMALE_7) ||
		(player->getSex() == PLAYERSEX_MALE && temp >= PLAYER_MALE_1 && temp <= PLAYER_MALE_7))
	{
		player->looktype = temp;
		player->lookmaster = player->looktype;
		player->lookhead = msg.GetByte();
		player->lookbody = msg.GetByte();
		player->looklegs = msg.GetByte();
		player->lookfeet = msg.GetByte();
		
		game->playerChangeOutfit(player);
	}
}

void Protocol76::parseUseItem(NetworkMessage& msg)
{
	Position pos = msg.GetPosition();
	uint16_t itemId = msg.GetItemId();
	uint8_t stackpos = msg.GetByte();
	uint8_t index = msg.GetByte();
/*	
#ifdef __DEBUG__
	std::cout << "parseUseItem: " << "x: " << pos.x << ", y: " << (int)pos.y <<  ", z: " << (int)pos.z << ", item: " << (int)itemId << ", stack: " << (int)stackpos << ", index: " << (int)index << std::endl;
#endif
*/
	game->playerUseItem(player, pos,stackpos, index, itemId);
}

void Protocol76::parseUseItemEx(NetworkMessage& msg)
{
	Position fromPos = msg.GetPosition();
	uint16_t fromItemId = msg.GetItemId();
	uint8_t fromStackpos = msg.GetByte();
	Position toPos = msg.GetPosition();
	uint16_t toItemId = msg.GetU16();
	uint8_t toStackpos = msg.GetByte();
	
	game->playerUseItemEx(player, fromPos, fromStackpos, fromItemId, toPos, toStackpos, toItemId);
}

void Protocol76::parseBattleWindow(NetworkMessage &msg)
{
	Position fromPos = msg.GetPosition();
	uint16_t itemId = msg.GetItemId();
	uint8_t fromStackPos = msg.GetByte();
	uint32_t creatureId = msg.GetU32();

	game->playerUseBattleWindow(player, fromPos, fromStackPos, creatureId, itemId);
}

void Protocol76::parseCloseContainer(NetworkMessage& msg)
{
	unsigned char containerid = msg.GetByte();

	OTSYS_THREAD_LOCK_CLASS lockClass(game->gameLock, "Protocol76::parseCloseContainer()");
	player->closeContainer(containerid);
	sendCloseContainer(containerid);
}

void Protocol76::parseUpArrowContainer(NetworkMessage& msg)
{
	uint32_t cid = msg.GetByte();
	OTSYS_THREAD_LOCK_CLASS lockClass(game->gameLock, "Protocol76::parseUpArrowContainer()");
	Container* container = player->getContainer(cid);
	if(!container)
		return;
	
	Container* parentcontainer = dynamic_cast<Container*>(container->getParent());
	if(parentcontainer){
		bool hasParent = (dynamic_cast<const Container*>(parentcontainer->getParent()) != NULL);
		player->addContainer(cid, parentcontainer);
		sendContainer(cid, parentcontainer, hasParent);
	}
}

void Protocol76::parseThrow(NetworkMessage& msg)
{
	Position fromPos = msg.GetPosition();
	uint16_t itemId = msg.GetItemId();
	uint8_t fromStackpos = msg.GetByte();
	Position toPos = msg.GetPosition();
	uint8_t count = msg.GetByte();

	/*
	std::cout << "parseThrow: " << "from_x: " << (int)fromPos.x << ", from_y: " << (int)fromPos.y
	<<  ", from_z: " << (int)fromPos.z << ", item: " << (int)itemId << ", fromStackpos: " 
	<< (int)fromStackpos << " to_x: " << (int)toPos.x << ", to_y: " << (int)toPos.y
	<<  ", to_z: " << (int)toPos.z  
	<< ", count: " << (int)count << std::endl;
	*/

	if(toPos == fromPos)
		return;

	game->thingMove(player, fromPos, itemId, fromStackpos, toPos, count);
}

void Protocol76::parseLookAt(NetworkMessage& msg)
{
	Position pos = msg.GetPosition();
	uint16_t itemId = msg.GetItemId();
	uint8_t stackpos = msg.GetByte();
	
	/*
	#ifdef __DEBUG__
	ss << "You look at x: " << x <<", y: " << y << ", z: " << z << " and see Item # " << itemId << ".";
	AddTextMessage(newmsg,MSG_INFO, ss.str().c_str());
	#endif
	*/

	game->playerLookAt(player, pos, itemId, stackpos);
}

void Protocol76::parseSay(NetworkMessage& msg)
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
	
	if(game->playerSaySpell(player, text))
		type = SPEAK_SAY;
	
	switch (type)
	{
	case SPEAK_SAY:
		game->playerSay(player, type, text);
		break;
	case SPEAK_WHISPER:
		game->playerWhisper(player, text);
		break;
	case SPEAK_YELL:
		game->playerYell(player, text);
		break;
	case SPEAK_PRIVATE:
	case SPEAK_PRIVATE_RED:
		game->playerSpeakTo(player, type, receiver, text);
		break;
	case SPEAK_CHANNEL_Y:
	case SPEAK_CHANNEL_R1:
	case SPEAK_CHANNEL_R2:
		game->playerTalkToChannel(player, type, text, channelId);
		break;
	case SPEAK_BROADCAST:
		game->playerBroadcastMessage(player, text);
		break;
	}
}

void Protocol76::parseAttack(NetworkMessage& msg)
{
	unsigned long creatureid = msg.GetU32();
	game->playerSetAttackedCreature(player, creatureid);
}

void Protocol76::parseFollow(NetworkMessage& msg)
{
	unsigned long creatureid = msg.GetU32();
	//std::cout << "follow " << creatureid << std::endl;
}

void Protocol76::parseTextWindow(NetworkMessage& msg)
{
	unsigned long id = msg.GetU32();
	std::string new_text = msg.GetString();
	if(new_text.length() > maxTextLenght)
		return;
	
	if(readItem && windowTextID == id){
		game->playerWriteItem(player, readItem, new_text);
		readItem->releaseThing2();
		readItem = NULL;
	}
}

void Protocol76::parseHouseWindow(NetworkMessage &msg)
{
	unsigned char _listid = msg.GetByte();
	unsigned long id = msg.GetU32();
	std::string new_list = msg.GetString();
	
	if(house && windowTextID == id && _listid == 0){
		house->setAccessList(listId, new_list);
		house = NULL;
		listId = 0;
	}
}

void Protocol76::parseRequestTrade(NetworkMessage& msg)
{
	Position pos = msg.GetPosition();
	uint16_t itemId = msg.GetItemId();
	uint8_t stackpos = msg.GetByte();
	uint32_t playerId = msg.GetU32();
	
	game->playerRequestTrade(player, pos, stackpos, playerId, itemId);
}

void Protocol76::parseAcceptTrade(NetworkMessage& msg)
{
	game->playerAcceptTrade(player);
}

void Protocol76::parseLookInTrade(NetworkMessage& msg)
{
	bool counterOffer = (msg.GetByte() == 0x01);
	int index = msg.GetByte();
	
	game->playerLookInTrade(player, counterOffer, index);
}

void Protocol76::parseCloseTrade()
{
	game->playerCloseTrade(player);
}

void Protocol76::parseAddVip(NetworkMessage& msg)
{
	std::string vip_name = msg.GetString();
	if(vip_name.size() > 32)
		return;

	game->playerRequestAddVip(player, vip_name);
}

void Protocol76::parseRemVip(NetworkMessage& msg)
{
	unsigned long id = msg.GetU32();
	player->removeVIP(id);
}

void Protocol76::parseRotateItem(NetworkMessage& msg)
{
	Position pos = msg.GetPosition();
	uint16_t itemId = msg.GetItemId();
	uint8_t stackpos = msg.GetByte();
	
	game->playerRotateItem(player, pos, stackpos, itemId);
}

// Send methods
void Protocol76::sendOpenPriv(const std::string& receiver)
{
	NetworkMessage newmsg; 
	newmsg.AddByte(0xAD); 
	newmsg.AddString(receiver);      
	WriteBuffer(newmsg);
}

void Protocol76::sendSetOutfit(const Creature* creature)
{
	if(CanSee(creature)){
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

void Protocol76::sendCreatureLight(const Creature* creature)
{
	if(CanSee(creature)){
		NetworkMessage msg;
		AddCreatureLight(msg, creature);
		WriteBuffer(msg);
	}
}

void Protocol76::sendWorldLight(const LightInfo& lightInfo)
{
	NetworkMessage msg; 
	AddWorldLight(msg, lightInfo);
	WriteBuffer(msg);
}

void Protocol76::sendCreatureSkull(const Creature* creature)
{
	if(CanSee(creature)){
		NetworkMessage msg;
		msg.AddByte(0x90);
		msg.AddU32(creature->getID());
		#ifdef __SKULLSYSTEM__
		if(const Player* playerSkull = creature->getPlayer()){
			msg.AddByte(player->getSkullClient(playerSkull));
		}
		else{
			msg.AddByte(0);	//no skull
		}
		#else
		msg.AddByte(0);	//no skull
		#endif
		
		WriteBuffer(msg);
	}
}

void Protocol76::sendCreatureShield(const Creature* creature)
{
	if(CanSee(creature)){
		NetworkMessage msg;
		msg.AddByte(0x91);
		msg.AddU32(creature->getID());
		msg.AddByte(0);	//no shield
		WriteBuffer(msg);
	}
}

void Protocol76::sendCreatureSquare(const Creature* creature, unsigned char color)
{
	if(CanSee(creature)){
		NetworkMessage msg;
		msg.AddByte(0x86);
		msg.AddU32(creature->getID());
		msg.AddByte(color);
		WriteBuffer(msg);
	}
}

void Protocol76::sendStats()
{
	NetworkMessage msg;
	AddPlayerStats(msg);
	WriteBuffer(msg);
}

void Protocol76::sendTextMessage(MessageClasses mclass, const char* message)
{
	NetworkMessage msg;
	AddTextMessage(msg,mclass, message);
	WriteBuffer(msg);
}

void Protocol76::sendTextMessage(MessageClasses mclass, const char* message,
	const Position &pos, unsigned char type)
{
	NetworkMessage msg;
	AddMagicEffect(msg,pos,type);
	AddTextMessage(msg,mclass, message);
	WriteBuffer(msg);
}

void Protocol76::sendChannelsDialog()
{
	NetworkMessage newmsg;
	ChannelList list;
	
	list = g_chat.getChannelList(player);
	
	newmsg.AddByte(0xAB);
	
	newmsg.AddByte(list.size()); //how many
	
	while(list.size()){
		ChatChannel *channel;
		channel = list.front();
		list.pop_front();
		
		newmsg.AddU16(channel->getId());
		newmsg.AddString(channel->getName());
	}

	WriteBuffer(newmsg);
}

void Protocol76::sendChannel(unsigned short channelId, std::string channelName)
{
	NetworkMessage newmsg;
	
	newmsg.AddByte(0xAC);
	newmsg.AddU16(channelId);
	newmsg.AddString(channelName);
	
	WriteBuffer(newmsg); 
}

void Protocol76::sendIcons(int icons)
{
	NetworkMessage newmsg;
	newmsg.AddByte(0xA2);
	newmsg.AddByte(icons);
	WriteBuffer(newmsg);
}

void Protocol76::sendContainer(uint32_t cid, const Container* container, bool hasParent)
{
	NetworkMessage msg;
	
	msg.AddByte(0x6E);
	msg.AddByte(cid);
	
	msg.AddItemId(container);
	msg.AddString(container->getName());
	msg.AddByte(container->capacity());
	msg.AddByte(hasParent ? 0x01 : 0x00);
	msg.AddByte(container->size());
	
	ItemList::const_iterator cit;
	for(cit = container->getItems(); cit != container->getEnd(); ++cit){
		msg.AddItem(*cit);
	}

	WriteBuffer(msg);
}

void Protocol76::sendTradeItemRequest(const Player* player, const Item* item, bool ack)
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
		
		ItemList::const_iterator it;
		
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

void Protocol76::sendCloseTrade()
{
	NetworkMessage msg;
	msg.AddByte(0x7F);
	
	WriteBuffer(msg);
}

void Protocol76::sendCloseContainer(uint32_t cid)
{
	NetworkMessage msg;
	
	msg.AddByte(0x6F);
	msg.AddByte(cid);
	WriteBuffer(msg);
}

void Protocol76::sendCreatureTurn(const Creature* creature, unsigned char stackPos)
{
	if(CanSee(creature)){
		NetworkMessage msg;
		
		msg.AddByte(0x6B);
		msg.AddPosition(creature->getPosition());
		msg.AddByte(stackPos); 
		
		//msg.AddByte(0x63);
		//msg.AddByte(0x00);
		msg.AddU16(0x63);
		msg.AddU32(creature->getID());
		msg.AddByte(creature->getDirection());
		WriteBuffer(msg);
	}
}

void Protocol76::sendCreatureSay(const Creature *creature, SpeakClasses type, const std::string &text)
{
	NetworkMessage msg;
	AddCreatureSpeak(msg,creature, type, text, 0);
	WriteBuffer(msg);
}

void Protocol76::sendToChannel(const Creature * creature, SpeakClasses type, const std::string &text, unsigned short channelId){
	NetworkMessage msg;
	AddCreatureSpeak(msg,creature, type, text, channelId);
	WriteBuffer(msg);
}

void Protocol76::sendCancel(const char *msg)
{
	NetworkMessage netmsg;
	AddTextMessage(netmsg,MSG_SMALLINFO, msg);
	WriteBuffer(netmsg);
}

void Protocol76::sendCancelAttacking()
{
	NetworkMessage netmsg;
	netmsg.AddByte(0xa3);
	WriteBuffer(netmsg);
}

void Protocol76::sendChangeSpeed(const Creature *creature)
{
	NetworkMessage netmsg;
	netmsg.AddByte(0x8F);
	
	netmsg.AddU32(creature->getID());
	netmsg.AddU16(creature->getSpeed());
	WriteBuffer(netmsg);
}

void Protocol76::sendCancelWalk()
{
	NetworkMessage netmsg;
	netmsg.AddByte(0xB5);
	netmsg.AddByte(player->getDirection()); // direction
	WriteBuffer(netmsg);
}

void Protocol76::sendSkills()
{
	NetworkMessage msg;
	AddPlayerSkills(msg);
	WriteBuffer(msg);
}

void Protocol76::sendPing()
{
	NetworkMessage msg;
	msg.AddByte(0x1E);
	WriteBuffer(msg);
}

void Protocol76::sendDistanceShoot(const Position &from, const Position &to, unsigned char type)
{
	NetworkMessage msg;
	AddDistanceShoot(msg,from, to,type );
	WriteBuffer(msg);
}

void Protocol76::sendMagicEffect(const Position &pos, unsigned char type)
{
	NetworkMessage msg;
	AddMagicEffect(msg, pos, type);
	WriteBuffer(msg);
}

void Protocol76::sendAnimatedText(const Position &pos, unsigned char color, std::string text)
{
	NetworkMessage msg;
	AddAnimatedText(msg, pos, color, text);
	WriteBuffer(msg);
}

void Protocol76::sendCreatureHealth(const Creature *creature)
{
	NetworkMessage msg;
	AddCreatureHealth(msg,creature);
	WriteBuffer(msg);
}

//tile
void Protocol76::sendAddTileItem(const Position& pos, const Item* item)
{
	if(CanSee(pos)){
		NetworkMessage msg;
		AddTileItem(msg, pos, item);
		WriteBuffer(msg);
	}
}

void Protocol76::sendUpdateTileItem(const Position& pos, uint32_t stackpos, const Item* item)
{
	if(CanSee(pos)){
		NetworkMessage msg;
		UpdateTileItem(msg, pos, stackpos, item);
		WriteBuffer(msg);
	}
}

void Protocol76::sendRemoveTileItem(const Position& pos, uint32_t stackpos)
{
	if(CanSee(pos)){
		NetworkMessage msg;
		RemoveTileItem(msg, pos, stackpos);
		WriteBuffer(msg);
	}
}

void Protocol76::UpdateTile(const Position& pos)
{
	if(CanSee(pos)){
		NetworkMessage msg;
		UpdateTile(msg, pos);
		WriteBuffer(msg);
	}
}

void Protocol76::sendAddCreature(const Creature* creature, bool isLogin)
{
	if(CanSee(creature->getPosition())){
		NetworkMessage msg;

		if(creature == player){
			msg.AddByte(0x0A);
			msg.AddU32(player->getID());

			msg.AddByte(0x32);
			msg.AddByte(0x00);

			msg.AddByte(0x00); //can report bugs 0,1

			//msg.AddByte(0x0B);//TODO?. GM actions
			//msg.AddByte(0xFF);msg.AddByte(0xFF);msg.AddByte(0xFF);msg.AddByte(0xFF);
			//msg.AddByte(0xFF);msg.AddByte(0xFF);msg.AddByte(0xFF);msg.AddByte(0xFF);
			//msg.AddByte(0xFF);msg.AddByte(0xFF);msg.AddByte(0xFF);msg.AddByte(0xFF);
			//msg.AddByte(0xFF);msg.AddByte(0xFF);msg.AddByte(0xFF);msg.AddByte(0xFF);
			//msg.AddByte(0xFF);msg.AddByte(0xFF);msg.AddByte(0xFF);msg.AddByte(0xFF);
			//msg.AddByte(0xFF);msg.AddByte(0xFF);msg.AddByte(0xFF);msg.AddByte(0xFF);
			//msg.AddByte(0xFF);msg.AddByte(0xFF);msg.AddByte(0xFF);msg.AddByte(0xFF);
			//msg.AddByte(0xFF);msg.AddByte(0xFF);msg.AddByte(0xFF);msg.AddByte(0xFF);

			AddMapDescription(msg, player->getPosition());

			if(isLogin){
				AddMagicEffect(msg, player->getPosition(), NM_ME_ENERGY_AREA);
			}
			
			AddInventoryItem(msg, SLOT_HEAD, player->getInventoryItem(SLOT_HEAD));
			AddInventoryItem(msg, SLOT_NECKLACE, player->getInventoryItem(SLOT_NECKLACE));
			AddInventoryItem(msg, SLOT_BACKPACK, player->getInventoryItem(SLOT_BACKPACK));
			AddInventoryItem(msg, SLOT_ARMOR, player->getInventoryItem(SLOT_ARMOR));
			AddInventoryItem(msg, SLOT_RIGHT, player->getInventoryItem(SLOT_RIGHT));
			AddInventoryItem(msg, SLOT_LEFT, player->getInventoryItem(SLOT_LEFT));
			AddInventoryItem(msg, SLOT_LEGS, player->getInventoryItem(SLOT_LEGS));
			AddInventoryItem(msg, SLOT_FEET, player->getInventoryItem(SLOT_FEET));
			AddInventoryItem(msg, SLOT_RING, player->getInventoryItem(SLOT_RING));
			AddInventoryItem(msg, SLOT_AMMO, player->getInventoryItem(SLOT_AMMO));

			AddPlayerStats(msg);
			AddPlayerSkills(msg);

			//gameworld light-settings
			LightInfo lightInfo;
			game->getWorldLightInfo(lightInfo);
			AddWorldLight(msg, lightInfo);

			//player light level
			AddCreatureLight(msg, creature);
			
			std::string tempstring = g_config.getGlobalString("loginmsg", "Welcome.").c_str();
			if(tempstring.size() > 0)
				AddTextMessage(msg,MSG_EVENT, tempstring.c_str());
			
			tempstring = "Your last visit was on ";
			time_t lastlogin = player->getLastLoginSaved();
			tempstring += ctime(&lastlogin);
			tempstring.erase(tempstring.length() -1);
			tempstring += ".";
			AddTextMessage(msg,MSG_EVENT, tempstring.c_str());
			WriteBuffer(msg);

			for(VIPListSet::iterator it = player->VIPList.begin(); it != player->VIPList.end(); it++){
				bool online;
				std::string vip_name;
				if(IOPlayer::instance()->getNameByGuid((*it), vip_name)){
					online = (game->getPlayerByName(vip_name) != NULL);
					sendVIP((*it), vip_name, online);
				}
			}

			//force flush
			flushOutputBuffer();
		}
		else{
			AddTileCreature(msg, creature->getPosition(), creature);

			if(isLogin){
				AddMagicEffect(msg, creature->getPosition(), NM_ME_ENERGY_AREA);
			}
			
			WriteBuffer(msg);
		}
	}
}

void Protocol76::sendRemoveCreature(const Creature* creature, const Position& pos, uint32_t stackpos, bool isLogout)
{
	if(CanSee(pos)){
		NetworkMessage msg;
		RemoveTileItem(msg, pos, stackpos); 

		if(isLogout){
			AddMagicEffect(msg, pos, NM_ME_PUFF);
		}
	
		WriteBuffer(msg);
	}
}

void Protocol76::sendMoveCreature(const Creature* creature, const Position& oldPos, uint32_t oldStackPos, bool teleport)
{
	const Position& newPos = creature->getPosition();

	if(creature == player){
		NetworkMessage msg;

		if(teleport){
			RemoveTileItem(msg, oldPos, oldStackPos);
			AddMapDescription(msg, player->getPosition());
		}
		else{
			if(oldPos.z == 7 && newPos.z >= 8){
				RemoveTileItem(msg, oldPos, oldStackPos);
			}
			else{
				if(oldStackPos < 10){
					msg.AddByte(0x6D);
					msg.AddPosition(oldPos);
					msg.AddByte(oldStackPos);
					msg.AddPosition(creature->getPosition());
				}
			}

			//floor change down
			if(newPos.z > oldPos.z){
				MoveDownCreature(msg, creature, newPos, oldPos, oldStackPos);
			}
			//floor change up
			else if(newPos.z < oldPos.z){
				MoveUpCreature(msg, creature, newPos, oldPos, oldStackPos);
			}

			if(oldPos.y > newPos.y){ // north, for old x
				msg.AddByte(0x65);
				GetMapDescription(oldPos.x - 8, newPos.y - 6, newPos.z, 18, 1, msg);
			}
			else if(oldPos.y < newPos.y){ // south, for old x
				msg.AddByte(0x67);
				GetMapDescription(oldPos.x - 8, newPos.y + 7, newPos.z, 18, 1, msg);
			}

			if(oldPos.x < newPos.x){ // east, [with new y]
				msg.AddByte(0x66);
				GetMapDescription(newPos.x + 9, newPos.y - 6, newPos.z, 1, 14, msg);
			}
			else if(oldPos.x > newPos.x){ // west, [with new y]
				msg.AddByte(0x68);
				GetMapDescription(newPos.x - 8, newPos.y - 6, newPos.z, 1, 14, msg);
			}
		}

		WriteBuffer(msg);
	}
	else if(CanSee(oldPos) && CanSee(creature->getPosition())){
		if(teleport || (oldPos.z == 7 && newPos.z >= 8)){
			sendRemoveCreature(creature, oldPos, oldStackPos, false);
			sendAddCreature(creature, false);
		}
		else{
			if(oldStackPos < 10){
				NetworkMessage msg;

				msg.AddByte(0x6D);
				msg.AddPosition(oldPos);
				msg.AddByte(oldStackPos);
				msg.AddPosition(creature->getPosition());
				WriteBuffer(msg);
			}
		}
	}
	else if(CanSee(oldPos)){
		sendRemoveCreature(creature, oldPos, oldStackPos, false);
	}
	else if(CanSee(creature->getPosition())){
		sendAddCreature(creature, false);
	}
}

//inventory
void Protocol76::sendAddInventoryItem(slots_t slot, const Item* item)
{
	NetworkMessage msg;
	AddInventoryItem(msg, slot, item);
	WriteBuffer(msg);
}

void Protocol76::sendUpdateInventoryItem(slots_t slot, const Item* item)
{
	NetworkMessage msg;
	UpdateInventoryItem(msg, slot, item);
	WriteBuffer(msg);
}

void Protocol76::sendRemoveInventoryItem(slots_t slot)
{
	NetworkMessage msg;
	RemoveInventoryItem(msg, slot);
	WriteBuffer(msg);
}

//containers
void Protocol76::sendAddContainerItem(uint8_t cid, const Item* item)
{
	NetworkMessage msg;
	AddContainerItem(msg, cid, item);
	WriteBuffer(msg);
}

void Protocol76::sendUpdateContainerItem(uint8_t cid, uint8_t slot, const Item* item)
{
	NetworkMessage msg;
	UpdateContainerItem(msg, cid, slot, item);
	WriteBuffer(msg);
}

void Protocol76::sendRemoveContainerItem(uint8_t cid, uint8_t slot)
{
	NetworkMessage msg;
	RemoveContainerItem(msg, cid, slot);
	WriteBuffer(msg);
}

void Protocol76::sendTextWindow(Item* item,const unsigned short maxlen, const bool canWrite)
{
	NetworkMessage msg;
	if(readItem){
		readItem->releaseThing2();
	}
	windowTextID++;
	msg.AddByte(0x96);
	msg.AddU32(windowTextID);
	msg.AddItemId(item);
	if(canWrite){
		msg.AddU16(maxlen);
		msg.AddString(item->getText());		
		item->useThing2();
		readItem = item;
		maxTextLenght = maxlen;
	}
	else{		
		msg.AddU16(item->getText().size());
		msg.AddString(item->getText());									
		readItem = NULL;
		maxTextLenght = 0;
	}
	msg.AddString("unknown");
	WriteBuffer(msg);
}

void Protocol76::sendHouseWindow(House* _house, unsigned long _listid, const std::string& text)
{
	NetworkMessage msg;
	windowTextID++;
	house = _house;
	listId = _listid;
	msg.AddByte(0x97);
	msg.AddByte(0);
	msg.AddU32(windowTextID);
	msg.AddString(text);
	WriteBuffer(msg);
}

void Protocol76::sendVIPLogIn(unsigned long guid)
{
	NetworkMessage msg;
	msg.AddByte(0xD3);
	msg.AddU32(guid);
	WriteBuffer(msg);
}

void Protocol76::sendVIPLogOut(unsigned long guid)
{
	NetworkMessage msg;
	msg.AddByte(0xD4);
	msg.AddU32(guid);
	WriteBuffer(msg);
}

void Protocol76::sendVIP(unsigned long guid, const std::string &name, bool isOnline)
{
	NetworkMessage msg;
	msg.AddByte(0xD2);
	msg.AddU32(guid);
	msg.AddString(name);
	msg.AddByte(isOnline == true ? 1 : 0);
	WriteBuffer(msg);
}

////////////// Add common messages
void Protocol76::AddMapDescription(NetworkMessage& msg, const Position& pos)
{
	msg.AddByte(0x64); 
	msg.AddPosition(player->getPosition()); 
	GetMapDescription(pos.x - 8, pos.y - 6, pos.z, 18, 14, msg);
}

void Protocol76::AddTextMessage(NetworkMessage &msg,MessageClasses mclass, const char* message)
{
	msg.AddByte(0xB4);
	msg.AddByte(mclass);
	msg.AddString(message);
}

void Protocol76::AddAnimatedText(NetworkMessage &msg,const Position &pos, unsigned char color, std::string text)
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

void Protocol76::AddMagicEffect(NetworkMessage &msg,const Position &pos, unsigned char type)
{
	msg.AddByte(0x83);
	msg.AddPosition(pos);
	msg.AddByte(type + 1);
}


void Protocol76::AddDistanceShoot(NetworkMessage &msg,const Position &from, const Position &to, unsigned char type)
{
	msg.AddByte(0x85); 
	msg.AddPosition(from);
	msg.AddPosition(to);
	msg.AddByte(type + 1);
}

void Protocol76::AddCreature(NetworkMessage &msg,const Creature *creature, bool known, unsigned int remove)
{
	if(known){
		msg.AddU16(0x62);
		msg.AddU32(creature->getID());
	}
	else{
		msg.AddU16(0x61);
		msg.AddU32(remove);
		msg.AddU32(creature->getID());
		msg.AddString(creature->getName());
	}
	
	msg.AddByte(std::max(1, creature->health*100/std::max(creature->healthmax,1)));
	
	msg.AddByte((unsigned char)creature->getDirection());
	
	msg.AddByte(creature->looktype);
	msg.AddByte(creature->lookhead);
	msg.AddByte(creature->lookbody);
	msg.AddByte(creature->looklegs);
	msg.AddByte(creature->lookfeet);
	
	LightInfo lightInfo;
	creature->getCreatureLight(lightInfo);
	msg.AddByte(lightInfo.level);
	msg.AddByte(lightInfo.color);
	
	msg.AddU16(creature->getSpeed());
	
	#ifdef __SKULLSYSTEM__
	if(const Player* playerSkull = creature->getPlayer()){
		msg.AddByte(player->getSkullClient(playerSkull));
	}
	else{
		msg.AddByte(0);	//no skull
	}
	#else
	msg.AddByte(0);	//no skull
	#endif
	msg.AddByte(0x00); // shield
}


void Protocol76::AddPlayerStats(NetworkMessage &msg)
{
	msg.AddByte(0xA0);
	msg.AddU16(player->getHealth());
	msg.AddU16(player->getPlayerInfo(PLAYERINFO_MAXHEALTH));
	msg.AddU16((unsigned short)std::floor(player->getFreeCapacity()));
	msg.AddU32(player->getExperience());
	msg.AddU16(player->getPlayerInfo(PLAYERINFO_LEVEL));
	msg.AddByte(player->getPlayerInfo(PLAYERINFO_LEVELPERCENT));
	msg.AddU16(player->getMana());
	msg.AddU16(player->getPlayerInfo(PLAYERINFO_MAXMANA));
	msg.AddByte(player->getMagicLevel());
	msg.AddByte(player->getPlayerInfo(PLAYERINFO_MAGICLEVELPERCENT));
	msg.AddByte(player->getPlayerInfo(PLAYERINFO_SOUL));
}

void Protocol76::AddPlayerSkills(NetworkMessage& msg)
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

void Protocol76::AddCreatureSpeak(NetworkMessage &msg,const Creature *creature, SpeakClasses  type, std::string text, unsigned short channelId)
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
		msg.AddPosition(creature->getPosition());
		break;	
	case SPEAK_CHANNEL_Y:
	case SPEAK_CHANNEL_R1:
	case SPEAK_CHANNEL_R2:
		msg.AddU16(channelId);
		break;	
	}
	msg.AddString(text);
}

void Protocol76::AddCreatureHealth(NetworkMessage &msg,const Creature *creature)
{
	msg.AddByte(0x8C);
	msg.AddU32(creature->getID());
	msg.AddByte(std::max(1, creature->health*100/std::max(creature->healthmax,1)));
}

void Protocol76::AddWorldLight(NetworkMessage &msg, const LightInfo& lightInfo)
{
	msg.AddByte(0x82);
	msg.AddByte(lightInfo.level);
	msg.AddByte(lightInfo.color);
}

void Protocol76::AddCreatureLight(NetworkMessage &msg, const Creature* creature)
{
	LightInfo lightInfo;
	creature->getCreatureLight(lightInfo);
	msg.AddByte(0x8D);
	msg.AddU32(creature->getID());
	msg.AddByte(lightInfo.level);
	msg.AddByte(lightInfo.color);
}

//tile
void Protocol76::AddTileItem(NetworkMessage& msg, const Position& pos, const Item* item)
{
	msg.AddByte(0x6A);
	msg.AddPosition(pos);	
	msg.AddItem(item);
}

void Protocol76::AddTileCreature(NetworkMessage& msg, const Position& pos, const Creature* creature)
{
	msg.AddByte(0x6A);
	msg.AddPosition(pos);	

	bool known;
	unsigned long removedKnown;
	checkCreatureAsKnown(creature->getID(), known, removedKnown);
	AddCreature(msg, creature, known, removedKnown);
}

void Protocol76::UpdateTileItem(NetworkMessage& msg, const Position& pos, uint32_t stackpos, const Item* item)
{
	if(stackpos < 10){
		msg.AddByte(0x6B);
		msg.AddPosition(pos);
		msg.AddByte(stackpos);
		msg.AddItem(item);
	}
}

void Protocol76::RemoveTileItem(NetworkMessage& msg, const Position& pos, uint32_t stackpos)
{
	if(stackpos < 10){
		msg.AddByte(0x6C);
		msg.AddPosition(pos);
		msg.AddByte(stackpos);
	}
}

void Protocol76::UpdateTile(NetworkMessage& msg, const Position& pos)
{
	msg.AddByte(0x69);
	msg.AddPosition(pos);
		
	Tile* tile = game->getTile(pos.x, pos.y, pos.z);
	if(tile){
		GetTileDescription(tile, msg);
		msg.AddByte(0);
		msg.AddByte(0xFF);
	}
	else{
		msg.AddByte(0x01);
		msg.AddByte(0xFF);
	}
}

void Protocol76::MoveUpCreature(NetworkMessage& msg, const Creature* creature,
	const Position& newPos, const Position& oldPos, uint32_t oldStackPos)
{
	if(creature == player){
		//floor change up
		msg.AddByte(0xBE);

		//going to surface
		if(newPos.z == 7){
			int skip = -1;
			GetFloorDescription(msg, oldPos.x - 8, oldPos.y - 6, 5, 18, 14, 3, skip); //(floor 7 and 6 already set)
			GetFloorDescription(msg, oldPos.x - 8, oldPos.y - 6, 4, 18, 14, 4, skip);
			GetFloorDescription(msg, oldPos.x - 8, oldPos.y - 6, 3, 18, 14, 5, skip);
			GetFloorDescription(msg, oldPos.x - 8, oldPos.y - 6, 2, 18, 14, 6, skip);
			GetFloorDescription(msg, oldPos.x - 8, oldPos.y - 6, 1, 18, 14, 7, skip);
			GetFloorDescription(msg, oldPos.x - 8, oldPos.y - 6, 0, 18, 14, 8, skip);

			if(skip >= 0){
				msg.AddByte(skip);
				msg.AddByte(0xFF);
			}
		}
		//underground, going one floor up (still underground)
		else if(newPos.z > 7){
			int skip = -1;
			GetFloorDescription(msg, oldPos.x - 8, oldPos.y - 6, oldPos.z - 3, 18, 14, 3, skip);

			if(skip >= 0){
				msg.AddByte(skip);
				msg.AddByte(0xFF);
			}
		}

		//moving up a floor up makes us out of sync
		//west
		msg.AddByte(0x68);
		GetMapDescription(oldPos.x - 8, oldPos.y + 1 - 6, newPos.z, 1, 14, msg);

		//north
		msg.AddByte(0x65);
		GetMapDescription(oldPos.x - 8, oldPos.y - 6, newPos.z, 18, 1, msg);
	}
}

void Protocol76::MoveDownCreature(NetworkMessage& msg, const Creature* creature,
	const Position& newPos, const Position& oldPos, uint32_t oldStackPos)
{
	if(creature == player){
		//floor change down
		msg.AddByte(0xBF);

		//going from surface to underground
		if(newPos.z == 8){
			int skip = -1;

			GetFloorDescription(msg, oldPos.x - 8, oldPos.y - 6, newPos.z, 18, 14, -1, skip);
			GetFloorDescription(msg, oldPos.x - 8, oldPos.y - 6, newPos.z + 1, 18, 14, -2, skip);
			GetFloorDescription(msg, oldPos.x - 8, oldPos.y - 6, newPos.z + 2, 18, 14, -3, skip);

			if(skip >= 0){
				msg.AddByte(skip);
				msg.AddByte(0xFF);
			}
		}
		//going further down
		else if(newPos.z > oldPos.z && newPos.z > 8 && newPos.z < 14){
			int skip = -1;
			GetFloorDescription(msg, oldPos.x - 8, oldPos.y - 6, newPos.z + 2, 18, 14, -3, skip);

			if(skip >= 0){
				msg.AddByte(skip);
				msg.AddByte(0xFF);
			}
		}

		//moving down a floor makes us out of sync
		//east
		msg.AddByte(0x66);
		GetMapDescription(oldPos.x + 9, oldPos.y - 1 - 6, newPos.z, 1, 14, msg);

		//south
		msg.AddByte(0x67);
		GetMapDescription(oldPos.x - 8, oldPos.y + 7, newPos.z, 18, 1, msg);
	}
}


//inventory
void Protocol76::AddInventoryItem(NetworkMessage& msg, slots_t slot, const Item* item)
{
	if(item == NULL){
		msg.AddByte(0x79);
		msg.AddByte(slot);
	}
	else{
		msg.AddByte(0x78);
		msg.AddByte(slot);
		msg.AddItem(item);
	}
}

void Protocol76::UpdateInventoryItem(NetworkMessage& msg, slots_t slot, const Item* item)
{
	if(item == NULL){
		msg.AddByte(0x79);
		msg.AddByte(slot);
	}
	else{
		msg.AddByte(0x78);
		msg.AddByte(slot);
		msg.AddItem(item);
	}
}

void Protocol76::RemoveInventoryItem(NetworkMessage& msg, slots_t slot)
{
	msg.AddByte(0x79);
	msg.AddByte(slot);
}

//containers
void Protocol76::AddContainerItem(NetworkMessage& msg, uint8_t cid, const Item *item)
{
	msg.AddByte(0x70);
	msg.AddByte(cid);
	msg.AddItem(item);
}

void Protocol76::UpdateContainerItem(NetworkMessage& msg, uint8_t cid, uint8_t slot, const Item* item)
{
	msg.AddByte(0x71);
	msg.AddByte(cid);
	msg.AddByte(slot);
	msg.AddItem(item);
}

void Protocol76::RemoveContainerItem(NetworkMessage& msg, uint8_t cid, uint8_t slot)
{
	msg.AddByte(0x72);
	msg.AddByte(cid);
	msg.AddByte(slot);
}

//////////////////////////

void Protocol76::flushOutputBuffer()
{
	OTSYS_THREAD_LOCK_CLASS lockClass(bufferLock, "Protocol76::flushOutputBuffer()");
	//force writetosocket	
	OutputBuffer.WriteToSocket(s);
	OutputBuffer.Reset();
	
	return;
}

void Protocol76::WriteBuffer(NetworkMessage& add)
{
	game->addPlayerBuffer(player);	
	
	OTSYS_THREAD_LOCK(bufferLock, "Protocol76::WriteBuffer")
	
	if(OutputBuffer.getMessageLength() + add.getMessageLength() >= NETWORKMESSAGE_MAXSIZE - 16){
		this->flushOutputBuffer();
	}
	
	OutputBuffer.JoinMessages(add);	
	OTSYS_THREAD_UNLOCK(bufferLock, "Protocol76::WriteBuffer")	
	return;
}

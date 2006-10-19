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
extern Game g_game;
Chat g_chat;


Protocol76::Protocol76(SOCKET s)
{
	OTSYS_THREAD_LOCKVARINIT(bufferLock);
	
	player = NULL;
	game = NULL;
	pendingLogout = false;
	
	windowTextID = 0;
	readItem = NULL;
	maxTextLength = 0;
	this->s = s;
}


Protocol76::~Protocol76()
{
	OTSYS_THREAD_LOCKVARRELEASE(bufferLock);
	if(s){
		closesocket(s);
		s = 0;
	}
	player = NULL;
	game = NULL;
}

unsigned long Protocol76::getIP() const
{
	sockaddr_in sain;
	socklen_t salen = sizeof(sockaddr_in);
	if (getpeername(s, (sockaddr*)&sain, &salen) == 0)
	{
#if defined WIN32 || defined __WINDOWS__
		return sain.sin_addr.S_un.S_addr;
#else
		return sain.sin_addr.s_addr;
#endif
	}
	
	return 0;
}

void Protocol76::setPlayer(Player* p)
{
	player = p;
	game   = &g_game;
}

void Protocol76::sleepTillMove()
{
	long long delay = player->getSleepTicks();
	if(delay > 0 ){       
#if __DEBUG__     
		std::cout << "Delaying "<< player->getName() << " --- " << delay << std::endl;		
#endif	
		OTSYS_SLEEP((uint32_t)delay);
	}
}


void Protocol76::reinitializeProtocol()
{
	windowTextID = 0;
	readItem = NULL;
	maxTextLength = 0;
	OutputBuffer.Reset();
	knownPlayers.clear();
}

connectResult_t Protocol76::ConnectPlayer()
{
	Waitlist* wait = Waitlist::instance();

	if(player->getAccessLevel() == 0 && !wait->clientLogin(player->getAccount(), player->getIP())){
		return CONNECT_TOMANYPLAYERS;
	}
	else{
		//last login position
		if(game->placeCreature(player->getLoginPosition(), player)){
			return CONNECT_SUCCESS;
		}
		//temple
		else if(game->placeCreature(player->getTemplePosition(), player, true)){
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

			while(player->hasCondition(CONDITION_INFIGHT) && !player->isRemoved() && s == 0){
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
	if((player->isRemoved() || player->getHealth() <= 0) && recvbyte != 0x14){
		OTSYS_SLEEP(10);
		return;
	}

	switch(recvbyte){
	case 0x14: // logout
		parseLogout(msg);
		break;

	case 0x1E: // keep alive / ping response
		parseRecievePing(msg);
		break;

	case 0x64: // move with steps
		parseAutoWalk(msg);
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

	case 0x69: // stop-autowalk
		parseStopAutoWalk(msg);
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
		parseFightModes(msg);
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

	case 0xC9: //client sends its position, unknown usage
		msg.GetPosition();
		break;

	case 0xCA: //client request to resend the container (happens when you store more than container maxsize)
		parseUpdateContainer(msg);
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
			if ((!c) || (!canSee(c)))
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

bool Protocol76::canSee(const Creature* c) const
{
	if(c->isRemoved())
		return false;

	return canSee(c->getPosition());
}

bool Protocol76::canSee(const Position& pos) const
{
	return canSee(pos.x, pos.y, pos.z);
}

bool Protocol76::canSee(int x, int y, int z) const
{
#ifdef __DEBUG__
	if(z < 0 || z >= MAP_MAX_LAYERS) {
		std::cout << "WARNING! Protocol76::canSee() Z-value is out of range!" << std::endl;
	}
#endif

	const Position& myPos = player->getPosition();

	if(myPos.z <= 7){
		//we are on ground level or above (7 -> 0)
		//view is from 7 -> 0
		if(z > 7){
			return false;
		}
	}
	else if(myPos.z >= 8){
		//we are underground (8 -> 15)
		//view is +/- 2 from the floor we stand on
		if(std::abs(myPos.z - z) > 2){
			return false;
		}
	}

	//negative offset means that the action taken place is on a lower floor than ourself
	int offsetz = myPos.z - z;

	if ((x >= myPos.x - 8 + offsetz) && (x <= myPos.x + 9 + offsetz) &&
		(y >= myPos.y - 6 + offsetz) && (y <= myPos.y + 7 + offsetz))
		return true;

	return false;
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
	if(player->hasCondition(CONDITION_INFIGHT) && !player->isRemoved()){
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
	if(player->isRemoved()){
		return;
	}

	if(g_chat.addUserToChannel(player, channelId)){
		sendChannel(channelId, g_chat.getChannelName(player, channelId));
	}
}

void Protocol76::parseCloseChannel(NetworkMessage &msg)
{
	unsigned short channelId = msg.GetU16();
	OTSYS_THREAD_LOCK_CLASS lockClass(game->gameLock, "Protocol76::parseCloseChannel()");
	if(player->isRemoved()){
		return;
	}

	g_chat.removeUserFromChannel(player, channelId);
}

void Protocol76::parseOpenPriv(NetworkMessage& msg)
{
	std::string receiver;
	receiver = msg.GetString();
	OTSYS_THREAD_LOCK_CLASS lockClass(game->gameLock, "Protocol76::parseOpenPriv()");
	if(player->isRemoved()){
		return;
	}

	Player* playerPriv = game->getPlayerByName(receiver);
	if(playerPriv){
		sendOpenPriv(playerPriv->getName());
	}
}

void Protocol76::parseCancelMove(NetworkMessage& msg)
{
	game->playerSetAttackedCreature(player, 0);
	game->playerFollowCreature(player, 0);
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


void Protocol76::parseRecievePing(NetworkMessage& msg)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(game->gameLock, "Protocol76::parseRecievePing()");
	if(player->isRemoved()){
		return;
	}

	player->receivePing();
}

void Protocol76::parseAutoWalk(NetworkMessage& msg)
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

void Protocol76::parseStopAutoWalk(NetworkMessage& msg)
{
	game->playerStopAutoWalk(player);
}

void Protocol76::parseMoveNorth(NetworkMessage& msg)
{
	sleepTillMove();

	game->movePlayer(player, NORTH);
}

void Protocol76::parseMoveEast(NetworkMessage& msg)
{
	sleepTillMove();

	game->movePlayer(player, EAST);
}

void Protocol76::parseMoveSouth(NetworkMessage& msg)
{
	sleepTillMove();

	game->movePlayer(player, SOUTH);
}

void Protocol76::parseMoveWest(NetworkMessage& msg)
{
	sleepTillMove();

	game->movePlayer(player, WEST);
}

void Protocol76::parseMoveNorthEast(NetworkMessage& msg)
{
	sleepTillMove();
	sleepTillMove();

	game->movePlayer(player, NORTHEAST);
}

void Protocol76::parseMoveSouthEast(NetworkMessage& msg)
{
	sleepTillMove();
	sleepTillMove();

	game->movePlayer(player, SOUTHEAST);
}

void Protocol76::parseMoveSouthWest(NetworkMessage& msg)
{
	sleepTillMove();
	sleepTillMove();

	game->movePlayer(player, SOUTHWEST);
}

void Protocol76::parseMoveNorthWest(NetworkMessage& msg)
{
	sleepTillMove();
	sleepTillMove();

	game->movePlayer(player, NORTHWEST);
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
	AddCreatureOutfit(msg, player, player->getDefaultOutfit());

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
	uint8_t lookType = msg.GetByte();
	
	if ( (player->getSex() == PLAYERSEX_FEMALE && lookType >= PLAYER_FEMALE_1 && lookType <= PLAYER_FEMALE_7) ||
		(player->getSex() == PLAYERSEX_MALE && lookType >= PLAYER_MALE_1 && lookType <= PLAYER_MALE_7))
	{
		Outfit_t newOutfit;
		newOutfit.lookType = lookType;
		newOutfit.lookHead = msg.GetByte();
		newOutfit.lookBody = msg.GetByte();
		newOutfit.lookLegs = msg.GetByte();
		newOutfit.lookFeet = msg.GetByte();

		game->playerChangeOutfit(player, newOutfit);
	}
}

void Protocol76::parseUseItem(NetworkMessage& msg)
{
	Position pos = msg.GetPosition();
	uint16_t spriteId = msg.GetSpriteId();
	uint8_t stackpos = msg.GetByte();
	uint8_t index = msg.GetByte();
/*
#ifdef __DEBUG__
	std::cout << "parseUseItem: " << "x: " << pos.x << ", y: " << (int)pos.y <<  ", z: " << (int)pos.z << ", item: " << (int)itemId << ", stack: " << (int)stackpos << ", index: " << (int)index << std::endl;
#endif
*/
	game->playerUseItem(player, pos, stackpos, index, spriteId);
}

void Protocol76::parseUseItemEx(NetworkMessage& msg)
{
	Position fromPos = msg.GetPosition();
	uint16_t fromSpriteId = msg.GetSpriteId();
	uint8_t fromStackpos = msg.GetByte();
	Position toPos = msg.GetPosition();
	uint16_t toSpriteId = msg.GetU16();
	uint8_t toStackpos = msg.GetByte();

	game->playerUseItemEx(player, fromPos, fromStackpos, fromSpriteId, toPos, toStackpos, toSpriteId);
}

void Protocol76::parseBattleWindow(NetworkMessage &msg)
{
	Position fromPos = msg.GetPosition();
	uint16_t spriteId = msg.GetSpriteId();
	uint8_t fromStackPos = msg.GetByte();
	uint32_t creatureId = msg.GetU32();

	game->playerUseBattleWindow(player, fromPos, fromStackPos, creatureId, spriteId);
}

void Protocol76::parseCloseContainer(NetworkMessage& msg)
{
	unsigned char containerid = msg.GetByte();

	OTSYS_THREAD_LOCK_CLASS lockClass(game->gameLock, "Protocol76::parseCloseContainer()");
	if(player->isRemoved()){
		return;
	}

	player->closeContainer(containerid);
	sendCloseContainer(containerid);
}

void Protocol76::parseUpArrowContainer(NetworkMessage& msg)
{
	uint32_t cid = msg.GetByte();
	OTSYS_THREAD_LOCK_CLASS lockClass(game->gameLock, "Protocol76::parseUpArrowContainer()");
	if(player->isRemoved()){
		return;
	}

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

void Protocol76::parseUpdateContainer(NetworkMessage& msg)
{
	uint32_t cid = msg.GetByte();
	OTSYS_THREAD_LOCK_CLASS lockClass(game->gameLock, "Protocol76::parseUpdateContainer()");
	if(player->isRemoved()){
		return;
	}

	Container* container = player->getContainer(cid);
	if(!container)
		return;

	bool hasParent = (dynamic_cast<const Container*>(container->getParent()) != NULL);
	sendContainer(cid, container, hasParent);
}

void Protocol76::parseThrow(NetworkMessage& msg)
{
	Position fromPos = msg.GetPosition();
	uint16_t spriteId = msg.GetSpriteId();
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

	game->thingMove(player, fromPos, spriteId, fromStackpos, toPos, count);
}

void Protocol76::parseLookAt(NetworkMessage& msg)
{
	Position pos = msg.GetPosition();
	uint16_t spriteId = msg.GetSpriteId();
	uint8_t stackpos = msg.GetByte();

/*
#ifdef __DEBUG__
	ss << "You look at x: " << x <<", y: " << y << ", z: " << z << " and see Item # " << itemId << ".";
#endif
*/

	game->playerLookAt(player, pos, spriteId, stackpos);
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

	if(game->playerSaySpell(player, type, text)){
		return;
	}

	switch(type){
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
		default:
			break;
	}
}

void Protocol76::parseFightModes(NetworkMessage& msg)
{
	uint8_t fightMode = msg.GetByte(); //1 - offensive, 2 - balanced, 3 - defensive
	uint8_t chaseMode = msg.GetByte(); // 0 - stand while fightning, 1 - chase opponent

	game->playerSetFightModes(player, fightMode, chaseMode);
}

void Protocol76::parseAttack(NetworkMessage& msg)
{
	unsigned long creatureid = msg.GetU32();
	game->playerSetAttackedCreature(player, creatureid);
}

void Protocol76::parseFollow(NetworkMessage& msg)
{
	unsigned long creatureId = msg.GetU32();
	game->playerFollowCreature(player, creatureId);
}

void Protocol76::parseTextWindow(NetworkMessage& msg)
{
	unsigned long id = msg.GetU32();
	std::string new_text = msg.GetString();
	if(new_text.length() > maxTextLength)
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

	OTSYS_THREAD_LOCK_CLASS lockClass(game->gameLock, "Protocol76::parseHouseWindow()");
	if(player->isRemoved()){
		return;
	}

	if(house && windowTextID == id && _listid == 0){
		house->setAccessList(listId, new_list);
		house = NULL;
		listId = 0;
	}
}

void Protocol76::parseRequestTrade(NetworkMessage& msg)
{
	Position pos = msg.GetPosition();
	uint16_t spriteId = msg.GetSpriteId();
	uint8_t stackpos = msg.GetByte();
	uint32_t playerId = msg.GetU32();

	game->playerRequestTrade(player, pos, stackpos, playerId, spriteId);
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
	uint16_t spriteId = msg.GetSpriteId();
	uint8_t stackpos = msg.GetByte();

	game->playerRotateItem(player, pos, stackpos, spriteId);
}

// Send methods
void Protocol76::sendOpenPriv(const std::string& receiver)
{
	NetworkMessage newmsg;
	newmsg.AddByte(0xAD);
	newmsg.AddString(receiver);
	WriteBuffer(newmsg);
}

void Protocol76::sendCreatureOutfit(const Creature* creature, const Outfit_t& outfit)
{
	if(canSee(creature)){
		NetworkMessage msg;
		msg.AddByte(0x8E);
		msg.AddU32(creature->getID());
		AddCreatureOutfit(msg, creature, outfit);
		WriteBuffer(msg);
	}
}

void Protocol76::sendCreatureInvisible(const Creature* creature)
{
	if(canSee(creature)){
		NetworkMessage msg;
		msg.AddByte(0x8E);
		msg.AddU32(creature->getID());
		AddCreatureInvisible(msg, creature);
		WriteBuffer(msg);
	}
}

void Protocol76::sendCreatureLight(const Creature* creature)
{
	if(canSee(creature)){
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

void Protocol76::sendCreatureSkull(const Creature* creature, Skulls_t skull)
{
	if(canSee(creature)){
		NetworkMessage msg;
		msg.AddByte(0x90);
		msg.AddU32(creature->getID());
		msg.AddByte(skull);
		WriteBuffer(msg);
	}
}

void Protocol76::sendCreatureShield(const Creature* creature)
{
	if(canSee(creature)){
		NetworkMessage msg;
		msg.AddByte(0x91);
		msg.AddU32(creature->getID());
		msg.AddByte(0);	//no shield
		WriteBuffer(msg);
	}
}

void Protocol76::sendCreatureSquare(const Creature* creature, SquareColor_t color)
{
	if(canSee(creature)){
		NetworkMessage msg;
		msg.AddByte(0x86);
		msg.AddU32(creature->getID());
		msg.AddByte((unsigned char)color);
		WriteBuffer(msg);
	}
}

void Protocol76::sendStats()
{
	NetworkMessage msg;
	AddPlayerStats(msg);
	WriteBuffer(msg);
}

void Protocol76::sendTextMessage(MessageClasses mclass, const std::string& message)
{
	NetworkMessage msg;
	AddTextMessage(msg,mclass, message);
	WriteBuffer(msg);
}

void Protocol76::sendTextMessage(MessageClasses mclass, const std::string& message,
	const Position& pos, unsigned char type)
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
	if(ack){
		msg.AddByte(0x7D);
	}
	else{
		msg.AddByte(0x7E);
	}

	msg.AddString(player->getName());

	if(const Container* tradeContainer = item->getContainer()){

		std::list<const Container*> listContainer;
		ItemList::const_iterator it;
		Container* tmpContainer = NULL;

		listContainer.push_back(tradeContainer);

		std::list<const Item*> listItem;
		listItem.push_back(tradeContainer);

		while(listContainer.size() > 0) {
			const Container* container = listContainer.front();
			listContainer.pop_front();

			for(it = container->getItems(); it != container->getEnd(); ++it){
				if(tmpContainer = (*it)->getContainer()){
					listContainer.push_back(tmpContainer);
				}

				listItem.push_back(*it);
			}
		}

		msg.AddByte(listItem.size());
		while(listItem.size() > 0) {
			const Item* item = listItem.front();
			listItem.pop_front();
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
	if(canSee(creature)){
		NetworkMessage msg;

		msg.AddByte(0x6B);
		msg.AddPosition(creature->getPosition());
		msg.AddByte(stackPos);

		msg.AddU16(0x63); /*99*/
		msg.AddU32(creature->getID());
		msg.AddByte(creature->getDirection());
		WriteBuffer(msg);
	}
}

void Protocol76::sendCreatureSay(const Creature* creature, SpeakClasses type, const std::string& text)
{
	NetworkMessage msg;
	AddCreatureSpeak(msg,creature, type, text, 0);
	WriteBuffer(msg);
}

void Protocol76::sendToChannel(const Creature * creature, SpeakClasses type, const std::string& text, unsigned short channelId){
	NetworkMessage msg;
	AddCreatureSpeak(msg,creature, type, text, channelId);
	WriteBuffer(msg);
}

void Protocol76::sendCancel(const std::string& message)
{
	NetworkMessage netmsg;
	AddTextMessage(netmsg, MSG_STATUS_SMALL, message);
	WriteBuffer(netmsg);
}

void Protocol76::sendCancelTarget()
{
	NetworkMessage netmsg;
	netmsg.AddByte(0xa3);
	WriteBuffer(netmsg);
}

void Protocol76::sendChangeSpeed(const Creature* creature, uint32_t speed)
{
	NetworkMessage netmsg;
	netmsg.AddByte(0x8F);

	netmsg.AddU32(creature->getID());
	netmsg.AddU16(speed);
	WriteBuffer(netmsg);
}

void Protocol76::sendCancelWalk()
{
	NetworkMessage netmsg;
	netmsg.AddByte(0xB5);
	netmsg.AddByte(player->getDirection());
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

void Protocol76::sendDistanceShoot(const Position& from, const Position& to, unsigned char type)
{
	NetworkMessage msg;
	AddDistanceShoot(msg,from, to,type);
	WriteBuffer(msg);
}

void Protocol76::sendMagicEffect(const Position& pos, unsigned char type)
{
	NetworkMessage msg;
	AddMagicEffect(msg, pos, type);
	WriteBuffer(msg);
}

void Protocol76::sendAnimatedText(const Position& pos, unsigned char color, std::string text)
{
	NetworkMessage msg;
	AddAnimatedText(msg, pos, color, text);
	WriteBuffer(msg);
}

void Protocol76::sendCreatureHealth(const Creature* creature)
{
	NetworkMessage msg;
	AddCreatureHealth(msg,creature);
	WriteBuffer(msg);
}

//tile
void Protocol76::sendAddTileItem(const Position& pos, const Item* item)
{
	if(canSee(pos)){
		NetworkMessage msg;
		AddTileItem(msg, pos, item);
		WriteBuffer(msg);
	}
}

void Protocol76::sendUpdateTileItem(const Position& pos, uint32_t stackpos, const Item* item)
{
	if(canSee(pos)){
		NetworkMessage msg;
		UpdateTileItem(msg, pos, stackpos, item);
		WriteBuffer(msg);
	}
}

void Protocol76::sendRemoveTileItem(const Position& pos, uint32_t stackpos)
{
	if(canSee(pos)){
		NetworkMessage msg;
		RemoveTileItem(msg, pos, stackpos);
		WriteBuffer(msg);
	}
}

void Protocol76::UpdateTile(const Position& pos)
{
	if(canSee(pos)){
		NetworkMessage msg;
		UpdateTile(msg, pos);
		WriteBuffer(msg);
	}
}

void Protocol76::sendAddCreature(const Creature* creature, bool isLogin)
{
	if(canSee(creature->getPosition())){
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

			std::string tempstring = g_config.getGlobalString("loginmsg", "Welcome.");
			if(tempstring.size() > 0){
				AddTextMessage(msg, MSG_STATUS_DEFAULT, tempstring);
			}

			tempstring = "Your last visit was on ";
			time_t lastlogin = player->getLastLoginSaved();
			tempstring += ctime(&lastlogin);
			tempstring.erase(tempstring.length() -1);
			tempstring += ".";
			AddTextMessage(msg, MSG_STATUS_DEFAULT, tempstring);
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
	if(canSee(pos)){
		NetworkMessage msg;
		RemoveTileItem(msg, pos, stackpos);

		if(isLogout){
			AddMagicEffect(msg, pos, NM_ME_PUFF);
		}

		WriteBuffer(msg);
	}
}

void Protocol76::sendMoveCreature(const Creature* creature, const Position& newPos, const Position& oldPos,
	uint32_t oldStackPos, bool teleport)
{
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
	else if(canSee(oldPos) && canSee(creature->getPosition())){
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
	else if(canSee(oldPos)){
		sendRemoveCreature(creature, oldPos, oldStackPos, false);
	}
	else if(canSee(creature->getPosition())){
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
		maxTextLength = maxlen;
	}
	else{
		msg.AddU16(item->getText().size());
		msg.AddString(item->getText());
		readItem = NULL;
		maxTextLength = 0;
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

void Protocol76::sendVIP(unsigned long guid, const std::string& name, bool isOnline)
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

void Protocol76::AddTextMessage(NetworkMessage& msg, MessageClasses mclass, const std::string& message)
{
	msg.AddByte(0xB4);
	msg.AddByte(mclass);
	msg.AddString(message);
}

void Protocol76::AddAnimatedText(NetworkMessage& msg, const Position& pos,
	unsigned char color, const std::string& text)
{
	msg.AddByte(0x84);
	msg.AddPosition(pos);
	msg.AddByte(color);
	msg.AddString(text);
}

void Protocol76::AddMagicEffect(NetworkMessage& msg,const Position& pos, unsigned char type)
{
	msg.AddByte(0x83);
	msg.AddPosition(pos);
	msg.AddByte(type + 1);
}


void Protocol76::AddDistanceShoot(NetworkMessage& msg, const Position& from, const Position& to,
	unsigned char type)
{
	msg.AddByte(0x85);
	msg.AddPosition(from);
	msg.AddPosition(to);
	msg.AddByte(type + 1);
}

void Protocol76::AddCreature(NetworkMessage &msg,const Creature* creature, bool known, unsigned int remove)
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

	msg.AddByte(std::max((int32_t)1, creature->getHealth() * 100 / std::max(creature->getMaxHealth(), (int32_t)1) ));

	msg.AddByte((unsigned char)creature->getDirection());

	if(!creature->isInvisible()){
		AddCreatureOutfit(msg, creature, creature->getCurrentOutfit());
	}
	else{
		AddCreatureInvisible(msg, creature);
	}

	LightInfo lightInfo;
	creature->getCreatureLight(lightInfo);
	msg.AddByte(lightInfo.level);
	msg.AddByte(lightInfo.color);

	msg.AddU16(creature->getSpeed());
	#ifdef __SKULLSYSTEM__
	msg.AddByte(player->getSkullClient(creature->getPlayer()));
	#else
	msg.AddByte(SKULL_NONE);
	#endif
	msg.AddByte(0x00); // shield
}


void Protocol76::AddPlayerStats(NetworkMessage& msg)
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

void Protocol76::AddCreatureSpeak(NetworkMessage& msg,const Creature* creature, SpeakClasses  type, std::string text, unsigned short channelId)
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
	default:
		break;
	}
	msg.AddString(text);
}

void Protocol76::AddCreatureHealth(NetworkMessage& msg,const Creature* creature)
{
	msg.AddByte(0x8C);
	msg.AddU32(creature->getID());
	msg.AddByte(std::max((int32_t)1, creature->getHealth() * 100 / std::max(creature->getMaxHealth(), (int32_t)1) ));
}

void Protocol76::AddCreatureInvisible(NetworkMessage& msg, const Creature* creature)
{
	msg.AddByte(0);
	msg.AddU16(0);
}

void Protocol76::AddCreatureOutfit(NetworkMessage& msg, const Creature* creature, const Outfit_t& outfit)
{
	msg.AddByte(outfit.lookType);

	if(outfit.lookType != 0){
		msg.AddByte(outfit.lookHead);
		msg.AddByte(outfit.lookBody);
		msg.AddByte(outfit.lookLegs);
		msg.AddByte(outfit.lookFeet);
	}
	else{
		msg.AddItemId(outfit.lookTypeEx);
	}
}

void Protocol76::AddWorldLight(NetworkMessage& msg, const LightInfo& lightInfo)
{
	msg.AddByte(0x82);
	msg.AddByte(lightInfo.level);
	msg.AddByte(lightInfo.color);
}

void Protocol76::AddCreatureLight(NetworkMessage& msg, const Creature* creature)
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
void Protocol76::AddContainerItem(NetworkMessage& msg, uint8_t cid, const Item* item)
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

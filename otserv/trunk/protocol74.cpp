//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// implementation of tibia v7.4 protocoll
//////////////////////////////////////////////////////////////////////
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful
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

#include "networkmessage.h"
#include "protocol74.h"

#include "items.h"

#include "tile.h"
#include "creature.h"
#include "player.h"
#include "status.h"

#include <stdio.h>

#include "luascript.h"

#include "tasks.h"
#include "otsystem.h"

extern LuaScript g_config;
std::map<long, Creature*> channel;

Protocol74::Protocol74(SOCKET s)
{
  OTSYS_THREAD_LOCKVARINIT(bufferLock);
  this->s = s;
}


Protocol74::~Protocol74()
{
	OTSYS_THREAD_LOCKVARRELEASE(bufferLock);
}


bool Protocol74::ConnectPlayer()
{	
  Status* stat = Status::instance();
  if(stat->playersonline >= g_config.getGlobalNumber("maxplayers") && player->access == 0)
    return false;
  else                    
    return game->placeCreature(player);
}


void Protocol74::ReceiveLoop()
{
  NetworkMessage msg;
  
	while (!pendingLogout && msg.ReadFromSocket(s))
  {
    parsePacket(msg);
  }

	if (s) {
		closesocket(s);
		s = 0;
	}

  // logout by disconnect?  -> kick
  if (!pendingLogout /*player*/)
  {		
		if(player->inFightTicks >=1000 && player->health >0) {
			//disconnect?
			game->removeCreature(player);
		}
		else{
			game->removeCreature(player);
		}
  }
}


void Protocol74::parsePacket(NetworkMessage &msg)
{
	if(msg.getMessageLength() <= 0)
		return;

  uint8_t recvbyte = msg.GetByte();
  	//a dead player can not performs actions
	if (player->health <= 0 && recvbyte != 0x14) {
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
      //parseCloseChannel(msg);
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
    case 0x69: // client quit without logout 
      break;
    case 0x1E: // keep alive / ping response
    	player->receivePing();
      break;  
    case 0xC9: // change position
      // update position   
      break; 
    case 0x6a:
      this->game->thingMove(player, player, (player->pos.x+1), (player->pos.y-1), player->pos.z, 1);   
      break;
    case 0x6b:
      this->game->thingMove(player, player, (player->pos.x+1), (player->pos.y+1), player->pos.z, 1);   
      break;
    case 0x6c:
      this->game->thingMove(player, player, (player->pos.x-1), (player->pos.y+1), player->pos.z, 1);   
      break;
    case 0x6d:
      this->game->thingMove(player, player, (player->pos.x-1), (player->pos.y-1), player->pos.z, 1);   
      break;                
    default:
         printf("unknown packet header: %x \n", recvbyte);
         parseDebug(msg);
         break;  
  }
  game->flushSendBuffers();  
				//so we got the action, now we ask the map to execut it
/*  if(action.type!=ACTION_NONE){
  	switch(map->requestAction(player, &action)){
		case TMAP_ERROR_TILE_OCCUPIED:
			sendPlayerSorry(TMAP_ERROR_TILE_OCCUPIED);
		break;
	}
  }
  */
}

/*int Protocol74::doAction(Action* action){
	if(action->type!=ACTION_NONE){
		switch(map->requestAction(player,action)){
			case TMAP_ERROR_TILE_OCCUPIED:
				sendPlayerSorry(TMAP_ERROR_TILE_OCCUPIED);
			break;
		}
	}
	return 0;
}*/


void Protocol74::GetTileDescription(const Tile* tile, NetworkMessage &msg)
{
	if (tile)
	{
		int count = 0;
		if(tile->ground) {
			msg.AddItem(tile->ground);
			++count;
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

void Protocol74::GetMapDescription(unsigned short x, unsigned short y, unsigned char z,
                                   unsigned short width, unsigned short height,
                                   NetworkMessage &msg)
{
	Tile* tile;
	int skip = -1;
	int startz, endz, offset, zstep, cc = 0;
	if (z > 7) {
		//startz = std::min(z + 2, MAP_LAYER - 1);
		//endz = std::max(z - 2, 6 /*(8 - 2)*/);
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


void Protocol74::checkCreatureAsKnown(unsigned long id, bool &known, unsigned long &removedKnown)
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
  if(knownPlayers.size() > 64)
  {
    // lets try to remove one from the end of the list
    for (int n = 0; n < 64; n++)
    {
      removedKnown = knownPlayers.front();

      Creature *c = game->getCreatureByID(removedKnown);
      if ((!c) || (!CanSee(c->pos.x, c->pos.y, c->pos.z)))
        break;

      // this creature we can't remove, still in sight, so back to the end
      knownPlayers.pop_front();
      knownPlayers.push_back(removedKnown);
    }

    // hopefully we found someone to remove :S, we got only 64 tries
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
void Protocol74::parseLogout(NetworkMessage &msg)
{
    if(player->inFightTicks >=1000 && player-> health >0) {
         sendCancel("You may not logout during or immediately after a fight!");
         return;
     }    
     logout();
}

void Protocol74::logout(){
	// we ask the game to remove us
	if (game->removeCreature(player)) {
		pendingLogout = true;
	}	
}

void Protocol74::parseGetChannels(NetworkMessage &msg){
     sendChannels();
}

void Protocol74::parseOpenChannel(NetworkMessage &msg){
     unsigned short channelId = msg.GetU16();
     sendChannel(channelId);
     std::map<long, Creature*>::iterator sit = channel.find(player->getID());
     if( sit == channel.end() ) {
          channel[player->getID()] = player;
         }
}

void Protocol74::parseCloseChannel(NetworkMessage &msg){
     /* unsigned short channelId = */msg.GetU16();
     std::map<long, Creature*>::iterator sit = channel.find(player->getID());
     if( sit != channel.end() ) {
          channel.erase(sit);
     }
}

void Protocol74::parseOpenPriv(NetworkMessage &msg){
     std::string receiver; 
     receiver = msg.GetString();
     Creature* c = game->getCreatureByName(receiver.c_str());
     Player* player = dynamic_cast<Player*>(c);
     if(player) 
     	sendOpenPriv(receiver);
}

void Protocol74::sendOpenPriv(std::string &receiver){
     NetworkMessage newmsg; 
     newmsg.AddByte(0xAD); 
     newmsg.AddString(receiver);      
     WriteBuffer(newmsg);
}     

void Protocol74::parseCancelMove(NetworkMessage &msg)
{
player->cancelMove = true;
}

void Protocol74::parseModes(NetworkMessage &msg)
{
player->fightMode = msg.GetByte();
player->followMode = msg.GetByte();
}

void Protocol74::parseDebug(NetworkMessage &msg)
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

void Protocol74::parseMoveByMouse(NetworkMessage &msg)
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

#ifdef __DEBUG__
		std::cout << "Walk by mouse: Direction: " << dir << std::endl;
#endif
		path.push_back(dir);
  }
  // then we schedule the movement...
  // the interval seems to depend on the speed of the char?
  if(player->cancelMove) {
		player->cancelMove = false;
		sendCancelWalk("");
	}
  else{     
		game->addEvent(makeTask(0, MovePlayer(player->getID()), path, 400, StopMovePlayer(player->getID())));
 }
}


void Protocol74::parseMoveNorth(NetworkMessage &msg)
{
	this->sleepTillMove();
  game->thingMove(player, player,
		player->pos.x, player->pos.y-1, player->pos.z, 1);
}


void Protocol74::parseMoveEast(NetworkMessage &msg)
{
	this->sleepTillMove();
  game->thingMove(player, player,
		player->pos.x+1, player->pos.y, player->pos.z, 1);
}


void Protocol74::parseMoveSouth(NetworkMessage &msg)
{
	this->sleepTillMove();
  game->thingMove(player, player,
		player->pos.x, player->pos.y+1, player->pos.z, 1);
}


void Protocol74::parseMoveWest(NetworkMessage &msg)
{
	this->sleepTillMove();
  game->thingMove(player, player,
		player->pos.x-1, player->pos.y, player->pos.z, 1);
}


void Protocol74::parseTurnNorth(NetworkMessage &msg)
{
	game->creatureTurn(player, NORTH);
}


void Protocol74::parseTurnEast(NetworkMessage &msg)
{
  game->creatureTurn(player, EAST);
  NetworkMessage newmsg;
  WriteBuffer(newmsg);
}


void Protocol74::parseTurnSouth(NetworkMessage &msg)
{
  game->creatureTurn(player, SOUTH);
}


void Protocol74::parseTurnWest(NetworkMessage &msg)
{
  game->creatureTurn(player, WEST);
  
}


void Protocol74::parseRequestOutfit(NetworkMessage &msg)
{
  msg.Reset();

  msg.AddByte(0xC8);
  msg.AddByte(player->looktype);
  msg.AddByte(player->lookhead);
  msg.AddByte(player->lookbody);
  msg.AddByte(player->looklegs);
  msg.AddByte(player->lookfeet);
  switch (player->sex) {
	  case 0:
		  msg.AddByte(PLAYER_FEMALE_1);
		  msg.AddByte(PLAYER_FEMALE_7);
		  break;
	  case 1:
		  msg.AddByte(PLAYER_MALE_1);
		  msg.AddByte(PLAYER_MALE_7);
		  break;
	  case 2:
		  msg.AddByte(160);
		  msg.AddByte(160);
		  break;
	  default:
		  msg.AddByte(PLAYER_MALE_1);
		  msg.AddByte(PLAYER_MALE_7);
  }

	WriteBuffer(msg);
}


void Protocol74::sendSetOutfit(const Creature* creature) {
	if (CanSee(creature->pos.x, creature->pos.y, creature->pos.z)) {
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

void Protocol74::sendInventory(unsigned char sl_id){	
	NetworkMessage msg;
	AddPlayerInventoryItem(msg,player,sl_id);
	WriteBuffer(msg);
}

void Protocol74::sendStats(){
	NetworkMessage msg;
	AddPlayerStats(msg,player);
	WriteBuffer(msg);
}

void Protocol74::sendTextMessage(MessageClasses mclass, const char* message){
	NetworkMessage msg;
	AddTextMessage(msg,mclass, message);
	WriteBuffer(msg);
}

void Protocol74::sendTextMessage(MessageClasses mclass, const char* message,const Position &pos, unsigned char type){
	NetworkMessage msg;
	AddMagicEffect(msg,pos,type);
	AddTextMessage(msg,mclass, message);
	WriteBuffer(msg);
}

void Protocol74::sendChannels(){
     NetworkMessage newmsg;
	 newmsg.AddByte(0xAB);
	 
	 newmsg.AddByte(3); //how many
	 
	 newmsg.AddByte(0xFF); //priv chan
	 newmsg.AddByte(0xFF); //priv chan
	 newmsg.AddString("Private Chat Channel");
	 
	 newmsg.AddByte(0x00); //clan chan
	 newmsg.AddByte(0x00); //clan chan
	 newmsg.AddString("Clan Channel");
	 
	 newmsg.AddByte(0x04);
	 newmsg.AddByte(0x00);
	 newmsg.AddString("Game-Chat");
	 WriteBuffer(newmsg);
}

void Protocol74::sendChannel(unsigned short channelId){
     NetworkMessage newmsg;
     if(channelId == 4){
		newmsg.AddByte(0xAC);
	 
		newmsg.AddU16(channelId);
	 
		newmsg.AddString("Game-Chat");
		WriteBuffer(newmsg);
     }
	 
}

void Protocol74::sendIcons(int icons){
     NetworkMessage newmsg;
	 newmsg.AddByte(0xA2);
	 newmsg.AddByte(icons);
	 WriteBuffer(newmsg);
}

void Protocol74::parseSetOutfit(NetworkMessage &msg)
{
    int temp = msg.GetByte();
    if ( (player->sex == 0 && temp >= PLAYER_FEMALE_1 && temp <= PLAYER_FEMALE_7) || (player->sex == 1 && temp >= PLAYER_MALE_1 && temp <= PLAYER_MALE_7)){ 
	player->looktype= temp;
	player->lookmaster = player->looktype;
	player->lookhead=msg.GetByte();
	player->lookbody=msg.GetByte();
	player->looklegs=msg.GetByte();
	player->lookfeet=msg.GetByte();

	if (player->sex > 1) {
		std::cout << "set outfit to: " << (int)(player->lookhead) << " / " << (int)player->lookbody << " / " << (int)player->looklegs << " / " <<  (int)player->lookfeet << std::endl;
	}
	game->creatureChangeOutfit(player);
    }
}


void Protocol74::parseUseItemEx(NetworkMessage &msg)
{
	unsigned short from_x = msg.GetU16();
	unsigned short from_y = msg.GetU16();
	unsigned char from_z = msg.GetByte();
	/*unsigned short item = */msg.GetU16();
	unsigned char from_stackpos = msg.GetByte();
	unsigned short to_x = msg.GetU16();
	unsigned short to_y = msg.GetU16();
	unsigned char to_z = msg.GetByte();
	/*unsigned short tile_id = */msg.GetU16();
	/*unsigned char to_stackpos =*/ msg.GetByte();

	Position pos;
	pos.x = to_x;
	pos.y = to_y;
	pos.z = to_z;

	if(from_x == 0xFFFF) {
		if(from_y & 0x40) { // use item from container
			unsigned char containerid = from_y & 0x0F;
			Container* container = player->getContainer(containerid);
			
			if(!container)
				return;

			Item* runeitem = container->getItem(from_z);
			if(!runeitem)
				return;

			if(game->creatureUseItem(player, pos, runeitem)) {
				runeitem->setItemCharge(std::max((int)runeitem->getItemCharge() - 1, 0) );
				
				if(runeitem->getItemCharge() == 0) {
					container->removeItem(runeitem);
					delete runeitem;
					
					NetworkMessage msg;
					for(containerLayout::const_iterator cit = player->getContainers(); cit != player->getEndContainer(); ++cit) {
						if(cit->second == container) {
							//remove item
							msg.AddByte(0x72);
							msg.AddByte(cit->first);
							msg.AddByte(from_z);
						}
					}
					WriteBuffer(msg);
					//sendContainerUpdated(runeitem, containerid, 0xFF, from_z, 0xFF, true);
				}
			}
		}
		else // use item from inventory
		{
			Item* runeitem = player->getItem(from_y);
			if(!runeitem)
				return;
			
			if(game->creatureUseItem(player, pos, runeitem)) {
				runeitem->setItemCharge(std::max((int)runeitem->getItemCharge() - 1, 0) );
				
				if(runeitem->getItemCharge() == 0) {
					player->items[from_y] = NULL;
					delete runeitem;
					NetworkMessage netmsgs;
					AddPlayerInventoryItem(netmsgs,player, from_y);
					player->sendNetworkMessage(&netmsgs);
				}
			}
		}
	}
	else // use item from ground
	{
		int dist_x = from_x - player->pos.x;
		int dist_y = from_y - player->pos.y;
		if(dist_x > 1 || dist_x < -1 || dist_y > 1 || dist_y < -1 || (from_z != player->pos.z))
			return;
			
		Tile *t = game->getTile(from_x, from_y, from_z);
		if(!t)
			return;
		
		Item *runeitem = (Item*)t->getThingByStackPos(from_stackpos);
		if(!runeitem)
			return;
		
		if(game->creatureUseItem(player, pos, runeitem)) {
			runeitem->setItemCharge(std::max((int)runeitem->getItemCharge() - 1, 0) );
			
			if(runeitem->getItemCharge() == 0) {
				t->removeThing(runeitem);
				delete runeitem;
				game->creatureBroadcastTileUpdated(Position(from_x, from_y, from_z));
			}
		}
	}
}

void Protocol74::sendContainer(unsigned char index, Container *container)
{
	if(!container)
		return;

	NetworkMessage msg;

	player->addContainer(index, container);

	msg.AddByte(0x6E);
	msg.AddByte(index);

	msg.AddU16(container->getID());
	msg.AddString(container->getName());
	msg.AddByte(container->capacity());
	if(container->getParent() != NULL)
		/* TODO: implement up arrow */
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

void Protocol74::parseUseItem(NetworkMessage &msg)
{
	//open main backpack
	//FFFF	0300	00	5506	00	00
	//sent:
	//n		bpid	len	backpack						max		cur	
	//00	5506	08	006261636B7061636B	1400	09	5406540654063B08C206DE0661DE066431080C4E08

	//open bag 0, new window
	//FFFF	4000	00	5406	00	01
	//sent:
	//n		bpid	len	backpack						max		cur	
	//01	5406	03	00626167						0801	00
	
	//open bag 1, new window
	//FFFF	4000	01	5406	01	02
	//sent:
	//n		bpid	len	backpack						max		cur	
	//02	5406	03	00626167						0801	00

	//close bag 0
	//8701
	//sent:

	//open bag 0, new window
	//FFFF	4000	00	5406	00	01
	//sent:
	//n		bpid	len	backpack						max		cur	
	//01	5406	03	00626167						0801	00

	//open bag 2, new window
	//FFFF	4000	02	5406	02	03
	//sent:
	//n		bpid	len	backpack						max		cur	
	//03	5406	03	00626167						0801	00
	unsigned short x = msg.GetU16();
	unsigned short y = msg.GetU16();
	unsigned char z = msg.GetByte();
	unsigned short item = msg.GetU16();
	unsigned char stack = msg.GetByte();
	unsigned char index = msg.GetByte();

#ifdef __DEBUG__
	std::cout << "parseUseItem: " << "x: " << x << ", y: " << (int)y <<  ", z: " << (int)z << ", item: " << (int)item << ", stack: " << (int)stack << ", index: " << (int)index << std::endl;
#endif

	if(Item::items[item].iscontainer)
	{
		msg.Reset();
		Container *newcontainer = NULL;
		Container *parentcontainer = NULL;

  	if(x != 0xFFFF) {
			if(std::abs(player->pos.x - x) > 1 || std::abs(player->pos.y - y) > 1)
				return;

			Tile *t = game->getTile(x, y, z);

			if(t) {
				newcontainer = dynamic_cast<Container*>(t->getThingByStackPos(stack));
			}
		}
		else if(x == 0xFFFF) {		
			if(0x40 & y) {
				unsigned char containerid = y & 0x0F;
				parentcontainer = player->getContainer(containerid);

				if(!parentcontainer) {
					return;
				}

				int n = 0;
				for (ContainerList::const_iterator cit = parentcontainer->getItems(); cit != parentcontainer->getEnd(); cit++) {
					if(n == z) {
						newcontainer = dynamic_cast<Container*>(*cit);
						break;
					}
					else
						n++;
				}
			}
			else
				
				newcontainer = dynamic_cast<Container *>(player->getItem(y));
		}

		if(newcontainer) {			
			if(newcontainer->depot == 0){
				sendContainer(index, newcontainer);
			}
			else{				
				Container *newcontainer2 = player->getDepot(newcontainer->depot);
				if(newcontainer2){
					//update depot coordinates					
					newcontainer2->pos = newcontainer->pos;
					sendContainer(index, newcontainer2);
				}
			}
		}
	}
}

void Protocol74::parseCloseContainer(NetworkMessage &msg)
{
	unsigned char containerid = msg.GetByte();
	player->closeContainer(containerid);

	msg.Reset();

	msg.AddByte(0x6F);
	msg.AddByte(containerid);
	WriteBuffer(msg);
}

void Protocol74::parseUpArrowContainer(NetworkMessage &msg)
{
	unsigned char containerid = msg.GetByte();

	Container *container = player->getContainer(containerid);
	if(!container)
		return;

	Container *parentcontainer = container->getParent();
	if(parentcontainer) {
		sendContainer(containerid, parentcontainer);
	}
}

void Protocol74::parseThrow(NetworkMessage &msg)
{
  unsigned short from_x     = msg.GetU16();
  unsigned short from_y     = msg.GetU16(); 
  unsigned char  from_z     = msg.GetByte();
  ///* unsigned char  duno1 = */ msg.GetByte(); // item type 2 bytes
  ///* unsigned char  duno2 = */ msg.GetByte();
	unsigned short itemid = msg.GetU16();
  unsigned char  from_stack = msg.GetByte();
  unsigned short to_x       = msg.GetU16();
  unsigned short to_y       = msg.GetU16(); 
  unsigned char  to_z       = msg.GetByte();
	unsigned char count       = msg.GetByte();

	bool toInventory = false;
	bool fromInventory = false;

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
		
		game->thingMove(player, from_cid, from_z, fromInventory, to_cid, to_z, toInventory, count);
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

		game->thingMove(player, from_cid, from_z, fromInventory, Position(to_x, to_y, to_z), count);
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

		game->thingMove(player, Position(from_x, from_y, from_z), from_stack, to_cid, to_z, toInventory, count);
	}
	//ground to ground
	else
		game->thingMove(player, from_x, from_y, from_z, from_stack, to_x, to_y, to_z, count);
}


void Protocol74::parseLookAt(NetworkMessage &msg){
  Position LookPos = msg.GetPosition();
  unsigned short ItemNum = msg.GetU16();
	unsigned char stackpos = msg.GetByte();

#ifdef __DEBUG__
  std::cout << "look at: " << LookPos << std::endl;
  std::cout << "itemnum: " << ItemNum << " stackpos: " << (long)stackpos<< std::endl;
#endif

  NetworkMessage newmsg;
  std::stringstream ss;

#ifdef __DEBUG__
	ss << "You look at " << LookPos << " and see Item # " << ItemNum << ".";
  AddTextMessage(newmsg,MSG_INFO, ss.str().c_str());
#else
	Item *item = NULL;
	Creature *creature = NULL;

	if(LookPos.x != 0xFFFF) {
		Tile* tile = game->getTile(LookPos.x, LookPos.y, LookPos.z);
		if(tile){
			item = dynamic_cast<Item*>(tile->getThingByStackPos(stackpos));
			creature = dynamic_cast<Creature*>(tile->getThingByStackPos(stackpos));
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
		AddTextMessage(newmsg,MSG_INFO, item->getDescription().c_str());
	}
	else if(creature) {
		if(player == creature)
			AddTextMessage(newmsg,MSG_INFO, creature->getDescription(true).c_str());
	else
		AddTextMessage(newmsg,MSG_INFO, creature->getDescription().c_str());
	}
#endif
  
  sendNetworkMessage(&newmsg);
}



void Protocol74::parseSay(NetworkMessage &msg)
{
  SpeakClasses type = (SpeakClasses)msg.GetByte();
  
  std::string receiver;
  unsigned short channelId = 0;
  if (type == 4)
    receiver = msg.GetString();
  if (type == 5)
    channelId = msg.GetU16();
  std::string text = msg.GetString();

	game->creatureSaySpell(player, text);

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
      game->creatureSpeakTo(player, receiver, text);
      break;
    case SPEAK_CHANNEL_Y:
      game->creatureToChannel(player, type, text, channelId);
      break;
    case SPEAK_BROADCAST:
      game->creatureBroadcastMessage(player, text);
      break;
  }
}

void Protocol74::parseAttack(NetworkMessage &msg)
{
  unsigned long playerid = msg.GetU32();
  player->setAttackedCreature(playerid);
}

/*
void Protocol74::sendAction(Action* action){

	std::string buf = "  ";
	if(action->type==ACTION_ITEM_APPEAR){
		sendPlayerItemAppear(action);
	}
	if(action->type==ACTION_ITEM_DISAPPEAR){
		sendPlayerItemDisappear(action);
	}
	if(action->type==ACTION_ITEM_CHANGE){
		sendPlayerItemChange(action);
	}
	if(action->type==ACTION_GROUND_CHANGE){
		sendPlayerChangeGround(action);
	}
	if(action->type==ACTION_LOOK_AT){
		this->sendPlayerLookAt(action->buffer);
	}
	if(action->type==ACTION_REQUEST_APPEARANCE){
		sendPlayerAppearance(action);
	}
	if(action->type==ACTION_CHANGE_APPEARANCE){
		sendPlayerChangeAppearance(action);
	}
}
*/


/*void Protocol74::sendPlayerItemAppear(Action* action){
	std::string buf = "  ";
	buf+=(char)0x6A;
	ADD2BYTE(buf, action->pos1.x);
	ADD2BYTE(buf, action->pos1.y);
	buf+=(char)action->pos1.z;
	ADD2BYTE(buf, action->id);
	if(action->count!=0)
	buf+=(unsigned char)action->count;
	std::cout << "Count:" << action->count << std::endl;
	buf[0]=(char)(buf.size()-2)%256;
	buf[1]=(char)((buf.size()-2)/256)%256;
//	SendData(psocket,buf);
}
*/

/*
void Protocol74::sendPlayerItemDisappear(Action* action){
	std::string buf = "  ";
	buf+=(char)0x6C;
	ADD2BYTE(buf, action->pos1.x);
	ADD2BYTE(buf, action->pos1.y);
	buf+=(char)action->pos1.z;
	buf+=(char)action->stack;
	buf[0]=(char)(buf.size()-2)%256;
	buf[1]=(char)((buf.size()-2)/256)%256;
//	SendData(psocket,buf);
}
*/

/*
void Protocol74::sendPlayerChangeAppearance(Action* action){
	std::string buf = "  ";
	buf+=(char)0x8E;
	ADD4BYTE(buf, action->creature->getID());
	buf+=action->creature->getLook();
	buf[0]=(char)(buf.size()-2)%256;
	buf[1]=(char)((buf.size()-2)/256)%256;
	SendData(psocket,buf);
}
*/


void Protocol74::sendPlayerLookAt(std::string msg){
/*	std::string buf = "  ";
	buf+=(char)0xB4;
	buf+=(char)0x13;
	ADD2BYTE(buf, msg.size());
	buf+=msg;
	buf[0]=(char)(buf.size()-2)%256;
	buf[1]=(char)((buf.size()-2)/256)%256;*/
//	SendData(psocket,buf);
}


/*void Protocol74::sendPlayerChangeGround(Action* action){
	std::string buf = "  ";
	buf+=(char)0x6B;
	ADD2BYTE(buf, action->pos1.x);
	ADD2BYTE(buf, action->pos1.y);
	buf+=(char)action->pos1.z;
	//this is a stackpos but is fixed as ground is always 0x00
	buf+=(char)0x00;
	ADD2BYTE(buf,action->id);
	buf[0]=(char)(buf.size()-2)%256;
	buf[1]=(char)((buf.size()-2)/256)%256;
//	SendData(psocket,buf);
}

  */

/*
void Protocol74::sendPlayerItemChange(Action* action){
	std::string buf = "  ";
	buf+=(char)(0x6B);
	ADDPOS(buf, action->pos1);
	buf+=(char)action->stack;//stack?
	ADD2BYTE(buf,action->id);
	ADD1BYTE(buf,action->count);
	buf[0]=(char)(buf.size()-2)%256;
	buf[1]=(char)((buf.size()-2)/256)%256;
//	SendData(psocket,buf);
}
*/


bool Protocol74::CanSee(int x, int y, int z) const
{
#ifdef __DEBUG__
	if(z < 0 || z >= MAP_LAYER) {
		std::cout << "WARNING! Protocol74::CanSee() Z-value is out of range!" << std::endl;
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


void Protocol74::sendNetworkMessage(NetworkMessage *msg)
{
	WriteBuffer(*msg);
}



void Protocol74::AddTileUpdated(NetworkMessage &msg, const Position &pos)
{
#ifdef __DEBUG__
		std::cout << "Pop-up item from below..." << std::endl;
#endif

	//1D00	69	CF81	587C	07	9501C405C405C405C405C405C405780600C405C40500FF
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

void Protocol74::sendTileUpdated(const Position &pos)
{
  NetworkMessage msg;
	
	AddTileUpdated(msg, pos);
	WriteBuffer(msg);
}

//container to container
void Protocol74::sendThingMove(const Creature *creature,
	const Container *fromContainer, const Container *toContainer, const Item* item,
	unsigned char from_slotid, unsigned char to_slotid, unsigned char oldcount, unsigned char count)
{
	NetworkMessage msg;

	if(fromContainer && fromContainer->pos.x != 0xFFFF && toContainer->pos.x != 0xFFFF) {
		//Auto-close container's
		if(std::abs(player->pos.x - toContainer->pos.x) > 1 || std::abs(player->pos.y - toContainer->pos.y) > 1) {
			const Container *container = dynamic_cast<const Container*>(item);
			if(container) {				
				autoCloseContainers(container, msg);
			}
		}
	}

	Item *container = NULL;
	for(containerLayout::const_iterator cit = player->getContainers(); cit != player->getEndContainer(); ++cit) {
		container  = cit->second;
		unsigned char cid = cit->first;

		if(container && container == fromContainer) {
			if(toContainer == fromContainer) {
				//Item *toSlotItem = toContainer->getItem(to_slotid);
				if(item->isStackable() && item->getItemCountOrSubtype() != oldcount) {
					//add item
					msg.AddByte(0x70);
					msg.AddByte(cid /*to_id*/);
					msg.AddU16(item->getID());
					msg.AddByte(count);

					//update count
					msg.AddByte(0x71);
					msg.AddByte(cid /*to_id*/);
					msg.AddByte(from_slotid + 1);
					msg.AddItem(item);
				}
				else /*Move all*/ {
					
					//remove item			
					msg.AddByte(0x72);
					msg.AddByte(cid /*from_id*/);
					msg.AddByte(from_slotid);

					//add item
					msg.AddByte(0x70);
					msg.AddByte(cid /*to_id*/);
					msg.AddItem(item);
				}
			}
			else {
				if(item->isStackable() && item->getItemCountOrSubtype() != oldcount) {
					//update count
					msg.AddByte(0x71);
					msg.AddByte(cid);
					msg.AddByte(from_slotid);
					msg.AddItem(item);
				}
				else {
					//remove item
					msg.AddByte(0x72);
					msg.AddByte(cid);
					msg.AddByte(from_slotid);
				}
			}
		}

		if(container && container == toContainer && toContainer != fromContainer) {
			if(item->isStackable() && item->getItemCountOrSubtype() != oldcount) {
				//add item
				msg.AddByte(0x70);
				msg.AddByte(cid /*to_id*/);
				msg.AddU16(item->getID());
				msg.AddByte(count);
			}
			else {
				//add item
				msg.AddByte(0x70);
				msg.AddByte(cid /*to_id*/);
				msg.AddItem(item);
			}
		}
	}
	WriteBuffer(msg);
}

//inventory to container
void Protocol74::sendThingMove(const Creature *creature, slots_t fromSlot, const Container *toContainer,
	const Item* item, unsigned char oldcount, unsigned char count)
{
	NetworkMessage msg;
	//Update up-arrow
	//

	if(creature == player) {
		sendThingMove(creature, (Container*)NULL, toContainer, item, 0, 0, count, count);

		AddPlayerInventoryItem(msg,player, fromSlot);
	}
	WriteBuffer(msg);
}

//container to inventory
void Protocol74::sendThingMove(const Creature *creature, const Container *fromContainer, slots_t toSlot,
	const Item* item, unsigned char from_slotid, unsigned char oldcount, unsigned char count)
{
	NetworkMessage msg;
	//Update up-arrow
	//

	if(creature == player) {
		sendThingMove(creature, fromContainer, (Container*)NULL, item, from_slotid, 0, count, count);

		AddPlayerInventoryItem(msg,player, toSlot);
	}
	WriteBuffer(msg);
}

//container to ground
void Protocol74::sendThingMove(const Creature *creature, const Container *fromContainer,
	const Position *newPos, const Item* item, unsigned char from_slotid, unsigned char oldcount, unsigned char count)
{
	NetworkMessage msg;

	//Update up-arrow
	if((fromContainer->pos.x == 0xFFFF && creature == player) &&
		(std::abs(player->pos.x - newPos->x) <= 1 && std::abs(player->pos.y - newPos->y) <= 1)) {
			//
	}
	//Auto-close container's
	else if(std::abs(player->pos.x - newPos->x) > 1 || std::abs(player->pos.y - newPos->y) > 1) {
		const Container *container = dynamic_cast<const Container*>(item);
		if(container) {			
			autoCloseContainers(container, msg);			
		}
	}

	if(CanSee(item->pos.x, item->pos.y, item->pos.z)) {
		AddAppearThing(msg,item->pos);
		msg.AddItem(item);
	}

	
	for(containerLayout::const_iterator cit = player->getContainers(); cit != player->getEndContainer(); ++cit) {
		unsigned char cid = cit->first;
		if(cit->second == fromContainer) {
			//remove item
			msg.AddByte(0x72);
			msg.AddByte(cid);
			msg.AddByte(from_slotid);
		}
	}
	WriteBuffer(msg);
}

//inventory to ground
void Protocol74::sendThingMove(const Creature *creature, slots_t fromSlot,
	const Position *newPos, const Item* item, unsigned char oldcount, unsigned char count)
{
	NetworkMessage msg;

	//Update up-arrow (if container)
	if(creature == player) {
		if(std::abs(player->pos.x - newPos->x) <= 1 && std::abs(player->pos.y - newPos->y) <= 1 && player->pos.z == newPos->z ) {
			//
		}
		//Auto-close container's
		else {
			const Container *container = dynamic_cast<const Container*>(item);
			if(container) {								
				autoCloseContainers(container, msg);				
			}
		}
	}

	if(CanSee(item->pos.x, item->pos.y, item->pos.z)) {
		AddAppearThing(msg,item->pos);		
		msg.AddItem(item);
	}
	
	if(creature == player) {
		AddPlayerInventoryItem(msg,player, fromSlot);
	}
	WriteBuffer(msg);
}

//ground to container
void Protocol74::sendThingMove(const Creature *creature, const Position *oldPos, const Container *toContainer,
	const Item* item, unsigned char stackpos, unsigned char to_slotid, unsigned char oldcount, unsigned char count)
{
	NetworkMessage msg;

	//Update up-arrow
	if((toContainer->pos.x == 0xFFFF && creature == player))
	{
	}
	//Auto-close container's
	else if((toContainer->pos.x == 0xFFFF) || (std::abs(player->pos.x - toContainer->pos.x) > 1 ||
																						 std::abs(player->pos.y - toContainer->pos.y) > 1)) {
		const Container *container = dynamic_cast<const Container*>(item);
		if(container) {			
			autoCloseContainers(container, msg);		
		}
	}
	//

	if(CanSee(oldPos->x, oldPos->y, oldPos->z)) {
		AddRemoveThing(msg,*oldPos,stackpos);
	}

	/*
	Tile *fromTile = game->getTile(oldPos->x, oldPos->y, oldPos->z);
	if(fromTile && fromTile->getThingCount() > 8) {
#ifdef __DEBUG__
		std::cout << "Pop-up item from below..." << std::endl;
#endif
		//We need to pop up this item
		Thing *newthing = fromTile->getThingByStackPos(9);

		if(newthing != NULL) {
			AddTileUpdated(msg, *oldPos);
		}
	}
	*/

	Container *container = NULL;
	for(containerLayout::const_iterator cit = player->getContainers(); cit != player->getEndContainer(); ++cit) {
		container = cit->second;
		if(container == toContainer) {
			unsigned char cid = cit->first;

			//add item
			msg.AddByte(0x70);
			msg.AddByte(cid /*to_id*/);
			msg.AddItem(item);
		}
	}
	WriteBuffer(msg);
}

//ground to inventory
void Protocol74::sendThingMove(const Creature *creature, const Position *oldPos, slots_t toSlot,
	const Item* item, unsigned char stackpos, unsigned char oldcount, unsigned char count)
{
	NetworkMessage msg;
	if(creature == player) {
		AddPlayerInventoryItem(msg,player, toSlot);		
	}
	else {
		const Container *container = dynamic_cast<const Container*>(item);
		//Auto-closing containers
		if(container) {			
			autoCloseContainers(container, msg);						
		}
	}

	if(CanSee(oldPos->x, oldPos->y, oldPos->z)) {
		AddRemoveThing(msg,*oldPos,stackpos);		
	}
	WriteBuffer(msg);
}

//ground to ground
void Protocol74::sendThingMove(const Creature *creature, const Thing *thing,
	const Position *oldPos, unsigned char oldStackPos, unsigned char oldcount, unsigned char count, bool tele)
{
	NetworkMessage msg;

	const Creature* c = dynamic_cast<const Creature*>(thing);
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
			if(std::abs(player->pos.x - thing->pos.x) > 1 || std::abs(player->pos.y - thing->pos.y) > 1 || player->pos.z != thing->pos.z ) {
				const Container *container = dynamic_cast<const Container*>(thing);
				if(container) {						
					autoCloseContainers(container, msg);						
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
			if(container && container->pos.x != 0xFFFF) {				
				if(std::abs(player->pos.x - container->pos.x) > 1 || std::abs(player->pos.y - container->pos.y) > 1 || player->pos.z != container->pos.z) {
					containers.push_back(cit->second);
				}
			}
		}

		for(std::vector<Container *>::const_iterator it = containers.begin(); it != containers.end(); ++it) {
			autoCloseContainers(*it, msg);
		}
  }	

	/*
	Tile *fromTile = game->getTile(oldPos->x, oldPos->y, oldPos->z);
	if(fromTile && fromTile->getThingCount() > 8) {
#ifdef __DEBUG__
		std::cout << "Pop-up item from below..." << std::endl;
#endif
		//We need to pop up this item
		Thing *newthing = fromTile->getThingByStackPos(9);

		if(newthing != NULL) {
			AddTileUpdated(msg, *oldPos);
		}
	}
	*/

	WriteBuffer(msg);
}


//close container and its child containers
void Protocol74::autoCloseContainers(const Container *container, NetworkMessage &msg)
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
/*
void Protocol74::sendCreatureAppear(const Creature *creature)
{
  NetworkMessage msg;
  if ((creature != player) && CanSee(creature->pos.x, creature->pos.y, creature->pos.z))
  {
   // msg.AddByte(0xD3);
   // msg.AddU32(creature->getID());

    bool known;
    unsigned long removedKnown;
    checkCreatureAsKnown(creature->getID(), known, removedKnown);

	AddAppearThing(msg,creature->pos);    
  	AddCreature(msg,creature, known, removedKnown);

    // login bubble
    AddMagicEffect(msg,creature->pos, 0x0A);

    //msg.AddByte(0x8D);
    //msg.AddU32(creature->getID());
    //msg.AddByte(0xFF); // 00
    //msg.AddByte(0xFF);
	WriteBuffer(msg);
  }
  else if (creature == player)
  {		
	msg.AddByte(0x0A);
    msg.AddU32(player->getID());

    msg.AddByte(0x32);
    msg.AddByte(0x00);

    msg.AddByte(0x00);
    msg.AddByte(0x64);
    msg.AddPosition(player->pos);
    GetMapDescription(player->pos.x-8, player->pos.y-6, player->pos.z, 18, 14, msg);

	AddMagicEffect(msg,player->pos, 0x0A);

	AddPlayerStats(msg,player);	

    msg.AddByte(0x82);
    msg.AddByte(0x6F); //LIGHT LEVEL
    msg.AddByte(0xd7);//light? (seems constant)

    /*msg.AddByte(0x8d);//8d
    msg.AddU32(player->getID());
    msg.AddByte(0x03);//00
    msg.AddByte(0xd7);//d7//
    
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
	WriteBuffer(msg);
	//force flush
	flushOutputBuffer();	
  }
}
*/
/*
void Protocol74::sendCreatureDisappear(const Creature *creature, unsigned char stackPos)
{	
  if ((creature != player) && CanSee(creature->pos.x, creature->pos.y, creature->pos.z))
  {
    NetworkMessage msg;
    AddMagicEffect(msg,creature->pos, NM_ME_PUFF);

	AddRemoveThing(msg,creature->pos,stackpos);
	WriteBuffer(msg);
  }
}
*/
/*
void Protocol74::sendItemChange(const Creature *creature, unsigned char stackPos, const Item* item)
{
  if (CanSee(creature->pos.x, creature->pos.y, creature->pos.z))
	{
    NetworkMessage msg;

    msg.AddByte(0x6B);
    msg.AddPosition(creature->pos);
    msg.AddByte(stackPos);
		msg.AddItem(item);

		msg.WriteToSocket(s);
	}
}
*/

void Protocol74::sendCreatureTurn(const Creature *creature, unsigned char stackPos)
{
  if (CanSee(creature->pos.x, creature->pos.y, creature->pos.z))
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


void Protocol74::sendCreatureSay(const Creature *creature, SpeakClasses type, const std::string &text)
{
  NetworkMessage msg;
  AddCreatureSpeak(msg,creature, type, text, 0);
  WriteBuffer(msg);
}

void Protocol74::sendToChannel(const Creature * creature, SpeakClasses type, const std::string &text, unsigned short channelId){
  NetworkMessage msg;
  AddCreatureSpeak(msg,creature, type, text, channelId);
  WriteBuffer(msg);
}

void Protocol74::sendCancel(const char *msg)
{
  NetworkMessage netmsg;
  AddTextMessage(netmsg,MSG_SMALLINFO, msg);
  WriteBuffer(netmsg);
}

void Protocol74::sendCancelAttacking()
{
  NetworkMessage netmsg;
  netmsg.AddByte(0xa3);
  WriteBuffer(netmsg);
}

void Protocol74::sendChangeSpeed(const Creature *creature)
{
  NetworkMessage netmsg;
  netmsg.AddByte(0x8F);
  
  netmsg.AddU32(creature->getID());
  netmsg.AddU16(creature->getSpeed());
  WriteBuffer(netmsg);
}

void Protocol74::sendCancelWalk(const char *msg)
{
  NetworkMessage netmsg;
  AddTextMessage(netmsg,MSG_SMALLINFO, msg);
  netmsg.AddByte(0xB5);
  netmsg.AddByte(player->getDirection()); // direction
  WriteBuffer(netmsg);
}

void Protocol74::sendThingDisappear(const Thing *thing, unsigned char stackPos, bool tele)
{
	NetworkMessage msg;

	if(!tele) {
		const Creature* creature = dynamic_cast<const Creature*>(thing);
		if(creature && creature->health > 0){
			const Player* remove_player = dynamic_cast<const Player*>(creature);
			if(remove_player == player)
				return;

			if(CanSee(creature->pos.x, creature->pos.y, creature->pos.z)){    	
    			AddMagicEffect(msg,thing->pos, NM_ME_PUFF);
			}
		}
	}

	if(CanSee(thing->pos.x, thing->pos.y, thing->pos.z)) {
		AddRemoveThing(msg,thing->pos, stackPos);
	}

	WriteBuffer(msg);
}

void Protocol74::sendThingAppear(const Thing *thing){
	NetworkMessage msg;
	const Creature* creature = dynamic_cast<const Creature*>(thing);
	if(creature){
		const Player* add_player = dynamic_cast<const Player*>(creature);
		if(add_player == player){
			msg.AddByte(0x0A);
    	msg.AddU32(player->getID());

		  msg.AddByte(0x32);
    	msg.AddByte(0x00);

    	msg.AddByte(0x00);
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
    	msg.AddByte(0xd7);//d7*/
  
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
			WriteBuffer(msg);
			//force flush
			flushOutputBuffer();
			return;
		}
		else if(CanSee(creature->pos.x, creature->pos.y, creature->pos.z)){
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
			AddAppearThing(msg,item->pos);
			msg.AddItem(item);
		}
	}
	
	WriteBuffer(msg);
}

void Protocol74::sendThingTransform(const Thing* thing,int stackpos){
	const Item *item = dynamic_cast<const Item*>(thing);
	if(item){
		NetworkMessage msg;
		//AddTileUpdated(msg,thing->pos);		
		
		msg.AddByte(0x6B);
		msg.AddPosition(thing->pos);
		msg.AddByte(stackpos);	
		msg.AddItem(item);
		
		WriteBuffer(msg);
	}
}

void Protocol74::sendSkills()
{
  NetworkMessage msg;
  AddPlayerSkills(msg,player);
  WriteBuffer(msg);
}

void Protocol74::sendPing()
{
  NetworkMessage msg;
  msg.AddByte(0x1E);
  WriteBuffer(msg);
}

void Protocol74::sendThingRemove(const Thing *thing){
	NetworkMessage msg;
	const Container *container = dynamic_cast<const Container *>(thing);
	if(container) {
		//Auto-close container's
		autoCloseContainers(container, msg);		
	}
	WriteBuffer(msg);
}

void Protocol74::sendDistanceShoot(const Position &from, const Position &to, unsigned char type){
	NetworkMessage msg;
	AddDistanceShoot(msg,from, to,type );
	WriteBuffer(msg);
}
void Protocol74::sendMagicEffect(const Position &pos, unsigned char type){
	NetworkMessage msg;
	AddMagicEffect(msg,pos,type );
	WriteBuffer(msg);
}
void Protocol74::sendAnimatedText(const Position &pos, unsigned char color, std::string text){
	NetworkMessage msg;
	AddAnimatedText(msg,pos,color,text);
	WriteBuffer(msg);
}
void Protocol74::sendCreatureHealth(const Creature *creature){
	NetworkMessage msg;
	AddCreatureHealth(msg,creature);
	WriteBuffer(msg);
}


////////////// before at NetwrokMessage class
void Protocol74::AddTextMessage(NetworkMessage &msg,MessageClasses mclass, const char* message)
{
  msg.AddByte(0xB4);
  msg.AddByte(mclass);
  msg.AddString(message);
}

void Protocol74::AddAnimatedText(NetworkMessage &msg,const Position &pos, unsigned char color, std::string text)
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


void Protocol74::AddMagicEffect(NetworkMessage &msg,const Position &pos, unsigned char type)
{
  msg.AddByte(0x83);
  msg.AddPosition(pos);
  msg.AddByte(type);
}


void Protocol74::AddDistanceShoot(NetworkMessage &msg,const Position &from, const Position &to, unsigned char type)
{
  msg.AddByte(0x85); 
  msg.AddPosition(from);
  msg.AddPosition(to);
  msg.AddByte(type);
}


void Protocol74::AddCreature(NetworkMessage &msg,const Creature *creature, bool known, unsigned int remove)
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

  msg.AddByte(0x00); //
  msg.AddByte(0xDC); // 

  msg.AddU16(creature->getSpeed());
  
  msg.AddByte(0x00); //
  msg.AddByte(0x00); //
}


void Protocol74::AddPlayerStats(NetworkMessage &msg,const Player *player)
{
  msg.AddByte(0xA0);
  msg.AddU16(player->health);
  msg.AddU16(player->healthmax);
  msg.AddU16(player->cap);
  msg.AddU32(player->experience);
  msg.AddByte(player->level);
  msg.AddByte(player->level_percent);
  msg.AddU16(player->mana);
  msg.AddU16(player->manamax);
  msg.AddByte(player->maglevel);
  msg.AddByte(player->maglevel_percent);
}

void Protocol74::AddPlayerSkills(NetworkMessage &msg,const Player *player)
{
  msg.AddByte(0xA1);

  msg.AddByte(player->skills[SKILL_FIST  ][SKILL_LEVEL]);
  msg.AddByte(player->skills[SKILL_FIST  ][SKILL_PERCENT]);
  msg.AddByte(player->skills[SKILL_CLUB  ][SKILL_LEVEL]);
  msg.AddByte(player->skills[SKILL_CLUB  ][SKILL_PERCENT]);
  msg.AddByte(player->skills[SKILL_SWORD ][SKILL_LEVEL]);
  msg.AddByte(player->skills[SKILL_SWORD  ][SKILL_PERCENT]);
  msg.AddByte(player->skills[SKILL_AXE   ][SKILL_LEVEL]);
  msg.AddByte(player->skills[SKILL_AXE  ][SKILL_PERCENT]);
  msg.AddByte(player->skills[SKILL_DIST  ][SKILL_LEVEL]);
  msg.AddByte(player->skills[SKILL_DIST  ][SKILL_PERCENT]);
  msg.AddByte(player->skills[SKILL_SHIELD][SKILL_LEVEL]);
  msg.AddByte(player->skills[SKILL_SHIELD  ][SKILL_PERCENT]);
  msg.AddByte(player->skills[SKILL_FISH  ][SKILL_LEVEL]);
  msg.AddByte(player->skills[SKILL_FISH  ][SKILL_PERCENT]);
}


void Protocol74::AddPlayerInventoryItem(NetworkMessage &msg,const Player *player, int item)
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


void Protocol74::AddCreatureSpeak(NetworkMessage &msg,const Creature *creature, SpeakClasses  type, std::string text, unsigned short channelId)
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

void Protocol74::AddCreatureHealth(NetworkMessage &msg,const Creature *creature)
{
  msg.AddByte(0x8C);
  msg.AddU32(creature->getID());
  msg.AddByte(creature->health*100/creature->healthmax);
}

void Protocol74::AddRemoveThing(NetworkMessage &msg, const Position &pos,unsigned char stackpos){
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

void Protocol74::AddAppearThing(NetworkMessage &msg, const Position &pos){
	if(CanSee(pos.x, pos.y, pos.z)) {		
		msg.AddByte(0x6A);
		msg.AddPosition(pos);	
	}
}


//////////////////////////

void Protocol74::flushOutputBuffer(){
	OTSYS_THREAD_LOCK(bufferLock)
	//force writetosocket	
	OutputBuffer.WriteToSocket(s);
	OutputBuffer.Reset();
	player->SendBuffer = false;	
	OTSYS_THREAD_UNLOCK(bufferLock)
	return;
}

void Protocol74::WriteBuffer(NetworkMessage &add){		
	OTSYS_THREAD_LOCK(bufferLock)
	if(player->SendBuffer == false){
		game->addPlayerBuffer(player);
		player->SendBuffer = true;
	}	
	if(OutputBuffer.getMessageLength() + add.getMessageLength() > NETWORKMESSAGE_MAXSIZE){		
		OTSYS_THREAD_UNLOCK(bufferLock)
		this->flushOutputBuffer();		
		OTSYS_THREAD_LOCK(bufferLock)
		player->SendBuffer = true;
	}	
	OutputBuffer.JoinMessages(add);	
	OTSYS_THREAD_UNLOCK(bufferLock)	
  	return;
}



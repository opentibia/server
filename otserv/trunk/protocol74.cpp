//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// implementation of tibia v7.0 protocoll
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

#include "protocol74.h"

#include "items.h"

#include "tile.h"
#include "creature.h"
#include "player.h"
#include "status.h"

#include "networkmessage.h"

#include <stdio.h>

#include "luascript.h"

#include "tasks.h"

extern LuaScript g_config;
std::map<long, Creature*> channel;

Protocol74::Protocol74(SOCKET s)
{
  this->s = s;
}


Protocol74::~Protocol74()
{
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

	while (msg.ReadFromSocket(s))
  {
    parsePacket(msg);
  }

  if (s)
    closesocket(s);

  // logout by disconnect?  -> kick
  if (player)
  {
			 game->removeCreature(player);
  }
}


void Protocol74::parsePacket(NetworkMessage &msg)
{
	if(msg.getMessageLength() <= 0)
		return;

  uint8_t recvbyte = msg.GetByte();

  if (s && player->health <= 0) {
	 if (recvbyte == 0x14)
		parseLogout(msg);
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
		msg.AddItem(&tile->ground);

		int count = 1;

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
  		msg.AddCreature(*itc, known, removedKnown);
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
	// we ask the game to remove us
	if (game->removeCreature(player))
	{
		player = NULL;
		closesocket(s);
		s = 0;
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
     newmsg.WriteToSocket(s);
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

		//Direction dir = (Direction)msg.GetByte();
#ifdef __DEBUG__
		std::cout << "Walk by mouse: Direction: " << dir << std::endl;
#endif
		path.push_back(dir);
		//path.push_back((Direction)msg.GetByte());
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
  newmsg.WriteToSocket(s);
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


  msg.WriteToSocket(s);
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
	newmsg.WriteToSocket(s);
}
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
	 newmsg.WriteToSocket(s);
	 
}

void Protocol74::sendChannel(unsigned short channelId){
     NetworkMessage newmsg;
     if(channelId == 4){
	 newmsg.AddByte(0xAC);
	 
	 newmsg.AddU16(channelId);
	 
	 newmsg.AddString("Game-Chat");
	 
	 newmsg.WriteToSocket(s);
     }
	 
}

void Protocol74::sendIcons(int icons){
     NetworkMessage newmsg;
	 newmsg.AddByte(0xA2);
	 newmsg.AddByte(icons);
	 newmsg.WriteToSocket(s);
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
	/*unsigned char newpos = */msg.GetByte();
	unsigned short to_x = msg.GetU16();
	unsigned short to_y = msg.GetU16();
	unsigned char to_z = msg.GetByte();
	/*unsigned short tile_id = */msg.GetU16();
	/*unsigned char count = */msg.GetByte();

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
					msg.WriteToSocket(s);
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
					netmsgs.AddPlayerInventoryItem(player, from_y);
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
		
		Item *runeitem = (Item*)t->getThingByStackPos(1);
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

	msg.WriteToSocket(s);
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
	unsigned char un = msg.GetByte();
	unsigned char stack = msg.GetByte();

#ifdef __DEBUG__
	std::cout << "parseUseItem: " << "x: " << x << ", y: " << (int)y <<  ", z: " << (int)z << ", item: " << (int)item << ", un: " << (int)un << ", stack: " << (int)stack << std::endl;
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
				newcontainer = dynamic_cast<Container*>(t->getThingByStackPos(un));
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
			sendContainer(stack, newcontainer);
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

	msg.WriteToSocket(s);
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


/*void Protocol74::parseUseItem(Action* action, NetworkMessage &msg){
	action->type=ACTION_ITEM_USE;
	action->pos1.x=(unsigned char)msg[2]*256+(unsigned char)msg[1];
	action->pos1.y=(unsigned char)msg[4]*256+(unsigned char)msg[3];
	action->pos1.z=(unsigned char)msg[5];
	action->stack=(unsigned char)msg[8];
	action->creature=creature;

}
*/



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
  std::cout << "itemnum: " << ItemNum << std::endl;
#endif

  NetworkMessage newmsg;
  std::stringstream ss;

#ifdef __DEBUG__
	ss << "You look at " << LookPos << " and see Item # " << ItemNum << ".";
  newmsg.AddTextMessage(MSG_INFO, ss.str().c_str());
#else
	Item *item = NULL;
	Creature *creature = NULL;

	if(LookPos.x != 0xFFFF) {
		Tile* tile = game->getTile(LookPos.x, LookPos.y, LookPos.z);
		item = dynamic_cast<Item*>(tile->getThingByStackPos(stackpos));
		creature = dynamic_cast<Creature*>(tile->getThingByStackPos(stackpos));
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
		newmsg.AddTextMessage(MSG_INFO, item->getDescription().c_str());
	}
	else if(creature) {
		if(player == creature)
			newmsg.AddTextMessage(MSG_INFO, creature->getDescription(true).c_str());
	else
		newmsg.AddTextMessage(MSG_INFO, creature->getDescription().c_str());
	}
#endif
  
  sendNetworkMessage(&newmsg);
}



void Protocol74::parseSay(NetworkMessage &msg)
{
  unsigned char type = msg.GetByte();
  
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
    case 0x01:
      game->creatureSay(player, type, text);
      break;
    case 0x02:
      game->creatureWhisper(player, text);
      break;

    case 0x03:
      game->creatureYell(player, text);
      break;

    case 0x04:
      game->creatureSpeakTo(player, receiver, text);
      break;
    case 0x05:
      game->creatureToChannel(player, type, text, channelId);
      break;
    case 0x09:
      game->creatureBroadcastMessage(player, text);
      break;
  }

	//we should check if this was a command
  /*
	if(msg[4]=='!'){
		action->type=ACTION_NONE;
		Position mypos=player->pos;
		int id;
		std::string tmpstr, tmpstr2;
		int count;
		unsigned int space;
		Action* a = new Action;
		switch(msg[5]){
		case 'q':
		  exit(0);
		  break;
		case 's':
//		  map->saveMapXml();
		  break;
		case 'd':
		  switch(player->lookdir){
		  case 0:
		    mypos.y-=1;
		    break;
		  case 1:
		    mypos.x+=1;
		    break;
		  case 2:
		    mypos.y+=1;
		    break;
		  case 3:
		    mypos.x-=1;
		    break;
		  }
		  map->removeItem(mypos);
		  break;
		case 'i':
		  switch(player->lookdir){
		  case 0:
		    mypos.y-=1;
		    break;
		  case 1:
		    mypos.x+=1;
		    break;
		  case 2:
		    mypos.y+=1;
		    break;
		  case 3:
		    mypos.x-=1;
		    break;
		  }

		  tmpstr=msg.substr(7,msg.length()-7);
		  space=tmpstr.find(" ", 0);
		  if(space==tmpstr.npos)
		    tmpstr2="0";
		  else
		    tmpstr2=tmpstr.substr(space,tmpstr.length()-space);
#ifdef __DEBUG__
		  std::cout << tmpstr << std::endl;
#endif
		  id=atoi(tmpstr.c_str());
		  count=atoi(tmpstr2.c_str());

		  a->type=ACTION_CREATE_ITEM;
		  a->id=id;
		  a->pos1=mypos;
		  a->count=count;
		  if(map->summonItem(a)==TMAP_ERROR_NO_COUNT)
		    sendPlayerSorry("You need to specify a count when you summon this item!");
		  delete a;
		  break;

		  
	      
		case 'g':
		  switch(player->lookdir){
		  case 0:
		    mypos.y-=1;
		    break;
		  case 1:
		    mypos.x+=1;
		    break;
		  case 2:
		    mypos.y+=1;
		    break;
		  case 3:
		    mypos.x-=1;
		    break;
		  }

		  tmpstr=msg.substr(7,msg.length()-7);
#ifdef __DEBUG__
		  std::cout << tmpstr << std::endl;
#endif
		  id=atoi(tmpstr.c_str());
#ifdef __DEBUG__
		  std::cout << id << std::endl;
#endif
		  map->changeGround(mypos,  id);
		  break;

		}
		return;
	}
	action->type=ACTION_SAY;
	action->playername=player->name;
	msg.erase(0,4);
	action->buffer=msg;
	action->pos1=player->pos;
  */
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
  msg->WriteToSocket(s);
}



void Protocol74::GetTileUpdated(const Position &pos, NetworkMessage &msg)
{
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

		//GetMapDescription(Pos->x, Pos->y, Pos->z, 1, 1, msg);
  }
}

void Protocol74::sendTileUpdated(const Position &pos)
{
  NetworkMessage msg;
	
	GetTileUpdated(pos, msg);
  msg.WriteToSocket(s);
}

/*
void Protocol74::sendContainerUpdated(Item *item, unsigned char from_id, unsigned char to_id,
																			unsigned char from_slot, unsigned char to_slot, bool remove)
{
	NetworkMessage msg;
	if(from_id == to_id) {
		//remove item
		msg.AddByte(0x72);
		msg.AddByte(from_id);
		msg.AddByte(from_slot);

		//add item
		msg.AddByte(0x70);
		msg.AddByte(to_id);
		msg.AddItem(item);
	}
	else if(remove) {
		//remove item
		msg.AddByte(0x72);
		msg.AddByte(from_id);
		msg.AddByte(from_slot);
	}
	else
	{
		msg.AddByte(0x70);
		msg.AddByte(to_id);
		msg.AddItem(item);
	}

	msg.WriteToSocket(s);
}
*/

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
				unsigned char cid = player->getContainerID(container);
				if(cid != 0xFF) {
					autoCloseContainers(container, msg);
				}
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

	msg.WriteToSocket(s);
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

		msg.AddPlayerInventoryItem(player, fromSlot);
	}

	msg.WriteToSocket(s);
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

		msg.AddPlayerInventoryItem(player, toSlot);
	}

	msg.WriteToSocket(s);
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
			unsigned char cid = player->getContainerID(container);
			if(cid != 0xFF) {
				autoCloseContainers(container, msg);
			}
		}
	}

	if(CanSee(item->pos.x, item->pos.y, item->pos.z)) {
		msg.AddByte(0x6A);
		msg.AddPosition(item->pos);
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

	msg.WriteToSocket(s);
}

//inventory to ground
void Protocol74::sendThingMove(const Creature *creature, slots_t fromSlot,
	const Position *newPos, const Item* item, unsigned char oldcount, unsigned char count)
{
	NetworkMessage msg;

	//Update up-arrow (if container)
	if(creature == player) {
		if(std::abs(player->pos.x - newPos->x) <= 1 && std::abs(player->pos.y - newPos->y) <= 1) {
			//
		}
		//Auto-close container's
		else {
			const Container *container = dynamic_cast<const Container*>(item);
			if(container) {
				unsigned char cid = player->getContainerID(container);
				if(cid != 0xFF) {
					autoCloseContainers(container, msg);
				}
			}
		}
	}

	if(CanSee(item->pos.x, item->pos.y, item->pos.z)) {
		msg.AddByte(0x6A);
		msg.AddPosition(item->pos);
		msg.AddItem(item);
	}
	
	if(creature == player) {
		msg.AddPlayerInventoryItem(player, fromSlot);
	}

	msg.WriteToSocket(s);
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
			unsigned char cid = player->getContainerID(container);
			if(cid != 0xFF) {
				autoCloseContainers(container, msg);
			}
		}
	}
	//

	if(CanSee(oldPos->x, oldPos->y, oldPos->z)) {
		msg.AddByte(0x6C);
		msg.AddPosition(*oldPos);
		msg.AddByte(stackpos);
	}

	Tile *fromTile = game->getTile(oldPos->x, oldPos->y, oldPos->z);
	if(fromTile && fromTile->getThingCount() > 8) {
#ifdef __DEBUG__
		std::cout << "Pop-up item from below..." << std::endl;
#endif
		//We need to pop up this item
		Thing *newthing = fromTile->getThingByStackPos(9);

		if(newthing != NULL) {
			GetTileUpdated(*oldPos, msg);
		}
	}

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

	msg.WriteToSocket(s);
}

//ground to inventory
void Protocol74::sendThingMove(const Creature *creature, const Position *oldPos, slots_t toSlot,
	const Item* item, unsigned char stackpos, unsigned char oldcount, unsigned char count)
{
	NetworkMessage msg;
	if(creature == player) {
		msg.AddPlayerInventoryItem(player, toSlot);
		//
	}
	else {
		const Container *container = dynamic_cast<const Container*>(item);
		//Auto-closing containers
		if(container) {
			unsigned char cid = player->getContainerID(container);
			if(cid != 0xFF) {
				autoCloseContainers(container, msg);
			}
		}
	}

	if(CanSee(oldPos->x, oldPos->y, oldPos->z)) {
		msg.AddByte(0x6C);
		msg.AddPosition(*oldPos);
		msg.AddByte(stackpos);
	}

	msg.WriteToSocket(s);
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
  }
  else
  {
    if (CanSee(oldPos->x, oldPos->y, oldPos->z))
    {
      msg.AddByte(0x6C);
      msg.AddPosition(*oldPos);
      msg.AddByte(oldStackPos);
    }

    if (!(tele && thing == this->player) && CanSee(thing->pos.x, thing->pos.y, thing->pos.z))
    {
      msg.AddByte(0x6A);
      msg.AddPosition(thing->pos);
      if (c) {
        bool known;
        unsigned long removedKnown;
        checkCreatureAsKnown(((Creature*)thing)->getID(), known, removedKnown);
  			msg.AddCreature((Creature*)thing, known, removedKnown);
      }
			else {
        msg.AddItem((Item*)thing);

				//Auto-close container's
				if(std::abs(player->pos.x - thing->pos.x) > 1 || std::abs(player->pos.y - thing->pos.y) > 1) {
					const Container *container = dynamic_cast<const Container*>(thing);
					if(container) {
						unsigned char cid = player->getContainerID(container);
						if(cid != 0xFF) {
							autoCloseContainers(container, msg);
						}
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
				if(std::abs(player->pos.x - container->pos.x) > 1 || std::abs(player->pos.y - container->pos.y) > 1) {
					containers.push_back(cit->second);
				}
			}
		}

		for(std::vector<Container *>::const_iterator it = containers.begin(); it != containers.end(); ++it) {
			autoCloseContainers(*it, msg);
		}
  }	

	Tile *fromTile = game->getTile(oldPos->x, oldPos->y, oldPos->z);
	if(fromTile && fromTile->getThingCount() > 8) {
#ifdef __DEBUG__
		std::cout << "Pop-up item from below..." << std::endl;
#endif
		//We need to pop up this item
		Thing *newthing = fromTile->getThingByStackPos(9);

		if(newthing != NULL) {
			GetTileUpdated(*oldPos, msg);
			//creatureBroadcastTileUpdated(newthing->pos /*&oldPos*/);
		}
	}

	msg.WriteToSocket(s);
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

    msg.AddByte(0x6A);
    msg.AddPosition(creature->pos);
  	msg.AddCreature(creature, known, removedKnown);

    // login bubble
    msg.AddMagicEffect(creature->pos, 0x0A);

    //msg.AddByte(0x8D);
    //msg.AddU32(creature->getID());
    //msg.AddByte(0xFF); // 00
    //msg.AddByte(0xFF);

    msg.WriteToSocket(s);
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

	  msg.AddMagicEffect(player->pos, 0x0A);

    msg.AddPlayerStats(player);

    msg.AddByte(0x82);
    msg.AddByte(0x6F); //LIGHT LEVEL
    msg.AddByte(0xd7);//light? (seems constant)

    /*msg.AddByte(0x8d);//8d
    msg.AddU32(player->getID());
    msg.AddByte(0x03);//00
    msg.AddByte(0xd7);//d7*/

    msg.AddPlayerSkills(player);

    msg.AddPlayerInventoryItem(player, 1);
    msg.AddPlayerInventoryItem(player, 2);
    msg.AddPlayerInventoryItem(player, 3);
    msg.AddPlayerInventoryItem(player, 4);
    msg.AddPlayerInventoryItem(player, 5);
    msg.AddPlayerInventoryItem(player, 6);
    msg.AddPlayerInventoryItem(player, 7);
    msg.AddPlayerInventoryItem(player, 8);
    msg.AddPlayerInventoryItem(player, 9);
    msg.AddPlayerInventoryItem(player, 10);


    msg.AddTextMessage(MSG_EVENT, g_config.getGlobalString("loginmsg", "Welcome.").c_str());

	  msg.WriteToSocket(s);
  }
}


void Protocol74::sendCreatureDisappear(const Creature *creature, unsigned char stackPos)
{
  if ((creature != player) && CanSee(creature->pos.x, creature->pos.y, creature->pos.z))
  {
    NetworkMessage msg;

    msg.AddMagicEffect(creature->pos, NM_ME_PUFF);

    msg.AddByte(0x6c);
    msg.AddPosition(creature->pos);
    msg.AddByte(stackPos);

    msg.WriteToSocket(s);
  }
}

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

    msg.WriteToSocket(s);
  }
}


void Protocol74::sendCreatureSay(const Creature *creature, unsigned char type, const std::string &text)
{
  NetworkMessage msg;

  msg.AddCreatureSpeak(creature, type, text, 0);
  
  msg.WriteToSocket(s);
}

void Protocol74::sendToChannel(const Creature * creature, unsigned char type, const std::string &text, unsigned short channelId){
     NetworkMessage msg;

  msg.AddCreatureSpeak(creature, type, text, channelId);
  
  msg.WriteToSocket(s);
     }

void Protocol74::sendCancel(const char *msg)
{
  NetworkMessage netmsg;
  netmsg.AddTextMessage(MSG_SMALLINFO, msg);
  netmsg.WriteToSocket(s);
}

void Protocol74::sendCancelAttacking()
{
  NetworkMessage netmsg;
  netmsg.AddByte(0xa3);
  netmsg.WriteToSocket(s);
}

void Protocol74::sendChangeSpeed(const Creature *creature)
{
  NetworkMessage netmsg;
  netmsg.AddByte(0x8F);
  
  netmsg.AddU32(creature->getID());
  netmsg.AddU16(creature->getSpeed());
  
  netmsg.WriteToSocket(s);
}

void Protocol74::sendCancelWalk(const char *msg)
{
  NetworkMessage netmsg;
  netmsg.AddTextMessage(MSG_SMALLINFO, msg);
  netmsg.AddByte(0xB5);
  netmsg.AddByte(player->getDirection()); // direction
  netmsg.WriteToSocket(s);
}

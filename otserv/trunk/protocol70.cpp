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

using namespace std;


#include "protocol70.h"

#include "tile.h"
#include "creature.h"
#include "player.h"

#include "networkmessage.h"

#include <stdio.h>

#include "luascript.h"

extern LuaScript g_config;


Protocol70::Protocol70(SOCKET s)
{
  this->s = s;
}


Protocol70::~Protocol70()
{
}


void Protocol70::ConnectPlayer()
{
  map->placeCreature(player);
}


void Protocol70::ReceiveLoop()
{
  NetworkMessage msg;

  while (msg.ReadFromSocket(s))
  {
    parsePacket(msg);
  }

  // TODO: logout -> kick
}


void Protocol70::parsePacket(NetworkMessage &msg)
{
  switch(msg.GetByte())
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
     case 0x82: // throw item
//      parseUseItem(&action, msg);
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

/*int Protocol70::doAction(Action* action){
	if(action->type!=ACTION_NONE){
		switch(map->requestAction(player,action)){
			case TMAP_ERROR_TILE_OCCUPIED:
				sendPlayerSorry(TMAP_ERROR_TILE_OCCUPIED);
			break;
		}
	}
	return 0;
}*/


void Protocol70::GetMapDescription(unsigned short x, unsigned short y, unsigned short z,
                                unsigned short width, unsigned short height,
                                NetworkMessage &msg)
{
	Tile* tile;

  for (int nx = 0; nx < width; nx++)
    for (int ny = 0; ny < height; ny++)
    {
      tile = map->getTile(x + nx, y + ny, z);

      if (tile)
      {
        msg.AddItem(&tile->ground);

        ItemVector::iterator it;
		    for (it = tile->topItems.begin(); it !=tile->topItems.end(); it++)
        {
  			  msg.AddItem(*it);
        }

        CreatureVector::iterator itc;
		    for (itc = tile->creatures.begin(); itc !=tile->creatures.end(); itc++)
        {
  			  msg.AddCreature(*itc, setCreatureAsKnown((*itc)->getID()), false);
        }


		    for (it = tile->downItems.begin(); it !=tile->downItems.end(); it++)
        {
  			  msg.AddItem(*it);
        }
      }
       // tile end
      if ((nx != width-1) || (ny != height-1))
      {
        msg.AddByte(0);
        msg.AddByte(0xFF);
      }
    }
}


bool Protocol70::setCreatureAsKnown(unsigned long id)
{
	std::list<unsigned long>::iterator i;
	for(i = knownPlayers.begin(); i != knownPlayers.end(); ++i)
  {
		if((*i) == id)
			return true;
	}

  if(knownPlayers.size() >= 64)
		knownPlayers.pop_front();

  knownPlayers.push_back(id);

	return false;
}



// Parse methods

void Protocol70::parseLogout(NetworkMessage &msg)
{
	// we ask the map to remove us
	if (map->removeCreature(player))
  {
	  closesocket(s);
  }
}

void Protocol70::parseMoveByMouse(NetworkMessage &msg)
{
  // cancel for now
  msg.Reset();
  msg.AddTextMessage(MSG_STATUS, "Sorry, not possible.");
  msg.AddByte(0xB5);
  msg.WriteToSocket(s);
}


void Protocol70::parseMoveNorth(NetworkMessage &msg)
{
  map->thingMove(player, player,
                 player->pos.x, player->pos.y-1, player->pos.z);
}


void Protocol70::parseMoveEast(NetworkMessage &msg)
{
  map->thingMove(player, player,
                 player->pos.x+1, player->pos.y, player->pos.z);
}


void Protocol70::parseMoveSouth(NetworkMessage &msg)
{
  map->thingMove(player, player,
                 player->pos.x, player->pos.y+1, player->pos.z);
}


void Protocol70::parseMoveWest(NetworkMessage &msg)
{
  map->thingMove(player, player,
                 player->pos.x-1, player->pos.y, player->pos.z);
}


void Protocol70::parseTurnNorth(NetworkMessage &msg)
{
	map->creatureTurn(player, NORTH);
}


void Protocol70::parseTurnEast(NetworkMessage &msg)
{
  map->creatureTurn(player, EAST);
}


void Protocol70::parseTurnSouth(NetworkMessage &msg)
{
  map->creatureTurn(player, SOUTH);
}


void Protocol70::parseTurnWest(NetworkMessage &msg)
{
  map->creatureTurn(player, WEST);
}


void Protocol70::parseRequestOutfit(NetworkMessage &msg)
{
  msg.Reset();

  msg.AddByte(0xC8);
  msg.AddByte(player->looktype);
  msg.AddByte(player->lookhead);
  msg.AddByte(player->lookbody);
  msg.AddByte(player->looklegs);
  msg.AddByte(player->lookfeet);
  msg.AddByte(player->sex?PLAYER_MALE_1:PLAYER_FEMALE_1);
  msg.AddByte(player->sex?PLAYER_MALE_4:PLAYER_FEMALE_4);

  msg.WriteToSocket(s);
}


void Protocol70::sendSetOutfit(const Creature* creature) {
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

void Protocol70::parseSetOutfit(NetworkMessage &msg)
{
	player->looktype=msg.GetByte();
	player->lookhead=msg.GetByte();
	player->lookbody=msg.GetByte();
	player->looklegs=msg.GetByte();
	player->lookfeet=msg.GetByte();

	map->playerChangeOutfit(player);
}


/*void Protocol70::parseUseItem(Action* action, NetworkMessage &msg){
	action->type=ACTION_ITEM_USE;
	action->pos1.x=(unsigned char)msg[2]*256+(unsigned char)msg[1];
	action->pos1.y=(unsigned char)msg[4]*256+(unsigned char)msg[3];
	action->pos1.z=(unsigned char)msg[5];
	action->stack=(unsigned char)msg[8];
	action->creature=creature;

}
*/



void Protocol70::parseThrow(NetworkMessage &msg)
{
  unsigned short from_x     = msg.GetU16();
  unsigned short from_y     = msg.GetU16(); 
  unsigned char  from_z     = msg.GetByte();
  unsigned char  duno1 = msg.GetByte();
  unsigned char  duno2 = msg.GetByte();
  unsigned char  from_stack = msg.GetByte();
  unsigned short to_x       = msg.GetU16();
  unsigned short to_y       = msg.GetU16(); 
  unsigned char  to_z       = msg.GetByte();

  map->thingMove(player, from_x, from_y, from_z, from_stack, to_x, to_y, to_z);
}


void Protocol70::parseLookAt(NetworkMessage &msg){
  Position LookPos = msg.GetPosition();
  unsigned short ItemNum = msg.GetU16();

#ifdef __DEBUG__
  std::cout << "look at: " << LookPos << std::endl;
  std::cout << "itemnum: " << ItemNum << std::endl;
#endif

  NetworkMessage newmsg;
  std::stringstream ss;
  ss << "You " << /*look at " << LookPos << " and todo: implement that operator*/ " see Item # " << ItemNum << ".";
  Position middle;
  newmsg.AddTextMessage(MSG_INFO, ss.str().c_str());
  
  sendNetworkMessage(&newmsg);
}



void Protocol70::parseSay(NetworkMessage &msg)
{
  unsigned char type = msg.GetByte();
  
  string receiver;
  if (type == 4)
    receiver = msg.GetString();

  string text = msg.GetString();

  switch (type)
  {
    case 0x01:
    case 0x02:
      map->playerSay(player, type, text);
      break;

    case 0x03:
      map->playerYell(player, text);
      break;

    case 0x04:
      map->playerSpeakTo(player, receiver, text);
      break;

    case 0x09:
      map->playerBroadcastMessage(player, text);
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

void Protocol70::parseAttack(NetworkMessage &msg)
{
  unsigned long playerid = msg.GetU32();

  player->setAttackedCreature(playerid);
}

/*
void Protocol70::sendAction(Action* action){

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


/*void Protocol70::sendPlayerItemAppear(Action* action){
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
void Protocol70::sendPlayerItemDisappear(Action* action){
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
void Protocol70::sendPlayerChangeAppearance(Action* action){
	std::string buf = "  ";
	buf+=(char)0x8E;
	ADD4BYTE(buf, action->creature->getID());
	buf+=action->creature->getLook();
	buf[0]=(char)(buf.size()-2)%256;
	buf[1]=(char)((buf.size()-2)/256)%256;
	SendData(psocket,buf);
}
*/


void Protocol70::sendPlayerSorry(){
	sendPlayerSorry("Sorry. Not possible");
}

void Protocol70::sendPlayerSorry(tmapEnum){
/*	std::string msg = "Sorry. Not possible";
		std::string buf = "  ";
	buf+=(char)0xB4;
	buf+=(char)0x14;
	ADD2BYTE(buf, msg.size());
	buf+=msg;
	buf[0]=(char)(buf.size()-2)%256;
	buf[1]=(char)((buf.size()-2)/256)%256;*/
	//SendData(psocket,buf);
}

void Protocol70::sendPlayerSorry(std::string msg){
	/*std::string buf = "  ";
	buf+=(char)0xB4;
	buf+=(char)0x14;
	ADD2BYTE(buf, msg.size());
	buf+=msg;
	buf[0]=(char)(buf.size()-2)%256;
	buf[1]=(char)((buf.size()-2)/256)%256;*/
//	SendData(psocket,buf);
}

void Protocol70::sendPlayerLookAt(std::string msg){
/*	std::string buf = "  ";
	buf+=(char)0xB4;
	buf+=(char)0x13;
	ADD2BYTE(buf, msg.size());
	buf+=msg;
	buf[0]=(char)(buf.size()-2)%256;
	buf[1]=(char)((buf.size()-2)/256)%256;*/
//	SendData(psocket,buf);
}


/*void Protocol70::sendPlayerChangeGround(Action* action){
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
void Protocol70::sendPlayerItemChange(Action* action){
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


bool Protocol70::CanSee(int x, int y)
{
  if ((x >= player->pos.x - 8) && (x <= player->pos.x + 9) &&
      (y >= player->pos.y - 6) && (y <= player->pos.y + 7))
    return true;

  return false;
}


void Protocol70::sendNetworkMessage(NetworkMessage *msg)
{
  msg->WriteToSocket(s);
}


void Protocol70::sendThingMove(const Player *player, const Thing *thing, const Position *oldPos, unsigned char oldStackPos)
{
  NetworkMessage msg;

  if ((thing->isCreature()) && (CanSee(oldPos->x, oldPos->y)) && (CanSee(thing->pos.x, thing->pos.y)))
  {
    msg.AddByte(0x6D);
    msg.AddPosition(*oldPos);
    msg.AddByte(oldStackPos);
    msg.AddPosition(thing->pos);
  }
  else
  {
    if (CanSee(oldPos->x, oldPos->y))
    {
      msg.AddByte(0x6C);
      msg.AddPosition(*oldPos);
      msg.AddByte(oldStackPos);
    }

    if (CanSee(thing->pos.x, thing->pos.y))
    {
      msg.AddByte(0x6A);
      msg.AddPosition(thing->pos);
      if (thing->isCreature())
        msg.AddCreature((Creature*)thing, setCreatureAsKnown(((Creature*)thing)->getID()), false);
      else
        msg.AddItem((Item*)thing);
    }
  }

  if (thing == this->player)
  {
    if (oldPos->y > thing->pos.y)        // north, for old x
    {
      msg.AddByte(0x65);
      GetMapDescription(oldPos->x - 8, thing->pos.y - 6, thing->pos.z, 18, 1, msg);
      msg.AddByte(0x7E);
      msg.AddByte(0xFF);
    }
    else if (oldPos->y < thing->pos.y)   // south, for old x
    {
      msg.AddByte(0x67);
      GetMapDescription(oldPos->x - 8, thing->pos.y + 7, thing->pos.z, 18, 1, msg);
      msg.AddByte(0x7E);
      msg.AddByte(0xFF);;
    }

    if (oldPos->x < thing->pos.x)        // east, [with new y]
    {
      msg.AddByte(0x66);
      GetMapDescription(thing->pos.x + 9, thing->pos.y - 6, thing->pos.z, 1, 14, msg);
      msg.AddByte(0x7E);
      msg.AddByte(0xFF);
    }
    else if (oldPos->x > thing->pos.x)   // west, [with new y]
    {
      msg.AddByte(0x68);
      GetMapDescription(thing->pos.x - 8, thing->pos.y - 6, thing->pos.z, 1, 14, msg);
      msg.AddByte(0x7E);
      msg.AddByte(0xFF);
    }
  }

  msg.WriteToSocket(s);
}


void Protocol70::sendCreatureAppear(const Creature *creature)
{
  NetworkMessage msg;

  if ((creature != player) && CanSee(creature->pos.x, creature->pos.y))
  {
    msg.AddByte(0xD3);
    msg.AddU32(creature->getID());

    msg.AddByte(0x6A);
    msg.AddPosition(creature->pos);
    msg.AddCreature(creature, setCreatureAsKnown(creature->getID()), true);

    // login bubble
    msg.AddMagicEffect(creature->pos, 0x0A);

    msg.AddByte(0x8D);
    msg.AddU32(creature->getID());
    msg.AddByte(0x00); // 00
    msg.AddByte(0xD7);

    msg.WriteToSocket(s);
  }
  else
  {
	  msg.AddByte(0x0A);
    msg.AddU32(player->getID());

    msg.AddByte(0x32);
    msg.AddByte(0x00);


    msg.AddByte(0x64);
    msg.AddPosition(player->pos);
    GetMapDescription(player->pos.x-8, player->pos.y-6, player->pos.z, 18, 14, msg);

    msg.AddByte(0xFF); msg.AddByte(0xFF); msg.AddByte(0xFF); msg.AddByte(0xFF);
    msg.AddByte(0xFF); msg.AddByte(0xFF); msg.AddByte(0xFF); msg.AddByte(0xFF);
    msg.AddByte(0xFF); msg.AddByte(0xFF); msg.AddByte(0xFF); msg.AddByte(0xFF);

	  msg.AddByte(0xE4); // TODO Light level
	  msg.AddByte(0xFF);

	  msg.AddMagicEffect(player->pos, 0x0A);

    msg.AddPlayerStats(player);

    msg.AddByte(0x82);
    msg.AddByte(0x6F); //LIGHT LEVEL
    msg.AddByte(0xd7);//ight?
    msg.AddByte(0x8d);//8d
    msg.AddU32(player->getID());
    msg.AddByte(0x00);//00
    msg.AddByte(0xd7);//d7

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


void Protocol70::sendCreatureDisappear(const Creature *creature, unsigned char stackPos)
{
  if ((creature != player) && CanSee(creature->pos.x, creature->pos.y))
  {
    NetworkMessage msg;

    msg.AddMagicEffect(creature->pos, NM_ME_PUFF);

    msg.AddByte(0x6c);
    msg.AddPosition(creature->pos);
    msg.AddByte(stackPos);

    msg.WriteToSocket(s);
  }
}


void Protocol70::sendCreatureTurn(const Creature *creature, unsigned char stackPos)
{
  if (CanSee(creature->pos.x, creature->pos.y))
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


void Protocol70::sendCreatureSay(const Creature *creature, unsigned char type, const string &text)
{
  NetworkMessage msg;

  msg.AddCreatureSpeak(creature, type, text);

  msg.WriteToSocket(s);
}



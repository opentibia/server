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
// $Id$
//////////////////////////////////////////////////////////////////////

#include "tprot70.h"
#include "network.h"
#include "eventscheduler.h"
#include "creature.h"
#include "player.h"
#include "action.h"

#include <unistd.h> // read
#include <stdio.h>
#include <iostream>
#include "luascript.h"

extern LuaScript g_config;

extern EventScheduler es;
extern int g_serverip;


TProt70::TProt70(const Socket& sock, const std::string& in) throw(texception) {
  // first we save the socket the player connected on...
  psocket = sock;
  
  // every data starts with 2 bytes equaling to the length of the data...
  // so firt we test if that value would be higher than the length of the
  // input...
  size_t length = (unsigned char)in[0]+((unsigned char)in[1])*256;
  if (length+2 > in.length()) throw texception("wrong protokoll! length",false);
  
  // the message should the contain 0x0a 0x02
  int i=2;
  if (in[i++]!= 0x0a) throw texception("wrong protokoll!id1",false);
  if (in[i++]!= 0x02) throw texception("wrong protokoll!id2",false);
  

  // so everything looks still ok...
  // next we have the client os...
  // 0x00 ->  windows
  clientos = in[i++];
  if (clientos != 0x00 && clientos != 0x01) 
    throw texception("wrong protokoll!os",false);
  
  
				// then the version should follow...
  version =  (unsigned char)in[i++]+((unsigned char)in[i++])*256;
  
				// and an unknown byte (0x00)
  if (in[i++] != 0x00) throw texception("wrong protokoll!unknown",false);

				// now the namelength should follow...
  int len=  (unsigned char)in[i++]+((unsigned char)in[i++])*256;
  
  for (int j=0; j<len; j++)
    name += in[i++];
  
				// now the passlength should follow...
  length=  (unsigned char)in[i++]+((unsigned char)in[i++])*256;
  
  for (int j=0; j<len; j++)
    passwd += in[i++];
  
  std::cout << "found tprot70 after redirect!\n";
  
} // TProt70::TProt70(Socket sock, string in) throw(texception)

TProt70::~TProt70() throw() {
				//TNetwork::ShutdownClient(psocket);
} // TProt70::~TProt70()

const std::string TProt70::getName() const throw() {
  return name;
}

const std::string TProt70::getPassword() const throw() {
  return passwd;
}

void TProt70::clread(const Socket& sock) throw() {
  static const int MAXMSG = 4096;
  char buffer[MAXMSG];
#ifdef __WINDOWS__
  int nbytes = recv(sock, buffer, MAXMSG,0);
#else
  int nbytes = read(sock, buffer, MAXMSG);
#endif
  
  if (nbytes < 0) { // error
    std::cerr << "read" << std::endl;
    exit(-1);
  } else if (nbytes == 0) { // eof (means logout)
	parseLogout(NULL, "");
  } else {  // lesen erfolgreich
    buffer[nbytes] = 0;
#ifdef __DEBUG__
    std::cout << "read" << std::endl;
#endif
    //TODO message bearbeiten
    std::string s= std::string(buffer, nbytes);;
    parsePacket(s);
   }
}

void TProt70::parsePacket(std::string msg){
  msg.erase(0,2);
				//i just hope CIP was intelligent enough to make the 3. byte unique
				//i guess they were not
  Action* action= new Action;
  action->pos1=player->pos;
  action->creature=this->creature;
#ifdef __DEBUG__
  std::cout << "byte is " << (int)msg[0]<<std::endl;
#endif
  
  
  switch( msg[0] ){
  case 0x14: //logout TODO remove player for others
    parseLogout(action, msg);
    break;
  case 0x65: //move north
    parseMoveNorth(action, msg);
    break;
  case 0x66: //move east
    parseMoveEast(action, msg);
    break;
  case 0x67: //move south
    parseMoveSouth(action, msg);
    break;
  case 0x68: //move west
    parseMoveWest(action, msg);
    break;
  case 0x6F: //turn north
    parseTurnNorth(action, msg);
    break;
  case 0x70: //turn east
    parseTurnEast(action, msg);
    break;
  case 0x71: //turn south
    parseTurnSouth(action, msg);
    break;
  case 0x72: //turn west
    parseTurnWest(action, msg);
    break;
  case 0x78: //throw item
    parseThrow(action, msg);
    break;
 case 0x8C: //throw item
    parseLookAt(action, msg);
    break;
  case 0x96: //say something
    parseSay(action, msg);
    break;
  case 0x64: //TODO client moving with steps
    std::cout << "player tried to move by mouse"<< std::endl;
    sendPlayerSorry("Moving by mouse not possible yet. Use the cursors keys!");
    return;
    break;
  case 0xA1: //attack
    parseAttack(action, msg);
    break;
  case 0xDB: //set outfit TODO
    parseSetOutfit(action, msg);
    break;
  }
				//so we got the action, now we ask the map to execut it
  if(action->type!=ACTION_NONE){
  	switch(map->requestAction(creature,action)){
		case TMAP_ERROR_TILE_OCCUPIED:
			sendPlayerSorry(TMAP_ERROR_TILE_OCCUPIED);
		break;
	}
  }
}

void TProt70::setMap(position newpos, Map& newmap) throw(texception) {
	//this happens when the player logs in
	// first we save the new map position...
	pos = newpos;
	map = &newmap;
	// now we generate he data to send the player for the map
	std::string buf="  "; // first two bytes are packet length
	// packet id, 01 = login? or new map?
	buf += (char)0x0A;
	// now get the playernumber

	//ADD4BYTE(buf,365137);

	buf += (char)(player->pnum%256);
	buf += (char)(player->pnum/256)%256;
	buf += (char)(player->pnum/(256*256))%256;
	buf += (char)(player->pnum/(256*256*256))%256;


	buf += (char)0x32;
	buf += (char)0x00;
	buf += (char)0x64;

			// map position

	buf += (char)(pos.x%256);
	buf += (char)(pos.x/256)%256;
	buf += (char)(pos.y%256);
	buf += (char)(pos.y/256)%256;
	buf += (char)pos.z;


	#ifdef __DEBUG__
	std::cout << "x: " << pos.x << " y: " << pos.y <<std::endl;
	#endif
	std::string buf2 = makeMap(position(pos.x-8,pos.y-6,pos.z),position(pos.x+9,pos.y+7,pos.z));

	buf2.resize(buf2.length()-2);
	buf += buf2;
	buf += (char)0xFF;buf += (char)0xFF;buf += (char)0xFF;buf += (char)0xFF;
	buf += (char)0xFF;buf += (char)0xFF;buf += (char)0xFF;buf += (char)0xFF;
	buf += (char)0xFF;buf += (char)0xFF;buf += (char)0xFF;buf += (char)0xFF;

	buf += (char)0xE4; // TODO Light level
	buf += (char)0xFF;

	//LOGIN BUBBLE
	buf += 0x83;
	buf += (char)(pos.x%256);
	buf += (char)(pos.x/256)%256;
	buf += (char)(pos.y%256);
	buf += (char)(pos.y/256)%256;
	buf += (char)pos.z;
	buf+= (char)0x0A;
	//LOGIN BUBBLE END


	buf+= (char)0xA0; //STATS

	ADD2BYTE(buf,this->player->health);//hitpoints
	ADD2BYTE(buf,this->player->health_max);//hitpoints
	ADD2BYTE(buf,this->player->cap);//cap
	ADD4BYTE(buf,this->player->experience); //experience
	buf+=(char)this->player->level;//level
	ADD2BYTE(buf,this->player->mana);//mana
	ADD2BYTE(buf,this->player->mana_max);//mana
	buf+=(char)this->player->mag_level;//maglevel
	buf+= (char)0x82;


	buf+= (char)0xFF; //LIGHT LEVEL

	//map window?
	//light effect? login bubble?
	buf+= (char)0xd7;//ight?
	buf+= (char)0x8d;//8d
	buf+= (char)0x51;//???
	buf+= (char)0x92;//92
	buf+= (char)0x05;//05
	buf+= (char)0x00;//00
	buf+= (char)0x00;//00
	buf+= (char)0xd7;//d7


	buf+= (char)0xA1; //skills follow
	buf+= (char)this->player->skills[ SKILL_FIST ][ SKILL_LEVEL ]; //fist
	buf+= (char)this->player->skills[ SKILL_CLUB ][ SKILL_LEVEL ]; //club
	buf+= (char)this->player->skills[ SKILL_SWORD ][ SKILL_LEVEL ]; //sword
	buf+= (char)this->player->skills[ SKILL_AXE ][ SKILL_LEVEL ]; //axe
	buf+= (char)this->player->skills[ SKILL_DIST ][ SKILL_LEVEL ]; //dist
	buf+= (char)this->player->skills[ SKILL_SHIELD ][ SKILL_LEVEL ]; //shield
	buf+= (char)this->player->skills[ SKILL_FISH ][ SKILL_LEVEL ]; //fishing
	buf+= (char)0xB4;
	buf+= (char)0x11;
	std::string welcomemsg=g_config.getGlobalString("loginmsg");
	ADD2BYTE(buf,welcomemsg.length());
	buf+= welcomemsg;
	// now we correct the first two bytes which corespond to the length
	// of the packet
	buf[0]=(char)(buf.size()-2)%256;
	buf[1]=(char)((buf.size()-2)/256)%256;
	// and send to client...
	TNetwork::SendData(psocket,buf);
	sendInventory();
} // void TProt70::setMap(position newpos) throw(texception)

std::string TProt70::makeMap(position topleft, position botright) {
	int xswap=1, yswap=1;
	position dif=botright-topleft;
	if(dif.x<0)
		xswap=-1;
	if(dif.y<0)
		yswap=-1;
	std::string buf;
	Tile* tile;

	std::string rowbuf, colbuf, tmpbuf;

	for (unsigned short i=topleft.x; i*xswap<=botright.x*xswap; i+=xswap) {
	#ifdef __DEBUG__
		std::cout << "," <<std::endl;
	#endif

		for (unsigned short j=topleft.y; j*yswap<=botright.y*yswap; j+=yswap) {
	#ifdef __DEBUG__
		std::cout << ".";
	#endif
		tile=map->tile(i,j,topleft.z);
		ADD2BYTE(buf,(*(tile->begin()))->getID());
		Tile::iterator start=tile->end();
		start--;
		for (Tile::iterator it=start; it !=tile->begin() ; --it) {
	#ifdef __DEBUG__
		std::cout << "-";
	#endif
			buf+=makeItem(*it);
		}
		if(tile->getCreature()!=NULL){
			std::cout << "creature" << std::endl;
			buf += makeCreature(tile->getCreature());
		}
		buf+=(char)0x00; // no special thing
		buf+=(char)0xFF; // tile end

		} //for (unsigned short j=topleft.y; j<=botright.y; j++)
	}//for (unsigned short i=topleft.x; i<=botright.x; i++)
	return buf;
} // std::string TProt70::makeMap(const position& topleft, const position& botright)

bool TProt70::knowsPlayer(long id){
	std::list<long>::iterator i;
	for(i=knownPlayers.begin();i!=knownPlayers.end();++i){
		if((*i)==id)
			return true;
	}
	return false;
}

void TProt70::addKnownPlayer(long id){
	if(knownPlayers.size()>=64)
		knownPlayers.pop_front();
	knownPlayers.push_back(id);
}

std::string TProt70::makeItem(Item* c){
	std::string tmp;
	ADD2BYTE(tmp, c->getID());
	if(c->isStackable()){
		ADD1BYTE(tmp, c->getItemCount());
		std::cout << "IS STACKABLE" << std::endl;
	}
	return tmp;
}

std::string TProt70::makeCreature(Creature* c){
	//FIXME this i a bad thing
std::string buf="";


if(c && c->isPlayer())
	{
	bool knows=true;
	player_mem p=((Player*)c)->player;
	//PLAYER??
		if(!this->knowsPlayer(p.pnum)){
			std::cout << "Dont know this player" << std::endl;
			addKnownPlayer(p.pnum);
			knows=false;
	}
	if(knows)
	buf += (char)0x62;
	else{
	buf += (char)0x61;
	buf += (char)0x00;
	buf += (char)0x00;
	buf += (char)0x00;
	buf += (char)0x00;

	buf += (char)0x00; //seperator?
}
	buf += (char)(p.pnum%256);
	buf += (char)(p.pnum/256)%256;
	buf += (char)(p.pnum/(256*256))%256;
	buf += (char)(p.pnum/(256*256*256))%256;
	if(!knows){
		ADD2BYTE(buf, p.name.length());
		buf+=p.name;
	}
	buf += (char)0x64;//SEPERATOR

	buf += (char)0x01; //FACING

	buf += (char)0x88;//FIX

	buf += (char)p.color_hair;
	buf += (char)p.color_shirt;
	buf += (char)p.color_legs;
	buf += (char)p.color_shoes;
	buf += (char)0x00; //NOTHING?

	buf += (char)0xD7; //NOTHING?
	buf += (char)0xDC;//NOTHING?


	buf += (char)0x00;
	}

else{
	buf += (char)0x61;
	buf += (char)0x00;
	buf += (char)0x00;
	buf += (char)0x00;
	buf += (char)0x00;

	buf += (char)0x00; //seperator?

	buf += (char)10;
	buf += (char)10;
	buf += (char)10;
	buf += (char)10;

	std::string name="Ruediger";
	ADD2BYTE(buf, name.length());
	buf+=name;
	buf += (char)0x64;//SEPERATOR

	buf += (char)0x01; //FACING

	buf += (char)0x88;//FIX

	buf += (char)217;
	buf += (char)123;
	buf += (char)45;
	buf += (char)1;
	buf += (char)0x00; //NOTHING?

	buf += (char)0xD7; //NOTHING?
	buf += (char)0xDC;//NOTHING?


	buf += (char)0x00;
	//FIXME implement creatures
}
return buf;
}

//Parse methods


void TProt70::parseMoveNorth(Action* action, std::string msg){
	action->type=ACTION_MOVE;
	action->direction=0;
	player->lookdir=0;
}

void TProt70::parseMoveEast(Action* action, std::string msg){
	action->type=ACTION_MOVE;
	action->direction=1;
	player->lookdir=1;
}

void TProt70::parseMoveSouth(Action* action, std::string msg){
	action->type=ACTION_MOVE;
	action->direction=2;
	player->lookdir=2;
}

void TProt70::parseMoveWest(Action* action, std::string msg){
	action->type=ACTION_MOVE;
	action->direction=3;
	player->lookdir=3;
}

void TProt70::parseTurnNorth(Action* action, std::string msg){
	action->type=ACTION_TURN;
	action->direction=0;
	player->lookdir=0;
}

void TProt70::parseTurnEast(Action* action, std::string msg){
	action->type=ACTION_TURN;
	action->direction=1;
	player->lookdir=1;
}

void TProt70::parseTurnSouth(Action* action, std::string msg){
	action->type=ACTION_TURN;
	action->direction=2;
	player->lookdir=2;
}

void TProt70::parseTurnWest(Action* action, std::string msg){
	action->type=ACTION_TURN;
	action->direction=3;
	player->lookdir=3;
}

void TProt70::parseSetOutfit(Action* action, std::string msg){}

void TProt70::parseLogout(Action* action, std::string msg){
    // if this is a player then save the player's data
	std::cout << "Logging out" << std::endl;
    if( creature->isPlayer() )
    {
        // save the character before we logout
        this->player->save();
    }
	//we ask the map to remove us
	map->removeCreature(player->pos);
	//we ask the eventscheduler to disconnect us
	es.deletesocket(psocket);
	//we remove ourself
	delete this;
}

void TProt70::parseThrow(Action* action, std::string msg){
	action->type=ACTION_THROW;
	action->pos1.x=(unsigned char)msg[2]*256+(unsigned char)msg[1];
	action->pos1.y=(unsigned char)msg[4]*256+(unsigned char)msg[3];
	action->pos1.z=(unsigned char)msg[5];
	action->stack=(unsigned char)msg[8];
	//FIXME this sucks
	//make something like GETPOS
	action->pos2.x=(unsigned char)msg[10]*256+(unsigned char)msg[9];
	action->pos2.y=(unsigned char)msg[12]*256+(unsigned char)msg[11];
	action->pos2.z=(unsigned char)msg[13];

	if(msg.size()==15&&action->pos1.x!=0xFFFF)
		action->count=msg[14];
	std::cout << "count is " << action->count << std::endl;
	printf("From %i %i to %i %i", action->pos1.x, action->pos1.y, action->pos2.x, action->pos2.y);
	action->creature=this->creature;
	//just in case something happend to our inventory, update it
	sendInventory();
}

void TProt70::parseLookAt(Action* action, std::string msg){
	//TODO add stackpos
	action->type=ACTION_LOOK_AT;
	action->pos1.x=(unsigned char)msg[2]*256+(unsigned char)msg[1];
	action->pos1.y=(unsigned char)msg[4]*256+(unsigned char)msg[3];
	action->pos1.z=(unsigned char)msg[5];

	action->creature=this->creature;
}

void TProt70::parseSay(Action* action, std::string msg){
	//we should check if this was a command
	if(msg[4]=='!'){
		action->type=ACTION_NONE;
		position mypos=player->pos;
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
		  map->saveMap();
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
}

void TProt70::parseAttack(Action* action, std::string msg){
}

void TProt70::sendAction(Action* action){
	std::string buf = "  ";
	#ifdef __DEBUG__
	std::cout << "I got an action" << std::endl;
	#endif
	if(action->type==ACTION_SAY){
		buf+=(char)0xAA;
		ADD2BYTE(buf,action->playername.length());
		buf+=action->playername;
		buf+=(char)0x01;
		ADD2BYTE(buf, action->pos1.x);
		ADD2BYTE(buf, action->pos1.y);
		buf+=(char)action->pos1.z;
		ADD2BYTE(buf,action->buffer.length());
		buf+=action->buffer;
		buf[0]=(char)(buf.size()-2)%256;
		buf[1]=(char)((buf.size()-2)/256)%256;
		TNetwork::SendData(psocket,buf);
	}
	if(action->type==ACTION_TURN){
		sendPlayerTurn(action);
	}
	if(action->type==ACTION_MOVE){
		sendPlayerMove(action);
	}
	if(action->type==ACTION_LOGIN){
		sendPlayerLogin(action);
	}
	if(action->type==ACTION_LOGOUT){
		sendPlayerLogout(action);
	}
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
	if(action->type==ACTION_THROW){
		//this should not occur
		//throws are sent as 1 create and 1 delete
	}
	if(action->type==ACTION_LOOK_AT){
		this->sendPlayerLookAt(action->buffer);
	}
	//TODO free a;
}

void TProt70::sendPlayerMove(Action* action){
	//TODO rename this, it also handles items
	std::string buf = "  ";
	position distancenow=action->pos2 - player->pos;
	position distancewas=action->pos1 - player->pos;
	#ifdef __DEBUG__
	std::cout << "distancewas.x: " << distancewas.x <<std::endl;
	#endif
	#ifdef __DEBUG__
	std::cout << "distancewas.y: " << distancewas.y <<std::endl;
	#endif

	#ifdef __DEBUG__
	std::cout << "distancenow.x: " << distancenow.x <<std::endl;
	#endif
	#ifdef __DEBUG__
	std::cout << "distancenow.y: " << distancenow.y <<std::endl;
	#endif


	if((distancewas.x<=9&&distancenow.x >= 10)||(distancewas.y<=7&&distancenow.y>=8)||(distancewas.x>=-8&&distancenow.x <= -9)||(distancewas.y>=-6&&distancenow.y<=-7)){
	#ifdef __DEBUG__
	 std::cout << "PLAYER MOVED OUT" << std::endl;
	#endif
		buf += (char)0x6C;
		ADD2BYTE(buf, action->pos1.x);
		ADD2BYTE(buf, action->pos1.y);
		buf+=(char)action->pos1.z;
		buf += (char)action->stack;
	}
	else if((distancewas.x>=10&&distancenow.x<=9)||(distancewas.y>=8&&distancenow.y<=7)||(distancewas.x<=-10&&distancenow.x>=-9)||(distancewas.y<=-7&&distancenow.y>=-6) ){
	#ifdef __DEBUG__
	 std::cout << "PLAYER MOVED IN" << std::endl;
	#endif
		buf += (char)0x6A;
		ADD2BYTE(buf, action->pos2.x);
		ADD2BYTE(buf, action->pos2.y);
		buf+=(char)action->pos2.z;
		buf+=makeCreature(action->creature);

	}
	else{ //just a normal walk
		buf += (char)0x6D;
		ADD2BYTE(buf, action->pos1.x);
		ADD2BYTE(buf, action->pos1.y);
		buf+=(char)action->pos1.z;
		buf += (char)action->stack;
		ADD2BYTE(buf, action->pos2.x);
		ADD2BYTE(buf, action->pos2.y);
		buf+=(char)action->pos2.z;
		if(action->pos1==player->pos){
			//if we are the player that moved
			//we need to add a new map
			buf+=(char)(0x65+action->direction);

			if(action->direction==2){
				#ifdef __DEBUG__
				 std::cout << "Move to the south" << std::endl;
				 #endif
				buf+=makeMap(player->pos-position(8,-8,7),player->pos-position(-9,-8,7));
			}
			if(action->direction==3){
				#ifdef __DEBUG__
				 std::cout << "Move to the west" << std::endl;
				 #endif
				buf+=makeMap(player->pos-position(9,6,7),player->pos-position(9,-7,7));
			}
			if(action->direction==0){
				#ifdef __DEBUG__
				 std::cout << "Move to the north" << std::endl;
				  #endif
				buf+=makeMap(player->pos-position(8,7,7),player->pos-position(-9,7,7));
			}
			if(action->direction==1){
				#ifdef __DEBUG__
				 std::cout << "Move to the east" << std::endl;
				  #endif
				buf+=makeMap(player->pos-position(-10,6,7),player->pos-position(-10,-7,7));
			}

			player->pos=action->pos2;
			buf[buf.length()-2]=0x7E;
		}
	}

	buf[0]=(char)(buf.size()-2)%256;
	buf[1]=(char)((buf.size()-2)/256)%256;

	FILE* dump=fopen("dump","w+");
	for(unsigned int i=0; i< buf.length(); i++)
	fputc(buf[i],dump);
	fclose(dump);

	TNetwork::SendData(psocket,buf);
}

void TProt70::sendPlayerMoveIn(Action* action){
	//---
}

void TProt70::sendPlayerLogin(Action* action){
	if(creature==action->creature)
		return;
	std::string buf = "  ";
	buf+=(char)0x6a;
	ADD2BYTE(buf, action->pos1.x);
	ADD2BYTE(buf, action->pos1.y);
	buf+=(char)action->pos1.z;
	buf+=makeCreature(action->creature);
	buf+=(char)0x83;
	ADD2BYTE(buf, action->pos1.x);
	ADD2BYTE(buf, action->pos1.y);
	buf+=(char)action->pos1.z; //these lines might be the login bubble
	buf+=(char)0x0A;buf+=(char)0x8D;buf+=(char)0x5B;buf+=(char)0xE6;
	buf+=(char)0x09;buf+=(char)0x00;buf+=(char)0x00;buf+=(char)0xD7;
	buf[0]=(char)(buf.size()-2)%256;
	buf[1]=(char)((buf.size()-2)/256)%256;
	TNetwork::SendData(psocket,buf);
}
void TProt70::sendPlayerItemAppear(Action* action){
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
	TNetwork::SendData(psocket,buf);
}

void TProt70::sendPlayerItemDisappear(Action* action){
	std::string buf = "  ";
	buf+=(char)0x6C;
	ADD2BYTE(buf, action->pos1.x);
	ADD2BYTE(buf, action->pos1.y);
	buf+=(char)action->pos1.z;
	buf+=(char)action->stack;
	buf[0]=(char)(buf.size()-2)%256;
	buf[1]=(char)((buf.size()-2)/256)%256;
	TNetwork::SendData(psocket,buf);
}

void TProt70::sendPlayerLogout(Action* action){
	std::string buf = "  ";
	buf+=(char)0x6C;
	ADD2BYTE(buf, action->pos1.x);
	ADD2BYTE(buf, action->pos1.y);
	buf+=(char)action->pos1.z;
	buf+=(char)0x01;
	buf[0]=(char)(buf.size()-2)%256;
	buf[1]=(char)((buf.size()-2)/256)%256;
	TNetwork::SendData(psocket,buf);
}


void TProt70::sendPlayerSorry(){
	sendPlayerSorry("Sorry. Not possible");
}

void TProt70::sendPlayerSorry(tmapEnum){
	std::string msg = "Sorry. Not possible";
		std::string buf = "  ";
	buf+=(char)0xB4;
	buf+=(char)0x14;
	ADD2BYTE(buf, msg.size());
	buf+=msg;
	buf[0]=(char)(buf.size()-2)%256;
	buf[1]=(char)((buf.size()-2)/256)%256;
	TNetwork::SendData(psocket,buf);
}

void TProt70::sendPlayerSorry(std::string msg){
	std::string buf = "  ";
	buf+=(char)0xB4;
	buf+=(char)0x14;
	ADD2BYTE(buf, msg.size());
	buf+=msg;
	buf[0]=(char)(buf.size()-2)%256;
	buf[1]=(char)((buf.size()-2)/256)%256;
	TNetwork::SendData(psocket,buf);
}

void TProt70::sendPlayerLookAt(std::string msg){
	std::string buf = "  ";
	buf+=(char)0xB4;
	buf+=(char)0x13;
	ADD2BYTE(buf, msg.size());
	buf+=msg;
	buf[0]=(char)(buf.size()-2)%256;
	buf[1]=(char)((buf.size()-2)/256)%256;
	TNetwork::SendData(psocket,buf);
}

void TProt70::sendPlayerChangeGround(Action* action){
	std::string buf = "  ";
	buf+=(char)0x6B;
	ADD2BYTE(buf, action->pos1.x);
	ADD2BYTE(buf, action->pos1.y);
	buf+=(char)action->pos1.z;
	buf+=(char)0x00;
	ADD2BYTE(buf,action->id);
	buf[0]=(char)(buf.size()-2)%256;
	buf[1]=(char)((buf.size()-2)/256)%256;
	TNetwork::SendData(psocket,buf);
}

void TProt70::sendPlayerItemChange(Action* action){
	std::string buf = "  ";
	buf+=(char)(0x6B);
	ADDPOS(buf, action->pos1);
	buf+=(char)(0x01);//stack?
	ADD2BYTE(buf,action->id);
	ADD1BYTE(buf,action->count);
	buf[0]=(char)(buf.size()-2)%256;
	buf[1]=(char)((buf.size()-2)/256)%256;
	TNetwork::SendData(psocket,buf);
}

void TProt70::sendPlayerTurn(Action* action){
	sendInventory();
	std::string buf = "  ";
	buf+=(char)(0x6B);
	ADDPOS(buf, action->pos1);
	buf+=(char)(0x01);//stack?
	buf+=(char)(0x63);
	buf+=(char)(0x00);
	ADD4BYTE(buf,action->creature->id);
	buf+=(char)(action->direction);
	buf[0]=(char)(buf.size()-2)%256;
	buf[1]=(char)((buf.size()-2)/256)%256;
	TNetwork::SendData(psocket,buf);
}

int TProt70::sendInventory(){
	std::string buf = "  ";

	for(int i=1; i < 11; i++){
		if(creature->getItem(i)!=NULL){
			buf+= (char)0x78;//backpack slot
			buf+= (char)(i);
			buf+=makeItem(creature->getItem(i));
		}
		else{
			buf+= (char)0x79;//backpack slot
			buf+= (char)(i);
		}
	}


	buf[0]=(char)(buf.size()-2)%256;
	buf[1]=(char)((buf.size()-2)/256)%256;
	TNetwork::SendData(psocket,buf);
	return true;
}

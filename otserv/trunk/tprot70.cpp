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
					std::cerr << "logout" << std::endl;
					es.deletesocket(sock);
					close(sock);
				} else {  // lesen erfolgreich
					buffer[nbytes] = 0;
					std::cout << "read" << std::endl;
					//TODO message bearbeiten
					std::string s= std::string(buffer, nbytes);;
					parsePacket(s);
								  printf("%s\n", buffer);
				}
			}

			void TProt70::parsePacket(std::string msg){
			msg.erase(0,2);
				//i just hope CIP was intelligent enough to make the 3. byte unique
				//i guess they were not
				Action* action= new Action;
				action->pos1=player->pos;
				std::cout << "byte is " << (int)msg[0]<<std::endl;


				switch( msg[0] ){
					case 0x14: //logout TODO
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
					case 0x6F: //turn north TODO
					parseTurnNorth(action, msg);
					break;
					case 0x70: //turn east TODO
					parseTurnEast(action, msg);
					break;
					case 0x71: //turn south TODO
					parseTurnSouth(action, msg);
					break;
					case 0x72: //turn west TODO
					parseTurnWest(action, msg);
					break;
					case 0x96: //say something
					parseSay(action, msg);
					break;
					case 0xDB: //set outfit TODO
					parseSetOutfit(action, msg);
					break;
				}
				//so we got the action, now we ask the map to execut it
				if(action->type!=ACTION_NONE){
					map->requestAction(creature,action);
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

	//		ADD4BYTE(buf,365137);

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


			std::cout << "x: " << pos.x << " y: " << pos.y <<std::endl;
			std::string buf2 = makeMap(position(pos.x-8,pos.y-6,pos.z),position(pos.x+9,pos.y+7,pos.z));

			buf2.resize(buf2.length()-2);
			buf += buf2;
			buf += (char)0xFF;buf += (char)0xFF;buf += (char)0xFF;buf += (char)0xFF;
			buf += (char)0xFF;buf += (char)0xFF;buf += (char)0xFF;buf += (char)0xFF;
			buf += (char)0xFF;buf += (char)0xFF;buf += (char)0xFF;buf += (char)0xFF;

			buf += (char)0xE4; // TODO Light level

			buf += (char)0xFF;

			buf += 0x83;
			buf += (char)(pos.x%256);
			buf += (char)(pos.x/256)%256;
			buf += (char)(pos.y%256);
			buf += (char)(pos.y/256)%256;
			buf += (char)pos.z;


			buf+= (char)0x0A; //ITEMS

			//4 bytes:
			//0x78 0xXX position
			//2 bytes what

			buf+= (char)0x78;//backpack slot
			buf+= (char)0x03;
			buf+= (char)0x82;//backpack
			buf+= (char)0x05;

			buf+= (char)0x78; //armor slot
			buf+= (char)0x04;
			buf+= (char)0x59; //jacket
			buf+= (char)0x07;

			buf+= (char)0x78; //left hand
			buf+= (char)0x05;
			buf+= (char)0x89; //club
			buf+= (char)0x06;

			buf+= (char)0x78; //right hand
			buf+= (char)0x06;
			buf+= (char)0xB2; //torch
			buf+= (char)0x05;


			buf+= (char)0xA0; //STATS

			ADD2BYTE(buf,150);//hitpoints
			ADD2BYTE(buf,150);//hitpoints
			ADD2BYTE(buf,300);//cap
			ADD4BYTE(buf,3000); //experience
			buf+=(char)0x0F;//level
			ADD2BYTE(buf,0);//mana
			ADD2BYTE(buf,0);//mana
			buf+=(char)0x0F;//maglevel
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
			buf+= (char)0x0A; //fist
			buf+= (char)0x0A; //club
			buf+= (char)0x0A; //sword
			buf+= (char)0x0A; //axe
			buf+= (char)0x0A; //dist
			buf+= (char)0x0A; //shield
			buf+= (char)0x0A; //fishing

			buf+= (char)0xB4;
			buf+= (char)0x11;

			//TODO richtiger motd support
			std::string welcomemsg="Welcome to the OTServ-County. Come to where the segfaults are.";
			ADD2BYTE(buf,welcomemsg.length());
			buf+= welcomemsg;

			// now we correct the first two bytes which corespond to the length
			// of the packet


			buf[0]=(char)(buf.size()-2)%256;
			buf[1]=(char)((buf.size()-2)/256)%256;

		/*	FILE* dump=fopen("dump","w+");
			for(int i=0; i< buf.length(); i++)
			fputc(buf[i],dump);
			fclose(dump);
*/
			// and send to client...
			TNetwork::SendData(psocket,buf);

		} // void TProt70::setMap(position newpos) throw(texception)

		std::string TProt70::makeMap(position topleft, position botright) {
			int xswap=1, yswap=1;
			position dif=botright-topleft;
			/*if(topleft.x > botright.x)
				swap(topleft.x, botright.x);
			if(topleft.y > botright.y)
				swap(topleft.y, botright.y);*/
			if(dif.x<0)
				xswap=-1;
			if(dif.y<0)
				yswap=-1;
			std::string buf;
			Tile* tile;
			/*FILE* dmp=fopen("map","r");
			int a;
			while((a=fgetc(dmp))!=EOF)
			buf+=(char)a;
			fclose(dmp);
*/
	std::cout << "   Yswap: " << yswap << "   Xswap: " << xswap << std::endl;
	std::cout << "   Topleft: " << topleft.x << "   botright: " << botright.x << std::endl;
// buf+=makeMap(player->pos-position(-8,7,7),player->pos-position(9,7,7));
			std::cout << topleft.y << "\t" << botright.y <<std::endl;
			// we just add the tilecode for every tile...
			std::string rowbuf, colbuf, tmpbuf;

			for (unsigned short i=topleft.x; i*xswap<=botright.x*xswap; i+=xswap) {
				std::cout << "," <<std::endl;
				for (unsigned short j=topleft.y; j*yswap<=botright.y*yswap; j+=yswap) {
					std::cout << ".";
					tile=map->tile(i,j,topleft.z);
					for (Item::iterator it=tile->begin(); it != tile->end(); it++) {
						std::cout << "-";
						ADD2BYTE(buf, (*it)->getID());
					}
					if(tile->getCreature()!=NULL)
					buf += makeCreature(tile->getCreature());
					buf+=(char)0x00; // no special thing
					buf += (char)0xFF; // tile end
				} //for (unsigned short j=topleft.y; j<=botright.y; j++)
			}//for (unsigned short i=topleft.x; i<=botright.x; i++)


			return buf;



			} // std::string TProt70::makeMap(const position& topleft, const position& botright)

		std::string TProt70::makeCreature(Creature* c){
			std::string buf;

			if(c->isPlayer())
			{
				player_mem p=((Player*)c)->player;
				//PLAYER??
				buf += (char)0x61;
				buf += (char)0x00;
				buf += (char)0x00;
				buf += (char)0x00;
				buf += (char)0x00;

				buf += (char)0x00; //seperator?

				buf += (char)(p.pnum%256);
				buf += (char)(p.pnum/256)%256;
				buf += (char)(p.pnum/(256*256))%256;
				buf += (char)(p.pnum/(256*256*256))%256;

				ADD2BYTE(buf, p.name.length());
				buf+=p.name;
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
			}



			return buf;
		}

//Parse methods


void TProt70::parseMoveNorth(Action* action, std::string msg){
	action->type=ACTION_MOVE;
	action->direction=0;
}

void TProt70::parseMoveEast(Action* action, std::string msg){
	action->type=ACTION_MOVE;
	action->direction=1;
}

void TProt70::parseMoveSouth(Action* action, std::string msg){
	action->type=ACTION_MOVE;
	action->direction=2;
}

void TProt70::parseMoveWest(Action* action, std::string msg){
	action->type=ACTION_MOVE;
	action->direction=3;
}

void TProt70::parseTurnNorth(Action* action, std::string msg){
	action->type=ACTION_TURN;
	action->direction=0;
}

void TProt70::parseTurnEast(Action* action, std::string msg){
	action->type=ACTION_TURN;
	action->direction=1;
}

void TProt70::parseTurnSouth(Action* action, std::string msg){
	action->type=ACTION_TURN;
	action->direction=2;
}

void TProt70::parseTurnWest(Action* action, std::string msg){
	action->type=ACTION_TURN;
	action->direction=3;
}

void TProt70::parseSetOutfit(Action* action, std::string msg){}

void TProt70::parseLogout(Action* action, std::string msg){
	//we ask the map to remove us
	map->removeCreature(pos, creature);
	//we ask the eventscheduler to disconnect us
	es.deletesocket(psocket);
	//we remove ourself
	delete this;
}

void TProt70::parseSay(Action* action, std::string msg){
	action->type=ACTION_SAY;
	action->playername=player->name;
	msg.erase(0,4);
	action->buffer=msg;
	action->pos1=player->pos;
}

void TProt70::sendAction(Action* action){
	std::string buf = "  ";
	std::cout << "I got an action" << std::endl;
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
}

void TProt70::sendPlayerMove(Action* action){
	std::string buf = "  ";
	position distancenow=action->pos2 - player->pos;
	position distancewas=action->pos1 - player->pos;
	std::cout << "distancewas.x: " << distancewas.x <<std::endl;
	std::cout << "distancewas.y: " << distancewas.y <<std::endl;

	std::cout << "distancenow.x: " << distancenow.x <<std::endl;
	std::cout << "distancenow.y: " << distancenow.y <<std::endl;


	if((distancewas.x<=9&&distancenow.x >= 10)||(distancewas.y<=7&&distancenow.y>=8)||(distancewas.x>=-8&&distancenow.x <= -9)||(distancewas.y>=-6&&distancenow.y<=-7)){
	std::cout << "PLAYER MOVED OUT" << std::endl;
		buf += (char)0x6C;
		ADD2BYTE(buf, action->pos1.x);
		ADD2BYTE(buf, action->pos1.y);
		buf+=(char)action->pos1.z;
		buf += (char)0x01;
	}
	else if((distancewas.x>=10&&distancenow.x<=9)||(distancewas.y>=8&&distancenow.y<=7)||(distancewas.x<=-10&&distancenow.x>=-9)||(distancewas.y<=-7&&distancenow.y>=-6) ){
	std::cout << "PLAYER MOVED IN" << std::endl;
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
		buf += (char)0x01;
		ADD2BYTE(buf, action->pos2.x);
		ADD2BYTE(buf, action->pos2.y);
		buf+=(char)action->pos2.z;
		if(action->pos1==player->pos){
			//if we are the player that moved
			//we need to add a new map
			buf+=(char)(0x65+action->direction);

			if(action->direction==2){
				std::cout << "Move to the south" << std::endl;
				buf+=makeMap(player->pos-position(8,-8,7),player->pos-position(-9,-8,7));
			}
			if(action->direction==3){
				std::cout << "Move to the west" << std::endl;
				buf+=makeMap(player->pos-position(9,6,7),player->pos-position(9,-7,7));
			}
			if(action->direction==0){
				std::cout << "Move to the north" << std::endl;
				buf+=makeMap(player->pos-position(8,7,7),player->pos-position(-9,7,7));
			}
			if(action->direction==1){
				std::cout << "Move to the east" << std::endl;
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

void TProt70::sendPlayerTurn(Action* action){
	std::string buf = "  ";
	buf+=(char)(0x6F+action->direction);
	buf[0]=(char)(buf.size()-2)%256;
	buf[1]=(char)((buf.size()-2)/256)%256;
	TNetwork::SendData(psocket,buf);
}


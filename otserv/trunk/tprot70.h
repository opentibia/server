//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// The protocoll for the clients up to version 6.4
// the newer clients still use the same protocoll just with a 
// prequel defined in the 6.5 protocoll
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
// $Id$
//////////////////////////////////////////////////////////////////////
// $Log$
// Revision 1.5  2003/09/23 20:00:51  tliffrag
// added !g command
//
// Revision 1.4  2003/09/17 16:35:08  tliffrag
// added !d command and fixed lag on windows
//
// Revision 1.2  2003/08/26 21:09:53  tliffrag
// fixed maphandling
//
// Revision 1.1  2003/05/19 16:51:20  tliffrag
// Forgot some files, blame it on cvs add
//
// Revision 1.6  2002/05/29 16:07:38  shivoc
// implemented non-creature display for login
//
// Revision 1.5  2002/05/28 13:55:57  shivoc
// some minor changes
//
//
//////////////////////////////////////////////////////////////////////

#ifndef tprot70_h
#define tprot70_h

#include "protokoll.h"
#include "creature.h"
#include "texcept.h"
#include "action.h"
#include <string>

#define ADD4BYTE(stream, val) (stream) += (char)((val)%256); \
(stream) += (char)(((val)/256)%256); \
(stream) += (char)(((val)/(256*256))%256); \
(stream) += (char)(((val)/(256*256*256))%256);

#define ADD2BYTE(stream, val) (stream) += (char)((val)%256); \
(stream) += (char)(((val)/256)%256);

#define ADDPOS(stream,pos) (stream)+=(char)((pos.x)%256); \
(stream)+=(char)((pos.x)/256);(stream)+=(char)((pos.y)%256);\
(stream)+=(char)((pos.y)/256);(stream)+=(char)((pos.z));\

    class TProt70 : public Protokoll {

        public:

            // our constructor get's the socket of the client and the initial
            // message the client sent
            TProt70(const Socket&, const std::string&) throw(texception);

            // set the map and update the client screen
            void setMap(position, Map&) throw(texception);

            // virtual methods form out base class...
            const std::string getName() const throw();
            const std::string getPassword() const throw();
			std::string TProt70::makeMap(const position topleft, const position botright);

            void clread(const Socket& sock) throw();
			void sendAction(Action* action);
            // our destructor to clean up the mess we made...
            ~TProt70() throw();

        private:
			//First, we have all the parse methods
			void parseMoveNorth(Action* action, std::string msg);
			void parseMoveEast(Action* action, std::string msg);
			void parseMoveSouth(Action* action, std::string msg);
			void parseMoveWest(Action* action, std::string msg);
			void parseTurnNorth(Action* action, std::string msg);
			void parseTurnEast(Action* action, std::string msg);
			void parseTurnSouth(Action* action, std::string msg);
			void parseTurnWest(Action* action, std::string msg);
			void parseSetOutfit(Action* action, std::string msg);
			void parseSay(Action* action, std::string msg);
			void parseLogout(Action* action, std::string msg);
			void parseThrow(Action* action, std::string msg);

			void sendPlayerMove(Action* action);
			void sendPlayerLogin(Action* action);
			void sendPlayerLogout(Action* action);
			void sendPlayerMoveIn(Action* action);
			void sendPlayerTurn(Action* action);
			void sendPlayerItemAppear(Action* action);
			void sendPlayerItemDisappear(Action* action);
			void sendPlayerChangeGround(Action* action);
            // translate a map area to clientreada format
            // uses the map the client is on
 //           std::string makeMap(const position topleft,
  //                  bool reverse);

			void TProt70::parsePacket(std::string);

			std::string makeCreature(Creature*);
            // the socket the player is on...
            Socket psocket;
            // the os of the client...
            unsigned char clientos;
            // version of the client
            unsigned char version;
            // name and passwd
            std::string name, passwd;
            // the position on the map...
            position pos;
            Map* map;


    }; // class TProt70 : public Protokoll  



#endif

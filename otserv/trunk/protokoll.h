//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// base class to be implemented by each protocoll to use
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
// Revision 1.10  2003/11/03 12:16:01  tliffrag
// started walking by mouse
//
// Revision 1.9  2003/10/21 17:55:07  tliffrag
// Added items on player
//
// Revision 1.8  2003/09/18 12:35:22  tliffrag
// added item dragNdrop
//
// Revision 1.7  2003/05/19 16:48:37  tliffrag
// Loggingin, talking, walking around, logging out working
//
// Revision 1.6  2002/05/29 16:07:38  shivoc
// implemented non-creature display for login
//
// Revision 1.5  2002/05/28 13:55:56  shivoc
// some minor changes
//
//
//////////////////////////////////////////////////////////////////////

#ifndef protokoll_h
#define protokoll_h

#include "definitions.h"
#include "tmap.h"
#include "texcept.h"
#include "creature.h"
#include "player_mem.h"
#include <string>

   // virtual base class to represent different protokolls...
    class Protokoll {
        public:

            Protokoll() throw();

            // our callback interface
            struct clientread : public unary_functor<Socket,void> {
                Protokoll& base;
                clientread(Protokoll& prot) : base(prot) { };
                void operator() (const Socket &sock) {
                    base.clread(sock);
                }
            };

            // set the playerdata to use
            void setdata(player_mem&);
			void setCreature(Creature* c);
			virtual void sendAction(Action* a){}
			virtual int sendInventory(){return 0;}
			virtual int doAction(Action*){return 0;};

            // get the name...
            virtual const std::string getName() const throw() =0;

            // and the password...
            virtual const std::string getPassword() const throw() =0;

            // virtual destructor
            virtual ~Protokoll() throw() {};

            // callback if data arives on our socket
            clientread cread;

            virtual void clread(const Socket &sock) = 0;

            // set the map and update the client screen
            virtual void setMap(position, Map&) throw(texception)=0;


        protected:
            player_mem* player;
            Creature* creature;
    };

    // class to choose the protokoll which acts like a pointer to the correct
    // protokoll or throws an exception if none is aplicable...
    class ProtokollP {
        public:
            // the constructor which choses the socket...
            ProtokollP(const Socket&) throw(texception);

            // resemble pointer usage...
            Protokoll* operator->() const throw() {
                return prot;
            }

            // our destructor...
            ~ProtokollP() throw();

        private:
            Protokoll* prot;
    };



#endif

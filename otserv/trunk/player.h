	//////////////////////////////////////////////////////////////////////
	// OpenTibia - an opensource roleplaying game
	//////////////////////////////////////////////////////////////////////
	// 
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
	// Revision 1.13  2004/11/20 14:56:28  shivoc
	// started adding haktivex battlesystem; fixed some bugs; changed serveroutput
	//
	// Revision 1.12  2003/11/05 23:28:23  tliffrag
	// Addex XML for players, outfits working
	//
	// Revision 1.11  2003/11/03 22:48:14  tliffrag
	// Changing look, walking by mouse working
	//
	// Revision 1.10  2003/11/03 12:16:01  tliffrag
	// started walking by mouse
	//
	// Revision 1.9  2003/10/21 17:55:07  tliffrag
	// Added items on player
	//
	// Revision 1.8  2003/09/25 21:17:52  timmit
	// Adding PlayerList in TMap and getID().  Not workigng!
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

	#ifndef __player_h_
	#define __player_h_

	#include "definitions.h"
	#include "creature.h"
	#include "protokoll.h"
	#include "player_mem.h"

	#include <string>

	//////////////////////////////////////////////////////////////////////
	// Defines a player...
	class Player : public Creature {

		public:

		// our constructor
		Player(const Socket&);

		// virtual destructor to be overloaded...
		virtual ~Player();

		bool isPlayer();

		int clearActionQueue();

		std::list<Action*> actionQueue;
		int addAction(Action* a){ actionQueue.push_back(a); return 0;};

		unsigned long getID();

		virtual position getPosition();

		void sendAction(Action*);

		int addItem(Item* item, int pos);
		int sendInventory();

		Item* getItem(int pos);

		std::string getLook();
		std::string getName(){return name;};

		unary_functor<Socket,void>* cb() {
			return &client->cread;
		}

		int tick(double time);
		void setMap(position,Map&) throw(texception);

	//	private:
		// pointer to the protokoll...
		ProtokollP client;

		// the data of our player
		player_mem player;

		// we need our name and password...
		std::string name, password;
	};


	#endif // __player_h_

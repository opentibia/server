//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// protocoll chooser
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
// Revision 1.10  2003/05/19 16:48:37  tliffrag
// Loggingin, talking, walking around, logging out working
//
// Revision 1.9  2002/08/01 14:11:28  shivoc
// added initial support for 6.9x clients
//
// Revision 1.8  2002/05/29 16:07:38  shivoc
// implemented non-creature display for login
//
// Revision 1.7  2002/05/28 13:55:56  shivoc
// some minor changes
//
//
//////////////////////////////////////////////////////////////////////

#include "protokoll.h"
#include "tprot.h"	// client <= 6.4 and 6.5+ clients after redirect
#include "tprot65.h" // v6.5+ clients
#include "tprot69.h" // 6.9+ clients
#include "tprot70.h" // 7.0+ clients
#include "network.h"
#include <iostream>

  Protokoll::Protokoll() throw() : cread(*this) { };
  
  void Protokoll::setdata(player_mem& pm) {
    player=&pm;
  }

void Protokoll::setCreature(Creature* c){
	creature=c;
}

  ProtokollP::ProtokollP(const Socket& psocket) throw(texception) {
    
    // we try to read from our socket...
    std::string buf=TNetwork::ReceiveData(psocket);

     
    // now we need to find out the protokoll...
    try {
      // if the protokoll is the tibia protokoll for 6.9+ clients 
      // there will be no
      // exception, else there will be one and we need to check the next
      // protokoll...
      
      prot = new TProt69(psocket, buf);
      return;
    }
    catch (texception e) {
      // so it's not the tibia protokoll...
      if (e.isCritical()) {
	throw texception(e.toString(),false);;
      } // if (e.isCritical()) 
    }


    try {
      // if the protokoll is the tibia protokoll for 7.0+ clients 
      // there will be no
      // exception, else there will be one and we need to check the next
      // protokoll...
      prot = new TProt70(psocket, buf);
      return;
    }
    catch (texception e) {
      // so it's not the tibia protokoll...
      std::cout <<e.toString()<<"AAAAAAAAAA"<< std::endl;
      if (e.isCritical()) {
 	std::cout << e.toString() << std::endl;
	throw texception(e.toString(),false);
      } // if (e.isCritical()) 
    }
    
    // other protokolls insert here...
    
    // since it's none of the protokolls above we throw an exception...
    throw texception("no protokoll found!", false);
  }
  
  ProtokollP::~ProtokollP() throw() {
    delete prot;
  }

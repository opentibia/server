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
#include "network.h"

namespace Protokoll {

    Protokoll::Protokoll() throw() : cread(*this) { };

    void Protokoll::setdata(Creatures::player_mem& pm) {
        player=&pm;
    }

    ProtokollP::ProtokollP(const Socket& psocket) throw(texception) {

        // we try to read from our socket...
        std::string buf=TNetwork::ReceiveData(psocket);

        // now we need to find out the protokoll...
        try {
            // if it's the tibia protokoll 6.5+ the client get's redirected
            // back to the server for the second login step...
            // and an exception get's thrown...
            prot = new TProt65(psocket, buf);
        }
        catch (texception e) {
            // if it's critical it's the 6.5+ protokoll...
            if (e.isCritical()) {
                throw texception(e.toString(),false);
            } // if (e.isCritical()) 
        }

        try {
            // if the protokoll is the tibia protokoll there will be no
            // exception, else there will be one and we need to check the next
            // protokoll...
            prot = new TProt(psocket, buf);
            return;
        }
        catch (texception e) {
            // so it's not the tibia protokoll...
            if (e.isCritical()) {
                throw;
            } // if (e.isCritical()) 
        }

        // other protokolls insert here...

        // since it's none of the protokolls above we throw an exception...
        throw texception("no protokoll found!", false);
    }

    ProtokollP::~ProtokollP() throw() {
        delete prot;
    }

} // namespace Protokoll 

/*******************************************************************************
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#ifndef tprot_h
#define tprot_h

#include "protokoll.h"
#include "texcept.h"
#include <string>

namespace Protokoll {
	class TProt : public Protokoll {

		public:

			// our constructor get's the socket of the client and the initial
			// message the client sent
			TProt(const SOCKET&, const string&) throw(texception);

			// set the map and update the client screen
//			void setMap(Map::mapposition) throw(texception);

			// virtual methods form out base class...
			const std::string getName() const throw();
			const std::string getPassword() const throw();

			void clread(const SOCKET& sock) throw();

			// our destructor to clean up the mess we made...
			~TProt() throw();

		private:

			// the socket the player is on...
			SOCKET psocket;
			// the os of the client...
			unsigned char clientos;
			// version of the client
			unsigned char version;
			// name and passwd
			std::string name, passwd;
			// the position on the map...
//			Map::mapposition our_pos;

	}; // class TProt : public Protokoll  

}

#endif

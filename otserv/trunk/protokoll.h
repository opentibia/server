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

#ifndef protokoll_h
#define protokoll_h

#include "definitions.h"
#include "map.h"
#include "texcept.h"
#include <string>

namespace Protokoll {

	// virtual base class to represent different protokolls...
	class Protokoll {
		public:

			Protokoll() throw();
			
			// our callback interface
			struct clientread : public unary_functor<int ,void> {
				Protokoll& base;
				clientread(Protokoll& prot) : base(prot) { };
				void operator() (const Socket &sock) {
					base.clread(sock);
				}
			};


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
			//			virtual void setMap(Map::mapposition) throw(texception) =0;


		protected:
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

}

#endif

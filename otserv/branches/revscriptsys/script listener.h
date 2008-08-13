//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
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

#ifndef __OTSERV_SCRIPT_EVENT_LISTENER_H__
#define __OTSERV_SCRIPT_EVENT_LISTENER_H__

#include "boost_common.h"
#include <boost/any.hpp>

namespace Script {
	class Manager;

	///////////////////////////////////////////////////////////////////////////////
	// Event Listener

	class Listener {
	public:
		// Listener MUST be destroyed before the manager
		Listener(const std::string& name, const boost::any& data, Manager& manager);
		~Listener();

		std::string getLuaTag() const;
		const boost::any& getData() const;
		
		bool isActive() const {return active;}
		void deactive();

	protected:
		static uint32_t ID_counter;
		uint32_t ID;
		bool active;
		std::string name;
		std::string datatag;
		boost::any data;
		Manager& manager;
	};

	typedef boost::shared_ptr<Listener> Listener_ptr;
	typedef boost::weak_ptr<Listener> Listener_wptr;

	inline std::string Listener::getLuaTag() const {
		return datatag;
	}

	inline const boost::any& Listener::getData() const {
		return data;
	}
}

#endif // __OTSERV_SCRIPT_EVENT_LISTENER_H__
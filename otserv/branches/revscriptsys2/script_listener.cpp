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

#include "otpch.h"

#include "script_listener.h"
#include "script_manager.h"
#include <sstream>

uint32_t Script::Listener::ID_counter = 0;

using namespace Script;

Listener::Listener(ListenerType t, const boost::any& data, Manager& manager) :
	ID(++ID_counter),
	active(true),
	type_(t),
	data(data),
	manager(manager)
{
	std::ostringstream os;
	os << "Listener_" << type_.value() << "_" << ID;
	datatag = os.str();
}

Listener::~Listener() {
}

void Listener::deactivate() {
	active = false;
	manager.pushNil();
	manager.setRegistryItem(getLuaTag());
}

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

#ifndef __OTSERV_CONNECTION_H__
#define __OTSERV_CONNECTION_H__

#include "definitions.h"
#include "networkmessage.h"

#include <asio.hpp>

class Protocol;

class Connection{
public:
	Connection(asio::io_service& io_service) : socket(io_service) { };
	~Connection() {}

	const asio::ip::tcp::socket& getHandle() const { return socket;}

	void closeConnection();
	void acceptConnection();
	void send(char* data, uint32_t size);
	
	void parseHeader(const asio::error& error);
	void parsePacket(const asio::error& error);

private:
	NetworkMessage msg;
	asio::ip::tcp::socket socket;
	Protocol* protocol;
};

#endif

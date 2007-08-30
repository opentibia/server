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

#ifndef __OTSERV_SERVER_H__
#define __OTSERV_SERVER_H__

#include "definitions.h"
#include "connection.h"

#include <boost/bind.hpp>
#include <boost/asio.hpp>

class Server
{
public:
	Server(boost::asio::io_service& io_service, uint32_t serverip, uint16_t port)
		: acceptor(io_service, 
		 boost::asio::ip::tcp::endpoint(
		 	boost::asio::ip::address(boost::asio::ip::address_v4(serverip)), 
			port))
	{
		accept();
	}
	
	~Server() { }

private:
	void accept()
	{
		Connection* connection = new Connection(acceptor.io_service());

		acceptor.async_accept(connection->getHandle(),
			boost::bind(&Server::onAccept, this, connection, 
			boost::asio::placeholders::error));
	}

	void onAccept(Connection* connection, const boost::asio::error& error)
	{
		if(!error){
			connection->acceptConnection();
			accept();
		}
	}

	boost::asio::ip::tcp::acceptor acceptor;
};

#endif

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

using boost;
using boost::asio::ip;

class server
{
public:
	server(asio::io_service& io_service, uint16_t port)
		: acceptor(io_service, tcp::endpoint(tcp::v4(), port))
	{
		accept();
	}

private:
	void accept()
	{
		Connection* connection = new Connection(acceptor.io_service());

		acceptor.async_accept(connection->getHandle(),
			boost::bind(&server::onAccept, this, connection, 
			asio::placeholders::error));
	}

	void onAccept(Connection* connection, const asio::error& error)
	{
		if(!error){
			connection->acceptConnection();
			accept();
		}
	}

	tcp::acceptor acceptor;
};

#endif

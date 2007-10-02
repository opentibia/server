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
#include "otpch.h"

#include "server.h"
#include "connection.h"

void Server::accept()
{
	//Connection* connection = new Connection(m_io_service);
	Connection* connection = ConnectionManager::getInstance()->createConnection(m_io_service);

	m_acceptor.async_accept(connection->getHandle(),
		boost::bind(&Server::onAccept, this, connection, 
		boost::asio::placeholders::error));
}

void Server::onAccept(Connection* connection, const boost::system::error_code& error)
{
	if(!error){
		connection->acceptConnection();
		#ifdef __DEBUG_NET_DETAIL__
		std::cout << "accept - OK" << std::endl;
		#endif
		accept();
	}
	else{
		PRINT_ASIO_ERROR("Accepting");
	}
}

void Server::stop()
{
	m_io_service.post(boost::bind(&Server::onStopServer, this));
}

void Server::onStopServer()
{
	m_acceptor.close();
	ConnectionManager::getInstance()->closeAll();
}

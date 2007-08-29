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
#include <boost/asio.hpp>

#include "networkmessage.h"

class Protocol;
class OutputMessage;

class Connection
{
public:
	Connection(boost::asio::io_service& io_service) : m_socket(io_service)
	{ 
		m_protocol = NULL;
		m_pendingWrite = 0;
	}
	
	~Connection() {}

	const boost::asio::ip::tcp::socket& getHandle() const { return m_socket; }

	void closeConnection();
	void acceptConnection();
	
	void send(OutputMessage* msg);
	
	void parseHeader(const boost::asio::error& error);
	void parsePacket(const boost::asio::error& error);

	void onWriteOperation(const boost::asio::error& error);
	
private:
	NetworkMessage m_msg;
	boost::asio::ip::tcp::socket m_socket;
	
	int32_t m_pendingWrite;
	
	Protocol* m_protocol;
};

#endif

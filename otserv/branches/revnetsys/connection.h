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

#include "otsystem.h"

#include "networkmessage.h"

class Protocol;
class OutputMessage;

class Connection
{
public:
	enum {
		CLOSE_STATE_NONE = 0,
		CLOSE_STATE_REQUESTED = 1,
		CLOSE_STATE_CLOSING = 2,
	};
	
	Connection(boost::asio::io_service& io_service) : m_socket(io_service)
	{ 
		m_protocol = NULL;
		m_pendingWrite = 0;
		m_pendingRead = 0;
		m_CloseState = CLOSE_STATE_NONE;
		m_socketClosed = false;
		OTSYS_THREAD_LOCKVARINIT(m_connectionLock);
	}
	
	~Connection()
	{
		OTSYS_THREAD_LOCKVARRELEASE(m_connectionLock);
	}

	boost::asio::ip::tcp::socket& getHandle() { return m_socket; }

	void closeConnection();
	void acceptConnection();
	
	void send(OutputMessage* msg);
	
	uint32_t getIP();
	
private:
	void parseHeader(const boost::asio::error& error);
	void parsePacket(const boost::asio::error& error);

	friend class OutputMessagePool;
	void onWriteOperation(const boost::asio::error& error);
	
	void closeConnectionTask();
	void closingConnection();
	
	NetworkMessage m_msg;
	boost::asio::ip::tcp::socket m_socket;
	bool m_socketClosed;
	
	int32_t m_pendingWrite;
	int32_t m_pendingRead;
	uint32_t m_CloseState;
	
	OTSYS_THREAD_LOCKVAR m_connectionLock;
	
	Protocol* m_protocol;
};

#endif

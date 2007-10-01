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

#include <boost/utility.hpp>

#include "otsystem.h"

#include "networkmessage.h"

class Protocol;
class OutputMessage;
class Connection;

#ifdef __DEBUG_NET__
#define PRINT_ASIO_ERROR(desc) \
	std::cout << "Error: [" << __FUNCTION__ << "] " << desc << " - Error: " <<  \
		error.value() << " Desc: " << error.message() << std::endl;
#else
#define PRINT_ASIO_ERROR(desc)
#endif

class ConnectionManager
{
public:
	~ConnectionManager()
	{
		OTSYS_THREAD_LOCKVARRELEASE(m_connectionManagerLock);
	}
	
	static ConnectionManager* getInstance(){
		static ConnectionManager instance;
		return &instance;
	}
	
	Connection* createConnection(boost::asio::io_service& io_service);
	void releaseConnection(Connection* connection);
	void closeAll();
	
protected:
	
	ConnectionManager()
	{
		OTSYS_THREAD_LOCKVARINIT(m_connectionManagerLock);
	}
	
	std::list<Connection*> m_connections;
	OTSYS_THREAD_LOCKVAR m_connectionManagerLock;
};

class Connection : boost::noncopyable
{
public:
	enum {
		CLOSE_STATE_NONE = 0,
		CLOSE_STATE_REQUESTED = 1,
		CLOSE_STATE_CLOSING = 2,
	};
	
private:
	Connection(boost::asio::io_service& io_service) : m_socket(io_service)
	{
		m_protocol = NULL;
		m_pendingWrite = 0;
		m_pendingRead = 0;
		m_closeState = CLOSE_STATE_NONE;
		m_socketClosed = false;
		OTSYS_THREAD_LOCKVARINIT(m_connectionLock);
		m_writeError = false;
		m_readError = false;
	}
	friend class ConnectionManager;
	
public:
	~Connection()
	{
		ConnectionManager::getInstance()->releaseConnection(this);
		OTSYS_THREAD_LOCKVARRELEASE(m_connectionLock);
	}

	boost::asio::ip::tcp::socket& getHandle() { return m_socket; }

	void closeConnection();
	void acceptConnection();

	bool send(OutputMessage* msg);

	uint32_t getIP() const;

private:
	void parseHeader(const boost::system::error_code& error);
	void parsePacket(const boost::system::error_code& error);

	void onWriteOperation(OutputMessage* msg, const boost::system::error_code& error);

	void handleReadError(const boost::system::error_code& error);
	void handleWriteError(const boost::system::error_code& error);

	void closeConnectionTask();
	bool closingConnection();
	void deleteConnectionTask();

	void internalSend(OutputMessage* msg);

	NetworkMessage m_msg;
	boost::asio::ip::tcp::socket m_socket;
	bool m_socketClosed;

	bool m_writeError;
	bool m_readError;

	int32_t m_pendingWrite;
	std::list <OutputMessage*> m_outputQueue;
	int32_t m_pendingRead;
	uint32_t m_closeState;

	OTSYS_THREAD_LOCKVAR m_connectionLock;

	Protocol* m_protocol;
};

#endif

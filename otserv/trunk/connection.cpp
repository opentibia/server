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

#include "protocol.h"
#include "outputmessage.h"
#include "tasks.h"
#include "scheduler.h"
#include "connection.h"
#include "tools.h"
#include "server.h"
#include "protocolgame.h"
#include "protocolold.h"
#include "admin.h"
#include "status.h"

#include <boost/bind.hpp>

bool Connection::m_logError = true;

#ifdef __ENABLE_SERVER_DIAGNOSTIC__
uint32_t Connection::connectionCount = 0;
#endif

Connection* ConnectionManager::createConnection(boost::asio::ip::tcp::socket* socket,
	boost::asio::io_service& io_service, ServicePort_ptr servicer)
{
	#ifdef __DEBUG_NET_DETAIL__
	std::cout << "Create new Connection" << std::endl;
	#endif

	boost::recursive_mutex::scoped_lock lockClass(m_connectionManagerLock);
	Connection* connection = new Connection(socket, io_service, servicer);
	m_connections.push_back(connection);
	return connection;
}

void ConnectionManager::releaseConnection(Connection* connection)
{
	#ifdef __DEBUG_NET_DETAIL__
	std::cout << "Releasing connection" << std::endl;
	#endif

	boost::recursive_mutex::scoped_lock lockClass(m_connectionManagerLock);
	std::list<Connection*>::iterator it =
		std::find(m_connections.begin(), m_connections.end(), connection);

	if(it != m_connections.end()){
		m_connections.erase(it);
	}
	else{
		std::cout << "Error: [ConnectionManager::releaseConnection] Connection not found" << std::endl;
	}
}

void ConnectionManager::closeAll()
{
	#ifdef __DEBUG_NET_DETAIL__
	std::cout << "Closing all connections" << std::endl;
	#endif
	boost::recursive_mutex::scoped_lock lockClass(m_connectionManagerLock);
	std::list<Connection*>::iterator it = m_connections.begin();
	while(it != m_connections.end()){
		boost::system::error_code error;
		(*it)->m_socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both, error);
		(*it)->m_socket->close(error);
		++it;
	}
	m_connections.clear();
}


//*****************

void Connection::closeConnection()
{
	//any thread
	#ifdef __DEBUG_NET_DETAIL__
	std::cout << "Connection::closeConnection" << std::endl;
	#endif

	boost::recursive_mutex::scoped_lock lockClass(m_connectionLock);
	if(m_closeState != CLOSE_STATE_NONE)
		return;

	m_closeState = CLOSE_STATE_REQUESTED;

	g_dispatcher.addTask(
		createTask(boost::bind(&Connection::closeConnectionTask, this)));
}

void Connection::closeConnectionTask()
{
	//dispatcher thread
	#ifdef __DEBUG_NET_DETAIL__
	std::cout << "Connection::closeConnectionTask" << std::endl;
	#endif

	m_connectionLock.lock();
	if(m_closeState != CLOSE_STATE_REQUESTED){
		std::cout << "Error: [Connection::closeConnectionTask] m_closeState = " << m_closeState << std::endl;
		m_connectionLock.unlock();
		return;
	}

	m_closeState = CLOSE_STATE_CLOSING;

	if(m_protocol){
		g_dispatcher.addTask(
			createTask(boost::bind(&Protocol::releaseProtocol, m_protocol)));
		m_protocol->setConnection(NULL);
		m_protocol = NULL;
	}

	if(!closingConnection()){
		m_connectionLock.unlock();
	}
}

bool Connection::closingConnection()
{
	//any thread
	#ifdef __DEBUG_NET_DETAIL__
	std::cout << "Connection::closingConnection" << std::endl;
	#endif

	if(m_pendingWrite == 0 || m_writeError == true){
		if(!m_socketClosed){
			#ifdef __DEBUG_NET_DETAIL__
			std::cout << "Closing socket" << std::endl;
			#endif

			boost::system::error_code error;
			m_socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both, error);
			if(error){
				if(error == boost::asio::error::not_connected){
					//Transport endpoint is not connected.
				}
				else{
					PRINT_ASIO_ERROR("Shutdown");
				}
			}
			m_socket->close(error);
			m_socketClosed = true;
			if(error){
				PRINT_ASIO_ERROR("Close");
			}
		}

		if(m_pendingRead == 0){
			#ifdef __DEBUG_NET_DETAIL__
			std::cout << "Deleting Connection" << std::endl;
			#endif

			m_connectionLock.unlock();

			g_dispatcher.addTask(
				createTask(boost::bind(&Connection::releaseConnection, this)));

			return true;
		}
	}
	return false;
}

void Connection::releaseConnection()
{
	if(m_refCount > 0){
		//Reschedule it and try again.
		g_scheduler.addEvent( createSchedulerTask(SCHEDULER_MINTICKS,
			boost::bind(&Connection::releaseConnection, this)));
	}
	else{
		deleteConnectionTask();
	}
}

void Connection::onStopOperation()
{
	//io_service thread
	m_connectionLock.lock();
	m_timer.cancel();
	ConnectionManager::getInstance()->releaseConnection(this);

	if(m_socket->is_open()){
		m_socket->cancel();
		m_socket->close();
	}

	delete m_socket;

	m_connectionLock.unlock();
	delete this;
}

void Connection::deleteConnectionTask()
{
	//dispather thread
	assert(m_refCount == 0);
	try{
		m_io_service.dispatch(boost::bind(&Connection::onStopOperation, this));
	}
	catch(boost::system::system_error& e){
		if(m_logError){
			LOG_MESSAGE("NETWORK", LOGTYPE_ERROR, 1, e.what());
			m_logError = false;
		}
	}
}

void Connection::acceptConnection(Protocol* protocol)
{
	m_protocol = protocol;
	m_protocol->onConnect();

	acceptConnection();
}

void Connection::acceptConnection()
{
	// Read size of the first packet
	m_pendingRead++;

	try{
		m_timer.expires_from_now(boost::posix_time::seconds(10));
		m_timer.async_wait( boost::bind(&Connection::handleTimeout, this, boost::asio::placeholders::error));

		boost::asio::async_read(getHandle(),
			boost::asio::buffer(m_msg.getBuffer(), NetworkMessage::header_length),
			boost::bind(&Connection::parseHeader, this, boost::asio::placeholders::error));
	}
	catch(boost::system::system_error& e){
		if(m_logError){
			LOG_MESSAGE("NETWORK", LOGTYPE_ERROR, 1, e.what());
			m_logError = false;
		}
	}
}

void Connection::parseHeader(const boost::system::error_code& error)
{
	m_connectionLock.lock();
	m_timer.cancel();
	m_pendingRead--;
	if(m_closeState == CLOSE_STATE_CLOSING){
		if(!closingConnection()){
			m_connectionLock.unlock();
		}
		return;
	}

	int32_t size = m_msg.decodeHeader();
	if(!error && size > 0 && size < NETWORKMESSAGE_MAXSIZE - 16){
		// Read packet content
		m_pendingRead++;
		m_msg.setMessageLength(size + NetworkMessage::header_length);
		boost::asio::async_read(getHandle(), boost::asio::buffer(m_msg.getBodyBuffer(), size),
			boost::bind(&Connection::parsePacket, this, boost::asio::placeholders::error));
	}
	else{
		handleReadError(error);
	}
	m_connectionLock.unlock();
}

void Connection::parsePacket(const boost::system::error_code& error)
{
	m_connectionLock.lock();
	m_pendingRead--;
	if(m_closeState == CLOSE_STATE_CLOSING){
		if(!closingConnection()){
			m_connectionLock.unlock();
		}
		return;
	}

	if(!error){
		//Check packet checksum
		uint32_t recvChecksum = m_msg.PeekU32();
		uint32_t checksum = 0;
		int32_t len = m_msg.getMessageLength() - m_msg.getReadPos() - 4;
		if(len > 0){
			checksum = adlerChecksum((uint8_t*)(m_msg.getBuffer() + m_msg.getReadPos() + 4), len);
		}

		if(recvChecksum == checksum)
			// remove the checksum
			m_msg.GetU32();

		if(!m_receivedFirst){
			m_receivedFirst = true;
			// First message received
			if(!m_protocol){ // Game protocol has already been created at this point
				m_protocol = m_service_port->make_protocol(recvChecksum == checksum, m_msg);
				if(!m_protocol){
					closeConnection();
					m_connectionLock.unlock();
					return;
				}
				m_protocol->setConnection(this);
			}
			else{
				// Skip protocol ID
				m_msg.GetByte();
			}
			m_protocol->onRecvFirstMessage(m_msg);
		}
		else{
			// Send the packet to the current protocol
			m_protocol->onRecvMessage(m_msg);
		}

		// Wait to the next packet
		m_pendingRead++;
		boost::asio::async_read(getHandle(),
			boost::asio::buffer(m_msg.getBuffer(), NetworkMessage::header_length),
			boost::bind(&Connection::parseHeader, this, boost::asio::placeholders::error));
	}
	else{
		handleReadError(error);
	}
	m_connectionLock.unlock();
}

void Connection::handleTimeout(const boost::system::error_code& error)
{
	if(!error){
		m_connectionLock.lock();
		if(m_pendingRead > 0){
			//cancel all asynchronous operations associated with the socket
			getHandle().cancel();
		}
		m_connectionLock.unlock();
	}
}

void Connection::handleReadError(const boost::system::error_code& error)
{
	#ifdef __DEBUG_NET_DETAIL__
	PRINT_ASIO_ERROR("Reading - detail");
	#endif
	if(error == boost::asio::error::operation_aborted){
		//Operation aborted because connection will be closed
		//Do NOT call closeConnection() from here
	}
	else if(error == boost::asio::error::eof){
		//No more to read
		closeConnection();
	}
	else if(error == boost::asio::error::connection_reset ||
			error == boost::asio::error::connection_aborted){
		//Connection closed remotely
		closeConnection();
	}
	else{
		PRINT_ASIO_ERROR("Reading");
		closeConnection();
	}
	m_readError = true;
}

bool Connection::send(OutputMessage_ptr msg)
{
	#ifdef __DEBUG_NET_DETAIL__
	std::cout << "Connection::send init" << std::endl;
	#endif

	m_connectionLock.lock();
	if(m_closeState == CLOSE_STATE_CLOSING || m_writeError){
		m_connectionLock.unlock();
		return false;
	}

	msg->getProtocol()->onSendMessage(msg);

	if(m_pendingWrite == 0){
		#ifdef __DEBUG_NET_DETAIL__
		std::cout << "Connection::send " << msg->getMessageLength() << std::endl;
		#endif
		internalSend(msg);
	}
	else{
		#ifdef __DEBUG_NET__
		std::cout << "Connection::send Adding to queue " << msg->getMessageLength() << std::endl;
		#endif
		m_outputQueue.push_back(msg);
	}
	m_connectionLock.unlock();
	return true;
}

void Connection::internalSend(OutputMessage_ptr msg)
{
	m_pendingWrite++;

	try{
		boost::asio::async_write(getHandle(),
			boost::asio::buffer(msg->getOutputBuffer(), msg->getMessageLength()),
			boost::bind(&Connection::onWriteOperation, this, msg, boost::asio::placeholders::error));
	}
	catch(boost::system::system_error& e){
		if(m_logError){
			LOG_MESSAGE("NETWORK", LOGTYPE_ERROR, 1, e.what());
			m_logError = false;
		}
	}
}

uint32_t Connection::getIP() const
{
	//Ip is expressed in network byte order
	boost::system::error_code error;
	const boost::asio::ip::tcp::endpoint endpoint = m_socket->remote_endpoint(error);
	if(!error){
		return htonl(endpoint.address().to_v4().to_ulong());
	}
	else{
		PRINT_ASIO_ERROR("Getting remote ip");
		return 0;
	}
}

void Connection::onWriteOperation(OutputMessage_ptr msg, const boost::system::error_code& error)
{
	#ifdef __DEBUG_NET_DETAIL__
	std::cout << "onWriteOperation" << std::endl;
	#endif

	m_connectionLock.lock();
	msg.reset();

	if(!error){
		if(m_pendingWrite > 0){
			if(!m_outputQueue.empty()){
				msg = m_outputQueue.front();
				m_outputQueue.pop_front();
				internalSend(msg);
				#ifdef __DEBUG_NET_DETAIL__
				std::cout << "Connection::onWriteOperation send " << msg->getMessageLength() << std::endl;
				#endif
			}
			m_pendingWrite--;
		}
		else{
			std::cout << "Error: [Connection::onWriteOperation] Getting unexpected notification!" << std::endl;
			// Error. Pending operations counter is 0, but we are getting a
			// notification!!
		}
	}
	else{
		m_pendingWrite--;
		handleWriteError(error);
	}

	if(m_closeState == CLOSE_STATE_CLOSING){
		if(!closingConnection()){
			m_connectionLock.unlock();
		}
		return;
	}

	m_connectionLock.unlock();
}

void Connection::handleWriteError(const boost::system::error_code& error)
{
	#ifdef __DEBUG_NET_DETAIL__
	PRINT_ASIO_ERROR("Writing - detail");
	#endif
	if(error == boost::asio::error::operation_aborted){
		//Operation aborted because connection will be closed
		//Do NOT call closeConnection() from here
	}
	else if(error == boost::asio::error::eof){
		//No more to read
		closeConnection();
	}
	else if(error == boost::asio::error::connection_reset ||
			error == boost::asio::error::connection_aborted){
		//Connection closed remotely
		closeConnection();
	}
	else{
		PRINT_ASIO_ERROR("Writing");
		closeConnection();
	}
	m_writeError = true;
}

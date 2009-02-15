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
#include "protocolgame.h"
#include "protocollogin.h"
#include "protocolold.h"
#include "admin.h"
#include "status.h"
#include "tasks.h"
#include "scheduler.h"
#include "connection.h"
#include "tools.h"

#include <boost/bind.hpp>

#ifdef __ENABLE_SERVER_DIAGNOSTIC__
uint32_t Connection::connectionCount = 0;
#endif

Connection* ConnectionManager::createConnection(boost::asio::io_service& io_service)
{
	#ifdef __DEBUG_NET_DETAIL__
	std::cout << "Create new Connection" << std::endl;
	#endif

	boost::recursive_mutex::scoped_lock lockClass(m_connectionManagerLock);
	Connection* connection = new Connection(io_service);
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
		(*it)->m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, error);
		(*it)->m_socket.close(error);
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

	Dispatcher::getDispatcher().addTask(
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
		Dispatcher::getDispatcher().addTask(
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
			m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, error);
			if(error){
				if(error == boost::asio::error::not_connected){
					//Transport endpoint is not connected.
				}
				else{
					PRINT_ASIO_ERROR("Shutdown");
				}
			}
			m_socket.close(error);
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

			Dispatcher::getDispatcher().addTask(
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
		Scheduler::getScheduler().addEvent( createSchedulerTask(SCHEDULER_MINTICKS,
			boost::bind(&Connection::releaseConnection, this)));
	}
	else{
		deleteConnectionTask();
	}
}

void Connection::deleteConnectionTask()
{
	//dispather thread
	assert(m_refCount == 0);

	while(!m_outputQueue.empty()){
		OutputMessagePool::getInstance()->releaseMessage(m_outputQueue.back(), true);
		m_outputQueue.pop_back();
	}

	delete this;
}

void Connection::acceptConnection()
{
	// Read size of te first packet
	m_pendingRead++;
	boost::asio::async_read(m_socket,
		boost::asio::buffer(m_msg.getBuffer(), NetworkMessage::header_length),
		boost::bind(&Connection::parseHeader, this, boost::asio::placeholders::error));
}

void Connection::parseHeader(const boost::system::error_code& error)
{
	m_connectionLock.lock();
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
		boost::asio::async_read(m_socket, boost::asio::buffer(m_msg.getBodyBuffer(), size),
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

		if(!m_protocol){

			if(recvChecksum == checksum){
				// remove the checksum
				m_msg.GetU32();
				// Protocol depends on the first byte of the packet
				uint8_t protocolId = m_msg.GetByte();
				switch(protocolId){
				case 0x01: // Login server protocol
					m_protocol = new ProtocolLogin(this);
					break;
				case 0x0A: // World server protocol
					m_protocol = new ProtocolGame(this);
					break;
				default:
					// No valid protocol
					closeConnection();
					m_connectionLock.unlock();
					return;
				}
				m_protocol->onRecvFirstMessage(m_msg);
			}
			else{
				//Protocols without checksum
				uint8_t protocolId = m_msg.GetByte();
				switch(protocolId){
				case 0x01: // Old Login server protocol
				case 0x0A: // Old World server protocol
					//This occurs if you try login with an old client version ( < 8.3)
					m_protocol = new ProtocolOld(this);
					break;
				case 0xFE: // Admin protocol
					m_protocol = new ProtocolAdmin(this);
					break;
				case 0xFF: // Status protocol
					m_protocol = new ProtocolStatus(this);
					break;
				default:
					closeConnection();
					m_connectionLock.unlock();
					return;
				}
				m_protocol->onRecvFirstMessage(m_msg);
			}
		}
		else{
			if(recvChecksum == checksum) m_msg.GetU32();
			// Send the packet to the current protocol
			m_protocol->onRecvMessage(m_msg);
		}

		// Wait to the next packet
		m_pendingRead++;
		boost::asio::async_read(m_socket,
			boost::asio::buffer(m_msg.getBuffer(), NetworkMessage::header_length),
			boost::bind(&Connection::parseHeader, this, boost::asio::placeholders::error));
	}
	else{
		handleReadError(error);
	}
	m_connectionLock.unlock();
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

bool Connection::send(OutputMessage* msg)
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

void Connection::internalSend(OutputMessage* msg)
{
	m_pendingWrite++;
	boost::asio::async_write(m_socket,
		boost::asio::buffer(msg->getOutputBuffer(), msg->getMessageLength()),
		boost::bind(&Connection::onWriteOperation, this, msg, boost::asio::placeholders::error));
}

uint32_t Connection::getIP() const
{
	//Ip is expressed in network byte order
	boost::system::error_code error;
	const boost::asio::ip::tcp::endpoint endpoint = m_socket.remote_endpoint(error);
	if(!error){
		return htonl(endpoint.address().to_v4().to_ulong());
	}
	else{
		PRINT_ASIO_ERROR("Getting remote ip");
		return 0;
	}
}

void Connection::onWriteOperation(OutputMessage* msg, const boost::system::error_code& error)
{
	#ifdef __DEBUG_NET_DETAIL__
	std::cout << "onWriteOperation" << std::endl;
	#endif

	OutputMessagePool::getInstance()->releaseMessage(msg, true);

	m_connectionLock.lock();

	if(!error){
		if(m_pendingWrite > 0){
			if(!m_outputQueue.empty()){
				OutputMessage* msg = m_outputQueue.front();
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
		PRINT_ASIO_ERROR("Writting");
		closeConnection();
	}
	m_writeError = true;
}

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

#include "connection.h"
#include "protocol.h"
#include "protocollogin.h"
#include "outputmessage.h"
#include "tasks.h"
#include "scheduler.h"

#include <boost/bind.hpp>

void Connection::closeConnection()
{
	std::cout << "Connection::closeConnection" << std::endl;
	OTSYS_THREAD_LOCK_CLASS lockClass(m_connectionLock);
	if(m_closeState != CLOSE_STATE_NONE)
		return;
	
	m_closeState = CLOSE_STATE_REQUESTED;
	/*
	Dispatcher::getDispatcher().addTask(
		createTask(boost::bind(&Connection::closeConnectionTask, this)));
	*/
	Scheduler::getScheduler().addEvent(
		createSchedulerTask(1000, boost::bind(&Connection::closeConnectionTask, this)));
}

void Connection::closeConnectionTask()
{
	std::cout << "Connection::closeConnectionTask" << std::endl;
	OTSYS_THREAD_LOCK_CLASS lockClass(m_connectionLock);
	if(m_closeState != CLOSE_STATE_REQUESTED){
		std::cout << "Error: [Connection::closeConnectionTask] m_closeState = " << m_closeState << std::endl;
		return;
	}
	
	m_closeState = CLOSE_STATE_CLOSING;
	
	if(m_protocol){
		Dispatcher::getDispatcher().addTask(
			createTask(boost::bind(&Protocol::deleteProtocolTask, m_protocol)));
		m_protocol->setConnection(NULL);
		m_protocol = NULL;
	}
	
	closingConnection();
}

void Connection::acceptConnection()
{
	// Read size of te first packet
	boost::asio::async_read(m_socket,
		boost::asio::buffer(m_msg.getBuffer(), NetworkMessage::header_length),
		boost::bind(&Connection::parseHeader, this, boost::asio::placeholders::error));
	m_pendingRead++;
}

void Connection::parseHeader(const boost::asio::error& error)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(m_connectionLock);
	m_pendingRead--;
	if(m_closeState == CLOSE_STATE_CLOSING){
		closingConnection();
		return;
	}
	
	int32_t size = m_msg.decodeHeader();
	if(!error && size > 0 && size < NETWORKMESSAGE_MAXSIZE - 16){
		// Read packet content
		m_msg.setMessageLength(size + NetworkMessage::header_length);
		boost::asio::async_read(m_socket, boost::asio::buffer(m_msg.getBodyBuffer(), size),
			boost::bind(&Connection::parsePacket, this, boost::asio::placeholders::error));
		m_pendingRead++;
	}
	else{
		if(error == boost::asio::error::operation_aborted){
			//
		}
		else{
			//TODO: error?
		}
	}
}

void Connection::parsePacket(const boost::asio::error& error)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(m_connectionLock);
	m_pendingRead--;
	if(m_closeState == CLOSE_STATE_CLOSING){
		closingConnection();
		return;
	}
	
	if(!error){
		// Protocol selection
		if(!m_protocol){
			// Protocol depends on the first byte of the packet
			uint8_t protocolId = m_msg.GetByte();
			switch(protocolId){
			case 0x01: // Login server protocol
				m_protocol = new ProtocolLogin(this);
				break;
			case 0x0A: // World server protocol
				//m_protocol = new protocol79(this);
				break;
			case 0xFE: // Admin protocol
				break;
			case 0xFF: // Status protocol
				//m_protocol = new protocolStatus(this);
				break;
			default:
				// No valid protocol
				closeConnection();
				break;
			}
		}
		
		// Send the packet to the current protocol
		m_protocol->parsePacket(m_msg);
		
		// Wait to the next packet
		boost::asio::async_read(m_socket,
			boost::asio::buffer(m_msg.getBuffer(), NetworkMessage::header_length),
			boost::bind(&Connection::parseHeader, this, boost::asio::placeholders::error));
		m_pendingRead++;
	}
	else{
		if(error == boost::asio::error::operation_aborted){
			//
		}
		else{
			//TODO: error?
		}
	}
}

void Connection::send(OutputMessage* msg)
{
	std::cout << "Connection::send init" << std::endl;
	OTSYS_THREAD_LOCK_CLASS lockClass(m_connectionLock);
	if(m_closeState == CLOSE_STATE_CLOSING)
		return;
	
	std::cout << "Connection::send " << msg->getMessageLength() << std::endl;
	boost::asio::async_write(m_socket,
		boost::asio::buffer(msg->getOutputBuffer(), msg->getMessageLength()),
		boost::bind(&OutputMessagePool::writeHandler, msg, boost::asio::placeholders::error));
	
	m_pendingWrite++;
}

uint32_t Connection::getIP()
{
	//Ip is expressed in network byte order
	boost::asio::error error;
	const boost::asio::ip::tcp::endpoint endpoint = m_socket.remote_endpoint(boost::asio::assign_error(error));
	if(!error){
		return htonl(endpoint.address().to_v4().to_ulong());
	}
	else{
		return 0;
	}
	/*
	sockaddr_in sain;
	socklen_t salen = sizeof(sockaddr_in);

	if(getpeername(s, (sockaddr*)&sain, &salen) == 0){
#if defined WIN32 || defined __WINDOWS__
		return sain.sin_addr.S_un.S_addr;
#else
		return sain.sin_addr.s_addr;
#endif
	}
*/
}


void Connection::onWriteOperation(const boost::asio::error& error)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(m_connectionLock);
	std::cout << "onWriteOperation" << std::endl;
	
	if(!error){
		if(m_pendingWrite > 0){
			m_pendingWrite--;
		}
		else{
			std::cout << "onWriteOperation. Error 1" << std::endl;
			// Error. Pending operations counter is 0, but we are getting a
			// notification!!
		}
	}
	else{
		std::cout << "onWriteOperation. Error 2" << std::endl;
		//TODO. write operation error occurred
	}
	
	if(m_closeState == CLOSE_STATE_CLOSING){
		closingConnection();
		return;
	}
}

void Connection::closingConnection()
{
	std::cout << "Connection::closingConnection" << std::endl;
	if(m_pendingWrite == 0){
		if(!m_socketClosed){
			std::cout << "Closing socket" << std::endl;
			boost::asio::error error;
			m_socket.close(boost::asio::assign_error(error));
			m_socketClosed = true;
			if(error){
				std::cout << "Closing socket - error" << std::endl;
				//
			}
		}
		if(m_pendingRead == 0){
			std::cout << "Deleting Connection" << std::endl;
			delete this;
		}
	}
}

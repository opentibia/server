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

#include <boost/bind.hpp>

void Connection::closeConnection()
{
	//TODO. Decide from which thread should be closed the connection
	// using a timer and a pool of connections(like OutputMessage)?
}

void Connection::acceptConnection()
{
	// Read size of te first packet
	boost::asio::async_read(m_socket,
		boost::asio::buffer(m_msg.getBuffer(), NetworkMessage::header_length),
		boost::bind(&Connection::parseHeader, this, boost::asio::placeholders::error));
}

void Connection::parseHeader(const boost::asio::error& error)
{
	int32_t size = m_msg.decodeHeader();
	if(!error && size > 0 && size < NETWORKMESSAGE_MAXSIZE - 16){
		// Read packet content
		boost::asio::async_read(m_socket, boost::asio::buffer(m_msg.getBodyBuffer(), size),
			boost::bind(&Connection::parsePacket, this, boost::asio::placeholders::error));
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
				// TODO: close connection
				break;
			}
		}
		
		// Send the packet to the current protocol
		m_protocol->parsePacket(m_msg);
		
		// Wait to the next packet
    	boost::asio::async_read(m_socket,
			boost::asio::buffer(m_msg.getBuffer(), NetworkMessage::header_length),
			boost::bind(&Connection::parseHeader, this, boost::asio::placeholders::error));
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
	boost::asio::async_write(m_socket,
		boost::asio::buffer(msg->getOutputBuffer(), msg->getMessageLength()),
		boost::bind(&OutputMessagePool::writeHandler, msg, boost::asio::placeholders::error));
	
	m_pendingWrite++;
}

void Connection::onWriteOperation(const boost::asio::error& error)
{
	if(!error){
		if(m_pendingWrite != 0){
			m_pendingWrite--;
		}
		else{
			// Error. Pending operations counter is 0, but we are getting a
			// notification!!
		}
	}
	else{
		//TODO. write operation error
	}
}

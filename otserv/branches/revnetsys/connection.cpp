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

#include <boost/bind.hpp>

void Connection::closeConnection()
{
	//
}

void Connection::acceptConnection()
{
	asio::async_read(socket,
		asio::buffer(msg.getBuffer(), NetworkMessage::header_length),
		boost::bind(&Connection::parseHeader, this, asio::placeholders::error));
}

void Connection::parseHeader(const asio::error& error)
{
  if(!error && msg.decodeHeader()){
    asio::async_read(socket, asio::buffer(msg.getBodyBuffer(), msg.getMessageLength()),
        boost::bind(&Connection::parsePacket, this, asio::placeholders::error));
  }
}

void Connection::parsePacket(const asio::error& error)
{
	if(!error){
    asio::async_read(socket,
        asio::buffer(msg.getBuffer(), NetworkMessage::header_length),
        boost::bind(&Connection::parseHeader, this, asio::placeholders::error));
	}
}

void Connection::send(char* data, uint32_t size)
{
	//
}


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

#ifndef __OTSERV_PROTOCOL_H__
#define __OTSERV_PROTOCOL_H__

//#include "definitions.h"
//#include <boost/asio.hpp>

class NetworkMessage;
class OutputMessage;
class Connection;
class RSA;

class Protocol
{
public:
	Protocol(Connection* connection)
	{ 
		m_connection = connection;
		m_encryptionEnabled = false;
		m_key[0] = 0; m_key[1] = 0; m_key[2] = 0; m_key[3] = 0;
	}
	
	virtual ~Protocol() {}

	virtual void parsePacket(NetworkMessage& msg) = 0;
	
	//Called from connection to know if the protocol instance
	// can be released
	virtual bool canBeReleased() { return true; }
	
	virtual void onSendMessage(OutputMessage* msg);
	
	Connection* getConnection() { return m_connection;}
	void setConnection(Connection* connection) { m_connection = connection; }
	
protected:
	
	void XTEA_encrypt(OutputMessage& msg);
	bool XTEA_decrypt(NetworkMessage& msg);
	bool RSA_decrypt(RSA* rsa, NetworkMessage& msg);
	
	Connection* m_connection;
	bool m_encryptionEnabled;
	uint32_t m_key[4];
};

#endif

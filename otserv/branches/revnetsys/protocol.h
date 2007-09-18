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

#include "definitions.h"
#include <boost/utility.hpp>
#include "outputmessage.h"

class Connection;
class RSA;

#define CLIENT_VERSION_MIN 792
#define CLIENT_VERSION_MAX 792

#define STRING_CLIENT_VERSION "This server requires client version 7.92."

class Protocol : boost::noncopyable
{
public:
	Protocol(Connection* connection)
	{
		m_connection = connection;
		m_encryptionEnabled = false;
		m_key[0] = 0; m_key[1] = 0; m_key[2] = 0; m_key[3] = 0;
		m_outputBuffer = NULL;
	}

	virtual ~Protocol() {}

	virtual void parsePacket(NetworkMessage& msg){};

	void onSendMessage(OutputMessage* msg);
	void onRecvMessage(NetworkMessage& msg);
	virtual void onRecvFirstMessage(NetworkMessage& msg) = 0;

	Connection* getConnection() { return m_connection;}
	const Connection* getConnection() const { return m_connection;}
	void setConnection(Connection* connection) { m_connection = connection; }

	uint32_t getIP() const;

protected:
	//Use this function for autosend messages only
	OutputMessage* getOutputBuffer()
	{
		if(m_outputBuffer){
			return m_outputBuffer;
		}
		else if(m_connection){
			m_outputBuffer = OutputMessagePool::getInstance()->getOutputMessage(this);
			return m_outputBuffer;
		}
		else{
			return NULL;
		}
	}

	void enableXTEAEncryption() { m_encryptionEnabled = true; }
	void disableXTEAEncryption() { m_encryptionEnabled = false; }
	void setXTEAKey(const uint32_t* key){
		memcpy(m_key, key, sizeof(uint32_t)*4);
	}

	void XTEA_encrypt(OutputMessage& msg);
	bool XTEA_decrypt(NetworkMessage& msg);
	bool RSA_decrypt(RSA* rsa, NetworkMessage& msg);

	virtual void deleteProtocolTask();
	friend class Connection;

private:

	OutputMessage* m_outputBuffer;
	Connection* m_connection;
	bool m_encryptionEnabled;
	uint32_t m_key[4];
};

#endif

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

#ifndef __OTSERV_OUTPUT_MESSAGE_H__
#define __OTSERV_OUTPUT_MESSAGE_H__

#include "definitions.h"
#include <boost/system/system_error.hpp>
#include <boost/utility.hpp>

#include "networkmessage.h"
#include "otsystem.h"
#include <list>

class Protocol;
class Connection;

#define OUTPUT_POOL_SIZE 100

class OutputMessage : public NetworkMessage, boost::noncopyable
{
private:
	OutputMessage();
	
public:
	~OutputMessage() {}
	
	char* getOutputBuffer() { return (char*)&m_MsgBuf[m_outputBufferStart];}
	void setBufferStart(uint32_t start) {m_outputBufferStart = start;}
	
	void addCryptoHeader()
	{
		*(uint16_t*)(m_MsgBuf) = m_MsgSize;
		m_MsgSize = m_MsgSize + 2;
		m_outputBufferStart = 0;
	}
	
	enum OutputMessageState{
		STATE_FREE,
		STATE_ALLOCATED,
		STATE_ALLOCATED_NO_AUTOSEND,
		STATE_WAITING
	};
	
	void releaseMessage()
	{
		if(m_state == STATE_ALLOCATED_NO_AUTOSEND){
			freeMessage();
		}
		else{
			#ifdef __DEBUG_NET__
			std::cout << "Warning: [OutputMessage::freeMessage] m_state != STATE_ALLOCATED_NO_AUTOSEND" << std::endl;
			#endif
		}
	}
	
	Protocol* getProtocol() { return m_protocol;}
	Connection* getConnection() { return m_connection;}
	
protected:
	
	void freeMessage()
	{
		setConnection(NULL);
		setProtocol(NULL);
		setState(OutputMessage::STATE_FREE);
		m_frame = 0;
	}
	
	friend class OutputMessagePool;
	
	void setProtocol(Protocol* protocol){ m_protocol = protocol;}
	void setConnection(Connection* connection){ m_connection = connection;}
	
	void setState(OutputMessageState state) { m_state = state;}
	OutputMessageState getState() const { return m_state;}
	
	void setFrame(uint32_t frame) { m_frame = frame;}
	uint32_t getFrame() const { return m_frame;}
	
	Protocol* m_protocol;
	Connection* m_connection;
	
	uint32_t m_outputBufferStart;
	uint32_t m_frame;
	
	OutputMessageState m_state;
};

class OutputMessagePool
{
private:
	OutputMessagePool();

public:
	~OutputMessagePool();
	
	static OutputMessagePool* getInstance()
	{
		static OutputMessagePool instance;
		return &instance;
	}
	
	void send(OutputMessage* msg);
	void sendAll();
	OutputMessage* getOutputMessage(Protocol* protocol, bool autosend = true);
	void startExecutionFrame();
	
protected:

	void configureOutputMessage(OutputMessage* msg, Protocol* protocol, bool autosend);
	
	void internalReleaseMessage(OutputMessage* msg);
	
	static void writeHandler(OutputMessage* msg, const boost::system::error_code& error);
	
	friend class Connection;

	typedef std::list<OutputMessage*> OutputMessageVector;
	
	OutputMessageVector m_outputMessages;
	OutputMessageVector m_autoSendOutputMessages;
	OTSYS_THREAD_LOCKVAR m_outputPoolLock;
	uint32_t m_frame;
};

#endif

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

#include "networkmessage.h"
#include "otsystem.h"
#include <list>

#include <boost/utility.hpp>

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

	void writeMessageLength()
	{
		*(uint16_t*)(m_MsgBuf + 2) = m_MsgSize;
		//added header size to the message size
		m_MsgSize = m_MsgSize + 2;
		m_outputBufferStart = 2;
	}

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

	Protocol* getProtocol() { return m_protocol;}
	Connection* getConnection() { return m_connection;}

#ifdef __TRACK_NETWORK__
	void Track(std::string file, long line, std::string func)
	{
		if(last_uses.size() >= 25) {
			last_uses.pop_front();
		}
		std::ostringstream os;
		os << /*file << ":"*/ "line " << line << " " << func;
		last_uses.push_back(os.str());
	}
	void PrintTrace()
	{
		int n = 1;
		for(std::list<std::string>::const_reverse_iterator iter = last_uses.rbegin(); iter != last_uses.rend(); ++iter, ++n) {
			std::cout << "\t" << n << ".\t" << *iter << std::endl;
		}
	}
#endif
protected:
#ifdef __TRACK_NETWORK__
	std::list<std::string> last_uses;
#endif

	void freeMessage()
	{
		setConnection(NULL);
		setProtocol(NULL);
		m_frame = 0;
		m_outputBufferStart = 4;
		//setState have to be the last one
		setState(OutputMessage::STATE_FREE);
	}

	friend class OutputMessagePool;

	void setProtocol(Protocol* protocol){ m_protocol = protocol;}
	void setConnection(Connection* connection){ m_connection = connection;}

	void setState(OutputMessageState state) { m_state = state;}
	OutputMessageState getState() const { return m_state;}

	void setFrame(uint64_t frame) { m_frame = frame;}
	uint64_t getFrame() const { return m_frame;}

	Protocol* m_protocol;
	Connection* m_connection;

	uint32_t m_outputBufferStart;
	uint64_t m_frame;

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

	void releaseMessage(OutputMessage* msg, bool sent = false);

	size_t getTotalMessageCount() const {return m_allOutputMessages.size();}
	size_t getAvailableMessageCount() const {return m_outputMessages.size();}
	size_t getAutoMessageCount() const {return m_autoSendOutputMessages.size();}

protected:

	void configureOutputMessage(OutputMessage* msg, Protocol* protocol, bool autosend);

	void internalReleaseMessage(OutputMessage* msg);

	typedef std::list<OutputMessage*> OutputMessageVector;

	OutputMessageVector m_outputMessages;
	OutputMessageVector m_autoSendOutputMessages;
	OutputMessageVector m_allOutputMessages;
	OTSYS_THREAD_LOCKVAR m_outputPoolLock;
	uint64_t m_frameTime;
};

#ifdef __TRACK_NETWORK__
#define TRACK_MESSAGE(omsg) if(dynamic_cast<OutputMessage*>(omsg)) dynamic_cast<OutputMessage*>(omsg)->Track(__FILE__, __LINE__, __FUNCTION__)
#else
#define TRACK_MESSAGE(omsg) 
#endif

#endif

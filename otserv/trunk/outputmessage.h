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
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include "tools.h"
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
		add_header((uint16_t)(m_MsgSize));
	}

	void addCryptoHeader(bool addChecksum)
	{
		if(addChecksum){
			add_header((uint32_t)(adlerChecksum((uint8_t*)(m_MsgBuf + m_outputBufferStart), m_MsgSize)));
		}
		add_header((uint16_t)(m_MsgSize));
	}

	enum OutputMessageState{
		STATE_FREE,
		STATE_ALLOCATED,
		STATE_ALLOCATED_NO_AUTOSEND,
		STATE_WAITING
	};

	Protocol* getProtocol() { return m_protocol;}
	Connection* getConnection() { return m_connection;}
	uint64_t getFrame() const { return m_frame;}

	//void setOutputBufferStart(uint32_t pos) {m_outputBufferStart = pos;}
	//uint32_t getOutputBufferStart() const {return m_outputBufferStart;}

#ifdef __TRACK_NETWORK__
	virtual void Track(std::string file, long line, std::string func)
	{
		if(last_uses.size() >= 25) {
			last_uses.pop_front();
		}
		std::ostringstream os;
		os << /*file << ":"*/ "line " << line << " " << func;
		last_uses.push_back(os.str());
	}

	virtual void clearTrack()
	{
		last_uses.clear();
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

	template <typename T>
	inline void add_header(T add){
		if((int32_t)m_outputBufferStart - (int32_t)sizeof(T) < 0){
			std::cout << "Error: [OutputMessage::add_header] m_outputBufferStart(" << m_outputBufferStart <<
					") < " << sizeof(T) << std::endl;
			return;
		}
		m_outputBufferStart = m_outputBufferStart - sizeof(T);
		*(T*)(m_MsgBuf + m_outputBufferStart) = add;
		//added header size to the message size
		m_MsgSize = m_MsgSize + sizeof(T);
	}


	void freeMessage()
	{
		setConnection(NULL);
		setProtocol(NULL);
		m_frame = 0;
		//allocate enough size for headers
		//2 bytes for unencrypted message size
		//4 bytes for checksum
		//2 bytes for encrypted message size
		m_outputBufferStart = 8;

		//setState have to be the last one
		setState(OutputMessage::STATE_FREE);
	}

	friend class OutputMessagePool;

	void setProtocol(Protocol* protocol){ m_protocol = protocol;}
	void setConnection(Connection* connection){ m_connection = connection;}

	void setState(OutputMessageState state) { m_state = state;}
	OutputMessageState getState() const { return m_state;}

	void setFrame(uint64_t frame) { m_frame = frame;}

	Protocol* m_protocol;
	Connection* m_connection;

	uint32_t m_outputBufferStart;
	uint64_t m_frame;

	OutputMessageState m_state;
};

typedef boost::shared_ptr<OutputMessage> OutputMessage_ptr;

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

	void send(OutputMessage_ptr msg);
	void sendAll();
	void stop() {m_isOpen = false;}
	OutputMessage_ptr getOutputMessage(Protocol* protocol, bool autosend = true);
	void startExecutionFrame();

	size_t getTotalMessageCount() const {return m_allOutputMessages.size();}
	size_t getAvailableMessageCount() const {return m_outputMessages.size();}
	size_t getAutoMessageCount() const {return m_autoSendOutputMessages.size();}
	void addToAutoSend(OutputMessage_ptr msg);

protected:

	void configureOutputMessage(OutputMessage_ptr msg, Protocol* protocol, bool autosend);
	void releaseMessage(OutputMessage* msg);
	void internalReleaseMessage(OutputMessage* msg);

	typedef std::list<OutputMessage*> InternalOutputMessageList;
	typedef std::list<OutputMessage_ptr> OutputMessageMessageList;

	InternalOutputMessageList m_outputMessages;
	InternalOutputMessageList m_allOutputMessages;
	OutputMessageMessageList m_autoSendOutputMessages;
	OutputMessageMessageList m_toAddQueue;
	boost::recursive_mutex m_outputPoolLock;
	uint64_t m_frameTime;
	bool m_isOpen;
};

#ifdef __TRACK_NETWORK__
#define TRACK_MESSAGE(omsg) (omsg)->Track(__FILE__, __LINE__, __FUNCTION__)
#else
#define TRACK_MESSAGE(omsg)
#endif

#endif

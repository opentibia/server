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

#include "outputmessage.h"
#include "protocol.h"
#include "scheduler.h"

extern Dispatcher g_dispatcher;

#ifdef __ENABLE_SERVER_DIAGNOSTIC__
uint32_t OutputMessagePool::OutputMessagePoolCount = OUTPUT_POOL_SIZE;
#endif

OutputMessage::OutputMessage()
{
	freeMessage();
}

char* OutputMessage::getOutputBuffer()
{
	return (char*)&m_MsgBuf[m_outputBufferStart];
}

void OutputMessage::writeMessageLength()
{
	add_header((uint16_t)(m_MsgSize));
}

void OutputMessage::addCryptoHeader(bool addChecksum)
{
	if(addChecksum){
		add_header((uint32_t)(adlerChecksum((uint8_t*)(m_MsgBuf + m_outputBufferStart), m_MsgSize)));
	}
	add_header((uint16_t)(m_MsgSize));
}

Protocol* OutputMessage::getProtocol()
{
	return m_protocol;
}

Connection_ptr OutputMessage::getConnection()
{
	return m_connection;
}

const uint64_t& OutputMessage::getFrame() const
{
	return m_frame;
}

void OutputMessage::freeMessage()
{
	setConnection(Connection_ptr());
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

void OutputMessage::setProtocol(Protocol* protocol)
{
	m_protocol = protocol;
}

void OutputMessage::setConnection(Connection_ptr connection)
{
	m_connection = connection;
}

void OutputMessage::setState(OutputMessageState state)
{
	m_state = state;
}

OutputMessage::OutputMessageState OutputMessage::getState() const
{
	return m_state;
}

void OutputMessage::setFrame(const uint64_t& frame)
{
	m_frame = frame;
}

//*********** OutputMessagePool ****************

OutputMessagePool::OutputMessagePool()
{
	for(uint32_t i = 0; i < OUTPUT_POOL_SIZE; ++i){
		OutputMessage* msg = new OutputMessage();
		m_outputMessages.push_back(msg);
#ifdef __TRACK_NETWORK__
		m_allOutputMessages.push_back(msg);
#endif
	}
	m_frameTime = OTSYS_TIME();
}

OutputMessagePool::~OutputMessagePool()
{
	InternalOutputMessageList::iterator it;
	for(it = m_outputMessages.begin(); it != m_outputMessages.end(); ++it){
		delete *it;
	}
}

OutputMessagePool* OutputMessagePool::getInstance()
{
	static OutputMessagePool instance;
	return &instance;
}

void OutputMessagePool::send(OutputMessage_ptr msg)
{
	m_outputPoolLock.lock();
	OutputMessage::OutputMessageState state = msg->getState();
	m_outputPoolLock.unlock();

	if(state == OutputMessage::STATE_ALLOCATED_NO_AUTOSEND){
		#ifdef __DEBUG_NET_DETAIL__
		std::cout << "Sending message - SINGLE" << std::endl;
		#endif

		if(msg->getConnection()){
			if(!msg->getConnection()->send(msg)){
				// Send only fails when connection is closing (or in error state)
				// This call will free the message
				msg->getProtocol()->onSendMessage(msg);
			}
		}
		else{
			#ifdef __DEBUG_NET__
			std::cout << "Error: [OutputMessagePool::send] NULL connection." << std::endl;
			#endif
		}
	}
	else{
		#ifdef __DEBUG_NET__
		std::cout << "Warning: [OutputMessagePool::send] State != STATE_ALLOCATED_NO_AUTOSEND" << std::endl;
		#endif
	}
}

void OutputMessagePool::sendAll()
{
	boost::recursive_mutex::scoped_lock lockClass(m_outputPoolLock);
	OutputMessageMessageList::iterator it;

	for(it = m_toAddQueue.begin(); it != m_toAddQueue.end();){
		//drop messages that are older than 10 seconds
		if(OTSYS_TIME() - (*it)->getFrame() > 10 * 1000){
			(*it)->getProtocol()->onSendMessage(*it);
			it = m_toAddQueue.erase(it);
			continue;
		}

		(*it)->setState(OutputMessage::STATE_ALLOCATED);
		m_autoSendOutputMessages.push_back(*it);
		++it;
	}

	m_toAddQueue.clear();

	for(it = m_autoSendOutputMessages.begin(); it != m_autoSendOutputMessages.end();){
		OutputMessage_ptr omsg = *it;
		#ifdef __NO_PLAYER_SENDBUFFER__
		//use this define only for debugging
		bool v = 1;
		#else
		//It will send only messages bigger then 1 kb or with a lifetime greater than 10 ms
		bool v = omsg->getMessageLength() > 1024 || (m_frameTime - omsg->getFrame() > 10);
		#endif
		if(v){
			#ifdef __DEBUG_NET_DETAIL__
			std::cout << "Sending message - ALL" << std::endl;
			#endif

			if(omsg->getConnection()){
				if(!omsg->getConnection()->send(omsg)){
					// Send only fails when connection is closing (or in error state)
					// This call will free the message
					omsg->getProtocol()->onSendMessage(omsg);
				}
			}
			else{
				#ifdef __DEBUG_NET__
				std::cout << "Error: [OutputMessagePool::send] NULL connection." << std::endl;
				#endif
			}

			it = m_autoSendOutputMessages.erase(it);
		}
		else{
			++it;
		}
	}
}

void OutputMessagePool::stop()
{
	m_isOpen = false;
}

void OutputMessagePool::releaseMessage(OutputMessage* msg)
{
	g_dispatcher.addTask(
		createTask(boost::bind(&OutputMessagePool::internalReleaseMessage, this, msg)), true);
}

void OutputMessagePool::internalReleaseMessage(OutputMessage* msg)
{
	if(msg->getProtocol()){
		msg->getProtocol()->unRef();
#ifdef __DEBUG_NET_DETAIL__
		std::cout << "Removing reference to protocol " << msg->getProtocol() << std::endl;
#endif
	}
	else{
		std::cout << "No protocol found." << std::endl;
	}

	if(msg->getConnection()){
		msg->getConnection()->unRef();
#ifdef __DEBUG_NET_DETAIL__
		std::cout << "Removing reference to connection " << msg->getConnection() << std::endl;
#endif
	}
	else{
		std::cout << "No connection found." << std::endl;
	}

	msg->freeMessage();

#ifdef __TRACK_NETWORK__
	msg->clearTrack();
#endif
	
	m_outputPoolLock.lock();
	m_outputMessages.push_back(msg);
	m_outputPoolLock.unlock();
}

OutputMessage_ptr OutputMessagePool::getOutputMessage(Protocol* protocol, bool autosend /*= true*/)
{
	#ifdef __DEBUG_NET_DETAIL__
	std::cout << "request output message - auto = " << autosend << std::endl;
	#endif

	if(!m_isOpen){
		return OutputMessage_ptr();
	}

	boost::recursive_mutex::scoped_lock lockClass(m_outputPoolLock);

	if(!protocol->getConnection()){
		return OutputMessage_ptr();
	}

	if(m_outputMessages.empty()){
		OutputMessage* msg = new OutputMessage();
		m_outputMessages.push_back(msg);

#ifdef __ENABLE_SERVER_DIAGNOSTIC__
	OutputMessagePoolCount++;
#endif

#ifdef __TRACK_NETWORK__
		m_allOutputMessages.push_back(msg);
#endif
	}

	OutputMessage_ptr outputmessage;
	outputmessage.reset(m_outputMessages.back(),
		boost::bind(&OutputMessagePool::releaseMessage, this, _1));

	m_outputMessages.pop_back();

	configureOutputMessage(outputmessage, protocol, autosend);
	return outputmessage;
}

void OutputMessagePool::startExecutionFrame()
{
	m_frameTime = OTSYS_TIME();
	m_isOpen = true;
}

size_t OutputMessagePool::getAvailableMessageCount() const
{
	return m_outputMessages.size();
}

size_t OutputMessagePool::getAutoMessageCount() const
{
	return m_autoSendOutputMessages.size();
}

void OutputMessagePool::configureOutputMessage(OutputMessage_ptr msg, Protocol* protocol, bool autosend)
{
	TRACK_MESSAGE(msg);
	msg->Reset();
	if(autosend){
		msg->setState(OutputMessage::STATE_ALLOCATED);
		m_autoSendOutputMessages.push_back(msg);
	}
	else{
		msg->setState(OutputMessage::STATE_ALLOCATED_NO_AUTOSEND);
	}

	Connection_ptr connection = protocol->getConnection();
	assert(connection != NULL);

	msg->setProtocol(protocol);
	protocol->addRef();
#ifdef __DEBUG_NET_DETAIL__
	std::cout << "Adding reference to protocol - " << protocol << std::endl;
#endif
	msg->setConnection(connection);
	connection->addRef();
#ifdef __DEBUG_NET_DETAIL__
	std::cout << "Adding reference to connection - " << connection << std::endl;
#endif
	msg->setFrame(m_frameTime);
}

void OutputMessagePool::addToAutoSend(OutputMessage_ptr msg)
{
	m_outputPoolLock.lock();
	m_toAddQueue.push_back(msg);
	m_outputPoolLock.unlock();
}

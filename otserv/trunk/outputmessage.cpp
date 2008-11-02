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
#include "connection.h"
#include "protocol.h"

OutputMessage::OutputMessage()
{
	freeMessage();
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

void OutputMessagePool::startExecutionFrame()
{
	//boost::recursive_mutex::scoped_lock lockClass(m_outputPoolLock);
	m_frameTime = OTSYS_TIME();
}

OutputMessagePool::~OutputMessagePool()
{
	OutputMessageVector::iterator it;
	for(it = m_outputMessages.begin(); it != m_outputMessages.end(); ++it){
		delete *it;
	}
	m_outputMessages.clear();
}

void OutputMessagePool::send(OutputMessage* msg)
{
	m_outputPoolLock.lock();
	OutputMessage::OutputMessageState state = msg->getState();
	m_outputPoolLock.unlock();

	if(state == OutputMessage::STATE_ALLOCATED_NO_AUTOSEND){
		#ifdef __DEBUG_NET_DETAIL__
		std::cout << "Sending message - SINGLE" << std::endl;
		#endif

		if(msg->getConnection()){
			if(msg->getConnection()->send(msg)){
				// Note: if we ever decide to change how the pool works this will have to change
				m_outputPoolLock.lock();
				if(msg->getState() != OutputMessage::STATE_FREE) {
					msg->setState(OutputMessage::STATE_WAITING);
				}
				m_outputPoolLock.unlock();
			}
			else{
				msg->getProtocol()->onSendMessage(msg);
				internalReleaseMessage(msg);
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
	OutputMessageVector::iterator it;
	for(it = m_autoSendOutputMessages.begin(); it != m_autoSendOutputMessages.end(); ){
		#ifdef __NO_PLAYER_SENDBUFFER__
		//use this define only for debugging
		bool v = 1;
		#else
		//It will send only messages bigger then 1 kb or with a lifetime greater than 10 ms
		bool v = (*it)->getMessageLength() > 1024 || (m_frameTime - (*it)->getFrame() > 10);
		#endif
		if(v){
			#ifdef __DEBUG_NET_DETAIL__
			std::cout << "Sending message - ALL" << std::endl;
			#endif

			if((*it)->getConnection()){
				if((*it)->getConnection()->send(*it)){
					// Note: if we ever decide to change how the pool works this will have to change
					if((*it)->getState() != OutputMessage::STATE_FREE) {
						(*it)->setState(OutputMessage::STATE_WAITING);
					}
				}
				else{
					(*it)->getProtocol()->onSendMessage((*it));
					internalReleaseMessage(*it);
				}
			}
			else{
				#ifdef __DEBUG_NET__
				std::cout << "Error: [OutputMessagePool::send] NULL connection." << std::endl;
				#endif
			}

			m_autoSendOutputMessages.erase(it++);
		}
		else{
			++it;
		}
	}
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
	m_outputMessages.push_back(msg);
}

void OutputMessagePool::releaseMessage(OutputMessage* msg, bool sent /*= false*/)
{
	boost::recursive_mutex::scoped_lock lockClass(m_outputPoolLock);
	switch(msg->getState()){
	case OutputMessage::STATE_ALLOCATED:
	{
		OutputMessageVector::iterator it =
			std::find(m_autoSendOutputMessages.begin(), m_autoSendOutputMessages.end(), msg);
		if(it != m_autoSendOutputMessages.end()){
			m_autoSendOutputMessages.erase(it);
		}
		internalReleaseMessage(msg);
		break;
	}
	case OutputMessage::STATE_ALLOCATED_NO_AUTOSEND:
		internalReleaseMessage(msg);
		break;
	case OutputMessage::STATE_WAITING:
		if(!sent){
			std::cout << "Error: [OutputMessagePool::releaseMessage] Releasing STATE_WAITING OutputMessage." << std::endl;
		}
		else{
			internalReleaseMessage(msg);
		}
		break;
	case OutputMessage::STATE_FREE:
		std::cout << "Error: [OutputMessagePool::releaseMessage] Releasing STATE_FREE OutputMessage." << std::endl;
		break;
	default:
		std::cout << "Error: [OutputMessagePool::releaseMessage] Releasing STATE_?(" << msg->getState() <<") OutputMessage." << std::endl;
		break;
	}
}

OutputMessage* OutputMessagePool::getOutputMessage(Protocol* protocol, bool autosend /*= true*/)
{
	#ifdef __DEBUG_NET_DETAIL__
	std::cout << "request output message - auto = " << autosend << std::endl;
	#endif

	boost::recursive_mutex::scoped_lock lockClass(m_outputPoolLock);

	if(protocol->getConnection() == NULL){
		return NULL;
	}

	OutputMessage* outputmessage;
	if(m_outputMessages.empty()) {
#ifdef __TRACK_NETWORK__
		if(m_allOutputMessages.size() >= 5000){
			std::cout << "High usage of outputmessages: " << std::endl;
			m_allOutputMessages.back()->PrintTrace();
		}
#endif
		outputmessage = new OutputMessage;

#ifdef __TRACK_NETWORK__
		m_allOutputMessages.push_back(outputmessage);
#endif
	} else {
		outputmessage = m_outputMessages.back();
#ifdef __TRACK_NETWORK__
		// Print message trace
		if(outputmessage->getState() != OutputMessage::STATE_FREE) {
			std::cout << "Using allocated message, message trace:" << std::endl;
			outputmessage->PrintTrace();
		}
#else
		assert(outputmessage->getState() == OutputMessage::STATE_FREE);
#endif
		m_outputMessages.pop_back();
	}

	configureOutputMessage(outputmessage, protocol, autosend);
	return outputmessage;
}

void OutputMessagePool::configureOutputMessage(OutputMessage* msg, Protocol* protocol, bool autosend)
{
	msg->Reset();
	if(autosend){
		msg->setState(OutputMessage::STATE_ALLOCATED);
		m_autoSendOutputMessages.push_back(msg);
	}
	else{
		msg->setState(OutputMessage::STATE_ALLOCATED_NO_AUTOSEND);
	}

	Connection* connection = protocol->getConnection();
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

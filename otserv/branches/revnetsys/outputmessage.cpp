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
	m_outputBufferStart = 2;
}

//*********** OutputMessagePool ****************

OutputMessagePool::OutputMessagePool()
{
	OTSYS_THREAD_LOCKVARINIT(m_outputPoolLock);
	for(uint32_t i = 0; i < OUTPUT_POOL_SIZE; ++i){
		m_outputMessages.push_back(new OutputMessage);
	}
	m_frame = 1;
}

void OutputMessagePool::startExecutionFrame()
{
	OTSYS_THREAD_LOCK_CLASS lockClass(m_outputPoolLock);
	m_frame++;
}

OutputMessagePool::~OutputMessagePool()
{
	OutputMessageVector::iterator it;
	for(it = m_outputMessages.begin(); it != m_outputMessages.end(); ++it){
		delete *it;
	}
	m_outputMessages.clear();
	OTSYS_THREAD_LOCKVARRELEASE(m_outputPoolLock);
}

void OutputMessagePool::send(OutputMessage* msg)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(m_outputPoolLock);
	if(msg->getState() == OutputMessage::STATE_ALLOCATED_NO_AUTOSEND){
		std::cout << "Sending message - SINGLE" << std::endl;
		msg->writeMessageLength();
		if(msg->getConnection()){
			msg->getConnection()->send(msg);
		}
		else{
			#ifdef __DEBUG_NET__
			std::cout << "Error: [OutputMessagePool::send] NULL connection." << std::endl;
			#endif
		}
		msg->setState(OutputMessage::STATE_WAITING);
	}
	else{
		#ifdef __DEBUG_NET__
		std::cout << "Warning: [OutputMessagePool::send] State != STATE_ALLOCATED_NO_AUTOSEND" << std::endl;
		#endif
	}
}

void OutputMessagePool::sendAll()
{
	OTSYS_THREAD_LOCK_CLASS lockClass(m_outputPoolLock);
	OutputMessageVector::iterator it;
	for(it = m_outputMessages.begin(); it != m_outputMessages.end(); ++it){
		if((*it)->getState() == OutputMessage::STATE_ALLOCATED){
			std::cout << "Sending message - ALL" << std::endl;
			if((*it)->getFrame() != m_frame){
				#ifdef __DEBUG_NET__
				std::cout << "Error: [OutputMessagePool::send] Trying to send message out of frame." << (*it)->getFrame() << " Current: " << m_frame << std::endl;
				#endif
				(*it)->freeMessage();
				return;		
			}
			(*it)->writeMessageLength();
			if((*it)->getConnection()){
				(*it)->getConnection()->send(*it);
			}
			else{
				#ifdef __DEBUG_NET__
				std::cout << "Error: [OutputMessagePool::send] NULL connection." << std::endl;
				#endif
			}
			(*it)->setState(OutputMessage::STATE_WAITING);
		}
	}
}

OutputMessage* OutputMessagePool::getOutputMessage(Protocol* protocol, bool autosend /*= true*/)
{
	#ifdef __DEBUG_NET__
	if(protocol->getConnection() == NULL){
		std::cout << "Warning: [OutputMessagePool::getOutputMessage] NULL connection." << std::endl;
	}
	#endif
		
	OTSYS_THREAD_LOCK_CLASS lockClass(m_outputPoolLock);
	OutputMessageVector::iterator it;
	for(it = m_outputMessages.begin(); it != m_outputMessages.end(); ++it){
		if((*it)->getState() == OutputMessage::STATE_FREE){
			configureOutputMessage(*it, protocol, autosend);
			return *it;
		}
	}

	OutputMessage* outputmessage = new OutputMessage;
	configureOutputMessage(outputmessage, protocol, autosend);
	m_outputMessages.push_back(outputmessage);
	return outputmessage;
}

void OutputMessagePool::configureOutputMessage(OutputMessage* msg, Protocol* protocol, bool autosend)
{
	msg->Reset();
	if(autosend){
		msg->setState(OutputMessage::STATE_ALLOCATED);
	}
	else{
		msg->setState(OutputMessage::STATE_ALLOCATED_NO_AUTOSEND);
	}
	msg->setConnection(protocol->getConnection());
	msg->setFrame(m_frame);
}

void OutputMessagePool::writeHandler(OutputMessage* msg, const boost::system::error_code& error)
{
	std::cout << "Write handler" << std::endl;
	Connection* connection = msg->getConnection();
	connection->onWriteOperation(error);
	msg->freeMessage();
}

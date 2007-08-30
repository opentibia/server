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
#include <boost/asio.hpp>

#include "networkmessage.h"
#include "otsystem.h"
#include <list>
#include "protocol.h"
#include "connection.h"

#define OUTPUT_POOL_SIZE 100

class OutputMessage : public NetworkMessage
{
private:
	OutputMessage() {
		m_protocol = NULL;
		m_connection = NULL;
		m_state = STATE_FREE;
		m_outputBufferStart = 2;
	}
	
public:
	~OutputMessage() {}
	
	char* getOutputBuffer() { return (char*)&m_MsgBuf[m_outputBufferStart];}
	void setBufferStart(uint32_t start) {m_outputBufferStart = start;}
	
	void addCryptoHeader(){
		*(uint16_t*)(m_MsgBuf) = m_MsgSize;
		m_MsgSize = m_MsgSize + 2;
		m_outputBufferStart = 0;
	}
	
	enum OutputMessageState{
		STATE_FREE,
		STATE_ALLOCATED,
		STATE_WAITING
	};
	
protected:
	
	friend class OutputMessagePool;
	
	void setProtocol(Protocol* protocol){ m_protocol = protocol;}
	Protocol* getProtocol() { return m_protocol;}
	
	void setConnection(Connection* connection){ m_connection = connection;}
	Connection* getConnection() { return m_connection;}
	
	void setState(OutputMessageState state) { m_state = state;}
	OutputMessageState getState() const { return m_state;}
	
	Protocol* m_protocol;
	Connection* m_connection;
	
	uint32_t m_outputBufferStart;
	
	OutputMessageState m_state;
};

class OutputMessagePool
{
private:
	OutputMessagePool()
	{
		OTSYS_THREAD_LOCKVARINIT(m_outputPoolLock);
		for(uint32_t i = 0; i < OUTPUT_POOL_SIZE; ++i){
			m_outputMessages.push_back(new OutputMessage);
		}
	}

public:
	~OutputMessagePool()
	{
		OutputMessageVector::iterator it;
		for(it = m_outputMessages.begin(); it != m_outputMessages.end(); ++it){
			delete *it;
		}
		m_outputMessages.clear();
		OTSYS_THREAD_LOCKVARRELEASE(m_outputPoolLock);
	}
	
	static OutputMessagePool* getInstance()
	{
		static OutputMessagePool instance;
		return &instance;
	}
	
	void send(OutputMessage* msg)
	{
		OTSYS_THREAD_LOCK_CLASS lockClass(m_outputPoolLock);
		if(msg->getState() == OutputMessage::STATE_ALLOCATED){
			std::cout << "Sending message" << std::endl;
			msg->writeMessageLength();
			Protocol* protocol = msg->getProtocol();
			protocol->onSendMessage(msg);
			#ifdef __DEBUG_NET__
			if(protocol->getConnection() == NULL){
				std::cout << "Error: [OutputMessagePool::send] NULL connection." << std::endl;
			}
			#endif
			msg->getConnection()->send(msg);
			msg->setState(OutputMessage::STATE_WAITING);
		}
	}
	
	void sendAll()
	{
		OTSYS_THREAD_LOCK_CLASS lockClass(m_outputPoolLock);
		OutputMessageVector::iterator it;
		for(it = m_outputMessages.begin(); it != m_outputMessages.end(); ++it){
			if((*it)->getState() == OutputMessage::STATE_ALLOCATED){
				(*it)->writeMessageLength();
				Protocol* protocol = (*it)->getProtocol();
				#ifdef __DEBUG_NET__
				if(protocol->getConnection() == NULL){
					std::cout << "Error: [OutputMessagePool::sendAll] NULL connection." << std::endl;
				}
				#endif
				protocol->onSendMessage(*it);
				(*it)->getConnection()->send(*it);
				(*it)->setState(OutputMessage::STATE_WAITING);
			}
		}
	}
	
	OutputMessage* getOutputMessage(Protocol* protocol)
	{
		#ifdef __DEBUG_NET__
		if(protocol->getConnection() == NULL){
			std::cout << "Error: [OutputMessagePool::getOutputMessage] NULL connection." << std::endl;
		}
		#endif
		
		OTSYS_THREAD_LOCK_CLASS lockClass(m_outputPoolLock);
		OutputMessageVector::iterator it;
		for(it = m_outputMessages.begin(); it != m_outputMessages.end(); ++it){
			if((*it)->getState() == OutputMessage::STATE_FREE){
				(*it)->Reset();
				(*it)->setState(OutputMessage::STATE_ALLOCATED);
				(*it)->setProtocol(protocol);
				(*it)->setConnection(protocol->getConnection());
				return *it;
			}
		}

		OutputMessage* outputmessage = new OutputMessage;
		outputmessage->setState(OutputMessage::STATE_ALLOCATED);
		outputmessage->setProtocol(protocol);
		outputmessage->setConnection(protocol->getConnection());
		m_outputMessages.push_back(outputmessage);
		return outputmessage;
	}

protected:
	
	static void writeHandler(OutputMessage* msg, const boost::asio::error& error)
	{
		OTSYS_THREAD_LOCK_CLASS lockClass(getInstance()->m_outputPoolLock);
		std::cout << "Write handler" << std::endl;
		Connection* connection = msg->getConnection();
		connection->onWriteOperation(error);
		msg->setState(OutputMessage::STATE_FREE);
		msg->setProtocol(NULL);
		msg->setConnection(NULL);
	}
	
	friend class Connection;

	typedef std::list<OutputMessage*> OutputMessageVector;
	
	OutputMessageVector m_outputMessages;
	OTSYS_THREAD_LOCKVAR m_outputPoolLock;
};

#endif

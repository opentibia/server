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

using namespace boost;



#define OUTPUT_POOL_SIZE 100

class OutputMessage : public NetworkMessage
{
private:
	OutputMessage() {
		m_protocol = NULL;
		m_state = STATE_FREE;
		m_outputBufferStart = 2;
	}
	
public:
	~OutputMessage() {}
	
	char* getOutputBuffer() { return (char*)&m_MsgBuf[m_outputBufferStart];}
	void setBufferStart(uint32_t start) {m_outputBufferStart = start;}
	
	void addCryptoHeader(){		
		m_MsgSize = m_MsgSize + 2;
		*(uint16_t*)(m_MsgBuf) = m_MsgSize;
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
	
	void setState(OutputMessageState state) { m_state = state;}
	OutputMessageState getState() const { return m_state;}
	
	Protocol* m_protocol;
	
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
			msg->writeMessageLength();
			Protocol* protocol = msg->getProtocol();
			protocol->onSendMessage(msg);
			protocol->getConnection()->send(msg);
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
				protocol->onSendMessage(*it);
				protocol->getConnection()->send(*it);
				(*it)->setState(OutputMessage::STATE_WAITING);
			}
		}
	}
	
	OutputMessage* getOutputMessage(Protocol* protocol)
	{
		OTSYS_THREAD_LOCK_CLASS lockClass(m_outputPoolLock);
		OutputMessageVector::iterator it;
		for(it = m_outputMessages.begin(); it != m_outputMessages.end(); ++it){
			if((*it)->getState() == OutputMessage::STATE_FREE){
				(*it)->setState(OutputMessage::STATE_ALLOCATED);
				(*it)->setProtocol(protocol);
				return *it;
			}
		}

		OutputMessage* outputmessage = new OutputMessage;
		outputmessage->setState(OutputMessage::STATE_ALLOCATED);
		outputmessage->setProtocol(protocol);
		m_outputMessages.push_back(outputmessage);
		return outputmessage;
	}

protected:
	
	static void writeHandler(OutputMessage* msg, const asio::error& error)
	{
		OTSYS_THREAD_LOCK_CLASS lockClass(getInstance()->m_outputPoolLock);
		Connection* connection = msg->getProtocol()->getConnection();
		connection->onWriteOperation(error);
		msg->setState(OutputMessage::STATE_FREE);
	}
	
	friend class Connection;

	typedef std::list<OutputMessage*> OutputMessageVector;
	
	OutputMessageVector m_outputMessages;
	OTSYS_THREAD_LOCKVAR m_outputPoolLock;
};

#endif

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

#if defined __WINDOWS__ || defined WIN32
#include <winerror.h>
#endif

#include "server.h"
#include "scheduler.h"

#include "connection.h"
#include "outputmessage.h"


///////////////////////////////////////////////////////////////////////////////
// Service

ServiceManager::ServiceManager()
	: m_io_service()
{
}

ServiceManager::~ServiceManager()
{
	stop();
}

std::list<uint16_t> ServiceManager::get_ports() const
{
	std::list<uint16_t> ports;
	for(std::map<uint16_t, ServicePort_ptr>::const_iterator it = m_acceptors.begin();
		it != m_acceptors.end(); ++it)
		ports.push_back(it->first);
	// Maps are ordered, so the elements are in order
	//ports.sort();
	ports.unique();
	return ports;
}

void ServiceManager::stop()
{
	for(std::map<uint16_t, ServicePort_ptr>::iterator it = m_acceptors.begin();
		it != m_acceptors.end(); ++it)
	{
		m_io_service.post(boost::bind(&ServicePort::onStopServer, it->second.get()));
	}
	OutputMessagePool::getInstance()->stop();
}

///////////////////////////////////////////////////////////////////////////////
// ServicePort

ServicePort::ServicePort(boost::asio::io_service& io_service) :
	m_io_service(io_service),
	m_acceptor(NULL),
	m_listenErrors(0),
	m_serverPort(0),
	m_pendingStart(false)
{
}

ServicePort::~ServicePort()
{
	close();
}

void ServicePort::accept()
{
	if(!m_acceptor){
#ifdef __DEBUG_NET__
		std::cout << "Error: [ServerPort::accept] NULL m_acceptor." << std::endl;
#endif
		return;
	}

	Connection* connection = ConnectionManager::getInstance()->createConnection(m_io_service, shared_from_this());

	m_acceptor->async_accept(connection->getHandle(),
		boost::bind(&ServicePort::onAccept, this, connection,
		boost::asio::placeholders::error));
}

void ServicePort::onAccept(Connection* connection, const boost::system::error_code& error)
{
	if(!error){
		if(m_services.empty()){
#ifdef __DEBUG_NET__
			std::cout << "Error: [ServerPort::accept] No services running!" << std::endl;
#endif
			return;
		}

		if(m_services.front()->is_single_socket()){
			// Only one handler, and it will send first
			connection->acceptConnection(m_services.front()->make_protocol(connection));
		}
		else{
			connection->acceptConnection();
		}

#ifdef __DEBUG_NET_DETAIL__
		std::cout << "accept - OK" << std::endl;
#endif
		accept();
	}
	else{
		if(error != boost::asio::error::operation_aborted){
			close();
			if(m_listenErrors > 99){
				std::cout << "Error: [Server::handle] More than 100 listen errors." << std::endl;
				return;
			}

			m_listenErrors++;
			if(!m_pendingStart){
				m_pendingStart = true;
				Scheduler::getScheduler().addEvent(createSchedulerTask(5000,
					boost::bind(&ServicePort::open, this, m_serverPort)));
			}
		}
		else{
			#ifdef __DEBUG_NET__
			std::cout << "Error: [ServerPort::onAccept] Operation aborted." << std::endl;
			#endif
		}
	}
}

Protocol* ServicePort::make_protocol(bool checksummed, NetworkMessage& msg) const
{
	uint8_t protocolId = msg.GetByte();
	for(std::vector<Service_ptr>::const_iterator it = m_services.begin(); it != m_services.end(); ++it)
	{
		Service_ptr service = *it;
		if(service->get_protocol_identifier() == protocolId && ((checksummed &&
			service->is_checksummed()) || !service->is_checksummed())){
			// Correct service! Create protocol and get on with it
			return service->make_protocol(NULL);
		}

		// We can ignore the other cases, they will most likely end up in return NULL anyways.
	}

	return NULL;
}

void ServicePort::onStopServer()
{
	close();
}

void ServicePort::open(uint16_t port)
{
	m_serverPort = port;
	if(m_pendingStart)
		m_pendingStart = false;

	try {
		m_acceptor = new boost::asio::ip::tcp::acceptor(m_io_service, boost::asio::ip::tcp::endpoint(
			boost::asio::ip::address(boost::asio::ip::address_v4(INADDR_ANY)), m_serverPort), false);
	} catch(boost::system::system_error& e) {
		std::cout << "ERROR: Can only bind one socket to a specific port (" << m_serverPort << ")" << std::endl;
		std::cout << "The exact error was : " << e.what() << std::endl;
	}

	accept();
}

void ServicePort::close()
{
	if(m_acceptor){
		if(m_acceptor->is_open()){
			boost::system::error_code error;
			m_acceptor->close(error);
			if(error){
				PRINT_ASIO_ERROR("Closing listen socket");
			}
		}
		delete m_acceptor;
		m_acceptor = NULL;
	}
}

bool ServicePort::add_service(Service_ptr new_svc)
{
	for(std::vector<Service_ptr>::const_iterator svc_iter = m_services.begin(); svc_iter != m_services.end(); ++svc_iter){
		Service_ptr svc = *svc_iter;
		if(svc->is_single_socket())
			return false;
	}

	m_services.push_back(new_svc);
	return true;
}

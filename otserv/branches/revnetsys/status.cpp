//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Status
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

#include "status.h"
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include "configmanager.h"
#include <sstream>
#include "game.h"
#include "connection.h"
#include "networkmessage.h"
#include "tools.h"

#ifndef WIN32
	#define SOCKET_ERROR -1
	#define INVALID_SOCKET -1
#endif

extern ConfigManager g_config;
extern Game g_game;

#define STATUS_SERVER_VERSION "0.6.0_SVN"
//#define STATUS_SERVER_VERSION "0.6.0"
#define STATUS_SERVER_NAME "otserv"
#define STATUS_CLIENT_VERISON "7.92"

enum RequestedInfo_t{
	REQUEST_BASIC_SERVER_INFO = 1,
	REQUEST_OWNER_SERVER_INFO = 2,
	REQUEST_MISC_SERVER_INFO = 4,
	REQUEST_PLAYERS_INFO = 8,
	REQUEST_MAP_INFO = 16
};

void ProtocolStatus::onRecvFirstMessage(NetworkMessage& msg)
{
	switch(msg.GetByte()){
	//XML info protocol
	case 0xFF:
	{
		if(msg.GetRaw() == "info"){
			OutputMessage* output = OutputMessagePool::getInstance()->getOutputMessage(this, false);
			Status* status = Status::instance();
			std::string str = status->getStatusString();
			output->AddBytes(str.c_str(), str.size());
			OutputMessagePool::getInstance()->send(output);
		}
		break;
	}
	//Another ServerInfo protocol
	case 0x01:
	{	
		uint32_t requestedInfo = 0;
		if(msg.GetByte() == 1){
			requestedInfo |= REQUEST_BASIC_SERVER_INFO;
		}
		if(msg.GetByte() == 1){
			requestedInfo |= REQUEST_OWNER_SERVER_INFO;
		}
		if(msg.GetByte() == 1){
			requestedInfo |= REQUEST_MISC_SERVER_INFO;
		}
		if(msg.GetByte() == 1){
			requestedInfo |= REQUEST_PLAYERS_INFO;
		}
		if(msg.GetByte() == 1){
			requestedInfo |= REQUEST_MAP_INFO;
		}
		
		OutputMessage* output = OutputMessagePool::getInstance()->getOutputMessage(this, false);
		Status* status = Status::instance();
		status->getInfo(requestedInfo, output);
		OutputMessagePool::getInstance()->send(output);
		break;
	}
	default:
		break;
	}
	getConnection()->closeConnection();
}

void ProtocolStatus::deleteProtocolTask()
{
	std::cout << "Deleting ProtocolStatus" << std::endl;
	delete this;
}

Status::Status()
{
	this->playersonline = 0;
	this->playersmax    = 0;
	this->playerspeak   = 0;
	this->start=OTSYS_TIME();
}

void Status::addPlayer()
{
	this->playersonline++;
	if(playerspeak < playersonline)
		playerspeak = playersonline;
}

void Status::removePlayer()
{
	this->playersonline--;
}

std::string Status::getStatusString()
{
	std::string xml;

	std::stringstream ss;

	xmlDocPtr doc;
	xmlNodePtr p, root;

	doc = xmlNewDoc((const xmlChar*)"1.0");
	doc->children = xmlNewDocNode(doc, NULL, (const xmlChar*)"tsqp", NULL);
	root=doc->children;
	
	xmlSetProp(root, (const xmlChar*) "version", (const xmlChar*)"1.0");
	
	
	p=xmlNewNode(NULL,(const xmlChar*)"serverinfo");
	uint64_t running = (OTSYS_TIME() - this->start)/1000;
	ss << running;
	xmlSetProp(p, (const xmlChar*) "uptime", (const xmlChar*)ss.str().c_str());
	ss.str("");
	xmlSetProp(p, (const xmlChar*) "ip", (const xmlChar*)g_config.getString(ConfigManager::IP).c_str());
	xmlSetProp(p, (const xmlChar*) "servername", (const xmlChar*)g_config.getString(ConfigManager::SERVER_NAME).c_str());
	
	ss << g_config.getNumber(ConfigManager::PORT);
	xmlSetProp(p, (const xmlChar*) "port", (const xmlChar*)ss.str().c_str());
	ss.str("");
	
	xmlSetProp(p, (const xmlChar*) "location", (const xmlChar*)g_config.getString(ConfigManager::LOCATION).c_str());
	xmlSetProp(p, (const xmlChar*) "url", (const xmlChar*)g_config.getString(ConfigManager::URL).c_str());
	xmlSetProp(p, (const xmlChar*) "server", (const xmlChar*)STATUS_SERVER_NAME);
	xmlSetProp(p, (const xmlChar*) "version", (const xmlChar*)STATUS_SERVER_VERSION);
	xmlSetProp(p, (const xmlChar*) "client", (const xmlChar*)STATUS_CLIENT_VERISON);
	xmlAddChild(root, p);

	p=xmlNewNode(NULL,(const xmlChar*)"owner");
	xmlSetProp(p, (const xmlChar*) "name", (const xmlChar*)g_config.getString(ConfigManager::OWNER_NAME).c_str());
	xmlSetProp(p, (const xmlChar*) "email", (const xmlChar*)g_config.getString(ConfigManager::OWNER_EMAIL).c_str());
	xmlAddChild(root, p);

	p=xmlNewNode(NULL,(const xmlChar*)"players");
	ss << this->playersonline;
	xmlSetProp(p, (const xmlChar*) "online", (const xmlChar*)ss.str().c_str());
	ss.str("");
	ss << this->playersmax;
	xmlSetProp(p, (const xmlChar*) "max", (const xmlChar*)ss.str().c_str());
	ss.str("");
	ss << this->playerspeak;
	xmlSetProp(p, (const xmlChar*) "peak", (const xmlChar*)ss.str().c_str());
	ss.str("");
	xmlAddChild(root, p);
	
	p=xmlNewNode(NULL,(const xmlChar*)"monsters");
	ss << g_game.getMonstersOnline();
	xmlSetProp(p, (const xmlChar*) "total", (const xmlChar*)ss.str().c_str());
	ss.str("");
	xmlAddChild(root, p);

	p=xmlNewNode(NULL,(const xmlChar*)"map");
	xmlSetProp(p, (const xmlChar*) "name", (const xmlChar*)this->mapname.c_str());
	xmlSetProp(p, (const xmlChar*) "author", (const xmlChar*)this->mapauthor.c_str());
	xmlSetProp(p, (const xmlChar*) "width", (const xmlChar*)"");
	xmlSetProp(p, (const xmlChar*) "height", (const xmlChar*)"");
	xmlAddChild(root, p);

	xmlNewTextChild(root, NULL, (const xmlChar*)"motd", (const xmlChar*)g_config.getString(ConfigManager::MOTD).c_str());

	xmlChar *s = NULL;
	int len = 0;
	xmlDocDumpMemory(doc, (xmlChar**)&s, &len);
	
	if(s){
		xml = std::string((char*)s, len);
	}
	else{
		xml = "";
	}

	xmlFreeOTSERV(s);
	xmlFreeDoc(doc);

	return xml;
}

void Status::getInfo(uint32_t requestedInfo, OutputMessage* output)
{
	// the client selects which information may be 
	// sent back, so we'll save some bandwidth and 
	// make many
	std::stringstream ss;
	uint64_t running = (OTSYS_TIME() - this->start) / 1000;
	// since we haven't all the things on the right place like map's 
	// creator/info and other things, i'll put the info chunked into
	// operators, so the httpd server will only receive the existing
	// properties of the server, such serverinfo, playersinfo and so

	if(requestedInfo & REQUEST_BASIC_SERVER_INFO){
		output->AddByte(0x10); // server info
		output->AddString(g_config.getString(ConfigManager::SERVER_NAME).c_str());
		output->AddString(g_config.getString(ConfigManager::IP).c_str());  
		ss << g_config.getNumber(ConfigManager::PORT);
		output->AddString(ss.str().c_str());
		ss.str(""); 
  	}

	if(requestedInfo & REQUEST_OWNER_SERVER_INFO){
    	output->AddByte(0x11); // server info - owner info
		output->AddString(g_config.getString(ConfigManager::OWNER_NAME).c_str());
		output->AddString(g_config.getString(ConfigManager::OWNER_EMAIL).c_str());
  	}

	if(requestedInfo & REQUEST_MISC_SERVER_INFO){
    	output->AddByte(0x12); // server info - misc
		output->AddString(g_config.getString(ConfigManager::MOTD).c_str());
		output->AddString(g_config.getString(ConfigManager::LOCATION).c_str());
		output->AddString(g_config.getString(ConfigManager::URL).c_str());
    	output->AddU32((uint32_t)(running >> 32)); // this method prevents a big number parsing
    	output->AddU32((uint32_t)(running));       // since servers can be online for months ;)
    	output->AddString(STATUS_SERVER_VERSION);
  	}

	if(requestedInfo & REQUEST_PLAYERS_INFO){
    	output->AddByte(0x20); // players info
    	output->AddU32(this->playersonline);
	    output->AddU32(this->playersmax);
	    output->AddU32(this->playerspeak);
  	} 

  	if(requestedInfo & REQUEST_MAP_INFO){
    	output->AddByte(0x30); // map info
    	output->AddString(this->mapname.c_str());
    	output->AddString(this->mapauthor.c_str());
    	int mw, mh;
    	g_game.getMapDimensions(mw, mh);  
    	output->AddU16(mw);
    	output->AddU16(mh);
  	}

	return;
}

bool Status::hasSlot()
{
	return this->playersonline < this->playersmax;
}

/*
OTSYS_THREAD_RETURN Status::SendInfoThread(void *p)
{
	SOCKET s;
	std::string post_message;
	while(g_game.getGameState() != GAME_STATE_SHUTDOWN){
		OTSYS_SLEEP(8*60*1000); //wait 8 minutes
		
		if(g_config.getNumber(ConfigManager::OTSERV_DB_ENABLED) == 0)
			continue;
		
		std::string host = g_config.getString(ConfigManager::OTSERV_DB_HOST);
		
		uint32_t host_ip = inet_addr(host.c_str()); //is it a numeric ip?
		if(host_ip == INADDR_NONE){
			hostent* he = gethostbyname(host.c_str()); //if not use dns
			if(he != NULL){
				host_ip = *(uint32_t*)(he->h_addr_list[0]);
			}
			else{
				std::cout << "[Status::SendInfoThread] Can not resolve host ip \"" << host << "\"" << std::endl;
				continue;
        	}
    	}
    	
    	s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if(s == INVALID_SOCKET){
			std::cout << "[Status::SendInfoThread] Can not create socket." << std::endl;
			continue;
		}
    	
    	sockaddr_in host_addr;
    	host_addr.sin_family = AF_INET;
		host_addr.sin_addr.s_addr = host_ip;
		host_addr.sin_port = htons(80);
		
		if(connect(s, (const sockaddr*)&host_addr, sizeof(host_addr)) == SOCKET_ERROR){
			std::cout << "[Status::SendInfoThread] Can not connect to " << host << std::endl;
			closesocket(s);
			continue;
		}

		Status* status_instance = Status::instance();
		
		std::stringstream status;
		uint64_t running = (OTSYS_TIME() - status_instance->start)/1000;
		status << "uptime=" << running << "&"
			"ip=" << urlEncode(g_config.getString(ConfigManager::IP)) << "&"
			"servername=" << urlEncode(g_config.getString(ConfigManager::SERVER_NAME)) << "&"
			"port=" << g_config.getNumber(ConfigManager::PORT) << "&"
			"location=" << urlEncode(g_config.getString(ConfigManager::LOCATION)) << "&"
			"url=" << urlEncode(g_config.getString(ConfigManager::URL)) << "&"
			"server=" << urlEncode(STATUS_SERVER_NAME) << "&"
			"version=" << urlEncode(STATUS_SERVER_VERSION) << "&"
			"client=" << urlEncode(STATUS_CLIENT_VERISON) << "&"
			"ownername=" << urlEncode(g_config.getString(ConfigManager::OWNER_NAME)) << "&"
			"owneremail=" << urlEncode(g_config.getString(ConfigManager::OWNER_EMAIL)) << "&"
			"playersonline=" << status_instance->playersonline << "&"
			"playersmax=" << status_instance->playersmax << "&"
			"playerspeak=" << status_instance->playerspeak << "&"
			"monsterstotal=" << g_game.getMonstersOnline() << "&"
			"mapname=" << urlEncode(status_instance->mapname) << "&"
			"mapauthor=" << urlEncode(status_instance->mapauthor) << "&"
			"width=" << "&"
			"heigh=" << "&"
			"motd=" << urlEncode(g_config.getString(ConfigManager::MOTD));
		
		
		char size[16] = {'0'};
		sprintf(size, "%d", (int)status.str().size());
		std::string status_length = std::string(size);
		post_message = "POST /otservdb.php HTTP/1.1\r\n"
						"Host: " + host +"\r\n"
						"User-Agent: otserv("STATUS_SERVER_NAME"/"STATUS_SERVER_VERSION")\r\n"
						"Connection: close\r\n"
						"Cache-Control: no-cache\r\n"
						"Content-Type: application/x-www-form-urlencoded\r\n"
						"Content-Length: " + status_length + "\r\n"
						"\r\n" +
						status.str();
		
		if(send(s, post_message.c_str(), post_message.size(), 0) == SOCKET_ERROR){
			std::cout << "[Status::SendInfoThread] Error while sending status to " << host << std::endl;
			closesocket(s);
			continue;
		}
		status.clear();
		
		//TODO: read server response?
		
		closesocket(s);
	}
#if defined WIN32 || defined WINDOWS
	//
#else
  return 0;
#endif
}
*/

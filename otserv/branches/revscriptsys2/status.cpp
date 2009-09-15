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
#include "outputmessage.h"
#include "game.h"
#include "player.h"
#include "configmanager.h"

#ifndef WIN32
	#define SOCKET_ERROR -1
	#define INVALID_SOCKET -1
#endif

extern ConfigManager g_config;
extern Game g_game;

enum RequestedInfo_t{
	REQUEST_BASIC_SERVER_INFO  = 0x01,
	REQUEST_OWNER_SERVER_INFO  = 0x02,
	REQUEST_MISC_SERVER_INFO   = 0x04,
	REQUEST_PLAYERS_INFO       = 0x08,
	REQUEST_MAP_INFO           = 0x10,
	REQUEST_EXT_PLAYERS_INFO   = 0x20,
	REQUEST_PLAYER_STATUS_INFO = 0x40,
	REQUEST_SERVER_SOFTWARE_INFORMATION = 0x80
};

#ifdef __ENABLE_SERVER_DIAGNOSTIC__
uint32_t ProtocolStatus::protocolStatusCount = 0;
#endif
std::map<uint32_t, int64_t> ProtocolStatus::ipConnectMap;

void ProtocolStatus::onRecvFirstMessage(NetworkMessage& msg)
{
	std::map<uint32_t, int64_t>::const_iterator it = ipConnectMap.find(getIP());
	if(it != ipConnectMap.end()){
		if(OTSYS_TIME() < it->second + g_config.getNumber(ConfigManager::STATUSQUERY_TIMEOUT)){
			getConnection()->closeConnection();
			return;
		}
	}

	ipConnectMap[getIP()] = OTSYS_TIME();

	switch(msg.GetByte()){
	//XML info protocol
	case 0xFF:
	{
		if(msg.GetRaw() == "info"){
			OutputMessage_ptr output = OutputMessagePool::getInstance()->getOutputMessage(this, false);
			if(output){
				TRACK_MESSAGE(output);
				Status* status = Status::instance();
				std::string str = status->getStatusString();
				output->AddBytes(str.c_str(), str.size());
				setRawMessages(true); // we dont want the size header, nor encryption
				OutputMessagePool::getInstance()->send(output);
			}
		}
		break;
	}
	//Another ServerInfo protocol
	case 0x01:
	{
		uint32_t requestedInfo = msg.GetU16(); //Only a Byte is necessary, though we could add new infos here

		OutputMessage_ptr output = OutputMessagePool::getInstance()->getOutputMessage(this, false);
		if(output){
			TRACK_MESSAGE(output);
			Status* status = Status::instance();
			status->getInfo(requestedInfo, output, msg);
			OutputMessagePool::getInstance()->send(output);
		}
		break;
	}
	default:
		break;
	}
	getConnection()->closeConnection();
}

#ifdef __DEBUG_NET_DETAIL__
void ProtocolStatus::deleteProtocolTask()
{
	std::cout << "Deleting ProtocolStatus" << std::endl;
	Protocol::deleteProtocolTask();
}
#endif

Status::Status()
{
	m_playersonline = 0;
	m_playersmax = 0;
	m_playerspeak = 0;
	m_start = OTSYS_TIME();
}

void Status::addPlayer()
{
	m_playersonline++;
	if(m_playerspeak < m_playersonline)
		m_playerspeak = m_playersonline;
}

void Status::removePlayer()
{
	m_playersonline--;
}

std::string Status::getStatusString() const
{
	std::string xml;

	std::stringstream ss;

	xmlDocPtr doc;
	xmlNodePtr p, root;

	doc = xmlNewDoc((const xmlChar*)"1.0");
	doc->children = xmlNewDocNode(doc, NULL, (const xmlChar*)"tsqp", NULL);
	root=doc->children;

	xmlSetProp(root, (const xmlChar*) "version", (const xmlChar*)"1.0");


	p = xmlNewNode(NULL,(const xmlChar*)"serverinfo");
	ss << getUpTime();
	xmlSetProp(p, (const xmlChar*) "uptime", (const xmlChar*)ss.str().c_str());
	ss.str("");
	xmlSetProp(p, (const xmlChar*) "ip", (const xmlChar*)g_config.getString(ConfigManager::IP).c_str());
	xmlSetProp(p, (const xmlChar*) "servername", (const xmlChar*)g_config.getString(ConfigManager::SERVER_NAME).c_str());

	ss << g_config.getNumber(ConfigManager::LOGIN_PORT);
	xmlSetProp(p, (const xmlChar*) "port", (const xmlChar*)ss.str().c_str());
	ss.str("");

	xmlSetProp(p, (const xmlChar*) "location", (const xmlChar*)g_config.getString(ConfigManager::LOCATION).c_str());
	xmlSetProp(p, (const xmlChar*) "url", (const xmlChar*)g_config.getString(ConfigManager::URL).c_str());
	xmlSetProp(p, (const xmlChar*) "server", (const xmlChar*)OTSERV_NAME);
	xmlSetProp(p, (const xmlChar*) "version", (const xmlChar*)OTSERV_VERSION);
	xmlSetProp(p, (const xmlChar*) "client", (const xmlChar*)OTSERV_CLIENT_VERSION);
	xmlAddChild(root, p);

	p = xmlNewNode(NULL,(const xmlChar*)"owner");
	xmlSetProp(p, (const xmlChar*) "name", (const xmlChar*)g_config.getString(ConfigManager::OWNER_NAME).c_str());
	xmlSetProp(p, (const xmlChar*) "email", (const xmlChar*)g_config.getString(ConfigManager::OWNER_EMAIL).c_str());
	xmlAddChild(root, p);

	p = xmlNewNode(NULL,(const xmlChar*)"players");
	ss << m_playersonline;
	xmlSetProp(p, (const xmlChar*) "online", (const xmlChar*)ss.str().c_str());
	ss.str("");
	ss << m_playersmax;
	xmlSetProp(p, (const xmlChar*) "max", (const xmlChar*)ss.str().c_str());
	ss.str("");
	ss << m_playerspeak;
	xmlSetProp(p, (const xmlChar*) "peak", (const xmlChar*)ss.str().c_str());
	ss.str("");
	xmlAddChild(root, p);

	/*
	p = xmlNewNode(NULL,(const xmlChar*)"monsters");
	ss << g_game.getMonstersOnline();
	xmlSetProp(p, (const xmlChar*) "total", (const xmlChar*)ss.str().c_str());
	ss.str("");
	xmlAddChild(root, p);
	*/

	p = xmlNewNode(NULL,(const xmlChar*)"map");
	xmlSetProp(p, (const xmlChar*) "name", (const xmlChar*)m_mapname.c_str());
	xmlSetProp(p, (const xmlChar*) "author", (const xmlChar*)m_mapauthor.c_str());

	uint32_t mapWidth, mapHeight;
	g_game.getMapDimensions(mapWidth, mapHeight);
	ss.str("");
	ss << mapWidth;
	xmlSetProp(p, (const xmlChar*) "width", (const xmlChar*)ss.str().c_str());
	ss.str("");
	ss << mapHeight;
	xmlSetProp(p, (const xmlChar*) "height", (const xmlChar*)ss.str().c_str());
	xmlAddChild(root, p);

	xmlNewTextChild(root, NULL, (const xmlChar*)"motd", (const xmlChar*)g_config.getString(ConfigManager::MOTD).c_str());

	xmlChar* s = NULL;
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

void Status::getInfo(uint32_t requestedInfo, OutputMessage_ptr output, NetworkMessage& msg) const
{
	// the client selects which information may be
	// sent back, so we'll save some bandwidth and
	// make many
	std::stringstream ss;
	uint64_t running = getUpTime();
	// since we haven't all the things on the right place like map's
	// creator/info and other things, i'll put the info chunked into
	// operators, so the httpd server will only receive the existing
	// properties of the server, such serverinfo, playersinfo and so

	if(requestedInfo & REQUEST_BASIC_SERVER_INFO){
		output->AddByte(0x10); // server info
		output->AddString(g_config.getString(ConfigManager::SERVER_NAME).c_str());
		output->AddString(g_config.getString(ConfigManager::IP).c_str());
		ss << g_config.getNumber(ConfigManager::LOGIN_PORT);
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
	}

	if(requestedInfo & REQUEST_PLAYERS_INFO){
		output->AddByte(0x20); // players info
		output->AddU32(m_playersonline);
		output->AddU32(m_playersmax);
		output->AddU32(m_playerspeak);
	}

	if(requestedInfo & REQUEST_MAP_INFO){
		output->AddByte(0x30); // map info
		output->AddString(m_mapname.c_str());
		output->AddString(m_mapauthor.c_str());
		uint32_t mapWidth, mapHeight;
		g_game.getMapDimensions(mapWidth, mapHeight);
		output->AddU16(mapWidth);
		output->AddU16(mapHeight);
	}

	if(requestedInfo & REQUEST_EXT_PLAYERS_INFO){
		output->AddByte(0x21); // players info - online players list
		output->AddU32(m_playersonline);
		for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it){
			//Send the most common info
			output->AddString(it->second->getName());
			output->AddU32(it->second->getLevel());
		}
	}

	if(requestedInfo & REQUEST_PLAYER_STATUS_INFO){
		output->AddByte(0x22); // players info - online status info of a player
		const std::string name = msg.GetString();
		if(g_game.getPlayerByName(name) != NULL){
			output->AddByte(0x01);
		}
		else{
			output->AddByte(0x00);
		}
	}

	if(requestedInfo & REQUEST_SERVER_SOFTWARE_INFORMATION){
		output->AddByte(0x23); // server software info
		output->AddString(OTSERV_NAME);
		output->AddString(OTSERV_VERSION);
		output->AddString(OTSERV_CLIENT_VERSION);
	}

	return;
}

bool Status::hasSlot() const
{
	return m_playersonline < m_playersmax;
}

uint64_t Status::getUpTime() const
{
	return (OTSYS_TIME() - m_start)/1000;
}

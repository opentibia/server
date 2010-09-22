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

#include "admin.h"
#include "game.h"
#include "connection.h"
#include "ioplayer.h"
#include "outputmessage.h"
#include "networkmessage.h"
#include "configmanager.h"
#include "house.h"
#include "ban.h"
#include "tools.h"
#include "rsa.h"
#include "mailbox.h"
#include "logger.h"

static void addLogLine(ProtocolAdmin* conn, eLogType type, int level, std::string message);

extern Game g_game;
extern ConfigManager g_config;
extern BanManager g_bans;

AdminProtocolConfig* g_adminConfig = NULL;

#ifdef __ENABLE_SERVER_DIAGNOSTIC__
uint32_t ProtocolAdmin::protocolAdminCount = 0;
#endif

ProtocolAdmin::ProtocolAdmin(Connection_ptr connection) :
Protocol(connection)
{
	m_state = NO_CONNECTED;
	m_loginTries = 0;
	m_lastCommand = 0;
	m_startTime = std::time(NULL);

#ifdef __ENABLE_SERVER_DIAGNOSTIC__
	protocolAdminCount++;
#endif
}

ProtocolAdmin::~ProtocolAdmin()
{
#ifdef __ENABLE_SERVER_DIAGNOSTIC__
	protocolAdminCount--;
#endif
}

void ProtocolAdmin::onRecvFirstMessage(NetworkMessage& msg)
{
	//is the remote admin protocol enabled?
	if(!g_adminConfig->isEnabled()){
		getConnection()->closeConnection();
		return;
	}

	m_state = NO_CONNECTED;
	//is allowed this ip?
	if(!g_adminConfig->allowIP(getIP())){
		addLogLine(this, LOGTYPE_EVENT, 1, "ip not allowed");
		getConnection()->closeConnection();
		return;
	}

	//max connections limit
	if(!g_adminConfig->addConnection()){
		addLogLine(this, LOGTYPE_EVENT, 1, "cannot add new connection");
		getConnection()->closeConnection();
		return;
	}

	addLogLine(this, LOGTYPE_EVENT, 1, "sending HELLO");
	//send hello
	OutputMessage_ptr output = OutputMessagePool::getInstance()->getOutputMessage(this, false);
	if(output){
		TRACK_MESSAGE(output);
		output->AddByte(AP_MSG_HELLO);
		output->AddU32(1); //version
		output->AddString("OTADMIN");
		output->AddU16(g_adminConfig->getProtocolPolicy()); //security policy
		output->AddU32(g_adminConfig->getProtocolOptions()); //protocol options(encryption, ...)
		OutputMessagePool::getInstance()->send(output);
	}

	m_lastCommand = std::time(NULL);
	m_state = ENCRYPTION_NO_SET;
}

void ProtocolAdmin::deleteProtocolTask()
{
	addLogLine(NULL, LOGTYPE_EVENT, 1, "end connection");
	g_adminConfig->removeConnection();
	Protocol::deleteProtocolTask();
}

void ProtocolAdmin::parsePacket(NetworkMessage& msg)
{
	if(g_game.getGameState() == GAME_STATE_SHUTDOWN){
		getConnection()->closeConnection();
		return;
	}

	uint8_t recvbyte = msg.GetByte();

	OutputMessagePool* outputPool = OutputMessagePool::getInstance();

	OutputMessage_ptr output = outputPool->getOutputMessage(this, false);
	if(output){
		TRACK_MESSAGE(output);

		switch(m_state){
		case ENCRYPTION_NO_SET:
		{
			if(g_adminConfig->requireEncryption()){
				if((time(NULL) - m_startTime) > 30000){
					getConnection()->closeConnection();
					addLogLine(this, LOGTYPE_WARNING, 1, "encryption timeout");
					return;
				}

				if(recvbyte != AP_MSG_ENCRYPTION && recvbyte != AP_MSG_KEY_EXCHANGE){
					output->AddByte(AP_MSG_ERROR);
					output->AddString("encryption needed");
					outputPool->send(output);
					getConnection()->closeConnection();
					addLogLine(this, LOGTYPE_WARNING, 1, "wrong command while ENCRYPTION_NO_SET");
					return;
				}
				break;
			}
			else{
				m_state = NO_LOGGED_IN;
			}
		}
		case NO_LOGGED_IN:
		{
			if(g_adminConfig->requireLogin()){
				if((time(NULL) - m_startTime) > 30000){
					//login timeout
					getConnection()->closeConnection();
					addLogLine(this, LOGTYPE_WARNING, 1, "login timeout");
					return;
				}

				if(m_loginTries > 3){
					output->AddByte(AP_MSG_ERROR);
					output->AddString("too many login tries");
					outputPool->send(output);
					getConnection()->closeConnection();
					addLogLine(this, LOGTYPE_WARNING, 1, "too many login tries");
					return;
				}

				if(recvbyte != AP_MSG_LOGIN){
					output->AddByte(AP_MSG_ERROR);
					output->AddString("you are not logged in");
					outputPool->send(output);
					getConnection()->closeConnection();
					addLogLine(this, LOGTYPE_WARNING, 1, "wrong command while NO_LOGGED_IN");
					return;
				}
				break;
			}
			else{
				m_state = LOGGED_IN;
			}
		}
		case LOGGED_IN:
		{
			//can execute commands
			break;
		}
		default:
			getConnection()->closeConnection();
			return;
		}

		m_lastCommand = std::time(NULL);

		switch(recvbyte){
		case AP_MSG_LOGIN:
		{
			if(m_state == NO_LOGGED_IN && g_adminConfig->requireLogin()){
				std::string password = msg.GetString();
				if(g_adminConfig->passwordMatch(password)){
					m_state = LOGGED_IN;
					output->AddByte(AP_MSG_LOGIN_OK);
					addLogLine(this, LOGTYPE_EVENT, 1, "login ok");
				}
				else{
					m_loginTries++;
					output->AddByte(AP_MSG_LOGIN_FAILED);
					output->AddString("wrong password");
					addLogLine(this, LOGTYPE_WARNING, 1, "login failed.("+ password + ")");
				}
			}
			else{
				output->AddByte(AP_MSG_LOGIN_FAILED);
				output->AddString("can not login");
				addLogLine(this, LOGTYPE_WARNING, 1, "wrong state at login");
			}
			break;
		}
		case AP_MSG_ENCRYPTION:
		{
			if(m_state == ENCRYPTION_NO_SET && g_adminConfig->requireEncryption()){
				uint8_t keyType = msg.GetByte();
				switch(keyType){
				case ENCRYPTION_RSA1024XTEA:
				{
					RSA* rsa = g_adminConfig->getRSAKey(ENCRYPTION_RSA1024XTEA);
					if(!rsa){
						output->AddByte(AP_MSG_ENCRYPTION_FAILED);
						addLogLine(this, LOGTYPE_WARNING, 1, "no valid server key type");
						break;
					}

					if(RSA_decrypt(rsa, msg)){
						m_state = NO_LOGGED_IN;
						uint32_t k[4];
						k[0] = msg.GetU32();
						k[1] = msg.GetU32();
						k[2] = msg.GetU32();
						k[3] = msg.GetU32();

						//use for in/out the new key we have
						enableXTEAEncryption();
						setXTEAKey(k);

						output->AddByte(AP_MSG_ENCRYPTION_OK);
						addLogLine(this, LOGTYPE_EVENT, 1, "encryption ok");
					}
					else{
						output->AddByte(AP_MSG_ENCRYPTION_FAILED);
						output->AddString("wrong encrypted packet");
						addLogLine(this, LOGTYPE_WARNING, 1, "wrong encrypted packet");
					}
					break;
				}
				default:
					output->AddByte(AP_MSG_ENCRYPTION_FAILED);
					output->AddString("no valid key type");
					addLogLine(this, LOGTYPE_WARNING, 1, "no valid client key type");
					break;
				}
			}
			else{
				output->AddByte(AP_MSG_ENCRYPTION_FAILED);
				output->AddString("can not set encryption");
				addLogLine(this, LOGTYPE_EVENT, 1, "can not set encryption");
			}
			break;
		}
		case AP_MSG_KEY_EXCHANGE:
		{
			if(m_state == ENCRYPTION_NO_SET && g_adminConfig->requireEncryption()){
				uint8_t keyType = msg.GetByte();
				switch(keyType){
				case ENCRYPTION_RSA1024XTEA:
				{
					RSA* rsa = g_adminConfig->getRSAKey(ENCRYPTION_RSA1024XTEA);
					if(!rsa){
						output->AddByte(AP_MSG_KEY_EXCHANGE_FAILED);
						addLogLine(this, LOGTYPE_WARNING, 1, "no valid server key type");
						break;
					}

					output->AddByte(AP_MSG_KEY_EXCHANGE_OK);
					output->AddByte(ENCRYPTION_RSA1024XTEA);
					char RSAPublicKey[128];
					rsa->getPublicKey(RSAPublicKey);
					output->AddBytes(RSAPublicKey, 128);
					break;
				}
				default:
					output->AddByte(AP_MSG_KEY_EXCHANGE_FAILED);
					addLogLine(this, LOGTYPE_WARNING, 1, "no valid client key type");
					break;
				}
			}
			else{
				output->AddByte(AP_MSG_KEY_EXCHANGE_FAILED);
				output->AddString("can not get public key");
				addLogLine(this, LOGTYPE_WARNING, 1, "can not get public key");
			}
			break;
		}
		case AP_MSG_COMMAND:
		{
			if(m_state != LOGGED_IN){
				addLogLine(this, LOGTYPE_ERROR, 1, "recvbyte == AP_MSG_COMMAND && m_state != LOGGED_IN !!!");
				//never should reach this point!!
				break;
			}
			uint8_t command = msg.GetByte();
			switch(command){
			case CMD_BROADCAST:
			{
				const std::string message = msg.GetString();
				addLogLine(this, LOGTYPE_EVENT, 1, "broadcast: " + message);
				g_dispatcher.addTask(
					createTask(boost::bind(&Game::anonymousBroadcastMessage, &g_game, MSG_STATUS_WARNING, message)));

				output->AddByte(AP_MSG_COMMAND_OK);
				break;
			}
			case CMD_OPEN_SERVER:
			{
				g_dispatcher.addTask(
					createTask(boost::bind(&ProtocolAdmin::adminCommandOpenServer, this)));

				break;
			}
			case CMD_CLOSE_SERVER:
			{
				g_dispatcher.addTask(
					createTask(boost::bind(&ProtocolAdmin::adminCommandCloseServer, this)));

				break;
			}
			case CMD_PAY_HOUSES:
			{
				g_dispatcher.addTask(
					createTask(boost::bind(&ProtocolAdmin::adminCommandPayHouses, this)));

				break;
			}
			case CMD_SHUTDOWN_SERVER:
			{
				g_dispatcher.addTask(
					createTask(boost::bind(&ProtocolAdmin::adminCommandShutdownServer, this)));
				return;
				break;
			}
			case CMD_SEND_MAIL:
			{
				const std::string xmlData = msg.GetString();
				g_dispatcher.addTask(
					createTask(boost::bind(&ProtocolAdmin::adminCommandSendMail, this, xmlData)));
				break;
			}
			case CMD_KICK:
			{
				const std::string name = msg.GetString();
				g_dispatcher.addTask(
					createTask(boost::bind(&ProtocolAdmin::adminCommandKickPlayer, this, name)));
				break;
			}
			case CMD_SHALLOW_SAVE_SERVER:
			case CMD_SAVE_SERVER:
			{
				g_dispatcher.addTask(
					createTask(boost::bind(&ProtocolAdmin::adminCommandSaveServer, this, command == CMD_SHALLOW_SAVE_SERVER)));

				break;
			}
			case CMD_RELATIONAL_SAVE_SERVER:
			{
				g_dispatcher.addTask(
					createTask(boost::bind(&ProtocolAdmin::adminCommandRelationalSaveServer, this)));

				break;
			}
			default:
			{
				output->AddByte(AP_MSG_COMMAND_FAILED);
				output->AddString("not known server command");
				addLogLine(this, LOGTYPE_WARNING, 1, "not known server command");
			}
			};
			break;
		}

		case AP_MSG_PING:
			output->AddByte(AP_MSG_PING_OK);
			break;

		case AP_MSG_KEEP_ALIVE:
			break;

		default:
			output->AddByte(AP_MSG_ERROR);
			output->AddString("not known command byte");
			addLogLine(this, LOGTYPE_WARNING, 1, "not known command byte");
			break;
		};
		if(output->getMessageLength() > 0){
			outputPool->send(output);
		}
	}
}

void ProtocolAdmin::adminCommandOpenServer()
{
	g_game.setGameState(GAME_STATE_NORMAL);
	addLogLine(this, LOGTYPE_EVENT, 1, "open server ok");

	OutputMessage_ptr output = OutputMessagePool::getInstance()->getOutputMessage(this, false);
	if(output){
		TRACK_MESSAGE(output);
		output->AddByte(AP_MSG_COMMAND_OK);
		OutputMessagePool::getInstance()->send(output);
	}
}

void ProtocolAdmin::adminCommandCloseServer()
{
	g_game.setGameState(GAME_STATE_CLOSED);
	addLogLine(this, LOGTYPE_EVENT, 1, "close server ok");

	AutoList<Player>::listiterator it = Player::listPlayer.list.begin();
	while(it != Player::listPlayer.list.end()){
		if(!(*it).second->hasFlag(PlayerFlag_CanAlwaysLogin)){
			(*it).second->kickPlayer();
			it = Player::listPlayer.list.begin();
		}
		else{
			++it;
		}
	}

	OutputMessage_ptr output = OutputMessagePool::getInstance()->getOutputMessage(this, false);
	if(output){
		TRACK_MESSAGE(output);
		output->AddByte(AP_MSG_COMMAND_OK);
		OutputMessagePool::getInstance()->send(output);
	}
}

void ProtocolAdmin::adminCommandShutdownServer()
{
	g_game.setGameState(GAME_STATE_SHUTDOWN);
	addLogLine(this, LOGTYPE_EVENT, 1, "start server shutdown");

	OutputMessage_ptr output = OutputMessagePool::getInstance()->getOutputMessage(this, false);
	if(output){
		TRACK_MESSAGE(output);
		output->AddByte(AP_MSG_COMMAND_OK);
		OutputMessagePool::getInstance()->send(output);
	}
}

void ProtocolAdmin::adminCommandPayHouses()
{
	Houses::getInstance().payHouses();
	addLogLine(this, LOGTYPE_EVENT, 1, "pay houses ok");

	OutputMessage_ptr output = OutputMessagePool::getInstance()->getOutputMessage(this, false);
	if(output){
		TRACK_MESSAGE(output);
		output->AddByte(AP_MSG_COMMAND_OK);
		OutputMessagePool::getInstance()->send(output);
	}
}

Item* ProtocolAdmin::createMail(const std::string xmlData, std::string& name, uint32_t& depotId)
{
	xmlDocPtr doc = xmlParseMemory(xmlData.c_str(), strlen(xmlData.c_str()));
	if(!doc){
		return NULL;
	}

	xmlNodePtr root = xmlDocGetRootElement(doc);

	if(xmlStrcmp(root->name,(const xmlChar*)"mail") != 0){
		return NULL;
	}

	int32_t itemId = ITEM_PARCEL;

	int32_t intValue;
	std::string strValue;

	if(readXMLString(root, "to", strValue)){
		name = strValue;
	}

	if(readXMLString(root, "town", strValue)){
		if(!Mailbox::getDepotId(strValue, depotId)){
			return false;
		}
	}
	else{
		//use the players default town
		if(!IOPlayer::instance()->getDefaultTown(name, depotId)){
			return false;
		}
	}

	if(readXMLInteger(root, "id", intValue)){
		itemId = intValue;
	}

	Item* mailItem = Item::CreateItem(itemId);
	mailItem->setParent(VirtualCylinder::virtualCylinder);

	if(Container* mailContainer = mailItem->getContainer()){
		xmlNodePtr node = root->children;
		while(node){
			if(node->type != XML_ELEMENT_NODE){
				node = node->next;
				continue;
			}

			if(!Item::loadItem(node, mailContainer)){
				delete mailContainer;
				return NULL;
			}

			node = node->next;
		}
	}

	return mailItem;
}

void ProtocolAdmin::adminCommandSendMail(const std::string& xmlData)
{
	OutputMessage_ptr output = OutputMessagePool::getInstance()->getOutputMessage(this, false);
	if(output){
		TRACK_MESSAGE(output);

		std::string name;
		uint32_t depotId;
		Item* mailItem = createMail(xmlData, name, depotId);

		if(mailItem){
			if(Mailbox::sendItemTo(name, depotId, mailItem)){
				output->AddByte(AP_MSG_COMMAND_OK);
			}
			else{
				output->AddByte(AP_MSG_COMMAND_FAILED);
				output->AddString("Could not mail item");
			}
		}
		else{
			output->AddByte(AP_MSG_COMMAND_FAILED);
			output->AddString("Could not mail item");
		}

		OutputMessagePool::getInstance()->send(output);
	}
}

void ProtocolAdmin::adminCommandKickPlayer(const std::string& name)
{
	OutputMessage_ptr output = OutputMessagePool::getInstance()->getOutputMessage(this, false);
	if(output){
		TRACK_MESSAGE(output);

		Player* player = g_game.getPlayerByName(name);
		if(player){
			player->kickPlayer();
			addLogLine(this, LOGTYPE_EVENT, 1, "kicked player " + name);

			output->AddByte(AP_MSG_COMMAND_OK);
		}
		else{
			addLogLine(this, LOGTYPE_WARNING, 1, "Could not kick player(not online): " + name);

			output->AddByte(AP_MSG_COMMAND_FAILED);
			output->AddString("player is not online");
		}
		OutputMessagePool::getInstance()->send(output);
	}
}

void ProtocolAdmin::adminCommandSaveServer(bool shallow)
{
	g_game.saveServer(false, shallow);
	addLogLine(this, LOGTYPE_EVENT, 1, "save server ok");

	OutputMessage_ptr output = OutputMessagePool::getInstance()->getOutputMessage(this, false);
	if(output){
		TRACK_MESSAGE(output);
		output->AddByte(AP_MSG_COMMAND_OK);
		OutputMessagePool::getInstance()->send(output);
	}
}

void ProtocolAdmin::adminCommandRelationalSaveServer()
{
	std::string old_type = g_config.getString(ConfigManager::MAP_STORAGE_TYPE);
	g_config.setString(ConfigManager::MAP_STORAGE_TYPE, "relational");
	g_game.saveServer(false);
	g_config.setString(ConfigManager::MAP_STORAGE_TYPE, old_type);
	addLogLine(this, LOGTYPE_EVENT, 1, "relational save server ok");

	OutputMessage_ptr output = OutputMessagePool::getInstance()->getOutputMessage(this, false);
	if(output){
		TRACK_MESSAGE(output);
		output->AddByte(AP_MSG_COMMAND_OK);
		OutputMessagePool::getInstance()->send(output);
	}
}

/////////////////////////////////////////////

AdminProtocolConfig::AdminProtocolConfig()
{
	m_enabled = true;
	m_onlyLocalHost = true;
	m_maxConnections = 1;
	m_currrentConnections = 0;
	m_password = "";
	m_key_RSA1024XTEA = NULL;
	m_requireLogin = true;
	m_requireEncryption = false;
}

AdminProtocolConfig::~AdminProtocolConfig()
{
	delete m_key_RSA1024XTEA;
}

bool AdminProtocolConfig::loadXMLConfig(const std::string& directory)
{
	std::string filename = directory + "admin.xml";

	xmlDocPtr doc = xmlParseFile(filename.c_str());
	if(!doc){
		return false;
	}

	xmlNodePtr root, p, q;
	root = xmlDocGetRootElement(doc);

	if(!xmlStrEqual(root->name,(const xmlChar*)"otadmin")){
		xmlFreeDoc(doc);
		return false;
	}

	int enabled;
	if(readXMLInteger(root, "enabled", enabled)){
		if(enabled){
			m_enabled = true;
		}
		else{
			m_enabled = false;
		}
	}

	int value;
	p = root->children;
	while(p){
		if(xmlStrEqual(p->name, (const xmlChar*)"security")){
			if(readXMLInteger(p, "onlylocalhost", value)){
				if(value){
					m_onlyLocalHost = true;
				}
				else{
					m_onlyLocalHost = false;
				}
			}
			if(readXMLInteger(p, "maxconnections", value) && value > 0){
				m_maxConnections = value;
			}
			if(readXMLInteger(p, "loginrequired", value)){
				if(value){
					m_requireLogin = true;
				}
				else{
					m_requireLogin = false;
				}
			}
			std::string password;
			if(readXMLString(p, "loginpassword", password)){
				m_password = password;
			}
			else{
				if(m_requireLogin){
					std::cout << "Security warning: require login but use default password." << std::endl;
				}
			}
		}
		else if(xmlStrEqual(p->name, (const xmlChar*)"encryption")){
			if(readXMLInteger(p, "required", value)){
				if(value){
					m_requireEncryption = true;
				}
				else{
					m_requireEncryption = false;
				}
			}
			q = p->children;
			while(q){
				if(xmlStrEqual(q->name, (const xmlChar*)"key")){
					std::string str;
					if(readXMLString(q, "type", str)){
						if(asLowerCaseString(str) == "rsa1024xtea"){
							if(readXMLString(q, "file", str)){
								m_key_RSA1024XTEA = new RSA();
								if(!m_key_RSA1024XTEA->setKey(directory + str)){
									delete m_key_RSA1024XTEA;
									m_key_RSA1024XTEA = NULL;
									std::cout << "Can not load key from " << directory << str << std::endl;
								}
							}
							else{
								std::cout << "Missing file for RSA1024XTEA key." << std::endl;
							}
						}
						else{
							std::cout << str << " is not a valid key type." << std::endl;
						}
					}
				}
				q = q->next;
			}
		}
		p = p->next;
	}
	xmlFreeDoc(doc);

	return true;
}

bool AdminProtocolConfig::isEnabled()
{
	return m_enabled;
}

bool AdminProtocolConfig::onlyLocalHost()
{
	return m_onlyLocalHost;
}

bool AdminProtocolConfig::addConnection()
{
	if(m_currrentConnections >= m_maxConnections){
		return false;
	}
	else{
		m_currrentConnections++;
		return true;
	}
}

void AdminProtocolConfig::removeConnection()
{
	if(m_currrentConnections > 0){
		m_currrentConnections--;
	}
}

bool AdminProtocolConfig::passwordMatch(std::string& password)
{
	//prevent empty password login
	if(m_password == ""){
		return false;
	}
	if(password == m_password){
		return true;
	}
	else{
		return false;
	}
}

bool AdminProtocolConfig::allowIP(uint32_t ip)
{
	if(m_onlyLocalHost){
		if(ip == 0x0100007F){ //127.0.0.1
			return true;
		}
		else{
			addLogLine(NULL, LOGTYPE_WARNING, 1, std::string("forbidden connection try from ") + convertIPToString(ip));
			return false;
		}
	}
	else{
		if(!g_bans.isIpDisabled(ip)){
			return true;
		}
		else{
			return false;
		}
	}
}

bool AdminProtocolConfig::requireLogin()
{
	return m_requireLogin;
}

bool AdminProtocolConfig::requireEncryption()
{
	return m_requireEncryption;
}

uint16_t AdminProtocolConfig::getProtocolPolicy()
{
	uint16_t policy = 0;
	if(requireLogin()){
		policy = policy | REQUIRE_LOGIN;
	}
	if(requireEncryption()){
		policy = policy | REQUIRE_ENCRYPTION;
	}
	return policy;
}

uint32_t AdminProtocolConfig::getProtocolOptions()
{
	uint32_t ret = 0;
	if(requireEncryption()){
		if(m_key_RSA1024XTEA){
			ret = ret | ENCRYPTION_RSA1024XTEA;
		}
	}
	return ret;
}

RSA* AdminProtocolConfig::getRSAKey(uint8_t type)
{
	switch(type){
	case ENCRYPTION_RSA1024XTEA:
		return m_key_RSA1024XTEA;
		break;
	default:
		return NULL;
	}
}

/////////////////////////////////////////////

static void addLogLine(ProtocolAdmin* conn, eLogType type, int level, std::string message)
{
	std::string logMsg;
	if(conn){
		uint32_t ip = conn->getIP();
		logMsg = convertIPToString(ip) +  "[" + logMsg + "] - ";
	}
	logMsg = logMsg + message;
	LOG_MESSAGE("OTADMIN", type, level, logMsg);
}

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


#include "ban.h"
#include "ioplayer.h"
#include "luascript.h"
#include "tools.h"

#include <sstream>

#ifdef __USE_MYSQL__
#include "database.h"
#endif

extern LuaScript g_config;

IOBan* IOBan::_instance = NULL;

bool Ban::isIpBanished(SOCKET s)
{
	sockaddr_in sain;
	socklen_t salen = sizeof(sockaddr_in);
	
	if(getpeername(s, (sockaddr*)&sain, &salen) == 0){
		unsigned long clientip = *(unsigned long*)&sain.sin_addr;

		for(IpBanList::iterator it = ipBanList.begin(); it !=  ipBanList.end(); ++it){
			if((it->ip & it->mask) == (clientip & it->mask)){
				uint32_t currentTime = std::time(NULL);
				if(it->time == 0 || currentTime < it->time){
					return true;
				}
			}
		}
	}
	return false;
}

bool Ban::isPlayerBanished(const std::string& name)
{
	unsigned long playerId;
	std::string playerName = name;
	if(!IOPlayer::instance()->getGuidByName(playerId, playerName)){
		return false;
	}
	for(PlayerBanList::iterator it = playerBanList.begin(); it !=  playerBanList.end(); ++it){
   		if(it->id  == playerId){
			uint32_t currentTime = std::time(NULL);
			if(it->time == 0 || currentTime < it->time){
				return true;
			}
		}
	}
	return false;
}

bool Ban::isAccountBanished(const unsigned long account)
{
	uint32_t currentTime = std::time(NULL);
	for(AccountBanList::iterator it = accountBanList.begin(); it !=  accountBanList.end(); ++it){
   		if(it->id  == account){
			uint32_t currentTime = std::time(NULL);
			if(it->time == 0 || currentTime < it->time){
				return true;
			}
		}
	}
	return false;
}

void Ban::addIpBan(unsigned long ip, unsigned long mask, unsigned long time)
{
	for(IpBanList::iterator it = ipBanList.begin(); it !=  ipBanList.end(); ++it){
		if(it->ip == ip && it->mask == mask){
			it->time = time;
			return;
		}
	}
	IpBanStruct ipBanStruct(ip, mask, time);
	ipBanList.push_back(ipBanStruct);
}

void Ban::addPlayerBan(unsigned long playerId, unsigned long time)
{
	for(PlayerBanList::iterator it = playerBanList.begin(); it !=  playerBanList.end(); ++it){
		if(it->id == playerId){
			it->time = time;
			return;
		}
	}
	PlayerBanStruct playerBanStruct(playerId, time);
	playerBanList.push_back(playerBanStruct);
}

void Ban::addAccountBan(unsigned long account, unsigned long time)
{
	for(AccountBanList::iterator it = accountBanList.begin(); it !=  accountBanList.end(); ++it){
		if(it->id == account){
			it->time = time;
			return;
		}
	}
	AccountBanStruct accountBanStruct(account, time);
	accountBanList.push_back(accountBanStruct);
}

bool Ban::removeIpBan(unsigned long n)
{
	for(IpBanList::iterator it = ipBanList.begin(); it !=  ipBanList.end(); ++it){
		--n;
		if(n == 0){
			ipBanList.erase(it);
			return true;
		}
	}
	return false;
}

bool Ban::removePlayerBan(unsigned long n)
{
	for(PlayerBanList::iterator it = playerBanList.begin(); it !=  playerBanList.end(); ++it){
		--n;
		if(n == 0){
			playerBanList.erase(it);
			return true;
		}
	}
	return false;
}

bool Ban::removeAccountBan(unsigned long n)
{
	for(AccountBanList::iterator it = accountBanList.begin(); it !=  accountBanList.end(); ++it){
		--n;
		if(n == 0){
			accountBanList.erase(it);
			return true;
		}
	}
	return false;
}

const IpBanList& Ban::getIpBans()
{
	return ipBanList;
}

const PlayerBanList& Ban::getPlayerBans()
{
	return playerBanList;
}

const AccountBanList& Ban::getAccountBans()
{
	return accountBanList;
}
	

bool Ban::loadBans(const std::string& identifier)
{
	return IOBan::getInstance()->loadBans(identifier, *this);
}
bool Ban::saveBans(const std::string& identifier)
{
	return IOBan::getInstance()->saveBans(identifier, *this);
}

IOBan* IOBan::getInstance()
{
	if(!_instance){
		#ifdef __USE_MYSQL__
		_instance = new IOBanSQL();
		#else
		_instance = new IOBanXML();
		#endif
	}
	return _instance;
}

#ifdef __USE_MYSQL__

IOBanSQL::IOBanSQL()
{
	m_host = g_config.getGlobalString("sql_host");
	m_user = g_config.getGlobalString("sql_user");
	m_pass = g_config.getGlobalString("sql_pass");
	m_db   = g_config.getGlobalString("sql_db");
}


bool IOBanSQL::loadBans(const std::string& identifier, Ban& banclass)
{
	Database db;
	if(!db.connect(m_db.c_str(), m_host.c_str(), m_user.c_str(), m_pass.c_str())){
		return false;
	}
	
	DBQuery query;
	DBResult result;
	query << "SELECT * FROM bans";
	if(!db.storeQuery(query, result))
		return true;
	
	uint32_t currentTime = std::time(NULL);
	for(int i=0; i < result.getNumRows(); ++i){
		int banType = result.getDataInt("type", i);		
		int time = result.getDataInt("time", i);
		if(time > currentTime){
			switch(banType){
				case BAN_IPADDRESS:
				{
					int ip = result.getDataInt("ip", i);
					int mask = result.getDataInt("mask", i);
					banclass.addIpBan(ip, mask, time);
					break;
				}
				
				case BAN_PLAYER:
				{
					int player = result.getDataInt("player", i);
					banclass.addPlayerBan(player, time);
					break;
				}
				
				case BAN_ACCOUNT:
				{
					int account = result.getDataInt("account", i);
					banclass.addAccountBan(account, time);
					break;
				}
			}
		}
	}
	
	return true;
}

bool IOBanSQL::saveBans(const std::string& identifier, const Ban& banclass)
{
	Database db;
	if(!db.connect(m_db.c_str(), m_host.c_str(), m_user.c_str(), m_pass.c_str())){
		return false;
	}
	
	DBQuery query;

	query << "BEGIN;";
	if(!db.executeQuery(query))
		return false;

	query << "DELETE FROM bans;";
	if(!db.executeQuery(query))
		return false;
	
	uint32_t currentTime = std::time(NULL);
	//save ip bans
	bool executeQuery = false;
	query.reset();
	query << "INSERT INTO `bans` (`type` , `ip` , `mask`, `time`) VALUES ";
	for(IpBanList::const_iterator it = banclass.ipBanList.begin(); it !=  banclass.ipBanList.end(); ++it){
		if(it->time > currentTime){
			executeQuery = true;
			query << query.getSeparator() << "(1," << it->ip << "," << it->mask << 
				"," << it->time << ")";
		}
	}

	if(executeQuery){
		if(!db.executeQuery(query))
			return false;
	}

	//save player bans
	executeQuery = false;
	query.reset();
	query << "INSERT INTO `bans` (`type` , `player` , `time`) VALUES ";
	for(PlayerBanList::const_iterator it = banclass.playerBanList.begin(); it !=  banclass.playerBanList.end(); ++it){
		if(it->time > currentTime){
			executeQuery = true;
			query << query.getSeparator() << "(2," << it->id << "," << it->time << ")";
		}
	}

	if(executeQuery){
		if(!db.executeQuery(query))
			return false;
	}

	//save account bans
	executeQuery = false;
	query.reset();
	query << "INSERT INTO `bans` (`type` , `account` , `time`) VALUES ";
	for(AccountBanList::const_iterator it = banclass.accountBanList.begin(); it != banclass.accountBanList.end(); ++it){
		if(it->time > currentTime){
			executeQuery = true;
			query << query.getSeparator() << "(3," << it->id << "," << it->time << ")";
		}
	}

	if(executeQuery){
		if(!db.executeQuery(query))
			return false;
	}
	
	query.reset();
	query << "COMMIT;";	
	if(!db.executeQuery(query))
		return false;
	
	return true;
}

#else

IOBanXML::IOBanXML()
{
	//
}

bool IOBanXML::loadBans(const std::string& identifier, Ban& banclass)
{
	xmlDocPtr doc = xmlParseFile(identifier.c_str());
	if(doc){
		xmlNodePtr root;
		root = xmlDocGetRootElement(doc);
		
		if(xmlStrcmp(root->name,(const xmlChar*)"bans") != 0){
			xmlFreeDoc(doc);
			return false;
		}

		xmlNodePtr banNode = root->children;
		while(banNode){
			if(xmlStrcmp(banNode->name,(const xmlChar*)"ban") == 0){

				int banType;
				if(readXMLInteger(banNode, "type", banType) && banType >= BAN_IPADDRESS && banType <= BAN_ACCOUNT){
					int time = 0;
					readXMLInteger(banNode, "time", time);

					switch(banType){
						case BAN_IPADDRESS:
						{
							int ip = 0;
							int mask = 0;

							if(readXMLInteger(banNode, "ip", ip)){

								readXMLInteger(banNode, "mask", ip);
								banclass.addIpBan(ip, mask, time);
							}

							break;
						}

						case BAN_PLAYER:
						{
							int playerguid = 0;
							if(readXMLInteger(banNode, "player", playerguid)){
								banclass.addPlayerBan(playerguid, time);
							}

							break;
						}
						
						case BAN_ACCOUNT:
						{
							int account = 0;
							if(readXMLInteger(banNode, "account", account)){
								banclass.addAccountBan(account, time);
							}

							break;
						}
					}
				}
				else{
					std::cout << "Warning: [IOBanXML::loadBans] could not load ban" << std::endl;
				}
			}

			banNode = banNode->next;
		}
	}

	return true;
}

bool IOBanXML::saveBans(const std::string& identifier, const Ban& banclass)
{
	xmlDocPtr doc = xmlNewDoc((xmlChar*) "1.0");
  xmlNodePtr nodeBans = xmlNewNode(NULL, (xmlChar*) "bans");
  xmlDocSetRootElement(doc, nodeBans);

	uint32_t currentTime = std::time(NULL);

	//save ip bans
	for(IpBanList::const_iterator it = banclass.ipBanList.begin(); it !=  banclass.ipBanList.end(); ++it){
		if(it->time > currentTime){
			
			xmlNodePtr nodeBan = xmlNewChild(nodeBans, NULL, (xmlChar*) "ban", NULL);

			std::stringstream ss;
			ss.str("");
			ss << (int) BAN_IPADDRESS;
			xmlNewProp(nodeBan, (xmlChar*) "type", (xmlChar*) ss.str().c_str());

			ss.str("");
			ss << (int) it->ip;
			xmlNewProp(nodeBan, (xmlChar*) "ip", (xmlChar*) ss.str().c_str());

			ss.str("");
			ss << (int) it->mask;
			xmlNewProp(nodeBan, (xmlChar*) "mask", (xmlChar*) ss.str().c_str());

			ss.str("");
			ss << (int) it->time;
			xmlNewProp(nodeBan, (xmlChar*) "time", (xmlChar*) ss.str().c_str());
		}
	}

	//save player bans
	for(PlayerBanList::const_iterator it = banclass.playerBanList.begin(); it !=  banclass.playerBanList.end(); ++it){
		if(it->time > currentTime){

			xmlNodePtr nodeBan = xmlNewChild(nodeBans, NULL, (xmlChar*) "ban", NULL);

			std::stringstream ss;
			ss.str("");
			ss << (int) BAN_PLAYER;
			xmlNewProp(nodeBan, (xmlChar*) "type", (xmlChar*) ss.str().c_str());

			ss.str("");
			ss << (int) it->id;
			xmlNewProp(nodeBan, (xmlChar*) "player", (xmlChar*) ss.str().c_str());

			ss.str("");
			ss << (int) it->time;
			xmlNewProp(nodeBan, (xmlChar*) "time", (xmlChar*) ss.str().c_str());
		}
	}

	for(AccountBanList::const_iterator it = banclass.accountBanList.begin(); it != banclass.accountBanList.end(); ++it){
		if(it->time > currentTime){
			xmlNodePtr nodeBan = xmlNewChild(nodeBans, NULL, (xmlChar*) "ban", NULL);

			std::stringstream ss;
			ss.str("");
			ss << (int) BAN_ACCOUNT;
			xmlNewProp(nodeBan, (xmlChar*) "type", (xmlChar*) ss.str().c_str());

			ss.str("");
			ss << (int) it->id;
			xmlNewProp(nodeBan, (xmlChar*) "account", (xmlChar*) ss.str().c_str());

			ss.str("");
			ss << (int) it->time;
			xmlNewProp(nodeBan, (xmlChar*) "time", (xmlChar*) ss.str().c_str());
		}
	}

	xmlSaveFormatFileEnc(identifier.c_str(), doc, "UTF-8", 1);
	xmlFreeDoc(doc);
	
	return true;
}

#endif

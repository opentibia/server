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

#ifndef __OTSERV_BAN_H__
#define __OTSERV_BAN_H__

#include "otsystem.h"
#include <list>

struct IpBanStruct{
	uint32_t ip;
	uint32_t mask;
	uint32_t time;
	IpBanStruct(uint32_t _ip, uint32_t _mask, uint32_t _time){
		ip = _ip;
		mask = _mask;
		time = _time;
	}
};

struct LoginBlock{
	uint32_t lastLoginTime;
	uint32_t numberOfLogins;
};

struct ConnectBlock{
	uint64_t lastConnection;
};

struct idBan{
	uint32_t id;
	uint32_t time;
	idBan(uint32_t _id, uint32_t _time){
		id = _id;
		time = _time;
	}
};
typedef idBan PlayerBanStruct;
typedef idBan AccountBanStruct;

typedef std::list< IpBanStruct > IpBanList;
typedef std::list< PlayerBanStruct > PlayerBanList;
typedef std::list< AccountBanStruct > AccountBanList;
typedef std::map<uint32_t, LoginBlock > IpLoginMap;
typedef std::map<uint32_t, ConnectBlock > IpConnectMap;

enum BanType_t{
  BAN_IPADDRESS = 1,
  BAN_PLAYER = 2,
  BAN_ACCOUNT = 3
};

class Ban{
public:
	Ban();
	~Ban(){};
	void init();

	bool isIpBanished(SOCKET s);
	bool isPlayerBanished(const std::string& name);
	bool isAccountBanished(uint32_t account);
	bool isIpDisabled(SOCKET s);
	bool acceptConnection(SOCKET s);

	void addIpBan(uint32_t ip, uint32_t mask, uint32_t time);
	void addPlayerBan(uint32_t playerId, uint32_t time);
	void addAccountBan(uint32_t account, uint32_t time);
	void addConnectionAttempt(SOCKET s);
	void addLoginAttempt(SOCKET s, bool isSuccess);

	bool removeIpBan(uint32_t n);
	bool removePlayerBan(uint32_t n);
	bool removeAccountBan(uint32_t n);

	bool loadBans(const std::string& identifier);
	bool saveBans(const std::string& identifier);

	const IpBanList& getIpBans();
	const PlayerBanList& getPlayerBans();
	const AccountBanList& getAccountBans();

protected:

	IpBanList ipBanList;
	PlayerBanList playerBanList;
	AccountBanList accountBanList;
	IpLoginMap ipLoginMap;
	IpConnectMap ipConnectMap;

	uint32_t loginTimeout;
	uint32_t maxLoginTries;
	uint32_t retryTimeout;

	OTSYS_THREAD_LOCKVAR banLock;

	friend class IOBanSQL;
	friend class IOBanXML;
};

class IOBan{
public:
	static IOBan* getInstance();

	virtual bool loadBans(const std::string& identifier, Ban& banclass) = 0;
	virtual bool saveBans(const std::string& identifier, const Ban& banclass) = 0;

protected:
	IOBan(){};
	virtual ~IOBan(){};
	static IOBan* _instance;
};

#if defined USE_SQL_ENGINE
class IOBanSQL : public IOBan{
public:
	IOBanSQL();
	virtual ~IOBanSQL(){};

	virtual bool loadBans(const std::string& identifier,Ban& banclass);
	virtual bool saveBans(const std::string& identifier, const Ban& banclass);

protected:
	std::string m_host;
	std::string m_user;
	std::string m_pass;
	std::string m_db;
};

#else

class IOBanXML : public IOBan {
public:
	IOBanXML();
	virtual ~IOBanXML(){};

	virtual bool loadBans(const std::string& identifier,Ban& banclass);
	virtual bool saveBans(const std::string& identifier, const Ban& banclass);
};
#endif

#endif

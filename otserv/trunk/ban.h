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
	unsigned long ip;
	unsigned long mask;
	unsigned long time;
	IpBanStruct(unsigned long _ip, unsigned long _mask, unsigned long _time){
		ip = _ip;
		mask = _mask;
		time = _time;
	}
};

struct LoginConnectionStruct{
	unsigned long lastLoginTime;
	unsigned long numberOfLogins;
};

struct idBan{
	unsigned long id;
	unsigned long time;
	idBan(unsigned long _id, unsigned long _time){
		id = _id;
		time = _time;
	}
};
typedef idBan PlayerBanStruct;
typedef idBan AccountBanStruct;

typedef std::list< IpBanStruct > IpBanList;
typedef std::list< PlayerBanStruct > PlayerBanList;
typedef std::list< AccountBanStruct > AccountBanList;
typedef std::map<unsigned long, LoginConnectionStruct > IpLoginMap;

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
	bool isAccountBanished(const unsigned long account);
	bool isIpDisabled(SOCKET s);

	void addIpBan(unsigned long ip, unsigned long mask, unsigned long time);
	void addPlayerBan(unsigned long playerId, unsigned long time);
	void addAccountBan(unsigned long account, unsigned long time);
	void addConnectionAttempt(SOCKET s, bool isSuccess);

	bool removeIpBan(unsigned long n);
	bool removePlayerBan(unsigned long n);
	bool removeAccountBan(unsigned long n);

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

	unsigned long loginTimeout;
	unsigned long maxLoginTries;
	unsigned long retryTimeout;

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

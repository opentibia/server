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

#include <boost/thread.hpp>
#include <list>

struct LoginBlock{
	uint32_t lastLoginTime;
	uint32_t numberOfLogins;
};

struct ConnectBlock{
	uint64_t startTime;
	uint64_t blockTime;
	uint32_t count;
};

typedef std::map<uint32_t, LoginBlock > IpLoginMap;
typedef std::map<uint32_t, ConnectBlock > IpConnectMap;

enum BanType_t{
  BAN_IPADDRESS = 1,
  BAN_PLAYER = 2,
  BAN_ACCOUNT = 3
};

struct Ban {
	BanType_t type;
	uint32_t id;
	uint32_t added;
	uint32_t expires;
	uint32_t adminid;
	uint32_t reason;
	std::string comment;
	std::string value;
	std::string param;
};

class BanManager {
public:
	BanManager();
	~BanManager();

	void loadSettings();
	bool clearTemporaryBans();

	bool isIpBanished(uint32_t clientip, uint32_t mask = 0xFFFFFFFF) const;
	bool isPlayerBanished(const std::string& name) const;
	bool isPlayerBanished(uint32_t guid) const;
	bool isAccountBanished(uint32_t accountId) const;
	bool isAccountDeleted(const std::string& account) const;
	bool isIpDisabled(uint32_t clientip) const;

	bool acceptConnection(uint32_t clientip);

	void addIpBan(uint32_t ip, uint32_t mask, uint32_t time, uint32_t adminid, std::string comment);
	void addPlayerBan(uint32_t playerId, uint32_t time, uint32_t adminid, std::string comment);
	void addPlayerBan(std::string, uint32_t time, uint32_t adminid, std::string comment);
	void addAccountBan(uint32_t account, uint32_t time, uint32_t adminid, std::string comment);
	void addConnectionAttempt(uint32_t clientup);
	void addLoginAttempt(uint32_t clientip, bool isSuccess);

	bool removeIpBans(uint32_t ip, uint32_t mask = 0xFFFFFFFF);
	bool removePlayerBans(uint32_t guid);
	bool removePlayerBans(std::string& name);
	bool removeAccountBans(uint32_t accno);
	//bool removeAccountBans(const std::string& name);

	std::vector<Ban> getBans(BanType_t type) const;
protected:
	IpLoginMap ipLoginMap;
	IpConnectMap ipConnectMap;

	uint32_t loginTimeout;
	uint32_t maxLoginTries;
	uint32_t retryTimeout;

	mutable boost::recursive_mutex banLock;

	friend class IOBan;
};

#endif

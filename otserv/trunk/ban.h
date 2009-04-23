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
#include "enums.h"

#include <boost/thread.hpp>
#include <list>

enum BanType_t {
	BAN_IPADDRESS = 1,
	BAN_PLAYER = 2,
	BAN_ACCOUNT = 3,
	BAN_NOTATION = 4
};

struct Ban {
	BanType_t type;
	uint32_t id;
	uint32_t added;
	uint32_t expires;
	uint32_t adminId;
	uint32_t reason;
	violationAction_t action;
	std::string comment;
	std::string statement;
	std::string value;
	std::string param;
};

struct LoginBlock {
	time_t lastLoginTime;
	uint32_t numberOfLogins;
};

struct ConnectBlock {
	uint64_t startTime;
	uint64_t blockTime;
	uint32_t count;
};

typedef std::map<uint32_t, LoginBlock > IpLoginMap;
typedef std::map<uint32_t, ConnectBlock > IpConnectMap;

class BanManager {
public:
	BanManager();
	~BanManager() {}

	bool clearTemporaryBans() const;
	bool acceptConnection(uint32_t clientip);

	bool isIpDisabled(uint32_t clientip);
	bool isIpBanished(uint32_t clientip, uint32_t mask = 0xFFFFFFFF) const;
	bool isPlayerBanished(uint32_t guid) const;
	bool isPlayerBanished(const std::string& name) const;
	bool isAccountBanished(uint32_t accountId) const;

	void addLoginAttempt(uint32_t clientip, bool isSuccess);
	bool addIpBan(uint32_t ip, uint32_t mask, int32_t time, uint32_t adminid, std::string comment) const;
	bool addPlayerBan(uint32_t playerId, int32_t time, uint32_t adminid, std::string comment,
		std::string statement, uint32_t reason, violationAction_t action) const;
	bool addPlayerBan(const std::string& name, int32_t time, uint32_t adminid, std::string comment,
		std::string statement, uint32_t reason, violationAction_t action) const;
	bool addAccountBan(uint32_t account, int32_t time, uint32_t adminid, std::string comment,
		std::string statement, uint32_t reason, violationAction_t action) const;
	bool addAccountNotation(uint32_t account, uint32_t adminid, std::string comment,
		std::string statement, uint32_t reason, violationAction_t action) const;

	bool removeIpBans(uint32_t ip, uint32_t mask = 0xFFFFFFFF) const;
	bool removePlayerBans(uint32_t guid) const;
	bool removePlayerBans(const std::string& name) const;
	bool removeAccountBans(uint32_t accno) const;
	bool removeNotations(uint32_t accno) const;

	uint32_t getNotationsCount(uint32_t account);
	std::vector<Ban> getBans(BanType_t type);
protected:
	mutable boost::recursive_mutex banLock;

	IpLoginMap ipLoginMap;
	IpConnectMap ipConnectMap;

	uint32_t loginTimeout, maxLoginTries, retryTimeout;
};

#endif

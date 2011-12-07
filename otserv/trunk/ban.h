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

#include "definitions.h"
#include "enums.h"
#include <boost/thread.hpp>
#include <list>
#include <vector>

enum BanType_t {
	BAN_IPADDRESS = 1,
	BAN_PLAYER = 2,
	BAN_ACCOUNT = 3,
	BAN_NOTATION = 4,
	BAN_STATEMENT = 5,
	BAN_NAMELOCK = 6
};

struct Ban {
	BanType_t type;
	uint32_t id;
	uint32_t added;
	int32_t expires;
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

	bool clearTemporaryBans() const;
	bool acceptConnection(const uint32_t& clientip);

	bool isIpDisabled(const uint32_t& clientip);
	bool isIpBanished(const uint32_t& clientip, const uint32_t& mask = 0xFFFFFFFF) const;
	bool isPlayerBanished(const uint32_t& guid) const;
	bool isPlayerBanished(const std::string& name) const;
	bool isAccountBanished(const uint32_t& accountId) const;

	void addLoginAttempt(const uint32_t& clientip, bool isSuccess);
	bool addIpBan(const uint32_t& ip, const uint32_t& mask, const int32_t& time,
		const uint32_t& adminid, const std::string& comment) const;
	bool addPlayerBan(const uint32_t& playerId, const int32_t& time, const uint32_t& adminid,
		const std::string& comment,	const std::string& statement,
		const uint32_t& reason, const violationAction_t& action) const;
	bool addPlayerBan(const std::string& name, const int32_t& time, const uint32_t& adminid,
		const std::string& comment, const std::string& statement,
		const uint32_t& reason, const violationAction_t& action) const;
	bool addPlayerStatement(const uint32_t& playerId, const uint32_t& adminid, const std::string& comment,
		const std::string& statement, const uint32_t& reason, const violationAction_t& action) const;
	bool addPlayerNameReport(const uint32_t& playerId, const uint32_t& adminid, const std::string& comment,
		const std::string& statement, const uint32_t& reason, const violationAction_t& action) const;
	bool addAccountBan(const uint32_t& account, const int32_t& time, const uint32_t& adminid,
		const std::string& comment,	const std::string& statement,
		const uint32_t& reason, const violationAction_t& action) const;
	bool addAccountNotation(const uint32_t& account, const uint32_t& adminid, const std::string& comment,
		const std::string& statement, const uint32_t& reason, const violationAction_t& action) const;

	bool removeIpBans(const uint32_t& ip, const uint32_t& mask = 0xFFFFFFFF) const;
	bool removePlayerBans(const uint32_t& guid) const;
	bool removePlayerBans(const std::string& name) const;
	bool removeAccountBans(const uint32_t& accno) const;
	bool removeNotations(const uint32_t& accno) const;

	uint32_t getNotationsCount(const uint32_t& account);
	std::vector<Ban> getBans(const BanType_t& type);
protected:
	mutable boost::recursive_mutex banLock;

	IpLoginMap ipLoginMap;
	IpConnectMap ipConnectMap;
};

#endif

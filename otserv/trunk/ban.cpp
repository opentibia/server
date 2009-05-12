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

#include "ban.h"
#include "tools.h"

#include "configmanager.h"
#include "ioplayer.h"
#include "database.h"

extern ConfigManager g_config;

bool BanManager::clearTemporaryBans() const
{
	Database* db = Database::instance();
	DBQuery query;
	return db->executeQuery("UPDATE `bans` SET `active` = 0 WHERE `expires` = 0");
}

bool BanManager::acceptConnection(uint32_t clientip)
{
	if(clientip == 0) return false;
	banLock.lock();

	uint64_t currentTime = OTSYS_TIME();
	IpConnectMap::iterator it = ipConnectMap.find(clientip);
	if(it == ipConnectMap.end())
	{
		ConnectBlock cb;
		cb.startTime = currentTime;
		cb.blockTime = 0;
		cb.count = 1;

		ipConnectMap[clientip] = cb;
		banLock.unlock();
		return true;
	}

	it->second.count++;
	if(it->second.blockTime > currentTime)
	{
		banLock.unlock();
		return false;
	}

	if(currentTime - it->second.startTime > 1000)
	{
		uint32_t connectionPerSec = it->second.count;
		it->second.startTime = currentTime;
		it->second.count = 0;
		it->second.blockTime = 0;

		if(connectionPerSec > 10)
		{
			it->second.blockTime = currentTime + 10000;
			banLock.unlock();
			return false;
		}
	}

	banLock.unlock();
	return true;
}

bool BanManager::isIpDisabled(uint32_t clientip)
{
	if(g_config.getNumber(ConfigManager::LOGIN_TRIES) == 0 || clientip == 0) return false;
	banLock.lock();

	time_t currentTime = std::time(NULL);
	IpLoginMap::iterator it = ipLoginMap.find(clientip);
	if(it != ipLoginMap.end())
	{
		uint32_t loginTimeout = (uint32_t)g_config.getNumber(ConfigManager::LOGIN_TIMEOUT) / 1000;
		if( (it->second.numberOfLogins >= g_config.getNumber(ConfigManager::LOGIN_TRIES)) &&
			(currentTime < it->second.lastLoginTime + loginTimeout) )
		{
			banLock.unlock();
			return true;
		}
	}

	banLock.unlock();
	return false;
}

bool BanManager::isIpBanished(uint32_t clientip, uint32_t mask /*= 0xFFFFFFFF*/) const
{
	if(clientip == 0) return false;
	Database* db = Database::instance();

	DBQuery query;
	query <<
		"SELECT "
			"COUNT(*) AS `count` "
		"FROM "
			"`bans` "
		"WHERE "
			"`type` = " << BAN_IPADDRESS << " AND "
			"((" << clientip << " & " << mask << " & `param`) = (`value` & `param` & " << mask << ")) AND "
			"`active` = 1 AND "
			"(`expires` >= " << std::time(NULL) << " OR `expires` <= 0)";

	DBResult* result;
	if(!(result = db->storeQuery(query.str())))
		return false;

	int t = result->getDataInt("count");
	db->freeResult(result);
	return t > 0;
}

bool BanManager::isPlayerBanished(uint32_t playerId) const
{
	Database* db = Database::instance();

	DBQuery query;
	query <<
		"SELECT "
			"COUNT(*) AS `count` "
		"FROM "
			"`bans` "
		"WHERE "
			"`type` = " << BAN_PLAYER << " AND "
			"`value` = " << playerId << " AND "
			"`active` = 1 AND "
			"(`expires` >= " << std::time(NULL) << " OR `expires` <= 0)";

	DBResult* result;
	if(!(result = db->storeQuery(query.str())))
		return false;

	int t = result->getDataInt("count");
	db->freeResult(result);
	return t > 0;
}

bool BanManager::isPlayerBanished(const std::string& name) const
{
	uint32_t playerId;
	std::string n = name;
	return IOPlayer::instance()->getGuidByName(playerId, n)
		&& isPlayerBanished(playerId);
}

bool BanManager::isAccountBanished(uint32_t accountId) const
{
	Database* db = Database::instance();

	DBQuery query;
	query <<
		"SELECT "
			"COUNT(*) AS `count` "
		"FROM "
			"`bans` "
		"WHERE "
			"`type` = " << BAN_ACCOUNT << " AND "
			"`value` = " << accountId << " AND "
			"`active` = 1 AND "
			"(`expires` >= " << std::time(NULL) << " OR `expires` <= 0)";

	DBResult* result;
	if(!(result = db->storeQuery(query.str())))
		return false;

	int t = result->getDataInt("count");
	db->freeResult(result);
	return t > 0;
}

void BanManager::addLoginAttempt(uint32_t clientip, bool isSuccess)
{
	if(clientip == 0) return;
	banLock.lock();

	time_t currentTime = std::time(NULL);
	IpLoginMap::iterator it = ipLoginMap.find(clientip);
	if(it == ipLoginMap.end())
	{
		LoginBlock lb;
		lb.lastLoginTime = 0;
		lb.numberOfLogins = 0;

		ipLoginMap[clientip] = lb;
		it = ipLoginMap.find(clientip);
	}

	if(it->second.numberOfLogins >= g_config.getNumber(ConfigManager::LOGIN_TRIES))
		it->second.numberOfLogins = 0;

	uint32_t retryTimeout = (uint32_t)g_config.getNumber(ConfigManager::RETRY_TIMEOUT) / 1000;
	if(!isSuccess || (currentTime < it->second.lastLoginTime + retryTimeout))
		++it->second.numberOfLogins;
	else
		it->second.numberOfLogins = 0;

	it->second.lastLoginTime = currentTime;
	banLock.unlock();
}

bool BanManager::addIpBan(uint32_t ip, uint32_t mask, int32_t time,
	uint32_t adminid, std::string comment) const
{
	if(ip == 0 || mask == 0) return false;
	Database* db = Database::instance();

	DBInsert stmt(db);
	stmt.setQuery("INSERT INTO `bans` (`type`, `value`, `param`, `expires`, `added`, `admin_id`, `comment`) VALUES ");

	DBQuery query;
	query << BAN_IPADDRESS << ", " << ip << ", " << mask << ", " << time << ", ";
	query << std::time(NULL) << ", " << adminid << ", " << db->escapeString(comment);


	if(!stmt.addRow(query.str())) return false;
	return stmt.execute();
}

bool BanManager::addPlayerBan(uint32_t playerId, int32_t time, uint32_t adminid,
	std::string comment, std::string statement, uint32_t reason, violationAction_t action) const
{
	if(playerId == 0) return false;
	Database* db = Database::instance();

	DBInsert stmt(db);
	stmt.setQuery("INSERT INTO `bans` (`type`, `value`, `expires`, `added`, `admin_id`, `comment`, `statement`, `reason`, `action`) VALUES ");

	DBQuery query;
	query << BAN_PLAYER << ", " << playerId << ", " << time << ", " << std::time(NULL) << ", " << adminid << ", ";
	query << db->escapeString(comment) << ", " << db->escapeString(statement) << ", " << reason << ", " << action;

	if(!stmt.addRow(query.str())) return false;
	return stmt.execute();
}

bool BanManager::addPlayerBan(const std::string& name, int32_t time, uint32_t adminid,
	std::string comment, std::string statement, uint32_t reason, violationAction_t action) const
{
	uint32_t guid = 0;
	std::string n = name;
	return IOPlayer::instance()->getGuidByName(guid, n) &&
		addPlayerBan(guid, time, adminid, comment, statement, reason, action);
}

bool BanManager::addAccountBan(uint32_t account, int32_t time, uint32_t adminid,
	std::string comment, std::string statement, uint32_t reason, violationAction_t action) const
{
	if(account == 0) return false;
	Database* db = Database::instance();

	DBInsert stmt(db);
	stmt.setQuery("INSERT INTO `bans` (`type`, `value`, `expires`, `added`, `admin_id`, `comment`, `statement`, `reason`, `action`) VALUES ");

	DBQuery query;
	query << BAN_ACCOUNT << ", " << account << ", " << time << ", " << std::time(NULL) << ", " << adminid << ", ";
	query << db->escapeString(comment) << ", " << db->escapeString(statement) << ", " << reason << ", " << action;

	if(!stmt.addRow(query.str())) return false;
	return stmt.execute();
}

bool BanManager::addAccountNotation(uint32_t account, uint32_t adminid, std::string comment,
	std::string statement, uint32_t reason, violationAction_t action) const
{
	if(account == 0) return false;
	Database* db = Database::instance();

	DBInsert stmt(db);
	stmt.setQuery("INSERT INTO `bans` (`type`, `value`, `expires`, `added`, `admin_id`, `comment`, `statement`, `reason`, `action`) VALUES ");

	DBQuery query;
	query << BAN_NOTATION << ", " << account << ", -1, " << std::time(NULL) << ", " << adminid << ", ";
	query << db->escapeString(comment) << ", " << db->escapeString(statement) << ", " << reason << ", " << action;

	if(!stmt.addRow(query.str())) return false;
	return stmt.execute();
}

bool BanManager::removeIpBans(uint32_t ip, uint32_t mask) const
{
	if(!isIpBanished(ip, mask)) return false;
	Database* db = Database::instance();

	DBQuery query;
	query << "UPDATE `bans` SET `active` = 0 WHERE `type` = " << BAN_IPADDRESS << " AND (`value` & `param` & " << mask << ") = (" << ip << " & `param` & " << mask << ")" << " AND `active` = 1";
	return db->executeQuery(query.str());
}

bool BanManager::removePlayerBans(uint32_t guid) const
{
	if(!isPlayerBanished(guid)) return false;
	Database* db = Database::instance();

	DBQuery query;
	query << "UPDATE `bans` SET `active` = 0 WHERE `type` = " << BAN_PLAYER << " AND `value` = " << guid << " AND `active` = 1";
	return db->executeQuery(query.str());
}

bool BanManager::removePlayerBans(const std::string& name) const
{
	uint32_t playerId = 0;
	std::string n = name;
	return IOPlayer::instance()->getGuidByName(playerId, n)
		&& removePlayerBans(playerId);
}

bool BanManager::removeAccountBans(uint32_t accno) const
{
	if(!isAccountBanished(accno)) return false;
	Database* db = Database::instance();

	DBQuery query;
	query << "UPDATE `bans` SET `active` = 0 WHERE `type` = " << BAN_ACCOUNT << " AND `value` = " << accno << " AND `active` = 1";
	return db->executeQuery(query.str());
}

bool BanManager::removeNotations(uint32_t accno) const
{
	Database* db = Database::instance();
	DBQuery query;

	query << "UPDATE `bans` SET `active` = 0 WHERE `type` = " << BAN_NOTATION << " AND `value` = " << accno << " AND `active` = 1";
	return db->executeQuery(query.str());
}

uint32_t BanManager::getNotationsCount(uint32_t account)
{
	Database* db = Database::instance();
	DBResult* result;

	DBQuery query;
	query << "SELECT COUNT(`id`) AS `count` FROM `bans` WHERE `value` = " << account << " AND `type` = " << BAN_NOTATION << " AND `active` = 1";
	if(!(result = db->storeQuery(query.str())))
		return 0;

	const uint32_t count = result->getDataInt("count");
	db->freeResult(result);
	return count;
}

std::vector<Ban> BanManager::getBans(BanType_t type)
{
	assert(type == BAN_IPADDRESS || type == BAN_PLAYER || type == BAN_ACCOUNT || type == BAN_NOTATION);
	Database* db = Database::instance();

	DBQuery query;
	query <<
		"SELECT "
			"`id`, "
			"`value`, "
			"`param`, "
			"`expires`, "
			"`added`, "
			"`admin_id`, "
			"`comment`, "
			"`reason` "
		"FROM "
			"`bans` "
		"WHERE "
			"`type` = " << type << " AND "
			"`active` = 1 AND " <<
			"(`expires` >= " << std::time(NULL) << " OR `expires` = 0)";

	std::vector<Ban> vec;
	DBResult* result;
	if(!(result = db->storeQuery(query.str()))){
		return vec;
	}

	do {
		Ban ban;
		ban.type = type;
		ban.id = result->getDataInt("id");
		ban.value = result->getDataString("value");
		ban.param = result->getDataString("param");
		ban.expires = (uint32_t)result->getDataLong("expires");
		ban.added = (uint32_t)result->getDataLong("id");
		ban.adminId = result->getDataInt("admin_id");
		ban.reason = result->getDataInt("reason");
		ban.action = (violationAction_t)result->getDataInt("action");
		ban.comment = result->getDataString("comment");
		ban.statement = result->getDataString("statement");

		vec.push_back(ban);
	} while(result->next());

	db->freeResult(result);
	return vec;
}

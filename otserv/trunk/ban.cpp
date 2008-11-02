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

#include "definitions.h"

#include "ban.h"
#include "ioplayer.h"
#include "configmanager.h"
#include <sstream>
#include "tools.h"

#include "database.h"

extern ConfigManager g_config;

BanManager::BanManager()
{
}

BanManager::~BanManager()
{
}

void BanManager::loadSettings()
{
	maxLoginTries = (uint32_t)g_config.getNumber(ConfigManager::LOGIN_TRIES);
	retryTimeout = (uint32_t)g_config.getNumber(ConfigManager::RETRY_TIMEOUT) / 1000;
	loginTimeout = (uint32_t)g_config.getNumber(ConfigManager::LOGIN_TIMEOUT) / 1000;
}

bool BanManager::clearTemporaryBans() {
	Database* db = Database::instance();
	DBQuery query;
	query << "UPDATE `bans` SET `active` = 0 WHERE `expires` = 0";
	return db->executeQuery(query.str());
}

bool BanManager::isIpBanished(uint32_t clientip, uint32_t mask /*= 0xFFFFFFFF*/) const
{
	if(clientip != 0){
		Database* db = Database::instance();
		DBQuery query;
		DBResult* result;

		uint32_t currentTime = std::time(NULL);
		query <<
			"SELECT "
				"COUNT(*) AS `count` "
			"FROM "
				"`bans` "
			"WHERE "
				"`type` = " << BAN_IPADDRESS << " AND "
				"((" << clientip << " & " << mask << " & `param`) = (`value` & `param` & " << mask << ")) AND "
				"`active` = 1 AND "
				"(`expires` >= " << currentTime << " OR `expires` = 0)";

		if((result = db->storeQuery(query.str())) != NULL){
			int t = result->getDataInt("count");
			db->freeResult(result);
			return t > 0;
		}
		db->freeResult(result);
	}

	return false;
}

bool BanManager::isIpDisabled(uint32_t clientip) const
{
	if(maxLoginTries == 0)
		return false;

	if(clientip != 0){
		banLock.lock();

		uint32_t currentTime = std::time(NULL);
		IpLoginMap::const_iterator it = ipLoginMap.find(clientip);
		if(it != ipLoginMap.end()){
			if( (it->second.numberOfLogins >= maxLoginTries) &&
				(currentTime < it->second.lastLoginTime + loginTimeout) )
			{
				banLock.unlock();
				return true;
			}
		}

		banLock.unlock();
	}

	return false;
}

bool BanManager::acceptConnection(uint32_t clientip)
{
	if(clientip == 0)
		return false;

	banLock.lock();

	uint64_t currentTime = OTSYS_TIME();

	IpConnectMap::iterator it = ipConnectMap.find(clientip);
	if(it == ipConnectMap.end()){
		ConnectBlock cb;
		cb.lastConnection = currentTime;

		ipConnectMap[clientip] = cb;

		banLock.unlock();
		return true;
	}

	if(currentTime - it->second.lastConnection < 1000){
		banLock.unlock();
		return false;
	}

	it->second.lastConnection = currentTime;

	banLock.unlock();
	return true;
}

void BanManager::addLoginAttempt(uint32_t clientip, bool isSuccess)
{
	if(clientip != 0){
		banLock.lock();

		uint32_t currentTime = std::time(NULL);

		IpLoginMap::iterator it = ipLoginMap.find(clientip);
		if(it == ipLoginMap.end()){
			LoginBlock lb;
			lb.lastLoginTime = 0;
			lb.numberOfLogins = 0;

			ipLoginMap[clientip] = lb;
			it = ipLoginMap.find(clientip);
		}

		if(it->second.numberOfLogins >= maxLoginTries){
			it->second.numberOfLogins = 0;
		}

		if(!isSuccess || (currentTime < it->second.lastLoginTime + retryTimeout)){
			++it->second.numberOfLogins;
		}
		else{
			it->second.numberOfLogins = 0;
		}

		it->second.lastLoginTime = currentTime;

		banLock.unlock();
	}
}

bool BanManager::isPlayerBanished(const std::string& name) const
{
	uint32_t playerId;
	std::string playerName = name;
	if(!IOPlayer::instance()->getGuidByName(playerId, playerName))
		return false;
	return isPlayerBanished(playerId);
}

bool BanManager::isPlayerBanished(uint32_t playerId) const
{
	Database* db = Database::instance();
	DBQuery query;
	DBResult* result;

	uint32_t currentTime = std::time(NULL);
	query <<
		"SELECT "
			"COUNT(*) AS `count` "
		"FROM "
			"`bans` "
		"WHERE "
			"`type` = " << BAN_PLAYER << " AND "
			"`value` = " << playerId << " AND "
			"`active` = 1 AND "
			"(`expires` >= " << currentTime << " OR `expires` = 0)";

	if((result = db->storeQuery(query.str())) != NULL){
		int t = result->getDataInt("count");
		db->freeResult(result);
		return t > 0;
	}
	return false;
}

bool BanManager::isAccountBanished(uint32_t account) const
{
	Database* db = Database::instance();
	DBQuery query;
	DBResult* result;

	uint32_t currentTime = std::time(NULL);
	query <<
		"SELECT "
			"COUNT(*) AS `count` "
		"FROM "
			"`bans` "
		"WHERE "
			"`type` = " << BAN_ACCOUNT << " AND "
			"`value` = " << account << " AND "
			"`active` = 1 AND "
			"(`expires` >= " << currentTime << " OR `expires` = 0)";

	if((result = db->storeQuery(query.str())) != NULL){
		int t = result->getDataInt("count");
		db->freeResult(result);
		return t > 0;
	}

	return false;
}

bool BanManager::isAccountDeleted(uint32_t account) const
{
	Database* db = Database::instance();
	DBQuery query;
	DBResult* result;

	query <<
		"SELECT "
			"`deleted` "
		"FROM "
			"`accounts` "
		"WHERE "
			"`id` = " << account;

	if((result = db->storeQuery(query.str())) != NULL){
		int b = result->getDataInt("deleted");
		db->freeResult(result);
		return b != 0;
	}

	return false;
}

void BanManager::addIpBan(uint32_t ip, uint32_t mask, uint32_t time, uint32_t adminid, std::string comment)
{
	if(ip == 0) return;
	if(mask == 0) return; // this would ban everybody

	Database* db = Database::instance();
	DBInsert stmt(db);
	DBQuery query;

	stmt.setQuery("INSERT INTO `bans` (`type`, `value`, `param`, `expires`, `added`, `admin_id`, `comment`) VALUES ");

	query << BAN_IPADDRESS << ", " << ip << ", " << mask << ", " << time << ", " << std::time(NULL) << ", " << adminid << ", " << db->escapeString(comment);
	if(!stmt.addRow(query.str())){
		return;
	}

	if(!stmt.execute()){
		return;
	}
}

void BanManager::addPlayerBan(uint32_t playerId, uint32_t time, uint32_t adminid, std::string comment)
{
	if(playerId == 0) return;

	Database* db = Database::instance();
	DBInsert stmt(db);
	DBQuery query;

	stmt.setQuery("INSERT INTO `bans` (`type`, `value`, `expires`, `added`, `admin_id`, `comment`) VALUES ");

	query << BAN_PLAYER << ", " << playerId << ", " << time << ", " << std::time(NULL) << ", " << adminid << ", " << db->escapeString(comment);
	if(!stmt.addRow(query.str())){
		return;
	}

	if(!stmt.execute()){
		return;
	}
}

void BanManager::addPlayerBan(std::string name, uint32_t time, uint32_t adminid, std::string comment)
{
	uint32_t guid;
	if(IOPlayer::instance()->getGuidByName(guid, name)) {
		addPlayerBan(guid, time, adminid, comment);
	}
}

void BanManager::addAccountBan(uint32_t account, uint32_t time, uint32_t adminid, std::string comment)
{
	if(account == 0) return;

	Database* db = Database::instance();
	DBInsert stmt(db);
	DBQuery query;

	stmt.setQuery("INSERT INTO `bans` (`type`, `value`, `expires`, `added`, `admin_id`, `comment`) VALUES ");

	query << BAN_ACCOUNT << ", " << account << ", " << time << ", " << std::time(NULL) << ", " << adminid << ", " << db->escapeString(comment);
	if(!stmt.addRow(query.str())){
		return;
	}

	if(!stmt.execute()){
		return;
	}
}

bool BanManager::removeIpBans(uint32_t ip, uint32_t mask)
{
	if(isIpBanished(ip, mask) == false) {
		return false;
	}

	Database* db = Database::instance();
	DBQuery query;
	query << "UPDATE `bans` SET `active` = 0 WHERE `type` = " << BAN_IPADDRESS << " AND (`value` & `param` & " << mask << ") = (" << ip << " & `param` & " << mask << ")" << " AND `active` = 1";
	return db->executeQuery(query.str());
}

bool BanManager::removePlayerBans(uint32_t guid)
{
	if(isPlayerBanished(guid) == false) {
		return false;
	}

	Database* db = Database::instance();
	DBQuery query;
	query << "UPDATE `bans` SET `active` = 0 WHERE `type` = " << BAN_PLAYER << " AND `value` = " << guid << " AND `active` = 1";
	return db->executeQuery(query.str());
}

bool BanManager::removePlayerBans(std::string name) {
	uint32_t playerId = 0;
	if(!IOPlayer::instance()->getGuidByName(playerId, name))
		return false;
	return removePlayerBans(playerId);
}

bool BanManager::removeAccountBans(uint32_t accno)
{
	if(isAccountBanished(accno) == false) {
		return false;
	}
	Database* db = Database::instance();
	DBQuery query;
	query << "UPDATE `bans` SET `active` = 0 WHERE `type` = " << BAN_ACCOUNT << " AND `value` = " << accno << " AND `active` = 1";
	return db->executeQuery(query.str());
}

std::vector<Ban> BanManager::getBans(BanType_t type) const {
	assert(type == BAN_IPADDRESS || type == BAN_PLAYER || type == BAN_ACCOUNT);

	Database* db = Database::instance();
	DBQuery query;
	DBResult* result;

	uint32_t currentTime = std::time(NULL);
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
			"(`expires` >= " << currentTime << " OR `expires` = 0)";

	std::vector<Ban> vec;
	if((result = db->storeQuery(query.str())) != NULL){
		do {
			Ban ban;
			ban.type = type;
			ban.id = result->getDataInt("id");
			ban.value = result->getDataString("value");
			ban.param = result->getDataString("param");
			ban.expires = (uint32_t)result->getDataLong("expires");
			ban.added = (uint32_t)result->getDataLong("id");
			ban.adminid = result->getDataInt("admin_id");
			ban.reason = (uint32_t)result->getDataLong("reason");
			ban.comment = result->getDataString("comment");
			vec.push_back(ban);
		} while(result->next());

		db->freeResult(result);
	}
	return vec;
}

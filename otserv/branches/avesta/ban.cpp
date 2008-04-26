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

Ban::Ban()
{
	OTSYS_THREAD_LOCKVARINIT(banLock);
}

void Ban::init()
{
	maxLoginTries = (uint32_t)g_config.getNumber(ConfigManager::LOGIN_TRIES);
	retryTimeout = (uint32_t)g_config.getNumber(ConfigManager::RETRY_TIMEOUT) / 1000;
	loginTimeout = (uint32_t)g_config.getNumber(ConfigManager::LOGIN_TIMEOUT) / 1000;
}

bool Ban::isIpBanished(uint32_t clientip)
{
	if(clientip != 0){
		OTSYS_THREAD_LOCK(banLock, "");

		for(IpBanList::iterator it = ipBanList.begin(); it !=  ipBanList.end(); ++it){
			if((it->ip & it->mask) == (clientip & it->mask)){
				uint32_t currentTime = std::time(NULL);

				if(it->time == 0 || currentTime < it->time){
					OTSYS_THREAD_UNLOCK(banLock, "");
					return true;
				}
			}
		}

		OTSYS_THREAD_UNLOCK(banLock, "");
	}

	return false;
}

bool Ban::isIpDisabled(uint32_t clientip)
{
	if(maxLoginTries == 0)
		return false;

	if(clientip != 0){
		OTSYS_THREAD_LOCK(banLock, "");

		uint32_t currentTime = std::time(NULL);
		IpLoginMap::const_iterator it = ipLoginMap.find(clientip);
		if(it != ipLoginMap.end()){
			if( (it->second.numberOfLogins >= maxLoginTries) &&
				(currentTime < it->second.lastLoginTime + loginTimeout) )
			{
				OTSYS_THREAD_UNLOCK(banLock, "");
				return true;
			}
		}

		OTSYS_THREAD_UNLOCK(banLock, "");
	}

	return false;
}

bool Ban::acceptConnection(uint32_t clientip)
{
	if(clientip == 0)
		return false;

	OTSYS_THREAD_LOCK(banLock, "");

	uint64_t currentTime = OTSYS_TIME();

	IpConnectMap::iterator it = ipConnectMap.find(clientip);
	if(it == ipConnectMap.end()){
		ConnectBlock cb;
		cb.lastConnection = currentTime;

		ipConnectMap[clientip] = cb;

		OTSYS_THREAD_UNLOCK(banLock, "");
		return true;
	}

	if(currentTime - it->second.lastConnection < 1000){
		OTSYS_THREAD_UNLOCK(banLock, "");
		return false;
	}

	it->second.lastConnection = currentTime;

	OTSYS_THREAD_UNLOCK(banLock, "");
	return true;
}

void Ban::addLoginAttempt(uint32_t clientip, bool isSuccess)
{
	if(clientip != 0){
		OTSYS_THREAD_LOCK(banLock, "");

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

		OTSYS_THREAD_UNLOCK(banLock, "");
	}
}

bool Ban::isPlayerBanished(const std::string& name)
{

	uint32_t playerId;
	std::string playerName = name;
	if(!IOPlayer::instance()->getGuidByName(playerId, playerName))
		return false;

	OTSYS_THREAD_LOCK(banLock, "");

	for(PlayerBanList::iterator it = playerBanList.begin(); it !=  playerBanList.end(); ++it){
   		if(it->id  == playerId){
			uint32_t currentTime = std::time(NULL);
			if(it->time == 0 || currentTime < it->time){
				OTSYS_THREAD_UNLOCK(banLock, "");
				return true;
			}
		}
	}

	OTSYS_THREAD_UNLOCK(banLock, "");
	return false;
}

bool Ban::isAccountBanished(uint32_t account)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(banLock);
	uint32_t currentTime = std::time(NULL);
	for(AccountBanList::iterator it = accountBanList.begin(); it !=  accountBanList.end(); ++it){
   		if(it->id  == account){
			if(it->time == 0 || currentTime < it->time){
				return true;
			}
   		}
	}

	return false;
}

void Ban::addIpBan(uint32_t ip, uint32_t mask, uint32_t time)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(banLock);
	for(IpBanList::iterator it = ipBanList.begin(); it !=  ipBanList.end(); ++it){
		if(it->ip == ip && it->mask == mask){
			it->time = time;
			return;
		}
	}

	IpBanStruct ipBanStruct(ip, mask, time);
	ipBanList.push_back(ipBanStruct);
}

void Ban::addPlayerBan(uint32_t playerId, uint32_t time)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(banLock);
	for(PlayerBanList::iterator it = playerBanList.begin(); it !=  playerBanList.end(); ++it){
		if(it->id == playerId){
			it->time = time;
			return;
		}
	}

	PlayerBanStruct playerBanStruct(playerId, time);
	playerBanList.push_back(playerBanStruct);
}

void Ban::addAccountBan(uint32_t account, uint32_t time)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(banLock);
	for(AccountBanList::iterator it = accountBanList.begin(); it !=  accountBanList.end(); ++it){
		if(it->id == account){
			it->time = time;
			return;
		}
	}

	AccountBanStruct accountBanStruct(account, time);
	accountBanList.push_back(accountBanStruct);
}

bool Ban::removeIpBan(uint32_t n)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(banLock);
	for(IpBanList::iterator it = ipBanList.begin(); it !=  ipBanList.end(); ++it){
		--n;
		if(n == 0){
			ipBanList.erase(it);
			return true;
		}
	}
	return false;
}

bool Ban::removePlayerBan(uint32_t n)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(banLock);
	for(PlayerBanList::iterator it = playerBanList.begin(); it !=  playerBanList.end(); ++it){
		--n;
		if(n == 0){
			playerBanList.erase(it);
			return true;
		}
	}
	return false;
}

bool Ban::removeAccountBan(uint32_t n)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(banLock);
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


bool Ban::loadBans()
{
	return IOBan::getInstance()->loadBans(*this);
}
bool Ban::saveBans()
{
	return IOBan::getInstance()->saveBans(*this);
}

bool IOBan::loadBans(Ban& banclass)
{
	Database* db = Database::instance();
	DBResult* result;
	DBQuery query;

	if(!(result = db->storeQuery("SELECT COUNT(*) FROM `bans`"))){
		return false;
	}

	uint32_t nbans = result->getDataInt("COUNT(*)");
	db->freeResult(result);
	if(nbans == 0){
		return true;
	}

	if(!(result = db->storeQuery("SELECT * FROM `bans`"))){
		return false;
	}

	uint32_t currentTime = std::time(NULL);
	do{
		int banType = result->getDataInt("type");
		int time = result->getDataInt("time");
		if(time > (int)currentTime){
			switch(banType){
				case BAN_IPADDRESS:
				{
					int ip = result->getDataInt("ip");
					int mask = result->getDataInt("mask");
					banclass.addIpBan(ip, mask, time);
					break;
				}

				case BAN_PLAYER:
				{
					int player = result->getDataInt("player");
					banclass.addPlayerBan(player, time);
					break;
				}

				case BAN_ACCOUNT:
				{
					int account = result->getDataInt("account");
					banclass.addAccountBan(account, time);
					break;
				}
			}
		}
	}while(result->next());
	db->freeResult(result);
	return true;
}

bool IOBan::saveBans(const Ban& banclass)
{
	Database* db = Database::instance();
	DBQuery query;
	DBTransaction transaction(db);

	if(!transaction.begin())
		return false;

	if(!db->executeQuery("DELETE FROM `bans`")){
		return false;
	}

	uint32_t currentTime = std::time(NULL);
	//save ip bans

	DBInsert stmt(db);
	stmt.setQuery("INSERT INTO `bans` (`type`, `ip`, `mask`, `time`) VALUES ");

	for(IpBanList::const_iterator it = banclass.ipBanList.begin(); it !=  banclass.ipBanList.end(); ++it){
		if(it->time > currentTime){
			query << 1 << ", " << it->ip << ", " << it->mask << ", " << it->time;

			if(!stmt.addRow(query)){
				return false;
			}
		}
	}

	if(!stmt.execute()){
		return false;
	}

	//save player bans
	stmt.setQuery("INSERT INTO `bans` (`type`, `player`, `time`) VALUES ");

	for(PlayerBanList::const_iterator it = banclass.playerBanList.begin(); it !=  banclass.playerBanList.end(); ++it){
		if(it->time > currentTime){
			query << 2 << ", " << it->id << ", " << it->time;

			if(!stmt.addRow(query)){
				return false;
			}
		}
	}

	if(!stmt.execute()){
		return false;
	}

	//save account bans
	stmt.setQuery("INSERT INTO `bans` (`type`, `account`, `time`) VALUES ");

	for(AccountBanList::const_iterator it = banclass.accountBanList.begin(); it != banclass.accountBanList.end(); ++it){
		if(it->time > currentTime){
			query << 3 << ", " << it->id << ", " << it->time;

			if(!stmt.addRow(query)){
				return false;
			}
		}
	}

	if(!stmt.execute()){
		return false;
	}

	return transaction.commit();
}

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
#include "otsystem.h"
#include "tools.h"
#include "configmanager.h"
#include "ioplayer.h"
#include "database_driver.h"

extern ConfigManager g_config;

bool BanManager::clearTemporaryBans() const
{
  DatabaseDriver* db = DatabaseDriver::instance();
  DBQuery query;
  return db->executeQuery("UPDATE `bans` SET `active` = 0 WHERE `expires` = 0");
}

bool BanManager::acceptConnection(uint32_t clientip)
{
  if(clientip == 0) return false;
  banLock.lock();

  uint64_t currentTime = OTSYS_TIME();
  IpConnectMap::iterator it = ipConnectMap.find(clientip);
  if(it == ipConnectMap.end()){
    ConnectBlock cb;
    cb.startTime = currentTime;
    cb.blockTime = 0;
    cb.count = 1;

    ipConnectMap[clientip] = cb;
    banLock.unlock();
    return true;
  }

  it->second.count++;
  if(it->second.blockTime > currentTime){
    banLock.unlock();
    return false;
  }

  if(currentTime - it->second.startTime > 1000){
    uint32_t connectionPerSec = it->second.count;
    it->second.startTime = currentTime;
    it->second.count = 0;
    it->second.blockTime = 0;

    if(connectionPerSec > 10){
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

  time_t currentTime = (OTSYS_TIME() / 1000);
  IpLoginMap::iterator it = ipLoginMap.find(clientip);
  if(it != ipLoginMap.end()){
    uint32_t loginTimeout = (uint32_t)g_config.getNumber(ConfigManager::LOGIN_TIMEOUT) / 1000;
    if( (it->second.numberOfLogins >= (uint32_t)g_config.getNumber(ConfigManager::LOGIN_TRIES)) &&
        ((uint32_t)currentTime < (uint32_t)it->second.lastLoginTime + loginTimeout) )
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
  if(clientip == 0){
    return false;
  }
  
  DatabaseDriver* db = DatabaseDriver::instance();

  DBQuery query;
  query <<
    "SELECT COUNT(*) AS `count` "
    "FROM `ip_bans` "
    "INNER JOIN `bans` ON `bans`.`id` = `ip_bans`.`ban_id` "
    "WHERE " << "((" << clientip << " & " << mask << " & `mask`) = (`ip` & `mask` & " << mask << ")) "
    "AND `active` = 1 AND (`expires` >= " << (OTSYS_TIME() / 1000) << " OR `expires` <= 0)";

  DBResult_ptr result;
  if(!(result = db->storeQuery(query.str()))){
    return false;
  }

  return result->getDataInt("count") > 0;
}

bool BanManager::isPlayerBanished(uint32_t playerId) const
{
  DatabaseDriver* db = DatabaseDriver::instance();

  DBQuery query;  
  query <<
    "SELECT COUNT(*) AS `count` "
    "FROM `player_bans` "
    "INNER JOIN `bans` ON `bans`.`id` = `player_bans`.`ban_id` "
    "WHERE "
    "`player_id` = " << playerId << " AND "
    "`active` = 1 AND "
    "(`expires` >= " << (OTSYS_TIME() / 1000) << " OR `expires` <= 0)";

  DBResult_ptr result;
  if(!(result = db->storeQuery(query.str()))){
    return false;
  }

  int t = result->getDataInt("count");
  return t > 0;
}

bool BanManager::isPlayerBanished(const std::string& name) const
{
  uint32_t playerId;
  std::string n = name;
  return IOPlayer::instance()->getGuidByName(playerId, n) && isPlayerBanished(playerId);
}

bool BanManager::isAccountBanished(uint32_t accountId) const
{
  DatabaseDriver* db = DatabaseDriver::instance();

  DBQuery query;
  query <<
    "SELECT COUNT(*) as `count` "
    "FROM `account_bans` "
    "INNER JOIN `bans` ON `bans`.`id` = `account_bans`.`ban_id` "
    "WHERE "
    "`account_id` = " << accountId << " AND `active` = 1 AND (`expires` >= " << (OTSYS_TIME() / 1000) << " OR `expires` <= 0)";

  DBResult_ptr result;
  if(!(result = db->storeQuery(query.str())))
  return false;

  int t = result->getDataInt("count");
  return t > 0;
}

void BanManager::addLoginAttempt(uint32_t clientip, bool isSuccess)
{
  if(clientip == 0){
    return;
  }
  
  banLock.lock();

  time_t currentTime = (OTSYS_TIME() / 1000);
  IpLoginMap::iterator it = ipLoginMap.find(clientip);
  if(it == ipLoginMap.end()){
    LoginBlock lb;
    lb.lastLoginTime = 0;
    lb.numberOfLogins = 0;

    ipLoginMap[clientip] = lb;
    it = ipLoginMap.find(clientip);
  }

  if(it->second.numberOfLogins >= (uint32_t)g_config.getNumber(ConfigManager::LOGIN_TRIES)){
    it->second.numberOfLogins = 0;
  }
  
  uint32_t retryTimeout = (uint32_t)g_config.getNumber(ConfigManager::RETRY_TIMEOUT) / 1000;
  if(!isSuccess || ((uint32_t)currentTime < (uint32_t)it->second.lastLoginTime + retryTimeout)){
    ++it->second.numberOfLogins;
  } else {
    it->second.numberOfLogins = 0;
  }

  it->second.lastLoginTime = currentTime;
  banLock.unlock();
}

bool BanManager::addIpBan(uint32_t ip, uint32_t mask, int32_t time,
uint32_t adminid, std::string comment) const
{
  if(ip == 0 || mask == 0){
    return false;
  }
  
  DatabaseDriver* db = DatabaseDriver::instance();
  
  DBInsert stmt(db);
  DBQuery query;
  
  stmt.setQuery("INSERT INTO `bans` (`expires`, `added`, `active`, `admin_id`, `comment`) VALUES ");
  query << time << ", " << (OTSYS_TIME() / 1000) << ", 1, " << adminid << ", " << db->escapeString(comment);
  
  if(!stmt.addRow(query.str())){
    return false;
  }
  
  if(!stmt.execute()){
    return false;
  }
  
  int banId = db->getLastInsertedRowID();
  
  stmt.setQuery("INSERT INTO `ip_bans` (`ban_id`, `ip`, `mask`) VALUES ");
  query.reset();
  query << banId << ", " << ip << ", " << mask;
  
  if(!stmt.addRow(query.str())){
    return false;
  }
  
  if(!stmt.execute()){
    return false;
  }
  
  return true;
}

bool BanManager::addPlayerBan(uint32_t playerId, int32_t time, uint32_t adminid,
std::string comment, std::string statement, uint32_t reason, ViolationAction action) const
{
  if(playerId == 0){
    return false;
  }
  
  DatabaseDriver* db = DatabaseDriver::instance();

  DBInsert stmt(db);
  DBQuery query;
  
  stmt.setQuery("INSERT INTO `bans` (`expires`, `added`, `active`, `admin_id`, `comment`) VALUES ");
  query << time << ", " << (OTSYS_TIME() / 1000) << ", 1, " << adminid << ", " << db->escapeString(comment);
  
  if(!stmt.addRow(query.str())){
    return false;
  }
  
  if(!stmt.execute()){
    return false;
  }
  
  int banId = db->getLastInsertedRowID();
  
  stmt.setQuery("INSERT INTO `player_bans` (`ban_id`, `player_id`) VALUES ");
  query.reset();
  query << banId << ", " << playerId;
  
  if(!stmt.addRow(query.str())){
    return false;
  }
  
  if(!stmt.execute()){
    return false;
  }
  
  return true;
}

bool BanManager::addPlayerBan(const std::string& name, int32_t time, uint32_t adminid,
std::string comment, std::string statement, uint32_t reason, ViolationAction action) const
{
  uint32_t guid = 0;
  std::string n = name;
  return IOPlayer::instance()->getGuidByName(guid, n) && addPlayerBan(guid, time, adminid, comment, statement, reason, action);
}

bool BanManager::addPlayerStatement(uint32_t playerId, uint32_t adminid, std::string comment,
std::string statement, uint32_t reason, ViolationAction action) const
{
  if(playerId == 0){
    return false;
  }
  
  /** 
    There is no place for statements in database 
  */
  
  return false;
}

bool BanManager::addAccountBan(uint32_t account, int32_t time, uint32_t adminid,
std::string comment, std::string statement, uint32_t reason, ViolationAction action) const
{
  if(account == 0){
    return false;
  }
  
  DatabaseDriver* db = DatabaseDriver::instance();

  DBInsert stmt(db);
  DBQuery query;
  
  stmt.setQuery("INSERT INTO `bans` (`expires`, `added`, `active`, `admin_id`, `comment`) VALUES ");
  query << time << ", " << (OTSYS_TIME() / 1000) << ", 1, " << adminid << ", " << db->escapeString(comment);
  
  if(!stmt.addRow(query.str())){
    return false;
  }
  
  if(!stmt.execute()){
    return false;
  }
  
  int banId = db->getLastInsertedRowID();
  
  stmt.setQuery("INSERT INTO `account_bans` (`ban_id`, `account_id`) VALUES ");
  query.reset();
  query << banId << ", " << account;
  
  if(!stmt.addRow(query.str())){
    return false;
  }
  
  if(!stmt.execute()){
    return false;
  }
  
  return true;
}

bool BanManager::addAccountNotation(uint32_t account, uint32_t adminid, std::string comment,
std::string statement, uint32_t reason, ViolationAction action) const
{
  if(account == 0){
    return false;
  }
  
  /** 
    There is no place for notations in database 
  */
  
  return false;
}

bool BanManager::removeIpBans(uint32_t ip, uint32_t mask) const
{
  if(!isIpBanished(ip, mask)){
    return false;
  }
  
  DatabaseDriver* db = DatabaseDriver::instance();

  DBQuery query;
  query <<
    "SELECT `ban_id` AS `id` "
    "FROM `ip_bans` "
    "INNER JOIN `bans` ON `bans`.`id` = `ip_bans`.`ban_id` "
    "WHERE " << "((" << ip << " & " << mask << " & `mask`) = (`ip` & `mask` & " << mask << ")) "
    "AND `active` = 1";
  
  for(DBResult_ptr result = db->storeQuery(query.str()); result; result = result->advance()){
    query.reset();
    query << "UPDATE `bans` SET `active` = 0 WHERE `id` = " << result->getDataInt("id");
    if(!db->executeQuery(query.str())){
      return false;
    }
  }

  return true;
}

bool BanManager::removePlayerBans(uint32_t guid) const
{
  if(!isPlayerBanished(guid)){
    return false;
  }
  
  DatabaseDriver* db = DatabaseDriver::instance();

  DBQuery query;
  query <<
    "SELECT `ban_id` AS `id` "
    "FROM `player_bans` "
    "INNER JOIN `bans` ON `bans`.`id` = `player_bans`.`ban_id` "
    "WHERE `player_id` = " << guid << " "
    "AND `active` = 1";
  
  for(DBResult_ptr result = db->storeQuery(query.str()); result; result = result->advance()){
    query.reset();
    query << "UPDATE `bans` SET `active` = 0 WHERE `id` = " << result->getDataInt("id");
    if(!db->executeQuery(query.str())){
      return false;
    }
  }

  return true;
}

bool BanManager::removePlayerBans(const std::string& name) const
{
  uint32_t playerId = 0;
  std::string n = name;
  return IOPlayer::instance()->getGuidByName(playerId, n) && removePlayerBans(playerId);
}

bool BanManager::removeAccountBans(uint32_t accno) const
{
  if(!isAccountBanished(accno)){
    return false;
  }
  
  DatabaseDriver* db = DatabaseDriver::instance();

  DBQuery query;
  query <<
  "SELECT `ban_id` AS `id` "
  "FROM `account_bans` "
  "INNER JOIN `bans` ON `bans`.`id` = `account_bans`.`ban_id` "
  "WHERE `account_id` = " << accno << " "
  "AND `active` = 1";
  
  for(DBResult_ptr result = db->storeQuery(query.str()); result; result = result->advance()){
    query.reset();
    query << "UPDATE `bans` SET `active` = 0 WHERE `id` = " << result->getDataInt("id");
    if(!db->executeQuery(query.str())){
      return false;
    }
  }

  return true;
}

bool BanManager::removeNotations(uint32_t accno) const
{
  /** 
    There is no place for notations in database 
  */
  
  return false;
}

uint32_t BanManager::getNotationsCount(uint32_t account) const
{
  /** 
    There is no place for notations in database 
  */
  
  return 0;
}

std::vector<Ban> BanManager::getBans(BanType_t type)
{
  std::vector<Ban> ret;
  DatabaseDriver* db = DatabaseDriver::instance();
  DBQuery query;
  
  switch(type){
  case BAN_IPADDRESS: {
      query <<
        "SELECT `id`, `ip` as `value`, `mask` as `param`, `expires`, `added`, `admin_id`, `comment`"
        "FROM `ip_bans` "
        "INNER JOIN `bans` ON `bans`.`id` = `ip_bans`.`ban_id` "
        "WHERE "
        "`active` = 1 "
        "AND "
        "(`expires` >= " << (OTSYS_TIME() / 1000) << " OR `expires` <= 0)";
      break;
    }
    
  case BAN_PLAYER: {
      query <<
        "SELECT `id`, `player_id` as `value`, `expires`, `added`, `admin_id`, `comment`"
        "FROM `player_bans` "
        "INNER JOIN `bans` ON `bans`.`id` = `player_bans`.`ban_id` "
        "WHERE "
        "`active` = 1 "
        "AND "
        "(`expires` >= " << (OTSYS_TIME() / 1000) << " OR `expires` <= 0)";
      break;
    }
    
  case BAN_ACCOUNT: {
      query <<
        "SELECT `id`, `account_id` as `value`, `expires`, `added`, `admin_id`, `comment`"
        "FROM `account_bans` "
        "INNER JOIN `bans` ON `bans`.`id` = `account_bans`.`ban_id` "
        "WHERE "
        "`active` = 1 "
        "AND "
        "(`expires` >= " << (OTSYS_TIME() / 1000) << " OR `expires` <= 0)";
      break;
    }
    
  default: break;
  }
  
  if(query.good()){
    for (DBResult_ptr result = db->storeQuery(query.str()); result; result = result->advance()) {
      Ban ban;
      
      ban.type = type;
      ban.id = result->getDataInt("id");
      ban.value = result->getDataString("value");
      ban.param = result->getDataString("param");
      ban.expires = (time_t)result->getDataLong("expires");
      ban.added = (uint32_t)result->getDataLong("added");
      ban.adminId = result->getDataInt("admin_id");
      ban.comment = result->getDataString("comment");
      
      ret.push_back(ban);
    }
  }
  
  return ret;
}

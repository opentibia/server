//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Base class for the Account Loader/Saver
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

#include "ioaccount.h"
#include "database_driver.h"
#include "game.h"
#include "configmanager.h"

extern Game g_game;
extern ConfigManager g_config;

bool predicateAccountCharactersByName(const AccountCharacter& a, const AccountCharacter& b)
{
  return a.name < b.name;
}

Account IOAccount::loadAccount(const std::string& accountName, bool preLoad /* = false*/)
{
  Account acc;
  acc.name = accountName;

  if(g_game.onAccountLogin(
    acc.name, acc.number, acc.password,
    acc.premiumEnd, acc.warnings, acc.charList)){
    //handled by script
    return acc;
  }

  DatabaseDriver* db = DatabaseDriver::instance();
  DBQuery query;
  DBResult_ptr result;

  query << "SELECT `id`, `name`, `password`, `premend`, `warnings` FROM `accounts` WHERE `name` = " << db->escapeString(accountName);
  if(!(result = db->storeQuery(query))){
    return acc;
  }

  acc.number = result->getDataInt("id");
  acc.password = result->getDataString("password");
  acc.premiumEnd = result->getDataInt("premend");
  acc.name = result->getDataString("name");
  acc.warnings = result->getDataInt("warnings");

  if(preLoad)
    return acc;

  query.reset();
  query << "SELECT " <<
      "`players`.`name` AS `name`, `worlds`.`name` AS `world`, " <<
            "`worlds`.`port` AS `port`, `worlds`.`ip` AS `ip`, `worlds`.`id` AS `world_id`" <<
    "FROM `players` " <<
    "LEFT JOIN `worlds` ON `worlds`.`id` = `players`.`world_id` " <<
    "WHERE `account_id` = " << acc.number;

  for(result = db->storeQuery(query); result; result = result->advance()) {
    AccountCharacter c;
    c.name = result->getDataString("name");
        c.world_name = result->getDataString("world");
        c.world_id = (uint16_t)result->getDataInt("world_id");
    c.port = (uint16_t)result->getDataInt("port");
        c.ip = (uint32_t)result->getDataLong("ip");

    acc.charList.push_back(c);
  }

  acc.charList.sort(predicateAccountCharactersByName);
  return acc;
}

bool IOAccount::saveAccount(const Account& acc)
{
  DatabaseDriver* db = DatabaseDriver::instance();
  DBQuery query;

  query << "UPDATE `accounts` SET `premend` = " << acc.premiumEnd << ", `warnings` = " << acc.warnings << " WHERE `id` = " << acc.number;
  return db->executeQuery(query);
}

bool IOAccount::getPassword(const std::string& accountName, const std::string& playerName, std::string& password)
{
  DatabaseDriver* db = DatabaseDriver::instance();
  DBQuery query;
  DBResult_ptr result;

  query << "SELECT `accounts`.`password` AS `password` FROM `accounts`, `players` " <<
    "WHERE `accounts`.`name` = " << db->escapeString(accountName) << 
    " AND `accounts`.`id` = `players`.`account_id` AND `players`.`name` = " << db->escapeString(playerName);
  if((result = db->storeQuery(query.str()))){
    password = result->getDataString("password");
    return true;
  }

  return false;
}

uint16_t IOAccount::getPremiumDaysLeft(uint32_t time)
{
  uint32_t now = (uint32_t)std::time(NULL) / 86400;
  
  if(uint32_t(time / 86400) < now)
    return 0;

  if(uint32_t(time / 86400) - now >= 0xFFFF)
    return 0xFFFF;

  return uint16_t(uint32_t(time / 86400) - now);
}


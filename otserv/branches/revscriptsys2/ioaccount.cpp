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
#include "database.h"
#include "configmanager.h"
#include "account.h"

extern ConfigManager g_config;

Account IOAccount::loadAccount(const std::string& name, bool preLoad /* = false*/)
{
	Account acc;

	Database* db = Database::instance();
	DBQuery query;
	DBResult* result;

	query << "SELECT `id`, `name`, `password`, `premend`, `warnings` FROM `accounts` WHERE `name` = " << db->escapeString(name);
	if(!(result = db->storeQuery(query.str()))){
		return acc;
	}

	acc.number = result->getDataInt("id");
	acc.password = result->getDataString("password");
	acc.premEnd = result->getDataInt("premend");
	acc.name = result->getDataString("name");
	acc.warnings = result->getDataInt("warnings");
	db->freeResult(result);

	if(preLoad)
		return acc;

	query.str("");
	query << "SELECT `name` FROM `players` WHERE `account_id` = " << acc.number;
	if(!(result = db->storeQuery(query.str()))){
		return acc;
	}

	do {
		acc.charList.push_back(result->getDataString("name"));
	} while(result->next());

	acc.charList.sort();
	db->freeResult(result);
	return acc;
}

bool IOAccount::saveAccount(Account acc)
{
	Database* db = Database::instance();
	DBQuery query;

	query << "UPDATE `accounts` SET `premend` = " << acc.premEnd << ", `warnings` = " << acc.warnings << " WHERE `id` = " << acc.number;
	return db->executeQuery(query.str());
}

bool IOAccount::getPassword(const std::string& accountname, const std::string &name, std::string &password)
{
	Database* db = Database::instance();
	DBQuery query;
	DBResult* result;

	query << "SELECT `accounts`.`password` AS `password` FROM `accounts`, `players` WHERE `accounts`.`name` = " << db->escapeString(accountname) << " AND `accounts`.`id` = `players`.`account_id` AND `players`.`name` = " << db->escapeString(name);
	if((result = db->storeQuery(query.str()))){
		password = result->getDataString("password");
		db->freeResult(result);
		return true;
	}

	return false;
}

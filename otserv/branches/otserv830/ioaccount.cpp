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

#include "definitions.h"
#include "ioaccount.h"

#include <algorithm>
#include <functional>
#include <sstream>

#include "database.h"
#include <iostream>

#include "configmanager.h"

extern ConfigManager g_config;

Account IOAccount::loadAccount(const std::string& name)
{
	Account acc;
#ifndef __USE_SQL_PREMDAYS__
	time_t premEnd;
#endif
	uint32_t premDays;
	uint32_t today = uint32_t(time(NULL) / 86400);

	Database* db = Database::instance();
	DBQuery query;
	DBResult* result;

	query << "SELECT `name`, `id`, `password` FROM `accounts` WHERE `name` = " << db->escapeString(name);
	if((result = db->storeQuery(query.str()))){
		acc.accnumber = result->getDataInt("id");
		acc.password = result->getDataString("password");
		acc.name = result->getDataInt("name");
		db->freeResult(result);

		query.str("");
#ifndef __USE_SQL_PREMDAYS__
		query << "SELECT `name`, `premend` FROM `players` WHERE `account_id` = " << acc.accnumber;
#else
		query << "SELECT `name`, `premdays` FROM `players` WHERE `account_id` = " << acc.accnumber;
#endif
		if((result = db->storeQuery(query.str()))){
			do{
				std::string ss = result->getDataString("name");
				acc.charList.push_back(ss.c_str());
#ifndef __USE_SQL_PREMDAYS__
				premEnd = result->getDataInt("premend");
				premDays = uint32_t(premEnd / 86400) - today;
				// give the account the highest number of premium days
				if(premDays > acc.premDays) {
					acc.premDays = premDays;
				}
#else
				premDays = result->getDataInt("premdays");
				// give the account the highest number of premium days
				if(premDays > acc.premDays) {
					acc.premDays = premDays;
				}
#endif
			}while(result->next());

			acc.charList.sort();
			db->freeResult(result);
		}
	}

	return acc;
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

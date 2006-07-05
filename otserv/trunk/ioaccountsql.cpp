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

#include "ioaccountsql.h"
#include <algorithm>
#include <functional>
#include <sstream>

#include "database.h"
#include <iostream>

#include "configmanager.h"

extern ConfigManager g_config;

IOAccountSQL::IOAccountSQL()
{
	m_host = g_config.getString(ConfigManager::SQL_HOST);
	m_user = g_config.getString(ConfigManager::SQL_USER);
	m_pass = g_config.getString(ConfigManager::SQL_PASS);
	m_db   = g_config.getString(ConfigManager::SQL_DB);
}

Account IOAccountSQL::loadAccount(unsigned long accno)
{
	Account acc;

//	try
//	{
		Database* mysql = Database::instance();

		if(!mysql->connect(m_db.c_str(), m_host.c_str(), m_user.c_str(), m_pass.c_str())){
			return acc;
		}

		DBQuery query;
		DBResult result;

		query << "SELECT * FROM accounts WHERE accno='" << accno << "'";
		if(!mysql->storeQuery(query, result))
			return acc;

		acc.accnumber = result.getDataInt("accno");
		acc.password = result.getDataString("password");
		acc.accType = result.getDataInt("type");
		acc.premDays = result.getDataInt("premDays");

        /*std::cout << "pass '" << acc.password
        << "'      acc '" << acc.accnumber
        << "'     type '" << acc.accType
        << "'      prem '" << acc.premDays
        << "'" <<std::endl;*/

		query << "SELECT name FROM players where account='" << accno << "'";
		if(!mysql->storeQuery(query, result))
			return acc;

		for(int i=0; i < result.getNumRows(); ++i){
			std::string ss = result.getDataString("name", i);
			acc.charList.push_back(ss.c_str());
		}

		acc.charList.sort();

/*	}
	catch(DBError e)
	{
		switch(e.getType())
		{
		case DB_ERROR_QUERY:
		case DB_ERROR_STORE:
		case DB_ERROR_DATA_NOT_FOUND:
			std::cout << "DB WARNING: (" << e.getType() << ") " << e.getMsg() << std::endl;
			break;
		default:
			std::cout << "DB ERROR: (" << e.getType() << ") " << e.getMsg() << std::endl;
			return acc;
		}
	}
	catch(...)
	{
		std::cout << "ERROR: Unknown exception raised.\n\tFile: " << __FILE__ << "\n\tLine: " << __LINE__ << std::endl;
		return acc;
	}*/


	return acc;
}


bool IOAccountSQL::getPassword(unsigned long accno, const std::string &name, std::string &password)
{
	Database* mysql = Database::instance();
	if(!mysql->connect(m_db.c_str(), m_host.c_str(), m_user.c_str(), m_pass.c_str())){
		return false;
	}

	DBQuery query;
	DBResult result;

	query << "SELECT password FROM accounts WHERE accno='" << accno << "'";
	if(!mysql->storeQuery(query, result))
		return false;

	std::string acc_password = result.getDataString("password");

	query << "SELECT name FROM players where account='" << accno << "'";
	if(!mysql->storeQuery(query, result))
		return false;

	for(int i=0; i < result.getNumRows(); ++i)
	{
		std::string ss = result.getDataString("name", i);
		if(ss == name){
			password = acc_password;
			return true;
		}
	}
	return false;
}

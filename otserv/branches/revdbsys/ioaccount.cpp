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

IOAccount* IOAccount::_instance = NULL;

IOAccount* IOAccount::instance(){
	if(!_instance){
		_instance = new IOAccount;
	}
	#ifdef __DEBUG__
	printf("%s \n", _instance->getSourceDescription());
	#endif
	return _instance;
}

IOAccount::IOAccount()
{
	//
}

Account IOAccount::loadAccount(uint32_t accno)
{
	Account acc;

	Database* db = Database::instance();
	DBQuery query;
	DBResult result;

	query << "SELECT id,password FROM accounts WHERE id=" << accno;
	if(db->connect() && db->storeQuery(query, result)){
		acc.accnumber = result.getDataInt("id");
		acc.password = result.getDataString("password");
		query << "SELECT name FROM players WHERE account_id=" << accno;
		if(db->storeQuery(query, result)){
			for(uint32_t i = 0; i < result.getNumRows(); ++i){
				std::string ss = result.getDataString("name", i);
				acc.charList.push_back(ss.c_str());
			}
			acc.charList.sort();
		}
	}
	return acc;
}


bool IOAccount::getPassword(uint32_t accno, const std::string &name, std::string &password)
{
	Database* db = Database::instance();
	DBQuery query;
	DBResult result;

	query << "SELECT password FROM accounts WHERE id=" << accno;
	if(db->connect() && db->storeQuery(query, result) && (result.getNumRows() == 1)){
		std::string acc_password = result.getDataString("password");

		query << "SELECT name FROM players WHERE account_id=" << accno;
		if(db->storeQuery(query, result)){

			for(uint32_t i = 0; i < result.getNumRows(); ++i){
				std::string ss = result.getDataString("name", i);
				if(ss == name){
					password = acc_password;
					return true;
				}
			}

		}
	}
	return false;
}

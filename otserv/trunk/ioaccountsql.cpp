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
#include <mysql++.h>
#include "luascript.h"

extern LuaScript g_config;

Account IOAccountSQL::loadAccount(unsigned long accno){
	Account acc;
	std::string host = g_config.getGlobalString("sql_host");
	std::string user = g_config.getGlobalString("sql_user");
	std::string pass = g_config.getGlobalString("sql_pass");
	std::string db   = g_config.getGlobalString("sql_db");


	mysqlpp::Connection con;
	con.connect(db.c_str(), host.c_str(), user.c_str(), pass.c_str()); 
	try{
		mysqlpp::Query query = con.query();
		query << "SELECT * FROM accounts WHERE accno =" << accno;
		
		mysqlpp::Result res = query.store();

		if(res.num_rows() != 1){
			return acc;
		}

		mysqlpp::Row row = *res.begin();
		acc.accnumber = row.lookup_by_name("accno");
		acc.password = row.lookup_by_name("password");
		std::cout << "pass " << acc.password << "      acc " << acc.accnumber << std::endl;
		acc.accType = row.lookup_by_name("type");
		acc.premDays = row.lookup_by_name("premDays");

		query.reset();

		query << "SELECT name FROM players where account=" << accno;
		res = query.store();
		mysqlpp::Result::iterator i;
		for(i = res.begin(); i != res.end(); i++){
			std::string ss = (*i)[0];
			acc.charList.push_back(ss.c_str());
		}
					
	}
	catch(mysqlpp::BadQuery e){
		std::cout << "MYSQL-ERROR: " << e.error << std::endl;
		return acc;
	}
	
	return acc;
}

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

#include <string>
#include "database.h"

#ifdef __USE_MYSQL__
#include "databasemysql.h"
#endif
#ifdef __USE_SQLITE__
#include "databasesqlite.h"
#endif
#ifdef __USE_ODBC__
#include "databaseodbc.h"
#endif
#ifdef __USE_PGSQL__
#include "databasepgsql.h"
#endif

#if defined MULTI_SQL_DRIVERS
#include "configmanager.h"
extern ConfigManager g_config;
#endif

OTSYS_THREAD_LOCKVAR DBQuery::database_lock;

Database* _Database::_instance = NULL;

Database* _Database::instance(){
	if(!_instance){
#if defined MULTI_SQL_DRIVERS
#ifdef __USE_MYSQL__
		if(g_config.getString(ConfigManager::SQL_TYPE) == "mysql")
			_instance = new DatabaseMySQL;
#endif
#ifdef __USE_ODBC__
		if(g_config.getString(ConfigManager::SQL_TYPE) == "odbc")
			_instance = new DatabaseODBC;
#endif
#ifdef __USE_ODBC__
		if(g_config.getString(ConfigManager::SQL_TYPE) == "sqlite")
			_instance = new DatabaseSQLite;
#endif
#ifdef __USE_PGSQL__
		if(g_config.getString(ConfigManager::SQL_TYPE) == "pgsql")
			_instance = new DatabasePgSQL;
#endif
#else
		_instance = new Database;
#endif
		OTSYS_THREAD_LOCKVARINIT(DBQuery::database_lock);
	}
	return _instance;
}

DBQuery::DBQuery()
{
	OTSYS_THREAD_LOCK(database_lock, NULL);
}

DBQuery::~DBQuery()
{
	OTSYS_THREAD_UNLOCK(database_lock, NULL);
}

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

boost::recursive_mutex DBQuery::database_lock;

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
#ifdef __USE_SQLITE__
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
	}
	return _instance;
}

DBResult* _Database::verifyResult(DBResult* result)
{
	if(!result->next()){
		_instance->freeResult(result);
		return NULL;
	}
	else{
		return result;
	}
}

DBQuery::DBQuery()
{
	database_lock.lock();
}

DBQuery::~DBQuery()
{
	database_lock.unlock();
}

DBInsert::DBInsert(Database* db)
{
	m_db = db;
	m_rows = 0;

	// checks if current database engine supports multiline INSERTs
	m_multiLine = m_db->getParam(DBPARAM_MULTIINSERT) != 0;
}

void DBInsert::setQuery(const std::string& query)
{
	m_query = query;
	m_buf = "";
	m_rows = 0;
}

bool DBInsert::addRow(const std::string& row)
{
	if(m_multiLine){
		m_rows++;
		int32_t size = m_buf.length();

		// adds new row to buffer
		if(size == 0){
			m_buf = "(" + row + ")";
		}
		else if(size > 8192){
			if(!execute())
				return false;

			m_buf = "(" + row + ")";
		}
		else{
				m_buf += ",(" + row + ")";
		}

		return true;
	}
	else{
		// executes INSERT for current row
		return m_db->executeQuery(m_query + "(" + row + ")" );
	}
}

bool DBInsert::addRow(std::stringstream& row)
{
	bool ret = addRow(row.str());
	row.str("");
	return ret;
}

bool DBInsert::execute()
{
	if(m_multiLine && m_buf.length() > 0){
		if(m_rows == 0){
			//no rows to execute
			return true;
		}

		m_rows = 0;

		// executes buffer
		bool res = m_db->executeQuery(m_query + m_buf);
		m_buf = "";
		return res;
	}
	else{
		// INSERTs were executed on-fly
		return true;
	}
}

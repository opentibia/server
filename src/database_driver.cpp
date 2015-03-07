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

#include "database_driver.h"

#ifdef __USE_MYSQL__
#include "database_driver_mysql.h"
#endif
#ifdef __USE_SQLITE__
#include "database_driver_sqlite.h"
#endif
#ifdef __USE_ODBC__
#include "database_driver_odbc.h"
#endif
#ifdef __USE_PGSQL__
#include "database_driver_pgsql.h"
#endif

#include "configmanager.h"
extern ConfigManager g_config;

boost::recursive_mutex DBQuery::database_lock;

DatabaseDriver* DatabaseDriver::_instance = NULL;

DatabaseDriver* DatabaseDriver::instance(){
  if(!_instance){
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
  }
  return _instance;
}

bool DatabaseDriver::executeQuery(DBQuery &query)
{
  return internalQuery(query.str());
}

bool DatabaseDriver::executeQuery(const std::string &query)
{
  return internalQuery(query);
}

DBResult_ptr DatabaseDriver::storeQuery(const std::string &query)
{
  return internalSelectQuery(query);
}

DBResult_ptr DatabaseDriver::storeQuery(DBQuery &query)
{
  return storeQuery(query.str());
}

void DatabaseDriver::freeResult(DBResult *res)
{
  throw std::runtime_error("No database driver loaded, yet a DBResult was freed.");
}

DBResult_ptr DatabaseDriver::verifyResult(DBResult_ptr result)
{
  if(!result->advance()){
    return DBResult_ptr();
  }
  else{
    return result;
  }
}

// DBQuery

DBQuery::DBQuery()
{
  database_lock.lock();
}

DBQuery::~DBQuery()
{
  database_lock.unlock();
}

// DBInsert

DBInsert::DBInsert(DatabaseDriver* db)
{
  m_db = db;
  m_rows = 0;

  // checks if current database engine supports multi line INSERTs
  m_multiLine = m_db->getParam(DBPARAM_MULTIINSERT) != 0;
}

void DBInsert::setQuery(const std::string& query)
{
  m_query = query;
  m_buf.str("");
  m_rows = 0;
}

bool DBInsert::addRow(const std::string& row)
{
  if(m_multiLine){
    m_rows++;
    size_t size = m_buf.tellp();

    // adds new row to buffer
    if(size == 0){
      m_buf << "(" << row << ")";
    }
    else if(size > 8192){
      if(!execute())
        return false;
      
      m_buf << "(" << row << ")";
    }
    else{
        m_buf << ",(" + row + ")";
    }

    return true;
  }
  else{
    // executes INSERT for current row
    return m_db->executeQuery(m_query + "(" + row + ")" );
  }
}

bool DBInsert::addRowAndReset(std::ostringstream& row)
{
  bool ret = addRow(row.str());
  row.str("");
  return ret;
}

bool DBInsert::execute()
{
  if(m_multiLine && m_buf.tellp() > 0){
    if(m_rows == 0){
      //no rows to execute
      return true;
    }
    // executes buffer
    bool res = m_db->executeQuery(m_query + m_buf.str());

    // Reset counters
    m_rows = 0;
    m_buf.str("");
    return res;
  }
  else{
    // INSERTs were executed on-fly
    return true;
  }
}

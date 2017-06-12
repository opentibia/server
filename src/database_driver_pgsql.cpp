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

#ifdef __USE_PGSQL__

#include "database_driver.h"
#include "database_driver_pgsql.h"

#include "configmanager.h"
extern ConfigManager g_config;

/** DatabasePgSQL definitions */

DatabasePgSQL::DatabasePgSQL()
{
  // load connection parameters
  std::stringstream dns;
  dns << "host='" << g_config.getString(ConfigManager::SQL_HOST) << "' dbname='" << g_config.getString(ConfigManager::SQL_DB) << "' user='" << g_config.getString(ConfigManager::SQL_USER) << "' password='" << g_config.getString(ConfigManager::SQL_PASS) << "' port='" << g_config.getNumber(ConfigManager::SQL_PORT) << "'";

  m_handle = PQconnectdb(dns.str().c_str());
  m_connected = PQstatus(m_handle) == CONNECTION_OK;

  if(!m_connected)
    std::cout << "Failed to connected to PostgreSQL database: " << PQerrorMessage(m_handle) << std::endl;
}

DatabasePgSQL::~DatabasePgSQL()
{
  PQfinish(m_handle);
}

bool DatabasePgSQL::getParam(DBParam_t param)
{
  switch(param){
    case DBPARAM_MULTIINSERT:
      return true;
      break;

    default:
      return false;
  }
}

bool DatabasePgSQL::beginTransaction()
{
  return executeQuery("BEGIN");
}

bool DatabasePgSQL::rollback()
{
  return executeQuery("ROLLBACK");
}

bool DatabasePgSQL::commit()
{
  return executeQuery("COMMIT");
}

bool DatabasePgSQL::internalQuery(const std::string &query)
{
  if(!m_connected)
    return false;

  #ifdef __DEBUG_SQL__
  std::cout << "PGSQL QUERY: " << query << std::endl;
  #endif

  // executes query
  PGresult* res = PQexec(m_handle, _parse(query).c_str() );
  ExecStatusType stat = PQresultStatus(res);

  if(stat != PGRES_COMMAND_OK && stat != PGRES_TUPLES_OK){
    std::cout << "PQexec(): " << query << ": " << PQresultErrorMessage(res) << std::endl;
    PQclear(res);
    return false;
  }

  // everything went fine
  PQclear(res);
  return true;
}

DBResult_ptr DatabasePgSQL::internalSelectQuery(const std::string &query)
{
  if(!m_connected)
    return DBResult_ptr();

  #ifdef __DEBUG_SQL__
  std::cout << "PGSQL QUERY: " << query << std::endl;
  #endif

  // executes query
  PGresult* res = PQexec(m_handle, _parse(query).c_str() );
  ExecStatusType stat = PQresultStatus(res);

  if(stat != PGRES_COMMAND_OK && stat != PGRES_TUPLES_OK){
    std::cout << "PQexec(): " << query << ": " << PQresultErrorMessage(res) << std::endl;
    PQclear(res);
    return DBResult_ptr();
  }

  // everything went fine
  DBResult_ptr results(new PgSQLResult(res), boost::bind(&DatabaseDriver::freeResult, this, _1));
  return verifyResult(results);
}

uint64_t DatabasePgSQL::getLastInsertedRowID()
{
  if(!m_connected)
    return 0;

  PGresult* res = PQexec(m_handle, "SELECT LASTVAL() as last;");
  ExecStatusType stat = PQresultStatus(res);

  if(stat != PGRES_COMMAND_OK && stat != PGRES_TUPLES_OK){
    std::cout << "PQexec(): failed to fetch last row: " << PQresultErrorMessage(res) << std::endl;
    PQclear(res);
    return 0;
  }

  // everything went fine
  uint64_t id = atoll( PQgetvalue(res, 0, PQfnumber(res, "last" )));
  PQclear(res);
  return id;
}

std::string DatabasePgSQL::escapeString(const std::string &s)
{
  // remember to quote even empty string!
  if(!s.size())
    return std::string("''");

  // the worst case is 2n + 1
  int32_t error;
  char* output = new char[ s.length() * 2 + 1];

  // quotes escaped string and frees temporary buffer
  PQescapeStringConn(m_handle, output, s.c_str(), s.length(), reinterpret_cast<int*>(&error));
  std::string r = std::string("'");
  r += output;
  r += "'";
  delete[] output;
  return r;
}

std::string DatabasePgSQL::escapeBlob(const char* s, uint32_t length)
{
  // remember to quote even empty stream!
  if(!s)
    return std::string("''");

  // quotes escaped string and frees temporary buffer
  size_t len;
  char* output = (char*)PQescapeByteaConn(m_handle, (unsigned char*)s, length, &len);
  std::string r = std::string("E'");
  r += output;
  r += "'";
  PQfreemem(output);
  return r;
}

std::string DatabasePgSQL::_parse(const std::string &s)
{
  std::string query = "";

  bool inString = false;
  uint8_t ch;
  for(uint32_t a = 0; a < s.length(); a++){
    ch = s[a];

    if(ch == '\''){
      if(inString && s[a + 1] != '\'')
        inString = false;
      else
        inString = true;
    }

    if(ch == '`' && !inString)
      ch = '"';

    query += ch;
  }

  return query;
}

void DatabasePgSQL::freeResult(DBResult* res)
{
  delete (PgSQLResult*)res;
}

/** PgSQLResult definitions */

int32_t PgSQLResult::getDataInt(const std::string &s)
{
  return atoi( PQgetvalue(m_handle, m_cursor, PQfnumber(m_handle, s.c_str() ) ) );
}

uint32_t PgSQLResult::getDataUInt(const std::string &s)
{
  return (uint32_t) atoi( PQgetvalue(m_handle, m_cursor, PQfnumber(m_handle, s.c_str() ) ) );
}

int64_t PgSQLResult::getDataLong(const std::string &s)
{
  return atoll( PQgetvalue(m_handle, m_cursor, PQfnumber(m_handle, s.c_str() ) ) );
}

std::string PgSQLResult::getDataString(const std::string &s)
{
  return std::string( PQgetvalue(m_handle, m_cursor, PQfnumber(m_handle, s.c_str() ) ) );
}

const char* PgSQLResult::getDataStream(const std::string &s, unsigned long &size)
{
  std::string buf = PQgetvalue(m_handle, m_cursor, PQfnumber(m_handle, s.c_str() ) );
  unsigned char* temp = PQunescapeBytea( (const unsigned char*)buf.c_str(), (size_t*)&size);
  char* value = new char[buf.size()];
  strcpy(value, (char*)temp);
  PQfreemem(temp);
  return value;
}

DBResult_ptr PgSQLResult::advance()
{
  if(m_cursor >= m_rows)
    return DBResult_ptr();

  m_cursor++;
  return shared_from_this();
}

bool PgSQLResult::empty()
{
  return m_cursor >= m_rows;
}

PgSQLResult::PgSQLResult(PGresult* results)
{
  m_handle = results;
  m_cursor = -1;
  m_rows = PQntuples(m_handle) - 1;
}

PgSQLResult::~PgSQLResult()
{
  PQclear(m_handle);
}

#endif

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

#include <iostream>
#include "databasesqlite.h"

#define FAILED(_code) _code != SQLITE_OK

#include "configmanager.h"
extern ConfigManager g_config;

/** DatabaseSQLite definitions */

DatabaseSQLite::DatabaseSQLite()
{
	m_connected = false;

	// Initialize sqlite
	if( FAILED( sqlite3_open(g_config.getString(ConfigManager::SQL_DB).c_str(), &m_handle) ) ){
		std::cout << "Failed to initialize SQLite connection." << std::endl;
		sqlite3_close(m_handle);
	} else {
		m_connected = true;
	}
}

DatabaseSQLite::~DatabaseSQLite()
{
	sqlite3_close(m_handle);
}

bool DatabaseSQLite::beginTransaction()
{
	return executeQuery("BEGIN");
}

bool DatabaseSQLite::rollback()
{
	return executeQuery("ROLLBACK");
}

bool DatabaseSQLite::commit()
{
	return executeQuery("COMMIT");
}

std::string DatabaseSQLite::_parse(const std::string &s)
{
	std::string query = "";

	query.reserve(s.size());
	bool inString = false;
	uint8_t ch;
	for(int a = 0; a < s.length(); a++) {
		ch = s[a];

		if(ch == '\'') {
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

DBStatement* DatabaseSQLite::prepareStatement(const std::string &query)
{
	if(!m_connected)
		return NULL;

	#ifdef __SQL_QUERY_DEBUG__
	std::cout << "SQLITE PREPARED STATEMENT: " << query << std::endl;
	#endif

	std::string buf = _parse(query);
	sqlite3_stmt* stmt;
	// prepares statement
	if( FAILED( sqlite3_prepare_v2(m_handle, buf.c_str(), buf.length(), &stmt, NULL) ) ) {
		std::cout << "sqlite3_prepare_v2(): SQLITE ERROR: " << sqlite3_errmsg(m_handle) << std::endl;
		return NULL;
	}

	DBStatement* statement = new SQLiteStatement(stmt);
	return statement;
}

bool DatabaseSQLite::executeQuery(const std::string &query)
{
	if(!m_connected)
		return false;

	#ifdef __SQL_QUERY_DEBUG__
	std::cout << "SQLITE QUERY: " << query << std::endl;
	#endif

	std::string buf = _parse(query);
	sqlite3_stmt* stmt;
	// prepares statement
	if( FAILED( sqlite3_prepare_v2(m_handle, buf.c_str(), buf.length(), &stmt, NULL) ) ) {
		std::cout << "sqlite3_prepare_v2(): SQLITE ERROR: " << sqlite3_errmsg(m_handle) << std::endl;
		return false;
	}

	// executes it once
	int ret = sqlite3_step(stmt);
	if( FAILED(ret) && ret != SQLITE_DONE && ret != SQLITE_ROW) {
		std::cout << "sqlite3_step(): SQLITE ERROR: " << sqlite3_errmsg(m_handle) << std::endl;
		return false;
	}

	// closes statement
	// at all not sure if it should be debugged - query was executed correctly...
	sqlite3_finalize(stmt);

	return true;
}

DBResult* DatabaseSQLite::storeQuery(const std::string &query)
{
	if(!m_connected)
		return NULL;

	#ifdef __SQL_QUERY_DEBUG__
	std::cout << "SQLITE QUERY: " << query << std::endl;
	#endif

	std::string buf = _parse(query);
	sqlite3_stmt* stmt;
	// prepares statement
	if( FAILED( sqlite3_prepare_v2(m_handle, buf.c_str(), buf.length(), &stmt, NULL) ) ) {
		std::cout << "sqlite3_prepare_v2(): SQLITE ERROR: " << sqlite3_errmsg(m_handle) << std::endl;
		return NULL;
	}

	DBResult* results = new SQLiteResult(stmt);
	return results;
}

std::string DatabaseSQLite::escapeString(const std::string &s)
{
	// remember about quoiting even an empty string!
	if(!s.size())
		return std::string("''");

	// the worst case is 2n + 1
	char* output = new char[ s.length() * 2 + 3];

	// quotes escaped string and frees temporary buffer
	sqlite3_snprintf( s.length() * 2 + 1, output, "'%q'", s.c_str() );
	std::string r(output);
	delete[] output;
	return r;
}

void DatabaseSQLite::freeStatement(DBStatement* stmt)
{
	delete (SQLiteStatement*)stmt;
}

void DatabaseSQLite::freeResult(DBResult* res)
{
	delete (SQLiteResult*)res;
}

/** SQLiteStatement definitions */

void SQLiteStatement::setInt(int32_t param, int32_t value)
{
	if( FAILED( sqlite3_bind_int(m_handle, param, value) ) )
		std::cout << "DBStatement::setInt(): SQLITE ERROR." << std::endl;
}

void SQLiteStatement::setLong(int32_t param, int64_t value)
{
	if( FAILED( sqlite3_bind_int64(m_handle, param, value) ) )
		std::cout << "DBStatement::setLong(): SQLITE ERROR." << std::endl;
}

void SQLiteStatement::setString(int32_t param, const std::string &value)
{
	if( FAILED( sqlite3_bind_text(m_handle, param, value.c_str(), value.size(), SQLITE_STATIC) ) )
		std::cout << "DBStatement::setString(): SQLITE ERROR." << std::endl;
}

void SQLiteStatement::bindStream(int32_t param, const char* value, unsigned long size)
{
	if( FAILED( sqlite3_bind_blob(m_handle, param, value, size, SQLITE_STATIC) ) )
		std::cout << "DBStatement::bindStream(): SQLITE ERROR." << std::endl;
}

bool SQLiteStatement::execute()
{
	// executes the query
	int ret = sqlite3_step(m_handle);
	if( FAILED(ret) && ret != SQLITE_DONE && ret != SQLITE_ROW) {
		std::cout << "sqlite3_step(): SQLITE ERROR." << std::endl;

		return false;
	} else {
		// resets query for next use
		sqlite3_reset(m_handle);
		// resets bindings
		sqlite3_clear_bindings(m_handle);

		return true;
	}
}

SQLiteStatement::SQLiteStatement(sqlite3_stmt* stmt)
{
	m_handle = stmt;
}

SQLiteStatement::~SQLiteStatement()
{
	sqlite3_finalize(m_handle);
}

/** SQLiteResult definitions */

int32_t SQLiteResult::getDataInt(const std::string &s)
{
	listNames_t::iterator it = m_listNames.find(s);
	if(it != m_listNames.end() )
		return sqlite3_column_int(m_handle, it->second);

	std::cout << "Error during getDataInt(" << s << ")." << std::endl;
	return 0; // Failed
}

int64_t SQLiteResult::getDataLong(const std::string &s)
{
	listNames_t::iterator it = m_listNames.find(s);
	if(it != m_listNames.end() )
		return sqlite3_column_int64(m_handle, it->second);

	std::cout << "Error during getDataLong(" << s << ")." << std::endl;
	return 0; // Failed
}

std::string SQLiteResult::getDataString(const std::string &s)
{
	listNames_t::iterator it = m_listNames.find(s);
	if(it != m_listNames.end() ) {
		std::string value = (const char*)sqlite3_column_text(m_handle, it->second);
		return value;
	}

	std::cout << "Error during getDataString(" << s << ")." << std::endl;
	return std::string(""); // Failed
}

const char* SQLiteResult::getDataStream(const std::string &s, unsigned long &size)
{
	listNames_t::iterator it = m_listNames.find(s);
	if(it != m_listNames.end() ) {
		const char* value = (const char*)sqlite3_column_blob(m_handle, it->second);
		size = sqlite3_column_bytes(m_handle, it->second);
		return value;
	}

	std::cout << "Error during getDataStream(" << s << ")." << std::endl;
	return NULL; // Failed
}

bool SQLiteResult::next()
{
	// checks if after moving to next step we have a row result
	return sqlite3_step(m_handle) == SQLITE_ROW;
}

SQLiteResult::SQLiteResult(sqlite3_stmt* stmt)
{
	m_handle = stmt;
	m_listNames.clear();

	int32_t fields = sqlite3_column_count(m_handle);
	for( int32_t i = 0; i < fields; i++)
		m_listNames[ sqlite3_column_name(m_handle, i) ] = i;
}

SQLiteResult::~SQLiteResult()
{
	sqlite3_finalize(m_handle);
}

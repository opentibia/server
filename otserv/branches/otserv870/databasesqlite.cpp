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

#ifdef __USE_SQLITE__

#include "database.h"
#include "databasesqlite.h"
#include "configmanager.h"
#include <iostream>
#include <fstream>

extern ConfigManager g_config;

#if SQLITE_VERSION_NUMBER < 3003009
#define OTS_SQLITE3_PREPARE sqlite3_prepare
#else
#define OTS_SQLITE3_PREPARE sqlite3_prepare_v2
#endif

/** DatabaseSQLite definitions */

DatabaseSQLite::DatabaseSQLite()
{
	m_connected = false;

	// test for existence of database file;
	// sqlite3_open will create a new one if it isn't there (which we don't want)
	std::fstream fin(g_config.getString(ConfigManager::SQL_DB).c_str(), std::ios::in | std::ios::binary);
	if(fin.fail()){
		std::cout << "Failed to initialize SQLite connection. File " << g_config.getString(ConfigManager::SQL_DB) <<
				" does not exist." << std::endl;
		return;
	}
	fin.close();

	// Initialize sqlite
	if( sqlite3_open(g_config.getString(ConfigManager::SQL_DB).c_str(), &m_handle) != SQLITE_OK){
		std::cout << "Failed to initialize SQLite connection." << std::endl;
		sqlite3_close(m_handle);
	}
	else{
		m_connected = true;
	}
}

DatabaseSQLite::~DatabaseSQLite()
{
	sqlite3_close(m_handle);
}

bool DatabaseSQLite::getParam(DBParam_t param)
{
	switch(param){
		case DBPARAM_MULTIINSERT:
			return false;
			break;
		default:
			return false;
	}
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
	for(uint32_t a = 0; a < s.length(); ++a){
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

bool DatabaseSQLite::executeQuery(const std::string &query)
{
	boost::recursive_mutex::scoped_lock lockClass(sqliteLock);

	if(!m_connected)
		return false;

	#ifdef __DEBUG_SQL__
	std::cout << "SQLITE QUERY: " << query << std::endl;
	#endif

	std::string buf = _parse(query);
	sqlite3_stmt* stmt;
	// prepares statement
	if( OTS_SQLITE3_PREPARE(m_handle, buf.c_str(), buf.length(), &stmt, NULL) != SQLITE_OK){
		sqlite3_finalize(stmt);
		std::cout << "OTS_SQLITE3_PREPARE(): SQLITE ERROR: " << sqlite3_errmsg(m_handle) << " (" << buf << ")" << std::endl;
		return false;
	}

	// executes it once
	int ret = sqlite3_step(stmt);
	if(ret != SQLITE_OK && ret != SQLITE_DONE && ret != SQLITE_ROW){
		sqlite3_finalize(stmt);
		std::cout << "sqlite3_step(): SQLITE ERROR: " << sqlite3_errmsg(m_handle) << " (" << buf << ")" << std::endl;
		return false;
	}

	// closes statement
	// at all not sure if it should be debugged - query was executed correctly...
	sqlite3_finalize(stmt);

	return true;
}

DBResult* DatabaseSQLite::storeQuery(const std::string &query)
{
	boost::recursive_mutex::scoped_lock lockClass(sqliteLock);

	if(!m_connected)
		return NULL;

	#ifdef __DEBUG_SQL__
	std::cout << "SQLITE QUERY: " << query << std::endl;
	#endif

	std::string buf = _parse(query);
	sqlite3_stmt* stmt;
	// prepares statement
	if( OTS_SQLITE3_PREPARE(m_handle, buf.c_str(), buf.length(), &stmt, NULL) != SQLITE_OK){
		sqlite3_finalize(stmt);
		std::cout << "OTS_SQLITE3_PREPARE(): SQLITE ERROR: " << sqlite3_errmsg(m_handle)  << " (" << buf << ")" << std::endl;
		return NULL;
	}

	DBResult* results = new SQLiteResult(stmt);
	return verifyResult(results);
}

uint64_t DatabaseSQLite::getLastInsertedRowID()
{
	return (uint64_t)sqlite3_last_insert_rowid(m_handle);
}

std::string DatabaseSQLite::escapeString(const std::string &s)
{
	// remember about quoiting even an empty string!
	if(!s.size())
		return std::string("''");

	// the worst case is 2n + 1
	char* output = new char[s.length() * 2 + 3];

	// quotes escaped string and frees temporary buffer
	sqlite3_snprintf(s.length() * 2 + 3, output, "%Q", s.c_str() );
	std::string r(output);
	delete[] output;
	return r;
}

std::string DatabaseSQLite::escapeBlob(const char* s, uint32_t length)
{
	std::string buf = "x'";

	char* hex = new char[2 + 1]; //need one extra byte for null-character

	for(uint32_t i = 0; i < length; ++i){
		sprintf(hex, "%02x", ((unsigned char)s[i]));
		buf += hex;
	}

	delete[] hex;

	buf += "'";
	return buf;
}

void DatabaseSQLite::freeResult(DBResult* res)
{
	delete (SQLiteResult*)res;
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
	if(it != m_listNames.end() ){
		std::string value = (const char*)sqlite3_column_text(m_handle, it->second);
		return value;
	}

	std::cout << "Error during getDataString(" << s << ")." << std::endl;
	return std::string(""); // Failed
}

const char* SQLiteResult::getDataStream(const std::string &s, unsigned long &size)
{
	listNames_t::iterator it = m_listNames.find(s);
	if(it != m_listNames.end() ){
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
	for(int32_t i = 0; i < fields; ++i){
		m_listNames[ sqlite3_column_name(m_handle, i) ] = i;
	}
}

SQLiteResult::~SQLiteResult()
{
	sqlite3_finalize(m_handle);
}

#endif

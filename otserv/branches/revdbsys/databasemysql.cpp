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
#include "databasemysql.h"
#ifdef __MYSQL_ALT_INCLUDE__
#include "errmsg.h"
#else
#include <mysql/errmsg.h>
#endif

#include "configmanager.h"
extern ConfigManager g_config;

/** DatabaseMySQL definitions */

DatabaseMySQL::DatabaseMySQL()
{
	m_connected = false;

	// connection handle initialization
	if( !mysql_init(&m_handle) ) {
		std::cout << "Failed to initialize MySQL connection handle." << std::endl;
		return;
	}

	// automatic reconnect
	my_bool reconnect = true;
	mysql_options(&m_handle, MYSQL_OPT_RECONNECT, &reconnect);

	// connects to database
	if( !mysql_real_connect(&m_handle, g_config.getString(ConfigManager::SQL_HOST).c_str(), g_config.getString(ConfigManager::SQL_USER).c_str(), g_config.getString(ConfigManager::SQL_PASS).c_str(), g_config.getString(ConfigManager::SQL_DB).c_str(), g_config.getNumber(ConfigManager::SQL_PORT), NULL, 0) ) {
		std::cout << "Failed to connect to database. MYSQL ERROR: " << mysql_error(&m_handle) << std::endl;
		return;
	}

	m_connected = true;
}

DatabaseMySQL::~DatabaseMySQL()
{
	mysql_close(&m_handle);
}

int DatabaseMySQL::getParam(DBParam_t param)
{
	switch(param) {
		case DBPARAM_MULTIINSERT:
			return true;
			break;
	}
}

bool DatabaseMySQL::beginTransaction()
{
	return executeQuery("BEGIN");
}

bool DatabaseMySQL::rollback()
{
	if(!m_connected)
		return false;

	#ifdef __SQL_QUERY_DEBUG__
	std::cout << "ROLLBACK" << std::endl;
	#endif

	if( mysql_rollback(&m_handle) != 0) {
		std::cout << "mysql_rollback(): MYSQL ERROR: " << mysql_error(&m_handle) << std::endl;
		return false;
	}

	return true;
}

bool DatabaseMySQL::commit()
{
	if(!m_connected)
		return false;

	#ifdef __SQL_QUERY_DEBUG__
	std::cout << "COMMIT" << std::endl;
	#endif
	if( mysql_commit(&m_handle) != 0) {
		std::cout << "mysql_commit(): MYSQL ERROR: " << mysql_error(&m_handle) << std::endl;
		return false;
	}

	return true;
}

bool DatabaseMySQL::executeQuery(const std::string &query)
{
	if(!m_connected)
		return false;

	#ifdef __SQL_QUERY_DEBUG__
	std::cout << "MYSQL QUERY: " << query << std::endl;
	#endif

	bool state = true;

	// executes the query
	if( mysql_real_query(&m_handle, query.c_str(), query.length() ) != 0) {
		std::cout << "mysql_real_query(): " << query << ": MYSQL ERROR: " << mysql_error(&m_handle) << std::endl;
		int error = mysql_errno(&m_handle);

		if(error == CR_SERVER_LOST || error == CR_SERVER_GONE_ERROR)
			m_connected = false;

		state = false;
	}

	// we should call that every time as someone would call executeQuery('SELECT...')
	// as it is described in MySQL manual: "it doesn't hurt" :P
	MYSQL_RES* m_res = mysql_store_result(&m_handle);

	if(m_res)
		mysql_free_result(m_res);

	return state;
}

DBResult* DatabaseMySQL::storeQuery(const std::string &query)
{
	if(!m_connected)
		return NULL;

	#ifdef __SQL_QUERY_DEBUG__
	std::cout << "MYSQL QUERY: " << query << std::endl;
	#endif

	bool state = true;

	// executes the query
	if( mysql_real_query(&m_handle, query.c_str(), query.length() ) != 0)
	{
		std::cout << "mysql_real_query(): " << query << ": MYSQL ERROR: " << mysql_error(&m_handle) << std::endl;
		int error = mysql_errno(&m_handle);

		if(error == CR_SERVER_LOST || error == CR_SERVER_GONE_ERROR)
			m_connected = false;

		state = false;
	}

	// we should call that every time as someone would call executeQuery('SELECT...')
	// as it is described in MySQL manual: "it doesn't hurt" :P
	MYSQL_RES* m_res = mysql_store_result(&m_handle);

	// error occured
	if(!m_res) {
		std::cout << "mysql_store_result(): " << query << ": MYSQL ERROR: " << mysql_error(&m_handle) << std::endl;
		int error = mysql_errno(&m_handle);

		if(error == CR_SERVER_LOST || error == CR_SERVER_GONE_ERROR)
			m_connected = false;

		return NULL;
	}

	// retriving results of query
	DBResult* res = new MySQLResult(m_res);
        return res;
}

std::string DatabaseMySQL::escapeString(const std::string &s)
{
	return escapeBlob( s.c_str(), s.length() );
}

std::string DatabaseMySQL::escapeBlob(const char* s, uint32_t length)
{
	// remember about quoiting even an empty string!
	if(!s)
		return std::string("''");

	// the worst case is 2n + 1
	char* output = new char[length * 2 + 1];

	// quotes escaped string and frees temporary buffer
	mysql_real_escape_string(&m_handle, output, s, length);
	std::string r = "'";
	r += output;
	r += "'";
	delete[] output;
	return r;
}

void DatabaseMySQL::freeResult(DBResult* res)
{
	delete (MySQLResult*)res;
}

/** MySQLResult definitions */

int32_t MySQLResult::getDataInt(const std::string &s)
{
	listNames_t::iterator it = m_listNames.find(s);
	if(it != m_listNames.end() ) {
		if(m_row[it->second] == NULL)
			return 0;
		else
			return atoi(m_row[it->second]);
	}

	std::cout << "Error during getDataInt(" << s << ")." << std::endl;
	return 0; // Failed
}

int64_t MySQLResult::getDataLong(const std::string &s)
{
	listNames_t::iterator it = m_listNames.find(s);
	if(it != m_listNames.end() ) {
		if(m_row[it->second] == NULL)
			return 0;
		else
			return ATOI64(m_row[it->second]);
	}

	std::cout << "Error during getDataLong(" << s << ")." << std::endl;
	return 0; // Failed
}

std::string MySQLResult::getDataString(const std::string &s)
{
	listNames_t::iterator it = m_listNames.find(s);
	if(it != m_listNames.end() ) {
		if(m_row[it->second] == NULL)
			return std::string("");
		else
			return std::string(m_row[it->second]);
	}

	std::cout << "Error during getDataString(" << s << ")." << std::endl;
	return std::string(""); // Failed
}

const char* MySQLResult::getDataStream(const std::string &s, unsigned long &size)
{
	listNames_t::iterator it = m_listNames.find(s);
	if(it != m_listNames.end() ) {
		if(m_row[it->second] == NULL) {
			size = 0;
			return NULL;
		} else {
			size = mysql_fetch_lengths(m_handle)[it->second];
			return m_row[it->second];
		}
	}

	std::cout << "Error during getDataStream(" << s << ")." << std::endl;
	size = 0;
	return NULL;
}

bool MySQLResult::next()
{
	m_row = mysql_fetch_row(m_handle);
	return m_row != NULL;
}

MySQLResult::MySQLResult(MYSQL_RES* res)
{
	m_handle = res;
	m_listNames.clear();

	MYSQL_FIELD* field;
	int32_t i = 0;
	while( field = mysql_fetch_field(m_handle) ) {
		m_listNames[field->name] = i;
		i++;
	}
}

MySQLResult::~MySQLResult()
{
	mysql_free_result(m_handle);
}

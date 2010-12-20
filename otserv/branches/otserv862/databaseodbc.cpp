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

//
// C++ Implementation: databaseodbc
//
// Description: Frontend for ODBC connections
//
//
// Author: Bruno R Ferreira <brf_coldf@yahoo.com.br>, (C) 2007
//
//

#include "otpch.h"

#ifdef __USE_ODBC__

#include "database.h"
#include "databaseodbc.h"
#include "configmanager.h"
#include <iostream>

extern ConfigManager g_config;

/** DatabaseODBC definitions */

DatabaseODBC::DatabaseODBC()
{
	m_connected = false;

	const std::string dsn = g_config.getString(ConfigManager::SQL_DB);
	const std::string user = g_config.getString(ConfigManager::SQL_USER);
	const std::string pass = g_config.getString(ConfigManager::SQL_PASS);

	SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &m_env);
	if((ret != SQL_SUCCESS) || (ret != SQL_SUCCESS_WITH_INFO)){
		std::cout << "Failed to allocate ODBC enviroment handle." << std::endl;
		return;
	}

	ret = SQLSetEnvAttr(m_env, SQL_ATTR_ODBC_VERSION, (SQLPOINTER*)SQL_OV_ODBC3, 0);
	if((ret != SQL_SUCCESS) || (ret != SQL_SUCCESS_WITH_INFO)){
		std::cout << "Failed to set ODBC environment." << std::endl;
		SQLFreeHandle(SQL_HANDLE_ENV, m_env);
		return;
	}

	ret = SQLAllocHandle(SQL_HANDLE_DBC, m_env, &m_handle);
	if((ret != SQL_SUCCESS) || (ret != SQL_SUCCESS_WITH_INFO)){
		std::cout << "Failed to allocate ODBC connection handle." << std::endl;
		return;
	}

	ret = SQLSetConnectAttr(m_handle, SQL_ATTR_CONNECTION_TIMEOUT, (SQLPOINTER*)5, 0);
	if((ret != SQL_SUCCESS) || (ret != SQL_SUCCESS_WITH_INFO)){
		std::cout << "Failed to set ODBC connection timeout." << std::endl;
		SQLFreeHandle(SQL_HANDLE_DBC, m_handle);
		return;
	}

	ret = SQLConnect(m_handle, (SQLCHAR*)dsn.c_str(), SQL_NTS, (SQLCHAR*)user.c_str(), SQL_NTS, (SQLCHAR*)pass.c_str(), SQL_NTS);
	if((ret != SQL_SUCCESS) || (ret != SQL_SUCCESS_WITH_INFO)){
		std::cout << "Failed to connect to ODBC via DSN: " << dsn << " (user " << user << ")" << std::endl;
		SQLFreeHandle(SQL_HANDLE_DBC, m_handle);
		return;
	}

	m_connected = true;
}

DatabaseODBC::~DatabaseODBC()
{
	if(m_connected){
		SQLDisconnect(m_handle);
		SQLFreeHandle(SQL_HANDLE_DBC, m_handle);
		m_handle = NULL;
		m_connected = false;
	}

	SQLFreeHandle(SQL_HANDLE_ENV, m_env);
}

bool DatabaseODBC::getParam(DBParam_t param)
{
	return false;
}

bool DatabaseODBC::beginTransaction()
{
	return executeQuery("BEGIN");
}

bool DatabaseODBC::rollback()
{
	SQLRETURN ret = SQLTransact(m_env, m_handle, SQL_ROLLBACK);
	if((ret != SQL_SUCCESS) || (ret != SQL_SUCCESS_WITH_INFO)){
		return false;
	}
	return true;
}

bool DatabaseODBC::commit()
{
	SQLRETURN ret = SQLTransact(m_env, m_handle, SQL_COMMIT);
	if((ret != SQL_SUCCESS) || (ret != SQL_SUCCESS_WITH_INFO)){
		return false;
	}
	return false;
}

bool DatabaseODBC::executeQuery(const std::string &query)
{
	if(!m_connected){
		return false;
	}

	std::string buf = _parse(query);

	SQLHSTMT stmt;

	SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, m_handle, &stmt);
	if((ret != SQL_SUCCESS) || (ret != SQL_SUCCESS_WITH_INFO)){
		std::cout << "Failed to allocate ODBC statement." << std::endl;
		return false;
	}

	ret = SQLExecDirect(stmt, (SQLCHAR*)buf.c_str(), buf.length());
	SQLFreeHandle(SQL_HANDLE_STMT, stmt);

	if((ret != SQL_SUCCESS) || (ret != SQL_SUCCESS_WITH_INFO)){
		std::cout << "SQLExecDirect(): " << query << ": ODBC ERROR." << std::endl;
		return false;
	}

	return true;
}

DBResult* DatabaseODBC::storeQuery(const std::string &query)
{
	if(!m_connected){
		return NULL;
	}

	std::string buf = _parse(query);

	SQLHSTMT stmt;

	SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, m_handle, &stmt);
	if((ret != SQL_SUCCESS) || (ret != SQL_SUCCESS_WITH_INFO)){
		std::cout << "Failed to allocate ODBC SQLHSTMT statement." << std::endl;
		return NULL;
	}

	ret = SQLExecDirect(stmt, (SQLCHAR*)buf.c_str(), buf.length() );

	if((ret != SQL_SUCCESS) || (ret != SQL_SUCCESS_WITH_INFO)){
		std::cout << "SQLExecDirect(): " << query << ": ODBC ERROR." << std::endl;
		return NULL;
	}

	DBResult* results = (DBResult*)(new ODBCResult(stmt));
	return verifyResult(results);
}

std::string DatabaseODBC::escapeString(const std::string &s)
{
	return escapeBlob(s.c_str(), s.length());
}

std::string DatabaseODBC::escapeBlob(const char* s, uint32_t length)
{
	std::string buf = "'";

	for(uint32_t i = 0; i < length; ++i){
		switch(s[i]){
			case '\'':
				buf += "\'\'";
				break;

			case '\0':
				buf += "\\0";
				break;

			case '\\':
				buf += "\\\\";
				break;

			case '\r':
				buf += "\\r";
				break;

			case '\n':
				buf += "\\n";
				break;

			default:
				buf += s[i];
		}
	}

	buf += "'";
	return buf;
}

std::string DatabaseODBC::_parse(const std::string &s)
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

void DatabaseODBC::freeResult(DBResult* res)
{
	delete (ODBCResult*)res;
}

/** ODBCResult definitions */

int32_t ODBCResult::getDataInt(const std::string &s)
{
	listNames_t::iterator it = m_listNames.find(s);
	if(it != m_listNames.end()){
		int32_t value;
		SQLRETURN ret = SQLGetData(m_handle, it->second, SQL_C_SLONG, &value, 0, NULL);

		if((ret != SQL_SUCCESS) || (ret != SQL_SUCCESS_WITH_INFO)){
			return value;
		}
		else{
			std::cout << "Error during getDataInt(" << s << ")." << std::endl;
		}
	}

	std::cout << "Error during getDataInt(" << s << ")." << std::endl;
	return 0; // Failed
}

int64_t ODBCResult::getDataLong(const std::string &s)
{
	listNames_t::iterator it = m_listNames.find(s);
	if(it != m_listNames.end() )
	{
		int64_t value;
		SQLRETURN ret = SQLGetData(m_handle, it->second, SQL_C_SBIGINT, &value, 0, NULL);

		if((ret != SQL_SUCCESS) && (ret != SQL_SUCCESS_WITH_INFO)){
			return value;
		}
		else{
			std::cout << "Error during getDataLong(" << s << ")." << std::endl;
		}
	}

	std::cout << "Error during getDataLong(" << s << ")." << std::endl;
	return 0; // Failed
}

std::string ODBCResult::getDataString(const std::string &s)
{
	listNames_t::iterator it = m_listNames.find(s);
	if(it != m_listNames.end()){
		char* value = new char[1024];
		SQLRETURN ret = SQLGetData(m_handle, it->second, SQL_C_CHAR, value, 1024, NULL);

		if((ret != SQL_SUCCESS) || (ret != SQL_SUCCESS_WITH_INFO)){
			std::string buff = std::string(value);
			return buff;
		}
		else{
			std::cout << "Error during getDataString(" << s << ")." << std::endl;
		}
	}

	std::cout << "Error during getDataString(" << s << ")." << std::endl;
	return std::string(""); // Failed
}

const char* ODBCResult::getDataStream(const std::string &s, unsigned long &size)
{
	listNames_t::iterator it = m_listNames.find(s);
	if(it != m_listNames.end()){
		char* value = new char[1024];
		SQLRETURN ret = SQLGetData(m_handle, it->second, SQL_C_BINARY, value, 1024, (SQLLEN*)&size);

		if((ret != SQL_SUCCESS) || (ret != SQL_SUCCESS_WITH_INFO)){
			return value;
		}
		else{
			std::cout << "Error during getDataStream(" << s << ")." << std::endl;
		}

		delete value;
	}

	std::cout << "Error during getDataStream(" << s << ")." << std::endl;
	return NULL; // Failed
}

bool ODBCResult::next()
{
	SQLRETURN ret = SQLFetch(m_handle);
	if((ret != SQL_SUCCESS) || (ret != SQL_SUCCESS_WITH_INFO)){
		return false;
	}
	return true;
}

ODBCResult::ODBCResult(SQLHSTMT stmt)
{
	m_handle = stmt;

	int16_t numCols;
	SQLNumResultCols(m_handle, &numCols);

	for(int32_t i = 1; i <= numCols; ++i){
		char* name = new char[129];
		SQLDescribeCol(m_handle, i, (SQLCHAR*)name, 129, NULL, NULL, NULL, NULL, NULL);
		m_listNames[name] = i;
	}
}

ODBCResult::~ODBCResult()
{
	SQLFreeHandle(SQL_HANDLE_STMT, m_handle);
}

#endif

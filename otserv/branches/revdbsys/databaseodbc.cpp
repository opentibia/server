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
// Author: bruno <brfwolf@thormenta.com>, (C) 2007
//
//

#include <iostream>

#include "tools.h"
#include "databaseodbc.h"
#include "configmanager.h"

#ifndef SQL_OK
#define SQL_OK(a) (a == SQL_SUCCESS || a == SQL_SUCCESS_WITH_INFO)
#endif

extern ConfigManager g_config;

DatabaseODBC::DatabaseODBC()
{
	hConnected = false;

	hEnv = NULL;
	hDbc = NULL;

	szDSN  = new uint8_t[SQL_MAX_DSN_LENGTH];
	szPass = new uint8_t[32];
	szUser = new uint8_t[32];

	strcpy((char*)szDSN,  g_config.getString(ConfigManager::SQL_DB).c_str());
	strcpy((char*)szPass, g_config.getString(ConfigManager::SQL_PASS).c_str());
	strcpy((char*)szUser, g_config.getString(ConfigManager::SQL_USER).c_str());

	SQLRETURN ret = SQLAllocEnv(&hEnv);
	if (!RETURN_SUCCESS(ret))
	{
		std::cout << "Failed to allocate SQLHENV." << std::endl;
		displayError(NULL);
		hEnv = NULL;
		return;
	}

	ret = SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER*)SQL_OV_ODBC3, 0);
	if (!RETURN_SUCCESS(ret))
	{
		std::cout << "Failed to set SQLHENV attribute SQL_ATTR_ODBC_VERSION = SQL_OV_ODBC3." << std::endl;
		displayError(NULL);
		SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
		hEnv = NULL;
	}
}


DatabaseODBC::~DatabaseODBC()
{
	disconnect();
	SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
}


bool DatabaseODBC::connect()
{
	if (hConnected == true)
		return true;

	if (hEnv == NULL)
	{
		std::cout << "SQLHENV not initialized." << std::endl;
		return false;
	}

	SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc);
	if (!RETURN_SUCCESS(ret))
	{
		std::cout << "Failed to allocate SQLHDBC." << std::endl;
		hDbc = NULL;
		return false;
	}

	ret = SQLSetConnectAttr(hDbc, SQL_ATTR_CONNECTION_TIMEOUT, (SQLPOINTER*)5, 0);
	if (!RETURN_SUCCESS(ret))
	{
		std::cout << "Failed to set SQLHDBC attribute SQL_ATTR_CONNECTION_TIMEOUT = 5." << std::endl;
		displayError(NULL);
		SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
		hDbc = NULL;
		return false;
	}

	ret = SQLConnect(hDbc, (SQLCHAR*)szDSN, SQL_NTS, (SQLCHAR*)szUser, SQL_NTS, (SQLCHAR*)szPass, SQL_NTS);
	if (!RETURN_SUCCESS(ret)) 
	{
		std::cout << "Failed to connect to ODBC via DSN: " << szDSN << " (user " << szUser << "; pwd " << szPass << ")" << std::endl;
		displayError(NULL);
		SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
		hDbc = NULL;
		return false;
	}

	hConnected = true;
	return true;
}

bool DatabaseODBC::disconnect()
{
	if(hConnected) {
		SQLDisconnect(hDbc);
		SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
		hDbc = NULL;
		hConnected = false;
	}
	return true;
}

void DatabaseODBC::displayError(SQLHSTMT hStmt)
{
	SQLCHAR sqlState[6];
	SQLINTEGER errorCode;
	SQLCHAR errorMsg[512];
	SQLSMALLINT errorMsgSize;
	SQLError(hEnv, hDbc, hStmt, sqlState, &errorCode, errorMsg, 512, &errorMsgSize);
	std::cout << "SQL Error #" << errorCode << ": " << errorMsg << std::endl << "Statement: " << sqlState << std::endl;
}

SQLCHAR* DatabaseODBC::prepareQuery(DBQuery& query)
{
	std::string tmp( query.str() );
	replaceString(tmp, "`", "\"");

	#ifdef __SQL_QUERY_DEBUG__
	std::cout << tmp << std::endl;
	#endif

	SQLCHAR *szQueryStr = new SQLCHAR[tmp.length()];
	strcpy((char*)szQueryStr, tmp.c_str());
	query.reset();

	return szQueryStr;
}

bool DatabaseODBC::executeQuery(DBQuery& q)
{
	if (!hConnected)
		return false;

	SQLHSTMT hStmt;
	
	SQLRETURN ret = SQLAllocStmt(hDbc, &hStmt);
	if (!RETURN_SUCCESS(ret))
	{
		std::cout << "Failed to allocate SQLHSTMT." << std::endl;
		displayError(hStmt);
		return false;
	}

	SQLCHAR *szQueryStr = prepareQuery(q);

	ret = SQLExecDirect(hStmt, (SQLCHAR*)szQueryStr, SQL_NTS);

	if (!RETURN_SUCCESS(ret))
	{
		std::cout << "Failed to execute query: \"" << szQueryStr << "\"." << std::endl;
		displayError(hStmt);
		return false;
	}

	return true;
}

bool DatabaseODBC::storeQuery(DBQuery& q, DBResult& res)
{
	if (!hConnected)
		return false;

	SQLHSTMT hStmt;
	
	SQLRETURN ret = SQLAllocStmt(hDbc, &hStmt);
	if (!RETURN_SUCCESS(ret))
	{
		std::cout << "Failed to allocate SQLHSTMT." << std::endl;
		displayError(hStmt);
		return false;
	}

	SQLCHAR *szQueryStr = prepareQuery(q);

	ret = SQLExecDirect(hStmt, (SQLCHAR*)szQueryStr, SQL_NTS);
	if (!RETURN_SUCCESS(ret))
	{
		std::cout << "Failed to execute query: \"" << szQueryStr << "\"." << std::endl;
		displayError(hStmt);
		SQLFreeStmt (hStmt, SQL_DROP); 
		return false;
	}

	SQLSMALLINT numCols;
	ret = SQLNumResultCols(hStmt, &numCols);
	if (!RETURN_SUCCESS(ret))
	{
		std::cout << "Failed to retrieve columns count for query \"" << szQueryStr << "\"." << std::endl;
		displayError(hStmt);
		SQLFreeStmt (hStmt, SQL_DROP); 
		return false;
	}

	res.clear();

	SQLCHAR **resultrow = new SQLCHAR*[numCols];
	bool *described = new bool[numCols];
	for (int i = 0; i < numCols; i++)
		described[i] = false;

	while (true)
	{
		ret = SQLFetch(hStmt);
		if (!RETURN_SUCCESS(ret))
			break;
		for (SQLUSMALLINT k = 1; k <= (SQLUSMALLINT)numCols; k++)
		{
			if (!described[k-1])
			{
				SQLCHAR columnName[129];
				SQLDescribeCol(hStmt, k, (SQLCHAR*)columnName, 129, NULL, NULL, NULL, NULL, NULL);
				std::string name((char*)columnName);
				res.setFieldName(name, (unsigned int)k-1);
				described[k-1] = true;
			}

			resultrow[k-1] = (SQLCHAR*)malloc(512);
			SQLLEN strlenorind;
			ret = SQLGetData(hStmt, k, SQL_C_CHAR, resultrow[k-1], 512, &strlenorind);
			if (!RETURN_SUCCESS(ret))
				break;
		}
		res.addRow((char**)resultrow, numCols);
	}

	SQLFreeStmt (hStmt, SQL_DROP); 
	return true;
}

bool DatabaseODBC::beginTransaction()
{
	return true;
}

bool DatabaseODBC::commit()
{
	return RETURN_SUCCESS( SQLTransact(hEnv, hDbc, SQL_COMMIT) );
}

bool DatabaseODBC::rollback()
{
	return RETURN_SUCCESS( SQLTransact(hEnv, hDbc, SQL_ROLLBACK) );
}

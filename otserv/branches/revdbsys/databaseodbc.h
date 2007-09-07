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
// C++ Interface: databaseodbc
//
// Description: Frontend for ODBC connections
//
//
// Author: Bruno R Ferreira <brf_coldf@yahoo.com.br>, (C) 2007
//
//
#ifndef __DATABASE_ODBC_H__
#define __DATABASE_ODBC_H__

#include "database.h"

#ifdef WIN32
#include <windows.h>
#else
#include <sqltypes.h>
#endif

#include <sql.h>
#include <sqlext.h>

/**
	@author Bruno R Ferreira <brf_coldf@yahoo.com.br>
*/
class DatabaseODBC : public _Database
{
public:
	DatabaseODBC();
	DATABASE_VIRTUAL ~DatabaseODBC();

	DATABASE_VIRTUAL int getParam(DBParam_t param);

	DATABASE_VIRTUAL bool beginTransaction();
	DATABASE_VIRTUAL bool rollback();
	DATABASE_VIRTUAL bool commit();

	DATABASE_VIRTUAL bool executeQuery(const std::string &query);
	DATABASE_VIRTUAL DBResult* storeQuery(const std::string &query);

	DATABASE_VIRTUAL std::string escapeString(const std::string &s);
	DATABASE_VIRTUAL std::string escapeBlob(const char* s, uint32_t length);

	DATABASE_VIRTUAL void freeResult(DBResult *res);

protected:
	std::string _parse(const std::string &s);

	SQLHDBC m_handle;
	SQLHENV m_env;

	bool m_connected;
};

class ODBCResult : public _DBResult
{
	friend class DatabaseODBC;

public:
	DATABASE_VIRTUAL int32_t getDataInt(const std::string &s);
	DATABASE_VIRTUAL int64_t getDataLong(const std::string &s);
	DATABASE_VIRTUAL std::string getDataString(const std::string &s);
	DATABASE_VIRTUAL const char* getDataStream(const std::string &s, unsigned long &size);

	DATABASE_VIRTUAL bool next();

protected:
	ODBCResult(SQLHSTMT stmt);
	DATABASE_VIRTUAL ~ODBCResult();

	typedef std::map<const std::string, uint32_t> listNames_t;
	listNames_t m_listNames;

	SQLHSTMT m_handle;
};

#endif

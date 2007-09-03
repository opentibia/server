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
// Author: bruno <brfwolf@thormenta.com>, (C) 2007
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
	@author bruno <brfwolf@thormenta.com>
*/
class DatabaseODBC : public _Database
{
public:
	DatabaseODBC();
	DATABASE_VIRTUAL ~DatabaseODBC();

	DATABASE_VIRTUAL bool beginTransaction();
	DATABASE_VIRTUAL bool rollback();
	DATABASE_VIRTUAL bool commit();

	DATABASE_VIRTUAL DBStatement* prepareStatement(const std::string &query);

	DATABASE_VIRTUAL bool executeQuery(const std::string &query);
	DATABASE_VIRTUAL DBResult* storeQuery(const std::string &query);

	DATABASE_VIRTUAL std::string escapeString(const std::string &s);

	DATABASE_VIRTUAL void freeStatement(DBStatement *stmt);
	DATABASE_VIRTUAL void freeResult(DBResult *res);

protected:
	std::string _parse(const std::string &s);

	SQLHDBC m_handle;
	SQLHENV m_env;

	bool m_connected;
};

class ODBCStatement : public _DBStatement
{
	friend class DatabaseODBC;

public:
	DATABASE_VIRTUAL void setInt(int32_t param, int32_t value);
	DATABASE_VIRTUAL void setLong(int32_t param, int64_t value);
	DATABASE_VIRTUAL void setString(int32_t param, const std::string &value);
	DATABASE_VIRTUAL void bindStream(int32_t param, const char* value, unsigned long size);

	DATABASE_VIRTUAL bool execute();

protected:
	ODBCStatement(SQLHSTMT stmt);
	DATABASE_VIRTUAL ~ODBCStatement();

	uint16_t m_params;
	uint16_t* m_types;
	uint32_t* m_lengths;

	typedef std::map<int32_t, void*> binds_t;
	binds_t m_binds;

	SQLHSTMT m_handle;
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

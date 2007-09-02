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

#ifndef __OTSERV_DatabaseMySQL_H__
#define __OTSERV_DatabaseMySQL_H__

#ifdef __MYSQL_ALT_INCLUDE__
#include <mysql.h>
#else
#include <mysql/mysql.h>
#endif
#include <sstream>
#include "database.h"

class DatabaseMySQL : public _Database
{
public:
	DatabaseMySQL();
	DATABASE_VIRTUAL ~DatabaseMySQL();

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
	MYSQL m_handle;

	bool m_connected;
};

class MySQLStatement : public _DBStatement
{
	friend class DatabaseMySQL;
	friend class _Database;

public:
	DATABASE_VIRTUAL void setInt(int32_t param, int32_t value);
	DATABASE_VIRTUAL void setLong(int32_t param, int64_t value);
	DATABASE_VIRTUAL void setString(int32_t param, const std::string &value);
	DATABASE_VIRTUAL void bindStream(int32_t param, const char* value, unsigned long size);

	DATABASE_VIRTUAL bool execute();

protected:
	MySQLStatement(MYSQL_STMT* stmt);
	DATABASE_VIRTUAL ~MySQLStatement();

	MYSQL_STMT* m_handle;
	MYSQL_BIND* m_bind;
	uint32_t m_count;
};

class MySQLResult : public _DBResult
{
	friend class DatabaseMySQL;
	friend class MySQLStatement;
	friend class _Database;

public:
	DATABASE_VIRTUAL int32_t getDataInt(const std::string &s);
	DATABASE_VIRTUAL int64_t getDataLong(const std::string &s);
	DATABASE_VIRTUAL std::string getDataString(const std::string &s);
	DATABASE_VIRTUAL const char* getDataStream(const std::string &s, unsigned long &size);

	DATABASE_VIRTUAL bool next();

protected:
	MySQLResult(MYSQL_RES* res);
	DATABASE_VIRTUAL ~MySQLResult();

	typedef std::map<const std::string, uint32_t> listNames_t;
	listNames_t m_listNames;

	MYSQL_RES* m_handle;
	MYSQL_ROW m_row;
};

#endif

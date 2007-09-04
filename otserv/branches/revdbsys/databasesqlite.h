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

#ifndef __DATABASE_SQLITE_H__
#define __DATABASE_SQLITE_H__

#include <sqlite3.h>
#include <sstream>
#include "database.h"

class DatabaseSQLite : public _Database
{
public:
	DatabaseSQLite();
	DATABASE_VIRTUAL ~DatabaseSQLite();

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

	sqlite3* m_handle;

	bool m_connected;
};

class SQLiteResult : public _DBResult
{
	friend class DatabaseSQLite;

public:
	DATABASE_VIRTUAL int32_t getDataInt(const std::string &s);
	DATABASE_VIRTUAL int64_t getDataLong(const std::string &s);
	DATABASE_VIRTUAL std::string getDataString(const std::string &s);
	DATABASE_VIRTUAL const char* getDataStream(const std::string &s, unsigned long &size);

	DATABASE_VIRTUAL bool next();

protected:
	SQLiteResult(sqlite3_stmt* stmt);
	DATABASE_VIRTUAL ~SQLiteResult();

	typedef std::map<const std::string, uint32_t> listNames_t;
	listNames_t m_listNames;

	sqlite3_stmt* m_handle;
};

#endif

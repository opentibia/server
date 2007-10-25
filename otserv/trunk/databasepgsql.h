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

#ifndef __DATABASE_PGSQL_H__
#define __DATABASE_PGSQL_H__

#ifndef __OTSERV_DATABASE_H__
#error "database.h should be included first."
#endif

#include <postgresql/libpq-fe.h>

class DatabasePgSQL : public _Database
{
public:
	DatabasePgSQL();
	DATABASE_VIRTUAL ~DatabasePgSQL();

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

	PGconn* m_handle;

	bool m_connected;
};

class PgSQLResult : public _DBResult
{
	friend class DatabasePgSQL;

public:
	DATABASE_VIRTUAL int32_t getDataInt(const std::string &s);
	DATABASE_VIRTUAL int64_t getDataLong(const std::string &s);
	DATABASE_VIRTUAL std::string getDataString(const std::string &s);
	DATABASE_VIRTUAL const char* getDataStream(const std::string &s, unsigned long &size);

	DATABASE_VIRTUAL bool next();

protected:
	PgSQLResult(PGresult* results);
	DATABASE_VIRTUAL ~PgSQLResult();

	int32_t m_rows, m_cursor;
	PGresult* m_handle;
};

#endif

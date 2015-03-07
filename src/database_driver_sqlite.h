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

#ifdef __USE_SQLITE__

#ifndef __OTSERV_DATABASESQLITE_H__
#define __OTSERV_DATABASESQLITE_H__

#include "database_driver.h"

#include <sqlite3.h>

class DatabaseSQLite : public DatabaseDriver
{
public:
  DatabaseSQLite();
  virtual ~DatabaseSQLite();

  virtual bool getParam(DBParam_t param);

  virtual bool beginTransaction();
  virtual bool rollback();
  virtual bool commit();

  virtual uint64_t getLastInsertedRowID();

  virtual std::string escapeString(const std::string &s);
  virtual std::string escapeBlob(const char* s, uint32_t length);


protected:
  virtual bool internalQuery(const std::string &query);
  virtual DBResult_ptr internalSelectQuery(const std::string &query);
  virtual void freeResult(DBResult *res);

  std::string _parse(const std::string &s);

  boost::recursive_mutex sqliteLock;
  sqlite3* m_handle;
};

class SQLiteResult : public DBResult
{
  friend class DatabaseSQLite;

public:
  virtual int32_t getDataInt(const std::string &s);
  virtual uint32_t getDataUInt(const std::string &s);
  virtual int64_t getDataLong(const std::string &s);
  virtual std::string getDataString(const std::string &s);
  virtual const char* getDataStream(const std::string &s, unsigned long &size);

  virtual DBResult_ptr advance();
  virtual bool empty();

protected:
  SQLiteResult(sqlite3_stmt* stmt);
  virtual ~SQLiteResult();

  typedef std::map<const std::string, uint32_t> listNames_t;
  listNames_t m_listNames;

  bool m_rowAvailable;
  sqlite3_stmt* m_handle;
};

#endif

#endif

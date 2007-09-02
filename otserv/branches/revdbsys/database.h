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

#ifndef __OTSERV_DATABASE_H__
#define __OTSERV_DATABASE_H__

#include "definitions.h"
#include "otsystem.h"

#include <sstream>
#include <map>

#ifdef MULTI_SQL_DRIVERS
#define DATABASE_VIRTUAL virtual
#define DATABASE_CLASS _Database
#define DBSTMT_CLASS _DBStatement
#define DBRES_CLASS _DBResult
class _Database;
class _DBStatement;
class _DBResult;
#else
#define DATABASE_VIRTUAL

#if defined(__USE_MYSQL__)
#define DATABASE_CLASS DatabaseMySQL
#define DBSTMT_CLASS MySQLStatement
#define DBRES_CLASS MySQLResult
class DatabaseMySQL;
class MySQLStatement;
class MySQLResult;

#elif defined(__USE_SQLITE__)
#define DATABASE_CLASS DatabaseSQLite
#define DBSTMT_CLASS SQLiteStatement
#define DBRES_CLASS SQLiteResult
class DatabaseSQLite;
class SQLiteStatement;
class SQLiteResult;

#elif defined(__USE_ODBC__)
#define DATABASE_CLASS DatabaseODBC
#define DBSTMT_CLASS ODBCStatement
#define DBRES_CLASS ODBCResult
class DatabaseODBC;
class ODBCStatement;
class ODBCResult;

#elif defined(__USE_PGSQL__)
#define DATABASE_CLASS DatabasePgSQL
#define DBSTMT_CLASS PgSQLStatement
#define DBRES_CLASS PgSQLResult
class DatabasePgSQL;
class PgSQLStatement;
class PgSQLResult;

#endif
#endif

typedef DATABASE_CLASS Database;
typedef DBSTMT_CLASS DBStatement;
typedef DBRES_CLASS DBResult;

/**
 * Generic database connection handler. All drivers must extend it.
 * 
 * @author wrzasq <wrzasq@gmail.com>
 */
class _Database
{
public:
	// thread-safety for database queries
	static OTSYS_THREAD_LOCKVAR lock;

/**
 * Singleton implementation.
 * 
 * Retruns instance of database handler. Don't create database (or drivers) instances in your code - instead of it use Database::instance(). This method stores static instance of connection class internaly to make sure exacly one instance of connection is created for entire system.
 * 
 * @return database connection handler singletor
 */
	static Database* instance();

/**
 * Transaction related methods.
 * 
 * Methods for starting, commiting and rolling back transaction. Each of the returns boolean value.
 * 
 * @return true on success, false on error
 * @note
 *	If your database system doesn't support transactions you should return true - it's not feature test, code should work without transaction, just will lack integrity.
 */
	DATABASE_VIRTUAL bool beginTransaction() { return false; }
	DATABASE_VIRTUAL bool rollback() { return false; };
	DATABASE_VIRTUAL bool commit() { return false; };

/**
 * Generates prepared statement.
 * 
 * Prepared statements are queries templates compiled by database server for further execution with given list of attributes.
 * 
 * @param std::string query pattern
 * @return preated statement instance
 */
	DATABASE_VIRTUAL DBStatement* prepareStatement(const std::string &query) { return NULL; }

/**
 * Executes command.
 * 
 * Executes query which doesn't generates results (eg. INSERT, UPDATE, DELETE...).
 * 
 * @param std::string query command
 * @return true on success, false on error
 */
	DATABASE_VIRTUAL bool executeQuery(const std::string &query) { return false; }
/**
 * Queries database.
 * 
 * Executes query which generates results (mostly SELECT).
 * 
 * @param std::string query
 * @return results object (null on error)
 */
	DATABASE_VIRTUAL DBResult* storeQuery(const std::string &query) { return NULL; }

/**
 * Escapes string for query.
 * 
 * Prepares string to fit SQL queries including quoting it.
 * 
 * @param std::string string to be escaped
 * @return quoted string
 */
	DATABASE_VIRTUAL std::string escapeString(const std::string &s) { return "''"; }

/**
 * Resource freeing.
 * 
 * @param DBResult*|DBStatement resource to be freed
 */
	DATABASE_VIRTUAL void freeStatement(DBStatement *stmt) {};
	DATABASE_VIRTUAL void freeResult(DBResult *res) {};

protected:
	_Database() {};
	DATABASE_VIRTUAL ~_Database() {};

private:
	static Database* _instance;
};

/**
 * Prepared statent class.
 * 
 * @author wrzasq <wrzasq@gmail.com>
 */
class _DBStatement
{
	// unused at the moment
	//friend class Database;

public:
/**
 * Binds parameter value to of given index.
 * 
 * @param int parameter index
 */
	DATABASE_VIRTUAL void setInt(int32_t param, int32_t value) {}
	DATABASE_VIRTUAL void setLong(int32_t param, int64_t value) {}
	DATABASE_VIRTUAL void setString(int32_t param, const std::string &value) {}
	DATABASE_VIRTUAL void bindStream(int32_t param, const char* value, unsigned long size) {}

/**
 * Executes statement.
 * 
 * Runs query with currently associated parameters.
 * 
 * @return true on success, false on error
 */
	DATABASE_VIRTUAL bool execute() { return false; }

protected:
	_DBStatement() {};
	DATABASE_VIRTUAL ~_DBStatement() {};
};

/**
 * Query results.
 * 
 * @author <wrzasq@gmail.com>
 */
class _DBResult
{
	// unused at the moment
	//friend class Database;
	//friend class DBStatement;

public:
	/**Get the Integer value of a field in database
	*\returns The Integer value of the selected field and row
	*\param s The name of the field
	*/
	DATABASE_VIRTUAL int32_t getDataInt(const std::string &s) { return 0; };
	/** Get the Long value of a field in database
	*\returns The Long value of the selected field and row
	*\param s The name of the field
	*/
	DATABASE_VIRTUAL int64_t getDataLong(const std::string &s) { return 0; };
	/** Get the String of a field in database
	*\returns The String of the selected field and row
	*\param s The name of the field
	*/
	DATABASE_VIRTUAL std::string getDataString(const std::string &s) { return "''"; };
	/** Get the blob of a field in database
	*\returns a PropStream that is initiated with the blob data field, if not exist it returns NULL.
	*\param s The name of the field
	*/
	DATABASE_VIRTUAL const char* getDataStream(const std::string &s, unsigned long &size) { return NULL; };

/**
 * Moves to next result in set.
 * 
 * @return true if moved, false if there are no more results.
 */
	DATABASE_VIRTUAL bool next() {};

protected:
	_DBResult() {};
	DATABASE_VIRTUAL ~_DBResult() {};
};

#ifndef MULTI_SQL_DRIVERS
#if defined(__USE_MYSQL__)
#include "databasemysql.h"
#elif defined(__USE_SQLITE__)
#include "databasesqlite.h"
#elif defined(__USE_ODBC__)
#include "databaseodbc.h"
#elif defined(__USE_PGSQL__)
#include "databasepgsql.h"
#endif
#endif

#endif

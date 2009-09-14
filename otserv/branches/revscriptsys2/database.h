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

#ifdef MULTI_SQL_DRIVERS
#define DATABASE_VIRTUAL virtual
#define DATABASE_CLASS _Database
#define DBRES_CLASS _DBResult
class _Database;
class _DBResult;
#else
#define DATABASE_VIRTUAL

#if defined(__USE_MYSQL__)
#define DATABASE_CLASS DatabaseMySQL
#define DBRES_CLASS MySQLResult
class DatabaseMySQL;
class MySQLResult;

#elif defined(__USE_SQLITE__)
#define DATABASE_CLASS DatabaseSQLite
#define DBRES_CLASS SQLiteResult
class DatabaseSQLite;
class SQLiteResult;

#elif defined(__USE_ODBC__)
#define DATABASE_CLASS DatabaseODBC
#define DBRES_CLASS ODBCResult
class DatabaseODBC;
class ODBCResult;

#elif defined(__USE_PGSQL__)
#define DATABASE_CLASS DatabasePgSQL
#define DBRES_CLASS PgSQLResult
class DatabasePgSQL;
class PgSQLResult;

#endif
#endif

typedef DATABASE_CLASS Database;
typedef DBRES_CLASS DBResult;

class DBQuery;

enum DBParam_t{
	DBPARAM_MULTIINSERT = 1
};

class _Database
{
public:
	/**
	* Singleton implementation.
	*
	* Retruns instance of database handler. Don't create database (or drivers) instances in your code - instead of it use Database::instance(). This method stores static instance of connection class internaly to make sure exacly one instance of connection is created for entire system.
	*
	* @return database connection handler singletor
	*/
	static Database* instance();

	/**
	* Database information.
	*
	* Returns currently used database attribute.
	*
	* @param DBParam_t parameter to get
	* @return suitable for given parameter
	*/
	DATABASE_VIRTUAL bool getParam(DBParam_t param) { return false; }

	/**
	* Database connected.
	*
	* Returns whether or not the database is connected.
	*
	* @return whether or not the database is connected.
	*/
	bool isConnected() { return m_connected; }

protected:
	/**
	* Transaction related methods.
	*
	* Methods for starting, commiting and rolling back transaction. Each of the returns boolean value.
	*
	* @return true on success, false on error
	* @note
	*	If your database system doesn't support transactions you should return true - it's not feature test, code should work without transaction, just will lack integrity.
	*/
	friend class DBTransaction;
	DATABASE_VIRTUAL bool beginTransaction() { return 0; }
	DATABASE_VIRTUAL bool rollback() { return 0; }
	DATABASE_VIRTUAL bool commit() { return 0; }

public:
	/**
	* Executes command.
	*
	* Executes query which doesn't generates results (eg. INSERT, UPDATE, DELETE...).
	*
	* @param std::string query command
	* @return true on success, false on error
	*/
	DATABASE_VIRTUAL bool executeQuery(const std::string &query) { return 0; }

	/**
	 * Returns ID of last inserted row
	 *
	 * @return id of last inserted row, 0 if last query did not result in any rows with auto_increment keys
	 */
	DATABASE_VIRTUAL uint64_t getLastInsertedRowID() {return 0;}

	/**
	* Queries database.
	*
	* Executes query which generates results (mostly SELECT).
	*
	* @param std::string query
	* @return results object (null on error)
	*/
	DATABASE_VIRTUAL DBResult* storeQuery(const std::string &query) { return 0; }

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
	* Escapes binary stream for query.
	*
	* Prepares binary stream to fit SQL queries.
	*
	* @param char* binary stream
	* @param long stream length
	* @return quoted string
	*/
	DATABASE_VIRTUAL std::string escapeBlob(const char* s, uint32_t length) { return "''"; };

	/**
	* Resource freeing.
	*
	* @param DBResult* resource to be freed
	*/
	DATABASE_VIRTUAL void freeResult(DBResult *res) {};

protected:
	_Database() : m_connected(false) {};
	DATABASE_VIRTUAL ~_Database() {};

	DBResult* verifyResult(DBResult* result);

	bool m_connected;

private:
	static Database* _instance;
};

class _DBResult
{
	// unused at the moment
	//friend class Database;

public:
	/** Get the Integer value of a field in database
	*\return The Integer value of the selected field and row
	*\param s The name of the field
	*/
	DATABASE_VIRTUAL int32_t getDataInt(const std::string &s) { return 0; }
	/** Get the Long value of a field in database
	*\return The Long value of the selected field and row
	*\param s The name of the field
	*/
	DATABASE_VIRTUAL int64_t getDataLong(const std::string &s) { return 0; }
	/** Get the String of a field in database
	*\return The String of the selected field and row
	*\param s The name of the field
	*/
	DATABASE_VIRTUAL std::string getDataString(const std::string &s) { return "''"; }
	/** Get the blob of a field in database
	*\return a PropStream that is initiated with the blob data field, if not exist it returns NULL.
	*\param s The name of the field
	*/
	DATABASE_VIRTUAL const char* getDataStream(const std::string &s, unsigned long &size) { return 0; }

	/**
	* Moves to next result in set.
	*
	* \return true if moved, false if there are no more results.
	*/
	DATABASE_VIRTUAL bool next() {return false;}

protected:
	_DBResult() {};
	DATABASE_VIRTUAL ~_DBResult() {};
};

/**
 * Thread locking hack.
 *
 * By using this class for your queries you lock and unlock database for threads.
*/
class DBQuery : public std::stringstream
{
	friend class _Database;

public:
	DBQuery();
	~DBQuery();

protected:
	static boost::recursive_mutex database_lock;
};

/**
 * INSERT statement.
 *
 * Gives possibility to optimize multiple INSERTs on databases that support multiline INSERTs.
 */
class DBInsert
{
public:
	/**
	* Associates with given database handler.
	*
	* @param Database* database wrapper
	*/
	DBInsert(Database* db);
	~DBInsert() {};

	/**
	* Sets query prototype.
	*
	* @param std::string& INSERT query
	*/
	void setQuery(const std::string& query);

	/**
	* Adds new row to INSERT statement.
	*
	* On databases that doesn't support multiline INSERTs it simply execute INSERT for each row.
	*
	* @param std::string& row data
	*/
	bool addRow(const std::string& row);
	/**
	* Allows to use addRow() with stringstream as parameter.
	*/
	bool addRow(std::stringstream& row);

	/**
	* Executes current buffer.
	*/
	bool execute();

	/**
	 * Returns ID of the inserted column if it had a AUTO_INCREMENT key
	 */
	uint64_t getInsertID();

protected:
	Database* m_db;
	bool m_multiLine;
	uint32_t m_rows;
	std::string m_query;
	std::string m_buf;
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

class DBTransaction
{
public:
	DBTransaction(Database* database)
	{
		m_database = database;
		m_state = STATE_NO_START;
	}

	~DBTransaction()
	{
		if(m_state == STATE_START){
			m_database->rollback();
		}
	}

	bool begin()
	{
		m_state = STATE_START;
		return m_database->beginTransaction();
	}

	bool commit()
	{
		if(m_state == STATE_START){
			m_state = STEATE_COMMIT;
			return m_database->commit();
		}
		else{
			return false;
		}
	}

private:
	enum TransactionStates_t{
		STATE_NO_START,
		STATE_START,
		STEATE_COMMIT
	};
	TransactionStates_t m_state;
	Database* m_database;
};

#endif

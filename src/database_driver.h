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

#ifndef __OTSERV_DATABASE_DRIVER_H__
#define __OTSERV_DATABASE_DRIVER_H__

#include <iosfwd>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include "definitions.h"

class DatabaseDriver;
class DBResult;
class DBQuery;

typedef boost::shared_ptr<DBResult> DBResult_ptr;

enum DBParam_t{
	DBPARAM_MULTIINSERT = 1
};

class DatabaseDriver
{
public:
	/**
	* Singleton implementation.
	*
	* Returns instance of database handler. Don't create database (or drivers) instances in your code - instead of it use Database::instance(). This method stores static instance of connection class internaly to make sure exacly one instance of connection is created for entire system.
	*
	* @return database connection handler singleton
	*/
	static DatabaseDriver* instance();

	/**
	* Database information.
	*
	* Returns currently used database attribute.
	*
	* @param DBParam_t parameter to get
	* @return suitable for given parameter
	*/
	virtual bool getParam(DBParam_t param) { return false; }

	/**
	* Database connected.
	*
	* Returns whether or not the database is connected.
	*
	* @return whether or not the database is connected.
	*/
	bool isConnected() const { return m_connected; }

protected:
	/**
	* Transaction related methods.
	*
	* Methods for starting, committing and rolling back transaction. Each of the returns boolean value.
	*
	* @return true on success, false on error
	* @note
	*	If your database system doesn't support transactions you should return true - it's not feature test, code should work without transaction, just will lack integrity.
	*/
	friend class DBTransaction;
	virtual bool beginTransaction() = 0;
	virtual bool rollback() = 0;
	virtual bool commit() = 0;

public:
	/**
	* Executes command.
	*
	* Executes query which doesn't generates results (eg. INSERT, UPDATE, DELETE...).
	*
	* @param std::string query command
	* @return true on success, false on error
	*/
	bool executeQuery(const std::string &query);
	bool executeQuery(DBQuery &query);

	/**
	 * Returns ID of last inserted row
	 *
	 * @return id of last inserted row, 0 if last query did not result in any rows with auto_increment keys
	 */
	virtual uint64_t getLastInsertedRowID() = 0;

	/**
	* Queries database.
	*
	* Executes query which generates results (mostly SELECT).
	*
	* @param std::string query
	* @return results object (null on error)
	*/
	DBResult_ptr storeQuery(const std::string &query);
	DBResult_ptr storeQuery(DBQuery &query);

	/**
	* Escapes string for query.
	*
	* Prepares string to fit SQL queries including quoting it.
	*
	* @param std::string string to be escaped
	* @return quoted string
	*/
	virtual std::string escapeString(const std::string &s) = 0;
	/**
	* Escapes binary stream for query.
	*
	* Prepares binary stream to fit SQL queries.
	*
	* @param char* binary stream
	* @param long stream length
	* @return quoted string
	*/
	virtual std::string escapeBlob(const char* s, uint32_t length) = 0;

	/**
	* Resource freeing.
	* Used as argument to shared_ptr, you need not call this directly
	*
	* @param DBResult* resource to be freed
	*/
	virtual void freeResult(DBResult *res);

protected:
	/**
	 * Executes a query directly
	 */
	virtual bool internalQuery(const std::string &query) = 0;
	virtual DBResult_ptr internalSelectQuery(const std::string &query) = 0;

	DatabaseDriver() : m_connected(false) {};
	virtual ~DatabaseDriver() {};

	DBResult_ptr verifyResult(DBResult_ptr result);

	bool m_connected;

private:
	static DatabaseDriver* _instance;
};

class DBResult : public boost::enable_shared_from_this<DBResult>
{
public:
	/** Get the Integer value of a field in database
	*\return The Integer value of the selected field and row
	*\param s The name of the field
	*/
	virtual int32_t getDataInt(const std::string &s) { return 0; }
	/** Get the Unsigned Integer value of a field in database
	*\return The Integer value of the selected field and row
	*\param s The name of the field
	*/
	virtual uint32_t getDataUInt(const std::string &s) { return 0; }
	/** Get the Long value of a field in database
	*\return The Long value of the selected field and row
	*\param s The name of the field
	*/
	virtual int64_t getDataLong(const std::string &s) { return 0; }
	/** Get the String of a field in database
	*\return The String of the selected field and row
	*\param s The name of the field
	*/
	virtual std::string getDataString(const std::string &s) { return "''"; }
	/** Get the blob of a field in database
	*\return a PropStream that is initiated with the blob data field, if not exist it returns NULL.
	*\param s The name of the field
	*/
	virtual const char* getDataStream(const std::string &s, unsigned long &size) { return 0; }

	/**
	* Moves to next result in set.
	*
	* \return true if moved, false if there are no more results.
	*/
	virtual DBResult_ptr advance() {return DBResult_ptr();}

	/**
	 * Are there any more rows to be fetched
	 * \return true if there are no more rows
	 */
	virtual bool empty() {return true;}

protected:
	DBResult() {};
	virtual ~DBResult() {};
};

/**
 * Thread locking hack.
 *
 * By using this class for your queries you lock and unlock database for threads.
*/
class DBQuery : public std::ostringstream
{
	friend class DatabaseDriver;

public:
	DBQuery();
	~DBQuery();

	void reset() {str("");}

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
	DBInsert(DatabaseDriver* db);
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
	* This also clears the stringstream!
	*/
	bool addRowAndReset(std::ostringstream& row);

	/**
	* Executes current buffer.
	*/
	bool execute();

	/**
	 * Returns ID of the inserted column if it had a AUTO_INCREMENT key
	 */
	uint64_t getInsertID();

protected:
	DatabaseDriver* m_db;
	bool m_multiLine;
	uint32_t m_rows;
	std::string m_query;
	std::ostringstream m_buf;
};

class DBTransaction
{
public:
	DBTransaction(DatabaseDriver* database)
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
	DatabaseDriver* m_database;
};

#endif

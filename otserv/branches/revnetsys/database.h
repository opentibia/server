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

#ifdef WIN32
#include <winsock2.h>
#endif

#ifdef __MYSQL_ALT_INCLUDE__
#include <mysql.h>
#elif defined __USE_MYSQL__
#include <mysql/mysql.h>
#endif
#include <sstream>
#include <map>

enum db_error_t {
	DB_ERROR_UNKNOWN = 100,
	DB_ERROR_INIT,
	DB_ERROR_CONNECT,
	DB_ERROR_SELECT,
	DB_ERROR_QUERY,
	DB_ERROR_STORE,
	DB_ERROR_DATA_NOT_FOUND,
	DB_ERROR_BUFFER_EXCEEDED,
};

struct RowData{
	char** row;
	unsigned long** length;
};

class DBQuery : public std::stringstream
{
public:
	DBQuery();
	~DBQuery();

	/** Reset the actual query */
	void reset(){ this->str("");};

	/** Get the text of the query
	*\returns The text of the actual query
	*/
	const char *getText(){ return this->str().c_str(); };

	/** Get size of the query text
	*\returns The size of the query text
	*/
	int getSize(){ return (int)this->str().length(); };

protected:
	static OTSYS_THREAD_LOCKVAR database_lock;
	friend class _Database;
};

class DBResult
{
public:
	DBResult();
	~DBResult();

	/** Get the Integer value of a field in database
	*\returns The Integer value of the selected field and row
	*\param s The name of the field
	*\param nrow The number of the row
	*/
	int32_t getDataInt(const std::string &s, unsigned int nrow=0);

	/** Get the Long value of a field in database
	*\returns The Long value of the selected field and row
	*\param s The name of the field
	*\param nrow The number of the row
	*/
	int64_t getDataLong(const std::string &s, unsigned int nrow=0);

	/** Get the String of a field in database
	*\returns The String of the selected field and row
	*\param s The name of the field
	*\param nrow The number of the row
	*/
	std::string getDataString(const std::string &s, unsigned int nrow=0);

	/** Get the blob of a field in database
	*\returns a PropStream that is initiated with the blob data field, if not exist it returns NULL.
	*\param s The name of the field
	*\param nrow The number of the row
	*/
	const char* getDataBlob(const std::string &s, unsigned long& size, unsigned int nrow=0);

	/** Get the number of rows
	*\returns The number of rows
	*/
	unsigned int getNumRows(){ return m_numRows; };

	/** Get the number of fields
	*\returns The number of fields
	*/
	unsigned int getNumFields(){ return m_numFields; };

private:
    //friend class Database;
	#ifdef __USE_MYSQL__
	friend class DatabaseMySQL;
	void addRow(MYSQL_ROW r, unsigned long* lengths, unsigned int num_fields);
	#endif
	#ifdef __USE_SQLITE__
	friend class DatabaseSqLite;
	void addRow(char **results, unsigned int num_fields);
	#endif
	void clear();
	//void clearRows();
	//void clearFieldNames();
	void setFieldName(const std::string &s, unsigned int n){
		m_listNames[s] = n;
		m_numFields++;
	};

	unsigned int m_numFields;
	//unsigned int m_lastNumFields;
	unsigned int m_numRows;
	typedef std::map<const std::string, unsigned int> listNames_type;
	listNames_type m_listNames;
	//typedef std::map<unsigned int, char **> RowDataMap;
	typedef std::map<unsigned int, RowData* > RowDataMap;
	RowDataMap m_listRows;
};

#ifdef USE_MYSQL_ONLY
#define DATABASE_VIRTUAL
#define DATABASE_CLASS DatabaseMySQL
class DatabaseMySQL;
#else
#define DATABASE_VIRTUAL virtual
#define DATABASE_CLASS _Database
class _Database;
#endif

typedef DATABASE_CLASS Database;

class _Database
{
public:
	/** Get Database instance
	*\returns
	* 	Database instance
	*\note
	*	When you get database instance
	*	be sure that you define a DBQuery object
	*	under it to lock database instance usage
	*/
    static Database* instance();

	/** Connect to a mysql database
	*\returns
	* 	TRUE if the connection is ok
	* 	FALSE if the connection fails
	*/
	DATABASE_VIRTUAL bool connect(){return false;};

	/** Disconnects from the connected database
	*\returns
	* 	TRUE if the database was disconnected
	* 	FALSE if the database was not disconnected or no database selected
	*/
	DATABASE_VIRTUAL bool disconnect(){return false;};

	/** Execute a query which don't get any information of the database (for ex.: INSERT, UPDATE, etc)
	*\returns
	* 	TRUE if the query is ok
	* 	FALSE if the query fails
	*\ref q The query object
	*/
	DATABASE_VIRTUAL bool executeQuery(DBQuery &q ){return false;};

	/** Store a query which get information of the database (for ex.: SELECT)
	*\returns
	* 	TRUE if the query is ok
	* 	FALSE if the query fails
	*\ref q The query object
	*\ref res The DBResult object where to insert the results of the query
	*/
	DATABASE_VIRTUAL bool storeQuery(DBQuery &q, DBResult &res){return false;};

	/** Transaciont related functions
	*\returns
	* 	TRUE
	* 	FALSE
	*/
	DATABASE_VIRTUAL bool rollback(){return false;};
	DATABASE_VIRTUAL bool commit(){return false;};

	/** Escape the special characters in a string for no problems with the query
	*\returns The string modified
	*\param s The source string
	*/
	static std::string escapeString(const std::string &s);

	/** Escape the special characters in a string for no problems with the query
	*\returns The string modified
	*\param s The source string
	*/
	static std::string escapeString(const char* s, unsigned long size);

protected:
	_Database(){};
	DATABASE_VIRTUAL ~_Database(){};
	static Database* _instance;
};

#ifdef USE_MYSQL_ONLY
#include "databasemysql.h"
#endif

class DBTransaction
{
public:
	DBTransaction(Database* database);
	~DBTransaction();

	bool start();
	bool success();

private:
	enum TransactionStates_t{
		STATE_NO_START,
		STATE_START,
		STEATE_COMMIT,
	};
	TransactionStates_t m_state;
	Database* m_database;
};

class DBSplitInsert
{
public:
	DBSplitInsert(Database* database);
	~DBSplitInsert();

	bool addRow(const std::string& row);

	void setQuery(const std::string& query);

	bool executeQuery();

	void clear();
private:

	bool internalExecuteQuery();

	Database* m_database;
	std::string m_query;
	std::string m_buffer;
};

#endif

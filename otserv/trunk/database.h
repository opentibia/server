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

#ifdef WIN32
//#include <winsock2.h>
#include <winsock.h>
#endif

#ifdef __USE_SQLITE__
    #ifndef __SPLIT_QUERIES__
        #define __SPLIT_QUERIES__
    #endif
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
	DBQuery(){first = true;};
	~DBQuery(){};

	/** Reset the actual query */
	void reset(){ this->str(""); first = true;};

	/** Get the text of the query
	*\returns The text of the actual query
	*/
	const char *getText(){ return this->str().c_str(); };

	/** Get size of the query text
	*\returns The size of the query text
	*/
	int getSize(){ return (int)this->str().length(); };

	std::string getSeparator(){
	    #ifndef __SPLIT_QUERIES__
		if(first){
			first = false;
			return "";
		}
		else{
			return ",";
		}
		#else
		return "";
		#endif
	}
protected:
	bool first;
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
	int getDataInt(const std::string &s, unsigned int nrow=0);

	/** Get the Long value of a field in database
	*\returns The Long value of the selected field and row
	*\param s The name of the field
	*\param nrow The number of the row
	*/
	long getDataLong(const std::string &s, unsigned int nrow=0);

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
    friend class Database;
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

class DBError
{
public:
	DBError(const char *msg, db_error_t type=DB_ERROR_UNKNOWN){
		m_msg = std::string(msg);
		m_type = type;
	};
	~DBError(){};

	/** Get the error message
	*\returns The text message
	*/
	const char *getMsg(){ return m_msg.c_str(); };

	/** Get the error type
	*\returns The error type
	*/
	int getType(){ return m_type; };

private:
	std::string m_msg;
	int m_type;
};


class Database
{
public:

    static Database* instance();

	/** Connect to a mysql database
	*\returns
	* 	TRUE if the connection is ok
	* 	FALSE if the connection fails
	*\param db_name The "name" of the database used
	*\param db_host The "host" to connect to
	*\param db_user The "username" used in the connection
	*\param db_pass The "password" of the username used
	*/
	virtual bool connect(const char *db_name, const char *db_host, const char *db_user, const char *db_pass){};

	/** Disconnects from the connected database
	*\returns
	* 	TRUE if the database was disconnected
	* 	FALSE if the database was not disconnected or no database selected
	*/
	virtual bool disconnect(){};

	/** Execute a query which don't get any information of the database (for ex.: INSERT, UPDATE, etc)
	*\returns
	* 	TRUE if the query is ok
	* 	FALSE if the query fails
	*\ref q The query object
	*/
	virtual bool executeQuery(DBQuery &q ){};

	/** Store a query which get information of the database (for ex.: SELECT)
	*\returns
	* 	TRUE if the query is ok
	* 	FALSE if the query fails
	*\ref q The query object
	*\ref res The DBResult object where to insert the results of the query
	*/
	virtual bool storeQuery(DBQuery &q, DBResult &res){};

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
	Database(){};
	virtual ~Database(){};
	static Database* _instance;
};

#endif

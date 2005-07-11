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

#ifndef __DATABASE_H
#define __DATABASE_H

#ifdef WIN32
	#include <winsock2.h>
#endif
#include <mysql.h>
#include <sstream>
#include <map>

#define DB_BUFFER_SIZE 2048

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

class DBQuery : public std::stringstream
{
public:
	DBQuery(){};
	~DBQuery(){};
	
	/** Reset the actual query */
	void Reset(){ this->str(""); };
	
	/** Get the text of the query
	*\returns The text of the actual query
	*/
	const char *GetText(){ return this->str().c_str(); };
	
	/** Get size of the query text
	*\returns The size of the query text
	*/
	int GetSize(){ return (int)this->str().size(); };
};

class DBResult
{
public:
	DBResult(){ m_numFields=0; m_numRows=0; m_lastNumFields=0; };
	~DBResult();
	
	/** Get the Integer value of a field in database
	*\returns The Integer value of the selected field and row
	*\param s The name of the field
	*\param nrow The number of the row
	*/ 
	int GetDataInt(std::string s, unsigned int nrow=0);
	
	/** Get the Long value of a field in database
	*\returns The Long value of the selected field and row
	*\param s The name of the field
	*\param nrow The number of the row
	*/
	long GetDataLong(std::string s, unsigned int nrow=0);
	
	/** Get the String of a field in database
	*\returns The String of the selected field and row
	*\param s The name of the field
	*\param nrow The number of the row
	*/
	std::string GetDataString(std::string s, unsigned int nrow=0);
	
	/** Get the number of rows
	*\returns The number of rows
	*/
	unsigned int GetNumRows(){ return m_numRows; };
	
	/** Get the number of fields
	*\returns The number of fields
	*/
	unsigned int GetNumFields(){ return m_numFields; };
	
private:
	friend class Database;
	void AddRow(MYSQL_ROW r, unsigned int num_fields);
	void ClearRows();
	void ClearFieldNames();
	void SetFieldName(std::string s, unsigned int n){
		m_listNames[s] = n; 
		m_numFields++;
	};
	
	unsigned int m_numFields;
	unsigned int m_lastNumFields;
	unsigned int m_numRows;
	std::map<std::string, unsigned int> m_listNames;
	std::map<unsigned int, char **> m_listRows;
};

class DBError
{
public:
	DBError(const char *msg, db_error_t type=DB_ERROR_UNKNOWN){
		m_msg = msg;
		m_type = type;
	};
	~DBError(){};
	
	/** Get the error message
	*\returns The text message
	*/
	const char *GetMsg(){ return m_msg; };
	
	/** Get the error type
	*\returns The error type
	*/
	int GetType(){ return m_type; };
	
private:
	const char *m_msg;
	int m_type;
};


class Database
{
public:
	Database();
	~Database();
	
	/** Connect to a mysql database
	*\returns
	* 	TRUE if the connection is ok
	* 	FALSE if the connection fails
	*\param db_name The "name" of the database used
	*\param db_host The "host" to connect to
	*\param db_user The "username" used in the connection
	*\param db_pass The "password" of the username used
	*/ 
	bool Connect(const char *db_name, const char *db_host, const char *db_user, const char *db_pass);
	
	/** Execute a query which don't get any information of the database (for ex.: INSERT, UPDATE, etc)
	*\returns
	* 	TRUE if the query is ok
	* 	FALSE if the query fails
	*\ref q The query object
	*/ 
	bool ExecuteQuery(DBQuery &q);
	
	/** Store a query which get information of the database (for ex.: SELECT)
	*\returns
	* 	TRUE if the query is ok
	* 	FALSE if the query fails
	*\ref q The query object
	*\ref res The DBResult object where to insert the results of the query
	*/ 
	bool StoreQuery(DBQuery &q, DBResult &res);
	
	/** Escape the special characters in a string for no problems with the query
	*\returns The string modified
	*\param s The source string
	*/
	static std::string EscapeString(std::string s);
	
private:
	bool m_initialized;
	bool m_connected;
	MYSQL m_handle;
};


#endif

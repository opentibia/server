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

#include <iostream>
#include "database.h"
#include "mysql.h"

DBResult::~DBResult()
{
	std::map<unsigned int, char **>::iterator it;
	for(it = m_listRows.begin(); it != m_listRows.end();)
	{
		for(unsigned int i=0; i < m_numFields; ++i)
			delete[] it->second[i];
		
		delete[] it->second;
		m_listRows.erase(it++);
	}
	m_numRows = 0;
}

void DBResult::addRow(MYSQL_ROW r, unsigned int num_fields)
{
	char **row = new char*[num_fields];
	for(unsigned int i=0; i < num_fields; ++i)
	{
		row[i] = new char[strlen(r[i])+1];
		memcpy(row[i], r[i], strlen(r[i])+1);
	}
	
	m_listRows[m_numRows] = row;
	m_numRows++;
}

void DBResult::clearRows()
{
	std::map<unsigned int, char **>::iterator it;
	for(it = m_listRows.begin(); it != m_listRows.end();)
	{
		for(unsigned int i=0; i < m_lastNumFields; ++i)
			delete[] it->second[i];
		
		delete[] it->second;
		m_listRows.erase(it++);
	}
	m_numRows = 0;
}

void DBResult::clearFieldNames()
{
	std::map<std::string, unsigned int>::iterator it;
	for(it = m_listNames.begin(); it != m_listNames.end();)
	{
		m_listNames.erase(it++);
	}
	m_lastNumFields = m_numFields;
	m_numFields = 0;
}

int DBResult::getDataInt(std::string s, unsigned int nrow)
{
	std::map<std::string, unsigned int>::iterator it=m_listNames.find(s);
	if(it != m_listNames.end())
	{
		std::map<unsigned int, char **>::iterator it2=m_listRows.find(nrow);
		if(it2 != m_listRows.end())
		{
			return atoi(it2->second[it->second]);
		}
	}
	
	//throw DBError("DBResult::GetDataInt()", DB_ERROR_DATA_NOT_FOUND);
	std::cout << "MYSQL ERROR DBResult::GetDataInt()" << std::endl;
	return 0; // Failed
}

long DBResult::getDataLong(std::string s, unsigned int nrow)
{
	std::map<std::string, unsigned int>::iterator it=m_listNames.find(s);
	if(it != m_listNames.end())
	{
		std::map<unsigned int, char **>::iterator it2=m_listRows.find(nrow);
		if(it2 != m_listRows.end())
		{
			return atol(it2->second[it->second]);
		}
	}
	
	//throw DBError("DBResult::GetDataLong()", DB_ERROR_DATA_NOT_FOUND);
	std::cout << "MYSQL ERROR DBResult::GetDataLong()" << std::endl;
	return 0; // Failed
}


std::string DBResult::getDataString(std::string s, unsigned int nrow)
{
	std::map<std::string, unsigned int>::iterator it=m_listNames.find(s);
	if(it != m_listNames.end())
	{
		std::map<unsigned int, char **>::iterator it2=m_listRows.find(nrow);
		if(it2 != m_listRows.end())
		{
			return std::string(it2->second[it->second]);
		}
	}
	
	//throw DBError("DBResult::GetDataString()", DB_ERROR_DATA_NOT_FOUND);
	std::cout << "MYSQL ERROR DBResult::GetDataString()" << std::endl;
	return std::string(""); // Failed
}

Database::Database()
{
	m_initialized = false;
	m_connected = false;
	
	// Initialize mysql
	if(mysql_init(&m_handle) == NULL){
		//throw DBError("mysql_init", DB_ERROR_INIT);
		std::cout << "MYSQL ERROR mysql_init" << std::endl;
	}
	else
		m_initialized = true;
}

Database::~Database()
{
	if(m_initialized)
	{
		mysql_close(&m_handle);
		m_initialized = false;
	}
}

bool Database::connect(const char *db_name, const char *db_host, const char *db_user, const char *db_pass)
{
	if(!m_initialized)
		return false;
	
	// Connect to the database host
	if(!mysql_real_connect(&m_handle, db_host, db_user, db_pass, NULL, 0, NULL, 0))
	{
		//throw DBError(mysql_error(&m_handle), DB_ERROR_CONNECT);
		std::cout << "MYSQL ERROR mysql_real_connect: " << mysql_error(&m_handle)  << std::endl;
		return false;
	}
	
	// Select the correct database
	if(mysql_select_db(&m_handle, db_name))
	{
		//throw DBError("mysql_select_db", DB_ERROR_SELECT);
		std::cout << "MYSQL ERROR mysql_select_db"  << std::endl;
		return false;
	}
	
	m_connected = true;
	return true;
}

bool Database::executeQuery(DBQuery &q)
{
	if(!m_initialized || !m_connected)
		return false;
	
	// Execute the query
	if(mysql_real_query(&m_handle, q.getText(), q.getSize()))
	{
		//throw DBError( q.getText() , DB_ERROR_QUERY );
		std::cout << "MYSQL ERROR mysql_real_query: " << q.getText() << " " << mysql_error(&m_handle)  << std::endl;
		return false;
	}
	
	// All is ok
	q.reset();
	return true;
}

bool Database::storeQuery(DBQuery &q, DBResult &dbres)
{	
	MYSQL_ROW row;
	MYSQL_FIELD *fields;
	MYSQL_RES *r;
	unsigned int num_fields;
	
	// Execute the query
	if(!this->executeQuery(q))
		return false;
	
	
	// Getting results from the query
	r = mysql_store_result(&m_handle);
	if(!r)
	{
		//throw DBError( mysql_error(&m_handle), DB_ERROR_STORE );
		std::cout << "MYSQL ERROR mysql_store_result: " << q.getText() << " " << mysql_error(&m_handle)  << std::endl;
		return false;
	}
	
	// Getting the rows of the result
	num_fields = mysql_num_fields(r);
	
	// Getting the field names
	dbres.clearFieldNames();
	fields = mysql_fetch_fields(r);
	for(int i=0; i < num_fields; ++i)
	{
		dbres.setFieldName(std::string(fields[i].name), i);
	}
	
	// Adding the rows to a list
	dbres.clearRows();
	while(row = mysql_fetch_row(r))
	{
		dbres.addRow(row, num_fields);
	}
	
	// Free query result
	mysql_free_result(r);
	r = NULL;
	
	// Check if there are rows in the query
	if(dbres.getNumRows() > 0)
		return true;
	else
		return false;
}

std::string Database::escapeString(std::string s)
{
	//char output[DB_BUFFER_SIZE];
	char* output = new char[s.size() * 2 + 1];
	/*
	if((DB_BUFFER_SIZE * 0.9) < s.size())
	{
		throw DBError("Database::EscapeString()", DB_ERROR_BUFFER_EXCEEDED);
		return std::string("");
	}
	*/
	mysql_escape_string(output, s.c_str(), s.size());
	std::string r = std::string(output);
	delete output;
	return r;
}

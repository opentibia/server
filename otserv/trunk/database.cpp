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
		if(r[i] == NULL)
		{
			row[i] = NULL;
			continue;
		}
		row[i] = new char[strlen(r[i])+1];
		memcpy(row[i], r[i], strlen(r[i])+1);
	}
	
	m_listRows[m_numRows] = row;
	m_numRows++;
}
/*
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
	m_listNames.clear();
	m_lastNumFields = m_numFields;
	m_numFields = 0;
}
*/

void DBResult::clear()
{
	std::map<unsigned int, char **>::iterator it;
	for(it = m_listRows.begin(); it != m_listRows.end();)
	{
		for(unsigned int i = 0; i < m_numFields; ++i)
			if(it->second[i] != NULL)
				delete[] it->second[i];
		
		delete[] it->second;
		m_listRows.erase(it++);
	}
	m_numRows = 0;
	m_listNames.clear();
	m_numFields = 0;
}

int DBResult::getDataInt(const std::string &s, unsigned int nrow)
{
	listNames_type::iterator it=m_listNames.find(s);
	if(it != m_listNames.end())
	{
		std::map<unsigned int, char **>::iterator it2=m_listRows.find(nrow);
		if(it2 != m_listRows.end())
		{
			if(it2->second[it->second] == NULL) 
				return 0;
			else
				return atoi(it2->second[it->second]);
		}
	}
	
	//throw DBError("DBResult::GetDataInt()", DB_ERROR_DATA_NOT_FOUND);
	std::cout << "MYSQL ERROR DBResult::GetDataInt()" << std::endl;
	return 0; // Failed
}

long DBResult::getDataLong(const std::string &s, unsigned int nrow)
{
	listNames_type::iterator it=m_listNames.find(s);
	if(it != m_listNames.end())
	{
		std::map<unsigned int, char **>::iterator it2=m_listRows.find(nrow);
		if(it2 != m_listRows.end())
		{
			if(it2->second[it->second] == NULL) 
				return 0;
			else
				return atol(it2->second[it->second]);
		}
	}
	
	//throw DBError("DBResult::GetDataLong()", DB_ERROR_DATA_NOT_FOUND);
	std::cout << "MYSQL ERROR DBResult::GetDataLong()" << std::endl;
	return 0; // Failed
}


std::string DBResult::getDataString(const std::string &s, unsigned int nrow)
{
	listNames_type::iterator it=m_listNames.find(s);
	if(it != m_listNames.end())
	{
		std::map<unsigned int, char **>::iterator it2=m_listRows.find(nrow);
		if(it2 != m_listRows.end())
		{
			if(it2->second[it->second] == NULL) 
				return std::string("");
			else
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
	
	std::string s = q.str();
	const char* querytext = s.c_str();
	int querylength = s.length(); //strlen(querytext);
	// Execute the query
	if(mysql_real_query(&m_handle, querytext, querylength))
	{
		//throw DBError( q.getText() , DB_ERROR_QUERY );
		std::cout << "MYSQL ERROR mysql_real_query: " << q.str() << " " << mysql_error(&m_handle)  << std::endl;
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
	
	dbres.clear();
	// Getting the field names
	//dbres.clearFieldNames();
	fields = mysql_fetch_fields(r);
	for(int i=0; i < num_fields; ++i)
	{
		dbres.setFieldName(std::string(fields[i].name), i);
	}
	
	// Adding the rows to a list
	//dbres.clearRows();
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

std::string Database::escapeString(const std::string &s)
{
	if(s.size() == 0)
		return std::string("");
	
	char* output = new char[s.size() * 2 + 1];
	
	mysql_escape_string(output, s.c_str(), s.size());
	std::string r = std::string(output);
	delete[] output;
	return r;
}

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
#include "databasemysql.h"
#include "errmsg.h"

DatabaseMySQL::DatabaseMySQL()
{
	init();
}

DatabaseMySQL::~DatabaseMySQL()
{
	disconnect();
}

bool DatabaseMySQL::init()
{
	m_initialized = false;
	m_connected = false;

	// Initialize mysql
	if(mysql_init(&m_handle) == NULL){
		std::cout << "MYSQL ERROR mysql_init" << std::endl;
	}
	else
		m_initialized = true;

	return m_initialized;
}

bool DatabaseMySQL::connect(const char *db_name, const char *db_host, const char *db_user, const char *db_pass)
{
	if(!m_initialized && !init()){
		return false;
	}
	
	if(m_connected){
		return true;
	}

	// Connect to the DatabaseMySQL host
	if(!mysql_real_connect(&m_handle, db_host, db_user, db_pass, NULL, 0, NULL, 0))
	{
		std::cout << "MYSQL ERROR mysql_real_connect: " << mysql_error(&m_handle)  << std::endl;
		return false;
	}

	// Select the correct DatabaseMySQL
	if(mysql_select_db(&m_handle, db_name))
	{
		std::cout << "MYSQL ERROR mysql_select_db"  << std::endl;
		return false;
	}

	m_connected = true;
	return true;
}

bool DatabaseMySQL::disconnect()
{
	if(m_initialized){
		mysql_close(&m_handle);
		m_initialized = false;
		return true;
	}

	return false;
}

bool DatabaseMySQL::executeQuery(DBQuery &q)
{
	if(!m_initialized || !m_connected)
		return false;

	std::string s = q.str();
	const char* querytext = s.c_str();
	int querylength = s.length(); //strlen(querytext);
	// Execute the query
	if(int error = mysql_real_query(&m_handle, querytext, querylength))
	{
		std::cout << "MYSQL ERROR mysql_real_query: " << q.str() << " " << mysql_error(&m_handle)  << std::endl;
		if(error == CR_SERVER_LOST || error == CR_SERVER_GONE_ERROR){
			m_connected = false;
		}
		return false;
	}

	// All is ok
	q.reset();
	return true;
}

bool DatabaseMySQL::storeQuery(DBQuery &q, DBResult &dbres)
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
		std::cout << "MYSQL ERROR mysql_store_result: " << q.getText() << " " << mysql_error(&m_handle)  << std::endl;
		int error = mysql_errno(&m_handle);
		if(error == CR_SERVER_LOST || error == CR_SERVER_GONE_ERROR){
			m_connected = false;
		}
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
		//get column sizes
		unsigned long* lengths = mysql_fetch_lengths(r);
		dbres.addRow(row, lengths, num_fields);
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

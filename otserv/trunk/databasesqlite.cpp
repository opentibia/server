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
#include "databasesqlite.h"
#include "configmanager.h"

extern ConfigManager g_config;

DatabaseSqLite::DatabaseSqLite()
{
	init();
}

DatabaseSqLite::~DatabaseSqLite()
{
	disconnect();
}

bool DatabaseSqLite::m_fieldnames = false;

bool DatabaseSqLite::init()
{
	m_initialized = false;
	m_connected = false;

	// Initialize mysql
	if(sqlite3_open(g_config.getString(ConfigManager::SQLITE_DB).c_str(), &m_handle) != SQLITE_OK){
		//throw DBError("sqlite_init", DB_ERROR_INIT);
		std::cout << "SQLITE ERROR sqlite_init" << std::endl;
		sqlite3_close(m_handle);
	}
	else
		m_initialized = true;

	return m_initialized;
}

bool DatabaseSqLite::connect()
{
	//don't need to connect
	m_connected = true;
	return true;
}

bool DatabaseSqLite::disconnect()
{
	if(m_initialized){
		sqlite3_close(m_handle);
		m_initialized = false;
		return true;
	}

	return false;
}

bool DatabaseSqLite::executeQuery(DBQuery &q)
{
	if(!m_initialized || !m_connected)
		return false;

	std::string s = q.str();
	const char* querytext = s.c_str();
	// Execute the query
	if(sqlite3_exec(m_handle, querytext, 0, 0, &zErrMsg) != SQLITE_OK)
	{
		//throw DBError( q.getText() , DB_ERROR_QUERY );
		std::cout << "SQLITE ERROR sqlite_exec: " << q.str() << " " << zErrMsg << std::endl;
		sqlite3_free(zErrMsg);
		return false;
	}

	// All is ok
	q.reset();
	return true;
}

bool DatabaseSqLite::storeQuery(DBQuery &q, DBResult &dbres)
{
	if(!m_initialized || !m_connected)
		return false;

	std::string s = q.str();
	const char* querytext = s.c_str();

	q.reset();
	dbres.clear();
	// Execute the query
	if(sqlite3_exec(m_handle, querytext, DatabaseSqLite::callback, &dbres, &zErrMsg) != SQLITE_OK)
	{
		//throw DBError( q.getText() , DB_ERROR_QUERY );
		std::cout << "SQLITE ERROR sqlite_exec: " << q.str() << " " << zErrMsg << std::endl;
		sqlite3_free(zErrMsg);
		return false;
	}
	DatabaseSqLite::m_fieldnames = false;

	// Check if there are rows in the query
	if(dbres.getNumRows() > 0)
		return true;
	else
		return false;
}

bool DatabaseSqLite::rollback()
{
	DBQuery query;
	query << "ROLLBACK;";

	if(executeQuery(query)){
		return true;
	}
	else{
		return false;
	}
}

bool DatabaseSqLite::commit()
{
	DBQuery query;
	query << "COMMIT;";

	if(executeQuery(query)){
		return true;
	}
	else{
		return false;
	}
}

int DatabaseSqLite::callback(void *db, int num_fields, char **results, char **columnNames){
	DBResult* dbres = (DBResult*)db;
	if(!DatabaseSqLite::m_fieldnames){
		for(int i=0; i<num_fields; i++){
			//std::cout <<"'"<< std::string(columnNames[i]) <<"' : '" << results[i] <<"'\t";
			dbres->setFieldName(std::string(columnNames[i]), i);
		}
		DatabaseSqLite::m_fieldnames = true;
	}
	//std::cout << "\n";
	dbres->addRow(results, num_fields);
	return 0;
}

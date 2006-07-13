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
#include <string>
#include "database.h"

#ifdef __USE_MYSQL__
#include "databasemysql.h"
#endif
#ifdef __USE_SQLITE__
#include "databasesqlite.h"
#endif

#if defined __USE_MYSQL__ && defined __USE_SQLITE__
#include "configmanager.h"
extern ConfigManager g_config;
#endif


OTSYS_THREAD_LOCKVAR DBQuery::database_lock;

DBQuery::DBQuery(){
	OTSYS_THREAD_LOCK(database_lock, NULL);
}

DBQuery::~DBQuery()
{
	OTSYS_THREAD_UNLOCK(database_lock, NULL);
}

DBResult::DBResult()
{
	m_numFields = 0;
	m_numRows = 0;
	/*m_lastNumFields = 0;*/
}

DBResult::~DBResult()
{
	clear();
}
#ifdef __USE_MYSQL__
void DBResult::addRow(MYSQL_ROW r, unsigned long* lengths, unsigned int num_fields)
{
	RowData* rd = new RowData;
	rd->row = new char*[num_fields];
	rd->length = new unsigned long*[num_fields];

	for(unsigned int i=0; i < num_fields; ++i)
	{
		if(r[i] == NULL)
		{
			rd->row[i] = NULL;
			rd->length[i] = NULL;
			continue;
		}

		unsigned long colLen = lengths[i];
		rd->row[i] = new char[colLen + 1];
		rd->length[i] = new unsigned long;

		memcpy(rd->row[i], r[i], colLen + 1);
		memcpy(rd->length[i], &lengths[i], sizeof(unsigned long));
	}

	m_listRows[m_numRows] = rd;
	m_numRows++;
}
#endif
#ifdef __USE_SQLITE__
void DBResult::addRow(char **results, unsigned int num_fields)
{
	RowData* rd = new RowData;
	rd->row = new char*[num_fields];
	rd->length = new unsigned long*[num_fields];

	for(unsigned int i=0; i < num_fields; ++i)
	{
		if(results[i] == NULL)
		{
			rd->row[i] = NULL;
			rd->length[i] = NULL;
			continue;
		}

		unsigned long colLen = strlen(results[i]);
		rd->row[i] = new char[colLen+1];
		rd->length[i] = new unsigned long;
        /*std::cout <<"'"<< colLen <<"' : '" << results[i] <<"'\n";*/
		memcpy(rd->row[i], results[i], colLen+1);
		*(rd->length[i]) = colLen;
	}

	m_listRows[m_numRows] = rd;
	m_numRows++;
}
#endif
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
	RowDataMap::iterator it;
	for(it = m_listRows.begin(); it != m_listRows.end();)
	{
		for(unsigned int i = 0; i < m_numFields; ++i){
			if(it->second->row[i] != NULL)
				delete[] it->second->row[i];

			if(it->second->length[i] != NULL)
				delete[] it->second->length[i];
		}

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
		RowDataMap::iterator it2=m_listRows.find(nrow);
		if(it2 != m_listRows.end())
		{
			if(it2->second->row[it->second] == NULL)
				return 0;
			else
				return atoi(it2->second->row[it->second]);
		}
	}

	std::cout << "SQL ERROR DBResult::GetDataInt()" << std::endl;
	return 0; // Failed
}

long DBResult::getDataLong(const std::string &s, unsigned int nrow)
{
	listNames_type::iterator it=m_listNames.find(s);
	if(it != m_listNames.end())
	{
		RowDataMap::iterator it2=m_listRows.find(nrow);
		if(it2 != m_listRows.end())
		{
			if(it2->second->row[it->second] == NULL)
				return 0;
			else
				return atol(it2->second->row[it->second]);
		}
	}

	std::cout << "SQL ERROR DBResult::GetDataLong()" << std::endl;
	return 0; // Failed
}


std::string DBResult::getDataString(const std::string &s, unsigned int nrow)
{
	listNames_type::iterator it=m_listNames.find(s);
	if(it != m_listNames.end())
	{
		RowDataMap::iterator it2=m_listRows.find(nrow);
		if(it2 != m_listRows.end())
		{
			if(it2->second->row[it->second] == NULL)
				return std::string("");
			else
				return std::string(it2->second->row[it->second]);
		}
	}

	std::cout << "SQL ERROR DBResult::GetDataString()" << std::endl;
	return std::string(""); // Failed
}

const char* DBResult::getDataBlob(const std::string &s, unsigned long& size, unsigned int nrow)
{
	listNames_type::iterator it=m_listNames.find(s);
	if(it != m_listNames.end())
	{
		RowDataMap::iterator it2=m_listRows.find(nrow);
		if(it2 != m_listRows.end())
		{
			if(it2->second->row[it->second] == NULL){
				size = 0;
				return NULL;
			}
			else{
				size = *it2->second->length[it->second];
				return it2->second->row[it->second];
			}
		}
	}

	std::cout << "SQL ERROR DBResult::getDataBlob()" << std::endl;
	size = 0;
	return NULL;
}

Database* _Database::_instance = NULL;

Database* _Database::instance(){
	if(!_instance){
#if defined __USE_MYSQL__ && defined __USE_SQLITE__
        if(g_config.getString(ConfigManager::SQL_TYPE) == "mysql"){
            _instance = new DatabaseMySQL;
		}
        else{
            _instance = new DatabaseSqLite;
		}
#elif defined __USE_MYSQL__
		_instance = new DatabaseMySQL;
#elif defined __USE_SQLITE__
		_instance = new DatabaseSqLite;
#endif
		OTSYS_THREAD_LOCKVARINIT(DBQuery::database_lock);
	}
	return _instance;
}

#ifndef __USE_MYSQL__
void escape_string(std::string & s)
{
	if (!s.size()) {
		return;
	}

	for (unsigned int i = 0; i < s.size(); i++) {
		switch (s[i]) {
			case '\0':		// Must be escaped for "mysql"
				s[i] = '\\';
				s.insert(i, "0", 1);
				i++;
				break;
			case '\n':		// Must be escaped for logs
				s[i] = '\\';
				s.insert(i, "n", 1);
				i++;
				break;
			case '\r':
				s[i] = '\\';
				s.insert(i, "r", 1);
				i++;
				break;
			case '\\':
				s[i] = '\\';
				s.insert(i, "\\", 1);
				i++;
				break;
			case '\"':
				s[i] = '\\';
				s.insert(i, "\"", 1);
				i++;
				break;
			case '\'':		// Better safe than sorry
				s[i] = '\\';
				s.insert(i, "\'", 1);
				i++;
				break;
			case '\032':	// This gives problems on Win32
				s[i] = '\\';
				s.insert(i, "Z", 1);
				i++;
				break;
			default:
				break;
		}
	}
}
#endif

std::string _Database::escapeString(const std::string &s)
{
    #ifdef __USE_MYSQL__
	return escapeString(s.c_str(), s.size());
	#else
	std::string r = std::string(s);
	escape_string(r);
	return r;
	#endif
}

// probably the mysql_escape_string version should be dropped as it's less generic
// but i'm keeping it atm
#ifdef __USE_MYSQL__
std::string _Database::escapeString(const char* s, unsigned long size)
{
	if(s == NULL)
		return std::string("");

	char* output = new char[size * 2 + 1];

	mysql_escape_string(output, s, size);
	std::string r = std::string(output);
	delete[] output;
	return r;
}
#else
std::string _Database::escapeString(const char* s, unsigned long size)
{
	if(s == NULL)
		return std::string("");

	std::string r = std::string(s);
	escape_string(r);
	return r;

}
#endif

DBTransaction::DBTransaction(Database* database)
{
	m_database = database;
	m_state = STATE_NO_START;
}

DBTransaction::~DBTransaction()
{
	if(m_state == STATE_START){
		if(!m_database->rollback()){
			//TODO: What to do here?
		}
	}
}
	
bool DBTransaction::start()
{
	DBQuery query;
	query << "START TRANSACTION;";
	if(m_database->executeQuery(query)){
		m_state = STATE_START;
		return true;
	}
	else{
		return false;
	}
}

bool DBTransaction::success()
{
	if(m_state == STATE_START){
		m_state = STEATE_COMMIT;
		return m_database->commit();
	}
	else{
		return false;
	}
}


DBSplitInsert::DBSplitInsert(Database* database)
{
	m_database = database;
}

DBSplitInsert::~DBSplitInsert()
{
	//
}

void DBSplitInsert::clear()
{
	m_buffer.clear();
	m_buffer.reserve(10240);
}

void DBSplitInsert::setQuery(const std::string& query)
{
	clear();
	m_query = query;
}

bool DBSplitInsert::addRow(const std::string& row)
{
#ifdef __SPLIT_QUERIES__
	
	m_buffer = row;
	bool ret = internalExecuteQuery();
	m_buffer = "";
	return ret;
	
#else
	int size = m_buffer.size();
	if(size == 0){
		m_buffer = row;
	}
	else if(size > 8192){
		if(!internalExecuteQuery()){
			return false;
		}
		m_buffer = row;
	}
	else{
		m_buffer += "," + row;
	}
	return true;
	
#endif
}
	

bool DBSplitInsert::internalExecuteQuery()
{
	DBQuery subquery;
	subquery << m_query;
	subquery << m_buffer;
	
	if(!m_database->executeQuery(subquery)){
		return false;
	}
	else{
		return true;
	}
}

bool DBSplitInsert::executeQuery()
{
	if(m_buffer.size() != 0){
		return internalExecuteQuery();
	}
	return true;
}


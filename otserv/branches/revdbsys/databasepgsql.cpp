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

#include "tools.h"
#include "databasepgsql.h"
#include "configmanager.h"

extern ConfigManager g_config;

DatabasePgSQL::DatabasePgSQL()
{
	m_connected = false;

	//load connection parameters
	m_dns = "host='" + g_config.getString(ConfigManager::SQL_HOST) + "' dbname='" + g_config.getString(ConfigManager::SQL_DB) + "' user='" + g_config.getString(ConfigManager::SQL_USER) + "' password='" + g_config.getString(ConfigManager::SQL_PASS) + "'";
}

DatabasePgSQL::~DatabasePgSQL()
{
	disconnect();
}

bool DatabasePgSQL::connect()
{
	if (m_connected == true)
		return true;

	m_handle = PQconnectdb(m_dns.c_str());
	m_connected = PQstatus(m_handle) == CONNECTION_OK;

	if (!m_connected) {
		std::cout << "Failed to connected to PostgreSQL database: " << PQerrorMessage(m_handle) << std::endl;
	}

	return m_connected;
}

bool DatabasePgSQL::disconnect()
{
	PQfinish(m_handle);
	m_connected = false;
	return true;
}

std::string DatabasePgSQL::prepareQuery(const std::string &origin)
{
	std::string ret = "";
	ret.reserve(origin.size());
	bool inString = false;
	uint8_t ch;
	for(int a = 0; a < origin.length(); a++){
		ch = origin[a];

		if(ch == '\\') {
			ret += "\\";
			a++;
			ret += origin[a];
			continue;
		}

		if(ch == '\'') {
			if(inString) {
				if(origin[a + 1] == '\'') {
					ret += "\\'";
					a++;
					continue;
				} else
					inString = false;
			} else
				inString = true;
		}

		if(ch == '`' && !inString)
			ch = '"';

		ret += ch;
	}
	return ret;
}

bool DatabasePgSQL::executeQuery(DBQuery& q)
{
	if (!m_connected)
		return false;

	std::string query = q.str();

	query = DatabasePgSQL::prepareQuery(query);

	PGresult* res;
	ExecStatusType stat;

	res = PQexec(m_handle, query.c_str() );
	stat = PQresultStatus(res);
	PQclear(res);

	if (stat != PGRES_COMMAND_OK && stat != PGRES_TUPLES_OK)
	{
		std::cout << "Failed to execute query: \"" << query << "\": " << PQresultErrorMessage(res) << "." << std::endl;
		return false;
	}

	q.reset();

	return true;
}

bool DatabasePgSQL::storeQuery(DBQuery& q, DBResult& res)
{
	if (!m_connected)
		return false;

	std::string query = q.str();

	query = DatabasePgSQL::prepareQuery(query);

	PGresult* _res;
	ExecStatusType _stat;

	_res = PQexec(m_handle, query.c_str() );
	_stat = PQresultStatus(_res);

	if (_stat != PGRES_TUPLES_OK)
	{
		if (_stat == PGRES_COMMAND_OK)
			std::cout << "Query \"" << query << "\" doesn't return results." << std::endl;
		else
			std::cout << "Failed to execute query: \"" << query << "\": " << PQresultErrorMessage(_res) << "." << std::endl;

		PQclear(_res);
		return false;
	}

	res.clear();
	int rows = PQntuples(_res);
	int fields = PQnfields(_res);
	int i;

	for(i = 0; i < fields; i++) {
		std::string name( PQfname(_res, i) );
		res.setFieldName(name, i);
	}
	
	char** row;

	for(i = 0; i < rows; i++) {
		row = new char*[fields];
		for(int j = 0; j < fields; j++)
			row[j] = PQgetvalue(_res, i, j);

		res.addRow(row, fields);
	}

	q.reset();
	return true;
}

bool DatabasePgSQL::beginTransaction()
{
	DBQuery query;
	query << "BEGIN;";

	return executeQuery(query);
}

bool DatabasePgSQL::rollback()
{
	DBQuery query;
	query << "ROLLBACK;";

	return executeQuery(query);
}

bool DatabasePgSQL::commit()
{
	DBQuery query;
	query << "COMMIT;";

	return executeQuery(query);
}

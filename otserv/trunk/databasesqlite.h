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

#ifndef __OTSERV_DatabaseSqLite_H__
#define __OTSERV_DatabaseSqLite_H__

#ifdef WIN32
//#include <winsock2.h>
#include <winsock.h>
#endif

#include <sqlite3.h>
#include <sstream>
#include "database.h"

class DatabaseSqLite : protected _Database
{
public:
	DatabaseSqLite();
	virtual ~DatabaseSqLite();

	/** Connect to a mysql DatabaseSqLite
	*\returns
	* 	TRUE if the connection is ok
	* 	FALSE if the connection fails
	*\param db_name The "name" of the DatabaseSqLite used
	*\param db_host The "host" to connect to
	*\param db_user The "username" used in the connection
	*\param db_pass The "password" of the username used
	*/
	virtual bool connect(const char *db_name, const char *db_host, const char *db_user, const char *db_pass);

	/** Disconnects from the connected DatabaseSqLite
	*\returns
	* 	TRUE if the DatabaseSqLite was disconnected
	* 	FALSE if the DatabaseSqLite was not disconnected or no DatabaseSqLite selected
	*/
	virtual bool disconnect();

	/** Execute a query which don't get any information of the DatabaseSqLite (for ex.: INSERT, UPDATE, etc)
	*\returns
	* 	TRUE if the query is ok
	* 	FALSE if the query fails
	*\ref q The query object
	*/
	virtual bool executeQuery(DBQuery &q);

	/** Store a query which get information of the DatabaseSqLite (for ex.: SELECT)
	*\returns
	* 	TRUE if the query is ok
	* 	FALSE if the query fails
	*\ref q The query object
	*\ref res The DBResult object where to insert the results of the query
	*/
	virtual bool storeQuery(DBQuery &q, DBResult &res);

private:

	/** initialize the DatabaseSqLite
	*\returns
	* 	TRUE if the DatabaseSqLite was successfully initialized
	* 	FALSE if the DatabaseSqLite was not successfully initialized
	*/
	bool init();
    static int callback(void *db, int argc, char **argv, char **azColName);

	bool m_initialized;
	bool m_connected;
	static bool m_fieldnames;
	char *zErrMsg;
	sqlite3 *m_handle;
};


#endif

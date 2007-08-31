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

//
// C++ Interface: databaseodbc
//
// Description: Frontend for ODBC connections
//
//
// Author: bruno <brfwolf@thormenta.com>, (C) 2007
//
//
#ifndef __OTSERV_DatabaseODBC_H__
#define __OTSERV_DatabaseODBC_H__

#include "database.h"

#ifdef WIN32
#include <windows.h>
#else
#include <sqltypes.h>
#endif

#include <sql.h>
#include <sqlext.h>

#define RETURN_SUCCESS(ret) ((ret == SQL_SUCCESS) || (ret == SQL_SUCCESS_WITH_INFO))

/**
	@author bruno <brfwolf@thormenta.com>
*/
class DatabaseODBC : public _Database
{
	public:
		DatabaseODBC();
		~DatabaseODBC();

		virtual bool connect();
		virtual bool disconnect();
		virtual bool executeQuery(DBQuery& q);
		virtual bool storeQuery(DBQuery& q, DBResult& res);
		virtual bool beginTransaction();
		virtual bool rollback();
		virtual bool commit();

	protected:
		void displayError(SQLHSTMT hStmt);
		SQLCHAR* prepareQuery(DBQuery& query);

		SQLHENV hEnv;
		SQLHDBC hDbc;
		bool hConnected;

		uint8_t *szDSN, *szUser, *szPass;
};

#endif

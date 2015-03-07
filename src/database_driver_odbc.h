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
// Author: Bruno R Ferreira <brf_coldf@yahoo.com.br>, (C) 2007
//
//

#ifdef __USE_ODBC__

#ifndef __OTSERV_DATABASEODBC_H__
#define __OTSERV_DATABASEODBC_H__

#include "database_driver.h"

#ifdef WIN32
#include <windows.h>
#else
#include <sqltypes.h>
#endif

#include <sql.h>
#include <sqlext.h>

/**
  @author Bruno R Ferreira <brf_coldf@yahoo.com.br>
*/
class DatabaseODBC : public DatabaseDriver
{
public:
  DatabaseODBC();
  virtual ~DatabaseODBC();

  virtual bool getParam(DBParam_t param);

  virtual bool beginTransaction();
  virtual bool rollback();
  virtual bool commit();

  virtual bool executeQuery(const std::string &query) { return false; }; // todo
  
  virtual uint64_t getLastInsertedRowID(){ return 0; }; // todo

  virtual std::string escapeString(const std::string &s);
  virtual std::string escapeBlob(const char* s, uint32_t length);

protected:
  virtual bool internalQuery(const std::string &query);
  virtual DBResult_ptr internalSelectQuery(const std::string &query);
  virtual void freeResult(DBResult *res);

  std::string _parse(const std::string &s);

  SQLHDBC m_handle;
  SQLHENV m_env;
};

class ODBCResult : public DBResult
{
  friend class DatabaseODBC;

public:
  virtual int32_t getDataInt(const std::string &s);
  virtual uint32_t getDataUInt(const std::string &s);
  virtual int64_t getDataLong(const std::string &s);
  virtual std::string getDataString(const std::string &s);
  virtual const char* getDataStream(const std::string &s, unsigned long &size);

  virtual DBResult_ptr advance();
  virtual bool empty();

protected:
  ODBCResult(SQLHSTMT stmt);
  virtual ~ODBCResult();

  typedef std::map<const std::string, uint32_t> listNames_t;
  listNames_t m_listNames;
  bool m_rowAvailable;

  SQLHSTMT m_handle;
};

#endif

#endif

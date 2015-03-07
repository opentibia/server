//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// various definitions needed by most files
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

#ifndef __OTSERV_DEFINITIONS_H__
#define __OTSERV_DEFINITIONS_H__


#define OTSERV_VERSION "0.7.0"
#define OTSERV_NAME "OTServ"
#define CURRENT_SCHEMA_VERSION 25

#define CLIENT_VERSION_MIN 870
#define CLIENT_VERSION_MAX 870
#define CLIENT_VERSION_STRING "8.70"

#ifdef __USE_SQLITE__
  #define SINGLE_SQL_DRIVER
#endif

#ifdef __USE_MYSQL__
  #ifdef SINGLE_SQL_DRIVER
    #define MULTI_SQL_DRIVERS
  #else
    #define SINGLE_SQL_DRIVER
  #endif
#endif

#ifdef __USE_ODBC__
  #ifdef SINGLE_SQL_DRIVER
    #define MULTI_SQL_DRIVERS
  #else
    #define SINGLE_SQL_DRIVER
  #endif
#endif

#ifdef __USE_PGSQL__
  #ifdef SINGLE_SQL_DRIVER
    #define MULTI_SQL_DRIVERS
  #else
    #define SINGLE_SQL_DRIVER
  #endif
#endif

//Default sql driver
#if !defined(SINGLE_SQL_DRIVER) && !defined(MULTI_SQL_DRIVERS)
  #define SINGLE_SQL_DRIVER
#endif

enum passwordType_t{
  PASSWORD_TYPE_PLAIN = 0,
  PASSWORD_TYPE_MD5,
  PASSWORD_TYPE_SHA1
};

// OpenTibia configuration
#if !defined(__NO_SKULLSYSTEM__) && !defined(__SKULLSYSTEM__)
  #define __SKULLSYSTEM__
#endif

#endif

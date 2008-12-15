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
#include "otpch.h"

#include "definitions.h"
#include "configmanager.h"
#include <iostream>

ConfigManager::ConfigManager()
{
	m_isLoaded = false;
}

ConfigManager::~ConfigManager()
{
	//
}

bool ConfigManager::loadFile(const std::string& _filename)
{
	lua_State* L = lua_open();

	if(!L) return false;

	if(luaL_dofile(L, _filename.c_str()))
	{
		lua_close(L);
		return false;
	}

	// parse config
	if(!m_isLoaded) // info that must be loaded one time (unless we reset the modules involved)
	{
		m_confString[CONFIG_FILE] = _filename;
		m_confString[IP] = getGlobalString(L, "ip", "127.0.0.1");
		m_confInteger[PORT] = getGlobalNumber(L, "port");

#if defined __CONFIG_V2__
		unsigned int pos = _filename.rfind("/");
		std::string configPath = "";
		if(pos != std::string::npos)
			configPath = _filename.substr(0, pos+1);

		m_confString[DATA_DIRECTORY] = configPath + getGlobalString(L, "datadir", "data/");
		m_confString[MAP_FILE] = m_confString[DATA_DIRECTORY] + getGlobalString(L, "map");
		m_confString[MAP_STORE_FILE] = m_confString[DATA_DIRECTORY] + getGlobalString(L, "mapstore");
		m_confString[HOUSE_STORE_FILE] = m_confString[DATA_DIRECTORY] + getGlobalString(L, "housestore");
#else
		m_confString[DATA_DIRECTORY] = getGlobalString(L, "datadir");
		m_confString[MAP_FILE] = getGlobalString(L, "map");
		m_confString[MAP_STORE_FILE] = getGlobalString(L, "mapstore");
		m_confString[HOUSE_STORE_FILE] = getGlobalString(L, "housestore");
#endif
		m_confString[HOUSE_RENT_PERIOD] = getGlobalString(L, "houserentperiod", "monthly");
		m_confString[MAP_KIND] = getGlobalString(L, "mapkind");
		if(getGlobalString(L, "md5passwords") != ""){
			std::cout << "Warning: [ConfigManager] md5passwords is deprecated. Use passwordtype instead." << std::endl;
		}
		m_confString[PASSWORD_TYPE_STR] = getGlobalString(L, "passwordtype");
		m_confString[WORLD_TYPE] = getGlobalString(L, "worldtype");
		m_confString[SQL_HOST] = getGlobalString(L, "sql_host");
		m_confString[SQL_USER] = getGlobalString(L, "sql_user");
		m_confString[SQL_PASS] = getGlobalString(L, "sql_pass");
		m_confString[SQL_DB] = getGlobalString(L, "sql_db");
		m_confString[SQL_TYPE] = getGlobalString(L, "sql_type");
		m_confString[MAP_HOST] = getGlobalString(L, "map_host");
		m_confString[MAP_USER] = getGlobalString(L, "map_user");
		m_confString[MAP_PASS] = getGlobalString(L, "map_pass");
		m_confString[MAP_DB] = getGlobalString(L, "map_db");
		m_confInteger[SQL_PORT] = getGlobalNumber(L, "sql_port");
	}

	m_confString[LOGIN_MSG] = getGlobalString(L, "loginmsg", "Welcome.");
	m_confString[SERVER_NAME] = getGlobalString(L, "servername");
	m_confString[WORLD_NAME] = getGlobalString(L, "worldname", "OpenTibia");
	m_confString[OWNER_NAME] = getGlobalString(L, "ownername");
	m_confString[OWNER_EMAIL] = getGlobalString(L, "owneremail");
	m_confString[URL] = getGlobalString(L, "url");
	m_confString[LOCATION] = getGlobalString(L, "location");
	m_confInteger[LOGIN_TRIES] = getGlobalNumber(L, "logintries", 3);
	m_confInteger[RETRY_TIMEOUT] = getGlobalNumber(L, "retrytimeout", 30 * 1000);
	m_confInteger[LOGIN_TIMEOUT] = getGlobalNumber(L, "logintimeout", 5 * 1000);
	m_confString[MOTD] = getGlobalString(L, "motd");
	m_confInteger[MOTD_NUM] = getGlobalNumber(L, "motdnum");
	m_confInteger[MAX_PLAYERS] = getGlobalNumber(L, "maxplayers");
	m_confInteger[EXHAUSTED] = getGlobalNumber(L, "exhausted", 1000);
	m_confInteger[EXHAUSTED_ADD] = getGlobalNumber(L, "exhaustedadd", 0);
	m_confInteger[FIGHTEXHAUSTED] = getGlobalNumber(L, "fightexhausted", 2000);
	m_confInteger[HEALEXHAUSTED] = getGlobalNumber(L, "healexhausted", 1000);
	m_confInteger[PZ_LOCKED] = getGlobalNumber(L, "pzlocked", 60 * 1000);
	m_confInteger[FIELD_OWNERSHIP_DURATION] = getGlobalNumber(L, "field_ownership_duration", 5 * 1000);
	m_confInteger[MIN_ACTIONTIME] = getGlobalNumber(L, "minactioninterval", 200);
	m_confInteger[MIN_ACTIONEXTIME] = getGlobalNumber(L, "minactionexinterval", 1000);
	m_confInteger[DEFAULT_DESPAWNRANGE] = getGlobalNumber(L, "despawnrange", 2);
	m_confInteger[DEFAULT_DESPAWNRADIUS] = getGlobalNumber(L, "despawnradius", 50);
	m_confInteger[ALLOW_CLONES] = getGlobalNumber(L, "allowclones", 0);
	m_confInteger[PARTY_MEMBER_EXP_BONUS] = getGlobalNumber(L, "party_exp_mul", 5);
	m_confInteger[RATE_EXPERIENCE] = getGlobalNumber(L, "rate_exp", 1);
	m_confInteger[RATE_SKILL] = getGlobalNumber(L, "rate_skill", 1);
	m_confInteger[RATE_LOOT] = getGlobalNumber(L, "rate_loot", 1);
	m_confInteger[RATE_MAGIC] = getGlobalNumber(L, "rate_mag", 1);
	m_confInteger[RATE_SPAWN] = getGlobalNumber(L, "rate_spawn", 1);
	m_confInteger[HOTKEYS] = getGlobalNumber(L, "enablehotkeys", 0);
	m_confInteger[MAX_MESSAGEBUFFER] = getGlobalNumber(L, "maxmessagebuffer", 4);
	m_confInteger[SAVE_CLIENT_DEBUG_ASSERTIONS] = getGlobalNumber(L, "saveclientdebug", 0);
	m_confInteger[CHECK_ACCOUNTS] = getGlobalNumber(L, "checkaccounts", 0);
	m_confString[USE_ACCBALANCE] = getGlobalString(L, "useaccbalance", "no");
	m_confInteger[PREMIUM_ONLY_BEDS] = (getGlobalString(L, "premonlybeds", "yes") == "yes");

	m_confString[OTSERV_DB_HOST] = getGlobalString(L, "otserv_db_host", "default_db_host_here");
	m_confInteger[OTSERV_DB_ENABLED] = getGlobalNumber(L, "otserv_db_enabled", 0);
	m_confInteger[PASSWORD_TYPE] = PASSWORD_TYPE_PLAIN;
	m_confInteger[STATUSQUERY_TIMEOUT] = getGlobalNumber(L, "statustimeout", 5 * 60 * 1000);
	m_isLoaded = true;

	lua_close(L);
	return true;
}

bool ConfigManager::reload()
{
	if(!m_isLoaded)
		return false;

	return loadFile(m_confString[CONFIG_FILE]);
}

const std::string& ConfigManager::getString(uint32_t _what) const
{
	if(m_isLoaded && _what < LAST_STRING_CONFIG)
		return m_confString[_what];
	else
	{
		std::cout << "Warning: [ConfigManager::getString] " << _what << std::endl;
		return m_confString[DUMMY_STR];
	}
}

int ConfigManager::getNumber(uint32_t _what) const
{
	if(m_isLoaded && _what < LAST_INTEGER_CONFIG)
		return m_confInteger[_what];
	else
	{
		std::cout << "Warning: [ConfigManager::getNumber] " << _what << std::endl;
		return 0;
	}
}
bool ConfigManager::setNumber(uint32_t _what, int _value)
{
	if(m_isLoaded && _what < LAST_INTEGER_CONFIG)
	{
		m_confInteger[_what] = _value;
		return true;
	}
	else
	{
		std::cout << "Warning: [ConfigManager::setNumber] " << _what << std::endl;
		return false;
	}
}

std::string ConfigManager::getGlobalString(lua_State* _L, const std::string& _identifier, const std::string& _default)
{
	lua_getglobal(_L, _identifier.c_str());

	if(!lua_isstring(_L, -1))
		return _default;

	int len = (int)lua_strlen(_L, -1);
	std::string ret(lua_tostring(_L, -1), len);
	lua_pop(_L,1);

	return ret;
}

int ConfigManager::getGlobalNumber(lua_State* _L, const std::string& _identifier, const int _default)
{
	lua_getglobal(_L, _identifier.c_str());

	if(!lua_isnumber(_L, -1))
		return _default;

	int val = (int)lua_tonumber(_L, -1);
	lua_pop(_L,1);

	return val;
}

std::string ConfigManager::getGlobalStringField (lua_State* _L, const std::string& _identifier, const int _key, const std::string& _default) {
	lua_getglobal(_L, _identifier.c_str());

	lua_pushnumber(_L, _key);
	lua_gettable(_L, -2);  /* get table[key] */
	if(!lua_isstring(_L, -1))
		return _default;
	std::string result = lua_tostring(_L, -1);
	lua_pop(_L, 2);  /* remove number and key*/
	return result;
}

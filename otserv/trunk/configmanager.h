#ifndef _CONFIG_MANAGER_H
#define _CONFIG_MANAGER_H

#include <string>
#include <list>

extern "C"
{
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}


class ConfigManager {
public:
	ConfigManager();
	~ConfigManager();
	
	enum string_config_t {
		CONFIG_FILE = 0,
		DATA_DIRECTORY,
		MAP_FILE,
		MAP_STORE_FILE,
		HOUSE_STORE_FILE,
		HOUSE_RENT_PERIOD,
		MAP_KIND,
		BAN_FILE,
		LOGIN_MSG,
		SERVER_NAME,
		OWNER_NAME,
		OWNER_EMAIL,
		URL,
		LOCATION,
		IP,
		MOTD,
		MD5_PASS,
		WORLD_TYPE,
		SQL_HOST,
		SQL_USER,
		SQL_PASS,
		SQL_DB,
		MAP_HOST,
		MAP_USER,
		MAP_PASS,
		MAP_DB,
		LAST_STRING_CONFIG /* this must be the last one */
	};
	
	enum integer_config_t {
		LOGIN_TRIES = 0,
		RETRY_TIMEOUT,
		LOGIN_TIMEOUT,
		PORT,
		MOTD_NUM,
		MAX_PLAYERS,
		EXHAUSTED,
		EXHAUSTED_ADD,
		PZ_LOCKED,
		ALLOW_CLONES,
		RATE_EXPERIENCE,
		RATE_SKILL,
		RATE_LOOT,
		RATE_MAGIC,
		LAST_INTEGER_CONFIG /* this must be the last one */
	};
	
	
	bool loadFile(const std::string& _filename);
	bool reload();
	std::string getString(int _what) { return m_confString[_what]; };
	int getNumber(int _what);
	std::string getVocationString(int _vocation) { return m_confVocationString[_vocation-1]; };
	
	bool setNumber(int _what, int _value);
	
private:
	std::string getGlobalString(lua_State* _L, const std::string& _identifier, const std::string& _default="");
	int getGlobalNumber(lua_State* _L, const std::string& _identifier, const int _default=0);
	std::string getGlobalStringField(lua_State* _L, const std::string& _identifier, const int _key, const std::string& _default="");
	
	bool m_isLoaded;
	std::string m_confString[LAST_STRING_CONFIG];
	int m_confInteger[LAST_INTEGER_CONFIG];
	std::string m_confVocationString[4];
};


#endif /* _CONFIG_MANAGER_H */

//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Player Loader/Saver based on MySQL
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
#ifndef __IOPLAYERSQL_H
#define __IOPLAYERSQL_H

#include <string>

#include "ioplayer.h"
#include "player.h"

class Database;

/** Baseclass for all Player-Loaders */
class IOPlayerSQL : protected IOPlayer{
public:
	/** Get a textual description of what source is used
	* \returns Name of the source*/
	virtual char* getSourceDescription(){return "Player source: SQL";};
	virtual bool loadPlayer(Player* player, std::string name);

	/** Save a player
	* \returns Wheter the player was successfully saved
	* \param player the player to save
	*/
	virtual bool savePlayer(Player* player);
	
	virtual bool getGuidByName(unsigned long& guid, unsigned long& alvl, std::string& name);
	virtual bool getNameByGuid(unsigned long guid, std::string &name);
	virtual bool getGuilIdByName(unsigned long& guildId, const std::string& guildName);
	virtual bool playerExists(std::string name);

	IOPlayerSQL();
	~IOPlayerSQL(){};

protected:
	std::string getItems(Item* i, int &startid, int startslot, int player, int parentid);
	bool storeNameByGuid(Database &mysql, unsigned long guid);

	typedef std::map<unsigned long, std::string> NameCacheMap;
	
	NameCacheMap nameCacheMap;
	
	std::string m_host;
	std::string m_user;
	std::string m_pass;
	std::string m_db;
};

#endif

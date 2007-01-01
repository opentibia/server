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
#ifndef __OTSERV_IOPLAYERSQL_H__
#define __OTSERV_IOPLAYERSQL_H__

#include <string>

#include "ioplayer.h"
#include "player.h"
#include "database.h"

class PlayerGroup
{
public:
	PlayerGroup(){};
	~PlayerGroup(){};
	
	std::string m_name;
	uint32_t m_flags;
	uint32_t m_access;
	uint32_t m_maxDepotItems;
	uint32_t m_maxVip;
};

typedef std::pair<int32_t, Item*> itemBlock;
typedef std::list<itemBlock> ItemBlockList;

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
	
	virtual bool getGuidByName(unsigned long& guid, std::string& name);
	virtual bool getGuidByNameEx(unsigned long &guid, unsigned long &alvl, std::string& name);
	virtual bool getNameByGuid(unsigned long guid, std::string& name);
	virtual bool getGuildIdByName(unsigned long& guildId, const std::string& guildName);
	virtual bool playerExists(std::string name);

	IOPlayerSQL();
	~IOPlayerSQL(){};

protected:
	bool storeNameByGuid(Database &mysql, unsigned long guid);
	
	const PlayerGroup* getPlayerGroup(uint32_t groupid);

	struct StringCompareCase{
		bool operator()(const std::string& l, const std::string& r) const{
			if(strcasecmp(l.c_str(), r.c_str()) < 0){
				return true;
			}
			else{
				return false;
			}
		}
	};

	typedef std::map<int,std::pair<Item*,int> > ItemMap;

	void loadItems(ItemMap& itemMap, DBResult& result);
	bool saveItems(Player* player, const ItemBlockList& itemList, DBSplitInsert& query_insert);

	typedef std::map<unsigned long, std::string> NameCacheMap;
	typedef std::map<std::string, unsigned long, StringCompareCase> GuidCacheMap;
	typedef std::map<uint32_t, PlayerGroup*> PlayerGroupMap;
	
	PlayerGroupMap playerGroupMap;
	NameCacheMap nameCacheMap;
	GuidCacheMap guidCacheMap;
};

#endif

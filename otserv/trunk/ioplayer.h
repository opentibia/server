//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Base class for the Player Loader/Saver
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

#ifndef __IOPLAYER_H
#define __IOPLAYER_H

#include <string>

#include "player.h"
#include "database.h"

struct PlayerGroup
{
	std::string name;
	uint64_t flags;
	uint16_t access, violation;
	uint32_t maxDepotItems, maxVip;
};

typedef std::pair<int32_t, Item*> itemBlock;
typedef std::list<itemBlock> ItemBlockList;

/** Class responsible for loading players from database. */
class IOPlayer {
public:
	IOPlayer() {}
	~IOPlayer() {}

	static IOPlayer* instance()
	{
		static IOPlayer instance;
		return &instance;
	}

	/** Load a player
	  * \param player Player structure to load to
	  * \param name Name of the player
	  * \param preload if set to true only group, guid and account id will be loaded, default: false
	  * \return returns true if the player was successfully loaded
	  */
	bool loadPlayer(Player* player, const std::string& name, bool preload = false);

	/** Save a player
	  * \param player the player to save
	  * \return true if the player was successfully saved
	  */
	bool savePlayer(Player* player, bool shallow = false);

	bool addPlayerDeath(Player* dying_player, const DeathList& dl);

	bool getGuidByName(uint32_t& guid, std::string& name);
	bool getAccountByName(uint32_t& acc, std::string& name);
	bool getAccountByName(std::string& acc, std::string& name);
	bool getGuidByNameEx(uint32_t& guid, bool& specialVip, std::string& name);
	bool getDefaultTown(std::string& name, uint32_t& townId);
	bool getNameByGuid(uint32_t guid, std::string& name);
	bool getGuildIdByName(uint32_t& guildId, const std::string& guildName);
	bool playerExists(std::string name);
	bool getLastIP(uint32_t& ip, uint32_t guid);
	bool hasFlag(PlayerFlags flag, uint32_t guid);
	void updateLoginInfo(Player* player);
	void updateLogoutInfo(Player* player);
	bool cleanOnlineInfo();

protected:
	bool storeNameByGuid(Database &mysql, uint32_t guid);

	const PlayerGroup getPlayerGroup(uint32_t groupid);

	struct StringCompareCase
	{
		bool operator()(const std::string& l, const std::string& r) const
		{
			return strcasecmp(l.c_str(), r.c_str()) < 0;
				return true;
		}
	};

	typedef std::map<int,std::pair<Item*,int> > ItemMap;

	void loadItems(ItemMap& itemMap, DBResult* result);
	bool saveItems(Player* player, const ItemBlockList& itemList, DBInsert& query_insert);

	typedef std::map<uint32_t, std::string> NameCacheMap;
	typedef std::map<std::string, uint32_t, StringCompareCase> GuidCacheMap;
	typedef std::map<uint32_t, PlayerGroup> PlayerGroupMap;

	PlayerGroupMap playerGroupMap;
	NameCacheMap nameCacheMap;
	GuidCacheMap guidCacheMap;
};

#endif

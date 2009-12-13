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

#ifndef __OTSERV_GUILD_H__
#define __OTSERV_GUILD_H__

#include "definitions.h"
#include "const.h"

#include <string>
#include <map>
#include <set>

struct GuildWar {
	uint32_t guildId;
	uint32_t opponentId;
	uint16_t killLimit;
	uint16_t guildKills;
	uint16_t opponentKills;
	uint32_t guildFee;
	uint32_t enemyGuildFee;
	//time_t duration;
};

typedef std::map<uint32_t, GuildWar* > GuildWarsMap;
typedef std::set<uint32_t> GuildSet;

class Guild
{
public:
	Guild();
	~Guild(){};

	void setAtWar();
	bool isPlayerEnemy(uint32_t _guildId) const;
	bool isGuildAtWar() const;
	bool isGuildEnemy(uint32_t _guildId) const;
	void endWar();

	void setGuildName(const std::string& _guildName) { guildName = _guildName; }
	void setGuildRank(const std::string& _guildRank) { guildRank = _guildRank; }
	void setGuildNick(const std::string& _guildNick) { guildNick = _guildNick; }
	void setGuildLevel(uint32_t _guildLevel) { guildLevel = _guildLevel; }
	void setGuildId(uint32_t _guildId) { guildId = _guildId; }

	std::string getGuildName() const { return guildName; }
	std::string getGuildRank() const { return guildRank; }
	std::string getGuildNick() const { return guildNick; }
	uint32_t getGuildLevel() const { return guildLevel; }
	uint32_t getGuildId() const { return guildId; }

protected:
	std::string guildName;
	std::string guildRank;
	std::string guildNick;
	uint32_t guildLevel;
	uint32_t guildId;
	GuildSet enemyGuilds;
	std::set<uint32_t> warIds;
};

class Guilds
{
public:
	Guilds();
	~Guilds(){};

	bool isGuildAtWar(uint32_t _guildId) const;
	void setGuildsAtWar();
	bool clearWar(uint32_t _warId) const;

	GuildWarsMap guildWars;
	GuildSet guildsAtWar;
};

#endif

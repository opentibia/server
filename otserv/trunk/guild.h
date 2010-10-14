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
#include "creature.h"
#include <string>
#include <map>

class Player;

struct GuildWar {
	uint32_t guildId;
	uint32_t opponentId;
	uint32_t guildFrags;
	uint32_t opponentFrags;
	uint32_t guildFee;
	uint32_t opponentFee;
	uint32_t fragLimit;
	bool finished;
};

typedef std::map<uint32_t, GuildWar> GuildWarsMap;

class Guild
{
public:
	Guild();
	~Guild();

	void setId(uint32_t _id){ id = _id; }
	void setName(std::string _name){ name = _name; }

	uint32_t getId() const { return id; }
	std::string getName() const { return name; }

	bool addFrag(uint32_t enemyId) const;
	bool isAtWar() const { return !enemyGuilds.empty(); }
	bool hasDeclaredWar(uint32_t warId) const;
	void broadcastMessage(SpeakClasses type, const std::string& msg) const;

	uint32_t isEnemy(uint32_t enemyId) const;
	void addEnemy(uint32_t enemyId, uint32_t warId);
	
protected:
	uint32_t id;
	std::string name;
	
	typedef std::map<uint32_t, uint32_t> EnemyGuildsMap; //enemy guild id, war id
	EnemyGuildsMap enemyGuilds;
};

class Guilds
{
public:
	void loadWars();
	#ifdef __GUILDWARSLUARELOAD__
	bool loadWar(uint32_t warId);
	#endif
	void endWar(uint32_t warId);
	bool transferMoney(uint32_t guildId, uint32_t opponentId, int32_t guildFee, int32_t opponentFee);
	bool setWarStatus(uint32_t warId, int32_t statusId);
	void broadcastKill(uint32_t guildId, Player* player, const DeathList& killers);
	GuildWarsMap& getWars() { return guildWars; }

	Guild* getGuildById(uint32_t guildId);
	bool getGuildIdByName(uint32_t& guildId, const std::string& guildName);

protected:
	typedef std::map<uint32_t, Guild*> GuildsMap; //guild id, guild class
	GuildsMap loadedGuilds;
	GuildWarsMap guildWars;
};

#endif

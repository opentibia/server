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

#include "guild.h"
#include "game.h"
#include "database.h"

extern Game g_game;

Guild::Guild()
{
	id = 0;
	name = "";
}

Guild::~Guild()
{
	clearEnemies();
}

void Guild::addFrag(uint32_t enemyId) const
{
	uint32_t warId = isEnemy(enemyId);
	if(warId != 0){
		GuildWarsMap::iterator it = g_game.getGuildWars().find(warId);
		if(it != g_game.getGuildWars().end()){
			Database* db = Database::instance();
			DBQuery query;
			query << "UPDATE `guild_wars` SET ";

			uint32_t frags;
			if(hasDeclaredWar(warId)){
				frags = ++it->second.guildFrags;
				query << "`guild_frags` ";
			}
			else{
				frags = ++it->second.opponentFrags;
				query << "`opponent_frags` ";
			}

			query << "= " << frags << " WHERE `id` = " << warId;
			db->executeQuery(query.str());

			if(frags >= it->second.fragLimit){
				g_game.endGuildWar(warId);
			}
		}
	}
}

bool Guild::hasDeclaredWar(uint32_t warId) const
{
	GuildWarsMap::iterator it = g_game.getGuildWars().find(warId);
	if(it != g_game.getGuildWars().end()){
		if(it->second.guildId == getId()){
			return true;
		}
	}

	return false;
}

void Guild::addEnemy(uint32_t guildId, uint32_t warId)
{
	if(isEnemy(guildId) == 0){
		enemyGuilds[guildId] = warId;
	}
}

uint32_t Guild::isEnemy(uint32_t guildId) const
{
	EnemyGuildsMap::const_iterator it = enemyGuilds.find(guildId);
	if(it != enemyGuilds.end()){
		if(it->first == guildId){
			return it->second;
		}
	}

	return 0;
}

void Guild::removeEnemy(uint32_t enemyId)
{
	EnemyGuildsMap::iterator it = enemyGuilds.find(enemyId);
	if(it != enemyGuilds.end()){
		enemyGuilds.erase(it);
	}
}

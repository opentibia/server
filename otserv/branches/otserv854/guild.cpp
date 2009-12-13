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
#include "database.h"

extern Guilds g_guilds;

Guild::Guild()
{
	guildName = "";
	guildRank = "";
	guildNick = "";
	guildLevel= 0;
	guildId = 0;
}

void Guild::setAtWar()
{
	if(g_guilds.isGuildAtWar(guildId)){
		GuildWarsMap warMap = g_guilds.guildWars;
		GuildWarsMap::iterator it;
		for(it = warMap.begin(); it != warMap.end(); ++it){
			if(it->second->guildId == guildId){
				enemyGuilds.insert(it->second->guildId);
				warIds.insert(it->first);
			}
			else if(it->second->opponentId == guildId){
				enemyGuilds.insert(it->second->opponentId);
				warIds.insert(it->first);
			}
		}
	}
}

bool Guild::isGuildAtWar() const
{
	return enemyGuilds.size() != 0;
}

bool Guild::isGuildEnemy(uint32_t _guildId) const
{
	if(enemyGuilds.find(_guildId) != enemyGuilds.end()){
		return true;
	}
	return false;
}

void Guild::endWar()
{

}

Guilds::Guilds()
{
	//
}

void Guilds::setGuildsAtWar()
{
	Database* db = Database::instance();
	DBResult* result;
	DBQuery query;

	query << "SELECT `war_declaration`.`id` as `wid`, `war_declaration`.`guild_id` as `guild_id`"
		  << ", `war_declaration`.`opponent_id` as `opponent_id`, `war_declaration`.`frag_limit` as `frag_limit`"
		  << ", `war_declaration`.`guild_fee` as `guild_fee`, `war_declaration`.`opponent_fee` as `opponent_fee`"
		  << ", `guild_wars`.`guild_frags` as `guild_frags`, `guild_wars`.`opponent_frags` as `opponent_frags`"
		  << " FROM `war_declaration`, `guild_wars`"
		  << " WHERE `war_declaration`.`active` = 1 AND `war_declaration`.`id` = `guild_wars`.`war_id`";
	if((result = db->storeQuery(query.str()))){
		do{
			uint32_t wid = result->getDataInt("wid");
			guildWars[wid] = new GuildWar();

			guildWars[wid]->guildId = result->getDataInt("guild_id");
			guildsAtWar.insert(result->getDataInt("guild_id"));
			guildWars[wid]->opponentId = result->getDataInt("opponent_id");
			guildsAtWar.insert(result->getDataInt("opponent_id"));

			guildWars[wid]->killLimit = result->getDataInt("frag_limit");
			guildWars[wid]->guildKills = result->getDataInt("guild_frags");
			guildWars[wid]->opponentKills = result->getDataInt("opponent_frags");
			guildWars[wid]->guildFee = result->getDataInt("guild_fee");
			guildWars[wid]->enemyGuildFee = result->getDataInt("opponent_fee");
		}while(result->next());
		db->freeResult(result);
	}
}

bool Guilds::isGuildAtWar(uint32_t _guildId) const
{
	if(guildsAtWar.find(_guildId) != guildsAtWar.end()){
		return true;
	}
	return false;
}

bool Guilds::clearWar(uint32_t _warId) const
{
	Database* db = Database::instance();
	DBQuery query;

	query << "DELETE FROM `guild_war`, `war_declaration`"
		  << " WHERE `guild_war`.`war_id` = " << _warId
		  << " AND `war_declaration`.`id` = " << _warId;
	return db->executeQuery(query.str());
}

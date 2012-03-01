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
#include "chat.h"
#include "player.h"
#include "configmanager.h"
#include "ioplayer.h"
#include <boost/algorithm/string/predicate.hpp>

extern Game g_game;
extern Chat g_chat;
extern ConfigManager g_config;
extern Guilds g_guilds;

void Guilds::loadWars()
{
	Database* db = Database::instance();
	DBResult* result;
	DBQuery query;

	query << "SELECT `id`, `guild_id`, `opponent_id`, `frag_limit`, `end_date`, `status`, \
		`guild_fee`, `opponent_fee`, `guild_frags`, `opponent_frags` FROM `guild_wars` WHERE `status` >= 0";

	if((result = db->storeQuery(query.str()))){
		//loop through data and processes the wars
		do{
			processWar(result, true);
		}while(result->next());

		//free database
		db->freeResult(result);
	}
}

int32_t Guilds::updateWar(uint32_t warId)
{
	Database* db = Database::instance();
	DBResult* result;
	DBQuery query;

	//Load the war
	query << "SELECT `id`, `guild_id`, `opponent_id`, `frag_limit`, `end_date`, `status`, \
		`guild_fee`, `opponent_fee`, `guild_frags`, `opponent_frags` FROM `guild_wars` WHERE `id` = " << warId;

	if((result = db->storeQuery(query.str()))){
		//process the war
		GuildWarsMap::iterator it = guildWars.find(warId);
		bool loadWar = (it == guildWars.end());
		int32_t status = processWar(result, loadWar);
		db->freeResult(result);

		//return new status
		return status;
	}

	return -1;
}

int32_t Guilds::processWar(DBResult* result, bool loadWar)
{
	//Get data from database result
	uint32_t id = result->getDataInt("id");
	int32_t endDate = result->getDataInt("end_date");
	int32_t status = result->getDataInt("status");
	int32_t oldStatus = status;

	//war struct
	GuildWar war;
	war.guildId = result->getDataInt("guild_id");
	war.opponentId = result->getDataInt("opponent_id");
	war.guildFrags = result->getDataInt("guild_frags");
	war.opponentFrags = result->getDataInt("opponent_frags");
	war.guildFee = result->getDataInt("guild_fee");
	war.opponentFee = result->getDataInt("opponent_fee");
	war.fragLimit = result->getDataInt("frag_limit");
	war.finished = false;

	//process data
	if(status == 1 && (endDate <= std::time(NULL) ||
		(war.fragLimit > 0 && (war.guildFrags >= war.fragLimit || war.opponentFrags >= war.fragLimit)))){
		//We need to end the war
		if(loadWar)
			endWar(war);
		else
			endWar(id);

		//This is the status of an ended war
		status = 4;
	}
	else if(status == 0 && endDate > std::time(NULL) && 
		war.guildFrags < war.fragLimit && war.opponentFrags < war.fragLimit){
		//Start the war if money can be transfered
		if(transferMoney(war.guildId, war.opponentId, (war.guildFee + g_config.getNumber(ConfigManager::GUILD_WAR_FEE)), (war.opponentFee + g_config.getNumber(ConfigManager::GUILD_WAR_FEE))))
			status = 1;
	}

	//Add guilds to each other's enemy list if war was activated or if it didn't finish yet
	//Also change war status in database if it has changed
	if(status == 1 && loadWar){
		Guild* guild = getGuildById(war.guildId);
		Guild* opponentGuild = getGuildById(war.opponentId);
		if(guild && opponentGuild){
			guildWars[id] = war;
			guild->addEnemy(opponentGuild->getId(), id);
			opponentGuild->addEnemy(guild->getId(), id);
		}
	}

	//Update status
	if(status != oldStatus)
		setWarStatus(id, status);

	return status;
}

void Guilds::unsetEnemies(uint32_t warId)
{
	GuildWarsMap::iterator it = guildWars.find(warId);
	if(it != guildWars.end()){
		//Get guilds
		Guild* guild = getGuildById(it->second.guildId);
		Guild* opponentGuild = getGuildById(it->second.opponentId);

		//Remove from enemy lists
		if(guild && opponentGuild){
			guild->removeEnemy(opponentGuild->getId());
			opponentGuild->removeEnemy(guild->getId());
		}
	}
}

bool Guilds::updateWarEmblems(uint32_t warId)
{
	GuildWarsMap::iterator wit = guildWars.find(warId);
	if(wit == guildWars.end())
		return false;

	uint32_t guild1 = wit->second.guildId;
	uint32_t guild2 = wit->second.opponentId;

	for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it){
		if(it->second->isRemoved())
			continue;

		if(it->second->getGuildId() == guild1 || it->second->getGuildId() == guild2)
			g_game.updateCreatureEmblem(it->second);
	}

	return true;
}

void Guilds::endWar(const GuildWar& war)
{
	int32_t realGuildFee = 0, realOpponentFee = 0;
	if(war.guildFrags >= war.fragLimit)
		realGuildFee = war.guildFee + war.opponentFee;
	else if(war.opponentFrags >= war.fragLimit)
		realOpponentFee = war.guildFee + war.opponentFee;
	else if(war.guildFrags == war.opponentFrags){ //We've got a tie - return the money
		realGuildFee = war.guildFee;
		realOpponentFee = war.opponentFee;
	}
	//Get proportional values positiveFrags/totalFrags in enemy's fee
	else if(war.guildFrags > war.opponentFrags){
		realGuildFee = (int32_t)std::ceil((double)((war.guildFrags - war.opponentFrags) / war.fragLimit) * war.opponentFee);
		realOpponentFee = war.opponentFee - realGuildFee;
		realGuildFee += war.guildFee;
	}
	else if(war.opponentFrags > war.guildFrags){
		realOpponentFee = (int32_t)std::ceil((double)((war.opponentFrags - war.guildFrags) / war.fragLimit) * war.guildFee);
		realGuildFee = war.guildFee - realOpponentFee;
		realOpponentFee += war.opponentFee;
	}

	//Do payment
	transferMoney(war.guildId, war.opponentId, -realGuildFee, -realOpponentFee);
}

void Guilds::endWar(uint32_t warId)
{
	GuildWarsMap::iterator it = guildWars.find(warId);
	if(it != guildWars.end()){
		//This method is for already-loaded wars, so besides transfering the money,
		//we should also unset guild enemies and update the war emblems.
		endWar(it->second);
		unsetEnemies(warId);
		updateWarEmblems(warId);
		guildWars.erase(it);
	}
}

bool Guilds::transferMoney(uint32_t guildId, uint32_t opponentId, int32_t guildFee, int32_t opponentFee)
{
	//Tries to get first leader that has enough money
	Database* db = Database::instance();
	DBResult* result;
	DBQuery query;

	bool guildPaid = false, opponentPaid = false;
	Player* guildLeader = NULL;
	Player* opponentLeader = NULL;

#ifdef __OLD_GUILD_SYSTEM__
	query << "SELECT `owner_id` FROM `guilds` WHERE `id` = " << guildId;
	if((result = db->storeQuery(query.str()))){
		if(Player* player = g_game.getPlayerByGuidEx(result->getDataInt("owner_id"))){
			if((int32_t)player->balance >= guildFee){
				guildPaid = true;
				guildLeader = player;
			}
		}
	}

	query.str("");
	query << "SELECT `owner_id` FROM `guilds` WHERE `id` = " << opponentId;
	if((result = db->storeQuery(query.str()))){
		if(Player* player = g_game.getPlayerByGuidEx(result->getDataInt("owner_id"))){
			if((int32_t)player->balance >= opponentFee){
				opponentPaid = true;
				opponentLeader = player;
			}
		}
	}

	db->freeResult(result);
#else
	query << "SELECT `guild_members`.`player_id`, `guild_ranks`.`guild_id` \
		FROM `guild_members` \
		LEFT JOIN `guild_ranks` ON `guild_members`.`rank_id` = `guild_ranks`.`id` \
		WHERE (`guild_ranks`.`guild_id` = " << guildId << " OR `guild_ranks`.`guild_id` = " << opponentId << ") \
		AND `guild_ranks`.`level` >= 3";

	if((result = db->storeQuery(query.str()))){
		do{
			uint32_t gid = result->getDataInt("guild_id");
			bool isOpponent = (gid == opponentId);

			if((!isOpponent && guildPaid) || (isOpponent && opponentPaid))
				continue;

			if(Player* player = g_game.getPlayerByGuidEx(result->getDataInt("player_id"))){
				if(!isOpponent && (int32_t)player->balance >= guildFee){
					guildPaid = true;
					guildLeader = player;
				}
				else if(isOpponent && (int32_t)player->balance >= opponentFee){
					opponentPaid = true;
					opponentLeader = player;
				}
				else if(player->isOffline())
					delete player;
			}
		} while(result->next());

		db->freeResult(result);
	}
#endif

	//If both guilds have leaders that can afford the war, return true..
	if(guildPaid && opponentPaid){
		if(guildLeader){
			guildLeader->balance -= guildFee;
			if(guildLeader->isOffline()){
				IOPlayer::instance()->savePlayer(guildLeader);
				delete guildLeader;
			}
		}

		if(opponentLeader){
			opponentLeader->balance -= opponentFee;
			if(opponentLeader->isOffline()){
				IOPlayer::instance()->savePlayer(opponentLeader);
				delete opponentLeader;
			}
		}

		return true;
	}
	else{
		//clean up memory
		if(guildLeader && guildLeader->isOffline())
			delete guildLeader;

		if(opponentLeader && opponentLeader->isOffline())
			delete opponentLeader;
	}

	return false;
}

bool Guilds::setWarStatus(uint32_t warId, int32_t statusId)
{
	Database* db = Database::instance();
	DBQuery query;

	query << "UPDATE `guild_wars` SET `status` = " << statusId << " WHERE `id` = " << warId;
	return db->executeQuery(query.str());
}

void Guilds::processKill(uint32_t guildId, Player* player, const DeathList& killers)
{
	Guild* guild = getGuildById(guildId);
	Guild* enemy = getGuildById(player->getGuildId());
	if(!guild || !enemy)
		return;

	uint32_t warId = guild->isEnemy(enemy->getId());
	GuildWarsMap::iterator it = guildWars.find(warId);
	if(it != guildWars.end()){
		//Get number of frags
		uint32_t frags, enemyFrags;
		if(guild->hasDeclaredWar(warId)){
			frags = it->second.guildFrags;
			enemyFrags = it->second.opponentFrags;
		}
		else{
			frags = it->second.opponentFrags;
			enemyFrags = it->second.guildFrags;
		}

		//Get list of killers that belong to guild
		std::string kmsg;
		bool first = true;
		for(DeathList::const_iterator itt = killers.begin(); itt != killers.end(); ++itt){
			if(itt->isCreatureKill()){
				Player* attackerPlayer = itt->getKillerCreature()->getPlayer();
				if(itt->getKillerCreature()->isPlayerSummon())
					attackerPlayer = itt->getKillerCreature()->getPlayerMaster();

				if(attackerPlayer && attackerPlayer->getGuildId() == guild->getId()){
					if(!first)
						kmsg += " and ";
					else
						first = false;

					kmsg += attackerPlayer->getName();
				}
			}
		}

		//Send message to channels
		std::stringstream msg;
		msg << "Opponent " << player->getName() << " of the " << enemy->getName() << " was killed by " << kmsg <<
			". The new score is " << frags << ":" << enemyFrags << " frags (limit " << it->second.fragLimit << ").";
		guild->broadcastMessage(SPEAK_CHANNEL_W, msg.str());

		msg.str("");
		msg << "Guild member " << player->getName() << " was killed by " << kmsg << " of the " << guild->getName() <<
			". The new score is " << enemyFrags << ":" << frags << " frags (limit " << it->second.fragLimit << ").";
		enemy->broadcastMessage(SPEAK_CHANNEL_W, msg.str());

		if(it->second.finished){
			//Send final war messages
			msg.str("");
			msg << "Congratulations! You have won the war against " << enemy->getName() <<
				" with " << frags << " frags.";
			guild->broadcastMessage(SPEAK_CHANNEL_W, msg.str());

			msg.str("");
			msg << "You have lost the war against " << guild->getName() <<
				". They have reached the limit of " << frags << " frags.";
			enemy->broadcastMessage(SPEAK_CHANNEL_W, msg.str());

			//End guild war and do all needed tasks if it's set to do so in-game
			if(!g_config.getNumber(ConfigManager::GUILD_WARS_END_ONLY_ON_STARTUP))
				endWar(warId);
		}
	}
}

Guild* Guilds::getGuildById(uint32_t guildId)
{
	GuildsMap::iterator it = loadedGuilds.find(guildId);
	if(it != loadedGuilds.end())
		return it->second;
	else{
		Database* db = Database::instance();
		DBResult* result;
		DBQuery query;

		query << "SELECT `id`, `name` FROM `guilds` WHERE `id` = " << guildId;
		if((result = db->storeQuery(query.str()))){
			Guild* guild = new Guild();
			guild->setId(result->getDataInt("id"));
			guild->setName(result->getDataString("name"));
			loadedGuilds[guild->getId()] = guild;
			db->freeResult(result);
			return guild;
		}
	}

	return NULL;
}

bool Guilds::getGuildIdByName(uint32_t& guildId, const std::string& guildName)
{
	//Check cache
	for(GuildsMap::iterator it = loadedGuilds.begin(); it != loadedGuilds.end(); ++it){
		if(boost::algorithm::iequals(it->second->getName(), guildName)){
			guildId = it->first;
			return true;
		}
	}

	//Not in cache, let's try database (also add in cache if found)
	Database* db = Database::instance();
	DBResult* result;
	DBQuery query;

	query << "SELECT `id`, `name` FROM `guilds` WHERE `name` = " << db->escapeString(guildName);
	if((result = db->storeQuery(query.str()))){
		Guild* guild = new Guild();
		guild->setId(result->getDataInt("id"));
		guild->setName(result->getDataString("name"));
		loadedGuilds[guild->getId()] = guild;
		db->freeResult(result);
		return true;
	}

	return false;
}

Guild::Guild()
{
	id = 0;
	name = "";
}

Guild::~Guild()
{
	enemyGuilds.clear();
}

bool Guild::addFrag(uint32_t enemyId) const
{
	uint32_t warId = isEnemy(enemyId);
	GuildWarsMap::iterator it = g_guilds.getWars().find(warId);
	if(it != g_guilds.getWars().end()){
		if(!it->second.finished){
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

			if(frags >= it->second.fragLimit && it->second.fragLimit > 0)
				it->second.finished = true;

			return true;
		}
	}

	return false;
}

bool Guild::hasDeclaredWar(uint32_t warId) const
{
	GuildWarsMap::iterator it = g_guilds.getWars().find(warId);
	if(it != g_guilds.getWars().end()){
		if(it->second.guildId == getId())
			return true;
	}

	return false;
}

void Guild::broadcastMessage(SpeakClasses type, const std::string& msg) const
{
	ChatChannel* channel = g_chat.getGuildChannel(getId());
	if(channel) //Channel doesn't necessarily exists
		channel->sendInfo(type, msg);
}

uint32_t Guild::isEnemy(uint32_t guildId) const
{
	EnemyGuildsMap::const_iterator it = enemyGuilds.find(guildId);
	if(it != enemyGuilds.end()){
		if(it->first == guildId)
			return it->second;
	}

	return 0;
}

void Guild::addEnemy(uint32_t guildId, uint32_t warId)
{
	if(isEnemy(guildId) == 0)
		enemyGuilds[guildId] = warId;
}

void Guild::removeEnemy(uint32_t guildId)
{
	if(isEnemy(guildId) != 0)
		enemyGuilds.erase(guildId);
}

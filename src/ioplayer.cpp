//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Base class for the account loader/saver
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

#include "ioplayer.h"
#include "ioaccount.h"
#include "game.h"
#include "fileloader.h"
#include "player.h"
#include "depot.h"
#include "town.h"
#include "configmanager.h"
#include "singleton.h"

extern ConfigManager g_config;
extern Game g_game;

IOPlayer* IOPlayer::instance()
{
	static Singleton<IOPlayer> instance;
	return instance.get();
}

bool IOPlayer::loadPlayer(Player* player, const std::string& name, bool preload /*= false*/)
{
	DatabaseDriver* db = DatabaseDriver::instance();
	DBQuery query;
	DBResult_ptr result;

	query << "SELECT `players`.`id` AS `id`, `players`.`name` AS `name`, `accounts`.`name` AS `accname`, \
		`account_id`, `sex`, `vocation`, `town_id`, `experience`, `level`, `maglevel`, `health`, \
		`groups`.`name` AS `groupname`, `groups`.`flags` AS `groupflags`, `groups`.`access` AS `access`, \
		`groups`.`maxviplist` AS `maxviplist`, `groups`.`maxdepotitems` AS `maxdepotitems`, `groups`.`violation` AS `violationaccess`, \
		`healthmax`, `mana`, `manamax`, `manaspent`, `soul`, `direction`, `lookbody`, \
		`lookfeet`, `lookhead`, `looklegs`, `looktype`, `lookaddons`, `posx`, `posy`, `posz`, `cap`, \
		`lastlogin`, `lastlogout`, `lastip`, `conditions`, `skull_time`, `skull_type`, `stamina`, \
		`loss_experience`, `loss_mana`, `loss_skills`, `loss_items`, `loss_containers` \
		FROM `players` \
		LEFT JOIN `accounts` ON `account_id` = `accounts`.`id`\
		LEFT JOIN `groups` ON `groups`.`id` = `players`.`group_id` \
		WHERE `world_id` = " << g_config.getNumber(ConfigManager::WORLD_ID) << " AND `players`.`name` = " + db->escapeString(name);

	if(!(result = db->storeQuery(query))){
		return false;
	}

	player->setGUID(result->getDataInt("id"));
	player->accountId = result->getDataInt("account_id");
	player->accountName = result->getDataString("accname");

	player->groupName = result->getDataString("groupname");
	player->accessLevel = result->getDataInt("access");
	player->violationLevel = result->getDataInt("violationaccess");
	player->maxDepotLimit = result->getDataInt("maxdepotitems");
	player->maxVipLimit = result->getDataInt("maxviplist");
	player->setFlags(result->getDataLong("groupflags"));

	if(preload){
		//only loading basic info
		return true;
	}

	// Getting all player properties
	player->setSex((PlayerSex)result->getDataInt("sex"));
	player->setDirection((Direction)result->getDataInt("direction"));
	player->level = std::max((uint32_t)1, (uint32_t)result->getDataInt("level"));

	player->town = result->getDataInt("town_id");
	Town* town = Towns::getInstance()->getTown(player->town);
	if(town){
		player->masterPos = town->getTemplePosition();
	}

	uint64_t currExpCount = Player::getExpForLevel(player->level);
	uint64_t nextExpCount = Player::getExpForLevel(player->level + 1);
	uint64_t experience = (uint64_t)result->getDataLong("experience");
	if(experience < currExpCount || experience  > nextExpCount){
		experience = currExpCount;
	}

	player->experience = experience;
	player->levelPercent = Player::getPercentLevel(player->experience - currExpCount, nextExpCount - currExpCount);
	player->soul = result->getDataInt("soul");
	player->capacity = result->getDataInt("cap");
	player->stamina = result->getDataInt("stamina");
	player->lastLoginSaved = result->getDataInt("lastlogin");
	player->lastLogout = result->getDataInt("lastlogout");

	player->health = result->getDataInt("health");
	player->healthMax = result->getDataInt("healthmax");
	player->defaultOutfit.lookType = result->getDataInt("looktype");
	player->defaultOutfit.lookHead = result->getDataInt("lookhead");
	player->defaultOutfit.lookBody = result->getDataInt("lookbody");
	player->defaultOutfit.lookLegs = result->getDataInt("looklegs");
	player->defaultOutfit.lookFeet = result->getDataInt("lookfeet");
	player->defaultOutfit.lookAddons = result->getDataInt("lookaddons");

	player->currentOutfit = player->defaultOutfit;

#ifdef __SKULLSYSTEM__
	SkullType skullType = SkullType(result->getDataInt("skull_type"));
	int64_t lastSkullTime = result->getDataLong("skull_time");

	if((skullType == SKULL_RED && std::time(NULL) < lastSkullTime + g_config.getNumber(ConfigManager::RED_SKULL_DURATION)) ||
		(skullType == SKULL_BLACK && std::time(NULL) < lastSkullTime + g_config.getNumber(ConfigManager::BLACK_SKULL_DURATION))){
		player->lastSkullTime = lastSkullTime;
		player->skullType = skullType;
	}
#endif

	unsigned long conditionsSize = 0;
	const char* conditions = result->getDataStream("conditions", conditionsSize);
	PropStream propStream;
	propStream.init(conditions, conditionsSize);

	Condition* condition;
	while((condition = Condition::createCondition(propStream))){
		if(condition->unserialize(propStream)){
			player->storedConditionList.push_back(condition);
		}
		else{
			delete condition;
		}
	}

	// you need to set the vocation after conditions in order to ensure the proper regeneration rates for the vocation
	player->setVocation(result->getDataInt("vocation"));
	// this stuff has to go after the vocation is set
	player->mana = result->getDataInt("mana");
	player->manaMax = result->getDataInt("manamax");
	player->magLevel = result->getDataInt("maglevel");

	uint32_t nextManaCount = (uint32_t)player->vocation->getReqMana(player->magLevel + 1);
	uint32_t manaSpent = (uint32_t)result->getDataInt("manaspent");
	if(manaSpent > nextManaCount){
		//make sure its not out of bound
		manaSpent = 0;
	}
	player->manaSpent = manaSpent;
	player->magLevelPercent = Player::getPercentLevel(player->manaSpent, nextManaCount);

	player->setLossPercent(LOSS_EXPERIENCE, result->getDataInt("loss_experience"));
	player->setLossPercent(LOSS_MANASPENT, result->getDataInt("loss_mana"));
	player->setLossPercent(LOSS_SKILLTRIES, result->getDataInt("loss_skills"));
	player->setLossPercent(LOSS_ITEMS, result->getDataInt("loss_items"));
	player->setLossPercent(LOSS_CONTAINERS, result->getDataInt("loss_containers"));

	player->loginPosition.x = result->getDataInt("posx");
	player->loginPosition.y = result->getDataInt("posy");
	player->loginPosition.z = result->getDataInt("posz");

// login position is temple position
	Position loginPos = player->loginPosition;
	if(loginPos.x == 0 && loginPos.y == 0 && loginPos.z == 0){
		player->loginPosition = player->masterPos;
	}

	query.reset();
	query <<
		"SELECT "
		"	`guild_ranks`.`name` as `rank`, `guild_ranks`.`guild_id` as `guildid`, "
		"	`guild_ranks`.`level` as `level`, `guilds`.`name` as `guildname`, "
		"	`guild_members`.`nick` AS `nick` "
		"FROM `guild_members` "
		"LEFT JOIN `guild_ranks` ON `guild_ranks`.`id` = `guild_members`.`rank_id` "
		"LEFT JOIN `guilds` ON `guilds`.`id` = `guild_ranks`.`guild_id` "
		"WHERE `guild_members`.`player_id` = " << player->getGUID();

	if((result = db->storeQuery(query))){
		player->guildName = result->getDataString("guildname");
		player->guildLevel = result->getDataInt("level");
		player->guildId = result->getDataInt("guildid");
		player->guildRank = result->getDataString("rank");
		player->guildNick = result->getDataString("nick");
	}

	//get password
	query.reset();
	query << "SELECT `password`, `premend` FROM `accounts` WHERE `id` = " << player->accountId;
	if(!(result = db->storeQuery(query))){
		return false;
	}

	player->password = result->getDataString("password");
	player->premiumDays = IOAccount::getPremiumDaysLeft(result->getDataInt("premend"));

	// we need to find out our skills
	// so we query the skill table
	query.reset();
	query << "SELECT `skill_id`, `value`, `count` FROM `player_skills` WHERE `player_id` = " << player->getGUID();
	for(result = db->storeQuery(query); result; result = result->advance()){
		//now iterate over the skills
		try {
			SkillType skillid = SkillType::fromInteger(result->getDataInt("skill_id"));

			uint32_t skillLevel = result->getDataInt("value");
			uint32_t skillCount = result->getDataInt("count");

			uint32_t nextSkillCount = player->vocation->getReqSkillTries(skillid, skillLevel + 1);
			if(skillCount > nextSkillCount){
				//make sure its not out of bound
				skillCount = 0;
			}

			player->skills[skillid.value()][SKILL_LEVEL] = skillLevel;
			player->skills[skillid.value()][SKILL_TRIES] = skillCount;
			player->skills[skillid.value()][SKILL_PERCENT] = Player::getPercentLevel(skillCount, nextSkillCount);
		} catch(enum_conversion_error&) {
			std::cout << "Unknown skill ID when loading player " << result->getDataInt("skillid") << std::endl;
		}
	}

	//load inventory items
	ItemMap itemMap;

	/*
	query.reset();
	query << "SELECT `pid`, `sid`, `itemtype`, `count`, `attributes` FROM `player_items` WHERE `player_id` = " << player->getGUID() << " ORDER BY `sid` DESC";
	if((result = db->storeQuery(query.str()))){
		loadItems(itemMap, result);

		ItemMap::reverse_iterator it;
		ItemMap::iterator it2;

		for(it = itemMap.rbegin(); it != itemMap.rend(); ++it){
			Item* item = it->second.first;
			int pid = it->second.second;
			if(pid >= 1 && pid <= 10){
				player->__internalAddThing(pid, item);
			}
			else{
				it2 = itemMap.find(pid);
				if(it2 != itemMap.end())
					if(Container* container = it2->second.first->getContainer()){
						container->__internalAddThing(item);
					}
			}
		}

		db->freeResult(result);
	}


	//load depot items
	itemMap.clear();

	query.str("");
	query << "SELECT `pid`, `sid`, `itemtype`, `count`, `attributes` FROM `player_depotitems` WHERE `player_id` = " << player->getGUID() << " ORDER BY `sid` DESC";
	if((result = db->storeQuery(query.str()))){
		loadItems(itemMap, result);

		ItemMap::reverse_iterator it;
		ItemMap::iterator it2;

		for(it = itemMap.rbegin(); it != itemMap.rend(); ++it){
			Item* item = it->second.first;
			int pid = it->second.second;
			if(pid >= 0 && pid < 100){
				if(Container* c = item->getContainer()){
					if(Depot* depot = c->getDepot())
						player->addDepot(depot, pid);
					else
						std::cout << "Error loading depot "<< pid << " for player " << player->getGUID() << std::endl;
				}
				else{
					std::cout << "Error loading depot "<< pid << " for player " <<
						player->getGUID() << std::endl;
				}
			}
			else{
				it2 = itemMap.find(pid);
				if(it2 != itemMap.end())
					if(Container* container = it2->second.first->getContainer()){
						container->__internalAddThing(item);
					}
			}
		}

		db->freeResult(result);
	}
	*/

	//load storage map
	query.str("");
	query << "SELECT `id`, `value` FROM `player_storage` WHERE `player_id` = " << player->getGUID();
	for(result = db->storeQuery(query); result; result = result->advance()){
		std::string key = result->getDataString("id");
		std::string value = result->getDataString("value");
		player->setCustomValue(key, value);
	}

	//load vips
	query.str("");
	query << "SELECT `vip_id` FROM `player_viplist` WHERE `player_id` = " << player->getGUID();
	for(result = db->storeQuery(query); result; result = result->advance()){
		uint32_t vip_id = result->getDataInt("vip_id");
		std::string dummy_str;
		if(storeNameByGuid(*db, vip_id))
			player->addVIP(vip_id, dummy_str, false, true);
	}

	player->updateBaseSpeed();
	player->updateInventoryWeight();
	player->updateItemsLight(true);

	return true;
}

bool IOPlayer::saveItems(Player* player, const ItemBlockList& itemList, DBInsert& query_insert)
{
	/*
	std::list<Container*> listContainer;
	std::stringstream stream;

	typedef std::pair<Container*, int32_t> containerBlock;
	std::list<containerBlock> stack;

	int32_t parentId = 0;
	int32_t runningId = 100;

	Database* db = Database::instance();
	Item* item;
	int32_t pid;

	for(ItemBlockList::const_iterator it = itemList.begin(); it != itemList.end(); ++it){
		pid = it->first;
		item = it->second;
		++runningId;

		uint32_t attributesSize;

		PropWriteStream propWriteStream;
		item->serializeAttr(propWriteStream);
		const char* attributes = propWriteStream.getStream(attributesSize);

		stream << player->getGUID() << ", " << pid << ", " << runningId << ", " << item->getID() << ", " << (int32_t)item->getSubType() << ", " << db->escapeBlob(attributes, attributesSize);

		if(!query_insert.addRow(stream)){
			return false;
		}

		if(Container* container = item->getContainer()){
			stack.push_back(containerBlock(container, runningId));
		}
	}

	while(stack.size() > 0){
		const containerBlock& cb = stack.front();
		Container* container = cb.first;
		parentId = cb.second;
		stack.pop_front();

		for(uint32_t i = 0; i < container->size(); ++i){
			++runningId;
			item = container->getItem(i);
			if(Container* sub = item->getContainer()){
				stack.push_back(containerBlock(sub, runningId));
			}

			uint32_t attributesSize;

			PropWriteStream propWriteStream;
			item->serializeAttr(propWriteStream);
			const char* attributes = propWriteStream.getStream(attributesSize);

			stream << player->getGUID() << ", " << parentId << ", " << runningId << ", " << item->getID() << ", " << (int32_t)item->getSubType() << ", " << db->escapeBlob(attributes, attributesSize);

			if(!query_insert.addRow(stream))
				return false;
		}
	}
	*/
	return true;
}

bool IOPlayer::savePlayer(Player* player, bool shallow)
{
	player->preSave();

	DatabaseDriver* db = DatabaseDriver::instance();
	DBQuery query;
	DBResult_ptr result;

	//check if the player has to be saved or not
	query << "SELECT `save` FROM `players` WHERE `id` = " << player->getGUID();
	if(!(result = db->storeQuery(query))){
		return false;
	}

	const uint32_t save = result->getDataInt("save");

	if(save == 0)
		return true;

	//serialize conditions
	PropWriteStream propWriteStream;
	for(ConditionList::const_iterator it = player->conditions.begin(); it != player->conditions.end(); ++it){
		if((*it)->isPersistent()){
			if(!(*it)->serialize(propWriteStream)){
				return false;
			}

			propWriteStream.ADD_UCHAR(*CONDITIONATTR_END);
		}
	}

	uint32_t conditionsSize;
	const char* conditions = propWriteStream.getStream(conditionsSize);

	//First, an UPDATE query to write the player itself
	query.reset();
	query << "UPDATE `players` SET `level` = " << player->level
	<< ", `vocation` = " << (int32_t)player->getVocationId()
	<< ", `health` = " << player->health
	<< ", `healthmax` = " << player->healthMax
	<< ", `direction` = " << player->getDirection().value()
	<< ", `experience` = " << player->experience
	<< ", `lookbody` = " << (int32_t)player->defaultOutfit.lookBody
	<< ", `lookfeet` = " << (int32_t)player->defaultOutfit.lookFeet
	<< ", `lookhead` = " << (int32_t)player->defaultOutfit.lookHead
	<< ", `looklegs` = " << (int32_t)player->defaultOutfit.lookLegs
	<< ", `looktype` = " << (int32_t)player->defaultOutfit.lookType
	<< ", `lookaddons` = " << (int32_t)player->defaultOutfit.lookAddons
	<< ", `maglevel` = " << player->magLevel
	<< ", `mana` = " << player->mana
	<< ", `manamax` = " << player->manaMax
	<< ", `manaspent` = " << player->manaSpent
	<< ", `soul` = " << player->soul
	<< ", `town_id` = " << player->town
	<< ", `posx` = " << player->getLoginPosition().x
	<< ", `posy` = " << player->getLoginPosition().y
	<< ", `posz` = " << player->getLoginPosition().z
	<< ", `cap` = " << player->getCapacity()
	<< ", `sex` = " << player->sex.value()
	<< ", `conditions` = " << db->escapeBlob(conditions, conditionsSize)
	<< ", `loss_experience` = " << (int32_t)player->getLossPercent(LOSS_EXPERIENCE)
	<< ", `loss_mana` = " << (int32_t)player->getLossPercent(LOSS_MANASPENT)
	<< ", `loss_skills` = " << (int32_t)player->getLossPercent(LOSS_SKILLTRIES)
	<< ", `loss_items` = " << (int32_t)player->getLossPercent(LOSS_ITEMS)
	<< ", `loss_containers` = " << (int32_t)player->getLossPercent(LOSS_CONTAINERS)
	<< ", `stamina` = " << player->stamina;

#ifdef __SKULLSYSTEM__
	query << ", `skull_type` = " << (player->getSkull() == SKULL_RED || player->getSkull() == SKULL_BLACK ? player->getSkull().value() : 0);
	query << ", `skull_time` = " << player->lastSkullTime;
#endif

	query << " WHERE `id` = " << player->getGUID();

	DBTransaction transaction(db);
	if(!transaction.begin())
		return false;

	if(!db->executeQuery(query)){
		return false;
	}

	//skills
	for(int32_t i = 0; i <= 6; i++){
		query.reset();
		query << "UPDATE `player_skills` SET `value` = " << player->skills[i][SKILL_LEVEL] << ", `count` = " << player->skills[i][SKILL_TRIES] << " WHERE `player_id` = " << player->getGUID() << " AND `skill_id` = " << i;

		if(!db->executeQuery(query)){
			return false;
		}
	}

	if(shallow)
		return transaction.commit();

	// deletes all player-related stuff

	/*
	query << "DELETE FROM `player_items` WHERE `player_id` = " << player->getGUID();

	if(!db->executeQuery(query.str())){
		return false;
	}
	query.str("");

	query << "DELETE FROM `player_depotitems` WHERE `player_id` = " << player->getGUID();

	if(!db->executeQuery(query.str())){
		return false;
	}
	query.str("");
	*/

	query.reset();
	query << "DELETE FROM `player_storage` WHERE `player_id` = " << player->getGUID();

	if(!db->executeQuery(query)){
		return false;
	}

	query.reset();
	query << "DELETE FROM `player_viplist` WHERE `player_id` = " << player->getGUID();

	if(!db->executeQuery(query.str())){
		return false;
	}

	// Starti inserting
	DBInsert insert(db);

	/*
	ItemBlockList itemList;
	Item* item;
	for(int32_t slotId = 1; slotId <= 10; ++slotId){
		if((item = player->inventory[slotId])){
			itemList.push_back(itemBlock(slotId, item));
		}
	}

	//item saving
	stmt.setQuery("INSERT INTO `player_items` (`player_id` , `pid` , `sid` , `itemtype` , `count` , `attributes` ) VALUES ");
	if(!(saveItems(player, itemList, stmt) && stmt.execute())){
		return false;
	}

	itemList.clear();
	for(DepotMap::iterator it = player->depots.begin(); it != player->depots.end(); ++it){
		itemList.push_back(itemBlock(it->first, it->second));
	}

	//save depot items
	stmt.setQuery("INSERT INTO `player_depotitems` (`player_id` , `pid` , `sid` , `itemtype` , `count` , `attributes` ) VALUES ");
	if(!(saveItems(player, itemList, stmt) && stmt.execute())){
		return false;
	}
	*/

	insert.setQuery("INSERT INTO `player_storage` (`player_id` , `id` , `value` ) VALUES ");
	for(StorageMap::const_iterator cit = player->getCustomValueIteratorBegin(); cit != player->getCustomValueIteratorEnd();cit++){
		query.reset();
		query << player->getGUID() << ", " << db->escapeString(cit->first) << ", " << db->escapeString(cit->second);
		if(!insert.addRow(query.str())){
			return false;
		}
	}

	if(!insert.execute()){
		return false;
	}

	//save vip list
	if(!player->VIPList.empty()){
		query.reset();
		query << "INSERT INTO `player_viplist` (`player_id`, `vip_id`) SELECT " << player->getGUID()
			<< ", `id` FROM `players` WHERE `id` IN (";
		for(VIPListSet::iterator it = player->VIPList.begin(); it != player->VIPList.end(); ){
			query << (*it);
			++it;
			if(it != player->VIPList.end()){
				query << ",";
			}
			else{
				query << ")";
			}
		}

		if(!db->executeQuery(query)){
			return false;
		}
	}

	//End the transaction
	return transaction.commit();
}

bool IOPlayer::storeNameByGuid(DatabaseDriver &db, uint32_t guid)
{
	DBQuery query;
	DBResult_ptr result;

	NameCacheMap::iterator it = nameCacheMap.find(guid);
	if(it != nameCacheMap.end())
		return true;

	query << "SELECT `name` FROM `players` WHERE `id` = " << guid;

	if(!(result = db.storeQuery(query)))
		return false;

	nameCacheMap[guid] = result->getDataString("name");
	return true;
}

bool IOPlayer::addPlayerDeath(Player* dying_player, const DeathList& dlist)
{
	DatabaseDriver* db = DatabaseDriver::instance();

	DBQuery query;
	DBTransaction transaction(db);
	transaction.begin();

	// First insert the actual death
	{
		DBInsert death_stmt(db);
		death_stmt.setQuery("INSERT INTO `player_deaths` (`player_id`, `date`, `level`) VALUES ");

		query.reset();
		query << dying_player->getGUID() << ", " << std::time(NULL) << " , " << dying_player->getLevel();
		if(!death_stmt.addRow(query.str()))
			return false;
		if(!death_stmt.execute())
			return false;
	}

	uint64_t death_id = db->getLastInsertedRowID();

	// Then insert the killers...
	for(DeathList::const_iterator dli = dlist.begin(); dli != dlist.end(); ++dli){
		DBInsert killer_stmt(db);
		killer_stmt.setQuery("INSERT INTO `killers` (`death_id`, `final_hit`) VALUES ");

		query.reset();
		query << death_id << ", " << (dli == dlist.begin()? 1 : 0);
		if(!killer_stmt.addRow(query.str()))
			return false;
		if(!killer_stmt.execute())
			return false;

		uint64_t kill_id = db->getLastInsertedRowID();

		const DeathEntry& de = *dli;

		std::string name;
		if(de.isCreatureKill()){
			Creature* c = de.getKillerCreature();
			Player* player = c->getPlayer();

			if(c->isPlayerSummon()){
				// Set player, next it will insert GUID
				player = c->getPlayerMaster();
				// Set name, so the environment insert happens
				name = c->getNameDescription();
			}

			if(player){
				//reset unjust kill cache
				UnjustCacheMap::iterator it = unjustKillCacheMap.find(player->getGUID());
				if(it != unjustKillCacheMap.end()){
					unjustKillCacheMap.erase(it);
				}

				DBInsert player_killers_stmt(db);
				player_killers_stmt.setQuery("INSERT INTO `player_killers` (`kill_id`, `player_id`, `unjustified`) VALUES ");

				query.reset();
				query << kill_id << ", " << player->getGUID() << ", " << (de.isUnjustKill() ? 1 : 0);
				if(!player_killers_stmt.addRow(query.str()))
					return false;
				if(!player_killers_stmt.execute())
					return false;
			}
			else{ // Kill wasn't player, store name so next insert catches it
				name = c->getNameDescription();
			}
		}
		else{ // Not a creature kill
			name = de.getKillerName();
		}

		if(name.size() > 0){
			DBInsert env_killers_stmt(db);
			env_killers_stmt.setQuery("INSERT INTO `environment_killers` (`kill_id`, `name`) VALUES ");

			query.reset();
			query << kill_id << ", " << db->escapeString(name);
			if(!env_killers_stmt.addRow(query.str()))
				return false;
			if(!env_killers_stmt.execute())
				return false;
		}
	}

	return transaction.commit();
}

int32_t IOPlayer::getPlayerUnjustKillCount(const Player* player, UnjustKillPeriod_t period)
{
	time_t currentTime = std::time(NULL);

	int64_t date = 0;
	switch(period){
		case UNJUST_KILL_PERIOD_DAY:
			date = currentTime - 24 * 60 * 60;
			break;

		case UNJUST_KILL_PERIOD_WEEK:
			date = currentTime - 7 * 24 * 60 * 60;
			break;

		case UNJUST_KILL_PERIOD_MONTH:
			date = currentTime - 30 * 24 * 60 * 60;
			break;

		default:
			return 0;
			break;
	}

	UnjustKillBlock uk;

	UnjustCacheMap::iterator it = unjustKillCacheMap.find(player->getGUID());
	if(it != unjustKillCacheMap.end()){
		uk = it->second;

		switch(period){
			case UNJUST_KILL_PERIOD_DAY:
				if(it->second.dayExpireTime < currentTime && it->second.dayQueryTime > 0 && date >= it->second.dayQueryTime){
					return it->second.dayUnjustCount;
				}
				break;

			case UNJUST_KILL_PERIOD_WEEK:
				if(it->second.weekExpireTime < currentTime && it->second.weekQueryTime > 0 && date >= it->second.weekQueryTime){
					return it->second.weekUnjustCount;
				}
				break;

			case UNJUST_KILL_PERIOD_MONTH:
				if(it->second.monthExpireTime < currentTime && it->second.monthQueryTime > 0 && date >= it->second.monthQueryTime){
					return it->second.monthUnjustCount;
				}
				break;
		}
	}

	DatabaseDriver* db = DatabaseDriver::instance();
	DBQuery query;
	DBResult_ptr result;

	query << "SELECT `player_deaths`.`date`";
	query << "FROM ";
	query << "`player_killers` ";
	query << "LEFT JOIN ";
	query << "`killers` ON `killers`.`id` = `player_killers`.`kill_id` ";
	query << "LEFT JOIN ";
	query << "`player_deaths` on `player_deaths`.`id` = `killers`.`death_id` ";
	query << "LEFT JOIN ";
	query << "`players` on `players`.`id` = `player_deaths`.`player_id` ";
	query << "WHERE ";
	query << "`player_killers`.`player_id` = " << player->getGUID() << " "
		<< "AND " << "`player_killers`.`unjustified` = " << " 1 "
		<< "AND " << date  << " < `player_deaths`.`date` "
		<< "ORDER BY `player_deaths`.`date` ASC";

	int64_t expireTime = 0;
	uint32_t count = 0;
	for(result = db->storeQuery(query); result; result = result->advance()){
		if(count == 0){
			expireTime = result->getDataLong("date");
		}
		++count;
	}

	switch(period){
		case UNJUST_KILL_PERIOD_DAY:
			uk.dayQueryTime = date;
			uk.dayExpireTime = expireTime;
			uk.dayUnjustCount = count;
			break;

		case UNJUST_KILL_PERIOD_WEEK:
			uk.weekQueryTime = date;
			uk.weekExpireTime = expireTime;
			uk.weekUnjustCount = count;
			break;

		case UNJUST_KILL_PERIOD_MONTH:
			uk.monthQueryTime = date;
			uk.monthExpireTime = expireTime;
			uk.monthUnjustCount = count;
			break;

		default:
			return 0;
			break;
	}

	unjustKillCacheMap[player->getGUID()] = uk;
	return count;
}

bool IOPlayer::sendMail(Creature* actor, const std::string name, uint32_t depotId, Item* item)
{
	uint32_t guid;
	std::string dbname = name;
	if(!IOPlayer::instance()->getGuidByName(guid, dbname)){
		return false;
	}

	Player* player = g_game.getPlayerByNameEx(name);
	if(!player){
		return false;
	}

	bool result = false;
	Depot* depot = player->getDepot(depotId, true);
	if(depot){
		if(g_game.internalMoveItem(actor, item->getParent(), depot, INDEX_WHEREEVER,
			item, item->getItemCount(), NULL, FLAG_IGNORECAPACITY) == RET_NOERROR)
		{
			result = true;
		}
	}

	if(player->isOffline()){
		IOPlayer::instance()->savePlayer(player);
		delete player;
	}

	return result;
}

bool IOPlayer::getNameByGuid(uint32_t guid, std::string& name)
{
	NameCacheMap::iterator it = nameCacheMap.find(guid);
	if(it != nameCacheMap.end()){
		name = it->second;
		return true;
	}

	DatabaseDriver* db = DatabaseDriver::instance();
	DBQuery query;
	DBResult_ptr result;

	query << "SELECT `name` FROM `players` WHERE `world_id` = " << g_config.getNumber(ConfigManager::WORLD_ID) << " AND `id` = " << guid;

	if(!(result = db->storeQuery(query)))
		return false;

	name = result->getDataString("name");
	nameCacheMap[guid] = name;
	return true;
}

bool IOPlayer::getGuidByName(uint32_t &guid, std::string& name)
{
	GuidCacheMap::iterator it = guidCacheMap.find(name);
	if(it != guidCacheMap.end()){
		name = it->first;
		guid = it->second;
		return true;
	}

	DatabaseDriver* db = DatabaseDriver::instance();
	DBResult_ptr result;
	DBQuery query;

	query <<
		"SELECT `name`, `id` "
		"FROM `players` "
		"WHERE `world_id` = " << g_config.getNumber(ConfigManager::WORLD_ID) << " AND `name` = " << db->escapeString(name);
	if(!(result = db->storeQuery(query)))
		return false;

	name = result->getDataString("name");
	guid = result->getDataInt("id");

	guidCacheMap[name] = guid;
	return true;
}

bool IOPlayer::getAccountByName(uint32_t& account, const std::string& name)
{
	DatabaseDriver* db = DatabaseDriver::instance();
	DBResult_ptr result;
	DBQuery query;

	query <<
		"SELECT `account_id` "
		"FROM `players` "
		"WHERE `world_id` = " << g_config.getNumber(ConfigManager::WORLD_ID) << " AND `name` = " << db->escapeString(name);
	if(!(result = db->storeQuery(query)))
		return false;

	account = result->getDataInt("account_id");
	return true;
}


bool IOPlayer::getAccountByName(std::string& account, const std::string& player_name)
{
	DatabaseDriver* db = DatabaseDriver::instance();
	DBResult_ptr result;
	DBQuery query;

	query << "SELECT `a`.`name` FROM `players` p LEFT JOIN `accounts` a ON `p`.`account_id` = `a`.`id` "
		"WHERE `p`.`world_id` = " << g_config.getNumber(ConfigManager::WORLD_ID) << " AND `p`.`name` = " << db->escapeString(player_name);

	if(!(result = db->storeQuery(query)))
		return false;

	account = result->getDataString("name");
	return true;
}


bool IOPlayer::getGuidByNameEx(uint32_t& guid, bool& specialVip, const std::string& player_name)
{
	DatabaseDriver* db = DatabaseDriver::instance();
	DBResult_ptr result;
	DBQuery query;

	query <<
		"SELECT `players`.`name`, `players`.`id`, `groups`.`flags` AS `flags` "
		"FROM `players` LEFT JOIN `groups` ON `groups`.`id` = `players`.`group_id` "
		"WHERE `players`.`world_id` = " << g_config.getNumber(ConfigManager::WORLD_ID) << " AND `players`.`name`= " << db->escapeString(player_name);
	if(!(result = db->storeQuery(query.str())))
		return false;

	guid = result->getDataInt("id");
	specialVip = (result->getDataLong("flags") & (1ull << PlayerFlag_SpecialVIP)) != 0;
	return true;
}

bool IOPlayer::getDefaultTown(std::string& name, uint32_t& depotId)
{
	DatabaseDriver* db = DatabaseDriver::instance();
	DBResult_ptr result;
	DBQuery query;

	query <<
		"SELECT `town_id` "
		"FROM `players` "
		"WHERE `players`.`world_id` = " << g_config.getNumber(ConfigManager::WORLD_ID) << " AND `name`= " << db->escapeString(name);
	if(!(result = db->storeQuery(query)))
		return false;

	depotId = result->getDataInt("town_id");
	return true;
}

bool IOPlayer::getGuildIdByName(uint32_t& guildId, const std::string& guildName)
{
	DatabaseDriver* db = DatabaseDriver::instance();
	DBResult_ptr result;
	DBQuery query;

	query <<
		"SELECT `guilds`.`id` "
		"FROM `guilds` "
		"LEFT JOIN `players` ON `players`.`id` = `guilds`.`owner_id` "
		"WHERE `players`.`world_id` = " << g_config.getNumber(ConfigManager::WORLD_ID) << " AND `guilds`.`name` = " << db->escapeString(guildName);
	if(!(result = db->storeQuery(query)))
		return false;

	guildId = result->getDataInt("id");
	return true;
}

bool IOPlayer::playerExists(std::string name)
{
	DatabaseDriver* db = DatabaseDriver::instance();
	DBResult_ptr result;
	DBQuery query;

	query <<
		"SELECT `id` "
		"FROM `players` "
		"WHERE `players`.`world_id` = " << g_config.getNumber(ConfigManager::WORLD_ID) << " AND `name`= " << db->escapeString(name);
	return db->storeQuery(query);
}

bool IOPlayer::isPlayerOnlineByAccount(uint32_t acc)
{
	DatabaseDriver* db = DatabaseDriver::instance();
	DBResult_ptr result;
	DBQuery query;

	query <<
		"SELECT COUNT(*) AS `co` "
		"FROM `players` "
		"WHERE `players`.`online` = 1 AND `players`.`account_id` = " << acc;
	if(!(result = db->storeQuery(query)))
		return false;

	if(result->getDataInt("co") > 0)
		return true;

	return false;
}

void IOPlayer::loadItems(ItemMap& itemMap, DBResult* result)
{
	/*
	do{
		int sid = result->getDataInt("sid");
		int pid = result->getDataInt("pid");
		int type = result->getDataInt("itemtype");
		int count = result->getDataInt("count");

		unsigned long attrSize = 0;
		const char* attr = result->getDataStream("attributes", attrSize);

		PropStream propStream;
		propStream.init(attr, attrSize);

		Item* item = Item::CreateItem(type, count);
		if(item){
			if(!item->unserializeAttr(propStream)){
				std::cout << "WARNING: Serialize error in IOPlayer::loadItems" << std::endl;
			}

			std::pair<Item*, int> pair(item, pid);
			itemMap[sid] = pair;
		}
	}while(result->next());
	*/
}

bool IOPlayer::hasFlag(PlayerFlags flag, uint32_t guid)
{
	DatabaseDriver* db = DatabaseDriver::instance();
	DBResult_ptr result;

	DBQuery query;
	query <<
		"SELECT `groups`.`flags` AS `flags` "
		"FROM `players` "
		"LEFT JOIN `groups` ON `groups`.`id` = `players`.`group_id` "
		"WHERE `world_id` = " << g_config.getNumber(ConfigManager::WORLD_ID) << " AND `players`.`id` = " << guid;
	if(!(result = db->storeQuery(query)))
		return false;

	uint64_t flags = result->getDataLong("flags");
	return (flags & (1ull << flag)) != 0;
}

bool IOPlayer::getLastIP(uint32_t& ip, uint32_t guid)
{
	DatabaseDriver* db = DatabaseDriver::instance();
	DBResult_ptr result;

	DBQuery query;
	query <<
		"SELECT `lastip` "
		"FROM `players` "
		"WHERE `world_id` = " << g_config.getNumber(ConfigManager::WORLD_ID) << " AND `id` = " << guid << ";";
	if(!(result = db->storeQuery(query)))
		return false;

	ip = result->getDataInt("lastip");
	return true;
}

void IOPlayer::updateLoginInfo(Player* player)
{
	DatabaseDriver* db = DatabaseDriver::instance();
	DBQuery query;

	query << "UPDATE `players` SET `lastlogin` = " << player->lastLoginSaved
			<< ", `lastip` = " << player->lastip
			<< ", `online` = 1"
			<< " WHERE `id` = " << player->getGUID();

	db->executeQuery(query);
}

void IOPlayer::updateLogoutInfo(Player* player)
{
	DatabaseDriver* db = DatabaseDriver::instance();
	DBQuery query;

	query << "UPDATE `players` SET `lastlogout` = " << player->lastLogout
			<< ", `online` = 0"
			<< " WHERE `id` = " << player->getGUID();

	db->executeQuery(query);
}

bool IOPlayer::cleanOnlineInfo()
{
	DatabaseDriver* db = DatabaseDriver::instance();
	DBQuery query;
	query << "UPDATE `players` SET `online` = 0 WHERE `world_id` = " << g_config.getNumber(ConfigManager::WORLD_ID);
	return db->executeQuery(query);
}

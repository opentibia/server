//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Base class for the Account Loader/Saver
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

#include "definitions.h"
#include "ioplayer.h"
#include "ioaccount.h"
#include "item.h"
#include "town.h"
#include "configmanager.h"
#include "tools.h"
#include "definitions.h"

#include <iostream>
#include <iomanip>

extern ConfigManager g_config;

#ifndef __GNUC__
#pragma warning( disable : 4005)
#pragma warning( disable : 4996)
#endif

bool IOPlayer::loadPlayer(Player* player, const std::string& name, bool preload /*= false*/)
{
	Database* db = Database::instance();
	DBQuery query;
	DBResult* result;

	query << "SELECT `players`.`id` AS `id`, `players`.`name` AS `name`, `accounts`.`name` AS `accname`, \
		`account_id`, `players`.`group_id` as `group_id`, `sex`, `vocation`, `experience`, `level`, `maglevel`, `health`, \
		`healthmax`, `mana`, `manamax`, `manaspent`, `soul`, `direction`, `lookbody`, \
		`lookfeet`, `lookhead`, `looklegs`, `looktype`, `lookaddons`, `posx`, `posy`, \
		`posz`, `cap`, `lastlogin`, `lastlogout`, `lastip`, `conditions`, `skull_time`, \
		`skull_type`, `guildnick`, `loss_experience`, `loss_mana`, `loss_skills`, \
		`loss_items`, `loss_containers`, `rank_id`, `town_id`, `balance`, `stamina` \
		FROM `players` LEFT JOIN `accounts` ON `account_id` = `accounts`.`id` \
		WHERE `players`.`name` = " + db->escapeString(name);

	if(!(result = db->storeQuery(query.str()))){
	  	return false;
	}
	query.str("");

	player->setGUID(result->getDataInt("id"));
	player->accountId = result->getDataInt("account_id");
	player->accountName = result->getDataString("accname");

	PlayerGroup group = getPlayerGroup(result->getDataInt("group_id"));
	player->groupName = group.name;
	player->accessLevel = group.access;
	player->violationLevel = group.violation;
	player->maxDepotLimit = group.maxDepotItems;
	player->maxVipLimit = group.maxVip;
	player->setFlags(group.flags);

	if(preload){
		//only loading basic info
		db->freeResult(result);
		return true;
	}

	// Getting all player properties
	player->setSex((playersex_t)result->getDataInt("sex"));
	player->setDirection((Direction)result->getDataInt("direction"));
	player->level = std::max((uint32_t)1, (uint32_t)result->getDataInt("level"));

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

	uint32_t outfitId = Outfits::getInstance()->getOutfitId(player->defaultOutfit.lookType);
	bool canWearOutfit = true;
	if(outfitId > 0){
		//Check if the current outfit is right
		Outfit outfit;
		canWearOutfit = Outfits::getInstance()->getOutfit(outfitId, player->getSex(), outfit);
		if(canWearOutfit){
			if(player->defaultOutfit.lookType != outfit.lookType){
				player->defaultOutfit.lookType = outfit.lookType;
			}
		}
	}

	if(!canWearOutfit){
		//Just pick the first default outfit we can find
		const OutfitMap& default_outfits = Outfits::getInstance()->getOutfits(player->getSex());
		if(!default_outfits.empty()){
			Outfit newOutfit = (*default_outfits.begin()).second;
			player->defaultOutfit.lookType = newOutfit.lookType;
		}
	}

	player->currentOutfit = player->defaultOutfit;

#ifdef __SKULLSYSTEM__
	int32_t skullType = result->getDataInt("skull_type");
	int32_t lastSkullTime = result->getDataInt("skull_time");

	if( (skullType == SKULL_RED && lastSkullTime + g_config.getNumber(ConfigManager::RED_SKULL_DURATION) < std::time(NULL)) ||
		(skullType == SKULL_BLACK && lastSkullTime + g_config.getNumber(ConfigManager::BLACK_SKULL_DURATION) < std::time(NULL)) ){
		player->lastSkullTime = lastSkullTime;
		player->skullType = (Skulls_t)skullType;
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

	player->town = result->getDataInt("town_id");
	Town* town = Towns::getInstance().getTown(player->town);
	if(town){
		player->masterPos = town->getTemplePosition();
	}

	//if posx == 0 AND posy == 0 AND posz == 0
	// login position is temple position
	Position loginPos = player->loginPosition;
	if(loginPos.x == 0 && loginPos.y == 0 && loginPos.z == 0){
		player->loginPosition = player->masterPos;
	}

	uint32_t rankid = result->getDataInt("rank_id");

	// place it here and now we can drop all additional query instances as all data were loaded
	player->balance = result->getDataInt("balance");
	player->stamina = result->getDataInt("stamina");

	player->guildNick = result->getDataString("guildnick");
	db->freeResult(result);

	if(rankid){
		query << "SELECT `guild_ranks`.`name` as `rank`, `guild_ranks`.`guild_id` as `guildid`, `guild_ranks`.`level` as `level`, `guilds`.`name` as `guildname` FROM `guild_ranks`, `guilds` WHERE `guild_ranks`.`id` = " << rankid << " AND `guild_ranks`.`guild_id` = `guilds`.`id`";
		if((result = db->storeQuery(query.str()))){
			player->guildName = result->getDataString("guildname");
			player->guildLevel = result->getDataInt("level");
			player->guildId = result->getDataInt("guildid");
			player->guildRank = result->getDataString("rank");

			db->freeResult(result);
		}
		query.str("");
	}

	//get password
	query << "SELECT `password`, `premend` FROM `accounts` WHERE `id` = " << player->accountId;
	if(!(result = db->storeQuery(query.str()))){
		return false;
	}

	player->password = result->getDataString("password");
	player->premiumDays = Account::getPremiumDaysLeft(result->getDataInt("premend"));
	db->freeResult(result);

	// we need to find out our skills
	// so we query the skill table
	query.str("");
	query << "SELECT `skillid`, `value`, `count` FROM `player_skills` WHERE `player_id` = " << player->getGUID();
	if((result = db->storeQuery(query.str()))){
		//now iterate over the skills
		do{
			int skillid = result->getDataInt("skillid");
			if(skillid >= SKILL_FIRST && skillid <= SKILL_LAST){
				uint32_t skillLevel = result->getDataInt("value");
				uint32_t skillCount = result->getDataInt("count");

				uint32_t nextSkillCount = player->vocation->getReqSkillTries(skillid, skillLevel + 1);
				if(skillCount > nextSkillCount){
					//make sure its not out of bound
					skillCount = 0;
				}

				player->skills[skillid][SKILL_LEVEL] = skillLevel;
				player->skills[skillid][SKILL_TRIES] = skillCount;
				player->skills[skillid][SKILL_PERCENT] = Player::getPercentLevel(skillCount, nextSkillCount);
			}
		}while(result->next());
		db->freeResult(result);
	}

	query.str("");
	query << "SELECT `name` FROM `player_spells` WHERE `player_id` = " << player->getGUID();
	if((result = db->storeQuery(query.str()))){
		do{
			std::string spellName = result->getDataString("name");
			player->learnedInstantSpellList.push_back(spellName);
		}while(result->next());
		db->freeResult(result);
	}

	//load inventory items
	ItemMap itemMap;

	query.str("");
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

	//load storage map
	query.str("");
	query << "SELECT `key`, `value` FROM `player_storage` WHERE `player_id` = " << player->getGUID();
	if((result = db->storeQuery(query.str()))){
		do{
			uint32_t key = result->getDataInt("key");
			int32_t value = result->getDataInt("value");
			player->addStorageValue(key,value);
		}while(result->next());
		db->freeResult(result);
	}

	//load vip
	query.str("");
	query << "SELECT `vip_id` FROM `player_viplist` WHERE `player_id` = " << player->getGUID();
	if((result = db->storeQuery(query.str()))){
		do{
			uint32_t vip_id = result->getDataInt("vip_id");
			std::string dummy_str;
			if(storeNameByGuid(*db, vip_id))
				player->addVIP(vip_id, dummy_str, false, true);
		}while(result->next());
		db->freeResult(result);
	}

	player->updateBaseSpeed();
	player->updateInventoryWeight();
	player->updateItemsLight(true);

	return true;
}

bool IOPlayer::saveItems(Player* player, const ItemBlockList& itemList, DBInsert& query_insert)
{
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

	return true;
}

bool IOPlayer::savePlayer(Player* player, bool shallow)
{
	player->preSave();

	Database* db = Database::instance();
	DBQuery query;
	DBResult* result;

	//check if the player has to be saved or not
	query << "SELECT `save` FROM `players` WHERE `id` = " << player->getGUID();
	if(!(result = db->storeQuery(query.str()))){
		return false;
	}

	const uint32_t save = result->getDataInt("save");
	db->freeResult(result);

	if(save == 0)
		return true;

	//serialize conditions
	PropWriteStream propWriteStream;
	for(ConditionList::const_iterator it = player->conditions.begin(); it != player->conditions.end(); ++it){
		if((*it)->isPersistent()){
			if(!(*it)->serialize(propWriteStream)){
				return false;
			}

			propWriteStream.ADD_UCHAR(CONDITIONATTR_END);
		}
	}

	uint32_t conditionsSize;
	const char* conditions = propWriteStream.getStream(conditionsSize);

	//First, an UPDATE query to write the player itself
	query.str("");
	query << "UPDATE `players` SET `level` = " << player->level
	<< ", `vocation` = " << (int32_t)player->getVocationId()
	<< ", `health` = " << player->health
	<< ", `healthmax` = " << player->healthMax
	<< ", `direction` = " << (int32_t)player->getDirection()
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
	<< ", `sex` = " << player->sex
	<< ", `conditions` = " << db->escapeBlob(conditions, conditionsSize)
	<< ", `loss_experience` = " << (int32_t)player->getLossPercent(LOSS_EXPERIENCE)
	<< ", `loss_mana` = " << (int32_t)player->getLossPercent(LOSS_MANASPENT)
	<< ", `loss_skills` = " << (int32_t)player->getLossPercent(LOSS_SKILLTRIES)
	<< ", `loss_items` = " << (int32_t)player->getLossPercent(LOSS_ITEMS)
	<< ", `loss_containers` = " << (int32_t)player->getLossPercent(LOSS_CONTAINERS)
	<< ", `balance` = " << player->balance
	<< ", `stamina` = " << player->stamina;

#ifdef __SKULLSYSTEM__
	query << ", `skull_type` = " << (player->getSkull() == SKULL_RED || player->getSkull() == SKULL_BLACK ? player->getSkull() : 0);
	query << ", `skull_time` = " << player->lastSkullTime;
#endif

	query << " WHERE `id` = " << player->getGUID();

	DBTransaction transaction(db);
	if(!transaction.begin())
		return false;

	if(!db->executeQuery(query.str())){
		return false;
	}
	query.str("");

	//skills
	for(int32_t i = 0; i <= 6; i++){
		query << "UPDATE `player_skills` SET `value` = " << player->skills[i][SKILL_LEVEL] << ", `count` = " << player->skills[i][SKILL_TRIES] << " WHERE `player_id` = " << player->getGUID() << " AND `skillid` = " << i;

		if(!db->executeQuery(query.str())){
			return false;
		}
		query.str("");
	}

	if(shallow)
		return transaction.commit();

	// deletes all player-related stuff

	query << "DELETE FROM `player_spells` WHERE `player_id` = " << player->getGUID();

	if(!db->executeQuery(query.str())){
		return false;
	}
	query.str("");

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

	query << "DELETE FROM `player_storage` WHERE `player_id` = " << player->getGUID();

	if(!db->executeQuery(query.str())){
		return false;
	}
	query.str("");

	query << "DELETE FROM `player_viplist` WHERE `player_id` = " << player->getGUID();

	if(!db->executeQuery(query.str())){
		return false;
	}
	query.str("");

	DBInsert stmt(db);
	//learned spells
	stmt.setQuery("INSERT INTO `player_spells` (`player_id`, `name`) VALUES ");
	for(LearnedInstantSpellList::const_iterator it = player->learnedInstantSpellList.begin();
			it != player->learnedInstantSpellList.end(); ++it){
		query << player->getGUID() << ", " << db->escapeString(*it);
		if(!stmt.addRow(query)){
			return false;
		}
	}

	if(!stmt.execute()){
		return false;
	}

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

	stmt.setQuery("INSERT INTO `player_storage` (`player_id` , `key` , `value` ) VALUES ");
	player->genReservedStorageRange();
	for(StorageMap::const_iterator cit = player->getStorageIteratorBegin(); cit != player->getStorageIteratorEnd();cit++){
		query << player->getGUID() << ", " << cit->first << ", " << cit->second;
		if(!stmt.addRow(query)){
			return false;
		}
	}

	if(!stmt.execute()){
		return false;
	}

	//save vip list
	if(!player->VIPList.empty()){
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

		if(!db->executeQuery(query.str())){
			return false;
		}
		query.str("");
	}

	//End the transaction
	return transaction.commit();
}

bool IOPlayer::storeNameByGuid(Database &db, uint32_t guid)
{
	DBQuery query;
	DBResult* result;

	NameCacheMap::iterator it = nameCacheMap.find(guid);
	if(it != nameCacheMap.end())
		return true;

	query << "SELECT `name` FROM `players` WHERE `id` = " << guid;

	if(!(result = db.storeQuery(query.str())))
		return false;

	nameCacheMap[guid] = result->getDataString("name");
	db.freeResult(result);
	return true;
}

/*
bool IOPlayer::getPlayerUnjustKillCount(Player* player, uin64_t date)
{
	Database* db = Database::instance();
	DBQuery query;
	DBResult* result;

	query << "";
	query << "SELECT COUNT(*)";
	query << "FROM";
	query << "player_killers";
	query << "LEFT JOIN";
	query << "killers ON killers.id = player_killers.kill_id";
	query << "LEFT JOIN";
	query << "player_deaths on player_deaths.id = killers.death_id";
	query << "LEFT JOIN";
	query << "players on players.id = player_deaths.player_id";
	query << "WHERE";
	query << "player_killers.player_id = " << player->getGUID() << " AND " << date  << "> player_deaths.date";

	if(!(result = db->storeQuery(query.str())))
		return false;

	name = result->getDataString("name");
	nameCacheMap[guid] = name;
	db->freeResult(result);
	return true;
}
*/

bool IOPlayer::addPlayerDeath(Player* dying_player, const DeathList& dlist)
{
	Database* db = Database::instance();
	
	DBQuery q;
	DBTransaction transaction(db);
	transaction.begin();
	std::ostringstream query;
	
	
	// First insert the actual death
	{
		DBInsert death_stmt(db);
		death_stmt.setQuery("INSERT INTO `player_deaths` (`player_id`, `date`, `level`) VALUES ");

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

		query.str("");
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
				// Set player, next if will insert GUID
				player = c->getPlayerMaster();
				// Set name, so the enviroment insert happenends
				name = c->getNameDescription();
			}

			if(player){
				DBInsert player_killers_stmt(db);
				player_killers_stmt.setQuery("INSERT INTO `player_killers` (`kill_id`, `player_id`) VALUES ");

				query.str("");
				query << kill_id << ", " << player->getGUID();
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

			query.str("");
			query << kill_id << ", " << db->escapeString(name);
			if(!env_killers_stmt.addRow(query.str()))
				return false;
			if(!env_killers_stmt.execute())
				return false;
		}
	}

	return transaction.commit();
}

bool IOPlayer::getNameByGuid(uint32_t guid, std::string& name)
{
	NameCacheMap::iterator it = nameCacheMap.find(guid);
	if(it != nameCacheMap.end()){
		name = it->second;
		return true;
	}

	Database* db = Database::instance();
	DBQuery query;
	DBResult* result;

	query << "SELECT `name` FROM `players` WHERE `id` = " << guid;

	if(!(result = db->storeQuery(query.str())))
		return false;

	name = result->getDataString("name");
	nameCacheMap[guid] = name;
	db->freeResult(result);
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

	Database* db = Database::instance();
	DBResult* result;
	DBQuery query;

	if(!(result = db->storeQuery("SELECT `name`, `id` FROM `players` WHERE `name` = " + db->escapeString(name))))
		return false;

	name = result->getDataString("name");
	guid = result->getDataInt("id");

	guidCacheMap[name] = guid;
	db->freeResult(result);
	return true;
}

bool IOPlayer::getAccountByName(uint32_t& account, std::string& name)
{
	Database* db = Database::instance();
	DBResult* result;
	DBQuery query;

	if(!(result = db->storeQuery("SELECT `account_id` FROM `players` WHERE `name` = " + db->escapeString(name))))
		return false;

	account = result->getDataInt("account_id");
	db->freeResult(result);
	return true;
}


bool IOPlayer::getAccountByName(std::string& account, std::string& name)
{
	Database* db = Database::instance();
	DBQuery query;
	query << "SELECT `a`.`name` FROM `players` p LEFT JOIN `accounts` a ON `p`.`account_id` = `a`.`id` WHERE `p`.`name` = " << db->escapeString(name);

	DBResult* result;
	if(!(result = db->storeQuery(query.str())))
		return false;

	account = result->getDataString("name");
	db->freeResult(result);
	return true;
}


bool IOPlayer::getGuidByNameEx(uint32_t& guid, bool& specialVip, std::string& name)
{
	Database* db = Database::instance();
	DBResult* result;
	DBQuery query;

	if(!(result = db->storeQuery("SELECT `name`, `id`, `group_id` FROM `players` WHERE `name`= " + db->escapeString(name))))
		return false;

	name = result->getDataString("name");
	guid = result->getDataInt("id");
	specialVip = (0 != (getPlayerGroup(result->getDataInt("group_id")).flags & (
		(uint64_t)1 << PlayerFlag_SpecialVIP)));

	db->freeResult(result);
	return true;
}

bool IOPlayer::getDefaultTown(std::string& name, uint32_t& depotId)
{
	Database* db = Database::instance();
	DBResult* result;
	DBQuery query;

	if(!(result = db->storeQuery("SELECT `town_id` FROM `players` WHERE `name`= " + db->escapeString(name))))
		return false;

	depotId = result->getDataInt("town_id");
	db->freeResult(result);
	return true;
}

bool IOPlayer::getGuildIdByName(uint32_t& guildId, const std::string& guildName)
{
	Database* db = Database::instance();
	DBResult* result;
	DBQuery query;

	if(!(result = db->storeQuery("SELECT `id` FROM `guilds` WHERE `name` = " + db->escapeString(guildName))))
		return false;

	guildId = result->getDataInt("id");
	db->freeResult(result);
	return true;
}

bool IOPlayer::playerExists(std::string name)
{
	Database* db = Database::instance();
	DBResult* result;
	DBQuery query;

	if(!(result = db->storeQuery("SELECT `id` FROM `players` WHERE `name`= " + db->escapeString(name))))
		return false;

	db->freeResult(result);
	return true;
}

const PlayerGroup IOPlayer::getPlayerGroup(uint32_t groupid)
{
	PlayerGroupMap::const_iterator it = playerGroupMap.find(groupid);
	if(it != playerGroupMap.end()){
		return it->second;
	}

	Database* db = Database::instance();
	DBResult* result;

	DBQuery query;
	PlayerGroup group;

	query << "SELECT * FROM `groups` WHERE `id`= " << groupid;
	if(!(result = db->storeQuery(query.str()))){
		return group;
	}

	group.name = result->getDataString("name");
	group.flags = result->getDataLong("flags");
	group.access = result->getDataInt("access");
	group.violation = result->getDataInt("violation");
	group.maxDepotItems = result->getDataInt("maxdepotitems");
	group.maxVip = result->getDataInt("maxviplist");

	playerGroupMap[groupid] = group;
	db->freeResult(result);
	return group;
}

void IOPlayer::loadItems(ItemMap& itemMap, DBResult* result)
{
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
}

bool IOPlayer::hasFlag(PlayerFlags flag, uint32_t guid)
{
	Database* db = Database::instance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `group_id` FROM `players` WHERE `id` = " << guid;
	if(!(result = db->storeQuery(query.str())))
		return false;

	PlayerGroup group = getPlayerGroup(result->getDataInt("group_id"));
	db->freeResult(result);
	return (0 != (group.flags & ((uint64_t)1 << flag)));
}

bool IOPlayer::getLastIP(uint32_t& ip, uint32_t guid)
{
	Database* db = Database::instance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `lastip` FROM `players` WHERE `id` = " << guid << " AND `deleted` = 0;";
	if(!(result = db->storeQuery(query.str())))
		return false;

	ip = result->getDataInt("lastip");
	db->freeResult(result);
	return true;
}

void IOPlayer::updateLoginInfo(Player* player)
{
	Database* db = Database::instance();
	DBQuery query;
	
	query << "UPDATE `players` SET `lastlogin` = " << player->lastLoginSaved
			<< ", `lastip` = " << player->lastip
			<< ", `online` = 1"
			<< " WHERE `id` = " << player->getGUID();
			
	db->executeQuery(query.str());
}

void IOPlayer::updateLogoutInfo(Player* player)
{
	Database* db = Database::instance();
	DBQuery query;
	
	query << "UPDATE `players` SET `lastlogout` = " << player->lastLogout
			<< ", `online` = 0"
			<< " WHERE `id` = " << player->getGUID();
			
	db->executeQuery(query.str());
}

bool IOPlayer::cleanOnlineInfo()
{
	Database* db = Database::instance();
	DBQuery query;
	return db->executeQuery("UPDATE `players` SET `online` = 0");
}

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

#include <boost/tokenizer.hpp>
#include <iostream>
#include <iomanip>

extern ConfigManager g_config;

typedef boost::tokenizer<boost::char_separator<char> > tokenizer;

#ifndef __GNUC__
#pragma warning( disable : 4005)
#pragma warning( disable : 4996)
#endif

IOPlayer* IOPlayer::_instance = NULL;

IOPlayer* IOPlayer::instance()
{
	if(!_instance)
		_instance = new IOPlayer;

	return _instance;
}

IOPlayer::IOPlayer()
{
	//
}

bool IOPlayer::loadPlayer(Player* player, std::string name)
{
	Database* db = Database::instance();
	DBQuery query;
	DBResult* result;

	if(!(result = db->storeQuery("SELECT `id`,`name`,`account_id`,`group_id`,`premend`,`sex`,"
	  "`vocation`,`experience`,`level`,`maglevel`,`health`,`healthmax`,`mana`,"
	  "`manamax`,`manaspent`,`soul`,`direction`,`lookbody`,`lookfeet`,`lookhead`,"
	  "`looklegs`,`looktype`,`lookaddons`,`posx`,`posy`,`posz`,`cap`,`lastlogin`,"
	  "`lastip`,`save`,`conditions`,`redskulltime`,`redskull`,`guildnick`,"
	  "`rank_id`,`town_id`,`loss_experience`,`loss_mana`,`loss_skills`"
	  "FROM `players` WHERE `name` = " + db->escapeString(name) )));
		return false;

	if(!result->next()) {
		db->freeResult(result);
		return false;
	}

	// Getting all player properties
	player->setGUID(result->getDataInt("id"));

	player->accountNumber = result->getDataInt("account_id");
	player->setSex((playersex_t)result->getDataInt("sex"));

	const PlayerGroup* group = getPlayerGroup(result->getDataInt("group_id"));
	if(group){
		player->accessLevel = group->m_access;
		player->maxDepotLimit = group->m_maxDepotItems;
		player->maxVipLimit = group->m_maxVip;
		player->setFlags(group->m_flags);
	}

	#ifdef __USE_SQL_PREMDAYS__
	player->premiumDays = result->getDataInt("premdays");
	#endif

	player->setDirection((Direction)result->getDataInt("direction"));
	player->experience = result->getDataInt("experience");
	player->level = result->getDataInt("level");
	player->soul = result->getDataInt("soul");
	player->capacity = result->getDataInt("cap");
	player->lastLoginSaved = result->getDataInt("lastlogin");
	player->setVocation(result->getDataInt("vocation"));
	player->updateBaseSpeed();

	player->mana = result->getDataInt("mana");
	player->manaMax = result->getDataInt("manamax");
	player->manaSpent = result->getDataInt("manaspent");
	player->magLevel = result->getDataInt("maglevel");

	player->health = result->getDataInt("health");
	if(player->health <= 0)
		player->health = 100;

	player->healthMax = result->getDataInt("healthmax");
	if(player->healthMax <= 0)
		player->healthMax = 100;

	player->defaultOutfit.lookType = result->getDataInt("looktype");
	player->defaultOutfit.lookHead = result->getDataInt("lookhead");
	player->defaultOutfit.lookBody = result->getDataInt("lookbody");
	player->defaultOutfit.lookLegs = result->getDataInt("looklegs");
	player->defaultOutfit.lookFeet = result->getDataInt("lookfeet");
	player->defaultOutfit.lookAddons = result->getDataInt("lookaddons");
	player->currentOutfit = player->defaultOutfit;

	#ifdef __SKULLSYSTEM__
	int32_t redSkullSeconds = result->getDataInt("redskulltime") - std::time(NULL);
	if(redSkullSeconds > 0){
		//ensure that we round up the number of ticks
		player->redSkullTicks = (redSkullSeconds + 2)*1000;
		if(result->getDataInt("redskull") == 1){
			player->skull = SKULL_RED;
		}
	}
	#endif

	unsigned long conditionsSize = 0;
	const char* conditions = result->getDataStream("conditions", conditionsSize);
	PropStream propStream;
	propStream.init(conditions, conditionsSize);

	Condition* condition;
	while(condition = Condition::createCondition(propStream))
		if(condition->unserialize(propStream))
			player->storedConditionList.push_back(condition);
		else
			delete condition;

	player->lossPercent[LOSS_EXPERIENCE] = result->getDataInt("loss_experience");
	player->lossPercent[LOSS_MANASPENT] = result->getDataInt("loss_mana");
	player->lossPercent[LOSS_SKILLTRIES] = result->getDataInt("loss_skills");

	player->loginPosition.x = result->getDataInt("posx");
	player->loginPosition.y = result->getDataInt("posy");
	player->loginPosition.z = result->getDataInt("posz");

	player->town = result->getDataInt("town_id");
	Town* town = Towns::getInstance().getTown(player->town);
	if(town)
		player->masterPos = town->getTemplePosition();

	//if posx == 0 AND posy == 0 AND posz == 0
	// login position is temple position
	Position loginPos = player->loginPosition;
	if(loginPos.x == 0 && loginPos.y == 0 && loginPos.z == 0)
		player->loginPosition = player->masterPos;

	uint32_t rankid = result->getDataInt("rank_id");

	// place it here and now we can drop all additional query instances as all data were loaded
	#ifndef __USE_SQL_PREMDAYS__
	time_t premEnd = result->getDataInt("premend");
	time_t timeNow = time(NULL);
	if(premEnd != 0 && premEnd < timeNow) {
		//TODO: remove every premium property of the player
		// outfit, vocation, temple, ...

		//update table
		query << "UPDATE `players` SET `premend` = 0 WHERE `id` = " << player->getGUID();
		db->executeQuery(query.str());
		query.str("");
	} else {
		player->premiumDays = (premEnd - timeNow)/(3600*24);
	}
	#endif

	player->guildNick = result->getDataString("guildnick");
	db->freeResult(result);

	if(rankid) {
		query << "SELECT `guild_ranks`.`name` as `rank`, `guild_ranks`.`guild_id` as `guildid`, `guild_ranks`.`level` as `level`, `guilds`.`name` as `guildname` FROM `guild_ranks`, `guilds` WHERE `guild_ranks`.`id` = " << rankid << " AND `guild_ranks`.`guild_id` = `guilds.id`";
		if(result = db->storeQuery(query.str())) {
			if(result->next()) {
				player->guildName = result->getDataString("guildname");
				player->guildLevel = result->getDataInt("level");
				player->guildId = result->getDataInt("guildid");
				player->guildRank = result->getDataString("rank");
			}

			db->freeResult(result);
		}
		query.str("");
	}

	//get password
	query << "SELECT `password` FROM `accounts` WHERE `id` = " << player->accountNumber;
	if(!(result = db->storeQuery(query.str())))
		return false;

	if(!result->next()) {
		db->freeResult(result);
		return false;
	}

	player->password = result->getDataString("password");

	// we need to find out our skills
	// so we query the skill table
	query.str("SELECT `skillid`, `value`, `count` FROM `player_skills` WHERE `player_id` = ");
	query << player->getGUID();
	if(result = db->storeQuery(query.str())) {
		//now iterate over the skills
		while(result->next()) {
			int skillid = result->getDataInt("skillid");
			if(skillid >= SKILL_FIRST && skillid <= SKILL_LAST) {
				player->skills[skillid][SKILL_LEVEL] = result->getDataInt("value");
				player->skills[skillid][SKILL_TRIES] = result->getDataInt("count");
			}
		}

		db->freeResult(result);
	}

	query.str("SELECT `name` FROM `player_spells` WHERE `player_id` = ");
	query << player->getGUID();
	if(result = db->storeQuery(query.str())) {
		while(result->next()) {
			std::string spellName = result->getDataString("name");
			player->learnedInstantSpellList.push_back(spellName);
		}

		db->freeResult(result);
	}

	//load inventory items
	ItemMap itemMap;

	query.str("SELECT `pid`, `sid`, `itemtype`, `count`, `attributes` FROM `player_items` WHERE `player_id` = ");
	query << player->getGUID() << " ORDER BY `sid` DESC";
	if(result = db->storeQuery(query.str()) ) {
		loadItems(itemMap, result);

		ItemMap::reverse_iterator it;
		ItemMap::iterator it2;

		for(it = itemMap.rbegin(); it != itemMap.rend(); ++it) {
			Item* item = it->second.first;
			int pid = it->second.second;
			if(pid >= 1 && pid <= 10){
				player->__internalAddThing(pid, item);
			} else {
				it2 = itemMap.find(pid);
				if(it2 != itemMap.end())
					if(Container* container = it2->second.first->getContainer())
						container->__internalAddThing(item);
			}
		}

		db->freeResult(result);
	}

	player->updateInventoryWeigth();
	player->updateItemsLight(true);
	player->setSkillsPercents();

	//load depot items
	itemMap.clear();

	query.str("SELECT `pid`, `sid`, `itemtype`, `count`, `attributes` FROM `player_depotitems` WHERE `player_id` = ");
	query << player->getGUID() << " ORDER BY `sid` DESC";
	if(result = db->storeQuery(query.str()) ) {
		loadItems(itemMap, result);

		ItemMap::reverse_iterator it;
		ItemMap::iterator it2;

		for(it = itemMap.rbegin(); it != itemMap.rend(); ++it) {
			Item* item = it->second.first;
			int pid = it->second.second;
			if(pid >= 0 && pid < 100){
				if(Container* c = item->getContainer()) {
					if(Depot* depot = c->getDepot())
						player->addDepot(depot, pid);
					else
						std::cout << "Error loading depot "<< pid << " for player " << player->getGUID() << std::endl;
				} else {
					std::cout << "Error loading depot "<< pid << " for player " <<
						player->getGUID() << std::endl;
				}
			} else {
				it2 = itemMap.find(pid);
				if(it2 != itemMap.end())
					if(Container* container = it2->second.first->getContainer())
						container->__internalAddThing(item);
			}
		}

		db->freeResult(result);
	}

	//load storage map
	query.str("SELECT `key`, `value` FROM `player_storage` WHERE `player_id` = ");
	query << player->getGUID();
	if(result = db->storeQuery(query.str())) {
		while(result->next()) {
			uint32_t key = result->getDataInt("key");
			int32_t value = result->getDataInt("value");
			player->addStorageValue(key,value);
		}
		db->freeResult(result);
	}

	//load vip
	query.str("SELECT `vip_id` FROM `player_viplist` WHERE `player_id` = ");
	query << player->getGUID();
	if(result = db->storeQuery(query.str())) {
		while(result->next()) {
			uint32_t vip_id = result->getDataInt("vip_id");
			std::string dummy_str;
			if(storeNameByGuid(*db, vip_id))
				player->addVIP(vip_id, dummy_str, false, true);
		}
		db->freeResult(result);
	}

	return true;
}

bool IOPlayer::saveItems(Player* player, const ItemBlockList& itemList, DBStatement* query_insert)
{
	std::list<Container*> listContainer;

	typedef std::pair<Container*, int32_t> containerBlock;
	std::list<containerBlock> stack;

	int32_t parentId = 0;
	int32_t runningId = 100;

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

		query_insert->setInt(1, pid);
		query_insert->setInt(2, runningId);
		query_insert->setInt(3, (int32_t)item->getID() );
		query_insert->setInt(4, (int32_t)item->getItemCountOrSubtype() );
		query_insert->bindStream(5, attributes, attributesSize);

		if(!query_insert->execute()){
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
			Container* container = item->getContainer();
			if(container){
				stack.push_back(containerBlock(container, runningId));
			}

			uint32_t attributesSize;

			PropWriteStream propWriteStream;
			item->serializeAttr(propWriteStream);
			const char* attributes = propWriteStream.getStream(attributesSize);

			query_insert->setInt(1, parentId);
			query_insert->setInt(2, runningId);
			query_insert->setInt(3, (int32_t)item->getID() );
			query_insert->setInt(4, (int32_t)item->getItemCountOrSubtype() );
			query_insert->bindStream(5, attributes, attributesSize);

			if(!query_insert->execute()){
				return false;
			}
		}
	}

	return true;
}

bool IOPlayer::savePlayer(Player* player)
{
	player->preSave();

	Database* db = Database::instance();
	DBQuery query;
	DBResult* result;

	//check if the player have to be saved or not
	query << "SELECT `save` FROM `players` WHERE `id` = " << player->getGUID();

	if(!(result = db->storeQuery(query.str())))
		return false;

	if(!result->next()) {
		db->freeResult(result);
		return false;
	}

	// If save var is not 1 don't save the player info
	if(result->getDataInt("save") != 1) {
		db->freeResult(result);
		return true;
	}

	db->freeResult(result);

	//serialize conditions
	PropWriteStream propWriteStream;

	for(ConditionList::const_iterator it = player->conditions.begin(); it != player->conditions.end(); ++it){
		if((*it)->isPersistent()){
			if(!(*it)->serialize(propWriteStream))
				return false;

			propWriteStream.ADD_UCHAR(CONDITIONATTR_END);
		}
	}

	uint32_t conditionsSize;
	const char* conditions = propWriteStream.getStream(conditionsSize);

	//First, an UPDATE query to write the player itself
	query.str("UPDATE `players` SET `level` = ");
	query << player->level
	<< ", `vocation` = " << (int)player->getVocationId()
	<< ", `health` = " << player->health
	<< ", `healthmax` = " << player->healthMax
	<< ", `direction` = " << (int)player->getDirection()
	<< ", `experience` = " << player->experience
	<< ", `lookbody` = " << (int)player->defaultOutfit.lookBody
	<< ", `lookfeet` = " << (int)player->defaultOutfit.lookFeet
	<< ", `lookhead` = " << (int)player->defaultOutfit.lookHead
	<< ", `looklegs` = " << (int)player->defaultOutfit.lookLegs
	<< ", `looktype` = " << (int)player->defaultOutfit.lookType
	<< ", `lookaddons` = " << (int)player->defaultOutfit.lookAddons
	<< ", `magLevel` = " << player->magLevel
	<< ", `mana` = " << player->mana
	<< ", `manamax` = " << player->manaMax
	<< ", `manaspent` = " << player->manaSpent
	<< ", `soul` = " << player->soul
	<< ", `town_id` = '" << player->town
	<< ", `posx` = '" << player->getLoginPosition().x
	<< ", `posy` = '" << player->getLoginPosition().y
	<< ", `posz` = '" << player->getLoginPosition().z
	<< ", `cap` = " << player->getCapacity()
	<< ", `sex` = " << player->sex
	<< ", `lastlogin` = " << player->lastlogin
	<< ", `lastip` = " << player->lastip
	<< ", `conditions` = " << db->escapeString(conditions)
	<< ", `loss_experience` = " << (int)player->getLossPercent(LOSS_EXPERIENCE)
	<< ", `loss_mana` = " << (int)player->getLossPercent(LOSS_MANASPENT)
	<< ", `loss_skills` = " << (int)player->getLossPercent(LOSS_SKILLTRIES);

#ifdef __SKULLSYSTEM__
	int32_t redSkullTime = 0;
	if(player->redSkullTicks > 0){
		redSkullTime = std::time(NULL) + player->redSkullTicks/1000;
	}

	query << ", `redskulltime` = " << redSkullTime;
	int32_t redSkull = 0;
	if(player->skull == SKULL_RED){
		redSkull = 1;
	}

	query << ", `redskull` = " << redSkull;
#endif

	query << " WHERE `id` = " << player->getGUID() << " LIMIT 1";

	if( !db->beginTransaction() )
		return false;

	if(!db->executeQuery(query.str())) {
		db->rollback();
		return false;
	}

	DBStatement* stmt;

	query.str("UPDATE `player_skills` SET `value` = ?, `count` = ? WHERE `player_id` = ");
	query << player->getGUID() << " AND `skillid` = ?";

	if(!(stmt = db->prepareStatement(query.str())))
		return false;

	//skills
	for(int i = 0; i <= 6; i++){
		stmt->setInt(1, player->skills[i][SKILL_LEVEL]);
		stmt->setInt(2, player->skills[i][SKILL_TRIES]);
		stmt->setInt(3, i);

		if(!stmt->execute()) {
			db->freeStatement(stmt);
			db->rollback();
			return false;
		}
	}

	db->freeStatement(stmt);

	//learned spells
	query.str("INSERT INTO `player_spells` (`player_id`, `name`) VALUES (");
	query << player->getGUID() << ", ?)";
	if(!(stmt = db->prepareStatement(query.str())))
		return false;

	for(LearnedInstantSpellList::const_iterator it = player->learnedInstantSpellList.begin();
			it != player->learnedInstantSpellList.end(); ++it){
		stmt->setString(1, db->escapeString(*it) );

		if(!stmt->execute()) {
			db->freeStatement(stmt);
			db->rollback();
			return false;
		}
	}

	db->freeStatement(stmt);

	ItemBlockList itemList;

	Item* item;
	for(int32_t slotId = 1; slotId <= 10; ++slotId){
		if(item = player->inventory[slotId])
			itemList.push_back(itemBlock(slotId, item));
	}

	//item saving
	query.str("INSERT INTO `player_items` (`player_id` , `pid` , `sid` , `itemtype` , `count` , `attributes` ) VALUES (");
	query << player->getGUID() << ", ?, ?, ?, ?, ?)";

	if(!(stmt = db->prepareStatement(query.str())))
		return false;

	if(!saveItems(player, itemList, stmt)) {
		db->freeStatement(stmt);
		db->rollback();
		return false;
	}

	db->freeStatement(stmt);

	itemList.clear();
	for(DepotMap::iterator it = player->depots.begin(); it != player->depots.end(); ++it)
		itemList.push_back(itemBlock(it->first, it->second));

	//save depot items
	query.str("INSERT INTO `player_depotitems` (`player_id` , `pid` , `sid` , `itemtype` , `count` , `attributes` ) VALUES (");
	query << player->getGUID() << ", ?, ?, ?, ?, ?)";

	if(!(stmt = db->prepareStatement(query.str())))
		return false;

	if(!saveItems(player, itemList, stmt)) {
		db->freeStatement(stmt);
		db->rollback();
		return false;
	}

	db->freeStatement(stmt);

	query.str("INSERT INTO `player_storage` (`player_id` , `key` , `value` ) VALUES (");
	query << player->getGUID() << ", ?, ?)";

	if(!(stmt = db->prepareStatement(query.str())))
		return false;

	player->genReservedStorageRange();
	for(StorageMap::const_iterator cit = player->getStorageIteratorBegin(); cit != player->getStorageIteratorEnd();cit++) {
		stmt->setInt(1, cit->first);
		stmt->setInt(2, cit->second);

		if(!stmt->execute()) {
			db->freeStatement(stmt);
			db->rollback();
			return false;
		}
	}

	db->freeStatement(stmt);

	//save vip list
	query.str("INSERT INTO `player_viplist` (`player_id`, `vip_id`) VALUES (");
	query << player->getGUID() << ", ?)";

	if(!(stmt = db->prepareStatement(query.str())))
		return false;

	for(VIPListSet::iterator it = player->VIPList.begin(); it != player->VIPList.end(); it++) {
		stmt->setInt(1, *it );

		if(!stmt->execute()) {
			db->freeStatement(stmt);
			db->rollback();
			return false;
		}
	}

	db->freeStatement(stmt);

	//End the transaction
	return db->commit();
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

	if(!result->next()) {
		db.freeResult(result);
		return false;
	}

	nameCacheMap[guid] = result->getDataString("name");
	db.freeResult(result);
	return true;
}

bool IOPlayer::getNameByGuid(uint32_t guid, std::string& name)
{
	NameCacheMap::iterator it = nameCacheMap.find(guid);
	if(it != nameCacheMap.end()) {
		name = it->second;
		return true;
	}

	Database* db = Database::instance();
	DBQuery query;
	DBResult* result;

	query << "SELECT `name` FROM `players` WHERE `id` = " << guid;

	if(!(result = db->storeQuery(query.str())))
		return false;

	if(!result->next()) {
		db->freeResult(result);
		return false;
	}

	name = result->getDataString("name");
	nameCacheMap[guid] = name;
	db->freeResult(result);
	return true;
}

bool IOPlayer::getGuidByName(uint32_t &guid, std::string& name)
{
	GuidCacheMap::iterator it = guidCacheMap.find(name);
	if(it != guidCacheMap.end()) {
		name = it->first;
		guid = it->second;
		return true;
	}

	Database* db = Database::instance();
	DBResult* result;
	DBQuery query;

	if(!(result = db->storeQuery("SELECT `name`, `id` FROM `players` WHERE `name` = " + db->escapeString(name))))
		return false;

	if(!result->next()) {
		db->freeResult(result);
		return false;
	}

	name = result->getDataString("name");
	guid = result->getDataInt("id");

	guidCacheMap[name] = guid;
	db->freeResult(result);
	return true;
}


bool IOPlayer::getGuidByNameEx(uint32_t &guid, bool &specialVip, std::string& name)
{
	Database* db = Database::instance();
	DBResult* result;
	DBQuery query;

	if(!(result = db->storeQuery("SELECT `name`, `id`, `group_id` FROM `players` WHERE `name`= " + db->escapeString(name))))
		return false;

	if(!result->next()) {
		db->freeResult(result);
		return false;
	}

	name = result->getDataString("name");
	guid = result->getDataInt("id");
	const PlayerGroup* group = getPlayerGroup(result->getDataInt("group_id"));

	if(group)
		specialVip = (0 != (group->m_flags & ((uint64_t)1 << PlayerFlag_SpecialVIP)));
	else
		specialVip = false;

	db->freeResult(result);
	return true;
}

bool IOPlayer::getGuildIdByName(uint32_t &guildId, const std::string& guildName)
{
	Database* db = Database::instance();
	DBResult* result;
	DBQuery query;

	if(!(result = db->storeQuery("SELECT `id` FROM `guilds` WHERE `name` = " + db->escapeString(guildName))))
		return false;

	if(result->next()) {
		guildId = result->getDataInt("id");
		db->freeResult(result);
		return true;
	}

	db->freeResult(result);
	return false;
}

bool IOPlayer::playerExists(std::string name)
{
	Database* db = Database::instance();
	DBResult* result;
	DBQuery query;

	if(!(result = db->storeQuery("SELECT `id` FROM `players` WHERE `name`= " + db->escapeString(name))))
		return false;

	if(result->next()) {
		db->freeResult(result);
		return true;
	}

	db->freeResult(result);
	return false;
}

const PlayerGroup* IOPlayer::getPlayerGroup(uint32_t groupid)
{
	PlayerGroupMap::const_iterator it = playerGroupMap.find(groupid);

	if(it != playerGroupMap.end()){
		return it->second;
	} else {
		Database* db = Database::instance();
		DBQuery query;
		DBResult* result;

		query << "SELECT * FROM `groups` WHERE `id`= " << groupid;

		if(result = db->storeQuery(query.str())) {
			if(result->next()) {
				PlayerGroup* group = new PlayerGroup;

				group->m_name = result->getDataString("name");
				group->m_flags = result->getDataLong("flags");
				group->m_access = result->getDataInt("access");
				group->m_maxDepotItems = result->getDataInt("maxdepotitems");
				group->m_maxVip = result->getDataInt("maxviplist");

				playerGroupMap[groupid] = group;

				db->freeResult(result);
				return group;
			}
			db->freeResult(result);
		}
	}
	return NULL;
}

void IOPlayer::loadItems(ItemMap& itemMap, DBResult* result)
{
	while(result->next()) {
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
	}
}

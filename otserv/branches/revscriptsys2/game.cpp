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

#include "game.h"
#include "scheduler.h"
#include "tasks.h"
#include "player.h"
#include "actor.h"
#include "tile.h"
#include "combat.h"
#include "ioplayer.h"
#include "account.h"
#include "ioaccount.h"
#include "chat.h"
#include "server.h"
#include "party.h"
#include "ban.h"
#include "spawn.h"
#include "beds.h"
#include "house.h"
#include "depot.h"
#include "creature_manager.h"
#include "script_environment.h"
#include "script_manager.h"
#include "script_event.h"
#include "configmanager.h"

#if defined __EXCEPTION_TRACER__
#include "exception.h"
extern boost::recursive_mutex maploadlock;
#endif

extern ConfigManager g_config;
extern Commands commands;
extern BanManager g_bans;
extern CreatureManager g_creature_types;
extern Chat g_chat;
extern Game g_game;

Game::Game()
{
	gameState = GAME_STATE_NORMAL;
	map = NULL;
	worldType = WORLD_TYPE_PVP;

	checkLightEvent = 0;
	checkCreatureEvent = 0;
	checkDecayEvent = 0;

	last_bucket = 0;
	int daycycle = 3600;
	//(1440 minutes/day)/(3600 seconds/day)*10 seconds event interval
	light_hour_delta = 1440*10/daycycle;
	/*light_hour = 0;
	lightlevel = LIGHT_LEVEL_NIGHT;
	light_state = LIGHT_STATE_NIGHT;*/
	light_hour = SUNRISE + (SUNSET - SUNRISE)/2;
	lightlevel = LIGHT_LEVEL_DAY;
	light_state = LIGHT_STATE_DAY;

	script_system = NULL;
	script_environment = NULL;
	waitingScriptEvent = 0;
}

void Game::start(ServiceManager* servicer)
{
	service_manager = servicer;

	checkLightEvent =
		g_scheduler.addEvent(createSchedulerTask(EVENT_LIGHTINTERVAL,
		boost::bind(&Game::checkLight, this)));
	checkCreatureLastIndex = 0;

	checkCreatureEvent =
		g_scheduler.addEvent(createSchedulerTask(EVENT_CREATURE_THINK_INTERVAL,
		boost::bind(&Game::checkCreatures, this)));

	checkDecayEvent =
		g_scheduler.addEvent(createSchedulerTask(EVENT_DECAYINTERVAL,
		boost::bind(&Game::checkDecay, this)));

	waitingScriptEvent = g_scheduler.addEvent(createSchedulerTask(EVENT_SCRIPT_CLEANUP_INTERVAL,
		boost::bind(&Game::scriptCleanup, this)));
}

Game::~Game()
{
	script_environment->cleanup();
	delete script_environment;
	delete script_system;
	if(map){
		delete map;
	}
}

void Game::setWorldType(WorldType type)
{
	worldType = type;
}

GameState_t Game::getGameState()
{
	return gameState;
}

void Game::setGameState(GameState_t newState)
{
	if(gameState == GAME_STATE_SHUTDOWN){
		//Can't go back from this state.
		return;
	}

	if(gameState != newState){
		switch(newState){
			case GAME_STATE_INIT:
			{
				Spawns::getInstance()->startup();

				loadGameState();
				break;
			}

			case GAME_STATE_SHUTDOWN:
			{
				//kick all players that are still online
				AutoList<Player>::listiterator it = Player::listPlayer.list.begin();
				while(it != Player::listPlayer.list.end()){
					(*it).second->kickPlayer();
					it = Player::listPlayer.list.begin();
				}

				runShutdownScripts(true);

				saveGameState();

				g_dispatcher.addTask(createTask(
					boost::bind(&Game::shutdown, this)));
				g_scheduler.stop();
				g_dispatcher.stop();
				break;
			}

			case GAME_STATE_CLOSED:
			{
				g_bans.clearTemporaryBans();
				break;
			}

			case GAME_STATE_STARTUP:
			case GAME_STATE_CLOSING:
			case GAME_STATE_NORMAL:
			default:
				break;
		}
		gameState = newState;
	}
}

void Game::saveGameState()
{
	// REVSCRIPT TODO Save global storage
}

bool Game::saveServer(bool payHouses, bool shallowSave /*=false*/)
{
	uint64_t start = OTSYS_TIME();

	saveGameState();

	for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin();
		it != Player::listPlayer.list.end();
		++it)
	{
		it->second->loginPosition = it->second->getPosition();
		IOPlayer::instance()->savePlayer(it->second, shallowSave);
	}

	if(shallowSave)
		return true;

	if(payHouses){
		Houses::getInstance().payHouses();
	}

	bool ret = map->saveMap();

	std::cout << "Notice: Server saved. Process took " <<
		(OTSYS_TIME() - start)/(1000.) << "s." << std::endl;

	return ret;
}

void Game::loadGameState()
{
	// REVSCRIPT TODO Load global storage
}

int Game::loadMap(std::string filename, std::string filekind)
{
	if(!map){
		map = new Map;
	}

	maxPlayers = g_config.getNumber(ConfigManager::MAX_PLAYERS);
	inFightTicks = g_config.getNumber(ConfigManager::IN_FIGHT_DURATION);
	exhaustionTicks = g_config.getNumber(ConfigManager::EXHAUSTED);
	addExhaustionTicks = g_config.getNumber(ConfigManager::EXHAUSTED_ADD);
	fightExhaustionTicks = g_config.getNumber(ConfigManager::COMBAT_EXHAUSTED);
	healExhaustionTicks = g_config.getNumber(ConfigManager::HEAL_EXHAUSTED);
	stairhopExhaustion = g_config.getNumber(ConfigManager::STAIRHOP_EXHAUSTED);
	Player::maxMessageBuffer = g_config.getNumber(ConfigManager::MAX_MESSAGEBUFFER);
	Actor::despawnRange = g_config.getNumber(ConfigManager::DEFAULT_DESPAWNRANGE);
	Actor::despawnRadius = g_config.getNumber(ConfigManager::DEFAULT_DESPAWNRADIUS);

	return map->loadMap(filename, filekind);
}

void Game::runWaitingScripts()
{
	if(script_system){
		script_system->runScheduledThreads();
	}
	waitingScriptEvent = g_scheduler.addEvent(createSchedulerTask(EVENT_SCRIPT_TIMER_INTERVAL,
		boost::bind(&Game::runWaitingScripts, this)));
}

bool Game::loadScripts() 
{
	bool is_reload = false;
	// Unload any old
	if(script_environment || script_system) {
		// Remove all old event listeners
		for(AutoList<Creature>::listiterator it = Game::listCreature.list.begin();
			it != Game::listCreature.list.end();
			++it)
		{
			it->second->clearListeners();
		}
		delete script_environment;
		delete script_system;
		script_environment = NULL;
		script_system = NULL;

		g_scheduler.stopEvent(waitingScriptEvent);
		is_reload = true;
	}

	// Load fresh!
	try {
		script_environment = new Script::Environment();
		script_system = new Script::Manager(*script_environment);
		script_system->loadFile(g_config.getString(ConfigManager::DATA_DIRECTORY) + "scripts/main.lua");

		waitingScriptEvent = g_scheduler.addEvent(createSchedulerTask(EVENT_SCRIPT_TIMER_INTERVAL,
			boost::bind(&Game::runWaitingScripts, this)));
	} catch(Script::Error&) {
		// Clear any listeners that were tied before the exception was thrown
		for(AutoList<Creature>::listiterator it = Game::listCreature.list.begin();
			it != Game::listCreature.list.end();
			++it)
		{
			it->second->clearListeners();
		}

		// Remove environment
		delete script_environment;
		delete script_system;
		script_environment = NULL;
		script_system = NULL;

		return false;
	}
	return true;
}

void Game::runStartupScripts(bool real_startup)
{
	if(!script_system)
		return;
	// This is run after the map is loaded.
	Script::OnServerLoad::Event evt(real_startup);
	script_system->dispatchEvent(evt);
}

void Game::runShutdownScripts(bool real_shutdown)
{
	if(!script_system)
		return;
	// This is run after the the scripts are /reloaded or when server shuts down
	Script::OnServerUnload::Event evt(real_shutdown);
	script_system->dispatchEvent(evt);
}

void Game::scriptCleanup()
{
	script_environment->cleanupUnusedListeners();

	g_scheduler.addEvent(createSchedulerTask(EVENT_SCRIPT_CLEANUP_INTERVAL,
		boost::bind(&Game::scriptCleanup, this)));
}

void Game::refreshMap(Map::TileMap::iterator* map_iter, int clean_max)
{
	Tile* tile;
	Item* item;


	Map::TileMap::iterator begin_here = map->refreshTileMap.begin();
	if(!map_iter)
		map_iter = &begin_here;
	Map::TileMap::iterator end_here = map->refreshTileMap.end();
	int cleaned = 0;
	for(; *map_iter != end_here && (clean_max == 0? true : (cleaned < clean_max)); ++*map_iter, ++cleaned){
		tile = (*map_iter)->first;

		//remove garbage
		int32_t downItemSize = tile->items_downCount();
		for(int32_t i = downItemSize - 1; i >= 0; --i){
			item = tile->items_get(i);
			if(item){
				ReturnValue ret = internalRemoveItem(NULL, item);
				if(ret != RET_NOERROR){
					std::cout << "Could not refresh item: " << item->getID() << "pos: " << tile->getPosition() << std::endl;
				}
			}
		}

		cleanup();

		//restore to original state
		/*
		TileItemVector list = (*map_iter)->second.list;
		for(TileItemReverseIterator it = list.rbegin(); it != list.rend(); ++it){
			Item* item = (*it)->clone();
			ReturnValue ret = internalAddItem(NULL, tile, item , INDEX_WHEREEVER, FLAG_NOLIMIT);
			if(ret == RET_NOERROR){
				// REVSCRIPT TODO Add unique items
				//if(item->getUniqueId() != 0){
				//	ScriptEnvironment::addUniqueThing(item);
				//}
				startDecay(item);
			}
			else{
				std::cout << "Could not refresh item: " << item->getID() << "pos: " << tile->getPosition() << std::endl;
				delete item;
			}
		}
		*/
	}
}

/*****************************************************************************/

void Game::proceduralRefresh(Map::TileMap::iterator* begin)
{
	if(!begin)
		begin = new Map::TileMap::iterator(map->refreshTileMap.begin());

	// Refresh 250 tiles each cycle
	refreshMap(begin, 250);

	if(*begin == map->refreshTileMap.end()){
		delete begin;
		return;
	}

	// Refresh some items every 100 ms until all tiles has been checked
	// For 100k tiles, this would take 100000/2500 = 40s = half a minute
	g_scheduler.addEvent(createSchedulerTask(100,
		boost::bind(&Game::proceduralRefresh, this, begin)));
}

ReturnValue Game::canUse(const Player* player, const Position& pos)
{
	const Position& playerPos = player->getPosition();

	if(pos.x != 0xFFFF){
		if(playerPos.z > pos.z){
			return RET_FIRSTGOUPSTAIRS;
		}
		else if(playerPos.z < pos.z){
			return RET_FIRSTGODOWNSTAIRS;
		}
		else if(!Position::areInRange<1,1,0>(playerPos, pos)){
			return RET_TOOFARAWAY;
		}
	}

	return RET_NOERROR;
}

ReturnValue Game::canUseFar(const Creature* creature, const Position& toPos, bool checkLineOfSight)
{
	if(toPos.x == 0xFFFF){
		return RET_NOERROR;
	}

	const Position& creaturePos = creature->getPosition();

	if(creaturePos.z > toPos.z){
		return RET_FIRSTGOUPSTAIRS;
	}
	else if(creaturePos.z < toPos.z){
		return RET_FIRSTGODOWNSTAIRS;
	}
	else if(!Position::areInRange<7,5,0>(toPos, creaturePos)){
		return RET_TOOFARAWAY;
	}

	if(checkLineOfSight && !canThrowObjectTo(creaturePos, toPos)){
		return RET_CANNOTTHROW;
	}

	return RET_NOERROR;
}

ReturnValue Game::internalUseItem(Player* player, const Position& pos,
	uint8_t index, Item* item, uint32_t creatureId)
{
	bool foundAction = false; //(getAction(item) != NULL);

	//check if it is a house door
	if(Door* door = item->getDoor()){
		if(!door->canUse(player)){
			return RET_CANNOTUSETHISOBJECT;
		}
	}
	if(BedItem* bed = item->getBed()) {
		if(!bed->canUse(player)) {
			return RET_CANNOTUSETHISOBJECT;
		}
		bed->sleep(player);
		return RET_NOERROR;
	}

	ReturnValue retval = RET_NOERROR;
	if(script_system){
		Script::OnUseItem::Event evt(player, item, NULL, retval);
		if(script_system->dispatchEvent(evt)) {
			return retval;
		}
	}

	if(item->isReadable()){
		if(item->canWriteText()){
			player->setWriteItem(item, item->getMaxWriteLength());
			player->sendTextWindow(item, item->getMaxWriteLength(), true);
		}
		else{
			player->setWriteItem(NULL);
			player->sendTextWindow(item, 0, false);
		}

		return RET_NOERROR;
	}

	//if it is a container try to open it
	if(Container* container = item->getContainer()){
		if(openContainer(player, container, index)){
			return RET_NOERROR;
		}
	}

	if(!foundAction){
		return RET_CANNOTUSETHISOBJECT;
	}

	return RET_NOERROR;
}

bool Game::useItem(Player* player, const Position& pos, uint8_t index,
	Item* item, bool isHotkey)
{
	if(!player->canDoAction()){
		return false;
	}
	
	player->stopWalk();

	if(isHotkey){
		int32_t subType = -1;
		if(item->hasSubType() && !item->hasCharges()){
			subType = item->getSubType();
		}

		const ItemType& it = Item::items[item->getID()];
		uint32_t itemCount = player->__getItemTypeCount(item->getID(), subType, false);
		ReturnValue ret = internalUseItem(player, pos, index, item, 0);
		if(ret != RET_NOERROR){
			player->sendCancelMessage(ret);
			return false;
		}

		showUseHotkeyMessage(player, it, itemCount);
	}
	else{
		ReturnValue ret = internalUseItem(player, pos, index, item, 0);
		if(ret != RET_NOERROR){
			player->sendCancelMessage(ret);
			return false;
		}
	}

	player->setNextAction(OTSYS_TIME() + g_config.getNumber(ConfigManager::MIN_ACTIONTIME));
	return true;
}

ReturnValue Game::internalUseItemEx(Player* player, const PositionEx& fromPosEx, const PositionEx& toPosEx,
	Item* item, bool isHotkey, uint32_t creatureId, bool& isSuccess)
{
	isSuccess = false;

	ReturnValue retval = RET_NOERROR;
	if(script_system){
		Script::OnUseItem::Event evt(player, item, &toPosEx, retval);
		if(script_system->dispatchEvent(evt)) {
			return retval;
		}
	}

	return RET_CANNOTUSETHISOBJECT;
}

bool Game::useItemEx(Player* player, const Position& fromPos, uint16_t fromSpriteId, const Position& toPos,
	uint8_t toStackPos, uint16_t toSpriteId, Item* item, bool isHotkey, uint32_t creatureId /* = 0*/)
{
	if(!player->canDoAction()){
		return false;
	}

	player->stopWalk();

	int32_t fromStackPos = item->getParent()->__getIndexOfThing(item);
	PositionEx fromPosEx(fromPos, fromStackPos);
	PositionEx toPosEx(toPos, toStackPos);
	ReturnValue ret = RET_NOERROR;
	bool isSuccess = false;

	if(isHotkey){
		int32_t subType = -1;
		if(item->hasSubType() && !item->hasCharges()){
			subType = item->getSubType();
		}

		const ItemType& it = Item::items[item->getID()];
		uint32_t itemCount = player->__getItemTypeCount(item->getID(), subType, false);
		ret = internalUseItemEx(player, fromPosEx, toPosEx, item, isHotkey, creatureId, isSuccess);

		if(isSuccess){
			showUseHotkeyMessage(player, it, itemCount);
		}
	}
	else{
		ret = internalUseItemEx(player, fromPosEx, toPosEx, item, isHotkey, creatureId, isSuccess);
		
		if(ret == RET_TOOFARAWAY) {
			return useItemFarEx(player->getID(), fromPos, fromStackPos, fromSpriteId,
				toPos, toStackPos, toSpriteId, isHotkey);
		}
	}

	if(ret != RET_NOERROR){
		player->sendCancelMessage(ret);
		return false;
	}

	player->setNextAction(OTSYS_TIME() + g_config.getNumber(ConfigManager::MIN_ACTIONEXTIME));
	return true;
}

bool Game::internalUseItemFarEx(uint32_t playerId, const Position& fromPos, uint8_t fromStackPos, uint16_t fromSpriteId,
	const Position& toPos, uint8_t toStackPos, uint16_t toSpriteId, bool isHotkey, uint32_t creatureId)
{
	Player* player = getPlayerByID(playerId);
	if(!player || player->isRemoved())
		return false;

	Thing* thing = internalGetThing(player, fromPos, fromStackPos, fromSpriteId);
	if(!thing){
		player->sendCancelMessage(RET_NOTPOSSIBLE);
		return false;
	}

	Item* item = thing->getItem();
	if(!item || item->getClientID() != fromSpriteId || !item->isUseable()){
		player->sendCancelMessage(RET_CANNOTUSETHISOBJECT);
		return false;
	}

	Position itemPos = fromPos;
	uint8_t itemStackPos = fromStackPos;

	if(fromPos.x != 0xFFFF && toPos.x != 0xFFFF && Position::areInRange<1,1,0>(fromPos, player->getPosition()) &&
		!Position::areInRange<1,1,0>(fromPos, toPos))
	{
		//need to pickup the item first
		Item* moveItem = NULL;
		ReturnValue ret = internalMoveItem(player, item->getParent(), player, INDEX_WHEREEVER,
			item, item->getItemCount(), &moveItem);
		if(ret != RET_NOERROR){
			player->sendCancelMessage(ret);
			return false;
		}

		//changing the position since its now in the inventory of the player
		internalGetPosition(moveItem, itemPos, itemStackPos);
	}

	std::list<Direction> listDir;
	if(getPathToEx(player, toPos, listDir, 0, 1, true, true)){
		g_dispatcher.addTask(createTask(boost::bind(&Game::playerAutoWalk,
			this, playerId, listDir)));

		SchedulerTask* task = NULL;
		if(creatureId != 0) {
			task = createSchedulerTask(400, boost::bind(&Game::playerUseBattleWindow, this,
				playerId, fromPos, fromStackPos, creatureId, fromSpriteId, isHotkey));
		} else {
			task = createSchedulerTask(400, boost::bind(&Game::playerUseItemEx, this,
				playerId, itemPos, itemStackPos, fromSpriteId, toPos, toStackPos, toSpriteId, isHotkey));
		}
		player->setNextWalkActionTask(task);
		return true;
	}
	else{
		player->sendCancelMessage(RET_THEREISNOWAY);
		return false;
	}
}

void Game::showUseHotkeyMessage(Player* player, const ItemType& it, uint32_t itemCount)
{
	std::stringstream ss;
	if(itemCount == 1){
		ss << "Using the last " << it.name << "...";
	}
	else{
		ss << "Using one of " << itemCount << " " << it.pluralName << "...";
	}

	player->sendTextMessage(MSG_INFO_DESCR, ss.str());
}

bool Game::openContainer(Player* player, Container* container, const uint8_t index)
{
	Container* openContainer = NULL;

	//depot container
	if(Depot* depot = container->getDepot()){
		Depot* myDepot = player->getDepot(depot->getDepotId(), true);
		myDepot->setParent(depot->getParent());
		openContainer = myDepot;
	}
	else{
		openContainer = container;
	}

	if(container->getCorpseOwner() != 0){
		if(!player->canOpenCorpse(container->getCorpseOwner())){
			player->sendCancel("You are not the owner.");
			return true;
		}
	}

	//open/close container
	int32_t oldcid = player->getContainerID(openContainer);
	if(oldcid != -1){
		player->onCloseContainer(openContainer);
		player->closeContainer(oldcid);
	}
	else{
		player->addContainer(index, openContainer);
		player->onSendContainer(openContainer);
	}

	return true;
}

/*****************************************************************************/

Cylinder* Game::internalGetCylinder(Player* player, const Position& pos)
{
	if(pos.x != 0xFFFF){
		return getTile(pos.x, pos.y, pos.z);
	}
	else{
		//container
		if(pos.y & 0x40){
			uint8_t from_cid = pos.y & 0x0F;
			return player->getContainer(from_cid);
		}
		//inventory
		else{
			return player;
		}
	}
}

Thing* Game::internalGetThing(Player* player, const Position& pos, int32_t index,
	uint32_t spriteId /*= 0*/, stackPosType_t type /*= STACKPOS_NORMAL*/)
{
	if(pos.x != 0xFFFF){
		Tile* tile = getTile(pos.x, pos.y, pos.z);

		if(tile){
			/*look at*/
			if(type == STACKPOS_LOOK){
				return tile->getTopVisibleThing(player);
			}

			Thing* thing = NULL;

			/*for move operations*/
			if(type == STACKPOS_MOVE){
				Item* item = tile->items_firstDown();
				if(item && item->isMoveable())
					thing = item;
				else
					thing = tile->getTopVisibleCreature(player);
			}
			/*use item*/
			else if(type == STACKPOS_USE){
				thing = tile->items_firstDown();
			}
			else if(type == STACKPOS_USEITEM){
				//First check items with topOrder 2 (ladders, signs, splashes)
				Item* item =  tile->getItemByTopOrder(2);
				if(item){// && g_actions->hasAction(item)){
					thing = item;
				}
				else{
					//then down items
					thing = tile->items_firstDown();
					if(thing == NULL){
						//then last we check items with topOrder 3 (doors etc)
						thing = tile->items_firstTop();
					}

					if(thing == NULL){
						thing = tile->ground;
					}
				}
			}
			else{
				thing = tile->__getThing(index);
			}

			if(player){
				//do extra checks here if the thing is accessable
				if(thing && thing->getItem()){
					if(tile->isVertical()){
						if(player->getPosition().x + 1 == tile->getPosition().x){
							thing = NULL;
						}
					}
					else if(tile->isHorizontal()){
						if(player->getPosition().y + 1 == tile->getPosition().y){
							thing = NULL;
						}
					}
				}
			}

			return thing;
		}
	}
	else{
		//from inside a container
		if(pos.y & 0x40){
			uint8_t fromCid = pos.y & 0x0F;
			uint8_t slot = pos.z;

			Container* parentcontainer = player->getContainer(fromCid);
			if(!parentcontainer)
				return NULL;

			return parentcontainer->getItem(slot);
		}
		else if(pos.y == 0 && pos.z == 0){
			const ItemType& it = Item::items.getItemIdByClientId(spriteId);
			if(it.id == 0){
				return NULL;
			}

			int32_t subType = -1;
			if(it.isFluidContainer()){
				int32_t maxFluidType = sizeof(reverseFluidMap) / sizeof(uint8_t);
				if(index < maxFluidType){
					subType = reverseFluidMap[index];
				}
			}

			return findItemOfType(player, it.id, true, subType);
		}
		//inventory
		else{
			SlotType slot = (SlotType)static_cast<unsigned char>(pos.y);
			return player->getInventoryItem(slot);
		}
	}

	return NULL;
}

void Game::internalGetPosition(Item* item, Position& pos, uint8_t& stackpos)
{
	pos.x = 0;
	pos.y = 0;
	pos.z = 0;
	stackpos = 0;

	Cylinder* topParent = item->getTopParent();
	if(topParent){
		if(Player* player = dynamic_cast<Player*>(topParent)){
			pos.x = 0xFFFF;

			Container* container = dynamic_cast<Container*>(item->getParent());
			if(container){
				pos.y = ((uint16_t) ((uint16_t)0x40) | ((uint16_t)player->getContainerID(container)) );
				pos.z = container->__getIndexOfThing(item);
				stackpos = pos.z;
			}
			else{
				pos.y = player->__getIndexOfThing(item);
				stackpos = pos.y;
			}
		}
		else if(Tile* tile = topParent->getTile()){
			pos = tile->getPosition();
			stackpos = tile->__getIndexOfThing(item);
		}
	}
}

void Game::setTile(Tile* newtile)
{
	return map->setTile(newtile->getPosition(), newtile);
}

Tile* Game::getTile(int32_t x, int32_t y, int32_t z)
{
	return map->getTile(x, y, z);
}

Tile* Game::getTile(const Position& pos)
{
	return map->getTile(pos);
}

QTreeLeafNode* Game::getLeaf(uint32_t x, uint32_t y)
{
	return map->getLeaf(x, y);
}

Creature* Game::getCreatureByID(uint32_t id)
{
	if(id == 0)
		return NULL;

	AutoList<Creature>::listiterator it = listCreature.list.find(id);
	if(it != listCreature.list.end()){
		if(!it->second->isRemoved())
			return it->second;
	}

	return NULL; //just in case the player doesnt exist
}

Player* Game::getPlayerByID(uint32_t id)
{
	if(id == 0)
		return NULL;

	AutoList<Player>::listiterator it = Player::listPlayer.list.find(id);
	if(it != Player::listPlayer.list.end()) {
		if(!it->second->isRemoved())
			return it->second;
	}

	return NULL; //just in case the player doesnt exist
}

Creature* Game::getCreatureByName(const std::string& s)
{
	std::string txt1 = asUpperCaseString(s);
	for(AutoList<Creature>::listiterator it = listCreature.list.begin(); it != listCreature.list.end(); ++it){
		if(!it->second->isRemoved()){
			std::string txt2 = asUpperCaseString(it->second->getName());
			if(txt1 == txt2)
				return it->second;
		}
	}

	return NULL; //just in case the creature doesnt exist
}

std::vector<Creature*> Game::getCreaturesByName(const std::string& s)
{
	std::vector<Creature*> creaturesFound;
	std::string txt1 = asUpperCaseString(s);
	for(AutoList<Creature>::listiterator it = listCreature.list.begin(); it != listCreature.list.end(); ++it){
		if(!it->second->isRemoved()){
			std::string txt2 = asUpperCaseString(it->second->getName());
			if(txt1 == txt2)
				creaturesFound.push_back(it->second);
		}
	}

	return creaturesFound;
}

Player* Game::getPlayerByName(const std::string& s)
{
	std::string txt1 = asUpperCaseString(s);
	for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it){
		if(!it->second->isRemoved()){
			std::string txt2 = asUpperCaseString(it->second->getName());
			if(txt1 == txt2)
				return it->second;
		}
	}

	return NULL; //just in case the player doesnt exist
}

std::vector<Player*> Game::getPlayersByName(const std::string& s)
{
	std::vector<Player*> playersFound;
	std::string txt1 = asUpperCaseString(s);
	for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it){
		if(!it->second->isRemoved()){
			std::string txt2 = asUpperCaseString(it->second->getName());
			if(txt1 == txt2)
				playersFound.push_back(it->second);
		}
	}

	return playersFound;
}

Player* Game::getPlayerByNameEx(const std::string& s)
{
	Player* player = getPlayerByName(s);
	if(player){
		return player;
	}

	if(IOPlayer::instance()->playerExists(s)){
		player = new Player(s, NULL);
		if(!IOPlayer::instance()->loadPlayer(player, s)){
#ifdef __DEBUG__
			std::cout << "Failure: [Game::getPlayerByNameEx], can not load player: " << s << std::endl;
#endif
			delete player;
			return NULL;
		}
	}

	return player;
}

Player* Game::getPlayerByGuid(uint32_t guid)
{
	if(guid == 0){
		return NULL;
	}

	for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it){
		if(!it->second->isRemoved()){
			if(it->second->getGUID() == guid){
				return it->second;
			}
		}
	}

	return NULL;
}

Player* Game::getPlayerByGuidEx(uint32_t guid)
{
	Player* player = getPlayerByGuid(guid);
	if(player){
		return player;
	}

	std::string name;
	if(IOPlayer::instance()->getNameByGuid(guid, name)){
		player = new Player(name, NULL);
		if(!IOPlayer::instance()->loadPlayer(player, name)){
#ifdef __DEBUG__
			std::cout << "Failure: [Game::getPlayerByGuidEx], can not load player: " << name << std::endl;
#endif
			delete player;
			player = NULL;
		}
	}

	return player;
}

ReturnValue Game::getPlayerByNameWildcard(const std::string& s, Player* &player)
{
	player = NULL;

	if(s.empty()){
		return RET_PLAYERWITHTHISNAMEISNOTONLINE;
	}

	if((*s.rbegin()) != '~'){
		player = getPlayerByName(s);
		if(!player){
			return RET_PLAYERWITHTHISNAMEISNOTONLINE;
		}
		return RET_NOERROR;
	}

	Player* lastFound = NULL;
	std::string txt1 = asUpperCaseString(s.substr(0, s.length()-1));

	for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it){
		if(!it->second->isRemoved()){
			std::string txt2 = asUpperCaseString(it->second->getName());
			if(txt2.substr(0, txt1.length()) == txt1){
				if(lastFound == NULL)
					lastFound = it->second;
				else
					return RET_NAMEISTOOAMBIGIOUS;
			}
		}
	}

	if(lastFound != NULL){
		player = lastFound;
		return RET_NOERROR;
	}

	return RET_PLAYERWITHTHISNAMEISNOTONLINE;
}

std::vector<Player*> Game::getPlayersByNameWildcard(const std::string& s)
{
	if(s.empty())
		return std::vector<Player*>();

	if((*s.rbegin()) != '~')
		return getPlayersByName(s);

	std::vector<Player*> playersFound;
	std::string txt1 = asUpperCaseString(s.substr(0, s.length()-1));

	for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it){
		if(!it->second->isRemoved()){
			std::string txt2 = asUpperCaseString(it->second->getName());
			if(txt2.substr(0, txt1.length()) == txt1){
				playersFound.push_back(it->second);
			}
		}
	}

	return playersFound;
}

Player* Game::getPlayerByAccount(uint32_t acc)
{
	for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it){
		if(!it->second->isRemoved()){
			if(it->second->getAccountId() == acc){
				return it->second;
			}
		}
	}

	return NULL;
}

PlayerVector Game::getPlayersByAccount(uint32_t acc)
{
	PlayerVector players;
	for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it){
		if(!it->second->isRemoved()){
			if(it->second->getAccountId() == acc){
				players.push_back(it->second);
			}
		}
	}

	return players;
}

PlayerVector Game::getPlayersByIP(uint32_t ipadress, uint32_t mask)
{
	PlayerVector players;
	for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it){
		if(!it->second->isRemoved()){
			if((it->second->getIP() & mask) == (ipadress & mask)){
				players.push_back(it->second);
			}
		}
	}

	return players;
}

bool Game::internalPlaceCreature(Creature* creature, const Position& pos, bool extendedPos /*=false*/, bool forced /*= false*/)
{
	if(creature->getParent() != NULL){
		return false;
	}

	if(!map->placeCreature(pos, creature, extendedPos, forced)){
		return false;
	}

	//std::cout << "internalPlaceCreature: " << creature << " " << creature->getID() << std::endl;

	creature->addRef();
	creature->setID();
	listCreature.addList(creature);
	creature->addList();
	return true;
}

bool Game::placeCreature(Creature* creature, const Position& pos, bool extendedPos /*=false*/, bool forced /*= false*/)
{
	if(!internalPlaceCreature(creature, pos, extendedPos, forced)){
		return false;
	}

	SpectatorVec list;
	SpectatorVec::iterator it;
	getSpectators(list, creature->getPosition(), false, true);

	//send to client
	Player* tmpPlayer = NULL;
	for(it = list.begin(); it != list.end(); ++it) {
		if((tmpPlayer = (*it)->getPlayer())){
			tmpPlayer->sendCreatureAppear(creature, creature->getPosition());
		}
	}

	//event method
	for(it = list.begin(); it != list.end(); ++it) {
		(*it)->onCreatureAppear(creature, true);
	}

	int32_t newIndex = creature->getParent()->__getIndexOfThing(creature);
	creature->getParent()->postAddNotification(NULL, creature, NULL, newIndex);

	addCreatureCheck(creature);

	creature->onPlacedCreature();
	return true;
}

bool Game::removeCreature(Creature* creature, bool isLogout /*= true*/)
{
	if(creature->isRemoved())
		return false;

	Tile* tile = creature->getTile();

	SpectatorVec list;
	SpectatorVec::iterator it;
	getSpectators(list, tile->getPosition(), false, true);

	Player* player = NULL;
	std::vector<uint32_t> oldStackPosVector;
	for(it = list.begin(); it != list.end(); ++it){
		if((player = (*it)->getPlayer())){
			if(player->canSeeCreature(creature)){
				oldStackPosVector.push_back(tile->getClientIndexOfThing(player, creature));
			}
		}
	}

	int32_t oldIndex = tile->__getIndexOfThing(creature);
	if(!map->removeCreature(creature)){
		return false;
	}

	//send to client
	uint32_t i = 0;
	for(it = list.begin(); it != list.end(); ++it){
		if((player = (*it)->getPlayer())){
			if(player->canSeeCreature(creature)){
				player->sendCreatureDisappear(creature, oldStackPosVector[i], isLogout);
				++i;
			}
		}
	}

	//event method
	for(it = list.begin(); it != list.end(); ++it){
		(*it)->onCreatureDisappear(creature, isLogout);
	}

	creature->getParent()->postRemoveNotification(NULL, creature, NULL, oldIndex, true);

	listCreature.removeList(creature->getID());
	creature->onRemoved();
	FreeThing(creature);

	removeCreatureCheck(creature);

	for(std::list<Creature*>::iterator cit = creature->summons.begin(); cit != creature->summons.end(); ++cit){
		(*cit)->setLossSkill(false);
		removeCreature(*cit);
	}

	creature->onRemovedCreature();
	return true;
}

bool Game::playerMoveThing(uint32_t playerId, const Position& fromPos,
	uint16_t spriteId, uint8_t fromStackPos, const Position& toPos, uint8_t count)
{
	Player* player = getPlayerByID(playerId);
	if(!player || player->isRemoved())
		return false;

	uint8_t fromIndex = 0;
	if(fromPos.x == 0xFFFF){
		if(fromPos.y & 0x40){
			fromIndex = static_cast<uint8_t>(fromPos.z);
		}
		else{
			fromIndex = static_cast<uint8_t>(fromPos.y);
		}
	}
	else
		fromIndex = fromStackPos;

	Thing* thing = internalGetThing(player, fromPos, fromIndex, spriteId, STACKPOS_MOVE);
	Cylinder* toCylinder = internalGetCylinder(player, toPos);

	if(!thing || !toCylinder){
		player->sendCancelMessage(RET_NOTPOSSIBLE);
		return false;
	}

	if(Creature* movingCreature = thing->getCreature()){
		if(Position::areInRange<1,1,0>(movingCreature->getPosition(), player->getPosition())){
			SchedulerTask* task = createSchedulerTask(2000,
				boost::bind(&Game::playerMoveCreature, this, player->getID(),
				movingCreature->getID(), movingCreature->getPosition(), toCylinder->getPosition()));
			player->setNextActionTask(task);
		}
		else{
			playerMoveCreature(playerId, movingCreature->getID(), movingCreature->getPosition(),
				toCylinder->getPosition());
		}
	}
	else if(thing->getItem()){
		playerMoveItem(playerId, fromPos, spriteId, fromStackPos, toPos, count);
	}

	return true;
}

bool Game::playerMoveCreature(uint32_t playerId, uint32_t movingCreatureId,
	const Position& movingCreatureOrigPos, const Position& toPos)
{
	Player* player = getPlayerByID(playerId);
	if(!player || player->isRemoved() || player->hasFlag(PlayerFlag_CannotMoveCreatures))
		return false;

	if(!player->canDoAction()){
		uint32_t delay = player->getNextActionTime();
		SchedulerTask* task = createSchedulerTask(delay, boost::bind(&Game::playerMoveCreature, this, playerId, movingCreatureId,
			movingCreatureOrigPos, toPos));
		player->setNextActionTask(task);
		return false;
	}

	player->setNextActionTask(NULL);

	Creature* movingCreature = getCreatureByID(movingCreatureId);

	if(!movingCreature || movingCreature->isRemoved())
		return false;

	if(!Position::areInRange<1,1,0>(movingCreatureOrigPos, player->getPosition())){
		//need to walk to the creature first before moving it
		std::list<Direction> listDir;
		if(getPathToEx(player, movingCreatureOrigPos, listDir, 0, 1, true, true)){
			g_dispatcher.addTask(createTask(boost::bind(&Game::playerAutoWalk,
				this, player->getID(), listDir)));

			SchedulerTask* task = createSchedulerTask(1500, boost::bind(&Game::playerMoveCreature, this,
				playerId, movingCreatureId, movingCreatureOrigPos, toPos));
			player->setNextWalkActionTask(task);
			return true;
		}
		else{
			player->sendCancelMessage(RET_THEREISNOWAY);
			return false;
		}
	}

	Tile* toTile = getTile(toPos);
	const Position& movingCreaturePos = movingCreature->getPosition();

	if(!toTile){
		player->sendCancelMessage(RET_NOTPOSSIBLE);
		return false;
	}

	if(!movingCreature->isPushable() && !player->hasFlag(PlayerFlag_CanPushAllCreatures)){
		player->sendCancelMessage(RET_NOTMOVEABLE);
		return false;
	}

	//check throw distance
	if((std::abs(movingCreaturePos.x - toPos.x) > movingCreature->getThrowRange()) ||
			(std::abs(movingCreaturePos.y - toPos.y) > movingCreature->getThrowRange()) ||
			(std::abs(movingCreaturePos.z - toPos.z) * 4 > movingCreature->getThrowRange())){
		player->sendCancelMessage(RET_DESTINATIONOUTOFREACH);
		return false;
	}

	if(player != movingCreature){
		if(toTile->blockProjectile()){
			player->sendCancelMessage(RET_NOTENOUGHROOM);
			return false;
		}
		else if(movingCreature->getZone() == ZONE_PROTECTION && !toTile->hasFlag(TILEPROP_PROTECTIONZONE)){
			player->sendCancelMessage(RET_NOTPOSSIBLE);
			return false;
		}
	}

	ReturnValue ret = internalMoveCreature(player, movingCreature, movingCreature->getTile(), toTile);
	if(ret != RET_NOERROR){
		player->sendCancelMessage(ret);
		return false;
	}

	return true;
}

bool Game::onPlayerLogin(Player* player)
{
	if(!script_system)
		return false; // Not handled
	Script::OnLogin::Event evt(player);
	return script_system->dispatchEvent(evt);
}

bool Game::onPlayerLogout(Player* player, bool forced, bool timeout)
{
	if(!script_system)
		return false; // Not handled
	Script::OnLogout::Event evt(player, forced, timeout);
	return script_system->dispatchEvent(evt);
}

bool Game::onPlayerChangeOutfit(Player* player, OutfitType& outfit)
{
	if(!script_system)
		return false; // Not handled
	Script::OnChangeOutfit::Event evt(player, outfit);
	return script_system->dispatchEvent(evt);
}

bool Game::onPlayerEquipItem(Player* player, Item* item, SlotType slot, bool equip)
{
	if(!script_system)
		return false; // Not handled
	Script::OnEquipItem::Event evt(player, item, slot, equip);
	return script_system->dispatchEvent(evt);
}

bool Game::onPlayerAdvance(Player* player, LevelType skill, uint32_t oldLevel, uint32_t newLevel)
{
	if(!script_system)
		return false; // Not handled
	Script::OnAdvance::Event evt(player, skill, oldLevel, newLevel);
	return script_system->dispatchEvent(evt);
}

bool Game::onCreatureMove(Creature* actor, Creature* moving_creature, Tile* fromTile, Tile* toTile)
{
	if(!script_system)
		return false; // Not handled
	Script::OnMoveCreature::Event evt(actor, moving_creature, fromTile, toTile);
	return script_system->dispatchEvent(evt);
}

bool Game::onItemMove(Creature* actor, Item* item, Tile* tile, bool addItem)
{
	if(!script_system)
		return false; // Not handled
	Script::OnMoveItem::Event evt(actor, item, tile, addItem);
	return script_system->dispatchEvent(evt);
}

bool Game::onSpawn(Actor* actor)
{
	if(!script_system)
		return true; // Not handled
	Script::OnSpawn::Event evt(actor);
	return script_system->dispatchEvent(evt); // If handled, we don't spawn
}

void Game::onSpotCreature(Creature* creature, Creature* spotted)
{
	if(!script_system)
		return; // Not handled
	Script::OnSpotCreature::Event evt(creature, spotted);
	script_system->dispatchEvent(evt);
}

void Game::onLoseCreature(Creature* creature, Creature* lost)
{
	if(!script_system)
		return; // Not handled
	Script::OnLoseCreature::Event evt(creature, lost);
	script_system->dispatchEvent(evt);
}

void Game::onCreatureThink(Creature* creature, int interval)
{
	if(!script_system)
		return; // Not handled
	Script::OnThink::Event evt(creature, interval);
	script_system->dispatchEvent(evt);
}

void Game::onCreatureHear(Creature* listener, Creature* speaker, const SpeakClass& sclass, const std::string& text)
{
	if(!script_system)
		return; // Not handled
	if(listener != speaker){
		Script::OnHear::Event evt(listener, speaker, text, sclass);
		script_system->dispatchEvent(evt);
	}
}

bool Game::onCreatureAttack(Creature* creature, Creature* attacked)
{
	if(!script_system)
		return false; // Not handled
	Script::OnAttack::Event evt(creature, attacked);
	return script_system->dispatchEvent(evt);
}

bool Game::onCreatureDamage(CombatType& combatType, CombatSource& combatSource, Creature* creature, int32_t& value)
{
	if(!script_system)
		return false; // Not handled
	Script::OnDamage::Event evt(combatType, combatSource, creature, value);
	return script_system->dispatchEvent(evt);
}

bool Game::onCreatureKill(Creature* creature, Creature* killer)
{
	if(!script_system)
		return false; // Not handled
	Script::OnKill::Event evt(creature, killer);
	return script_system->dispatchEvent(evt);
}

bool Game::onCreatureDeath(Creature* creature, Item* corpse, Creature* killer)
{
	if(!script_system)
		return false; // Not handled
	Script::OnDeath::Event evt(creature, corpse, killer);
	return script_system->dispatchEvent(evt);
}

ReturnValue Game::internalMoveCreature(Creature* actor, Creature* creature, Direction direction, uint32_t flags /*= 0*/)
{
	Cylinder* fromTile = creature->getTile();
	Cylinder* toTile = NULL;

	const Position& currentPos = creature->getPosition();
	Position destPos = currentPos;

	bool canChangeFloor = true;
	switch(direction.value()){
		case enums::NORTH:
			destPos.y -= 1;
			break;

		case enums::SOUTH:
			destPos.y += 1;
			break;

		case enums::WEST:
			destPos.x -= 1;
			break;

		case enums::EAST:
			destPos.x += 1;
			break;

		case enums::SOUTHWEST:
			destPos.x -= 1;
			destPos.y += 1;
			canChangeFloor = false;
			break;

		case enums::NORTHWEST:
			destPos.x -= 1;
			destPos.y -= 1;
			canChangeFloor = false;
			break;

		case enums::NORTHEAST:
			destPos.x += 1;
			destPos.y -= 1;
			canChangeFloor = false;
			break;

		case enums::SOUTHEAST:
			destPos.x += 1;
			destPos.y += 1;
			canChangeFloor = false;
			break;
	}

	if(creature->getPlayer() && canChangeFloor){
		//try go up
		if(currentPos.z != 8 && creature->getTile()->hasHeight(3)){
			Tile* tmpTile = getTile(currentPos.x, currentPos.y, currentPos.z - 1);
			if(tmpTile == NULL || (tmpTile->ground == NULL && !tmpTile->blockSolid())){
				tmpTile = getTile(destPos.x, destPos.y, destPos.z - 1);
				if(tmpTile && tmpTile->ground && !tmpTile->blockSolid()){
					flags = flags | FLAG_IGNOREBLOCKITEM | FLAG_IGNOREBLOCKCREATURE;
					destPos.z -= 1;
				}
			}
		}
		else{
			//try go down
			Tile* tmpTile = getTile(destPos);
			if(currentPos.z != 7 && (tmpTile == NULL || (tmpTile->ground == NULL && !tmpTile->blockSolid()))){
				tmpTile = getTile(destPos.x, destPos.y, destPos.z + 1);

				if(tmpTile && tmpTile->hasHeight(3)){
					flags = flags | FLAG_IGNOREBLOCKITEM | FLAG_IGNOREBLOCKCREATURE;
					destPos.z += 1;
				}
			}
		}
	}

	toTile = getTile(destPos);

	ReturnValue ret = RET_NOTPOSSIBLE;
	if(toTile != NULL){
		ret = internalMoveCreature(actor, creature, fromTile, toTile, flags);
	}

	if(ret != RET_NOERROR){
		if(Player* player = creature->getPlayer()){
			player->sendCancelMessage(ret);
			player->sendCancelWalk();
		}
	}

	return ret;
}

ReturnValue Game::internalMoveCreature(Creature* actor, Creature* creature,
	Cylinder* fromCylinder, Cylinder* toCylinder, uint32_t flags /*= 0*/)
{
	//check if we can move the creature to the destination
	ReturnValue ret = toCylinder->__queryAdd(0, creature, 1, flags);
	if(ret != RET_NOERROR){
		return ret;
	}

	fromCylinder->getTile()->moveCreature(actor, creature, toCylinder);

	if(creature->getParent() == toCylinder){
		int32_t index = 0;
		Item* toItem = NULL;
		Cylinder* subCylinder = NULL;

		uint32_t n = 0;
		while((subCylinder = toCylinder->__queryDestination(index, creature, &toItem, flags)) != toCylinder){
			toCylinder->getTile()->moveCreature(actor, creature, subCylinder);
			toCylinder = subCylinder;
			flags = 0;

			//to prevent infinite loop
			if(++n >= MAP_MAX_LAYERS)
				break;
		}
	}

	return RET_NOERROR;
}

bool Game::playerMoveItem(uint32_t playerId, const Position& fromPos,
	uint16_t spriteId, uint8_t fromStackPos, const Position& toPos, uint8_t count)
{
	Player* player = getPlayerByID(playerId);
	if(!player || player->isRemoved() || player->hasFlag(PlayerFlag_CannotMoveItems))
		return false;

	if(!player->canDoAction()){
		uint32_t delay = player->getNextActionTime();
		SchedulerTask* task = createSchedulerTask(delay, boost::bind(&Game::playerMoveItem, this,
			playerId, fromPos, spriteId, fromStackPos, toPos, count));
		player->setNextActionTask(task);
		return false;
	}
	
	player->setNextActionTask(NULL);

	Cylinder* fromCylinder = internalGetCylinder(player, fromPos);
	uint8_t fromIndex = 0;

	if(fromPos.x == 0xFFFF){
		if(fromPos.y & 0x40){
			fromIndex = static_cast<uint8_t>(fromPos.z);
		}
		else{
			fromIndex = static_cast<uint8_t>(fromPos.y);
		}
	}
	else
		fromIndex = fromStackPos;

	Thing* thing = internalGetThing(player, fromPos, fromIndex, spriteId, STACKPOS_MOVE);
	if(!thing || !thing->getItem()){
		player->sendCancelMessage(RET_NOTPOSSIBLE);
		return false;
	}

	Item* item = thing->getItem();

	Cylinder* toCylinder = internalGetCylinder(player, toPos);
	uint8_t toIndex = 0;

	if(toPos.x == 0xFFFF){
		if(toPos.y & 0x40){
			toIndex = static_cast<uint8_t>(toPos.z);
		}
		else{
			toIndex = static_cast<uint8_t>(toPos.y);
		}
	}

	if(fromCylinder == NULL || toCylinder == NULL || item == NULL || item->getClientID() != spriteId){
		player->sendCancelMessage(RET_NOTPOSSIBLE);
		return false;
	}

	if(!item->isPushable()){
		player->sendCancelMessage(RET_NOTMOVEABLE);
		return false;
	}

	const Position& playerPos = player->getPosition();
	const Position& mapFromPos = fromCylinder->getTile()->getPosition();
	const Position& mapToPos = toCylinder->getTile()->getPosition();

	if(playerPos.z > mapFromPos.z){
		player->sendCancelMessage(RET_FIRSTGOUPSTAIRS);
		return false;
	}

	if(playerPos.z < mapFromPos.z){
		player->sendCancelMessage(RET_FIRSTGODOWNSTAIRS);
		return false;
	}

	if(!Position::areInRange<1,1,0>(playerPos, mapFromPos)){
		//need to walk to the item first before using it
		std::list<Direction> listDir;
		if(getPathToEx(player, item->getPosition(), listDir, 0, 1, true, true)){
			g_dispatcher.addTask(createTask(boost::bind(&Game::playerAutoWalk,
				this, player->getID(), listDir)));

			SchedulerTask* task = createSchedulerTask(400, boost::bind(&Game::playerMoveItem, this,
				playerId, fromPos, spriteId, fromStackPos, toPos, count));
			player->setNextWalkActionTask(task);
			return true;
		}
		else{
			player->sendCancelMessage(RET_THEREISNOWAY);
			return false;
		}
	}

	//hangable item specific code
	Tile* toTile = toCylinder->getTile();
	if(item->isHangable() && (toTile->isVertical() || toTile->isHorizontal())){
		//destination supports hangable objects so need to move there first

		if(toCylinder->getTile()->isVertical()){
			if(player->getPosition().x + 1 == mapToPos.x){
				player->sendCancelMessage(RET_NOTPOSSIBLE);
				return false;
			}
		}
		else if(toCylinder->getTile()->isHorizontal()){
			if(player->getPosition().y + 1 == mapToPos.y){
				player->sendCancelMessage(RET_NOTPOSSIBLE);
				return false;
			}
		}

		if(!Position::areInRange<1,1,0>(playerPos, mapToPos)){
			Position walkPos = mapToPos;
			if(toCylinder->getTile()->isVertical()){
				walkPos.x -= -1;
			}

			if(toCylinder->getTile()->isHorizontal()){
				walkPos.y -= -1;
			}

			Position itemPos = fromPos;
			uint8_t itemStackPos = fromStackPos;

			if(fromPos.x != 0xFFFF && Position::areInRange<1,1,0>(mapFromPos, player->getPosition())
				&& !Position::areInRange<1,1,0>(mapFromPos, walkPos)){
				//need to pickup the item first
				Item* moveItem = NULL;
				ReturnValue ret = internalMoveItem(player, fromCylinder, player, INDEX_WHEREEVER, item, count, &moveItem);
				if(ret != RET_NOERROR){
					player->sendCancelMessage(ret);
					return false;
				}

				//changing the position since its now in the inventory of the player
				internalGetPosition(moveItem, itemPos, itemStackPos);
			}

			std::list<Direction> listDir;
			if(map->getPathTo(player, walkPos, listDir)){
				g_dispatcher.addTask(createTask(boost::bind(&Game::playerAutoWalk,
					this, player->getID(), listDir)));

				SchedulerTask* task = createSchedulerTask(400, boost::bind(&Game::playerMoveItem, this,
					playerId, itemPos, spriteId, itemStackPos, toPos, count));
				player->setNextWalkActionTask(task);
				return true;
			}
			else{
				player->sendCancelMessage(RET_THEREISNOWAY);
				return false;
			}
		}
	}

	if((std::abs(playerPos.x - mapToPos.x) > item->getThrowRange()) ||
			(std::abs(playerPos.y - mapToPos.y) > item->getThrowRange()) ||
			(std::abs(mapFromPos.z - mapToPos.z) * 4 > item->getThrowRange()) ){
		player->sendCancelMessage(RET_DESTINATIONOUTOFREACH);
		return false;
	}

	if(!canThrowObjectTo(mapFromPos, mapToPos)){
		player->sendCancelMessage(RET_CANNOTTHROW);
		return false;
	}

	ReturnValue ret = internalMoveItem(player, fromCylinder, toCylinder, toIndex, item, count, NULL);
	if(ret != RET_NOERROR){
		player->sendCancelMessage(ret);
		return false;
	}

	return true;
}

ReturnValue Game::internalMoveItem(Creature* actor, Cylinder* fromCylinder, Cylinder* toCylinder,
	int32_t index, Item* item, uint32_t count, Item** _moveItem, uint32_t flags /*= 0*/)
{
	if(!toCylinder){
		return RET_NOTPOSSIBLE;
	}

	if(item->getParent() == NULL){
		assert(fromCylinder == item->getParent());
		return g_game.internalAddItem(actor, toCylinder, item, INDEX_WHEREEVER, FLAG_NOLIMIT);
	}

	Item* toItem = NULL;

	Cylinder* subCylinder;
	int floorN = 0;
	while((subCylinder = toCylinder->__queryDestination(index, item, &toItem, flags)) != toCylinder){
		toCylinder = subCylinder;
		flags = 0;

		//to prevent infinite loop
		if(++floorN >= MAP_MAX_LAYERS)
			break;
	}

	//destination is the same as the source?
	if(item == toItem){
		return RET_NOERROR; //silently ignore move
	}

	//check if we can add this item
	ReturnValue ret = toCylinder->__queryAdd(index, item, count, flags);
	if(ret == RET_NEEDEXCHANGE){
		//check if we can add it to source cylinder
		int32_t fromIndex = fromCylinder->__getIndexOfThing(item);

		ret = fromCylinder->__queryAdd(fromIndex, toItem, toItem->getItemCount(), 0);
		if(ret == RET_NOERROR){
			//check how much we can move
			uint32_t maxExchangeQueryCount = 0;
			ReturnValue retExchangeMaxCount = fromCylinder->__queryMaxCount(-1, toItem, toItem->getItemCount(), maxExchangeQueryCount, 0);

			if(retExchangeMaxCount != RET_NOERROR && maxExchangeQueryCount == 0){
				return retExchangeMaxCount;
			}

			if((toCylinder->__queryRemove(toItem, toItem->getItemCount(), flags) == RET_NOERROR) && ret == RET_NOERROR){
				int32_t oldToItemIndex = toCylinder->__getIndexOfThing(toItem);
				toCylinder->__removeThing(actor, toItem, toItem->getItemCount());
				fromCylinder->__addThing(actor, toItem);

				if(oldToItemIndex != -1){
					toCylinder->postRemoveNotification(actor, toItem, fromCylinder, oldToItemIndex, true);
				}

				int32_t newToItemIndex = fromCylinder->__getIndexOfThing(toItem);
				if(newToItemIndex != -1){
					fromCylinder->postAddNotification(actor, toItem, toCylinder, newToItemIndex);
				}

				ret = toCylinder->__queryAdd(index, item, count, flags);
				toItem = NULL;
			}
		}
	}

	if(ret != RET_NOERROR){
		return ret;
	}

	//check how much we can move
	uint32_t maxQueryCount = 0;
	ReturnValue retMaxCount = toCylinder->__queryMaxCount(index, item, count, maxQueryCount, flags);

	if(retMaxCount != RET_NOERROR && maxQueryCount == 0){
		return retMaxCount;
	}

	uint32_t m = 0;
	uint32_t n = 0;

	if(item->isStackable()){
		m = std::min((uint32_t)count, maxQueryCount);
	}
	else
		m = maxQueryCount;

	Item* moveItem = item;

	//check if we can remove this item
	ret = fromCylinder->__queryRemove(item, m, flags);
	if(ret != RET_NOERROR){
		return ret;
	}

	//remove the item
	int32_t itemIndex = fromCylinder->__getIndexOfThing(item);
	Item* updateItem = NULL;
	fromCylinder->__removeThing(actor, item, m);
	bool isCompleteRemoval = item->isRemoved();

	//update item(s)
	if(item->isStackable()) {
		if(toItem && toItem->getID() == item->getID()){
			n = std::min((uint32_t)100 - toItem->getItemCount(), m);
			toCylinder->__updateThing(actor, toItem, toItem->getID(), toItem->getItemCount() + n);
			updateItem = toItem;
		}

		if(m - n > 0){
			moveItem = Item::CreateItem(item->getID(), m - n);
		}
		else{
			moveItem = NULL;
		}

		if(item->isRemoved()){
			FreeThing(item);
		}
	}

	//add item
	if(moveItem /*m - n > 0*/){
		toCylinder->__addThing(actor, index, moveItem);
	}

	if(itemIndex != -1){
		fromCylinder->postRemoveNotification(actor, item, toCylinder, itemIndex, isCompleteRemoval);
	}

	if(moveItem){
		int32_t moveItemIndex = toCylinder->__getIndexOfThing(moveItem);
		if(moveItemIndex != -1){
			toCylinder->postAddNotification(actor, moveItem, fromCylinder, moveItemIndex);
		}
	}

	if(updateItem){
		int32_t updateItemIndex = toCylinder->__getIndexOfThing(updateItem);
		if(updateItemIndex != -1){
			toCylinder->postAddNotification(actor, updateItem, fromCylinder, updateItemIndex);
		}
	}

	if(_moveItem){
		if(moveItem){
			*_moveItem = moveItem;
		}
		else{
			*_moveItem = item;
		}
	}

	//we could not move all, inform the player
	if(item->isStackable() && maxQueryCount < count){
		return retMaxCount;
	}

	return ret;
}

ReturnValue Game::internalAddItem(Creature* actor, Cylinder* toCylinder, Item* item, int32_t index /*= INDEX_WHEREEVER*/,
	uint32_t flags /*= 0*/, bool test /*= false*/)
{
	if(toCylinder == NULL || item == NULL){
		return RET_NOTPOSSIBLE;
	}

	Item* toItem = NULL;
	toCylinder = toCylinder->__queryDestination(index, item, &toItem, flags);

	//check if we can add this item
	ReturnValue ret = toCylinder->__queryAdd(index, item, item->getItemCount(), flags);
	if(ret != RET_NOERROR){
		return ret;
	}

	//check how much we can move
	uint32_t maxQueryCount = 0;
	ret = toCylinder->__queryMaxCount(index, item, item->getItemCount(), maxQueryCount, flags);

	if(ret != RET_NOERROR){
		return ret;
	}

	uint32_t m = 0;
	uint32_t n = 0;

	if(item->isStackable()){
		m = std::min((uint32_t)item->getItemCount(), maxQueryCount);
	}
	else
		m = maxQueryCount;

	if(!test){
		Item* moveItem = item;

		//update item(s)
		if(item->isStackable()) {
			if(toItem && toItem->getID() == item->getID()){
				n = std::min((uint32_t)100 - toItem->getItemCount(), m);
				toCylinder->__updateThing(actor, toItem, toItem->getID(), toItem->getItemCount() + n);
			}

			if(m - n > 0){
				if(m - n != item->getItemCount()){
					moveItem = Item::CreateItem(item->getID(), m - n);
				}
			}
			else{
				moveItem = NULL;
				FreeThing(item);
			}
		}

		if(moveItem){
			toCylinder->__addThing(actor, index, moveItem);

			int32_t moveItemIndex = toCylinder->__getIndexOfThing(moveItem);
			if(moveItemIndex != -1){
				toCylinder->postAddNotification(actor, moveItem, NULL, moveItemIndex);
			}
		}
		else{
			int32_t itemIndex = toCylinder->__getIndexOfThing(item);

			if(itemIndex != -1){
				toCylinder->postAddNotification(actor, item, NULL, itemIndex);
			}
		}
	}

	return RET_NOERROR;
}

ReturnValue Game::internalRemoveItem(Creature* actor, Item* item, int32_t count /*= -1*/,  bool test /*= false*/, uint32_t flags /*= 0*/)
{
	Cylinder* cylinder = item->getParent();
	if(cylinder == NULL){
		return RET_NOTPOSSIBLE;
	}

	if(count == -1){
		count = item->getItemCount();
	}

	//check if we can remove this item
	ReturnValue ret = cylinder->__queryRemove(item, count, flags | FLAG_IGNORENOTMOVEABLE);
	if(ret != RET_NOERROR){
		return ret;
	}

	if(!item->canRemove()){
		return RET_NOTPOSSIBLE;
	}

	if(!test){
		int32_t index = cylinder->__getIndexOfThing(item);

		//remove the item
		cylinder->__removeThing(actor, item, count);
		bool isCompleteRemoval = false;

		if(item->isRemoved()){
			isCompleteRemoval = true;
			FreeThing(item);
		}

		cylinder->postRemoveNotification(NULL, item, NULL, index, isCompleteRemoval);
	}

	item->onRemoved();

	return RET_NOERROR;
}

ReturnValue Game::internalPlayerAddItem(Player* player, Item* item, bool dropOnMap /*= true*/)
{
	ReturnValue ret = internalAddItem(NULL, player, item);

	if(ret != RET_NOERROR && dropOnMap){
		ret = internalAddItem(NULL, player->getTile(), item, INDEX_WHEREEVER, FLAG_NOLIMIT);
	}

	return ret;
}

Item* Game::findItemOfType(Cylinder* cylinder, uint16_t itemId,
	bool depthSearch /*= true*/, int32_t subType /*= -1*/)
{
	if(cylinder == NULL){
		return false;
	}

	std::list<Container*> listContainer;
	Container* tmpContainer = NULL;
	Thing* thing = NULL;
	Item* item = NULL;

	for(int i = cylinder->__getFirstIndex(); i < cylinder->__getLastIndex();){

		if((thing = cylinder->__getThing(i)) && (item = thing->getItem())){
			if(item->getID() == itemId && (subType == -1 || subType == item->getSubType())){
				return item;
			}
			else{
				++i;

				if(depthSearch && (tmpContainer = item->getContainer())){
					listContainer.push_back(tmpContainer);
				}
			}
		}
		else{
			++i;
		}
	}

	while(listContainer.size() > 0){
		Container* container = listContainer.front();
		listContainer.pop_front();

		for(int i = 0; i < (int32_t)container->size();){
			Item* item = container->getItem(i);
			if(item->getID() == itemId && (subType == -1 || subType == item->getSubType())){
				return item;
			}
			else{
				++i;

				if((tmpContainer = item->getContainer())){
					listContainer.push_back(tmpContainer);
				}
			}
		}
	}

	return NULL;
}

bool Game::removeItemOfType(Creature* actor, Cylinder* cylinder, uint16_t itemId, int32_t count, int32_t subType /*= -1*/)
{
	if(cylinder == NULL || ((int32_t)cylinder->__getItemTypeCount(itemId, subType) < count) ){
		return false;
	}

	if(count <= 0){
		return true;
	}

	std::list<Container*> listContainer;
	Container* tmpContainer = NULL;
	Thing* thing = NULL;
	Item* item = NULL;

	for(int i = cylinder->__getFirstIndex(); i < cylinder->__getLastIndex() && count > 0;){

		if((thing = cylinder->__getThing(i)) && (item = thing->getItem())){
			if(item->getID() == itemId){
				if(item->isStackable()){
					if(item->getItemCount() > count){
						internalRemoveItem(actor, item, count);
						count = 0;
					}
					else{
						count -= item->getItemCount();
						internalRemoveItem(actor, item);
					}
				}
				else if(subType == -1 || subType == item->getSubType()){
					--count;
					internalRemoveItem(actor, item);
				}
				else //Item has subtype but not the required one.
					++i;
			}
			else{
				++i;

				if((tmpContainer = item->getContainer())){
					listContainer.push_back(tmpContainer);
				}
			}
		}
		else{
			++i;
		}
	}

	while(listContainer.size() > 0 && count > 0){
		Container* container = listContainer.front();
		listContainer.pop_front();

		for(int i = 0; i < (int32_t)container->size() && count > 0;){
			Item* item = container->getItem(i);
			if(item->getID() == itemId){
				if(item->isStackable()){
					if(item->getItemCount() > count){
						internalRemoveItem(actor, item, count);
						count = 0;
					}
					else{
						count-= item->getItemCount();
						internalRemoveItem(actor, item);
					}
				}
				else if(subType == -1 || subType == item->getSubType()){
					--count;
					internalRemoveItem(actor, item);
				}
				else //Item has subtype but not the required one.
					++i;
			}
			else{
				++i;

				if((tmpContainer = item->getContainer())){
					listContainer.push_back(tmpContainer);
				}
			}
		}
	}

	return (count == 0);
}

uint32_t Game::getMoney(const Cylinder* cylinder)
{
	if(cylinder == NULL){
		return 0;
	}

	std::list<Container*> listContainer;
	ItemList::const_iterator it;
	Container* tmpContainer = NULL;

	Thing* thing = NULL;
	Item* item = NULL;

	uint32_t moneyCount = 0;

	for(int i = cylinder->__getFirstIndex(); i < cylinder->__getLastIndex(); ++i){
		if(!(thing = cylinder->__getThing(i)))
			continue;

		if(!(item = thing->getItem()))
			continue;

		if((tmpContainer = item->getContainer())){
			listContainer.push_back(tmpContainer);
		}
		else{
			if(item->getWorth() != 0){
				moneyCount+= item->getWorth();
			}
		}
	}

	while(listContainer.size() > 0){
		Container* container = listContainer.front();
		listContainer.pop_front();

		for(it = container->getItems(); it != container->getEnd(); ++it){
			Item* item = *it;

			if((tmpContainer = item->getContainer())){
				listContainer.push_back(tmpContainer);
			}
			else if(item->getWorth() != 0){
				moneyCount+= item->getWorth();
			}
		}
	}

	return moneyCount;
}

bool Game::removeMoney(Creature* actor, Cylinder* cylinder, uint32_t money, uint32_t flags /*= 0*/)
{
	if(cylinder == NULL){
		return false;
	}
	if(money <= 0){
		return true;
	}

	std::list<Container*> listContainer;
	Container* tmpContainer = NULL;

	typedef std::multimap<uint32_t, Item*, std::less<uint32_t> > MoneyMap;
	typedef MoneyMap::value_type moneymap_pair;
	MoneyMap moneyMap;
	Thing* thing = NULL;
	Item* item = NULL;

	uint32_t moneyCount = 0;

	for(int i = cylinder->__getFirstIndex(); i < cylinder->__getLastIndex() && money > 0; ++i){
		if(!(thing = cylinder->__getThing(i)))
			continue;

		if(!(item = thing->getItem()))
			continue;

		if((tmpContainer = item->getContainer())){
			listContainer.push_back(tmpContainer);
		}
		else{
			if(item->getWorth() != 0){
				moneyCount += item->getWorth();
				moneyMap.insert(moneymap_pair(item->getWorth(), item));
			}
		}
	}

	while(listContainer.size() > 0 && money > 0){
		Container* container = listContainer.front();
		listContainer.pop_front();

		for(int i = 0; i < (int32_t)container->size() && money > 0; i++){
			Item* item = container->getItem(i);

			if((tmpContainer = item->getContainer())){
				listContainer.push_back(tmpContainer);
			}
			else if(item->getWorth() != 0){
				moneyCount += item->getWorth();
				moneyMap.insert(moneymap_pair(item->getWorth(), item));
			}
		}
	}

	if(moneyCount < money){
		/*not enough money*/
		return false;
	}

	MoneyMap::iterator it2;
	for(it2 = moneyMap.begin(); it2 != moneyMap.end() && money > 0; it2++){
		Item* item = it2->second;
		internalRemoveItem(actor, item);

		if(it2->first <= money){
			money = money - it2->first;
		}
		else{
			/* Remove a monetary value from an item*/
			int remaind = item->getWorth() - money;
			addMoney(actor, cylinder, remaind, flags);
			money = 0;
		}

		it2->second = NULL;
	}

	moneyMap.clear();

	return (money == 0);
}

bool Game::addMoney(Creature* actor, Cylinder* cylinder, uint32_t money, uint32_t flags /*= 0*/)
{
	for(std::map<uint32_t, ItemType*>::reverse_iterator it = Item::items.currencyMap.rbegin();
		it != Item::items.currencyMap.rend(); ++it)
	{
		if(money >= it->first){
			uint32_t count = money / it->first;
			money -= count * it->first;

			while(count > 0){
				uint32_t moneyCount = std::min(count, (uint32_t)100);
				Item* moneyItem = Item::CreateItem(it->second->id, moneyCount);

				ReturnValue ret = internalAddItem(actor, cylinder, moneyItem, INDEX_WHEREEVER, flags);
				if(ret != RET_NOERROR){
					internalAddItem(actor, cylinder->getTile(), moneyItem, INDEX_WHEREEVER, FLAG_NOLIMIT);
				}

				count -= moneyCount;
			}
		}
	}
	return true;
}

Item* Game::transformItem(Creature* actor, Item* item, uint16_t newId, int32_t newCount /*= -1*/)
{
	if(item->getID() == newId && (newCount == -1 || newCount == item->getSubType()))
		return item;

	Cylinder* cylinder = item->getParent();
	if(cylinder == NULL){
		return NULL;
	}

	int32_t itemIndex = cylinder->__getIndexOfThing(item);

	if(itemIndex == -1){
#ifdef __DEBUG__
		std::cout << "Error: transformItem, itemIndex == -1" << std::endl;
#endif
		return item;
	}

	if(!item->canTransform()){
		return item;
	}

	const ItemType& curType = Item::items[item->getID()];
	const ItemType& newType = Item::items[newId];

	if(curType.alwaysOnTop != newType.alwaysOnTop){
		//This only occurs when you transform items on tiles from a downItem to a topItem (or vice versa)
		//Remove the old, and add the new

		ReturnValue ret = internalRemoveItem(actor, item);
		if(ret != RET_NOERROR){
			return item;
		}

		Item* newItem = NULL;
		if(newCount == -1){
			newItem = Item::CreateItem(newId);
		}
		else{
			newItem = Item::CreateItem(newId, newCount);
		}

		newItem->copyAttributes(item);

		ret = internalAddItem(actor, cylinder, newItem, INDEX_WHEREEVER, FLAG_NOLIMIT);
		if(ret != RET_NOERROR){
			delete newItem;
			return NULL;
		}

		// Update script environment
		script_environment->reassignThing(item, newItem);

		return newItem;
	}

	if(curType.type == newType.type){
		//Both items has the same type so we can safely change id/subtype

		if(newCount == 0 && (item->isStackable() || item->hasCharges())){
			if(item->isStackable()){
				internalRemoveItem(actor, item);
				return NULL;
			}
			else{
				int32_t newItemId = newId;
				if(curType.id == newType.id){
					newItemId = curType.decayTo;
				}

				if(newItemId != -1){
					item = transformItem(actor, item, newItemId);
					return item;
				}
				else{
					internalRemoveItem(actor, item);
					return NULL;
				}
			}
		}
		else{
			cylinder->postRemoveNotification(actor, item, cylinder, itemIndex, false);
			uint16_t itemId = item->getID();
			int32_t count = item->getSubType();

			if(curType.id != newType.id){
				if(newType.group != curType.group){
					item->setDefaultSubtype();
				}

				itemId = newId;
			}

			if(newCount != -1 && newType.hasSubType()){
				count = newCount;
			}

			cylinder->__updateThing(actor, item, itemId, count);
			cylinder->postAddNotification(actor, item, cylinder, itemIndex);
			return item;
		}
	}
	else{
		//Replacing the the old item with the new while maintaining the old position
		Item* newItem = NULL;
		if(newCount == -1){
			newItem = Item::CreateItem(newId);
		}
		else{
			newItem = Item::CreateItem(newId, newCount);
		}
		if(newItem == NULL) {
			// Decaying into deprecated item?
#ifdef __DEBUG__
			std::cout << "Error: [Game::transformItem] Item of type " << item->getID() << " transforming into invalid type " << newId << std::endl;
#endif
			return NULL;
		}
		cylinder->__replaceThing(actor, itemIndex, newItem);
		cylinder->postAddNotification(actor, newItem, cylinder, itemIndex);

		item->setParent(NULL);
		cylinder->postRemoveNotification(actor, item, cylinder, itemIndex, true);
		FreeThing(item);

		// Update script environment
		script_environment->reassignThing(item, newItem);

		return newItem;
	}

	return NULL;
}

ReturnValue Game::internalTeleport(Creature* actor, Thing* thing, const Position& newPos, uint32_t flags /*= 0*/)
{
	if(newPos == thing->getPosition())
		return RET_NOERROR;
	else if(thing->isRemoved())
		return RET_NOTPOSSIBLE;

	Tile* toTile = getTile(newPos.x, newPos.y, newPos.z);
	if(toTile){
		if(Creature* creature = thing->getCreature()){
			ReturnValue ret = toTile->__queryAdd(0, creature, 1, FLAG_NOLIMIT);
			if(ret != RET_NOERROR){
				return ret;
			}

			creature->getTile()->moveCreature(actor, creature, toTile, true);
			return RET_NOERROR;
		}
		else if(Item* item = thing->getItem()){
			return internalMoveItem(actor, item->getParent(), toTile, INDEX_WHEREEVER, item, item->getItemCount(), NULL, flags);
		}
	}

	return RET_NOTPOSSIBLE;
}

bool Game::anonymousBroadcastMessage(MessageClasses type, const std::string& text)
{
	if(type < MSG_CLASS_FIRST || type > MSG_CLASS_LAST)
		return false;

	for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it){
		(*it).second->sendTextMessage(type, text);
	}

	return true;
}

//Implementation of player invoked events
bool Game::playerMove(uint32_t playerId, Direction dir)
{
	Player* player = getPlayerByID(playerId);
	if(!player || player->isRemoved())
		return false;

	player->stopWalk();
	int32_t delay = player->getWalkDelay(dir);

	if(delay > 0){
		player->setNextAction(OTSYS_TIME() + player->getStepDuration(dir) - 1);
		SchedulerTask* task = createSchedulerTask( ((uint32_t)delay), boost::bind(&Game::playerMove, this,
			playerId, dir));
		player->setNextWalkTask(task);
		return false;
	}

	player->resetIdle();
	player->setFollowCreature(NULL);
	player->onWalk(dir);
	return (internalMoveCreature(player, player, dir) == RET_NOERROR);
}

bool Game::internalBroadcastMessage(Player* player, const std::string& text)
{
	if(!player->hasFlag(PlayerFlag_CanBroadcast))
		return false;

	for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it){
		(*it).second->sendCreatureSay(player, SPEAK_BROADCAST, text);
	}

	return true;
}

bool Game::playerCreatePrivateChannel(uint32_t playerId)
{
	Player* player = getPlayerByID(playerId);
	if(!player || player->isRemoved())
		return false;

	ChatChannel* channel = g_chat.createChannel(player, 0xFFFF);

	if(!channel){
		return false;
	}

	if(!channel->addUser(player)){
		return false;
	}

	player->sendCreatePrivateChannel(channel->getId(), channel->getName());
	return true;
}

bool Game::playerChannelInvite(uint32_t playerId, const std::string& name)
{
	Player* player = getPlayerByID(playerId);
	if(!player || player->isRemoved())
		return false;

	PrivateChatChannel* channel = g_chat.getPrivateChannel(player);

	if(!channel){
		return false;
	}

	Player* invitePlayer = getPlayerByName(name);

	if(!invitePlayer){
		return false;
	}

	channel->invitePlayer(player, invitePlayer);
	return true;
}

bool Game::playerChannelExclude(uint32_t playerId, const std::string& name)
{
	Player* player = getPlayerByID(playerId);
	if(!player || player->isRemoved())
		return false;

	PrivateChatChannel* channel = g_chat.getPrivateChannel(player);

	if(!channel){
		return false;
	}

	Player* excludePlayer = getPlayerByName(name);

	if(!excludePlayer){
		return false;
	}

	channel->excludePlayer(player, excludePlayer);
	return true;
}

bool Game::playerRequestChannels(uint32_t playerId)
{
	Player* player = getPlayerByID(playerId);
	if(!player || player->isRemoved())
		return false;

	player->sendChannelsDialog();
	return true;
}

bool Game::playerOpenChannel(uint32_t playerId, uint16_t channelId)
{
	Player* player = getPlayerByID(playerId);
	if(!player || player->isRemoved())
		return false;

	if(!g_chat.addUserToChannel(player, channelId)){
		return false;
	}

	ChatChannel* channel = g_chat.getChannel(player, channelId);
	if(!channel){
		return false;
	}

	// The event handler will probably want to send messages to the 
	// channel. To prevent debugs, we make the player unable to hear 
	// messages in the channel, since we haven't actually sent the 
	// channel to him yet.
	channel->makePlayerDeaf(player);

	bool interrupted = false;
	if(script_system){
		Script::OnJoinChannel::Event evt(player, channel);
		 interrupted = script_system->dispatchEvent(evt);
	}

	channel->makePlayerDeaf(NULL);

	if(interrupted) {
		// If the event was not propagated, stop the action!
		g_chat.removeUserFromChannel(player, channelId);
		return false;
	}

	if(channel->getId() != CHANNEL_RULE_REP){
		player->sendChannel(channel->getId(), channel->getName());
	}
	else{
		player->sendRuleViolationsChannel(channel->getId());
	}

	return true;
}

// Called from removeFromAllChannels
void g_gameOnLeaveChannel(Player* player, ChatChannel* channel)
{
	if(!g_game.script_system)
		return;
	Script::OnLeaveChannel::Event evt(player, channel);
	g_game.script_system->dispatchEvent(evt);
}

bool Game::playerCloseChannel(uint32_t playerId, uint16_t channelId)
{
	Player* player = getPlayerByID(playerId);
	if(!player || player->isRemoved())
		return false;

	ChatChannel* channel = g_chat.getChannel(player, channelId);
	if(!channel)
		return false;

	g_chat.removeUserFromChannel(player, channelId);

	if(script_system){
		Script::OnLeaveChannel::Event evt(player, channel);
		// We can't abort this action, no need to check return
		script_system->dispatchEvent(evt);
	}

	return true;
}

bool Game::playerOpenPrivateChannel(uint32_t playerId, const std::string& receiver)
{
	Player* player = getPlayerByID(playerId);
	if(!player || player->isRemoved())
		return false;

	uint32_t guid;
	std::string receiverName = receiver;
	if(IOPlayer::instance()->getGuidByName(guid, receiverName))
		player->sendOpenPrivateChannel(receiverName);
	else
		player->sendCancel("A player with this name does not exist.");

	return true;
}

bool Game::playerProcessRuleViolation(uint32_t playerId, const std::string& name)
{
	Player* player = getPlayerByID(playerId);
	if(!player || player->isRemoved())
		return false;

	if(!player->hasFlag(PlayerFlag_CanAnswerRuleViolations))
		return false;

	Player* reporter = getPlayerByName(name);
	if(!reporter){
		return false;
	}

	RuleViolationsMap::iterator it = ruleViolations.find(reporter->getID());
	if(it == ruleViolations.end()){
		return false;
	}

	RuleViolation& rvr = *it->second;

	if(!rvr.isOpen){
		return false;
	}

	rvr.isOpen = false;
	rvr.gamemaster = player;

	ChatChannel* channel = g_chat.getChannelById(CHANNEL_RULE_REP);
	if(channel){
		for(UsersMap::const_iterator it = channel->getUsers().begin(); it != channel->getUsers().end(); ++it){
			if(it->second){
				it->second->sendRemoveReport(reporter->getName());
			}
		}
	}

	return true;
}

bool Game::playerCloseRuleViolation(uint32_t playerId, const std::string& name)
{
	Player* player = getPlayerByID(playerId);
	if(!player || player->isRemoved())
		return false;

	Player* reporter = getPlayerByName(name);
	if(!reporter){
		return false;
	}

	return closeRuleViolation(reporter);
}

bool Game::playerCancelRuleViolation(uint32_t playerId)
{
	Player* player = getPlayerByID(playerId);
	if(!player || player->isRemoved()){
		return false;
	}

	return cancelRuleViolation(player);
}

bool Game::playerCloseNpcChannel(uint32_t playerId)
{
	Player* player = getPlayerByID(playerId);
	if(!player || player->isRemoved())
		return false;

	SpectatorVec list;
	SpectatorVec::iterator it;
	getSpectators(list, player->getPosition());
	
	for(it = list.begin(); it != list.end(); ++it){
		//if((npc = (*it)->getNpc())){
			// REVSCRIPT TODO
			//npc->onPlayerCloseChannel(player);
		//}
	}
	return true;
}

bool Game::playerReceivePing(uint32_t playerId)
{
	Player* player = getPlayerByID(playerId);
	if(!player || player->isRemoved())
		return false;

	player->receivePing();
	return true;
}

bool Game::playerAutoWalk(uint32_t playerId, std::list<Direction>& listDir)
{
	Player* player = getPlayerByID(playerId);
	if(!player || player->isRemoved())
		return false;

	player->resetIdle();
	player->setNextWalkTask(NULL);
	return player->startAutoWalk(listDir);
}

bool Game::playerStopAutoWalk(uint32_t playerId)
{
	Player* player = getPlayerByID(playerId);
	if(!player || player->isRemoved())
		return false;

	player->stopWalk();
	return true;
}

bool Game::playerUseItemEx(uint32_t playerId, const Position& fromPos, uint8_t fromStackPos, uint16_t fromSpriteId, const Position& toPos, uint8_t toStackPos, uint16_t toSpriteId, bool isHotkey)
{
	Player* player = getPlayerByID(playerId);
	if(!player || player->isRemoved())
		return false;

	if(isHotkey && !g_config.getNumber(ConfigManager::HOTKEYS)){
		return false;
	}

	Thing* thing = internalGetThing(player, fromPos, fromStackPos, fromSpriteId, STACKPOS_USEITEM);
	if(!thing){
		player->sendCancelMessage(RET_NOTPOSSIBLE);
		return false;
	}

	Item* item = thing->getItem();
	if(!item || !item->isUseable()){
		player->sendCancelMessage(RET_CANNOTUSETHISOBJECT);
		return false;
	}

	Position walkToPos = fromPos;
	ReturnValue ret = canUse(player, fromPos);
	
	if(ret != RET_NOERROR){
		if(ret == RET_TOOFARAWAY){
			return useItemFarEx(playerId, fromPos, fromStackPos, fromSpriteId,
				toPos, toStackPos, toSpriteId, isHotkey);
		}

		player->sendCancelMessage(ret);
		return false;
	}

	if(isHotkey){
		showUseHotkeyMessage(player, item);
	}

	player->resetIdle();

	if(!player->canDoAction()){
		uint32_t delay = player->getNextActionTime();
		SchedulerTask* task = createSchedulerTask(delay, boost::bind(&Game::playerUseItemEx, this,
			playerId, fromPos, fromStackPos, fromSpriteId, toPos, toStackPos, toSpriteId, isHotkey));
		player->setNextActionTask(task);
		return false;
	}

	player->setNextActionTask(NULL);

	return useItemEx(player, fromPos, fromSpriteId, toPos, toStackPos, toSpriteId, item, isHotkey);
}

bool Game::playerUseItem(uint32_t playerId, const Position& pos, uint8_t stackPos,
	uint8_t index, uint16_t spriteId, bool isHotkey)
{
	Player* player = getPlayerByID(playerId);
	if(!player || player->isRemoved())
		return false;

	if(isHotkey && !g_config.getNumber(ConfigManager::HOTKEYS)){
		return false;
	}

	Thing* thing = internalGetThing(player, pos, stackPos, spriteId, STACKPOS_USEITEM);
	if(!thing){
		player->sendCancelMessage(RET_NOTPOSSIBLE);
		return false;
	}

	Item* item = thing->getItem();
	if(!item){
		player->sendCancelMessage(RET_CANNOTUSETHISOBJECT);
		return false;
	}

	ReturnValue ret = canUse(player, pos);
	if(ret != RET_NOERROR){
		if(ret == RET_TOOFARAWAY){
			std::list<Direction> listDir;
			if(getPathToEx(player, pos, listDir, 0, 1, true, true)){
				g_dispatcher.addTask(createTask(boost::bind(&Game::playerAutoWalk,
					this, player->getID(), listDir)));

				SchedulerTask* task = createSchedulerTask(400, boost::bind(&Game::playerUseItem, this,
					playerId, pos, stackPos, index, spriteId, isHotkey));
				player->setNextWalkActionTask(task);
				return true;
			}

			ret = RET_THEREISNOWAY;
		}

		player->sendCancelMessage(ret);
		return false;
	}

	if(isHotkey){
		showUseHotkeyMessage(player, item);
	}

	player->resetIdle();

	if(!player->canDoAction()){
		uint32_t delay = player->getNextActionTime();
		SchedulerTask* task = createSchedulerTask(delay, boost::bind(&Game::playerUseItem, this,
			playerId, pos, stackPos, index, spriteId, isHotkey));
		player->setNextActionTask(task);
		return false;
	}

	player->setNextActionTask(NULL);

	useItem(player, pos, index, item, isHotkey);
	return true;
}

bool Game::playerUseBattleWindow(uint32_t playerId, const Position& fromPos, uint8_t fromStackPos,
	uint32_t creatureId, uint16_t spriteId, bool isHotkey)
{
	Player* player = getPlayerByID(playerId);
	if(!player || player->isRemoved())
		return false;

	Creature* creature = getCreatureByID(creatureId);
	if(!creature){
		return false;
	}

	if(!Position::areInRange<7,5,0>(creature->getPosition(), player->getPosition())){
		return false;
	}

	if(!g_config.getNumber(ConfigManager::HOTKEYS)){
		if(creature->getPlayer() || isHotkey){
			player->sendCancelMessage(RET_DIRECTPLAYERSHOOT);
			return false;
		}
	}

	Thing* thing = internalGetThing(player, fromPos, fromStackPos, spriteId, STACKPOS_USE);
	if(!thing){
		player->sendCancelMessage(RET_NOTPOSSIBLE);
		return false;
	}

	Item* item = thing->getItem();
	if(!item || item->getClientID() != spriteId){
		player->sendCancelMessage(RET_CANNOTUSETHISOBJECT);
		return false;
	}

	ReturnValue ret = canUse(player, fromPos);
	if(ret != RET_NOERROR){
		if(ret == RET_TOOFARAWAY){
			return useItemFarEx(playerId, fromPos, fromStackPos, spriteId, item->getPosition(), 
				item->getParent()->__getIndexOfThing(item), isHotkey, creatureId);
		}

		player->sendCancelMessage(ret);
		return false;
	}

	if(isHotkey){
		showUseHotkeyMessage(player, item);
	}

	player->resetIdle();

	if(!player->canDoAction()){
		uint32_t delay = player->getNextActionTime();
		SchedulerTask* task = createSchedulerTask(delay, boost::bind(&Game::playerUseBattleWindow, this,
			playerId, fromPos, fromStackPos, creatureId, spriteId, isHotkey));
		player->setNextActionTask(task);
		return false;
	}

	player->setNextActionTask(NULL);

	return useItemEx(player, fromPos, spriteId, creature->getPosition(), creature->getParent()->__getIndexOfThing(creature), 0, item, isHotkey, creatureId);
}

bool Game::playerCloseContainer(uint32_t playerId, uint8_t cid)
{
	Player* player = getPlayerByID(playerId);
	if(!player || player->isRemoved())
		return false;

	player->closeContainer(cid);
	player->sendCloseContainer(cid);
	return true;
}

bool Game::playerMoveUpContainer(uint32_t playerId, uint8_t cid)
{
	Player* player = getPlayerByID(playerId);
	if(!player || player->isRemoved())
		return false;

	Container* container = player->getContainer(cid);
	if(!container){
		return false;
	}

	Container* parentContainer = dynamic_cast<Container*>(container->getParent());
	if(!parentContainer){
		return false;
	}

	bool hasParent = (dynamic_cast<const Container*>(parentContainer->getParent()) != NULL);
	player->addContainer(cid, parentContainer);
	player->sendContainer(cid, parentContainer, hasParent);

	return true;
}

bool Game::playerUpdateTile(uint32_t playerId, const Position& pos)
{
	Player* player = getPlayerByID(playerId);
	if(!player || player->isRemoved())
		return false;

	if(player->canSee(pos)){
		Tile* tile = getTile(pos.x, pos.y, pos.z);
		player->sendUpdateTile(tile, pos);
		return true;
	}
	return false;
}

bool Game::playerUpdateContainer(uint32_t playerId, uint8_t cid)
{
	Player* player = getPlayerByID(playerId);
	if(!player || player->isRemoved())
		return false;

	Container* container = player->getContainer(cid);
	if(!container){
		return false;
	}

	bool hasParent = (dynamic_cast<const Container*>(container->getParent()) != NULL);
	player->sendContainer(cid, container, hasParent);

	return true;
}

bool Game::playerRotateItem(uint32_t playerId, const Position& pos, uint8_t stackPos, const uint16_t spriteId)
{
	Player* player = getPlayerByID(playerId);
	if(!player || player->isRemoved())
		return false;

	Thing* thing = internalGetThing(player, pos, stackPos);
	if(!thing){
		return false;
	}

	Item* item = thing->getItem();
	if(!item || item->getClientID() != spriteId || !item->isRotateable()){
		player->sendCancelMessage(RET_NOTPOSSIBLE);
		return false;
	}

	if(pos.x != 0xFFFF && !Position::areInRange<1,1,0>(pos, player->getPosition())){
		std::list<Direction> listDir;
		if(getPathToEx(player, pos, listDir, 0, 1, true, true)){
			g_dispatcher.addTask(createTask(boost::bind(&Game::playerAutoWalk,
				this, player->getID(), listDir)));

			SchedulerTask* task = createSchedulerTask(400, boost::bind(&Game::playerRotateItem, this,
				playerId, pos, stackPos, spriteId));
			player->setNextWalkActionTask(task);
			return true;
		}
		else{
			player->sendCancelMessage(RET_THEREISNOWAY);
			return false;
		}
	}

	uint16_t newId = Item::items[item->getID()].rotateTo;
	if(newId != 0){
		transformItem(player, item, newId);
	}

	return true;
}

bool Game::playerWriteItem(uint32_t playerId, uint32_t windowTextId, const std::string& text)
{
	Player* player = getPlayerByID(playerId);
	if(!player || player->isRemoved())
		return false;

	uint16_t maxTextLength = 0;
	uint32_t internalWindowTextId = 0;
	Item* writeItem = player->getWriteItem(internalWindowTextId, maxTextLength);

	if(text.length() > maxTextLength || windowTextId != internalWindowTextId){
		return false;
	}

	if(!writeItem || writeItem->isRemoved()){
		player->sendCancelMessage(RET_NOTPOSSIBLE);
		return false;
	}

	Cylinder* topParent = writeItem->getTopParent();
	Player* owner = dynamic_cast<Player*>(topParent);
	if(owner && owner != player){
		player->sendCancelMessage(RET_NOTPOSSIBLE);
		return false;
	}

	if(!Position::areInRange<1,1,0>(writeItem->getPosition(), player->getPosition())){
		player->sendCancelMessage(RET_NOTPOSSIBLE);
		return false;
	}

	if(!text.empty()){
		if(writeItem->getText() != text){
			writeItem->setText(text);
			writeItem->setWriter(player->getName());
			writeItem->setWrittenDate(std::time(NULL));
		}
	}
	else{
		writeItem->clearText();
		writeItem->clearWriter();
		writeItem->clearWrittenDate();
	}

	uint16_t newId = Item::items[writeItem->getID()].writeOnceItemId;
	if(newId != 0){
		transformItem(player, writeItem, newId);
	}

	player->setWriteItem(NULL);
	return true;
}

bool Game::playerUpdateHouseWindow(uint32_t playerId, uint8_t listId, uint32_t windowTextId, const std::string& text)
{
	Player* player = getPlayerByID(playerId);
	if(!player || player->isRemoved())
		return false;

	uint32_t internalWindowTextId;
	uint32_t internalListId;
	House* house = player->getEditHouse(internalWindowTextId, internalListId);

	if(house && internalWindowTextId == windowTextId && listId == 0){
		house->setAccessList(internalListId, text);
		player->setEditHouse(NULL);
	}

	return true;
}

bool Game::playerRequestTrade(uint32_t playerId, const Position& pos, uint8_t stackPos,
	uint32_t tradePlayerId, uint16_t spriteId)
{
	Player* player = getPlayerByID(playerId);
	if(!player || player->isRemoved())
		return false;

	Player* tradePartner = getPlayerByID(tradePlayerId);
	if(!tradePartner || tradePartner == player) {
		player->sendTextMessage(MSG_INFO_DESCR, "Sorry, not possible.");
		return false;
	}

	if(!Position::areInRange<2,2,0>(tradePartner->getPosition(), player->getPosition())){
		std::stringstream ss;
		ss << tradePartner->getName() << " tells you to move closer.";
		player->sendTextMessage(MSG_INFO_DESCR, ss.str());
		return false;
	}

	Item* tradeItem = dynamic_cast<Item*>(internalGetThing(player, pos, stackPos, spriteId, STACKPOS_USE));
	if(!tradeItem || tradeItem->getClientID() != spriteId || !tradeItem->isPickupable() || !tradeItem->isTradeable()) {
		player->sendCancelMessage(RET_NOTPOSSIBLE);
		return false;
	}
	else if(player->getPosition().z > tradeItem->getPosition().z){
		player->sendCancelMessage(RET_FIRSTGOUPSTAIRS);
		return false;
	}
	else if(player->getPosition().z < tradeItem->getPosition().z){
		player->sendCancelMessage(RET_FIRSTGODOWNSTAIRS);
		return false;
	}
	else if(!Position::areInRange<1,1,0>(tradeItem->getPosition(), player->getPosition())){
		std::list<Direction> listDir;
		if(getPathToEx(player, pos, listDir, 0, 1, true, true)){
			g_dispatcher.addTask(createTask(boost::bind(&Game::playerAutoWalk,
				this, player->getID(), listDir)));

			SchedulerTask* task = createSchedulerTask(400, boost::bind(&Game::playerRequestTrade, this,
				playerId, pos, stackPos, tradePlayerId, spriteId));
			player->setNextWalkActionTask(task);
			return true;
		}
		else{
			player->sendCancelMessage(RET_THEREISNOWAY);
			return false;
		}
	}

	std::map<Item*, uint32_t>::const_iterator it;
	const Container* container = NULL;
	for(it = tradeItems.begin(); it != tradeItems.end(); it++) {
		if(tradeItem == it->first ||
			((container = dynamic_cast<const Container*>(tradeItem)) && container->isHoldingItem(it->first)) ||
			((container = dynamic_cast<const Container*>(it->first)) && container->isHoldingItem(tradeItem)))
		{
			player->sendTextMessage(MSG_INFO_DESCR, "This item is already being traded.");
			return false;
		}
	}

	Container* tradeContainer = tradeItem->getContainer();
	if(tradeContainer && tradeContainer->getItemHoldingCount() + 1 > 100){
		player->sendTextMessage(MSG_INFO_DESCR, "You can not trade more than 100 items.");
		return false;
	}

	return internalStartTrade(player, tradePartner, tradeItem);
}

bool Game::internalStartTrade(Player* player, Player* tradePartner, Item* tradeItem)
{
	if(player->tradeState != TRADE_NONE && !(player->tradeState == TRADE_ACKNOWLEDGE && player->tradePartner == tradePartner)) {
		player->sendCancelMessage(RET_YOUAREALREADYTRADING);
		return false;
	}
	else if(tradePartner->tradeState != TRADE_NONE && tradePartner->tradePartner != player) {
		player->sendCancelMessage(RET_THISPLAYERISALREADYTRADING);
		return false;
	}

	player->tradePartner = tradePartner;
	player->tradeItem = tradeItem;
	player->tradeState = TRADE_INITIATED;
	tradeItem->addRef();
	tradeItems[tradeItem] = player->getID();

	player->sendTradeItemRequest(player, tradeItem, true);

	if(tradePartner->tradeState == TRADE_NONE){
		std::stringstream trademsg;
		trademsg << player->getName() <<" wants to trade with you.";
		tradePartner->sendTextMessage(MSG_INFO_DESCR, trademsg.str());
		tradePartner->tradeState = TRADE_ACKNOWLEDGE;
		tradePartner->tradePartner = player;
	}
	else{
		Item* counterOfferItem = tradePartner->tradeItem;
		player->sendTradeItemRequest(tradePartner, counterOfferItem, false);
		tradePartner->sendTradeItemRequest(player, tradeItem, false);
	}

	return true;

}

bool Game::playerAcceptTrade(uint32_t playerId)
{
	Player* player = getPlayerByID(playerId);
	if(!player || player->isRemoved())
		return false;

	if(!(player->getTradeState() == TRADE_ACKNOWLEDGE || player->getTradeState() == TRADE_INITIATED)){
		return false;
	}

	player->setTradeState(TRADE_ACCEPT);
	Player* tradePartner = player->tradePartner;
	if(tradePartner && tradePartner->getTradeState() == TRADE_ACCEPT){

		Item* tradeItem1 = player->tradeItem;
		Item* tradeItem2 = tradePartner->tradeItem;

		player->setTradeState(TRADE_TRANSFER);
		tradePartner->setTradeState(TRADE_TRANSFER);

		std::map<Item*, uint32_t>::iterator it;

		it = tradeItems.find(tradeItem1);
		if(it != tradeItems.end()) {
			FreeThing(it->first);
			tradeItems.erase(it);
		}

		it = tradeItems.find(tradeItem2);
		if(it != tradeItems.end()) {
			FreeThing(it->first);
			tradeItems.erase(it);
		}

		bool isSuccess = false;

		ReturnValue ret1 = internalAddItem(NULL, tradePartner, tradeItem1, INDEX_WHEREEVER, 0, true);
		ReturnValue ret2 = internalAddItem(NULL, player, tradeItem2, INDEX_WHEREEVER, 0, true);

		if(ret1 == RET_NOERROR && ret2 == RET_NOERROR){
			ret1 = internalRemoveItem(NULL, tradeItem1, tradeItem1->getItemCount(), true);
			ret2 = internalRemoveItem(NULL, tradeItem2, tradeItem2->getItemCount(), true);

			if(ret1 == RET_NOERROR && ret2 == RET_NOERROR){
				Cylinder* cylinder1 = tradeItem1->getParent();
				Cylinder* cylinder2 = tradeItem2->getParent();

				internalMoveItem(NULL, cylinder1, tradePartner, INDEX_WHEREEVER, tradeItem1, tradeItem1->getItemCount(), NULL);
				internalMoveItem(NULL, cylinder2, player, INDEX_WHEREEVER, tradeItem2, tradeItem2->getItemCount(), NULL);

				tradeItem1->onTradeEvent(ON_TRADE_TRANSFER, tradePartner);
				tradeItem2->onTradeEvent(ON_TRADE_TRANSFER, player);

				isSuccess = true;
			}
		}

		if(!isSuccess){
			std::string errorDescription = getTradeErrorDescription(ret1, tradeItem1);
			tradePartner->sendTextMessage(MSG_INFO_DESCR, errorDescription);
			tradePartner->tradeItem->onTradeEvent(ON_TRADE_CANCEL, tradePartner);

			errorDescription = getTradeErrorDescription(ret2, tradeItem2);
			player->sendTextMessage(MSG_INFO_DESCR, errorDescription);
			player->tradeItem->onTradeEvent(ON_TRADE_CANCEL, player);
		}

		player->setTradeState(TRADE_NONE);
		player->tradeItem = NULL;
		player->tradePartner = NULL;
		player->sendTradeClose();

		tradePartner->setTradeState(TRADE_NONE);
		tradePartner->tradeItem = NULL;
		tradePartner->tradePartner = NULL;
		tradePartner->sendTradeClose();

		return isSuccess;
	}

	return false;
}

std::string Game::getTradeErrorDescription(ReturnValue ret, Item* item)
{
	std::stringstream ss;
	if(ret == RET_NOTENOUGHCAPACITY){
		ss << "You do not have enough capacity to carry";
		if(item->isStackable() && item->getItemCount() > 1){
			ss << " these objects.";
		}
		else{
			ss << " this object." ;
		}
		ss << std::endl << " " << item->getWeightDescription();
	}
	else if(ret == RET_NOTENOUGHROOM || ret == RET_CONTAINERNOTENOUGHROOM){
		ss << "You do not have enough room to carry";
		if(item->isStackable() && item->getItemCount() > 1){
			ss << " these objects.";
		}
		else{
			ss << " this object.";
		}
	}
	else{
		ss << "Trade could not be completed.";
	}
	return ss.str();
}

bool Game::playerLookInTrade(uint32_t playerId, bool lookAtCounterOffer, int index)
{
	Player* player = getPlayerByID(playerId);
	if(!player || player->isRemoved())
		return false;

	Player* tradePartner = player->tradePartner;
	if(!tradePartner)
		return false;

	Item* tradeItem = NULL;

	if(lookAtCounterOffer)
		tradeItem = tradePartner->getTradeItem();
	else
		tradeItem = player->getTradeItem();

	if(!tradeItem)
		return false;

	int32_t lookDistance = std::max(std::abs(player->getPosition().x - tradeItem->getPosition().x),
		std::abs(player->getPosition().y - tradeItem->getPosition().y));

	if(index == 0){
		std::stringstream ss;
		ss << "You see " << tradeItem->getDescription(lookDistance);
		player->sendTextMessage(MSG_INFO_DESCR, ss.str());
		return false;
	}

	Container* tradeContainer = tradeItem->getContainer();
	if(!tradeContainer || index > (int)tradeContainer->getItemHoldingCount())
		return false;

	bool foundItem = false;
	std::list<const Container*> listContainer;
	ItemList::const_iterator it;
	Container* tmpContainer = NULL;

	listContainer.push_back(tradeContainer);

	while(!foundItem && listContainer.size() > 0){
		const Container* container = listContainer.front();
		listContainer.pop_front();

		for(it = container->getItems(); it != container->getEnd(); ++it){
			if((tmpContainer = (*it)->getContainer())){
				listContainer.push_back(tmpContainer);
			}

			--index;
			if(index == 0){
				tradeItem = *it;
				foundItem = true;
				break;
			}
		}
	}

	if(foundItem){
		std::stringstream ss;
		ss << "You see " << tradeItem->getDescription(lookDistance);
		player->sendTextMessage(MSG_INFO_DESCR, ss.str());
	}

	return foundItem;
}

bool Game::playerCloseTrade(uint32_t playerId)
{
	Player* player = getPlayerByID(playerId);
	if(!player || player->isRemoved())
		return false;

	return internalCloseTrade(player);
}

bool Game::internalCloseTrade(Player* player)
{
	Player* tradePartner = player->tradePartner;
	if((tradePartner && tradePartner->getTradeState() == TRADE_TRANSFER) ||
		  player->getTradeState() == TRADE_TRANSFER){
#ifdef __DEBUG__
  		std::cout << "Warning: [Game::playerCloseTrade] TradeState == TRADE_TRANSFER. " <<
		  	player->getName() << " " << player->getTradeState() << " , " <<
		  	tradePartner->getName() << " " << tradePartner->getTradeState() << std::endl;
#endif
		return true;
	}

	std::vector<Item*>::iterator it;
	if(player->getTradeItem()){
		std::map<Item*, uint32_t>::iterator it = tradeItems.find(player->getTradeItem());
		if(it != tradeItems.end()) {
			FreeThing(it->first);
			tradeItems.erase(it);
		}

		player->tradeItem->onTradeEvent(ON_TRADE_CANCEL, player);
		player->tradeItem = NULL;
	}

	player->setTradeState(TRADE_NONE);
	player->tradePartner = NULL;

	player->sendTextMessage(MSG_STATUS_SMALL, "Trade cancelled.");
	player->sendTradeClose();

	if(tradePartner){
		if(tradePartner->getTradeItem()){
			std::map<Item*, uint32_t>::iterator it = tradeItems.find(tradePartner->getTradeItem());
			if(it != tradeItems.end()) {
				FreeThing(it->first);
				tradeItems.erase(it);
			}

			tradePartner->tradeItem->onTradeEvent(ON_TRADE_CANCEL, tradePartner);
			tradePartner->tradeItem = NULL;
		}

		tradePartner->setTradeState(TRADE_NONE);
		tradePartner->tradePartner = NULL;

		tradePartner->sendTextMessage(MSG_STATUS_SMALL, "Trade cancelled.");
		tradePartner->sendTradeClose();
	}

	return true;
}

bool Game::playerPurchaseItem(uint32_t playerId, uint16_t spriteId, uint8_t count,
	uint8_t amount, bool ignoreCapacity, bool buyWithBackpack)
{
	Player* player = getPlayerByID(playerId);
	if(player == NULL || player->isRemoved())
		return false;

	// REVSCRIPT TODO
	// shop buy item
	/*
	Creature* merchant = player->getShopOwner(onBuy, onSell);
	if(merchant == NULL)
		return false;

	const ItemType& it = Item::items.getItemIdByClientId(spriteId);
	if(it.id == 0){
		return false;
	}

	uint8_t subType = 0;
	if(it.isFluidContainer()){
		int32_t maxFluidType = sizeof(reverseFluidMap) / sizeof(uint8_t);
		if(count < maxFluidType){
			subType = (uint8_t)reverseFluidMap[count];
		}
	}
	else{
		subType = count;
	}
	*/

	// REVSCRIPT TODO
	//merchant->onPlayerTrade(player, SHOPEVENT_BUY, onBuy, it.id, subType, amount);
	return true;
}

bool Game::playerSellItem(uint32_t playerId, uint16_t spriteId, uint8_t count, uint8_t amount)
{
	Player* player = getPlayerByID(playerId);
	if(player == NULL || player->isRemoved())
		return false;

	// REVSCRIPT TODO
	// shop sell item
	/*
	Creature* merchant = player->getShopOwner(onBuy, onSell);
	if(merchant == NULL)
		return false;

	const ItemType& it = Item::items.getItemIdByClientId(spriteId);
	if(it.id == 0){
		return false;
	}

	uint8_t subType = 0;
	if(it.isFluidContainer()){
		int32_t maxFluidType = sizeof(reverseFluidMap) / sizeof(uint8_t);
		if(count < maxFluidType){
			subType = (uint8_t)reverseFluidMap[count];
		}
	}
	else{
		subType = count;
	}
	*/
	// REVSCRIPT TODO
	//merchant->onPlayerTrade(player, SHOPEVENT_SELL, onSell, it.id, subType, amount);
	return true;
}

bool Game::playerCloseShop(uint32_t playerId)
{
	Player* player = getPlayerByID(playerId);
	if(player == NULL || player->isRemoved())
		return false;

	// REVSCRIPTSYS TODO
	// Close shop
	//player->closeShopWindow();
	return true;
}

bool Game::playerLookInShop(uint32_t playerId, uint16_t spriteId, uint8_t count)
{
	Player* player = getPlayerByID(playerId);
	if(player == NULL || player->isRemoved())
		return false;

	const ItemType& it = Item::items.getItemIdByClientId(spriteId);
	if(it.id == 0){
		return false;
	}

	int32_t subType = 0;
	if(it.isFluidContainer()){
		int32_t maxFluidType = sizeof(reverseFluidMap) / sizeof(uint8_t);
		if(count < maxFluidType){
			subType = reverseFluidMap[count];
		}
	}
	else{
		subType = count;
	}

	std::stringstream ss;
	ss << "You see " << Item::getDescription(it, 1, NULL, subType);
	player->sendTextMessage(MSG_INFO_DESCR, ss.str());

	return true;
}

bool Game::playerLookAt(uint32_t playerId, const Position& pos, uint16_t spriteId, uint8_t stackPos)
{
	Player* player = getPlayerByID(playerId);
	if(!player || player->isRemoved())
		return false;

	Thing* thing = internalGetThing(player, pos, stackPos, spriteId, STACKPOS_LOOK);
	if(!thing){
		player->sendCancelMessage(RET_NOTPOSSIBLE);
		return false;
	}

	Position thingPos = thing->getPosition();
	if(!player->canSee(thingPos)){
		player->sendCancelMessage(RET_NOTPOSSIBLE);
		return false;
	}

	Position playerPos = player->getPosition();

	int32_t lookDistance = 0;
	if(thing == player)
		lookDistance = -1;
	else{
		lookDistance = std::max(std::abs(playerPos.x - thingPos.x), std::abs(playerPos.y - thingPos.y));
		if(playerPos.z != thingPos.z)
			lookDistance = lookDistance + 9 + 6;
	}

	std::string desc = thing->getDescription(lookDistance);

	if(script_system){
		Script::OnLook::Event evt(player, desc, thing);
		if(script_system->dispatchEvent(evt))
			return false;
	}

	if(desc.length() == 0) 
		return false;

	player->sendTextMessage(MSG_INFO_DESCR, "You see " + desc);

	return true;
}

bool Game::playerCancelAttackAndFollow(uint32_t playerId)
{
	Player* player = getPlayerByID(playerId);
	if(!player || player->isRemoved())
		return false;

	playerSetAttackedCreature(playerId, 0);
	playerFollowCreature(playerId, 0);
	player->stopWalk();
	return true;
}

bool Game::playerSetAttackedCreature(uint32_t playerId, uint32_t creatureId)
{
	Player* player = getPlayerByID(playerId);
	if(!player || player->isRemoved())
		return false;

	if(player->getAttackedCreature() && creatureId == 0){
		player->setAttackedCreature(NULL);
		player->sendCancelTarget();
		return true;
	}

	Creature* attackCreature = getCreatureByID(creatureId);
	if(!attackCreature){
		player->setAttackedCreature(NULL);
		player->sendCancelTarget();
		return false;
	}

	ReturnValue ret = Combat::canTargetCreature(player, attackCreature);
	if(ret != RET_NOERROR){
		player->sendCancelMessage(ret);
		player->sendCancelTarget();
		player->setAttackedCreature(NULL);
		return false;
	}

	player->setAttackedCreature(attackCreature);
	return true;
}

bool Game::playerFollowCreature(uint32_t playerId, uint32_t creatureId)
{
	Player* player = getPlayerByID(playerId);
	if(!player || player->isRemoved())
		return false;

	player->setAttackedCreature(NULL);
	Creature* followCreature = NULL;

	if(creatureId != 0){
		followCreature = getCreatureByID(creatureId);
	}

	return player->setFollowCreature(followCreature);
}

bool Game::playerInviteToParty(uint32_t playerId, uint32_t invitedId)
{
	Player* player = getPlayerByID(playerId);
	if(!player || player->isRemoved()){
		return false;
	}

	Player* invitedPlayer = getPlayerByID(invitedId);
	if(!invitedPlayer || invitedPlayer->isRemoved() || invitedPlayer->isInviting(player)){
		return false;
	}

	if(invitedPlayer->getParty()){
		std::stringstream ss;
		ss << invitedPlayer->getName() << " is already in a party.";
		player->sendTextMessage(MSG_INFO_DESCR, ss.str());
		return false;
	}

	Party* party = player->getParty();
	if(!party){
		party = new Party(player);
	}
	else if(party->getLeader() != player){
		return false;
	}

	return party->invitePlayer(invitedPlayer);
}

bool Game::playerJoinParty(uint32_t playerId, uint32_t leaderId)
{
	Player* player = getPlayerByID(playerId);
	if(!player || player->isRemoved()){
		return false;
	}

	Player* leader = getPlayerByID(leaderId);
	if(!leader || leader->isRemoved() || !leader->isInviting(player)){
		return false;
	}

	if(!leader->getParty() || leader->getParty()->getLeader() != leader){
		return false;
	}

	if(player->getParty()){
		player->sendTextMessage(MSG_INFO_DESCR, "You are already in a party.");
		return false;
	}

	return leader->getParty()->joinParty(player);
}

bool Game::playerRevokePartyInvitation(uint32_t playerId, uint32_t invitedId)
{
	Player* player = getPlayerByID(playerId);
	if(!player || player->isRemoved() || !player->getParty() || player->getParty()->getLeader() != player){
		return false;
	}

	Player* invitedPlayer = getPlayerByID(invitedId);
	if(!invitedPlayer || invitedPlayer->isRemoved() || !player->isInviting(invitedPlayer)){
		return false;
	}

	return player->getParty()->revokeInvitation(invitedPlayer);
}

bool Game::playerPassPartyLeadership(uint32_t playerId, uint32_t newLeaderId)
{
	Player* player = getPlayerByID(playerId);
	if(!player || player->isRemoved() || !player->getParty() || player->getParty()->getLeader() != player){
		return false;
	}

	Player* newLeader = getPlayerByID(newLeaderId);
	if(!newLeader || newLeader->isRemoved() || !player->isPartner(newLeader)){
		return false;
	}

	return player->getParty()->passPartyLeadership(newLeader);
}

bool Game::playerLeaveParty(uint32_t playerId)
{
	Player* player = getPlayerByID(playerId);
	if(!player || player->isRemoved()){
		return false;
	}

	if(!player->getParty() || player->hasCondition(CONDITION_INFIGHT)){
		return false;
	}

	return player->getParty()->leaveParty(player);
}

bool Game::playerEnableSharedPartyExperience(uint32_t playerId, uint8_t sharedExpActive, uint8_t unknown)
{
	Player* player = getPlayerByID(playerId);
	if(!player || player->isRemoved()){
		return false;
	}

	if(!player->getParty() || player->hasCondition(CONDITION_INFIGHT)){
		return false;
	}

	return player->getParty()->setSharedExperience(player, sharedExpActive == 1);
}

bool Game::playerShowQuestLog(uint32_t playerId)
{
	Player* player = getPlayerByID(playerId);
	if(!player || player->isRemoved()){
		return false;
	}

	player->sendQuestLog();
	return true;
}

bool Game::playerShowQuestLine(uint32_t playerId, uint16_t questId)
{
	Player* player = getPlayerByID(playerId);
	if(!player || player->isRemoved()){
		return false;
	}

	// REVSCRIPT TODO Event callback

	//player->sendQuestLine(quest);
	return true;
}

bool Game::playerSetFightModes(uint32_t playerId, FightMode fightMode, ChaseMode chaseMode, bool safeMode)
{
	Player* player = getPlayerByID(playerId);
	if(!player || player->isRemoved())
		return false;

	player->setFightMode(fightMode);
	player->setChaseMode(chaseMode);
	player->setSafeMode(safeMode);
	return true;
}

bool Game::playerRequestAddVip(uint32_t playerId, const std::string& vip_name)
{
	Player* player = getPlayerByID(playerId);
	if(!player || player->isRemoved())
		return false;

	std::string real_name;
	real_name = vip_name;
	uint32_t guid;
	bool specialVip;

	if(!IOPlayer::instance()->getGuidByNameEx(guid, specialVip, real_name)){
		player->sendTextMessage(MSG_STATUS_SMALL, "A player with that name does not exist.");
		return false;
	}
	if(specialVip && !player->hasFlag(PlayerFlag_SpecialVIP)){
		player->sendTextMessage(MSG_STATUS_SMALL, "You can not add this player.");
		return false;
	}

	bool online = (getPlayerByName(real_name) != NULL);
	return player->addVIP(guid, real_name, online);
}

bool Game::playerRequestRemoveVip(uint32_t playerId, uint32_t guid)
{
	Player* player = getPlayerByID(playerId);
	if(!player || player->isRemoved())
		return false;

	player->removeVIP(guid);
	return true;
}

bool Game::playerTurn(uint32_t playerId, Direction dir)
{
	Player* player = getPlayerByID(playerId);
	if(!player || player->isRemoved())
		return false;

	player->resetIdle();
	return internalCreatureTurn(player, dir);
}

bool Game::playerRequestOutfit(uint32_t playerId)
{
	Player* player = getPlayerByID(playerId);
	if(!player || player->isRemoved())
		return false;

	player->sendOutfitWindow();
	return true;
}

bool Game::playerChangeOutfit(uint32_t playerId, OutfitType outfit)
{
	Player* player = getPlayerByID(playerId);
	if(!player || player->isRemoved())
		return false;

	uint32_t outfitId = Outfits::getInstance()->getOutfitId(outfit.lookType);
	if(player->canWearOutfit(outfitId, outfit.lookAddons)){		
		if(onPlayerChangeOutfit(player, outfit)){
			player->defaultOutfit = outfit;

			if(player->hasCondition(CONDITION_OUTFIT)){
				return false;
			}

			internalCreatureChangeOutfit(player, outfit);
		}
	}

	return true;
}

bool Game::checkReload(Player* player, const std::string& text)
{
	if(text.length() > 7 && text.substr(0, 7) == "/reload" && player->hasFlag(PlayerFlag_CanReloadContent)){
		std::string param = text.substr(7);

		if(param == " monsters" || param == " monster" || param == "m"){
			std::cout << "================================================================================\n";
			g_creature_types.reload();
			std::cout << ":: Reloaded Monsters " << std::endl;
			if(player) player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Reloaded monsters.");
		}
		else if(param == " config" || param == "c"){
			std::cout << "================================================================================\n";
			g_config.reload();
			std::cout << ":: Reloaded config " << std::endl;
			if(player) player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Reloaded config.");
		}
		else if(param == " scripts" || param == "s"){
			std::cout << "================================================================================\n";
			
			runShutdownScripts(false);
			g_game.loadScripts();
			runStartupScripts(false);
			
			for(AutoList<Creature>::listiterator it = Game::listCreature.list.begin();
				it != Game::listCreature.list.end();
				++it)
			{
				Actor* a = it->second->getActor();
				if(a && a->shouldReload()){
					Script::OnSpawn::Event e(a, true);
					script_system->dispatchEvent(e);
				}
			}

			std::cout << ":: Reloaded Scripts " << (script_system == NULL? "[ Failed ]" : "") << std::endl;
			if(player) player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Reloaded scripts.");
		}
		return true;
	}
	return false;
}

bool Game::playerSay(uint32_t playerId, uint16_t channelId, SpeakClass type,
	std::string receiver, std::string text)
{
	Player* player = getPlayerByID(playerId);
	if(!player || player->isRemoved())
		return false;

	if(checkReload(player, text))
		return true;

	bool isMuteableChannel = g_chat.isMuteableChannel(channelId, type);
	uint32_t muteTime = player->getMuteTime();

	if(isMuteableChannel && muteTime > 0){
		std::stringstream ss;
		ss << "You are still muted for " << muteTime << " seconds.";
		player->sendTextMessage(MSG_STATUS_SMALL, ss.str());
		return false;
	}

	switch(type) {
		case SPEAK_PRIVATE:
		case SPEAK_PRIVATE_RED:
		case SPEAK_RVR_ANSWER:
		case SPEAK_RVR_CONTINUE:
		case SPEAK_RVR_CHANNEL:
			break;
		default:
			if(!script_system)
				break;
			Script::OnSay::Event evt(player, type, g_chat.getChannel(player, channelId), text);
			if(script_system->dispatchEvent(evt)) {
				// Handled
				return false;
			}
			break;
	}

	if(text.empty()) return false;

	if(isMuteableChannel){
		player->removeMessageBuffer();
	}

	switch(type){
		case SPEAK_SAY:
			return internalCreatureSay(player, SPEAK_SAY, text);
			break;
		case SPEAK_WHISPER:
			return playerWhisper(player, text);
			break;
		case SPEAK_YELL:
			return playerYell(player, text);
			break;
		case SPEAK_PRIVATE:
		case SPEAK_PRIVATE_RED:
		case SPEAK_RVR_ANSWER:
			return playerSpeakTo(player, type, receiver, text);
			break;
		case SPEAK_CHANNEL_Y:
		case SPEAK_CHANNEL_R1:
		case SPEAK_CHANNEL_R2:
			if(playerTalkToChannel(player, type, text, channelId)){
				return true;
			}
			else{
				// Resend in default channel
				return playerSay(playerId, 0, SPEAK_SAY, receiver, text);
			}
			break;
		case SPEAK_PRIVATE_PN:
			return playerSpeakToNpc(player, text);
			break;
		case SPEAK_BROADCAST:
			return internalBroadcastMessage(player, text);
			break;
		case SPEAK_RVR_CHANNEL:
			return playerReportRuleViolation(player, text);
			break;
		case SPEAK_RVR_CONTINUE:
			return playerContinueReport(player, text);
			break;

		default:
			break;
	}

	return false;
}

bool Game::playerWhisper(Player* player, const std::string& text)
{
	SpectatorVec list;
	SpectatorVec::iterator it;
	getSpectators(list, player->getPosition(), false, false,
		Map::maxClientViewportX, Map::maxClientViewportX,
		Map::maxClientViewportY, Map::maxClientViewportY);

	//send to client
	Player* tmpPlayer = NULL;
	for(it = list.begin(); it != list.end(); ++it){
		if((tmpPlayer = (*it)->getPlayer())){
			if(!Position::areInRange<1,1,0>(player->getPosition(), (*it)->getPosition())){
				tmpPlayer->sendCreatureSay(player, SPEAK_WHISPER, "pspsps");
			}
			else{
				tmpPlayer->sendCreatureSay(player, SPEAK_WHISPER, text);
			}
		}
	}

	//event method
	for(it = list.begin(); it != list.end(); ++it) {
		(*it)->onCreatureSay(player, SPEAK_WHISPER, text);
	}

	return true;
}

bool Game::playerYell(Player* player, const std::string& text)
{
	uint32_t addExhaustion = 0;
	bool isExhausted = false;
	if(!player->hasCondition(CONDITION_EXHAUSTED)){
		addExhaustion = getExhaustionTicks();
		internalCreatureSay(player, SPEAK_YELL, asUpperCaseString(text));
	}
	else{
		isExhausted = true;
		addExhaustion = getAddExhaustionTicks();
		player->sendCancelMessage(RET_YOUAREEXHAUSTED);
	}

	if(addExhaustion > 0){
		Condition* condition = Condition::createCondition(CONDITIONID_DEFAULT, CONDITION_EXHAUSTED, addExhaustion, 0);
		player->addCondition(condition);
	}

	return !isExhausted;
}

bool Game::playerSpeakTo(Player* player, SpeakClass type, const std::string& receiver,
	const std::string& text)
{
	Player* toPlayer = getPlayerByName(receiver);
	if(!toPlayer || player->isRemoved()) {
		player->sendTextMessage(MSG_STATUS_SMALL, "A player with this name is not online.");
		return false;
	}

	if(type == SPEAK_PRIVATE_RED && !player->hasFlag(PlayerFlag_CanTalkRedPrivate)){
		type = SPEAK_PRIVATE;
	}

	toPlayer->sendCreatureSay(player, type, text);
	toPlayer->onCreatureSay(player, type, text);

	std::stringstream ss;
	ss << "Message sent to " << toPlayer->getName() << ".";
	player->sendTextMessage(MSG_STATUS_SMALL, ss.str());
	return true;
}

bool Game::playerTalkToChannel(Player* player, SpeakClass type, const std::string& text, unsigned short channelId)
{
	if(type == SPEAK_CHANNEL_R1 && !player->hasFlag(PlayerFlag_CanTalkRedChannel)){
		type = SPEAK_CHANNEL_Y;
	}
	else if(type == SPEAK_CHANNEL_R2 && !player->hasFlag(PlayerFlag_CanTalkRedChannelAnonymous)){
		type = SPEAK_CHANNEL_Y;
	}

	return g_chat.talkToChannel(player, type, text, channelId);
}

bool Game::playerSpeakToNpc(Player* player, const std::string& text)
{
	SpectatorVec list;
	SpectatorVec::iterator it;
	getSpectators(list, player->getPosition());

	//send to npcs only
	// REVSCRIPTSYS TODO
	// Player speak to NPC (is this necessary?)
	/*
	Creature* tmpNpc = NULL;
	for(it = list.begin(); it != list.end(); ++it){
		if((tmpNpc = (*it)->getNpc())){
			(*it)->onCreatureSay(player, SPEAK_PRIVATE_PN, text);
		}
	}
	*/

	return true;
}

bool Game::playerReportRuleViolation(Player* player, const std::string& text)
{
	//Do not allow reports on multiclones worlds
	//Since reports are name-based
	if(g_config.getNumber(ConfigManager::ALLOW_CLONES)){
		player->sendTextMessage(MSG_INFO_DESCR, "Rule violations reports are disabled.");
		return false;
	}

	cancelRuleViolation(player);

	shared_ptr<RuleViolation> rvr(new RuleViolation(
		player,
		text,
		std::time(NULL)
		));

	ruleViolations[player->getID()] = rvr;

	ChatChannel* channel = g_chat.getChannelById(CHANNEL_RULE_REP);
	if(channel){
		for(UsersMap::const_iterator it = channel->getUsers().begin(); it != channel->getUsers().end(); ++it){
			if(it->second){
				it->second->sendToChannel(player, SPEAK_RVR_CHANNEL, text, CHANNEL_RULE_REP, rvr->time);
			}
		}
		return true;
	}

	return false;
}

bool Game::playerContinueReport(Player* player, const std::string& text)
{
	RuleViolationsMap::iterator it = ruleViolations.find(player->getID());
	if(it == ruleViolations.end()){
		return false;
	}

	RuleViolation& rvr = *it->second;
	Player* toPlayer = rvr.gamemaster;
	if(!toPlayer){
		return false;
	}

	toPlayer->sendCreatureSay(player, SPEAK_RVR_CONTINUE, text);

	player->sendTextMessage(MSG_STATUS_SMALL, "Message sent to Gamemaster.");
	return true;
}

bool Game::kickPlayer(uint32_t playerId)
{
	Player* player = getPlayerByID(playerId);
	if(!player || player->isRemoved())
		return false;

	player->kickPlayer();
	return true;
}

//--
bool Game::canThrowObjectTo(const Position& fromPos, const Position& toPos, bool checkLineOfSight /*= true*/,
	int32_t rangex /*= Map_maxClientViewportX*/, int32_t rangey /*= Map_maxClientViewportY*/)
{
	return map->canThrowObjectTo(fromPos, toPos, checkLineOfSight, rangex, rangey);
}

bool Game::isSightClear(const Position& fromPos, const Position& toPos, bool floorCheck)
{
	return map->isSightClear(fromPos, toPos, floorCheck);
}

bool Game::internalCreatureTurn(Creature* creature, Direction dir)
{
	Script::OnTurn::Event evt(creature, dir);
	
	if(!script_system || !script_system->dispatchEvent(evt)){
		if(creature->getDirection() != dir){
			creature->setDirection(dir);

			const SpectatorVec& list = getSpectators(creature->getPosition());
			SpectatorVec::const_iterator it;

			//send to client
			Player* tmpPlayer = NULL;
			for(it = list.begin(); it != list.end(); ++it) {
				if((tmpPlayer = (*it)->getPlayer())){
					tmpPlayer->sendCreatureTurn(creature);
				}
			}

			//event method
			for(it = list.begin(); it != list.end(); ++it) {
				(*it)->onCreatureTurn(creature);
			}
		}
	}

	return false;
}

bool Game::internalCreatureSay(Creature* creature, SpeakClass type, const std::string& text)
{
	// This somewhat complex construct ensures that the cached SpectatorVec
	// is used if available and if it can be used, else a local vector is
	// used. (Hopefully the compiler will optimize away the construction of
	// the temporary when it's not used.
	SpectatorVec list;
	SpectatorVec::const_iterator it;

	if(type == SPEAK_YELL || type == SPEAK_MONSTER_YELL){
		getSpectators(list, creature->getPosition(), false, true, 18, 18, 14, 14);
	}
	else{
		getSpectators(list, creature->getPosition(), false, false,
			Map::maxClientViewportX, Map::maxClientViewportX,
			Map::maxClientViewportY, Map::maxClientViewportY);
	}

	//send to client
	Player* tmpPlayer = NULL;
	for(it = list.begin(); it != list.end(); ++it){
		if((tmpPlayer = (*it)->getPlayer())){
			tmpPlayer->sendCreatureSay(creature, type, text);
		}
	}

	//event method
	for(it = list.begin(); it != list.end(); ++it){
		(*it)->onCreatureSay(creature, type, text);
	}

	return true;
}

bool Game::getPathTo(const Creature* creature, const Position& destPos,
	std::list<Direction>& listDir, int32_t maxSearchDist /*= -1*/)
{
	return map->getPathTo(creature, destPos, listDir, maxSearchDist);
}

bool Game::getPathToEx(const Creature* creature, const Position& targetPos,
	std::list<Direction>& dirList, const FindPathParams& fpp)
{
	return map->getPathMatching(creature, dirList, FrozenPathingConditionCall(targetPos), fpp);
}

bool Game::getPathToEx(const Creature* creature, const Position& targetPos, std::list<Direction>& dirList,
	uint32_t minTargetDist, uint32_t maxTargetDist, bool fullPathSearch /*= true*/,
	bool clearSight /*= true*/, int32_t maxSearchDist /*= -1*/)
{
	FindPathParams fpp;
	fpp.fullPathSearch = fullPathSearch;
	fpp.maxSearchDist = maxSearchDist;
	fpp.clearSight = clearSight;
	fpp.minTargetDist = minTargetDist;
	fpp.maxTargetDist = maxTargetDist;

	return getPathToEx(creature, targetPos, dirList, fpp);
}

void Game::checkCreatureWalk(uint32_t creatureId)
{
	Creature* creature = getCreatureByID(creatureId);
	if(creature && creature->getHealth() > 0){
		creature->onWalk();
		cleanup();
	}
}

void Game::updateCreatureWalk(uint32_t creatureId)
{
	Creature* creature = getCreatureByID(creatureId);
	if(creature && creature->getHealth() > 0){
		creature->getPathToFollowCreature();
	}
}

void Game::checkCreatureAttack(uint32_t creatureId)
{
	Creature* creature = getCreatureByID(creatureId);
	if(creature && creature->getHealth() > 0){
		creature->onAttacking(0);
	}
}

void Game::addCreatureCheck(Creature* creature)
{
	if(creature->isRemoved())
		return;

	creature->creatureCheck = true;

	if(creature->checkCreatureVectorIndex >= 0){
		// Already in a vector
		return;
	}

	toAddCheckCreatureVector.push_back(creature);
	creature->checkCreatureVectorIndex = random_range(0, EVENT_CREATURECOUNT - 1);
	creature->addRef();
}

void Game::removeCreatureCheck(Creature* creature)
{
	if(creature->checkCreatureVectorIndex == -1){
		// Not in any vector
		return;
	}

	creature->creatureCheck = false;
}

uint32_t Game::getPlayersOnline() 
{
	return (uint32_t)Player::listPlayer.list.size();
}

uint32_t Game::getMonstersOnline() 
{
	return (uint32_t)Actor::listMonster.list.size();
}

uint32_t Game::getCreaturesOnline() 
{
	return (uint32_t)listCreature.list.size();
}

void Game::checkCreatures()
{
	g_scheduler.addEvent(createSchedulerTask(
		EVENT_CHECK_CREATURE_INTERVAL, boost::bind(&Game::checkCreatures, this)));

	Creature* creature;
	std::vector<Creature*>::iterator it;

	//add any new creatures
	for(it = toAddCheckCreatureVector.begin(); it != toAddCheckCreatureVector.end(); ++it){
		creature = (*it);
		checkCreatureVectors[creature->checkCreatureVectorIndex].push_back(creature);
	}
	toAddCheckCreatureVector.clear();

	checkCreatureLastIndex++;
	if(checkCreatureLastIndex == EVENT_CREATURECOUNT){
		checkCreatureLastIndex = 0;
	}

	std::vector<Creature*>& checkCreatureVector = checkCreatureVectors[checkCreatureLastIndex];

	for(it = checkCreatureVector.begin(); it != checkCreatureVector.end();){
		creature = (*it);

		if(creature->creatureCheck){
			if(creature->getHealth() > 0){
				creature->onThink(EVENT_CREATURE_THINK_INTERVAL);
			}
			else{
				creature->onDie();
			}
			++it;
		}
		else{
			creature->checkCreatureVectorIndex = -1;
			it = checkCreatureVector.erase(it);
			FreeThing(creature);
		}
	}

	cleanup();
}

void Game::changeSpeed(Creature* creature, int32_t varSpeedDelta)
{
	int32_t varSpeed = creature->getSpeed() - creature->getBaseSpeed();
	varSpeed += varSpeedDelta;

	creature->setSpeed(varSpeed);

	const SpectatorVec& list = getSpectators(creature->getPosition());
	SpectatorVec::const_iterator it;

	//send to client
	Player* tmpPlayer = NULL;
	for(it = list.begin(); it != list.end(); ++it){
		if((tmpPlayer = (*it)->getPlayer())){
			tmpPlayer->sendChangeSpeed(creature, creature->getStepSpeed());
		}
	}
}

void Game::internalCreatureChangeOutfit(Creature* creature, const OutfitType& outfit)
{
	creature->setCurrentOutfit(outfit);

	if(!creature->isInvisible()){
		const SpectatorVec& list = getSpectators(creature->getPosition());
		SpectatorVec::const_iterator it;

		//send to client
		Player* tmpPlayer = NULL;
		for(it = list.begin(); it != list.end(); ++it) {
			if((tmpPlayer = (*it)->getPlayer())){
				tmpPlayer->sendCreatureChangeOutfit(creature, outfit);
			}
		}

		//event method
		for(it = list.begin(); it != list.end(); ++it) {
			(*it)->onCreatureChangeOutfit(creature, outfit);
		}
	}
}

void Game::internalCreatureChangeVisible(Creature* creature, bool visible)
{
	const SpectatorVec& list = getSpectators(creature->getPosition());
	SpectatorVec::const_iterator it;

	//send to client
	Player* tmpPlayer = NULL;
	for(it = list.begin(); it != list.end(); ++it) {
		if((tmpPlayer = (*it)->getPlayer())){
			tmpPlayer->sendCreatureChangeVisible(creature, visible);
		}
	}

	//event method
	for(it = list.begin(); it != list.end(); ++it) {
		(*it)->onCreatureChangeVisible(creature, visible);
	}
}

void Game::changeLight(const Creature* creature)
{
	const SpectatorVec& list = getSpectators(creature->getPosition());

	//send to client
	Player* tmpPlayer = NULL;
	for(SpectatorVec::const_iterator it = list.begin(); it != list.end(); ++it){
		if((tmpPlayer = (*it)->getPlayer())){
			tmpPlayer->sendCreatureLight(creature);
		}
	}
}

void Game::combatToTarget(Creature* attacker, CombatParams& params, Creature* target)
{
	CombatSource combatSource(attacker);
	combat.combatToTarget(combatSource, params, target);
}

void Game::combatToTarget(CombatSource combatSource, CombatParams& params, Creature* target)
{
	combat.combatToTarget(combatSource, params, target);
}

void Game::combatToArea(Creature* attacker, CombatParams& params, const Position& pos, const CombatArea* area)
{
	CombatSource combatSource(attacker);
	combat.combatToArea(combatSource, params, pos, area);
}

void Game::combatToArea(CombatSource combatSource, CombatParams& params, const Position& pos, const CombatArea* area)
{
	combat.combatToArea(combatSource, params, pos, area);
}

bool Game::combatBlockHit(CombatType combatType, CombatSource combatSource, Creature* target,
	int32_t& healthChange, bool checkDefense, bool checkArmor)
{
	if(target->getPlayer() && target->getPlayer()->hasFlag(PlayerFlag_CannotBeSeen)){
		return true;
	}

	if(healthChange > 0){
		return false;
	}

	const Position& targetPos = target->getPosition();
	const SpectatorVec& list = getSpectators(targetPos);

	if(!target->isAttackable() || Combat::canDoCombat(combatSource.getSourceCreature(), target) != RET_NOERROR){
		addMagicEffect(list, targetPos, NM_ME_PUFF);
		return true;
	}

	int32_t damage = -healthChange;
	BlockType blockType = target->blockHit(combatType, combatSource, damage, checkDefense, checkArmor);
	healthChange = -damage;

	if(blockType == BLOCK_DEFENSE){
		addMagicEffect(list, targetPos, NM_ME_PUFF);
		return true;
	}
	else if(blockType == BLOCK_ARMOR){
		addMagicEffect(list, targetPos, NM_ME_BLOCKHIT);
		return true;
	}
	else if(blockType == BLOCK_IMMUNITY){
		uint8_t hitEffect = 0;

		if(combatType == COMBAT_UNDEFINEDDAMAGE)
			; // Nothing
		else if(combatType == COMBAT_ENERGYDAMAGE &&
				combatType == COMBAT_FIREDAMAGE &&
				combatType == COMBAT_PHYSICALDAMAGE &&
				combatType == COMBAT_ICEDAMAGE &&
				combatType == COMBAT_DEATHDAMAGE)
			hitEffect = NM_ME_BLOCKHIT;
		else if(combatType == COMBAT_EARTHDAMAGE)
			hitEffect = NM_ME_POISON_RINGS;
		else if(combatType == COMBAT_HOLYDAMAGE)
			hitEffect = NM_ME_HOLYDAMAGE;
		else
			hitEffect = NM_ME_PUFF;

		addMagicEffect(list, targetPos, hitEffect);

		return true;
	}

	return false;
}

bool Game::combatChangeHealth(CombatType combatType, Creature* attacker,
	Creature* target, int32_t healthChange, bool showEffect /*= true*/)
{
	CombatSource combatSource(attacker);
	CombatEffect combatEffect(showEffect);
	return combatChangeHealth(combatType, combatSource, combatEffect, target, healthChange);
}

bool Game::combatChangeHealth(CombatType combatType, CombatSource combatSource,
	Creature* target, int32_t healthChange, bool showEffect /*= true*/)
{
	CombatEffect combatEffect(showEffect);
	return combatChangeHealth(combatType, combatSource, combatEffect, target, healthChange);
}

bool Game::combatChangeHealth(CombatType combatType, CombatSource combatSource, CombatEffect combatEffect,
	Creature* target, int32_t healthChange)
{
	if(!g_game.onCreatureDamage(combatType, combatSource, target, healthChange)){
		return true;
	}

	const Position& targetPos = target->getPosition();

	if(healthChange > 0){
		if(target->getHealth() <= 0){
			return false;
		}

		target->gainHealth(combatSource, healthChange);
	}
	else{
		const SpectatorVec& list = getSpectators(targetPos);

		if(!target->isAttackable() || Combat::canDoCombat(combatSource.getSourceCreature(), target) != RET_NOERROR){
			if(combatEffect.showEffect)
				addMagicEffect(list, targetPos, NM_ME_PUFF);
			return true;
		}

		int32_t damage = -healthChange;
		if(damage != 0){
			if(target->hasCondition(CONDITION_MANASHIELD) && combatType != COMBAT_UNDEFINEDDAMAGE){
				int32_t manaDamage = std::min(target->getMana(), damage);
				damage = std::max((int32_t)0, damage - manaDamage);

				if(manaDamage != 0){
					target->drainMana(combatSource, manaDamage, combatEffect.showEffect);

					std::stringstream ss;
					ss << manaDamage;
					if(combatEffect.showEffect){
						addMagicEffect(list, targetPos, NM_ME_LOSE_ENERGY);
						addAnimatedText(list, targetPos, TEXTCOLOR_BLUE, ss.str());
					}
				}
			}

			damage = std::min(target->getHealth(), damage);
			if(damage > 0){
				if(target->getHealth() - damage <= 0){
					// If event handler changes health we shouldn't
					int ohealth = target->getHealth();
					if(onCreatureKill(target, combatSource.getSourceCreature())){
						//prevent death
						damage = ohealth - 1;
					}
				}

				target->drainHealth(combatType, combatSource, damage, combatEffect.showEffect);

				if(combatEffect.showEffect){
					TextColor_t textColor = TEXTCOLOR_NONE;
					uint8_t hitEffect = 0;

					if(combatType == COMBAT_PHYSICALDAMAGE)
					{
						RaceType race = target->getRace();

						if(race == RACE_VENOM){
							textColor = TEXTCOLOR_LIGHTGREEN;
							hitEffect = NM_ME_POISON;
						}
						else if(race == RACE_BLOOD){
							textColor = TEXTCOLOR_RED;
							hitEffect = NM_ME_DRAW_BLOOD;
						}
						else if(race == RACE_UNDEAD){
							textColor = TEXTCOLOR_LIGHTGREY;
							hitEffect = NM_ME_HIT_AREA;
						}
						else if(race == RACE_FIRE){
							textColor = TEXTCOLOR_ORANGE;
							hitEffect = NM_ME_DRAW_BLOOD;
						}
						else if(race == RACE_ENERGY){
							textColor = TEXTCOLOR_PURPLE;
							hitEffect = NM_ME_ENERGY_DAMAGE;
						}
					}
					else if(combatType == COMBAT_ENERGYDAMAGE)
					{
						textColor = TEXTCOLOR_PURPLE;
						hitEffect = NM_ME_ENERGY_DAMAGE;
					}
					else if(combatType == COMBAT_EARTHDAMAGE)
					{
						textColor = TEXTCOLOR_LIGHTGREEN;
						hitEffect = NM_ME_POISON_RINGS;
					}
					else if(combatType == COMBAT_DROWNDAMAGE)
					{
						textColor = TEXTCOLOR_LIGHTBLUE;
						hitEffect = NM_ME_LOSE_ENERGY;
					}
					else if(combatType == COMBAT_FIREDAMAGE)
					{
						textColor = TEXTCOLOR_ORANGE;
						hitEffect = NM_ME_HITBY_FIRE;
					}
					else if(combatType == COMBAT_ICEDAMAGE)
					{
						textColor = TEXTCOLOR_LIGHTBLUE;
						hitEffect = NM_ME_ICEATTACK;
					}
					else if(combatType == COMBAT_HOLYDAMAGE)
					{
						textColor = TEXTCOLOR_YELLOW;
						hitEffect = NM_ME_HOLYDAMAGE;
					}
					else if(combatType == COMBAT_DEATHDAMAGE)
					{
						textColor = TEXTCOLOR_DARKRED;
						hitEffect = NM_ME_SMALLCLOUDS;
					}
					else if(combatType == COMBAT_LIFEDRAIN)
					{
						textColor = TEXTCOLOR_RED;
						hitEffect = NM_ME_MAGIC_BLOOD;
					}

					if(combatEffect.hitEffect != NM_ME_UNK)
						hitEffect = combatEffect.hitEffect;

					if(combatEffect.hitTextColor != TEXTCOLOR_UNK)
						textColor = combatEffect.hitTextColor;

					if(textColor != TEXTCOLOR_NONE){
						std::stringstream ss;
						ss << damage;
						addMagicEffect(list, targetPos, hitEffect);
						addAnimatedText(list, targetPos, textColor, ss.str());
					}
				}
			}
		}
	}

	return true;
}

bool Game::combatChangeMana(Creature* attacker, Creature* target,
	int32_t manaChange, bool showEffect /*= true*/)
{
	CombatSource combatSource(attacker);
	CombatEffect combatEffect(showEffect);
	return combatChangeMana(combatSource, combatEffect, target, manaChange);
}

bool Game::combatChangeMana(CombatSource combatSource, Creature* target,
	int32_t manaChange, bool showEffect /*= true*/)
{
	CombatEffect combatEffect(showEffect);
	return combatChangeMana(combatSource, combatEffect, target, manaChange);
}

bool Game::combatChangeMana(CombatSource combatSource, CombatEffect combatEffect,
	Creature* target, int32_t manaChange)
{
	CombatType combatType = COMBAT_MANADRAIN;
	if(!g_game.onCreatureDamage(combatType, combatSource, target, manaChange) || combatType != COMBAT_MANADRAIN){
		return true;
	}

	const Position& targetPos = target->getPosition();
	const SpectatorVec& list = getSpectators(targetPos);

	if(manaChange > 0){
		target->changeMana(manaChange);
	}
	else{
		if(!target->isAttackable() || Combat::canDoCombat(combatSource.getSourceCreature(), target) != RET_NOERROR){
			if(combatEffect.showEffect)
				addMagicEffect(list, targetPos, NM_ME_PUFF);
			return false;
		}

		int32_t manaLoss = std::min(target->getMana(), -manaChange);
		BlockType blockType = target->blockHit(COMBAT_MANADRAIN, combatSource, manaLoss);

		if(blockType != BLOCK_NONE){
			if(combatEffect.showEffect)
				addMagicEffect(list, targetPos, NM_ME_PUFF);
			return false;
		}

		if(manaLoss > 0){
			target->drainMana(combatSource, manaLoss, combatEffect.showEffect);

			if(combatEffect.showEffect){
				std::stringstream ss;
				ss << manaLoss;
				addAnimatedText(list, targetPos, TEXTCOLOR_BLUE, ss.str());
			}
		}
	}

	return true;
}

void Game::addCreatureHealth(const Creature* target)
{
	const SpectatorVec& list = getSpectators(target->getPosition());
	addCreatureHealth(list, target);
}

void Game::addCreatureHealth(const SpectatorVec& list, const Creature* target)
{
	Player* player = NULL;
	for(SpectatorVec::const_iterator it = list.begin(); it != list.end(); ++it){
		if((player = (*it)->getPlayer())){
			player->sendCreatureHealth(target);
		}
	}
}

void Game::addAnimatedText(const Position& pos, uint8_t textColor,
	const std::string& text)
{
	const SpectatorVec& list = getSpectators(pos);

	addAnimatedText(list, pos, textColor, text);
}

void Game::addAnimatedText(const SpectatorVec& list, const Position& pos, uint8_t textColor,
	const std::string& text)
{
	Player* player = NULL;
	for(SpectatorVec::const_iterator it = list.begin(); it != list.end(); ++it){
		if((player = (*it)->getPlayer())){
			player->sendAnimatedText(pos, textColor, text);
		}
	}
}

void Game::addMagicEffect(const Position& pos, uint8_t effect)
{
	const SpectatorVec& list = getSpectators(pos);

	addMagicEffect(list, pos, effect);
}

void Game::addMagicEffect(const SpectatorVec& list, const Position& pos, uint8_t effect)
{
	Player* player = NULL;
	for(SpectatorVec::const_iterator it = list.begin(); it != list.end(); ++it){
		if((player = (*it)->getPlayer())){
			player->sendMagicEffect(pos, effect);
		}
	}
}

void Game::addDistanceEffect(Creature* creature, const Position& fromPos, const Position& toPos,
	uint8_t effect)
{
	if(creature){
		if(effect == NM_SHOOT_WEAPONTYPE){
			switch(creature->getWeaponType()){
				case WEAPON_AXE: effect = NM_SHOOT_WHIRLWINDAXE; break;
				case WEAPON_SWORD: effect = NM_SHOOT_WHIRLWINDSWORD; break;
				case WEAPON_CLUB: effect = NM_SHOOT_WHIRLWINDCLUB; break;

				default: effect = NM_ME_NONE; break;
			}
		}
	}

	if(effect != NM_ME_NONE){
		SpectatorVec list;
		getSpectators(list, fromPos, false);
		getSpectators(list, toPos, true);

		//send to client
		Player* tmpPlayer = NULL;
		for(SpectatorVec::const_iterator it = list.begin(); it != list.end(); ++it){
			if((tmpPlayer = (*it)->getPlayer())){
				tmpPlayer->sendDistanceShoot(fromPos, toPos, effect);
			}
		}
	}
}

#ifdef __SKULLSYSTEM__
void Game::updateCreatureSkull(Player* player)
{
	const SpectatorVec& list = getSpectators(player->getPosition());

	//send to client
	Player* tmpPlayer = NULL;
	for(SpectatorVec::const_iterator it = list.begin(); it != list.end(); ++it){
		if((tmpPlayer = (*it)->getPlayer())){
			tmpPlayer->sendCreatureSkull(player);
		}
	}
}
#endif

void Game::startDecay(Item* item)
{
	Container* container = item->getContainer();
	if(container){
		for(ItemList::const_iterator it = container->getItems(); it != container->getEnd(); ++it){
			startDecay(const_cast<Item*>(*it));
		}
	}

	if(item && item->canDecay()){
		uint32_t decayState = item->getDecaying();
		if(decayState == DECAYING_TRUE){
			//already decaying
			return;
		}

		int32_t dur = item->getDuration();
		if(dur > 0){
			item->addRef();
			item->setDecaying(DECAYING_TRUE);
			toDecayItems.push_back(item);
		}
		else{
			internalDecayItem(item);
		}
	}
}

void Game::internalDecayItem(Item* item)
{
	const ItemType& it = Item::items[item->getID()];

	if(it.decayTo != 0){
		Item* newItem = transformItem(NULL, item, it.decayTo);
		startDecay(newItem);
	}
	else{
		ReturnValue ret = internalRemoveItem(NULL, item);

		if(ret != RET_NOERROR){
#ifdef __DEBUG__
			std::cout << "DEBUG, internalDecayItem failed, error code: " << (int) ret << "item id: " << item->getID() << std::endl;
#endif
		}
	}
}

void Game::checkDecay()
{
	g_scheduler.addEvent(createSchedulerTask(EVENT_DECAYINTERVAL,
		boost::bind(&Game::checkDecay, this)));
	
	size_t bucket = (last_bucket + 1) % EVENT_DECAY_BUCKETS;

	for(DecayList::iterator it = decayItems[bucket].begin(); it != decayItems[bucket].end();){
		Item* item = *it;
		int32_t decreaseTime = EVENT_DECAYINTERVAL*EVENT_DECAY_BUCKETS;
		if(item->getDuration() - decreaseTime < 0){
			decreaseTime = item->getDuration();
		}
		item->decreaseDuration(decreaseTime);
#ifdef __DEBUG__
		std::cout << "checkDecay: " << item << ", id:" << item->getID() << ", name: " << item->getName() << ", duration: " << item->getDuration() << std::endl;
#endif

		if(!item->canDecay()){
			item->setDecaying(DECAYING_FALSE);
			FreeThing(item);
			it = decayItems[bucket].erase(it);
			continue;
		}

		int32_t dur = item->getDuration();

		if(dur <= 0) {
			it = decayItems[bucket].erase(it);
			internalDecayItem(item);
			FreeThing(item);
		}
		else if(dur < EVENT_DECAYINTERVAL*EVENT_DECAY_BUCKETS)
		{
			it = decayItems[bucket].erase(it);
			size_t new_bucket = (bucket + ((dur + EVENT_DECAYINTERVAL/2) / 1000)) % EVENT_DECAY_BUCKETS;
			if(new_bucket == bucket) {
				internalDecayItem(item);
				FreeThing(item);
			} else {
				decayItems[new_bucket].push_back(item);
			}
		}
		else{
			++it;
		}
	}

	last_bucket = bucket;

	cleanup();
}

void Game::checkLight()
{
	g_scheduler.addEvent(createSchedulerTask(EVENT_LIGHTINTERVAL,
		boost::bind(&Game::checkLight, this)));

	light_hour = light_hour + light_hour_delta;
	if(light_hour > 1440)
		light_hour = light_hour - 1440;

	if(std::abs(light_hour - SUNRISE) < 2*light_hour_delta){
		light_state = LIGHT_STATE_SUNRISE;
	}
	else if(std::abs(light_hour - SUNSET) < 2*light_hour_delta){
		light_state = LIGHT_STATE_SUNSET;
	}

	int newlightlevel = lightlevel;
	bool lightChange = false;
	switch(light_state){
	case LIGHT_STATE_SUNRISE:
		newlightlevel += (LIGHT_LEVEL_DAY - LIGHT_LEVEL_NIGHT)/30;
		lightChange = true;
		break;
	case LIGHT_STATE_SUNSET:
		newlightlevel -= (LIGHT_LEVEL_DAY - LIGHT_LEVEL_NIGHT)/30;
		lightChange = true;
		break;
	default:
		break;
	}

	if(newlightlevel <= LIGHT_LEVEL_NIGHT){
		lightlevel = LIGHT_LEVEL_NIGHT;
		light_state = LIGHT_STATE_NIGHT;
	}
	else if(newlightlevel >= LIGHT_LEVEL_DAY){
		lightlevel = LIGHT_LEVEL_DAY;
		light_state = LIGHT_STATE_DAY;
	}
	else{
		lightlevel = newlightlevel;
	}

	if(lightChange){
		LightInfo lightInfo;
		getWorldLightInfo(lightInfo);
		for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it){
			(*it).second->sendWorldLight(lightInfo);
		}
	}
}

void Game::getWorldLightInfo(LightInfo& lightInfo)
{
	lightInfo.level = lightlevel;
	lightInfo.color = 0xD7;
}

bool Game::cancelRuleViolation(Player* player)
{
	RuleViolationsMap::iterator it = ruleViolations.find(player->getID());
	if(it == ruleViolations.end()){
		return false;
	}

	Player* gamemaster = it->second->gamemaster;
	if(!it->second->isOpen && gamemaster){
		//Send to the responder
		gamemaster->sendRuleViolationCancel(player->getName());
	}
	else{
		//Send to channel
		ChatChannel* channel = g_chat.getChannelById(CHANNEL_RULE_REP);
		if(channel){
			for(UsersMap::const_iterator ut = channel->getUsers().begin(); ut != channel->getUsers().end(); ++ut){
				if(ut->second){
					ut->second->sendRemoveReport(player->getName());
				}
			}
		}
	}

	//Now erase it
	ruleViolations.erase(it);
	return true;
}

bool Game::closeRuleViolation(Player* player)
{
	RuleViolationsMap::iterator it = ruleViolations.find(player->getID());
	if(it == ruleViolations.end()){
		return false;
	}

	ruleViolations.erase(it);
	player->sendLockRuleViolation();

	ChatChannel* channel = g_chat.getChannelById(CHANNEL_RULE_REP);
	if(channel){
		for(UsersMap::const_iterator ut = channel->getUsers().begin(); ut != channel->getUsers().end(); ++ut){
			if(ut->second){
				ut->second->sendRemoveReport(player->getName());
			}
		}
	}

	return true;
}

void Game::addCommandTag(std::string tag)
{
	bool found = false;
	for(uint32_t i=0; i< commandTags.size() ;i++){
		if(commandTags[i] == tag){
			found = true;
			break;
		}
	}

	if(!found){
		commandTags.push_back(tag);
	}
}

void Game::resetCommandTag()
{
	commandTags.clear();
}


void Game::shutdown()
{
	std::cout << "Shutting down server...";

	g_scheduler.shutdown();
	g_dispatcher.shutdown();
	Spawns::getInstance()->clear();

	cleanup();

	if(service_manager){
		service_manager->stop();
	}

	std::cout << "[done]" << std::endl;
}

void Game::cleanup()
{
	//free memory
	for(std::vector<Thing*>::iterator it = toReleaseThings.begin(); it != toReleaseThings.end(); ++it){
		(*it)->unRef();
	}

	toReleaseThings.clear();

	//turn tiles into faster IndexedTiles (but takes more memory)
	for(std::vector<Position>::iterator it = toIndexTiles.begin(); it != toIndexTiles.end(); ++it){
		map->makeTileIndexed(*it);
	}

	for(DecayList::iterator it = toDecayItems.begin(); it != toDecayItems.end(); ++it){
		int32_t dur = (*it)->getDuration();
		if(dur >= EVENT_DECAYINTERVAL * EVENT_DECAY_BUCKETS) {
			decayItems[last_bucket].push_back(*it);
		} else {
			decayItems[(last_bucket + 1 + (*it)->getDuration() / 1000) % EVENT_DECAY_BUCKETS].push_back(*it);
		}
	}

	toDecayItems.clear();
}

void Game::makeTileIndexed(Tile* tile)
{
	toIndexTiles.push_back(tile->getPosition());
}

void Game::FreeThing(Thing* thing)
{
	toReleaseThings.push_back(thing);
}

void Game::showUseHotkeyMessage(Player* player, Item* item)
{
	int32_t subType = -1;
	if(item->hasSubType() && !item->hasCharges()){
		subType = item->getSubType();
	}
	const ItemType& it = Item::items[item->getID()];
	uint32_t itemCount = player->__getItemTypeCount(item->getID(), subType, false);

	std::stringstream ss;
	if(itemCount == 1){
		ss << "Using the last " << it.name << "...";
	}
	else{
		ss << "Using one of " << itemCount << " " << it.pluralName << "...";
	}

	player->sendTextMessage(MSG_INFO_DESCR, ss.str());
}

bool Game::playerViolationWindow(uint32_t playerId, std::string targetName, uint8_t reasonId, ViolationAction actionType,
		std::string comment, std::string statement, uint16_t channelId, bool ipBanishment)
{
	Player* player = getPlayerByID(playerId);
	if(!player || player->isRemoved())
		return false;

	uint32_t access = player->getViolationLevel();
	if((ipBanishment && ((violationNames[access] & Action_IpBan) != Action_IpBan || (violationStatements[access] & Action_IpBan) != Action_IpBan)) ||
		!(violationNames[access] & (1 << actionType) || violationStatements[access] & (1 << actionType)) || reasonId > violationReasons[access])
	{
		player->sendCancel("You do not have authorization for this action.");
		return false;
	}

	if(comment.size() > 1000){
		std::stringstream ss;
		ss << "The comment may not exceed 1000 characters.";
		player->sendCancel(ss.str());
		return false;
	}

	toLowerCaseString(targetName);
	uint32_t guid = 0;
	if(!IOPlayer::instance()->getGuidByName(guid, targetName)){
		player->sendCancel("A player with this name does not exist.");
		return false;
	}

	uint32_t ip = 0;
	std::string acc;

	Player* targetPlayer = getPlayerByName(targetName);
	if(targetPlayer){
		if(targetPlayer->hasFlag(PlayerFlag_CannotBeBanned)){
			player->sendCancel("You do not have authorization for this action.");
			return false;
		}

		acc = targetPlayer->getAccountName();
		ip = targetPlayer->getIP();
	}
	else{
		if(IOPlayer::instance()->hasFlag(PlayerFlag_CannotBeBanned, guid)){
			player->sendCancel("You do not have authorization for this action.");
			return false;
		}

		IOPlayer::instance()->getAccountByName(acc, targetName);
		IOPlayer::instance()->getLastIP(ip, guid);
	}

	Account account =  IOAccount::instance()->loadAccount(acc, true);
	int16_t removeNotations = 2; //2 - remove notations & kick, 1 - kick, 0 - nothing
		
	if(actionType == ACTION_NOTATION)
	{
		g_bans.addAccountNotation(account.number, player->getGUID(), comment, statement, reasonId, actionType);
		if(g_bans.getNotationsCount(account.number) >= (uint32_t)g_config.getNumber(ConfigManager::NOTATIONS_TO_BAN)){
			account.warnings++;
			if(account.warnings >= (uint32_t)g_config.getNumber(ConfigManager::WARNINGS_TO_DELETION)){
				actionType = ACTION_DELETION;
				g_bans.addAccountBan(account.number, -1, player->getGUID(), comment, statement, reasonId, actionType);
			}
			else if(account.warnings >= (uint32_t)g_config.getNumber(ConfigManager::WARNINGS_TO_FINALBAN)){
				if(!g_bans.addAccountBan(account.number, (time(NULL) + g_config.getNumber(
					ConfigManager::FINALBAN_LENGTH)), player->getGUID(), comment, statement, reasonId, actionType))
					account.warnings--;
			}
			else{
				if(!g_bans.addAccountBan(account.number, (time(NULL) + g_config.getNumber(
					ConfigManager::BAN_LENGTH)), player->getGUID(), comment, statement, reasonId, actionType))
					account.warnings--;
			}
		}
		else
			removeNotations = 0;
	}

	else if(actionType == ACTION_NAMEREPORT)
	{
		g_bans.addPlayerBan(guid, -1, player->getGUID(), comment, statement, reasonId, actionType);
		removeNotations = 1;
	}

	else if(actionType == ACTION_BANREPORT)
	{
		if(account.warnings >= (uint32_t)g_config.getNumber(ConfigManager::WARNINGS_TO_DELETION)){
			actionType = ACTION_DELETION;
			g_bans.addAccountBan(account.number, -1, player->getGUID(), comment, statement, reasonId, actionType);
		}
		else
		{
			account.warnings++;
			if(account.warnings >= (uint32_t)g_config.getNumber(ConfigManager::WARNINGS_TO_FINALBAN)){
				if(!g_bans.addAccountBan(account.number, (time(NULL) + g_config.getNumber(
					ConfigManager::FINALBAN_LENGTH)), player->getGUID(), comment, statement, reasonId, actionType))
					account.warnings--;
			}
			else{
				if(!g_bans.addAccountBan(account.number, (time(NULL) + g_config.getNumber(
					ConfigManager::BAN_LENGTH)), player->getGUID(), comment, statement, reasonId, actionType))
					account.warnings--;
			}

			g_bans.addPlayerBan(guid, -1, player->getGUID(), comment, statement, reasonId, actionType);
		}
	}

	else if(actionType == ACTION_BANFINAL)
	{
		if(account.warnings++ >= (uint32_t)g_config.getNumber(ConfigManager::WARNINGS_TO_DELETION)){
			actionType = ACTION_DELETION;
			if(!g_bans.addAccountBan(account.number, -1, player->getGUID(), comment, statement, reasonId, actionType))
				account.warnings--;
		}
		else if(g_bans.addAccountBan(account.number, (time(NULL) + g_config.getNumber(
				ConfigManager::FINALBAN_LENGTH)), player->getGUID(), comment, statement, reasonId, actionType))
		{
			account.warnings = g_config.getNumber(ConfigManager::WARNINGS_TO_DELETION) - 1;
		}
	}

	else if(actionType == ACTION_BANREPORTFINAL)
	{
		if(account.warnings++ >= (uint32_t)g_config.getNumber(ConfigManager::WARNINGS_TO_DELETION)){
			actionType = ACTION_DELETION;
			if(!g_bans.addAccountBan(account.number, -1, player->getGUID(), comment, statement, reasonId, actionType))
				account.warnings--;
		}
		else{
			if(g_bans.addAccountBan(account.number, (time(NULL) + g_config.getNumber(
				ConfigManager::FINALBAN_LENGTH)), player->getGUID(), comment, statement, reasonId, actionType))
				account.warnings = g_config.getNumber(ConfigManager::WARNINGS_TO_DELETION);

			g_bans.addPlayerBan(guid, -1, player->getGUID(), comment, statement, reasonId, actionType);
		}
	}

	else if(actionType == ACTION_STATEMENT)
	{
		ChannelStatementMap::iterator it = Player::channelStatementMap.find(channelId);
		if(it != Player::channelStatementMap.end()){
			statement = it->second;
			g_bans.addPlayerStatement(guid, player->getGUID(), comment, statement, reasonId, actionType);
			Player::channelStatementMap.erase(it);
		}
		else{
			player->sendCancel("Statement has already been reported.");
			return false;
		}

		removeNotations = 0;
	}

	else
	{
		account.warnings++;
		if(account.warnings >= (uint32_t)g_config.getNumber(ConfigManager::WARNINGS_TO_DELETION)){
			actionType = ACTION_DELETION;
			g_bans.addAccountBan(account.number, -1, player->getGUID(), comment, statement, reasonId, actionType);
		}
		else if(account.warnings >= (uint32_t)g_config.getNumber(ConfigManager::WARNINGS_TO_FINALBAN)){
			if(!g_bans.addAccountBan(account.number, (time(NULL) + g_config.getNumber(
				ConfigManager::FINALBAN_LENGTH)), player->getGUID(), comment, statement, reasonId, actionType))
				account.warnings--;
		}
		else if(!g_bans.addAccountBan(account.number, (time(NULL) + g_config.getNumber(
			ConfigManager::BAN_LENGTH)), player->getGUID(), comment, statement, reasonId, actionType))
			account.warnings--;
	}

	if(ipBanishment && ip > 0)
		g_bans.addIpBan(ip, 0xFFFFFFFF, (time(NULL) + g_config.getNumber(ConfigManager::IPBANISHMENT_LENGTH)),
			player->getGUID(), comment);

	if(removeNotations > 1)
		g_bans.removeNotations(account.number);

	bool announceViolation = g_config.getNumber(ConfigManager::BROADCAST_BANISHMENTS) != 0;
	std::ostringstream ss;
	if(actionType == ACTION_STATEMENT){
		if(announceViolation){
			ss << player->getName() << " has taken the action";
		}
		else{
			ss << "You have taken the action";
		}

		ss << "  \"" << getViolationActionString(actionType, ipBanishment) << "\" " <<
			"for the statement: \"" << statement << "\" against: " << targetName << " " <<
			"(Warnings: " << account.warnings << "), with reason: \"" << getViolationReasonString(reasonId) <<
			"\", and comment: \"" << comment << "\".";
	}
	else{
		if(announceViolation){
			ss << player->getName() << " has taken the action";
		}
		else{
			ss << "You have taken the action";
		}

		ss << " \"" << getViolationActionString(actionType, ipBanishment) << "\" against: " << targetName <<
			" (Warnings: " << account.warnings << "), with reason: \"" << getViolationReasonString(reasonId) <<
			"\", and comment: \"" << comment << "\".";
	}

	if(announceViolation){
		anonymousBroadcastMessage(MSG_STATUS_WARNING, ss.str());
	}
	else{
		player->sendTextMessage(MSG_STATUS_CONSOLE_RED, ss.str());
	}

	if(targetPlayer && removeNotations > 0){
		targetPlayer->sendTextMessage(MSG_INFO_DESCR, "You have been banished.");
		addMagicEffect(targetPlayer->getPosition(), NM_ME_MAGIC_POISON);

		uint32_t targetId = targetPlayer->getID();
		g_scheduler.addEvent(createSchedulerTask(1000, boost::bind(
			&Game::kickPlayer, this, targetId)));
	}

	IOAccount::instance()->saveAccount(account);
	return true;
}

bool Game::playerReportBug(uint32_t playerId, std::string comment)
{
	Player* player = getPlayerByID(playerId);
	if(!player || player->isRemoved())
		return false;

	if(!player->hasFlag(PlayerFlag_CanReportBugs)){ //avoid saving unwanted stuff
		return false;
	}

	std::ofstream of("player_reports.txt", std::ios_base::out | std::ios_base::app);

	if(of){
		const Position& pos = player->getPosition();

		char today[32];
		formatDate(time(NULL), today);

		of <<
			"-------------------------------------------------------------------------------" << std::endl <<
			today << " - " << player->getName() <<
			"[x:" << pos.x << " y:" << pos.y << " z:" << pos.z << "]" << std::endl <<
			"\tComment: " << comment << std::endl;

		player->sendTextMessage(MSG_EVENT_DEFAULT, "Your report has been sent.");
		return true;
	}
	return false;
}

void Game::unscriptThing(Thing* thing)
{
	script_environment->removeThing(thing);
}

void Game::unscript(void* v)
{
	script_environment->removeObject(v);
}

// Shortens compilation time as it can be called without including game.h
void g_gameUnscriptThing(Thing* thing)
{
	g_game.unscriptThing(thing);
}


// Shortens compilation time as it can be called without including game.h
void g_gameUnscript(void* v)
{
	g_game.unscript(v);
}


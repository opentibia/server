//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// class representing the gamestate
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
#ifndef __OTSERV_GAME_H__
#define __OTSERV_GAME_H__

#include "definitions.h"
#include "map.h"
#include "position.h"
#include "item.h"
#include "container.h"
#include "player.h"
#include "npc.h"
#include "spawn.h"
#include "templates.h"
#include "enums.h"
#include "scheduler.h"
#include <queue>
#include <vector>
#include <set>

class ServiceManager;
class Player;
class Creature;
class Monster;
class Npc;
class CombatInfo;

enum stackPosType_t
{
	STACKPOS_NORMAL,
	STACKPOS_MOVE,
	STACKPOS_LOOK,
	STACKPOS_USE,
	STACKPOS_USEITEM
};

enum WorldType_t
{
	WORLD_TYPE_OPTIONAL_PVP = 1,
	WORLD_TYPE_OPEN_PVP = 2,
	WORLD_TYPE_HARDCORE_PVP = 3
};

enum GameState_t
{
	GAME_STATE_STARTUP,
	GAME_STATE_INIT,
	GAME_STATE_NORMAL,
	GAME_STATE_CLOSED,
	GAME_STATE_SHUTDOWN,
	GAME_STATE_CLOSING,
	GAME_STATE_LAST = GAME_STATE_CLOSING
};

enum LightState_t
{
	LIGHT_STATE_DAY,
	LIGHT_STATE_NIGHT,
	LIGHT_STATE_SUNSET,
	LIGHT_STATE_SUNRISE
};

struct RuleViolation
{
	RuleViolation(Player* _reporter, const std::string& _text, const uint32_t& _time)
		: reporter(_reporter)
		, gamemaster(NULL)
		, text(_text)
		, time(_time)
		, isOpen(true)
	{}

	Player* reporter;
	Player* gamemaster;
	std::string text;
	uint32_t time;
	bool isOpen;

private:
	RuleViolation(const RuleViolation&);
};

typedef std::map< uint32_t, shared_ptr<RuleViolation> > RuleViolationsMap;
typedef std::vector<Player*> PlayerVector;

#define EVENT_LIGHTINTERVAL  10000
#define EVENT_DECAYINTERVAL  1000
#define EVENT_DECAY_BUCKETS  16

/**
  * Main Game class.
  * This class is responsible to control everything that happens
  */

class Game
{
public:
	Game();
	~Game();

	void start(ServiceManager* servicer);

	/**
	  * Load a map.
	  * \param filename Mapfile to load
	  * \param filekind Kind of the map, BIN SQL or TXT
	  * \return Int 0 built-in spawns, 1 needs xml spawns, 2 needs sql spawns, -1 if got error
	  */
	int loadMap(std::string filename, std::string filekind);

	/**
	  * Get the map size - info purpose only
	  * \param width width of the map
	  * \param height height of the map
	  */
	void getMapDimensions(uint32_t& width, uint32_t& height) const
	{
		width = map->mapWidth;
		height = map->mapHeight;
		return;
	}

	void setWorldType(const WorldType_t& type);
	const WorldType_t& getWorldType() const;

	Cylinder* internalGetCylinder(Player* player, const Position& pos);
	Thing* internalGetThing(Player* player, const Position& pos, const int32_t& index,
		const uint32_t& spriteId = 0, const stackPosType_t& type = STACKPOS_NORMAL);
	void internalGetPosition(Item* item, Position& pos, uint8_t& stackpos);

	/**
	  * Get a single tile of the map.
	  * \return A pointer to the tile
		*/
	Tile* getTile(const int32_t& x, const int32_t& y, const int32_t& z);
	Tile* getTile(const Position& pos);

	/**
	  * Set a single tile of the map, position is read from this tile
		*/
	void setTile(Tile* newtile);

	/**
	  * Get a leaf of the map.
	  * \return A pointer to a leaf
		*/
	QTreeLeafNode* getLeaf(const uint32_t& x, const uint32_t& y);

	/**
	  * Returns a creature based on the unique creature identifier
	  * \param id is the unique creature id to get a creature pointer to
	  * \return A Creature pointer to the creature
	  */
	Creature* getCreatureByID(const uint32_t& id);

	/**
	  * Returns a player based on the unique creature identifier
	  * \param id is the unique player id to get a player pointer to
	  * \return A Pointer to the player
	  */
	Player* getPlayerByID(const uint32_t& id);

	/**
	  * Returns a creature based on a string name identifier
	  * \param s is the name identifier
	  * \return A Pointer to the creature
	  */
	Creature* getCreatureByName(const std::string& s);

	/**
	  * Returns a player based on a string name identifier
	  * \param s is the name identifier
	  * \return A Pointer to the player
	  */
	Player* getPlayerByName(const std::string& s);

	/**
	  * Returns a player based on a string name identifier
	  * this function returns a pointer even if the player is offline,
	  * it is up to the caller of the function to delete the pointer - if the player is offline
	  * use isOffline() to determine if the player was offline
	  * \param s is the name identifier
	  * \return A Pointer to the player
	  */
	Player* getPlayerByNameEx(const std::string& s);

	/**
	  * Returns a player based on a guid identifier
	  * this function returns a pointer even if the player is offline,
	  * it is up to the caller of the function to delete the pointer - if the player is offline
	  * use isOffline() to determine if the player was offline
	  * \param guid is the identifier
	  * \return A Pointer to the player
	  */
	Player* getPlayerByGuid(const uint32_t& guid);

	/**
	  * Returns a player based on a guid identifier
	  * this function returns a pointer even if the player is offline,
	  * it is up to the caller of the function to delete the pointer - if the player is offline
	  * use isOffline() to determine if the player was offline
	  * \param guid is the identifier
	  */
	Player* getPlayerByGuidEx(const uint32_t& guid);

	/**
	  * Returns a player based on a string name identifier, with support for the "~" wildcard.
	  * \param s is the name identifier, with or without wildcard
	  * \param player will point to the found player (if any)
	  * \return "RET_PLAYERWITHTHISNAMEISNOTONLINE" or "RET_NAMEISTOOAMBIGIOUS"
	  */
	ReturnValue getPlayerByNameWildcard(const std::string& s, Player* &player);

	/**
	  * Returns a player based on an account number identifier
	  * \param acc is the account identifier
	  * \return A Pointer to the player
	  */
	Player* getPlayerByAccount(const uint32_t& acc);

	/**
	  * Returns all players based on their account number identifier
	  * \param acc is the account identifier
	  * \return A vector of all players with the selected account number
	  */
	PlayerVector getPlayersByAccount(const uint32_t& acc);

	/**
	  * Returns all players with a certain IP address
	  * \param ip is the IP address of the clients, as an unsigned long
	  * \param mask An IP mask, default 255.255.255.255
	  * \return A vector of all players with the selected IP
	  */
	PlayerVector getPlayersByIP(const uint32_t& ip, const uint32_t& mask = 0xFFFFFFFF);

	/** Place Creature on the map without sending out events to the surrounding.
	  * \param creature Creature to place on the map
	  * \param pos The position to place the creature
	  * \param forced If true, placing the creature will not fail because of obstacles (creatures/items)
	  */
	bool internalPlaceCreature(Creature* creature, const Position& pos,
		bool extendedPos = false, bool forced = false);

	/**
	  * Place Creature on the map.
	  * \param creature Creature to place on the map
	  * \param pos The position to place the creature
	  * \param forced If true, placing the creature will not fail because of obstacles (creatures/items)
	  */
	bool placeCreature(Creature* creature, const Position& pos, bool extendedPos = false, bool force = false);

	/**
		* Remove Creature from the map.
		* Removes the Creature the map
		* \param c Creature to remove
		*/
	bool removeCreature(Creature* creature, bool isLogout = true);

	void addCreatureCheck(Creature* creature);
	void removeCreatureCheck(Creature* creature);

	uint32_t getPlayersOnline() const;
	uint32_t getMonstersOnline() const;
	uint32_t getNpcsOnline() const;
	uint32_t getCreaturesOnline() const;

	void getWorldLightInfo(LightInfo& lightInfo);

	void getSpectators(SpectatorVec& list, const Position& centerPos,
	                   bool checkforduplicate = false, bool multifloor = false,
	                   const int32_t& minRangeX = 0, const int32_t& maxRangeX = 0,
	                   const int32_t& minRangeY = 0, const int32_t& maxRangeY = 0);
	const SpectatorVec& getSpectators(const Position& centerPos);
	void clearSpectatorCache();

	ReturnValue internalMoveCreature(Creature* creature, const Direction& direction, const uint32_t& flags = 0);
	ReturnValue internalMoveCreature(Creature* creature, Cylinder* fromCylinder,
		Cylinder* toCylinder, const uint32_t& flags = 0);

	ReturnValue internalMoveItem(Cylinder* fromCylinder, Cylinder* toCylinder,
		int32_t index, Item* item, const uint32_t& count, Item** _moveItem, uint32_t flags = 0);

	ReturnValue internalAddItem(Cylinder* toCylinder, Item* item, 
		const int32_t& index = INDEX_WHEREEVER,	const uint32_t& flags = 0, bool test = false);
	ReturnValue internalAddItem(Cylinder* toCylinder, Item* item,
		int32_t index, uint32_t flags, bool test, uint32_t& remainderCount);
	ReturnValue internalRemoveItem(Item* item, int32_t count = -1, bool test = false, uint32_t flags = 0);

	ReturnValue internalPlayerAddItem(Player* player, Item* item,
		bool dropOnMap = true, const slots_t& slot = SLOT_WHEREEVER);

	/**
	  * Find an item of a certain type
	  * \param cylinder to search the item
	  * \param itemId is the item to remove
	  * \param depthSearch if true it will check child containers aswell
	  * \param subType is the extra type an item can have such as charges/fluidtype, default is -1
		* meaning it's not used
	  * \return A pointer to the item to an item and NULL if not found
	  */
	Item* findItemOfType(Cylinder* cylinder, const uint16_t& itemId,
	                     bool depthSearch = true, const int32_t& subType = -1);

	/**
	  * Remove item(s) of a certain type
	  * \param cylinder to remove the item(s) from
	  * \param itemId is the item to remove
	  * \param count is the amount to remove
	  * \param subType is the extra type an item can have such as charges/fluidtype, default is -1
		* meaning it's not used
	  * \param onlyContainers if true it will remove only items from containers in cylinder, default is false
		* meaning it's disabled
	  * \return true if the removal was successful
	  */
	bool removeItemOfType(Cylinder* cylinder, const uint16_t& itemId,
		const int32_t& count, const int32_t& subType = -1, bool onlyContainers = false);

	/**
	  * Get the amount of money in a a cylinder
	  * \return the amount of money found
	  */
	uint32_t getMoney(const Cylinder* cylinder);

	/**
	  * Remove item(s) with a monetary value
	  * \param cylinder to remove the money from
	  * \param money is the amount to remove
	  * \param flags optional flags to modifiy the default behaviour
	  * \return true if the removal was successful
	  */
	bool removeMoney(Cylinder* cylinder, const uint32_t& money, const uint32_t& flags = 0);

	/**
	  * Add item(s) with monetary value
	  * \param cylinder which will receive money
	  * \param money the amount to give
	  * \param flags optional flags to modify default behavior
	  * \return true
	  */
	bool addMoney(Cylinder* cylinder, const uint32_t& money, const uint32_t& flags = 0);

	/**
	  * Transform one item to another type/count
	  * \param item is the item to transform
	  * \param newtype is the new type
	  * \param newCount is the new count value, use default value (-1) to not change it
	  * \return true if the tranformation was successful
	  */
	Item* transformItem(Item* item, const uint16_t& newId, const int32_t& newCount = -1);

	/**
	  * Teleports an object to another position
	  * \param thing is the object to teleport
	  * \param newPos is the new position
	  * \param flags optional flags to modify default behavior
	  * \return true if the teleportation was successful
	  */
	ReturnValue internalTeleport(Thing* thing, const Position& newPos, const uint32_t& flags = 0);

	/**
		* Turn a creature to a different direction.
		* \param creature Creature to change the direction
		* \param dir Direction to turn to
		*/
	bool internalCreatureTurn(Creature* creature, const Direction& dir);

	/**
	  * Creature wants to say something.
	  * \param creature Creature pointer
	  * \param type Type of message
	  * \param text The text to say
	  */
	bool internalCreatureSay(Creature* creature, SpeakClasses type, const std::string& text);

	bool internalStartTrade(Player* player, Player* partner, Item* tradeItem);
	bool internalCloseTrade(Player* player);
	bool internalBroadcastMessage(Player* player, const std::string& text);

	bool anonymousBroadcastMessage(const MessageClasses& type, const std::string& text);

	//Implementation of player invoked events
	bool playerMoveThing(const uint32_t& playerId, const Position& fromPos,
		const uint16_t& spriteId, const uint8_t& fromStackPos, const Position& toPos, const uint8_t& count);
	bool playerMoveCreature(const uint32_t& playerId, const uint32_t& movingCreatureId,
		const Position& movingCreatureOrigPos, const Position& toPos);
	bool playerMoveItem(const uint32_t& playerId, const Position& fromPos,
		const uint16_t& spriteId, const uint8_t& fromStackPos, const Position& toPos, const uint8_t& count);
	bool playerMove(const uint32_t& playerId, const Direction& dir);
	bool playerCreatePrivateChannel(const uint32_t& playerId);
	bool playerChannelInvite(const uint32_t& playerId, const std::string& name);
	bool playerChannelExclude(const uint32_t& playerId, const std::string& name);
	bool playerRequestChannels(const uint32_t& playerId);
	bool playerOpenChannel(const uint32_t& playerId, const uint16_t& channelId);
	bool playerCloseChannel(const uint32_t& playerId, const uint16_t& channelId);
	bool playerOpenPrivateChannel(const uint32_t& playerId, const std::string& receiver);
	bool playerCloseNpcChannel(const uint32_t& playerId);
	bool playerProcessRuleViolation(const uint32_t& playerId, const std::string& name);
	bool playerCloseRuleViolation(const uint32_t& playerId, const std::string& name);
	bool playerCancelRuleViolation(const uint32_t& playerId);
	bool playerReceivePing(const uint32_t& playerId);
	bool playerAutoWalk(const uint32_t& playerId, std::list<Direction>& listDir);
	bool playerStopAutoWalk(const uint32_t& playerId);
	bool playerUseItemEx(const uint32_t& playerId, const Position& fromPos,
		const uint8_t& fromStackPos, const uint16_t& fromSpriteId, const Position& toPos,
		const uint8_t& toStackPos, const uint16_t& toSpriteId, bool isHotkey);
	bool playerUseItem(const uint32_t& playerId, const Position& pos,
		const uint8_t& stackPos, const uint8_t& index, const uint16_t& spriteId, bool isHotkey);
	bool playerUseBattleWindow(const uint32_t& playerId, const Position& fromPos,
		const uint8_t& fromStackPos, const uint32_t& creatureId, const uint16_t& spriteId, bool isHotkey);
	bool playerCloseContainer(const uint32_t& playerId, const uint8_t& cid);
	bool playerMoveUpContainer(const uint32_t& playerId, const uint8_t& cid);
	bool playerUpdateContainer(const uint32_t& playerId, const uint8_t& cid);
	bool playerUpdateTile(const uint32_t& playerId, const Position& pos);
	bool playerRotateItem(const uint32_t& playerId, const Position& pos, 
		const uint8_t& stackPos, const uint16_t& spriteId);
	bool playerWriteItem(const uint32_t& playerId, const uint32_t& windowTextId, const std::string& text);
	bool playerUpdateHouseWindow(const uint32_t& playerId, const uint8_t& listId,
		const uint32_t& windowTextId, const std::string& text);
	bool playerRequestTrade(const uint32_t& playerId, const Position& pos,
		const uint8_t& stackPos, const uint32_t& tradePlayerId, const uint16_t& spriteId);
	bool playerAcceptTrade(const uint32_t& playerId);
	bool playerLookInTrade(const uint32_t& playerId, bool lookAtCounterOffer, int index);
	bool playerCloseTrade(const uint32_t& playerId);
	bool playerPurchaseItem(const uint32_t& playerId, const uint16_t& spriteId,
		const uint8_t& count, const uint8_t& amount, bool ignoreCapacity = false, bool buyWithBackpack = false);
	bool playerSellItem(const uint32_t& playerId, const uint16_t& spriteId,
		const uint8_t& count, const uint8_t& amount, bool ignoreEquipped = false);
	bool playerCloseShop(const uint32_t& playerId);
	bool playerLookInShop(const uint32_t& playerId, const uint16_t& spriteId, const uint8_t& count);
	bool playerSetAttackedCreature(const uint32_t& playerId, const uint32_t& creatureId);
	bool playerFollowCreature(const uint32_t& playerId, const uint32_t& creatureId);
	bool playerCancelAttackAndFollow(const uint32_t& playerId);
	bool playerSetFightModes(const uint32_t& playerId, const fightMode_t& fightMode,
		const chaseMode_t& chaseMode, bool safeMode);
	bool playerLookAt(const uint32_t& playerId, const Position& pos,
		const uint16_t& spriteId, const uint8_t& stackPos);
	bool playerRequestAddVip(const uint32_t& playerId, const std::string& name);
	bool playerRequestRemoveVip(const uint32_t& playerId, const uint32_t& guid);
	bool playerTurn(const uint32_t& playerId, const Direction& dir);
	bool playerRequestOutfit(const uint32_t& playerId);
	bool playerSay(const uint32_t& playerId, const uint16_t& channelId,
		const SpeakClasses& type, const std::string& receiver, const std::string& text);
	bool checkPlayerMute(const uint16_t& channelId, const SpeakClasses& type);
	bool playerChangeOutfit(const uint32_t& playerId, const Outfit_t& outfit);
	bool playerInviteToParty(const uint32_t& playerId, const uint32_t& invitedId);
	bool playerJoinParty(const uint32_t& playerId, const uint32_t& leaderId);
	bool playerRevokePartyInvitation(const uint32_t& playerId, const uint32_t& invitedId);
	bool playerPassPartyLeadership(const uint32_t& playerId, const uint32_t& newLeaderId);
	bool playerLeaveParty(const uint32_t& playerId);
	bool playerEnableSharedPartyExperience(const uint32_t& playerId,
		const uint8_t& sharedExpActive, const uint8_t& unknown);
	bool playerShowQuestLog(const uint32_t& playerId);
	bool playerShowQuestLine(const uint32_t& playerId, const uint16_t& questId);
	bool playerViolationWindow(const uint32_t& playerId, const std::string& targetName,
		const uint8_t& reasonId, violationAction_t actionType, const std::string& comment,
		std::string statement, const uint16_t& channelId, bool ipBanishment);
	bool playerReportBug(const uint32_t& playerId, const std::string& comment);
	bool playerRegisterWalkAction(const uint32_t& playerId, SchedulerTask* task);

	void cleanup();
	void shutdown();
	void FreeThing(Thing* thing);

	bool canThrowObjectTo(const Position& fromPos, const Position& toPos,
		bool checkLineOfSight = true, int32_t rangex = Map::maxClientViewportX,
		int32_t rangey = Map::maxClientViewportY);
	bool isSightClear(const Position& fromPos, const Position& toPos, bool sameFloor);

	bool getPathTo(const Creature* creature, const Position& destPos,
	               std::list<Direction>& listDir, const int32_t& maxSearchDist /*= -1*/);

	bool getPathToEx(const Creature* creature, const Position& targetPos,
		std::list<Direction>& dirList, const FindPathParams& fpp);

	bool getPathToEx(const Creature* creature, const Position& targetPos, 
		std::list<Direction>& dirList, const uint32_t& minTargetDist, const uint32_t& maxTargetDist,
		bool fullPathSearch = true, bool clearSight = true, const int32_t& maxSearchDist = -1);

	void changeSpeed(Creature* creature, const int32_t& varSpeedDelta);
	void internalCreatureChangeOutfit(Creature* creature, const Outfit_t& oufit);
	void internalCreatureChangeVisible(Creature* creature, bool visible);
#ifdef __MIN_PVP_LEVEL_APPLIES_TO_SUMMONS__
	void forceClientsToReloadCreature(const Creature* creature);
#endif
	void changeLight(const Creature* creature);

	void updateCreatureSkull(Player* player);

#ifdef __GUILDWARSLUARELOAD__
	void updateCreatureEmblem(Creature* creature);
#endif
	GameState_t getGameState();
	void setGameState(const GameState_t& newState);
	bool saveServer(bool payHouses, bool shallowSave = false);
	void saveGameState();
	void loadGameState();
	void refreshMap(Map::TileMap::iterator* begin = NULL, int clean_max = 0);
	void proceduralRefresh(Map::TileMap::iterator* begin = NULL);

	//Events
	void checkCreatureWalk(const uint32_t& creatureId);
	void updateCreatureWalk(const uint32_t& creatureId);
	void checkCreatureAttack(const uint32_t& creatureId);
	void checkCreatures();
	void checkLight();
	bool kickPlayer(const uint32_t& playerId);

	bool combatBlockHit(const CombatType_t& combatType, Creature* attacker,
		Creature* target, int32_t& healthChange, bool checkDefense, bool checkArmor);

	bool combatChangeHealth(CombatType_t combatType, Creature* attacker, Creature* target, int32_t healthChange);
	bool combatChangeHealth(CombatType_t combatType, MagicEffectClasses hitEffect, TextColor_t customTextColor, Creature* attacker, Creature* target, int32_t healthChange);
	bool combatChangeMana(Creature* attacker, Creature* target, int32_t manaChange);

	//animation help functions
	void addCreatureHealth(const Creature* target);
	void addCreatureHealth(const SpectatorVec& list, const Creature* target);
	void addAnimatedText(const Position& pos, const uint8_t& textColor,
		const std::string& text);
	void addAnimatedText(const SpectatorVec& list, const Position& pos,
		const uint8_t& textColor, const std::string& text);
	void addMagicEffect(const Position& pos, const uint8_t& effect);
	void addMagicEffect(const SpectatorVec& list, const Position& pos, const uint8_t& effect);
	void addDistanceEffect(const Position& fromPos, const Position& toPos, const uint8_t& effect);

	std::string getTradeErrorDescription(const ReturnValue& ret, Item* item);

	void startDecay(Item* item);

	Map* getMap();
	const Map* getMap() const;
	int getLightHour();
	const RuleViolationsMap& getRuleViolations() const;
	bool cancelRuleViolation(Player* player);
	bool closeRuleViolation(Player* player);

	void showUseHotkeyMessage(Player* player, Item* item);

	void reloadInfo(const reloadTypes_t& info);

protected:

	bool playerSaySpell(Player* player, const SpeakClasses& type, const std::string& text);
	bool playerWhisper(Player* player, const std::string& text);
	bool playerYell(Player* player, const std::string& text);
	bool playerSpeakTo(Player* player, const SpeakClasses& type,
		const std::string& receiver, const std::string& text);
	bool playerTalkToChannel(Player* player, const SpeakClasses& type,
		const std::string& text, const uint16_t& channelId);
	bool playerSpeakToNpc(Player* player, const std::string& text);
	bool playerReportRuleViolation(Player* player, const std::string& text);
	bool playerContinueReport(Player* player, const std::string& text);

	std::vector<Thing*> ToReleaseThings;

	uint32_t checkLightEvent;
	uint32_t checkCreatureEvent;
	uint32_t checkDecayEvent;

	//list of items that are in trading state, mapped to the player
	std::map<Item*, uint32_t> tradeItems;

	//list of reported rule violations, for correct channel listing
	RuleViolationsMap ruleViolations;

	AutoList<Creature> listCreature;
	size_t checkCreatureLastIndex;
	std::vector<Creature*> checkCreatureVectors[EVENT_CREATURECOUNT];
	std::vector<Creature*> toAddCheckCreatureVector;

	struct GameEvent
	{
		int64_t  tick;
		int      type;
		void*    data;
	};

	void checkDecay();
	void internalDecayItem(Item* item);

	typedef std::list<Item*> DecayList;
	DecayList decayItems[EVENT_DECAY_BUCKETS];
	DecayList toDecayItems;
	size_t last_bucket;

	static const int LIGHT_LEVEL_DAY = 250;
	static const int LIGHT_LEVEL_NIGHT = 40;
	static const int SUNSET = 1305;
	static const int SUNRISE = 430;
	int lightlevel;
	LightState_t light_state;
	int light_hour;
	int light_hour_delta;

	GameState_t gameState;
	WorldType_t worldType;

	ServiceManager* service_manager;
	Map* map;
};

#endif

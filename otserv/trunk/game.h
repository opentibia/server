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

#include <queue>
#include <vector>
#include <set>

#include "map.h"
#include "position.h"
#include "item.h"
#include "container.h"
#include "player.h"
#include "npc.h"
#include "spawn.h"
#include "templates.h"
#include "scheduler.h"

class Creature;
class Player;
class Monster;
class Npc;
class CombatInfo;
class Commands;
class Task;
class lessSchedTask;
class SchedulerTask;

#define STACKPOS_MOVE -1
#define STACKPOS_LOOK -2
#define STACKPOS_USE -3

enum WorldType_t {
	WORLD_TYPE_NO_PVP,
	WORLD_TYPE_PVP,
	WORLD_TYPE_PVP_ENFORCED,
};

enum GameState_t {
	GAME_STATE_STARTUP,
	GAME_STATE_NORMAL,
	GAME_STATE_CLOSED,
	GAME_STATE_SHUTDOWN,
};

enum LightState_t {
	LIGHT_STATE_DAY,
	LIGHT_STATE_NIGHT,
	LIGHT_STATE_SUNSET,
	LIGHT_STATE_SUNRISE,
};

/**
  * Main Game class.
  * This class is responsible to controll everything that happens
  */

class Game
{
public:
  Game();
	~Game();
	
	/**
	  * Load a map.
	  * \param filename Mapfile to load
	  * \param filekind Kind of the map, BIN SQL or TXT
	  * \returns Int 0 built-in spawns, 1 needs xml spawns, 2 needs sql spawns, -1 if got error
	  */
	int loadMap(std::string filename, std::string filekind);
	
	/**
	  * Get the map size - info purpose only
	  * \param a the referenced witdh var
	  * \param b the referenced height var
	  */
	void getMapDimensions(int& a, int& b) {
		a = map->mapwidth;  
		b = map->mapheight;  
		return;
	}
  
	void setWorldType(WorldType_t type);
	WorldType_t getWorldType() const {return worldType;}
	int32_t getInFightTicks() {return inFightTicks;}
	int32_t getExhaustionTicks() {return exhaustionTicks;}

	Cylinder* internalGetCylinder(Player* player, const Position& pos);
	Thing* internalGetThing(Player* player, const Position& pos, int32_t index);

	/**
	  * Get a single tile of the map.
	  * \returns A pointer to the tile
		*/
	Tile* getTile(uint32_t x, uint32_t y, uint32_t z);

	/**
	  * Get a leaf of the map.
	  * \returns A pointer to a leaf
		*/
	QTreeLeafNode* getLeaf(uint32_t x, uint32_t y);

	/**
	  * Returns a creature based on the unique creature identifier
	  * \param id is the unique creature id to get a creature pointer to
	  * \returns A Creature pointer to the creature
	  */
	Creature* getCreatureByID(unsigned long id);

	/**
	  * Returns a player based on the unique creature identifier
	  * \param id is the unique player id to get a player pointer to
	  * \returns A Pointer to the player
	  */
	Player* getPlayerByID(unsigned long id);

	/**
	  * Returns a creature based on a string name identifier
	  * \param s is the name identifier
	  * \returns A Pointer to the creature
	  */
	Creature* getCreatureByName(const std::string& s);

	/**
	  * Returns a player based on a string name identifier
	  * \param s is the name identifier
	  * \returns A Pointer to the player
	  */
	Player* getPlayerByName(const std::string& s);

	/**
	  * Starts an event.
	  * \returns A unique identifier for the event
	  */
	unsigned long addEvent(SchedulerTask*);

	/**
	  * Stop an event.
	  * \param eventid unique identifier to an event
	  */
	bool stopEvent(unsigned long eventid);

	/**
	  * Place Player on the map.
	  * \param player Player to place on the map
	  * \param pos The position to place the player
	  * \param forced If true, placing the player will not fail becase of obstacles (creatures/chests)
	  */
	bool placePlayer(Player* player, const Position& pos, bool forced = false);

	/**
	  * Place Creature on the map.
	  * \param creature Creature to place on the map
	  * \param pos The position to place the creature
	  * \param forced If true, placing the creature will not fail becase of obstacles (creatures/chests)
	  */
	bool placeCreature(Creature* creature, const Position& pos, bool forced = false);

	/**
		* Remove Creature from the map.
		* Removes the Creature the map
		* \param c Creature to remove
		*/
	bool removeCreature(Creature* creature, bool isLogout = true);

	uint32_t getPlayersOnline() {return (uint32_t)Player::listPlayer.list.size();}
	uint32_t getMonstersOnline() {return (uint32_t)Monster::listMonster.list.size();}
	uint32_t getNpcsOnline() {return (uint32_t)Npc::listNpc.list.size();}
	uint32_t getCreaturesOnline() {return (uint32_t)listCreature.list.size();}

	void getWorldLightInfo(LightInfo& lightInfo);

	void getSpectators(SpectatorVec& list, const Position& centerPos, bool multifloor = false,
		int32_t minRangeX = 0, int32_t maxRangeX = 0,
		int32_t minRangeY = 0, int32_t maxRangeY = 0);

	void thingMove(Player* player, const Position& fromPos, uint16_t spriteId, uint8_t fromStackpos,
		const Position& toPos, uint8_t count);

	void moveCreature(uint32_t playerID, const Position& playerPos, uint32_t movingCreatureID, const Position& toPos);

	ReturnValue internalMoveCreature(Creature* creature, Direction direction);
	ReturnValue internalMoveCreature(Creature* creature, Cylinder* fromCylinder, Cylinder* toCylinder);

	void moveItem(Player* player, Cylinder* fromCylinder, Cylinder* toCylinder, int32_t index,
		Item* item, uint32_t count, uint16_t spriteId);
	
	ReturnValue internalMoveItem(Cylinder* fromCylinder, Cylinder* toCylinder, int32_t index,
		Item* item, uint32_t count, uint32_t flags = 0);

	ReturnValue internalAddItem(Cylinder* toCylinder, Item* item, int32_t index = INDEX_WHEREEVER,
		uint32_t flags = 0, bool test = false);
	ReturnValue internalRemoveItem(Item* item, int32_t count = -1,  bool test = false);

	ReturnValue internalPlayerAddItem(Player* player, Item* item);

	/**
	  * Remove item(s) of a certain type
	  * \param cylinder to remove the item(s) from
	  * \param spriteId is the item sprite to remove
	  * \param count is the amount to remove
	  * \returns true if the removal was successful
	  */
	bool removeItemOfType(Cylinder* cylinder, uint16_t itemId, int32_t count);

	/**
	  * Get the amount of money in a a cylinder
	  * \returns the amount of money found
	  */
	uint32_t getMoney(Cylinder* cylinder);

	/**
	  * Remove item(s) with a monetary value
	  * \param cylinder to remove the money from
	  * \param money is the amount to remove
	  * \param flags optional flags to modifiy the default behaviour
	  * \returns true if the removal was successful
	  */
	bool removeMoney(Cylinder* cylinder, int32_t money, uint32_t flags = 0);

	/**
	  * Transform one item to another type/count
	  * \param item is the item to transform
	  * \param newtype is the new type
	  * \param count is the new count value, use default value (-1) to not change it
	  * \returns true if the tranformation was successful
	  */
	Item* transformItem(Item* item, uint16_t newtype, int32_t count = -1);

	/**
	  * Teleports an object to another position
	  * \param thing is the object to teleport
	  * \param newPos is the new position
	  * \returns true if the teleportation was successful
	  */
	ReturnValue internalTeleport(Thing* thing, const Position& newPos);

	/**
		* Turn a creature to a different direction.
		* \param creature Creature to change the direction
		* \param dir Direction to turn to
		*/
	bool internalCreatureTurn(Creature* creature, Direction dir);

	/**
	  * Creature wants to say something.
	  * \param creature Creature pointer
	  * \param type Type of message
	  * \param text The text to say
	  */
	bool internalCreatureSay(Creature* creature, SpeakClasses type, const std::string& text);

	//Implementation of player invoked events
	bool movePlayer(Player* player, Direction direction);
	bool playerWhisper(Player* player, const std::string& text);
	bool playerYell(Player* player, std::string& text);
	bool playerSpeakTo(Player* player, SpeakClasses type, const std::string& receiver, const std::string& text);
	bool playerBroadcastMessage(Player* player, const std::string& text);
	bool anonymousBroadcastMessage(const std::string& text);
	bool playerTalkToChannel(Player* player, SpeakClasses type, const std::string& text, unsigned short channelId);
	bool playerAutoWalk(Player* player, std::list<Direction>& listDir);
	bool playerStopAutoWalk(Player* player);
	bool playerUseItemEx(Player* player, const Position& fromPos, uint8_t fromStackPos, uint16_t fromSpriteId,
		const Position& toPos, uint8_t toStackPos, uint16_t toSpriteId);
	bool internalUseItemEx(Player* player, const Position& fromPos, Item* item, const Position& toPos, uint8_t toStackPos, uint16_t toSpriteId);
	bool playerUseItem(Player* player, const Position& pos, uint8_t stackpos, uint8_t index, uint16_t spriteId);
	bool playerUseBattleWindow(Player* player, const Position& fromPos, uint8_t fromStackPos,
		uint32_t creatureId, uint16_t spriteId);
	bool playerRotateItem(Player* player, const Position& pos, uint8_t stackpos, const uint16_t spriteId);
	bool playerWriteItem(Player* player, Item* item, const std::string& text);

	bool playerRequestTrade(Player* player, const Position& pos, uint8_t stackpos,
		uint32_t playerId, uint16_t spriteId);
	bool playerAcceptTrade(Player* player);
	bool playerLookInTrade(Player* player, bool lookAtCounterOffer, int index);
	bool playerCloseTrade(Player* player);
	bool internalStartTrade(Player* player, Player* partner, Item* tradeItem);
	bool playerSetAttackedCreature(Player* player, unsigned long creatureId);
	bool playerFollowCreature(Player* player, unsigned long creatureId);
	bool playerSetFightModes(Player* player, fightMode_t fightMode, chaseMode_t chaseMode);
	bool playerLookAt(Player* player, const Position& pos, uint16_t spriteId, uint8_t stackpos);
	bool playerRequestAddVip(Player* player, const std::string& vip_name);
	bool playerTurn(Player* player, Direction dir);
	bool playerSay(Player* player, SpeakClasses type, const std::string& text);
	bool playerChangeOutfit(Player* player, Outfit_t outfit);
	bool playerSaySpell(Player* player, SpeakClasses type, const std::string& text);

	void flushSendBuffers();
	void addPlayerBuffer(Player* p);
	void FreeThing(Thing* thing);

	bool canThrowObjectTo(const Position& fromPos, const Position& toPos);
	bool getPathTo(const Creature* creature, Position toPosition, std::list<Direction>& listDir);
	bool isPathValid(const Creature* creature, const std::list<Direction>& listDir, const Position& destPos);

	bool getPathToEx(const Creature* creature, const Position& targetPos, uint32_t minDist, uint32_t maxDist,
		bool fullPathSearch, bool targetMustBeReachable, std::list<Direction>& dirList);

	void changeSpeed(Creature* creature, int32_t varSpeedDelta);
	void internalCreatureChangeOutfit(Creature* creature, const Outfit_t& oufit);
	void internalCreatureChangeVisible(Creature* creature, bool visible);
	void changeLight(const Creature* creature);

#ifdef __SKULLSYSTEM__
	void changeSkull(Player* player, Skulls_t newSkull);
#endif
	
	GameState_t getGameState();
	void setGameState(GameState_t newstate);

	//Lock variable for Game class
	OTSYS_THREAD_LOCKVAR gameLock;   

	//Events
	void checkWalk(unsigned long creatureId);
	void checkCreature(uint32_t creatureId, uint32_t interval);
	void checkLight(int t);
	
	bool combatBlockHit(CombatType_t combatType, Creature* attacker, Creature* target,
		int32_t& healthChange, bool checkDefense, bool checkArmor);
	
	bool combatChangeHealth(CombatType_t combatType, Creature* attacker, Creature* target, int32_t healthChange);
	bool combatChangeMana(Creature* attacker, Creature* target, int32_t manaChange);

	//animation help functions
	void addCreatureHealth(const Creature* target);
	void addCreatureHealth(const SpectatorVec& list, const Creature* target);
	void addAnimatedText(const Position& pos, uint8_t textColor,
		const std::string& text);
	void addAnimatedText(const SpectatorVec& list, const Position& pos, uint8_t textColor,
		const std::string& text);
	void addMagicEffect(const Position& pos, uint8_t effect);
	void addMagicEffect(const SpectatorVec& list, const Position& pos, uint8_t effect);
	void addDistanceEffect(const Position& fromPos, const Position& toPos,
	uint8_t effect);

	void startDecay(Item* item);

protected:
	std::vector<Player*> BufferedPlayers;
	std::vector<Thing*> ToReleaseThings;

	//list of items that are in trading state, mapped to the player
	std::map<Item*, unsigned long> tradeItems;
	
	AutoList<Creature> listCreature;

	OTSYS_THREAD_LOCKVAR eventLock;
	OTSYS_THREAD_SIGNALVAR eventSignal;

	static OTSYS_THREAD_RETURN eventThread(void *p);

#ifdef __DEBUG_CRITICALSECTION__
	static OTSYS_THREAD_RETURN monitorThread(void *p);
#endif

	struct GameEvent
	{
		int64_t  tick;
		int      type;
		void*    data;
	};
	
	#define DECAY_INTERVAL  10000
	void checkDecay(int32_t interval);
	void internalDecayItem(Item* item);

	typedef std::vector<Item*> DecayList;
	DecayList decayItems;
	
	static const int LIGHT_LEVEL_DAY = 250;
	static const int LIGHT_LEVEL_NIGHT = 40;
	static const int SUNSET = 1305;
	static const int SUNRISE = 430;
	int lightlevel;
	LightState_t light_state;
	int light_hour;
	int light_hour_delta;
	
	std::priority_queue<SchedulerTask*, std::vector<SchedulerTask*>, lessSchedTask > eventList;
	std::map<unsigned long, SchedulerTask*> eventIdMap;
	unsigned long eventIdCount;

	uint32_t maxPlayers;
	uint32_t inFightTicks;
	uint32_t exhaustionTicks;

	GameState_t gameState;
	WorldType_t worldType;

	Map* map;
	
	std::vector<std::string> commandTags;
	void addCommandTag(std::string tag);
	void resetCommandTag();

	friend class Commands;
	friend class Monster;
	friend class Npc;
	friend class GameState;
	friend class Spawn;
	friend class SpawnManager;
	friend class LuaScriptInterface;
	friend class Actions;
	friend class AdminProtocol;
	friend class Combat;
	friend class AreaCombat;
	friend class Action;
};

template<class ArgType>
class TCallList : public SchedulerTask{
public:
	TCallList(
		boost::function<bool(Game*, ArgType)> f1,
		Task* f2,
		std::list<ArgType>& call_list,
		int64_t interval) :
			_f1(f1), _f2(f2), _list(call_list), _interval(interval)
	{
		//
	}
	
	virtual void operator()(Game* arg)
	{
		if(_eventid != 0){
			bool ret = _f1(arg, _list.front());
			_list.pop_front();

			if(ret){
				if(_list.empty()){
					//callback function
					if(_f2){
						(_f2)(arg);
						delete _f2;
					}
				}
				else{
					//fire next task
					SchedulerTask* newTask = new TCallList(_f1, _f2, _list, _interval);
					newTask->setTicks(_interval);
					newTask->setEventId(this->getEventId());
					arg->addEvent(newTask);
				}
			}
		}
	}

private:
	boost::function<bool(Game*, ArgType)> _f1;
	Task* _f2;

	std::list<ArgType> _list;
	int64_t _interval;
};

template<class ArgType>
SchedulerTask* makeTask(int64_t ticks,
	boost::function<bool(Game*, ArgType)>* f1,
	std::list<ArgType>& call_list,
	int64_t interval,
	Task* f2)
{
	TCallList<ArgType>* t = new TCallList<ArgType>(f1, f2, call_list, interval);
	t->setTicks(ticks);
	return t;
}

#endif

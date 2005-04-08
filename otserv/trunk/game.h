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


#ifndef __OTSERV_GAME_H
#define __OTSERV_GAME_H


#include <queue>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include "position.h"
#include "item.h"
#include "container.h"
#include "creature.h"
#include "monster.h"
#include "player.h"
#include "magic.h"
#include "otsystem.h"
#include "map.h"

#include "scheduler.h"
#include "networkmessage.h"


class Creature;   // see creature.h
class Player;

#define MAP_WIDTH    512
#define MAP_HEIGHT   512
#define MAP_LAYER     16

/** State of a creature at a given time
  * Keeps track of all the changes to be able to send to the client
	*/

class CreatureState {
public:
	CreatureState() {};
	~CreatureState() {};

	int damage;
	int manaDamage;
	bool drawBlood;
	std::vector<Creature*> attackerlist;
};

//typedef map::map<Creature*, CreatureState> CreatureStateMap;
typedef std::vector< std::pair<Creature*, CreatureState> > CreatureStateVec;
typedef std::map<Tile*, CreatureStateVec> CreatureStates;

/** State of the game at a given time
  * Keeps track of all the changes to be able to send to the client
	*/

class Game;

class GameState {
public:
	GameState(Game *game, const Range &range);
	~GameState() {};

	void onAttack(Creature* attacker, const Position& pos, const MagicEffectClass* me);
	void onAttack(Creature* attacker, const Position& pos, Creature* attackedCreature);
	void getChanges(Player* spectator, NetworkMessage &msg);
	const CreatureStateVec& getCreatureStateList(Tile* tile) {return creaturestates[tile];};
	const std::vector<Creature*>& getSpectators() {return spectatorlist;}

protected:
	void addCreatureState(Tile* tile, Creature* attackedCreature, int damage, int manaDamage, bool drawBlood);
	void onAttackedCreature(Tile* tile, Creature* attacker, Creature* attackedCreature, int damage, bool drawBlood);
	void removeCreature(Creature* creature, unsigned char stackPos);
	Game *game;

	std::vector<Creature*> spectatorlist;
	MapState mapstate;
	CreatureStates creaturestates;
};

/**
  * Main Game class.
  * This class is responsible to controll everything that happens
  */

class Game {
  public:
		Game();
    ~Game();
	
	/**
	  * Load the map from a file.
	  * Delegates the actual loading to the map-class
	  * \param filename the name of the mapfile to load
	  */
    bool loadMap(std::string filename);

	/**
	  * Get a single tile of the map.
	  * \returns A Pointer to the tile */
    Tile* getTile(unsigned short _x, unsigned short _y, unsigned char _z);

	/**
	  * Set a Tile to a specific ground id
	  * \param groundId ID of the ground to set
	  */
		void setTile(unsigned short _x, unsigned short _y, unsigned char _z, unsigned short groundId);

	/** List holding the creatures in the game
	  * \todo This also contains NPCs, should change the name and rework max_players to work with this */
    std::map<long, Creature*> playersOnline;

	/**
	  * Place Creature on the map.
	  * Adds the Creature to playersOnline and to the map
	  * \param c Creature to add
	  */
    bool placeCreature(Creature* c);

	/**
	  * Remove Creature from the map.
	  * Removes the Creature from playersOnline and from the map
	  * \param c Creature to remove
	  */
		bool removeCreature(Creature* c);

    void thingMove(Creature *player, Thing *thing,
        unsigned short to_x, unsigned short to_y, unsigned char to_z, unsigned char count);
	
		//container/inventory to container/inventory
		void thingMove(Creature *player,
				unsigned char from_cid, unsigned char from_slotid, bool fromInventory,
				unsigned char to_cid, unsigned char to_slotid, bool toInventory,
				unsigned char count);

		//container/inventory to ground
		void thingMove(Creature *player,
				unsigned char from_cid, unsigned char from_slotid, bool fromInventory,
				const Position& toPos, unsigned char count);

		//ground to container/inventory
		void thingMove(Creature *player,
				const Position& fromPos, unsigned char stackPos,
				unsigned char to_cid, unsigned char to_slotid,
				bool isInventory, unsigned char count);
		
		//ground to ground
    void thingMove(Creature *player,
        unsigned short from_x, unsigned short from_y, unsigned char from_z,
        unsigned char stackPos,
        unsigned short to_x, unsigned short to_y, unsigned char to_z, unsigned char count);

	/**
	  * Creature wants to turn.
	  * \param creature Creature pointer
	  * \param dir Direction to turn to
	  */
		void creatureTurn(Creature *creature, Direction dir);

	/**
	  * Creature wants to say something.
	  * \param creature Creature pointer
	  * \param type Type of message
	  * \todo document types
	  * \param text The text to say
	  */
		void creatureSay(Creature *creature, unsigned char type, const std::string &text);

    void creatureWhisper(Creature *creature, const std::string &text);
    void creatureYell(Creature *creature, std::string &text);
    void creatureSpeakTo(Creature *creature, const std::string &receiver, const std::string &text);
    void creatureBroadcastMessage(Creature *creature, const std::string &text);
    void creatureToChannel(Creature *creature, unsigned char type, const std::string &text, unsigned short channelId);
		void creatureMonsterYell(Monster* monster, const std::string& text);
		void creatureChangeOutfit(Creature *creature);

		bool creatureThrowRune(Creature *creature, const Position& centerpos, const MagicEffectClass& me);
		bool creatureCastSpell(Creature *creature, const Position& centerpos, const MagicEffectClass& me);
		bool creatureSaySpell(Creature *creature, const std::string &text);
		bool creatureUseItem(Creature *creature, const Position& pos, Item* item);
    void changeOutfitAfter(unsigned long id, int looktype, long time);
    void changeSpeed(unsigned long id, unsigned short speed);
    void addEvent(long ticks, int type, void *data);
		void addEvent(SchedulerTask*);
   
   
    Creature* getCreatureByID(unsigned long id);
		Creature* getCreatureByName(const char* s);

		std::list<Position> getPathTo(Creature *creature, Position start, Position to, bool creaturesBlock=true);

		/** Lockvar for Game. */
    OTSYS_THREAD_LOCKVAR gameLock;

  protected:
		bool onPrepareMoveThing(Creature *player, const Thing* thing, const Position& fromPos, const Position& toPos);
		bool onPrepareMoveThing(Creature *player, const Thing* thing, const Tile *fromTile, const Tile *toTile);
		bool onPrepareMoveThing(Creature *player, const Item* item, const Container *fromContainer, const Container *toContainer);
		bool onPrepareMoveCreature(Creature *player, const Creature* creatureMoving, const Tile *fromTile, const Tile *toTile);
		bool onPrepareMoveThing(Player *player, const Position& fromPos, const Item *item, slots_t toSlot);
		bool onPrepareMoveThing(Player *player, slots_t fromSlot, slots_t toSlot);

		//container/inventory to container/inventory
		void thingMoveInternal(Creature *player,
				unsigned char from_cid, unsigned char from_slotid, bool fromInventory,
				unsigned char to_cid, unsigned char to_slotid, bool toInventory,
				unsigned char count);

		//container/inventory to ground
		void thingMoveInternal(Creature *player,
				unsigned char from_cid, unsigned char from_slotid, bool fromInventory,
				const Position& toPos, unsigned char count);

		//ground to container/inventory
		void thingMoveInternal(Creature *player,
				const Position& fromPos, unsigned char stackPos,
				unsigned char to_cid, unsigned char to_slotid,
				bool toInventory, unsigned char count);

		// use this internal function to move things around to avoid the need of
    // recursive locks
    void thingMoveInternal(Creature *player,
        unsigned short from_x, unsigned short from_y, unsigned char from_z,
        unsigned char stackPos,
        unsigned short to_x, unsigned short to_y, unsigned char to_z, unsigned char count);

		void changeOutfit(unsigned long id, int looktype);
		bool creatureOnPrepareAttack(Creature *creature, Position pos);
		void creatureMakeDamage(Creature *creature, Creature *attackedCreature, fight_t damagetype);
		void creatureBroadcastTileUpdated(const Position& pos);
		void teleport(Creature *creature, Position newPos);

		bool creatureMakeMagic(Creature *creature, const Position& centerpos, const MagicEffectClass* me);
		bool creatureOnPrepareMagicAttack(Creature *creature, Position pos, const MagicEffectClass* me);
	
	/**
		* Change the players hitpoints
		* Return: the mana damage and the actual hitpoint loss
		*/
		void creatureApplyDamage(Creature *creature, int damage, int &outDamage, int &outManaDamage);

		void CreateDamageUpdate(Creature* player, Creature* attackCreature, int damage, NetworkMessage& msg);
		void getSpectators(const Range& range, std::vector<Creature*>& list);
		void creatureApplyMagicEffect(Creature *target, const MagicEffectClass& me, NetworkMessage& msg);
		void CreateManaDamageUpdate(Creature* player, Creature* attackCreature, int damage, NetworkMessage& msg);

		//void creatureAddDamageAnimation(Player* spectator, const CreatureState& creatureState, NetworkMessage& msg);

		OTSYS_THREAD_LOCKVAR eventLock;
		OTSYS_THREAD_SIGNALVAR eventSignal;

		static OTSYS_THREAD_RETURN eventThread(void *p);

		struct GameEvent
		{
			__int64  tick;
			int      type;
			void*    data;
		};

		//void checkMonsterAttacking(unsigned long id);
		void checkPlayerAttacking(unsigned long id);
		void checkPlayer(unsigned long id);
		void decayItem(Item *item);
		//void decayItem(Position& pos, unsigned short id, unsigned char stackpos);
		void decaySplash(Item* item);

		std::priority_queue<SchedulerTask*, std::vector<SchedulerTask*>, lessSchedTask > eventList;

		uint32_t max_players;

		Map* map;

		friend class Monster;
		friend class GameState;
};


#endif

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
#include <vector>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include "spawn.h"
#include "position.h"
#include "item.h"
#include "container.h"
#include "map.h"
#include "attack.h"

class Creature;   // see creature.h
class Player;
class Commands;
class SchedulerTask;
class lessSchedTask;

#define MAP_WIDTH    512
#define MAP_HEIGHT   512
#define MAP_LAYER     16

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

	const std::string& getSpawnFile() {return map->spawnfile;}

	/**
	  * Get a single tile of the map.
	  * \returns A Pointer to the tile */
	Tile* getTile(unsigned short _x, unsigned short _y, unsigned char _z);

	/**
	  * Set a Tile to a specific ground id
	  * \param groundId ID of the ground to set
	  */
	void setTile(unsigned short _x, unsigned short _y, unsigned char _z, unsigned short groundId);

	/**
	  * Place Creature on the map.
	  * Adds the Creature to playersOnline and to the map
	  * \param c Creature to add
	  */
	bool placeCreature(Position &pos, Creature* c);

	/**
		* Remove Creature from the map.
		* Removes the Creature the map
		* \param c Creature to remove
		*/
	bool removeCreature(Creature* c);

	uint32_t getPlayersOnline() {return (uint32_t)AutoList<Player>::list.size();};
	uint32_t getMonstersOnline() {return (uint32_t)AutoList<Monster>::list.size();};


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
	void creatureSay(Creature *creature, SpeakClasses type, const std::string &text);

  void creatureWhisper(Creature *creature, const std::string &text);
  void creatureYell(Creature *creature, std::string &text);
  void creatureSpeakTo(Creature *creature, const std::string &receiver, const std::string &text);
  void creatureBroadcastMessage(Creature *creature, const std::string &text);
  void creatureToChannel(Creature *creature, SpeakClasses type, const std::string &text, unsigned short channelId);
	void creatureMonsterYell(Monster* monster, const std::string& text);
	void creatureChangeOutfit(Creature *creature);

	//bool creatureThrowRune(Creature *creature, const Position& centerpos, const MagicEffectClass& me);
	//bool creatureCastSpell(Creature *creature, const Position& centerpos, const MagicEffectClass& me);
	bool creatureSaySpell(Creature *creature, const std::string &text);
	
	bool playerUseItemEx(Player *player, const Position& posFrom,const unsigned char  stack_from,
		const Position &posTo,const unsigned char stack_to, const unsigned short itemid);
	bool playerUseItem(Player *player, const Position& pos, const unsigned char stackpos, const unsigned short itemid, const unsigned char index);
	void playerUseBattleWindow(Player *player, Position &posFrom, unsigned char stackpos, unsigned short itemid, unsigned long creatureid);
	bool playerUseMagicItem(Player *player, const Position& posFrom, Item* item);

  void changeOutfitAfter(unsigned long id, int looktype, long time);
  void changeSpeed(unsigned long id, unsigned short speed);

	void addEvent(SchedulerTask*);
	void creatureBroadcastTileUpdated(const Position& pos);
	void teleport(Thing *thing, Position newPos);
      
  std::vector<Player*> BufferedPlayers;   
  void flushSendBuffers();
  void addPlayerBuffer(Player* p);
  
  std::vector<Thing*> ToReleaseThings;   
  void FreeThing(Thing* thing);

  Thing* getThing(const Position &pos,unsigned char stack,Player* player = NULL);
  void addThing(Player* player,const Position &pos,Thing* thing);
  void removeThing(Player* player,const Position &pos,Thing* thing);
	Position getThingMapPos(Player *player, const Position &pos);
  
  void sendAddThing(Player* player,const Position &pos,const Thing* thing);
  void sendRemoveThing(Player* player,const Position &pos,const Thing* thing,const unsigned char stackpos = 1,const bool autoclose = false);
  void sendUpdateThing(Player* player,const Position &pos,const Thing* thing,const unsigned char stackpos = 1);
   
	Player* getPlayerByID(unsigned long id);
	Creature* getCreatureByID(unsigned long id);
	Creature* getCreatureByName(const char* s);

	std::list<Position> getPathTo(Creature *creature, Position start, Position to, bool creaturesBlock=true);	
	bool canThrowTo(Position from, Position to, bool creaturesBlock /* = true*/, bool isProjectile = false);

	//void creatureAttackCreature(Creature *attacker, Creature *attackedCreature, attacktype_t attackType, amuEffect_t ammunition, int damage);
	//void internalCreatureAttackCreature(Creature *attacker, Creature *attackedCreature, attacktype_t attackType, amuEffect_t ammunition, int damage);
	//void internalCreatureAttackedCreature(Creature *attacker, Creature *attackedCreature);
	void executeAttack(Attack *attack, Creature *attackedCreature);
	void addSplash(const Position &pos,unsigned short type, eBloodColor color);

	void addAnimationShoot(Creature *attacker, const Position& posTo, unsigned char distanceEffect);
	void addMagicEffect(const Position &pos, unsigned char type);

	void creatureChangeLight(Player* player, int time, unsigned char lightlevel);
	void addCondition(Creature *creature, conditiontype_t conditionType, int time, int n);
	/** Lockvar for Game. */
  OTSYS_THREAD_LOCKVAR gameLock;    

protected:
	bool onPrepareMoveThing(Creature *player, const Thing* thing, const Position& fromPos, const Position& toPos);
	bool onPrepareMoveThing(Creature *player, const Thing* thing, const Tile *fromTile, const Tile *toTile);
	bool onPrepareMoveThing(Creature *player, const Item* fromItem, const Container *fromContainer, const Container *toContainer, const Item* toItem);
	bool onPrepareMoveCreature(Creature *player, const Creature* creatureMoving, const Tile *fromTile, const Tile *toTile);
	bool onPrepareMoveThing(Player *player, const Position& fromPos, const Item *item, slots_t toSlot);
	bool onPrepareMoveThing(Player *player, slots_t fromSlot, const Item *fromItem, slots_t toSlot, const Item *toItem);
	bool onPrepareMoveThing(Player *player, const Container *fromContainer, const Item *fromItem, slots_t toSlot, const Item *toItem);
	bool onPrepareMoveThing(Player *player, const Item *item, slots_t toSlot);

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
	void getSpectators(const Range& range, std::vector<Creature*>& list);

	OTSYS_THREAD_LOCKVAR eventLock;
	OTSYS_THREAD_SIGNALVAR eventSignal;

	static OTSYS_THREAD_RETURN eventThread(void *p);

	struct GameEvent
	{
		__int64  tick;
		int      type;
		void*    data;
	};

	void checkCreatureAttacking(unsigned long id);
	void checkCreature(unsigned long id);
	void checkDecay(int t);
	
	#define DECAY_INTERVAL  10000
	void startDecay(Item* item);
	struct decayBlock{
		long decayTime;
		std::vector<Item*> decayItems;
	};
	std::vector<decayBlock*> decayVector;
	unsigned char lightlevel;
	int lightdelta;
	void checkLight(int delta);
	
	void checkSpawns(int t);
	std::priority_queue<SchedulerTask*, std::vector<SchedulerTask*>, lessSchedTask > eventList;

	uint32_t max_players;

	Map* map;
	
	std::vector<std::string> commandTags;
	void addCommandTag(std::string tag);
	void resetCommandTag();

	friend class Commands;
	friend class Monster;
	friend class Spawn;
	friend class SpawnManager;
	friend class BaseScript;
	//friend class ActionScript;
};

template<class ArgType>
class TCallList : public SchedulerTask {
		  public:
					 TCallList(boost::function<int(Game*, ArgType)> f1, boost::function<bool(Game*)> f2, std::list<ArgType>& call_list, __int64 interval) :
								_f1(f1), _f2(f2), _list(call_list), _interval(interval) {
								}
					 void operator()(Game* arg) {
								if (!_f2(arg)) {
										  int ret = _f1(arg, _list.front());
										  _list.pop_front();
										  if (!_list.empty()) {
													 SchedulerTask* newTask = new TCallList(_f1, _f2, _list, _interval);
													 newTask->setTicks(_interval);
													 arg->addEvent(newTask);
										  }
								}
								return;
					 }

		  private:
		  		boost::function<int(Game*, ArgType)> _f1;
				boost::function<bool(Game*)>_f2;
				std::list<ArgType> _list;
				__int64 _interval;
};

// from scheduler.h
// needed here for proper initialisation order forced by gcc 3.4.2+
//template<class Functor, class Functor2,  class Arg>
//class TCallList : public SchedulerTask {
//		  public:
//					 TCallList(Functor f, Functor2 f2, std::list<Arg>& call_list, __int64 interval) : _f(f), _f2(f2), _list(call_list), _interval(interval) {
//					 }
//
//					 result_type operator()(const argument_type& arg) {
//                              if(!_f2(arg)){   
//								result_type ret = _f(arg, _list.front());
//								_list.pop_front();
//								if (!_list.empty()) {
//										  SchedulerTask* newtask = new TCallList<Functor, Functor2, Arg>(_f, _f2, _list, _interval);
//										  newtask->setTicks(_interval);
//										  arg->addEvent(newtask);
//								}
//								return ret;
//                          }	
//										return result_type();
//					 }
//		  protected:
//					 Functor _f;
//					 Functor2 _f2;
//					 std::list<Arg> _list;
//					 __int64 _interval;
//};
//

template<class ArgType>
SchedulerTask* makeTask(__int64 ticks, boost::function<int(Game*, ArgType)> f1, std::list<ArgType>& call_list, __int64 interval, boost::function<bool(Game*)> f2) {
		  TCallList<ArgType> *t = new TCallList<ArgType>(f1, f2, call_list, interval);
		  t->setTicks(ticks);
		  return t;
}

#endif

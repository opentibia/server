//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// the map of OpenTibia
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


#ifndef __OTSERV_MAP_H
#define __OTSERV_MAP_H


#include <queue>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include "position.h"
#include "item.h"
#include "creature.h"
#include "magic.h"
#include "otsystem.h"

#include "scheduler.h"
#include "networkmessage.h"


class Creature;   // see creature.h
class Player;



#define MAP_WIDTH    512
#define MAP_HEIGHT   512
#define MAP_LAYER     16


class Tile;

class Range {
public:
	Range(Position centerpos, bool multilevel = false) {
		this->startx = centerpos.x - 9;
		this->endx = centerpos.x + 9;
		this->starty = centerpos.y - 7;
		this->endy = centerpos.y + 7;
		if(multilevel)
			this->startz = 0;
		else
			this->startz = centerpos.z;
		this->endz = centerpos.z;
		this->multilevel = multilevel;
	}

	Range(int startx, int endx, int starty, int endy, int z, bool multilevel = false)
	{
		this->startx = startx;
		this->endx = endx;
		this->starty = starty;
		this->endy = endy;
		if(multilevel)
			this->startz = 0;
		else
			this->startz = startz;
		this->endz = z;
		this->multilevel = multilevel;
	}

	Range(int startx, int endx, int starty, int endy, int startz, int endz)
	{
		this->startx = startx;
		this->endx = endx;
		this->starty = starty;
		this->endy = endy;
		this->startz = startz;
		this->endz = endz;
		this->multilevel = multilevel;
	}

	int startx;
	int endx;
	int starty;
	int endy;
	int startz;
	int endz;
	bool multilevel;
};

struct targetdata {
	int damage;
	int manaDamage;
	unsigned char stackpos;
	bool hadSplash;
};

struct tiletargetdata {
	Position pos;
	int targetCount;
	unsigned char thingCount;
};

struct AStarNode{
	int x,y;
	AStarNode* parent;
	float f, g, h;
	bool operator<(const AStarNode &node){return this->h < node.h;}
};

template<class T> class lessPointer : public std::binary_function<T*, T*, bool> {
		  public:
		  bool operator()(T*& t1, T*& t2) {
				return *t1 < *t2;
		  }
};

class Map {
  public:
    Map();
    ~Map();
		
    bool LoadMap(std::string filename);

    Tile* getTile(unsigned short _x, unsigned short _y, unsigned char _z);

    std::map<long, Creature*> playersOnline;

    bool placeCreature(Creature* c);
    bool removeCreature(Creature* c);

    void thingMove(Creature *player, Thing *thing,
        unsigned short to_x, unsigned short to_y, unsigned char to_z);

    void thingMove(Creature *player,
        unsigned short from_x, unsigned short from_y, unsigned char from_z,
        unsigned char stackPos,
        unsigned short to_x, unsigned short to_y, unsigned char to_z);

		void creatureTurn(Creature *creature, Direction dir);

    void creatureSay(Creature *creature, unsigned char type, const std::string &text);
    void creatureWhisper(Creature *creature, const std::string &text);
    void creatureYell(Creature *creature, std::string &text);
    void creatureSpeakTo(Creature *creature, const std::string &receiver, const std::string &text);
    void creatureBroadcastMessage(Creature *creature, const std::string &text);
    void Map::creatureToChannel(Creature *creature, unsigned char type, const std::string &text, unsigned short channelId);
	  void creatureChangeOutfit(Creature *creature);

	  bool creatureThrowRune(Creature *creature, const MagicEffectClass& me);
  	void creatureCastSpell(Creature *creature, const MagicEffectClass& me);
		bool creatureSaySpell(Creature *creature, const std::string &text);
     void changeOutfitAfter(unsigned long id, int looktype, long time);
     void changeSpeed(unsigned long id, unsigned short speed);
    //void addEvent(long ticks, int type, void *data);
	  void addEvent(SchedulerTask*);
   
   
    Creature* getCreatureByID(unsigned long id);
		Creature* getCreatureByName(const char* s);

	std::list<Position> getPathTo(Position start, Position to, bool creaturesBlock=true);
	bool canThrowItemTo(Position from, Position to, bool creaturesBlock=true);
	//bool canThrowItemTo(int x0, int y0, int x1, int y1, int floor);

    OTSYS_THREAD_LOCKVAR mapLock;
  protected:
    // use this internal function to move things around to avoid the need of
    // recursive locks
		//bool OnPrepareMoveInternal(const Thing *thing);
    void thingMoveInternal(Creature *player,
        unsigned short from_x, unsigned short from_y, unsigned char from_z,
        unsigned char stackPos,
        unsigned short to_x, unsigned short to_y, unsigned char to_z);

    void changeOutfit(unsigned long id, int looktype);
		bool creatureOnPrepareAttack(Creature *creature, Position pos);
		void creatureMakeDamage(Creature *creature, Creature *attackedCreature, fight_t damagetype);
    void creatureBroadcastTileUpdated(const Position& pos);
		bool creatureMakeMagic(Creature *creature, const MagicEffectClass* me);

		void CreateDamageUpdate(Creature* player, Creature* attackCreature, int damage, NetworkMessage& msg);
	  void getSpectators(const Range& range, std::vector<Creature*>& list);
	  void getAreaTiles(Position pos, const unsigned char area[14][18], unsigned char dir, std::list<tiletargetdata>& list);
		void creatureApplyMagicEffect(Creature *target, const MagicEffectClass& me, NetworkMessage& msg);
	  void CreateManaDamageUpdate(Creature* player, Creature* attackCreature, int damage, NetworkMessage& msg);

    OTSYS_THREAD_LOCKVAR eventLock;
	  OTSYS_THREAD_SIGNALVAR eventSignal;

    static OTSYS_THREAD_RETURN eventThread(void *p);

    struct MapEvent
    {
      __int64  tick;
      int      type;
      void*    data;
    };

    void checkPlayerAttacking(unsigned long id);
    void checkPlayer(unsigned long id);
    void decayItem(Item* item);
    void decaySplash(Item* item);

	 std::priority_queue<SchedulerTask*, std::vector<SchedulerTask*>, lessSchedTask > eventList;
    //list<MapEvent> *eventLists[12000];

    int loadMapXml(const char *filename);


    typedef std::map<unsigned long, Tile*> TileMap;
    TileMap tileMaps[64][64][MAP_LAYER];

    void Map::setTile(unsigned short _x, unsigned short _y, unsigned char _z, unsigned short groundId);

	 uint32_t max_players;
};

#endif

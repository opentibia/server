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

#include "position.h"
#include "item.h"
#include "creature.h"
#include "magic.h"
#include "otsystem.h"

#include "scheduler.h"
#include "networkmessage.h"

#include "tools.h"

class Creature;   // see creature.h
class Player;
class Game;


#define MAP_WIDTH    512
#define MAP_HEIGHT   512

#define MAP_LAYER     16


class Tile;

struct targetdata {
	int damage;
	int manaDamage;
	bool physical;
};

class TilePreChangeData {
public:
	Position pos;
	unsigned char thingCount;
};

struct tilechangedata {
	Thing *thing;
	int stackpos;
	bool remove;
};

typedef std::pair<Position, bool> targetAreaItem;
typedef std::vector< targetAreaItem > TargetAreaVec;

typedef std::pair<Creature*, struct targetdata> targetitem;
typedef std::vector< targetitem > TargetDataVec;

typedef std::vector<TilePreChangeData> TileExDataVec;
typedef std::vector<struct tilechangedata> TileChangeDataVec;

typedef std::map<Tile*, TilePreChangeData > TileExDataMap;
typedef std::map<Tile*, TileChangeDataVec > TileChangeDataVecMap;


class Map;

class MapState {
public:
	MapState(Map* imap);
	void addThing(Tile *t, Thing *thing);
	bool removeThing(Tile *t, Thing *thing);
	void refreshThing(Tile *t, Thing *thing);

	void getMapChanges(Player *spectator, NetworkMessage &msg);
	
protected:
	Map* map;

	void addThingInternal(Tile *t, Thing *thing, bool onlyRegister);
	bool removeThingInternal(Tile *t, Thing *thing, bool onlyRegister);

	void addTile(Tile *t, Position& tilepos);
	bool isTileStored(const Tile *t) const;
	
	TileChangeDataVecMap changesItemMap;
	TileExDataMap preChangeItemMap;
};

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
			this->startz = z;
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

/**
  * A Node inside the A*-Algorithm
  */
struct AStarNode{
	/** Current position */
	int x,y;
	/** Parent of this node. Null if this is the rootnode */
	AStarNode* parent;
	/** Heuristics variable */
	float f, g, h;
	/** Operator to sort so we can find the best node */
	bool operator<(const AStarNode &node){return this->h < node.h;}
};


template<class T> class lessPointer : public std::binary_function<T*, T*, bool> {
		  public:
		  bool operator()(T*& t1, T*& t2) {
				return *t1 < *t2;
		  }
};

/**
  * Map class.
  * Holds all the actual map-data
  */
class Map {
  public:
    Map();
    ~Map();

	/** Lock the map */
	void lock(){OTSYS_THREAD_LOCK(mapLock);};

	/** Unlock the map */
	void unlock(){OTSYS_THREAD_UNLOCK(mapLock);};

	/**
	  * Load a map.
	  * \param filename Mapfile to load
	  * \returns Bool if load was successful.
	  */
    bool loadMap(std::string filename);

	/**
	  * Get a single tile.
	  * \returns A pointer to that tile.
	  */
    Tile* getTile(unsigned short _x, unsigned short _y, unsigned char _z);

	/**
	  * Place a creature on the map
	  * \param c Creature pointer to the creature to place
	  * \returns The postition the Creature was actually added at
	  */
    Position placeCreature(Creature* c);
	
	/**
	  * Remove a creature from the map.
	  * \param c Creature pointer to the creature to remove
	  */
    bool removeCreature(Creature* c);

	/**
	 * Checks if you can throw an object to that position
	 *	\param from from Source point
	 *	\param to Destination point
	 *	\param creaturesBlock Wether a Creature is an obstacle or not
	 *	\param isProjectile Takes into consideration for windows/door-ways.
	 *	\returns The result if you can throw there or not
	 */
		bool canThrowItemTo(Position from, Position to, bool creaturesBlock /* = true*/, bool isProjectile = false);

	/**
	  * Get the path to a specific position on the map.
	  * \param start The start position of the path
	  * \param to The destination position
	  * \param creaturesBlock Wether a Creature is an obstacle or not
	  * \returns A list of all positions you have to traverse to reacg the destination
	  */
	std::list<Position> getPathTo(Position start, Position to, bool creaturesBlock=true);

	/** The Map-Lock */
	OTSYS_THREAD_LOCKVAR mapLock;
  protected:
    /**
	  * Get the Creatures within a specific Range */
	void getSpectators(const Range& range, std::vector<Creature*>& list);

  typedef std::map<unsigned long, Tile*> TileMap;

	TileMap tileMaps[64][64][MAP_LAYER];

    void Map::setTile(unsigned short _x, unsigned short _y, unsigned char _z, unsigned short groundId);
	
	friend class MapState;
	friend class Game;
	//FIXME friend for derived classes?
	friend class IOMapXML;
	friend class IOMap;
};

#endif

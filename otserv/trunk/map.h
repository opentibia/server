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

class TilePreChangeData {
public:
	Position pos;
	bool isBlocking;
	unsigned char thingCount;
};

struct tilechangedata {
	Position pos;
	Thing *thing;
	int stackpos;
	bool remove;
};

typedef std::vector<TilePreChangeData> TileExDataVec;
typedef std::vector<struct tilechangedata> TileChangeDataVec;

typedef std::map<Tile*, TilePreChangeData > TileExDataMap;
typedef std::map<Tile*, TileChangeDataVec > TileChangeDataVecMap;

class Tile;
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
		setRange(centerpos, multilevel);
	}
	
	//Creates a union of 2 positions
	Range(const Position& pos1, const Position& pos2)
	{
		/*
		Range r1(pos1, true);
		Range r2(pos2, true);
		*/

		//*
		Position topleft(std::min(pos1.x, pos2.x), std::min(pos1.y, pos2.y), pos1.z);
		Position bottomright(std::max(pos1.x, pos2.x), std::max(pos1.y, pos2.y), pos1.z);

		setRange(topleft, true);

		minRange.x = -9;
		minRange.y = -7;
		maxRange.x = std::max(topleft.x + 9, bottomright.x + 9) - topleft.x; //(bottomright.x - topleft.x);
		maxRange.y = std::max(topleft.y + 7, bottomright.y + 7) - topleft.y; //(bottomright.y - topleft.y);
		//*/
	}

	Range(Position centerpos, int minRangeX, int maxRangeX, int minRangeY, int maxRangeY)
	{
		setRange(centerpos, true);

		minRange.x = -minRangeX;
		minRange.y = -minRangeY;

		maxRange.x = maxRangeX;
		maxRange.y = maxRangeY;
	}

	Position centerpos;
	Position minRange;
	Position maxRange;

	int zstep;	
	bool multilevel;

private:
	bool isUnderground() const {
		return (centerpos.z > 7);
	}

	void setRange(Position pos, bool multilevel = false)
	{
		centerpos = pos;
		
		minRange.x = -8;
		minRange.y = -6;

		maxRange.x = 9; //maxRange.x = 8;
		maxRange.y = 7; //maxRange.y = 6;
		
		zstep = 1;

		if(multilevel) {
			if(isUnderground()) {
				//8->15
				minRange.z = std::min(centerpos.z + 2, MAP_LAYER - 1);
				maxRange.z = std::max(centerpos.z - 2, 8);

				zstep = -1;
			} else {
				minRange.z = 7;
				maxRange.z = 0;

				zstep = -1;
			}
		}
		else {
			minRange.z = centerpos.z;
			maxRange.z = centerpos.z;
		}
	}
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

	//bool removeItem(Tile t, Item);

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

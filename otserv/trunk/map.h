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

//#include "scheduler.h"
#include "tools.h"
#include "tile.h"

class Creature;   // see creature.h
class Player;
class Game;

#ifdef _M1K_
 #define MAP_WIDTH    1024
 #define MAP_HEIGHT   1024
#elif _M2K_
 #define MAP_WIDTH    2048
 #define MAP_HEIGHT   2048
#elif _M4K_
 #define MAP_WIDTH    4096
 #define MAP_HEIGHT   4096
#else       
 #define MAP_WIDTH    512
 #define MAP_HEIGHT   512
#endif   

#define MAP_LAYER     16

class Tile;
class Map;

class Range {
public:
	
	Range(Position centerpos, bool multilevel = false) {
		setRange(centerpos, multilevel);
	}
	
	//Creates a union of 2 positions
	//Should only be used when a player makes a move.
	Range(const Position& pos1, const Position& pos2)
	{
		Position topleft(std::min(pos1.x, pos2.x), std::min(pos1.y, pos2.y), pos1.z);
		Position bottomright(std::max(pos1.x, pos2.x), std::max(pos1.y, pos2.y), pos1.z);

		setRange(topleft, true);

		minRange.x = -9;
		minRange.y = -7;
		maxRange.x = std::max(topleft.x + 9, bottomright.x + 9) - topleft.x;
		maxRange.y = std::max(topleft.y + 7, bottomright.y + 7) - topleft.y;
	}

	Range(Position centerpos, int minRangeX, int maxRangeX, int minRangeY, int maxRangeY, bool multilevel = true)
	{
		setRange(centerpos, multilevel);

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
		
		//This is the maximum view that the viewer AND the viewers that is seeing the viewer :o
		minRange.x = -9; //minRange.x = -8;
		minRange.y = -7; //minRange.y = -6;
		
    //just the visible ones
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
	//float f, g, h;
	int h;
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
	//void lock(){OTSYS_THREAD_LOCK(mapLock);};

	/** Unlock the map */
	//void unlock(){OTSYS_THREAD_UNLOCK(mapLock);};

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
    Tile* getTile(const Position &pos);
    
	/**
	  * Set a single tile.
	  * \param groundId Ground kind (ID)
	  * \returns Nothing =]
	  */
    void setTile(unsigned short _x, unsigned short _y, unsigned char _z, unsigned short groundId);

	/**
	  * Place a creature on the map
	  * \param pos The position to place the creature
	  * \param c Creature pointer to the creature to place
	  */
    bool placeCreature(Position &pos, Creature* c);
	
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
		bool canThrowItemTo(Position from, Position to, bool creaturesBlock  = true, bool isProjectile = false);

		bool isPathValid(Creature *creature, const std::list<Position>& path, bool ignoreMoveableBlockingItems = false);

	/**
	  * Get the path to a specific position on the map.
	  * \param creature The creature that wants a route
	  * \param start The start position of the path
	  * \param to The destination position
	  * \param creaturesBlock Wether a Creature is an obstacle or not
	  * \returns A list of all positions you have to traverse to reacg the destination
	  */
	std::list<Position> getPathTo(Creature* creature, Position start, Position to, bool creaturesBlock = true, bool ignoreMoveableBlockingItems = false);

	/** The Map-Lock */
	//OTSYS_THREAD_LOCKVAR mapLock;
	
	/* Map Width and Height - for Info purposes */
	int mapwidth, mapheight;
	
  protected:    
    /**
	  * Get the Creatures within a specific Range */
	void getSpectators(const Range& range, std::vector<Creature*>& list);
    
    typedef std::map<unsigned long, Tile*> TileMap;
	//TileMap tileMaps[32][32][MAP_LAYER];	
	//TileMap tileMaps[256][256];
	TileMap tileMaps[128][128];

  // setTile moved into public

	friend class MapState;
	friend class Game;
	//FIXME friend for derived classes?
	friend class IOMapXML;
	friend class IOMap;

private:
	std::string spawnfile;

};
#endif

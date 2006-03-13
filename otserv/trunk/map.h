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


#ifndef __MAP_H__
#define __MAP_H__

#include <queue>
#include <bitset>
#include <map>

#include "definitions.h"
#include "position.h"
#include "item.h"
#include "creature.h"
#include "magic.h"
#include "iomapserialize.h"

#include "tools.h"
#include "tile.h"

class Creature;   // see creature.h
class Player;
class Game;

#define MAP_MAX_LAYERS 16

class Tile;
class Map;
class IOMap;

class Range{
public:
	Range(Position centerpos, bool multilevel = false){
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
	bool isUnderground() const{
		return (centerpos.z > 7);
	}

	void setRange(Position pos, bool multilevel = false)
	{
		centerpos = pos;
		
		//This is the maximum view that the viewer AND the viewers that is seeing the viewer :o
		minRange.x = -9;
		minRange.y = -7;
		
		maxRange.x = 9;
		maxRange.y = 7;
		
		zstep = 1;

		if(multilevel){
			if(isUnderground()){
				//8->15
				minRange.z = std::min(centerpos.z + 2, MAP_MAX_LAYERS - 1);
				maxRange.z = std::max(centerpos.z - 2, 0);

				//minRange.z = std::min(centerpos.z + 2, MAP_MAX_LAYERS - 1);
				//maxRange.z = std::max(centerpos.z - 2, 8);

				zstep = -1;
			}
			else{
				minRange.z = 7;
				maxRange.z = 0;

				if(centerpos.z == 7)
					minRange.z = 9;
				else if(centerpos.z == 6)
					minRange.z = 8;

				zstep = -1;
			}
		}
		else{
			minRange.z = centerpos.z;
			maxRange.z = centerpos.z;
		}
	}
};

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

#define MAX_NODES 512
#define GET_NODE_INDEX(a) (a - &nodes[0])

class AStarNodes{
public:
	AStarNodes();
	~AStarNodes(){};

	AStarNode* createOpenNode();
	AStarNode* getBestNode();
	void closeNode(AStarNode* node);
	unsigned long countClosedNodes();
	unsigned long countOpenNodes();
	bool isInList(unsigned long x, unsigned long y);
private:
	AStarNode nodes[MAX_NODES];
	std::bitset<MAX_NODES> openNodes;
	unsigned long curNode;
};

template<class T> class lessPointer : public std::binary_function<T*, T*, bool>
{
public:
	bool operator()(T*& t1, T*& t2) {
		return *t1 < *t2;
	}
};

typedef std::list<Creature*> SpectatorVec;
typedef std::list<Player*> PlayerList;

/**
  * Map class.
  * Holds all the actual map-data
  */

class Map
{
public:
	Map();
	~Map();
    
	/**
	* Load a map.
	* \param identifier file/database to load
	* \param type map type "OTBM", "XML"
	* \returns true if the map was loaded successfully
	*/
	bool loadMap(const std::string& identifier, const std::string& type);

	/**
	* Save a map.
	* \param identifier file/database to save to
	* \returns true if the map was saved successfully
	*/
	bool saveMap(const std::string& identifier);

	/**
	* Get a single tile.
	* \returns A pointer to that tile.
	*/
	Tile* getTile(uint16_t _x, uint16_t _y, uint8_t _z);
	Tile* getTile(const Position &pos);
    
	/**
	* Set a single tile.
	* \param a tile to set for the 
	*/
	void setTile(uint16_t _x, uint16_t _y, uint8_t _z, Tile* newtile);

	/**
	* Place a creature on the map
	* \param pos The position to place the creature
  * \param creature Creature to place on the map
  * \param forceLogin If true, placing the creature will not fail becase of obstacles (creatures/chests)
	*/
	bool placeCreature(const Position &pos, Creature* creature, bool forceLogin = false);
	
	/**
	* Remove a creature from the map.
	* \param c Creature pointer to the creature to remove
	*/
	bool removeCreature(Creature* c);

	/**
	* Checks if you can throw an object to that position
	*	\param fromPos from Source point
	*	\param toPos Destination point
	*	\returns The result if you can throw there or not
	*/
	bool canThrowObjectTo(const Position& fromPos, const Position& toPos);

	bool isPathValid(Creature* creature, const std::list<Position>& path, int pathSize = -1);

	/**
	* Get the path to a specific position on the map.
	* \param creature The creature that wants a route
	* \param start The start position of the path
	* \param to The destination position
	* \returns A list of all positions you have to traverse to reach the destination
	*/
	std::list<Position> getPathTo(Creature* creature, Position start, Position to, int maxNodeSize = 100);

	/* Map Width and Height - for Info purposes */
	int mapwidth, mapheight;
	
	MapError_t getLastError() {return lasterrortype;}
	int getErrorCode() {return lasterrorcode;}

	void setLastError(MapError_t errtype, unsigned long _code = 0)
	{
		lasterrorcode = _code;
		lasterrortype = errtype;
	}

protected:
	bool defaultMapLoaded;
	MapError_t lasterrortype;
	unsigned long lasterrorcode;
	std::string spawnfile;
	std::string housefile;
	std::string mapStoreIdentifier;
	std::string houseStoreIdentifier;

	/**
	* Get the Creatures within a specific Range */
	void getSpectators(const Range& range, SpectatorVec& list);
    
	typedef std::map<unsigned long, Tile*> TileMap;
	TileMap tileMaps[128][128];

	friend class Game;

	friend class IOMapOTBM;
	friend class IOMapXML;
	friend class IOMap;
	friend class IOMapSerializeXML;

#ifdef __USE_MYSQL__
	friend class IOMapSerializeSQL;
#endif
};

#endif

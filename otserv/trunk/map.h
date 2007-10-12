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

#ifndef __OTSERV_MAP_H__
#define __OTSERV_MAP_H__

#include <queue>
#include <bitset>
#include <map>

#include "definitions.h"
#include "position.h"
#include "item.h"
#include "creature.h"
#include "iomapserialize.h"
#include "fileloader.h"

#include "tools.h"
#include "tile.h"

class Creature;
class Player;
class Game;

#define MAP_MAX_LAYERS 16

class Tile;
class Map;
class IOMap;

struct AStarNode{
	int32_t x, y;
	AStarNode* parent;
	int32_t f, g, h;
};

#define MAX_NODES 512
#define GET_NODE_INDEX(a) (a - &nodes[0])

#define MAP_NORMALWALKCOST 10
#define MAP_DIAGONALWALKCOST 25

class AStarNodes{
public:
	AStarNodes();
	~AStarNodes(){};

	AStarNode* createOpenNode();
	AStarNode* getBestNode();
	void closeNode(AStarNode* node);
	void openNode(AStarNode* node);
	uint32_t countClosedNodes();
	uint32_t countOpenNodes();
	bool isInList(int32_t x, int32_t y);
	AStarNode* getNodeInList(int32_t x, int32_t y);

	int getMapWalkCost(const Creature* creature, AStarNode* node, const Tile* neighbourTile);
	int getEstimatedDistance(int32_t x, int32_t y, int32_t xGoal, int32_t yGoal);

private:
	AStarNode nodes[MAX_NODES];
	std::bitset<MAX_NODES> openNodes;
	uint32_t curNode;
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

#define FLOOR_BITS 3
#define FLOOR_SIZE (1 << FLOOR_BITS)
#define FLOOR_MASK (FLOOR_SIZE - 1)

struct Floor{
	Floor();
	Tile* tiles[FLOOR_SIZE][FLOOR_SIZE];
};

class QTreeLeafNode;

class QTreeNode{
public:
	QTreeNode();
	virtual ~QTreeNode();

	bool isLeaf(){return m_isLeaf;}
	QTreeLeafNode* getLeaf(uint32_t x, uint32_t y);
	static QTreeLeafNode* getLeafStatic(QTreeNode* root, uint32_t x, uint32_t y);
	QTreeLeafNode* createLeaf(uint32_t x, uint32_t y, uint32_t level);
	
protected:
	bool m_isLeaf;
	QTreeNode* m_child[4];

	friend class Map;
};


class QTreeLeafNode : public QTreeNode{
public:
	QTreeLeafNode();
	virtual ~QTreeLeafNode();

	Floor* createFloor(uint32_t z);
	Floor* getFloor(uint32_t z){return m_array[z];}

	QTreeLeafNode* stepSouth(){return m_leafS;}
	QTreeLeafNode* stepEast(){return m_leafE;}

protected:
	static bool newLeaf;
	QTreeLeafNode* m_leafS;
	QTreeLeafNode* m_leafE;
	Floor* m_array[MAP_MAX_LAYERS];

	friend class Map;
	friend class QTreeNode;
};



/**
  * Map class.
  * Holds all the actual map-data
  */

class Map
{
public:
	Map();
	~Map();

	static int32_t maxViewportX;
	static int32_t maxViewportY;

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
	Tile* getTile(uint16_t x, uint16_t y, uint8_t z);
	Tile* getTile(const Position& pos);

	QTreeLeafNode* getLeaf(uint16_t x, uint16_t y){ return root.getLeaf(x, y);}

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
	bool placeCreature(const Position& centerPos, Creature* creature, bool forceLogin = false);
	
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

	/**
	* Checks if view is clear from fromPos to toPos
	*	\param fromPos from Source point
	*	\param toPos Destination point
	*	\param floorCheck if true then view is not clear if fromPos.z is not the same as toPos.z
	*	\returns The result if there is no obstacles
	*/
	bool isViewClear(const Position& fromPos, const Position& toPos, bool floorCheck);

	/**
	* Get the path to a specific position on the map.
	* \param creature The creature that wants a route
	* \param start The start position of the path
	* \param to The destination position
	* \returns A list of all positions you have to traverse to reach the destination
	*/
	bool getPathTo(const Creature* creature, Position toPosition, std::list<Direction>& listDir);
	bool isPathValid(const Creature* creature, const std::list<Direction>& listDir, const Position& destPos);

	/* Map Width and Height - for Info purposes */
	uint32_t mapWidth, mapHeight;

	MapError_t getLastError() {return lasterrortype;}
	int getErrorCode() {return lasterrorcode;}

	void setLastError(MapError_t errtype, NODE _code = 0)
	{
		if(_code){
			lasterrorcode = _code->start;
		}
		else{
			lasterrorcode = 0;
		}
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

	void getSpectators(SpectatorVec& list, const Position& centerPos, bool multifloor = false,
		int32_t minRangeX = 0, int32_t maxRangeX = 0,
		int32_t minRangeY = 0, int32_t maxRangeY = 0);

	QTreeNode root;

	friend class Game;

	friend class IOMapOTBM;
	friend class IOMapXML;
	friend class IOMap;
	friend class IOMapSerializeXML;

#if defined USE_SQL_ENGINE
	friend class IOMapSerializeSQL;
#endif
};

#endif

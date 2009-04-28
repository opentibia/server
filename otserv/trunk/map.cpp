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
#include "otpch.h"

#include "definitions.h"

#include <string>
#include <sstream>
#include <map>
#include <algorithm>

#include <boost/config.hpp>
#include <boost/bind.hpp>

#include "iomap.h"

#include "iomapxml.h"
#include "iomapotbm.h"
#include "iomapserialize.h"

#include <stdio.h>
#include <iomanip>

#include "items.h"
#include "map.h"
#include "tile.h"
#include "combat.h"
#include "creature.h"

#include "player.h"
#include "configmanager.h"

extern ConfigManager g_config;

Map::Map()
{
	mapWidth = 0;
	mapHeight = 0;
}

Map::~Map()
{
	//
}

bool Map::loadMap(const std::string& identifier, const std::string& type)
{
	IOMap* loader;

	if(type == "XML"){
		loader = new IOMapXML();
	}
	else if(type == "OTBM"){
		loader = new IOMapOTBM();
	}
	else{
		std::cout << "FATAL: Could not determine the map format!" << std::endl;
		return false;
	}

	std::cout << ":: Loading map from: " << identifier << " " << loader->getSourceDescription() << std::endl;

	if(!loader->loadMap(this, identifier)){
		std::cout << "FATAL: [OTBM loader] " << loader->getLastErrorString() << std::endl;
		return false;
	}

	if(!loader->loadSpawns(this)){
		std::cout << "WARNING: could not load spawn data." << std::endl;
	}

	if(!loader->loadHouses(this)){
		std::cout << "WARNING: could not load house data." << std::endl;
	}

	delete loader;

	IOMapSerialize* IOMapSerialize = IOMapSerialize::getInstance();
	IOMapSerialize->loadHouseInfo(this);
	IOMapSerialize->loadMap(this);

	return true;
}


bool Map::saveMap()
{
	IOMapSerialize* IOMapSerialize = IOMapSerialize::getInstance();
	bool saved = false;
	for(uint32_t tries = 0; tries < 3; tries++){
		if(IOMapSerialize->saveMap(this)){
			saved = true;
			break;
		}
	}

	if(!saved)
		return false;

	saved = false;
	for(uint32_t tries = 0; tries < 3; tries++){
		if(IOMapSerialize->saveHouseInfo(this)){
			saved = true;
			break;
		}
	}
	return saved;
}

Tile* Map::getTile(uint16_t x, uint16_t y, uint16_t z)
{
	if(z < MAP_MAX_LAYERS){
		//QTreeLeafNode* leaf = getLeaf(x, y);
		QTreeLeafNode* leaf = QTreeNode::getLeafStatic(&root, x, y);
		if(leaf){
			Floor* floor = leaf->getFloor(z);
			if(floor){
				return floor->tiles[x & FLOOR_MASK][y & FLOOR_MASK];
			}
			else{
				return NULL;
			}
		}
		else{
			return NULL;
		}
	}
	else{
		return NULL;
	}
}

Tile* Map::getTile(const Position& pos)
{
	return getTile(pos.x, pos.y, pos.z);
}

void Map::setTile(uint16_t x, uint16_t y, uint16_t z, Tile* newtile)
{
	if(z >= MAP_MAX_LAYERS) {
		std::cout << "ERROR: Attempt to set tile on invalid Z coordinate " << int(z) << "!" << std::endl;
		return;
	}

	QTreeLeafNode::newLeaf = false;
	QTreeLeafNode* leaf = root.createLeaf(x, y, 15);
	if(QTreeLeafNode::newLeaf){
		//update north
		QTreeLeafNode* northLeaf = root.getLeaf(x, y - FLOOR_SIZE);
		if(northLeaf){
			northLeaf->m_leafS = leaf;
		}

		//update west leaf
		QTreeLeafNode* westLeaf = root.getLeaf(x - FLOOR_SIZE, y);
		if(westLeaf){
			westLeaf->m_leafE = leaf;
		}

		//update south
		QTreeLeafNode* southLeaf = root.getLeaf(x, y + FLOOR_SIZE);
		if(southLeaf){
			leaf->m_leafS = southLeaf;
		}

		//update east
		QTreeLeafNode* eastLeaf = root.getLeaf(x + FLOOR_SIZE, y);
		if(eastLeaf){
			leaf->m_leafE = eastLeaf;
		}
	}

	Floor* floor = leaf->createFloor(z);
	uint32_t offsetX = x & FLOOR_MASK;
	uint32_t offsetY = y & FLOOR_MASK;
	if(!floor->tiles[offsetX][offsetY]){
		floor->tiles[offsetX][offsetY] = newtile;
		newtile->qt_node = leaf;
	}
	else{
		std::cout << "Error: Map::setTile() already exists." << std::endl;
	}

	if(newtile->hasFlag(TILESTATE_REFRESH)){
		RefreshBlock_t rb;
		rb.lastRefresh = OTSYS_TIME();
		if(TileItemVector* newtileItems = newtile->getItemList()){
			for(ItemVector::iterator it = newtileItems->getBeginDownItem(); it != newtileItems->getEndDownItem(); ++it){
				rb.list.push_back((*it)->clone());
			}
		}
		refreshTileMap[newtile] = rb;
	}
}

bool Map::placeCreature(const Position& centerPos, Creature* creature, bool extendedPos /*=false*/, bool forceLogin /*=false*/)
{
	Tile* tile = getTile(centerPos);

	bool foundTile = false;
	bool placeInPZ = false;

	if(tile){
		placeInPZ = tile->hasFlag(TILESTATE_PROTECTIONZONE);

		ReturnValue ret = tile->__queryAdd(0, creature, 1, FLAG_IGNOREBLOCKITEM);
		if(forceLogin || ret == RET_NOERROR || ret == RET_PLAYERISNOTINVITED){
			foundTile = true;
		}
	}

	typedef std::pair<int32_t, int32_t> relPair;
	std::vector<relPair> relList;
	// Extended pos is used when summoning.
	// X is player, 1 is first choice, 2 is second choice
	// --1--
	// -222-
	// 12X21
	// -222-
	// --1--
	if(extendedPos) {
		relList.push_back(relPair(0, -2));
		relList.push_back(relPair(-2, 0));
		relList.push_back(relPair(0, 2));
		relList.push_back(relPair(2, 0));
		std::random_shuffle(relList.begin(), relList.end());
	}
	relList.push_back(relPair(-1, -1));
	relList.push_back(relPair(-1, 0));
	relList.push_back(relPair(-1, 1));
	relList.push_back(relPair(0, -1));
	relList.push_back(relPair(0, 1));
	relList.push_back(relPair(1, -1));
	relList.push_back(relPair(1, 0));
	relList.push_back(relPair(1, 1));

	std::random_shuffle(relList.begin() + (extendedPos? 4 : 0), relList.end());
	uint32_t radius = 1;

	Position tryPos;
	for(uint32_t n = 1; n <= radius && !foundTile; ++n){
		for(std::vector<relPair>::iterator it = relList.begin(); it != relList.end() && !foundTile; ++it){
			int32_t dx = it->first * n;
			int32_t dy = it->second * n;

			tryPos = centerPos;
			tryPos.x = tryPos.x + dx;
			tryPos.y = tryPos.y + dy;

			tile = getTile(tryPos);
			if(!tile || (placeInPZ && !tile->hasFlag(TILESTATE_PROTECTIONZONE)))
				continue;

			if(tile->__queryAdd(0, creature, 1, 0) == RET_NOERROR){
				if(extendedPos) {
					if(isSightClear(centerPos, tryPos, false)) {
						foundTile = true;
						break;
					}
				} else {
					foundTile = true;
					break;
				}
			}
		}
	}

	if(foundTile){
		int32_t index = 0;
		Item* toItem = NULL;
		uint32_t flags = 0;
		Cylinder* toCylinder = tile->__queryDestination(index, creature, &toItem, flags);
		toCylinder->__internalAddThing(creature);
		Tile* toTile = toCylinder->getTile();
		toTile->qt_node->addCreature(creature);
		return true;
	}

#ifdef __DEBUG__
	std::cout << "Failed to place creature onto map!" << std::endl;
#endif

	return false;
}

bool Map::removeCreature(Creature* creature)
{
	Tile* tile = creature->getTile();
	if(tile){
		tile->qt_node->removeCreature(creature);
		tile->__removeThing(creature, 0);
		return true;
	}

	return false;
}

void Map::getSpectatorsInternal(SpectatorVec& list, const Position& centerPos, bool checkforduplicate,
	int32_t minRangeX, int32_t maxRangeX,
	int32_t minRangeY, int32_t maxRangeY,
	int32_t minRangeZ, int32_t maxRangeZ)
{
	int32_t minoffset = centerPos.z - maxRangeZ;
	int32_t x1 = std::min((int32_t)0xFFFF, std::max((int32_t)0, (centerPos.x + minRangeX + minoffset  )));
	int32_t y1 = std::min((int32_t)0xFFFF, std::max((int32_t)0, (centerPos.y + minRangeY + minoffset )));

	int32_t maxoffset = centerPos.z - minRangeZ;
	int32_t x2 = std::min((int32_t)0xFFFF, std::max((int32_t)0, (centerPos.x + maxRangeX + maxoffset )));
	int32_t y2 = std::min((int32_t)0xFFFF, std::max((int32_t)0, (centerPos.y + maxRangeY + maxoffset )));

	int32_t startx1 = x1 - (x1 % FLOOR_SIZE);
	int32_t starty1 = y1 - (y1 % FLOOR_SIZE);
	int32_t endx2 = x2 - (x2 % FLOOR_SIZE);
	int32_t endy2 = y2 - (y2 % FLOOR_SIZE);

	QTreeLeafNode* startLeaf;
	QTreeLeafNode* leafE;
	QTreeLeafNode* leafS;

	startLeaf = getLeaf(startx1, starty1);
	leafS = startLeaf;

	for(int32_t ny = starty1; ny <= endy2; ny += FLOOR_SIZE){
		leafE = leafS;
		for(int32_t nx = startx1; nx <= endx2; nx += FLOOR_SIZE){
			if(leafE){

				CreatureVector& node_list = leafE->creature_list;
				CreatureVector::const_iterator node_iter = node_list.begin();
				CreatureVector::const_iterator node_end = node_list.end();
				if(node_iter != node_end){
					do{
						Creature* creature = *node_iter;
						const Position& cpos = creature->getPosition();
						int32_t offsetZ = centerPos.z - cpos.z;

						if(cpos.z < minRangeZ || cpos.z > maxRangeZ){
							continue;
						}
						if(cpos.y < (centerPos.y + minRangeY + offsetZ) || cpos.y > (centerPos.y + maxRangeY + offsetZ)){
							continue;
						}
						if(cpos.x < (centerPos.x + minRangeX + offsetZ) || cpos.x > (centerPos.x + maxRangeX + offsetZ) ){
							continue;
						}

						if(checkforduplicate){
							if(std::find(list.begin(), list.end(), creature) == list.end()){
								list.push_back(creature);
							}
						}
						else{
							list.push_back(creature);
						}
					}while(++node_iter != node_end);
				}

				leafE = leafE->stepEast();
			}
			else{
				leafE = getLeaf(nx + FLOOR_SIZE, ny);
			}
		}

		if(leafS){
			leafS = leafS->stepSouth();
		}
		else{
			leafS = getLeaf(startx1, ny + FLOOR_SIZE);
		}
	}
}

void Map::getSpectators(SpectatorVec& list, const Position& centerPos,
	bool checkforduplicate /*= false*/, bool multifloor /*= false*/,
	int32_t minRangeX /*= 0*/, int32_t maxRangeX /*= 0*/,
	int32_t minRangeY /*= 0*/, int32_t maxRangeY /*= 0*/)
{
	bool foundCache = false;
	bool cacheResult = false;
	if(minRangeX == 0 && maxRangeX == 0 && minRangeY == 0 && maxRangeY == 0 && multifloor == true && checkforduplicate == false) {
		SpectatorCache::iterator it = spectatorCache.find(centerPos);
		if(it != spectatorCache.end()){
			list = *it->second;
			foundCache = true;
		}
		else{
			cacheResult = true;
		}
	}

	if(!foundCache){
		minRangeX = (minRangeX == 0 ? -maxViewportX : -minRangeX);
		maxRangeX = (maxRangeX == 0 ? maxViewportX : maxRangeX);
		minRangeY = (minRangeY == 0 ? -maxViewportY : -minRangeY);
		maxRangeY = (maxRangeY == 0 ? maxViewportY : maxRangeY);

		int32_t minRangeZ;
		int32_t maxRangeZ;

		if(multifloor){
			if(centerPos.z > 7){
				//underground

				//8->15
				minRangeZ = std::max(centerPos.z - 2, 0);
				maxRangeZ = std::min(centerPos.z + 2, MAP_MAX_LAYERS - 1);
			}
			//above ground
			else if(centerPos.z == 6){
				minRangeZ = 0;
				maxRangeZ = 8;
			}
			else if(centerPos.z == 7){
				minRangeZ = 0;
				maxRangeZ = 9;
			}
			else{
				minRangeZ = 0;
				maxRangeZ = 7;
			}
		}
		else{
			minRangeZ = centerPos.z;
			maxRangeZ = centerPos.z;
		}

		getSpectatorsInternal(list, centerPos, true,
			minRangeX, maxRangeX,
			minRangeY, maxRangeY,
			minRangeZ, maxRangeZ);

		if(cacheResult){
			spectatorCache[centerPos].reset(new SpectatorVec(list));
		}
	}
}

const SpectatorVec& Map::getSpectators(const Position& centerPos)
{
	SpectatorCache::iterator it = spectatorCache.find(centerPos);
	if(it != spectatorCache.end()) {
		return *it->second;
	} else {
		boost::shared_ptr<SpectatorVec> p(new SpectatorVec());
		spectatorCache[centerPos] = p;
		SpectatorVec& list = *p;

		int32_t minRangeX = -maxViewportX;
		int32_t maxRangeX = maxViewportX;
		int32_t minRangeY = -maxViewportY;
		int32_t maxRangeY = maxViewportY;

		int32_t minRangeZ;
		int32_t maxRangeZ;

		if(centerPos.z > 7){
			//underground

			//8->15
			minRangeZ = std::max(centerPos.z - 2, 0);
			maxRangeZ = std::min(centerPos.z + 2, MAP_MAX_LAYERS - 1);
		}
		//above ground
		else if(centerPos.z == 6){
			minRangeZ = 0;
			maxRangeZ = 8;
		}
		else if(centerPos.z == 7){
			minRangeZ = 0;
			maxRangeZ = 9;
		}
		else{
			minRangeZ = 0;
			maxRangeZ = 7;
		}
		
		getSpectatorsInternal(list, centerPos, false,
			minRangeX, maxRangeX,
			minRangeY, maxRangeY,
			minRangeZ, maxRangeZ);

		return list;
	}
}

void Map::clearSpectatorCache()
{
	spectatorCache.clear();
}

bool Map::canThrowObjectTo(const Position& fromPos, const Position& toPos, bool checkLineOfSight /*= true*/,
	int32_t rangex /*= Map::maxClientViewportX*/, int32_t rangey /*= Map::maxClientViewportY*/)
{
	//z checks
	//underground 8->15
	//ground level and above 7->0
	if((fromPos.z >= 8 && toPos.z < 8) || (toPos.z >= 8 && fromPos.z < 8)){
		return false;
	}

	if(fromPos.z - fromPos.z > 2){
		return false;
	}

	int deltax, deltay, deltaz;
	deltax = std::abs(fromPos.x - toPos.x);
	deltay = std::abs(fromPos.y - toPos.y);
	deltaz = std::abs(fromPos.z - toPos.z);

	//distance checks
	if(deltax - deltaz > rangex || deltay - deltaz > rangey){
		return false;
	}

	if(!checkLineOfSight){
		return true;
	}

	return isSightClear(fromPos, toPos, false);
}

bool Map::checkSightLine(const Position& fromPos, const Position& toPos) const
{
	Position start = fromPos;
	Position end = toPos;

	int x, y, z;
	int dx, dy, dz;
	int sx, sy, sz;
	int ey, ez;

	dx = abs(start.x - end.x);
	dy = abs(start.y - end.y);
	dz = abs(start.z - end.z);

	int max = dx, dir = 0;
	if(dy > max){
		max = dy;
		dir = 1;
	}
	if(dz > max){
		max = dz;
		dir = 2;
	}

	switch(dir){
		case 0:
			//x -> x
			//y -> y
			//z -> z
			break;
		case 1:
			//x -> y
			//y -> x
			//z -> z
			std::swap(start.x, start.y);
			std::swap(end.x, end.y);
			std::swap(dx, dy);
			break;
		case 2:
			//x -> z
			//y -> y
			//z -> x
			std::swap(start.x, start.z);
			std::swap(end.x, end.z);
			std::swap(dx, dz);
			break;
	}

	sx = ((start.x < end.x) ? 1 : -1);
	sy = ((start.y < end.y) ? 1 : -1);
	sz = ((start.z < end.z) ? 1 : -1);

	ey = 0, ez = 0;
	x = start.x;
	y = start.y;
	z = start.z;

	int lastrx = x, lastry = y, lastrz = z;

	for( ; x != end.x + sx; x += sx){
		int rx, ry, rz;
		switch(dir){
		case 1:
			rx = y; ry = x; rz = z;
			break;
		case 2:
			rx = z; ry = y; rz = x;
			break;
		default: //0
			rx = x; ry = y; rz = z;
			break;
		}

		if(!(toPos.x == rx && toPos.y == ry && toPos.z == rz) &&
		  !(fromPos.x == rx && fromPos.y == ry && fromPos.z == rz)){
			if(lastrz != rz){
				if(const_cast<Map*>(this)->getTile(lastrx, lastry, std::min(lastrz, rz))){
					return false;
				}
			}
			lastrx = rx; lastry = ry; lastrz = rz;

			const Tile* tile = const_cast<Map*>(this)->getTile(rx, ry, rz);
			if(tile)
			{
				if(tile->hasProperty(BLOCKPROJECTILE))
				{
					return false;
				}
			}
		}

		ey += dy;
		ez += dz;
		if(2*ey >= dx){
			y  += sy;
			ey -= dx;
		}
		if(2*ez >= dx){
			z  += sz;
			ez -= dx;

		}
	}
	return true;
}

bool Map::isSightClear(const Position& fromPos, const Position& toPos, bool floorCheck) const
{
	if(floorCheck && fromPos.z != toPos.z){
		return false;
	}

	// Cast two converging rays and see if either yields a result.
	return
		checkSightLine(fromPos, toPos) ||
		checkSightLine(toPos, fromPos);
}

const Tile* Map::canWalkTo(const Creature* creature, const Position& pos)
{
	switch(creature->getWalkCache(pos)){
		case 0: return NULL;
		case 1: return getTile(pos);
		break;
	}

	//used for none-cached tiles
	Tile* tile = getTile(pos);
	if(creature->getTile() != tile){
		if(!tile || tile->__queryAdd(0, creature, 1, FLAG_PATHFINDING | FLAG_IGNOREFIELDDAMAGE) != RET_NOERROR){
			return NULL;
		}
	}

	return tile;
}

bool Map::getPathTo(const Creature* creature, const Position& destPos,
	std::list<Direction>& listDir, int32_t maxSearchDist /*= -1*/)
{
	if(canWalkTo(creature, destPos) == NULL){
		return false;
	}

	listDir.clear();

	Position startPos = destPos;
	Position endPos = creature->getPosition();

	if(startPos.z != endPos.z){
		return false;
	}

	AStarNodes nodes;
	AStarNode* startNode = nodes.createOpenNode();

	startNode->x = startPos.x;
	startNode->y = startPos.y;

	startNode->g = 0;
	startNode->h = nodes.getEstimatedDistance(startPos.x, startPos.y, endPos.x, endPos.y);
	startNode->f = startNode->g + startNode->h;
	startNode->parent = NULL;

	Position pos;
	pos.z = startPos.z;

	static int32_t neighbourOrderList[8][2] =
	{
		{-1, 0},
		{0, 1},
		{1, 0},
		{0, -1},

		//diagonal
		{-1, -1},
		{1, -1},
		{1, 1},
		{-1, 1},
	};

	const Tile* tile = NULL;
	AStarNode* found = NULL;

	while(maxSearchDist != -1 || nodes.countClosedNodes() < 100){
		AStarNode* n = nodes.getBestNode();
		if(!n){
			listDir.clear();
			return false; //no path found
		}

		if(n->x == endPos.x && n->y == endPos.y){
			found = n;
			break;
		}
		else{
			for(int i = 0; i < 8; ++i){
				pos.x = n->x + neighbourOrderList[i][0];
				pos.y = n->y + neighbourOrderList[i][1];

				bool outOfRange = false;
				if(maxSearchDist != -1 && (std::abs(endPos.x - pos.x) > maxSearchDist ||
					std::abs(endPos.y - pos.y) > maxSearchDist) ){
					outOfRange = true;
				}

				if(!outOfRange && (tile = canWalkTo(creature, pos))){
					//The cost (g) for this neighbour
					int32_t cost = nodes.getMapWalkCost(creature, n, tile, pos);
					int32_t extraCost = nodes.getTileWalkCost(creature, tile);
					int32_t newg = n->g + cost + extraCost;

					//Check if the node is already in the closed/open list
					//If it exists and the nodes already on them has a lower cost (g) then we can ignore this neighbour node

					AStarNode* neighbourNode = nodes.getNodeInList(pos.x, pos.y);
					if(neighbourNode){
						if(neighbourNode->g <= newg){
							//The node on the closed/open list is cheaper than this one
							continue;
						}

						nodes.openNode(neighbourNode);
					}
					else{
						//Does not exist in the open/closed list, create a new node
						neighbourNode = nodes.createOpenNode();
						if(!neighbourNode){
							//seems we ran out of nodes
							listDir.clear();
							return false;
						}
					}

					//This node is the best node so far with this state
					neighbourNode->x = pos.x;
					neighbourNode->y = pos.y;
					neighbourNode->parent = n;
					neighbourNode->g = newg;
					neighbourNode->h = nodes.getEstimatedDistance(neighbourNode->x, neighbourNode->y,
						endPos.x, endPos.y);
					neighbourNode->f = neighbourNode->g + neighbourNode->h;
				}
			}

			nodes.closeNode(n);
		}
	}

	int32_t prevx = endPos.x;
	int32_t prevy = endPos.y;
	int32_t dx, dy;

	while(found){
		pos.x = found->x;
		pos.y = found->y;

		found = found->parent;

		dx = pos.x - prevx;
		dy = pos.y - prevy;

		prevx = pos.x;
		prevy = pos.y;

		if(dx == -1 && dy == -1){
			listDir.push_back(NORTHWEST);
		}
		else if(dx == 1 && dy == -1){
			listDir.push_back(NORTHEAST);
		}
		else if(dx == -1 && dy == 1){
			listDir.push_back(SOUTHWEST);
		}
		else if(dx == 1 && dy == 1){
			listDir.push_back(SOUTHEAST);
		}
		else if(dx == -1){
			listDir.push_back(WEST);
		}
		else if(dx == 1){
			listDir.push_back(EAST);
		}
		else if(dy == -1){
			listDir.push_back(NORTH);
		}
		else if(dy == 1){
			listDir.push_back(SOUTH);
		}
	}

	return !listDir.empty();
}

bool Map::getPathMatching(const Creature* creature, std::list<Direction>& dirList,
	const FrozenPathingConditionCall& pathCondition, const FindPathParams& fpp)
{
	dirList.clear();

	Position startPos = creature->getPosition();
	Position endPos;

	AStarNodes nodes;
	AStarNode* startNode = nodes.createOpenNode();

	startNode->x = startPos.x;
	startNode->y = startPos.y;

	startNode->f = 0;
	startNode->parent = NULL;

	Position pos;
	pos.z = startPos.z;
	int32_t bestMatch = 0;

	static int32_t neighbourOrderList[8][2] = {
		{-1, 0},
		{0, 1},
		{1, 0},
		{0, -1},

		//diagonal
		{-1, -1},
		{1, -1},
		{1, 1},
		{-1, 1},
	};

	const Tile* tile = NULL;
	AStarNode* found = NULL;
	
	while(fpp.maxSearchDist != -1 || nodes.countClosedNodes() < 100){
		AStarNode* n = nodes.getBestNode();
		if(!n){
			if(found){
				//not quite what we want, but we found something
				break;
			}

			dirList.clear();
			return false; //no path found
		}
		
		if(pathCondition(startPos, Position(n->x, n->y, startPos.z), fpp, bestMatch)){
			found = n;
			endPos = Position(n->x, n->y, startPos.z);
			if(bestMatch == 0){
				break;
			}
		}

		int32_t dirCount = (fpp.allowDiagonal ? 8 : 4);
		for(int32_t i = 0; i < dirCount; ++i){
			pos.x = n->x + neighbourOrderList[i][0];
			pos.y = n->y + neighbourOrderList[i][1];

			bool inRange = true;
			if(fpp.maxSearchDist != -1 && (std::abs(startPos.x - pos.x) > fpp.maxSearchDist ||
				std::abs(startPos.y - pos.y) > fpp.maxSearchDist) ){
				inRange = false;
			}

			if(fpp.keepDistance){
				if(!pathCondition.isInRange(startPos, pos, fpp)){
					inRange = false;
				}
			}

			if(inRange && (tile = canWalkTo(creature, pos))){
				//The cost (g) for this neighbour
				int32_t cost = nodes.getMapWalkCost(creature, n, tile, pos);
				int32_t extraCost = nodes.getTileWalkCost(creature, tile);
				int32_t newf = n->f + cost + extraCost;

				//Check if the node is already in the closed/open list
				//If it exists and the nodes already on them has a lower cost (g) then we can ignore this neighbour node

				AStarNode* neighbourNode = nodes.getNodeInList(pos.x, pos.y);
				if(neighbourNode){
					if(neighbourNode->f <= newf){
						//The node on the closed/open list is cheaper than this one
						continue;
					}

					nodes.openNode(neighbourNode);
				}
				else{
					//Does not exist in the open/closed list, create a new node
					neighbourNode = nodes.createOpenNode();
					if(!neighbourNode){
						if(found){
							//not quite what we want, but we found something
							break;
						}

						//seems we ran out of nodes
						dirList.clear();
						return false;
					}
				}

				//This node is the best node so far with this state
				neighbourNode->x = pos.x;
				neighbourNode->y = pos.y;
				neighbourNode->parent = n;
				neighbourNode->f = newf;
			}
		}

		nodes.closeNode(n);
	}
	
	int32_t prevx = endPos.x;
	int32_t prevy = endPos.y;
	int32_t dx, dy;
	
	if(!found){
		return false;
	}
	
	found = found->parent;
	while(found){
		pos.x = found->x;
		pos.y = found->y;
		
		dx = pos.x - prevx;
		dy = pos.y - prevy;

		prevx = pos.x;
		prevy = pos.y;

		if(dx == 1 && dy == 1){
			dirList.push_front(NORTHWEST);
		}
		else if(dx == -1 && dy == 1){
			dirList.push_front(NORTHEAST);
		}
		else if(dx == 1 && dy == -1){
			dirList.push_front(SOUTHWEST);
		}
		else if(dx == -1 && dy == -1){
			dirList.push_front(SOUTHEAST);
		}
		else if(dx == 1){
			dirList.push_front(WEST);
		}
		else if(dx == -1){
			dirList.push_front(EAST);
		}
		else if(dy == 1){
			dirList.push_front(NORTH);
		}
		else if(dy == -1){
			dirList.push_front(SOUTH);
		}

		found = found->parent;
	}

	return true;
}

//*********** AStarNodes *************

AStarNodes::AStarNodes()
{
	curNode = 0;
	openNodes.reset();
}

AStarNode* AStarNodes::createOpenNode()
{
	if(curNode >= MAX_NODES){
		return NULL;
	}

	uint32_t ret_node = curNode;
	curNode++;
	openNodes[ret_node] = 1;
	return &nodes[ret_node];
}

AStarNode* AStarNodes::getBestNode()
{
	if(curNode == 0)
		return NULL;

	int best_node_f = 100000;
	uint32_t best_node = 0;
	bool found = false;

	for(uint32_t i = 0; i < curNode; i++){
		if(nodes[i].f < best_node_f && openNodes[i] == 1){
			found = true;
			best_node_f = nodes[i].f;
			best_node = i;
		}
	}

	if(found){
		return &nodes[best_node];
	}

	return NULL;
}

void AStarNodes::closeNode(AStarNode* node)
{
	uint32_t pos = GET_NODE_INDEX(node);
	if(pos >= MAX_NODES){
		assert(pos >= MAX_NODES);
		std::cout << "AStarNodes. trying to close node out of range" << std::endl;
		return;
	}

	openNodes[pos] = 0;
}

void AStarNodes::openNode(AStarNode* node)
{
	uint32_t pos = GET_NODE_INDEX(node);
	if(pos >= MAX_NODES){
		assert(pos >= MAX_NODES);
		std::cout << "AStarNodes. trying to open node out of range" << std::endl;
		return;
	}

	openNodes[pos] = 1;
}

uint32_t AStarNodes::countClosedNodes()
{
	uint32_t counter = 0;
	for(uint32_t i = 0; i < curNode; i++){
		if(openNodes[i] == 0){
			counter++;
		}
	}
	return counter;
}

uint32_t AStarNodes::countOpenNodes()
{
	uint32_t counter = 0;
	for(uint32_t i = 0; i < curNode; i++){
		if(openNodes[i] == 1){
			counter++;
		}
	}
	return counter;
}

bool AStarNodes::isInList(int32_t x, int32_t y)
{
	for(uint32_t i = 0; i < curNode; i++){
		if(nodes[i].x == x && nodes[i].y == y){
			return true;
		}
	}
	return false;
}

AStarNode* AStarNodes::getNodeInList(int32_t x, int32_t y)
{
	for(uint32_t i = 0; i < curNode; i++){
		if(nodes[i].x == x && nodes[i].y == y){
			return &nodes[i];
		}
	}

	return NULL;
}

int32_t AStarNodes::getMapWalkCost(const Creature* creature, AStarNode* node,
	const Tile* neighbourTile, const Position& neighbourPos)
{
	int cost = 0;
	if(std::abs((int)node->x - neighbourPos.x) == std::abs((int)node->y - neighbourPos.y)){
		//diagonal movement extra cost
		cost = MAP_DIAGONALWALKCOST;
	}
	else{
		cost = MAP_NORMALWALKCOST;
	}

	return cost;
}

int32_t AStarNodes::getTileWalkCost(const Creature* creature, const Tile* tile)
{
	int cost = 0;
	if(tile->getCreatureCount()){
		//destroy creature cost
		cost += MAP_NORMALWALKCOST * 3;
	}

	if(const MagicField* field = tile->getFieldItem()){
		if(!creature->isImmune(field->getCombatType())){
			cost += MAP_NORMALWALKCOST * 3;
		}
	}

	return cost;
}

int AStarNodes::getEstimatedDistance(int32_t x, int32_t y, int32_t xGoal, int32_t yGoal)
{
	int h_diagonal = std::min(std::abs(x - xGoal), std::abs(y - yGoal));
	int h_straight = (std::abs(x - xGoal) + std::abs(y - yGoal));

	return MAP_DIAGONALWALKCOST * h_diagonal + MAP_NORMALWALKCOST * (h_straight - 2 * h_diagonal);
	//return (std::abs(x - xGoal) + std::abs(y - yGoal)) * MAP_NORMALWALKCOST;
}

//*********** Floor constructor **************

Floor::Floor()
{
	for(unsigned int i = 0; i < FLOOR_SIZE; ++i){
		for(unsigned int j = 0; j < FLOOR_SIZE; ++j){
			tiles[i][j] = 0;
		}
	}
}

//**************** QTreeNode **********************
QTreeNode::QTreeNode()
{
	m_isLeaf = false;
	m_child[0] = NULL;
	m_child[1] = NULL;
	m_child[2] = NULL;
	m_child[3] = NULL;
}

QTreeNode::~QTreeNode()
{
	delete m_child[0];
	delete m_child[1];
	delete m_child[2];
	delete m_child[3];
}

QTreeLeafNode* QTreeNode::getLeaf(uint32_t x, uint32_t y)
{
	if(!isLeaf()){
		uint32_t index = ((x & 0x8000) >> 15) | ((y & 0x8000) >> 14);
		if(m_child[index]){
			return m_child[index]->getLeaf(x*2, y*2);
		}
		else{
			return NULL;
		}
	}
	else{
		return static_cast<QTreeLeafNode*>(this);
	}
}

QTreeLeafNode* QTreeNode::getLeafStatic(QTreeNode* root, uint32_t x, uint32_t y)
{
	QTreeNode* currentNode = root;
	uint32_t currentX = x, currentY = y;
	while(currentNode){
		if(!currentNode->isLeaf()){
			uint32_t index = ((currentX & 0x8000) >> 15) | ((currentY & 0x8000) >> 14);
			if(currentNode->m_child[index]){
				currentNode = currentNode->m_child[index];
				currentX = currentX*2;
				currentY = currentY*2;
			}
			else{
				return NULL;
			}
		}
		else{
			return static_cast<QTreeLeafNode*>(currentNode);
		}
	}
	return NULL;
}

QTreeLeafNode* QTreeNode::createLeaf(uint32_t x, uint32_t y, uint32_t level)
{
	if(!isLeaf()){
		uint32_t index = ((x & 0x8000) >> 15) | ((y & 0x8000) >> 14);
		if(!m_child[index]){
			if(level != FLOOR_BITS){
				m_child[index] = new QTreeNode();
			}
			else{
				m_child[index] = new QTreeLeafNode();
				QTreeLeafNode::newLeaf = true;
			}
		}
		return m_child[index]->createLeaf(x*2, y*2, level - 1);
	}
	else{
		return static_cast<QTreeLeafNode*>(this);
	}
}


//************ LeafNode  ************************
bool QTreeLeafNode::newLeaf = false;
QTreeLeafNode::QTreeLeafNode()
{
	for(unsigned int i = 0; i < MAP_MAX_LAYERS; ++i){
		m_array[i] = NULL;
	}
	m_isLeaf = true;
	m_leafS = NULL;
	m_leafE = NULL;
}

QTreeLeafNode::~QTreeLeafNode()
{
	for(unsigned int i = 0; i < MAP_MAX_LAYERS; ++i){
		delete m_array[i];
	}
}

Floor* QTreeLeafNode::createFloor(uint32_t z)
{
	if(!m_array[z]){
		m_array[z] = new Floor();
	}
	return m_array[z];
}

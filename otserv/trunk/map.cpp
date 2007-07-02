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

#include "otsystem.h"
#include <stdio.h>

#include "items.h"
#include "map.h"
#include "tile.h"

#include "player.h"
#include "configmanager.h"

extern ConfigManager g_config;

//client viewport: 8, 6
//minimum viewport 9, 7
//int32_t Map::maxViewportX = 9;
//int32_t Map::maxViewportY = 7;

int32_t Map::maxViewportX = 10; 
int32_t Map::maxViewportY = 10;

Map::Map()
{
	defaultMapLoaded = false;
	mapwidth = 0;
	mapheight = 0;

	mapStoreIdentifier = g_config.getString(ConfigManager::MAP_STORE_FILE);
	houseStoreIdentifier = g_config.getString(ConfigManager::HOUSE_STORE_FILE);
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
		std::cin.get();
		return false;
	}

	std::cout << ":: Loading map from: " << identifier << " " << loader->getSourceDescription() << std::endl;
	
	bool loadMapSuccess = loader->loadMap(this, identifier);
	defaultMapLoaded = true;

	if(!loadMapSuccess){
		switch(getLastError()){
		case LOADMAPERROR_CANNOTOPENFILE:
			std::cout << "FATAL: Could not open the map stream." << std::endl;
			break;
		case LOADMAPERROR_GETPROPFAILED:
			std::cout << "FATAL: Failed to read stream properties. Code: " << getErrorCode() << std::endl;
			break;
		case LOADMAPERROR_OUTDATEDHEADER:
			std::cout << "FATAL: Header information is outdated. Code: " << getErrorCode() << std::endl;
			break;
		case LOADMAPERROR_GETROOTHEADERFAILED:
			std::cout << "FATAL: Failed to read header information. Code: " << getErrorCode() << std::endl;
			break;
		case LOADMAPERROR_FAILEDTOCREATEITEM:
			std::cout << "FATAL: Failed to create an object. Code: " << getErrorCode() << std::endl;
			break;
		case LOADMAPERROR_FAILEDUNSERIALIZEITEM:
			std::cout << "FATAL: Failed to unserialize an object. Code: " << getErrorCode() << std::endl;
			break;
		case LOADMAPERROR_FAILEDTOREADCHILD:
			std::cout << "FATAL: Failed to read child stream. Code: " << getErrorCode() << std::endl;
			break;
		case LOADMAPERROR_UNKNOWNNODETYPE:
			std::cout << "FATAL: Unknown stream node found. Code: " << getErrorCode() << std::endl;
			break;
		
		default:
			std::cout << "FATAL: Unknown error!" << std::endl;
			break;
		}

		std::cin.get();
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
	IOMapSerialize->loadHouseInfo(this, houseStoreIdentifier);
	IOMapSerialize->loadMap(this, mapStoreIdentifier);

	return true;
}


bool Map::saveMap(const std::string& identifier)
{
	std::string storeIdentifier = identifier;
	if(storeIdentifier.empty()){
		storeIdentifier = mapStoreIdentifier;
	}

	IOMapSerialize* IOMapSerialize = IOMapSerialize::getInstance();
	bool saved = false;
	for(uint32_t tries = 0; tries < 3; tries++){
		if(IOMapSerialize->saveMap(this, storeIdentifier)){
			saved = true;
			break;
		}
	}
	
	if(!saved)
		return false;
	
	saved = false;
	for(uint32_t tries = 0; tries < 3; tries++){
		if(IOMapSerialize->saveHouseInfo(this, houseStoreIdentifier)){
			saved = true;
			break;
		}
	}
	return saved;
}

Tile* Map::getTile(uint16_t x, uint16_t y, uint8_t z)
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

void Map::setTile(uint16_t x, uint16_t y, uint8_t z, Tile* newtile)
{
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
	}
	else{
		std::cout << "Error: Map::setTile() already exists." << std::endl;
	}
}

bool Map::placeCreature(const Position& centerPos, Creature* creature, bool forceLogin /*=false*/)
{
	Tile* tile = getTile(centerPos);

	bool foundTile = false;
	bool placeInPZ = false;

	if(tile){
		placeInPZ = tile->isPz();		

		ReturnValue ret = tile->__queryAdd(0, creature, 1, 0);
		if(forceLogin || ret == RET_NOERROR || ret == RET_PLAYERISNOTINVITED){
			foundTile = true;
		}
	}

	typedef std::pair<int32_t, int32_t> relPair;
	std::vector<relPair> relList;
	relList.push_back(relPair(-1, 0));
	relList.push_back(relPair(-1, 0));
	relList.push_back(relPair(0, 1));
	relList.push_back(relPair(0, -1));
	relList.push_back(relPair(1, 1));
	relList.push_back(relPair(1, 0));
	relList.push_back(relPair(-1, 0));
	relList.push_back(relPair(-1, -1));

	std::random_shuffle(relList.begin(), relList.end());
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
			if(!tile || (placeInPZ && !tile->isPz()))
				continue;

			if(tile->__queryAdd(0, creature, 1, 0) == RET_NOERROR){
				foundTile = true;
				break;
			}
		}
	}

	if(foundTile){
		int32_t index = 0;
		Item* toItem = NULL;
		uint32_t flags = 0;
		Cylinder* toCylinder = tile->__queryDestination(index, creature, &toItem, flags);
		toCylinder->__internalAddThing(creature);
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
		tile->__removeThing(creature, 0);
		return true;
	}

	return false;
}

void Map::getSpectators(SpectatorVec& list, const Position& centerPos, bool multifloor /*= false*/,
	int32_t minRangeX /*= 0*/, int32_t maxRangeX /*= 0*/,
	int32_t minRangeY /*= 0*/, int32_t maxRangeY /*= 0*/)
{
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

	CreatureVector::iterator cit;
	Tile* tile;

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
	
	int32_t floorx1, floory1, floorx2, floory2;

	QTreeLeafNode* startLeaf;
	QTreeLeafNode* leafE;
	QTreeLeafNode* leafS;
	Floor* floor;
	int32_t offsetZ;

	startLeaf = getLeaf(startx1, starty1);
	leafS = startLeaf;

	for(int32_t ny = starty1; ny <= endy2; ny += FLOOR_SIZE){
		leafE = leafS;
		for(int32_t nx = startx1; nx <= endx2; nx += FLOOR_SIZE){
			if(leafE){
				for(int32_t nz = minRangeZ; nz <= maxRangeZ; ++nz){

					if(floor = leafE->getFloor(nz)){
						//get current floor limits
						offsetZ = centerPos.z - nz;

						floorx1 = std::min((int32_t)0xFFFF, std::max((int32_t)0, (centerPos.x + minRangeX + offsetZ)));
						floory1 = std::min((int32_t)0xFFFF, std::max((int32_t)0, (centerPos.y + minRangeY + offsetZ)));
						floorx2 = std::min((int32_t)0xFFFF, std::max((int32_t)0, (centerPos.x + maxRangeX + offsetZ)));
						floory2 = std::min((int32_t)0xFFFF, std::max((int32_t)0, (centerPos.y + maxRangeY + offsetZ)));

						for(int ly = 0; ly < FLOOR_SIZE; ++ly){
							for(int lx = 0; lx < FLOOR_SIZE; ++lx){
								if((nx + lx >= floorx1 && nx + lx <= floorx2) && (ny + ly >= floory1 && ny + ly <= floory2)){
									if(tile = floor->tiles[(nx + lx) & FLOOR_MASK][(ny + ly) & FLOOR_MASK]){
										for(cit = tile->creatures.begin(); cit != tile->creatures.end(); ++cit){
											if(std::find(list.begin(), list.end(), *cit) == list.end()){
												list.push_back(*cit);
											}
										}
									}
								}
							}
						}
					}
				}

				leafE = leafE->stepEast();
				//leafE = getLeaf(nx + FLOOR_SIZE, ny);
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

	/*
	//int offsetZ;
	for(int32_t nz = minRangeZ; nz < maxRangeZ + 1; ++nz){
		offsetZ = centerPos.z - nz;

		int32_t floorx1 = std::min((int32_t)0xFFFF, std::max((int32_t)0, (centerPos.x + minRangeX + offsetZ)));
		int32_t floory1 = std::min((int32_t)0xFFFF, std::max((int32_t)0, (centerPos.y + minRangeY + offsetZ)));
		int32_t floorx2 = std::min((int32_t)0xFFFF, std::max((int32_t)0, (centerPos.x + maxRangeX + offsetZ)));
		int32_t floory2 = std::min((int32_t)0xFFFF, std::max((int32_t)0, (centerPos.y + maxRangeY + offsetZ)));

		//std::cout << "x1: " << floorx1 << ", y1: " << floory1 << ", x2: " << floorx2 << ", xy: " << floory2 << ", z: " << nz << std::endl;
			for(int32_t nx = minRangeX + offsetZ; nx <= maxRangeX + offsetZ; ++nx){
				for(int32_t ny = minRangeY + offsetZ; ny <= maxRangeY + offsetZ; ++ny){

				if((nx + centerPos.x >= floorx1 && nx + centerPos.x <= floorx2) && (ny + centerPos.y >= floory1 && ny + centerPos.y <= floory2)){
					tile = getTile(nx + centerPos.x, ny + centerPos.y, nz);
					if(tile){
						for(cit = tile->creatures.begin(); cit != tile->creatures.end(); ++cit){
							if(std::find(list.begin(), list.end(), *cit) == list.end()){
								list.push_back(*cit);
							}
						}
					}
				}

			}
		}
	}
	*/
}

bool Map::canThrowObjectTo(const Position& fromPos, const Position& toPos)
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
	if(deltax - deltaz > 8 || deltay - deltaz > 6){
		return false;
	}

	return isViewClear(fromPos, toPos, false);
}

bool Map::isViewClear(const Position& fromPos, const Position& toPos, bool floorCheck)
{
	if(floorCheck && fromPos.z != toPos.z){
		return false;
	}

	Position start = fromPos;
	Position end = toPos;

	int deltax, deltay, deltaz;
	deltax = abs(start.x - end.x);
	deltay = abs(start.y - end.y);
	deltaz = abs(start.z - end.z);

	int max = deltax, dir = 0;
	if(deltay > max){
		max = deltay; 
		dir = 1;
	}
	if(deltaz > max){
		max = deltaz; 
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
		std::swap(deltax, deltay);
		break;
	case 2:
		//x -> z
		//y -> y
		//z -> x
		std::swap(start.x, start.z);
		std::swap(end.x, end.z);
		std::swap(deltax, deltaz);
		break;
	}

	int stepx = ((start.x < end.x) ? 1 : -1);
	int stepy = ((start.y < end.y) ? 1 : -1);
	int stepz = ((start.z < end.z) ? 1 : -1);
	
	int x, y, z;
	int errory = 0, errorz = 0;
	x = start.x;
	y = start.y;
	z = start.z;
	
	int lastrx = x, lastry = y, lastrz = z;
	
	for( ; x != end.x + stepx; x += stepx){
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
				if(getTile(lastrx, lastry, std::min(lastrz, rz))){
					return false;
				}
			}
			lastrx = rx; lastry = ry; lastrz = rz;
			
			Tile* tile = getTile(rx, ry, rz);
			if(tile){
				if(tile->hasProperty(BLOCKPROJECTILE))
					return false;
			}
		}

		errory += deltay;
		errorz += deltaz;
		if(2*errory >= deltax){
			y += stepy;
			errory -= deltax;
		}
		if(2*errorz >= deltax){
			z += stepz;
			errorz -= deltax;
		}
	}
	return true;
}

bool Map::isPathValid(const Creature* creature, const std::list<Direction>& listDir, const Position& destPos)
{
	Position pos = creature->getPosition();

	std::list<Direction>::const_iterator it;
	for(it = listDir.begin(); it != listDir.end(); ++it) {
		switch(*it){
			case NORTH: pos.y -= 1; break;
			case SOUTH: pos.y += 1; break;
			case WEST: pos.x -= 1; break;
			case EAST: pos.x += 1; break;
			case NORTHEAST: pos.x += 1; pos.y -= 1; break;
			case NORTHWEST: pos.x -= 1; pos.y -= 1; break;
			case SOUTHEAST: pos.x += 1; pos.y += 1; break;
			case SOUTHWEST: pos.x -= 1; pos.y += 1; break;
		}

		Tile* tile = getTile(pos);
		if(!tile || !tile->creatures.empty() ||  tile->__queryAdd(0, creature, 1, FLAG_PATHFINDING) != RET_NOERROR){
			return false;
		}
	}

	if(std::abs(destPos.x - pos.x) <= 1 && std::abs(destPos.y - pos.y) <= 1){
		return true;
	}

	return false;
}

bool Map::getPathTo(const Creature* creature, Position toPosition, std::list<Direction>& listDir)
{
	Position startPos = creature->getPosition();

	if(startPos.z != toPosition.z){
		return false;
	}

	AStarNodes nodes;
	AStarNode* startNode = nodes.createOpenNode();

	startNode->x = startPos.x;
	startNode->y = startPos.y;

	startNode->g = 0; 
	startNode->h = nodes.getEstimatedDistance(startPos.x, startPos.y, toPosition.x, toPosition.y);
	startNode->f = startNode->g + startNode->h;
	startNode->parent = NULL;

	int32_t x, y;
	int32_t z = startPos.z;

	int32_t neighbourOrderList[8][2] =
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

	Tile* tile = NULL;
	AStarNode* found = NULL;
	
	while(nodes.countClosedNodes() < 100){		
		AStarNode* n = nodes.getBestNode();
		if(!n){
			listDir.clear();
			return false; //no path found
		}

		if(n->x == toPosition.x && n->y == toPosition.y){
			found = n;
			break;
		}
		else{
			for(int i = 0; i < 8; ++i){
				x = n->x + neighbourOrderList[i][0];
				y = n->y + neighbourOrderList[i][1];
			
				if((x == startPos.x && y == startPos.y)){
					continue;
				}

				tile = getTile(x, y, z);

				if(tile && tile->__queryAdd(0, creature, 1, FLAG_PATHFINDING) == RET_NOERROR){
					//The cost (g) for this neighbour
					int32_t newg = n->g + nodes.getMapWalkCost(creature, n, tile);

					//Check if the node is already in the closed/open list
					//If it exists and the nodes already on them has a lower cost (g) then we can ignore this neighbour node

					AStarNode* neighbourNode = nodes.getNodeInList(x, y);
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
					}

					//This node is the best node so far with this state
					neighbourNode->x = x;
					neighbourNode->y = y;
					neighbourNode->parent = n;
					neighbourNode->g = newg;
					neighbourNode->h = nodes.getEstimatedDistance(neighbourNode->x, neighbourNode->y,
						toPosition.x, toPosition.y);
					neighbourNode->f = neighbourNode->g + neighbourNode->h;
				}

			}
			
			nodes.closeNode(n);
		}
	}

	int32_t prevx = toPosition.x;
	int32_t prevy = toPosition.y;
	int32_t dx, dy;

	while(found){
		x = found->x;
		y = found->y;

		found = found->parent;

		dx = x - prevx;
		dy = y - prevy;

		prevx = x;
		prevy = y;

		if(dx == -1 && dy == -1){
			listDir.insert(listDir.begin(), SOUTHEAST);
		}
		else if(dx == 1 && dy == -1){
			listDir.insert(listDir.begin(), SOUTHWEST);
		}
		else if(dx == -1 && dy == 1){
			listDir.insert(listDir.begin(), NORTHEAST);
		}
		else if(dx == 1 && dy == 1){
			listDir.insert(listDir.begin(), NORTHWEST);
		}
		else if(dx == -1){
			listDir.insert(listDir.begin(), EAST);
		}
		else if(dx == 1){
			listDir.insert(listDir.begin(), WEST);
		}
		else if(dy == -1){
			listDir.insert(listDir.begin(), SOUTH);
		}
		else if(dy == 1){
			listDir.insert(listDir.begin(), NORTH);
		}
	}

	return !listDir.empty();
}

//*********** AStarNodes *************

AStarNodes::AStarNodes()
{
	curNode = 0;
	openNodes.reset();
}

AStarNode* AStarNodes::createOpenNode()
{
	if(curNode >= MAX_NODES)
		return NULL;
	
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
	if(pos < 0 || pos >= MAX_NODES){
		std::cout << "AStarNodes. trying to close node out of range" << std::endl;
		return;
	}

	openNodes[pos] = 0;
}

void AStarNodes::openNode(AStarNode* node)
{
	uint32_t pos = GET_NODE_INDEX(node);
	if(pos < 0 || pos >= MAX_NODES){
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

int AStarNodes::getMapWalkCost(const Creature* creature, AStarNode* node, const Tile* neighbourTile)
{
	int cost = 0;
	if(std::abs((int)node->x - neighbourTile->getPosition().x) == std::abs((int)node->y - neighbourTile->getPosition().y)){
		//diagonal movement extra cost
		cost = MAP_DIAGONALWALKCOST;
	}
	else{
		cost = MAP_NORMALWALKCOST;
	}

	/*
	if(!neighbourTile->hasProperty(BLOCKPATHFIND)){
		//extra cost for blockpath find flag
		cost = cost + 20;
	}
	*/

	if(!neighbourTile->creatures.empty()){
		//destroy creature cost
		cost = cost + MAP_NORMALWALKCOST * 10;
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

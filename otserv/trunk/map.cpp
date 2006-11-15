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
int32_t Map::maxViewportX = 9; 
int32_t Map::maxViewportY = 7;

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

	std::cout << ":: Loading map from: " << loader->getSourceDescription() << std::endl;

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
	for(long tries = 0;tries < 3;tries++){
		if(IOMapSerialize->saveMap(this, storeIdentifier)){
			saved = true;
			break;
		}
	}
	
	if(!saved)
		return false;
	
	saved = false;
	for(long tries = 0;tries < 3;tries++){
		if(IOMapSerialize->saveHouseInfo(this, houseStoreIdentifier)){
			saved = true;
			break;
		}
	}
	return saved;
}

Tile* Map::getTile(uint16_t _x, uint16_t _y, uint8_t _z)
{
	if(_z < MAP_MAX_LAYERS){
		// _x & 0x7F  is like _x % 128
		//TileMap *tm = &tileMaps[_x & 0x1F][_y & 0x1F][_z];
		//TileMap *tm = &tileMaps[_x & 0xFF][_y & 0xFF];
		TileMap* tm = &tileMaps[_x & 0x7F][_y & 0x7F];

		if(!tm)
			return NULL;
	
		// search in the stl map for the requested tile
		//TileMap::iterator it = tm->find((_x << 16) | _y);
		//TileMap::iterator it = tm->find(_x & 0xFF00) << 8 | (_y & 0xFF00) | _z);
		TileMap::iterator it = tm->find((_x & 0xFF80) << 16 | (_y & 0xFF80) << 1 | _z);

		// ... found
		if(it != tm->end())
			return it->second;
	}
	
	// or not
	return NULL;
}

Tile* Map::getTile(const Position& pos)
{ 
	return getTile(pos.x, pos.y, pos.z);
}

void Map::setTile(uint16_t _x, uint16_t _y, uint8_t _z, Tile* newtile)
{
	Tile* tile = getTile(_x, _y, _z);

	if(!tile){
		tileMaps[_x & 0x7F][_y & 0x7F][ (_x & 0xFF80) << 16 | (_y & 0xFF80) << 1 | _z] = newtile;
	}
	else{
		std::cout << "Error: Map::setTile() already exists." << std::endl;
	}
}

bool Map::placeCreature(const Position& pos, Creature* creature, bool forceLogin /*=false*/)
{
	Tile* tile = getTile(pos.x, pos.y, pos.z);

	bool foundTile = false;
	bool placeInPZ = false;

	if(tile){
		placeInPZ = tile->isPz();		

		ReturnValue ret = tile->__queryAdd(0, creature, 1, 0);
		if(forceLogin || ret == RET_NOERROR || ret == RET_PLAYERISNOTINVITED){
			foundTile = true;
		}
	}

	for(int cx = pos.x - 1; cx <= pos.x + 1 && !foundTile; cx++){
		for(int cy = pos.y - 1; cy <= pos.y + 1 && !foundTile; cy++){
			tile = getTile(cx, cy, pos.z);
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
	int32_t offsetZ;

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

	for(int nz = minRangeZ; nz < maxRangeZ + 1; ++nz){
		offsetZ = centerPos.z - nz;

		for(int nx = minRangeX + offsetZ; nx <= maxRangeX + offsetZ; ++nx){
			for(int ny = minRangeY + offsetZ; ny <= maxRangeY + offsetZ; ++ny){
				tile = getTile(nx + centerPos.x, ny + centerPos.y, nz);
				if(tile){
					for(cit = tile->creatures.begin(); cit != tile->creatures.end(); ++cit){
						if(std::find(list.begin(), list.end(), *cit) == list.end()){
							//if((*cit)->canSee(centerPos)){
								list.push_back(*cit);
							//}
						}
					}
				}
			}
		}
	}
}

bool Map::canThrowObjectTo(const Position& fromPos, const Position& toPos)
{
	Position start = fromPos;
	Position end = toPos;
	
	//z checks
	//underground 8->15
	//ground level and above 7->0
	if((start.z >= 8 && end.z < 8) || (end.z >= 8 && start.z < 8))
		return false;
	
	if(start.z - end.z > 2)
		return false;
	
	int deltax, deltay, deltaz;
	deltax = abs(start.x - end.x);
	deltay = abs(start.y - end.y);
	deltaz = abs(start.z - end.z);
    
	//distance checks
	if(deltax - deltaz > 8 || deltay - deltaz > 6){
		return false;
	}
    
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
			
			Tile *tile = getTile(rx, ry, rz);
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
		if(!tile || tile->__queryAdd(0, creature, 1, FLAG_PATHFINDING) != RET_NOERROR){
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
	if(creature->getPosition().z != toPosition.z){
		return false;
	}

	Tile* tile;
	AStarNodes nodes;
	AStarNode* found = NULL;

	Position startPos = creature->getPosition();

	AStarNode* startNode = nodes.createOpenNode();
	startNode->parent = NULL;
	startNode->h = 0;
	startNode->x = toPosition.x;
	startNode->y = toPosition.y;

	int32_t x, y;
	int32_t dx, dy;
	int32_t z = toPosition.z;

	while(!found && nodes.countClosedNodes() < 100){		
		AStarNode* current = nodes.getBestNode();
		if(!current){
			listDir.clear();
			return false; //no path found
		}

		nodes.closeNode(current);

		for(int dx = -1; dx <= 1; dx++){
			for(int dy = -1; dy <= 1; dy++){

				x = current->x + dx;
				y = current->y + dy;

				if(!(x == startPos.x && y == startPos.y)){
					tile = getTile(x, y, z);

					if(!tile || tile->__queryAdd(0, creature, 1, FLAG_PATHFINDING) != RET_NOERROR){
						continue;
					}
				}
				
				if(!nodes.isInList(x,y)){
					AStarNode* n = nodes.createOpenNode();
					if(n){
						n->x = x;
						n->y = y;

						n->h = std::abs(n->x - startPos.x) * std::abs(n->x - startPos.x) +
									std::abs(n->y - startPos.y) * std::abs(n->y - startPos.y);

						n->parent = current;

						if(x == startPos.x && y == startPos.y){
							found = n;
						}
					}
				}
			}
		}
	}

	int32_t prevx = startPos.x;
	int32_t prevy = startPos.y;

	while(found){
		x = found->x;
		y = found->y;

		found = found->parent;

		dx = x - prevx;
		dy = y - prevy;

		prevx = x;
		prevy = y;

		if(dx == -1 && dy == -1){
			//north-west
			tile = getTile(x + 1, y, z);
			if(tile && tile->__queryAdd(0, creature, 1, FLAG_PATHFINDING) == RET_NOERROR){
				listDir.push_back(NORTH);
				listDir.push_back(WEST);
				continue;
			}

			//west-north
			tile = getTile(x, y + 1, z);
			if(tile && tile->__queryAdd(0, creature, 1, FLAG_PATHFINDING) == RET_NOERROR){
				listDir.push_back(WEST);
				listDir.push_back(NORTH);
				continue;
			}
			
			listDir.push_back(NORTHWEST);
		}
		else if(dx == 1 && dy == -1){
			//north-east
			tile = getTile(x - 1, y, z);
			if(tile && tile->__queryAdd(0, creature, 1, FLAG_PATHFINDING) == RET_NOERROR){
				listDir.push_back(NORTH);
				listDir.push_back(EAST);
				continue;
			}

			//east-north
			tile = getTile(x, y + 1, z);
			if(tile && tile->__queryAdd(0, creature, 1, FLAG_PATHFINDING) == RET_NOERROR){
				listDir.push_back(EAST);
				listDir.push_back(NORTH);
				continue;
			}

			listDir.push_back(NORTHEAST);
		}
		else if(dx == -1 && dy == 1){
			//south-west
			tile = getTile(x + 1, y, z);
			if(tile && tile->__queryAdd(0, creature, 1, FLAG_PATHFINDING) == RET_NOERROR){
				listDir.push_back(SOUTH);
				listDir.push_back(WEST);
				continue;
			}

			//west-south
			tile = getTile(x, y - 1, z);
			if(tile && tile->__queryAdd(0, creature, 1, FLAG_PATHFINDING) == RET_NOERROR){
				listDir.push_back(WEST);
				listDir.push_back(SOUTH);
				continue;
			}

			listDir.push_back(SOUTHWEST);
		}
		else if(dx == 1 && dy == 1){
			//south-east
			tile = getTile(x - 1, y, z);
			if(tile && tile->__queryAdd(0, creature, 1, FLAG_PATHFINDING) == RET_NOERROR){
				listDir.push_back(SOUTH);
				listDir.push_back(EAST);
				continue;
			}

			//east-south
			tile = getTile(x, y - 1, z);
			if(tile && tile->__queryAdd(0, creature, 1, FLAG_PATHFINDING) == RET_NOERROR){
				listDir.push_back(EAST);
				listDir.push_back(SOUTH);
				continue;
			}

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

AStarNodes::AStarNodes()
{
	curNode = 0;
	openNodes.reset();
}

AStarNode* AStarNodes::createOpenNode()
{
	if(curNode >= MAX_NODES)
		return NULL;
	
	unsigned long ret_node = curNode;
	curNode++;
	openNodes[ret_node] = 1;
	return &nodes[ret_node];
}

AStarNode* AStarNodes::getBestNode()
{
	if(curNode == 0)
		return NULL;

	int best_node_h;
	unsigned long best_node;
	bool found;
	
	best_node_h = 100000;
	best_node = 0;
	found = false;

	for(unsigned long i = 0; i < curNode; i++){
		if(nodes[i].h < best_node_h && openNodes[i] == 1){
			found = true;
			best_node_h = nodes[i].h;
			best_node = i;
		}
	}
	if(found){
		return &nodes[best_node];
	}
	else{
		return NULL;
	}
}

void AStarNodes::closeNode(AStarNode* node)
{
	unsigned long pos = GET_NODE_INDEX(node);
	if(pos < 0 || pos >= MAX_NODES){
		std::cout << "AStarNodes. trying to close node out of range" << std::endl;
		return;
	}

	openNodes[pos] = 0;
}

unsigned long AStarNodes::countClosedNodes()
{
	unsigned long counter = 0;
	for(unsigned long i = 0; i < curNode; i++){
		if(openNodes[i] == 0){
			counter++;
		}
	}
	return counter;
}

unsigned long AStarNodes::countOpenNodes()
{
	unsigned long counter = 0;
	for(unsigned long i = 0; i < curNode; i++){
		if(openNodes[i] == 1){
			counter++;
		}
	}
	return counter;
}

bool AStarNodes::isInList(long x, long y)
{
	for(unsigned long i = 0; i < curNode; i++){
		if(nodes[i].x == x && nodes[i].y == y){
			return true;
		}
	}
	return false;
}

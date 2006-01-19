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


#include "definitions.h"

#include <string>
#include <sstream>
#include <map>
#include <algorithm>

#include <boost/config.hpp>
#include <boost/bind.hpp>

#include "iomap.h"

#ifdef ENABLESQLMAPSUPPORT	
#include "iomapsql.h"
#endif
//#include "iomapbin.h"
#include "iomapxml.h"
#include "iomapotbm.h"

#include "otsystem.h"
#include <stdio.h>

#include "items.h"
#include "map.h"
#include "tile.h"

#include "player.h"
#include "luascript.h"
//#include "spells.h"

#define EVENT_CHECKPLAYER          123
#define EVENT_CHECKPLAYERATTACKING 124

extern LuaScript g_config;
//extern Spells spells;


Map::Map() :
	spawnfile(""),
	mapwidth(0),
	mapheight(0)
{
	//
}


Map::~Map()
{
	//
}


int Map::loadMap(std::string filename, std::string filekind)
{
	int ret;
	IOMap* loader;
	
	/*if(filekind == "BIN"){
		loader = new IOMapBin();
		ret = SPAWN_BUILTIN;
	}
	else */if(filekind == "XML"){
		loader = new IOMapXML();
		ret = SPAWN_XML;
	}
	else if(filekind == "OTBM"){
		loader = new IOMapOTBM();
		//ret = SPAWN_BUILTIN;
		ret = SPAWN_XML;
	}
	#ifdef ENABLESQLMAPSUPPORT
	else if(filekind == "SQL"){
		loader = new IOMapSQL();
		ret = SPAWN_SQL;
	}
#endif	
	else{
 		std::cout << "FATAL: couldnt determine the map format! exiting1" << std::endl;
		exit(1);
	}

	std::cout << ":: Loading map from: " << loader->getSourceDescription() << std::endl;
	bool success = loader->loadMap(this, filename);
	delete loader;
	
	///*
	if(success){
		return ret;
	}
	else{
		return MAP_LOADER_ERROR;
	}
	//*/
}

Tile* Map::getTile(unsigned short _x, unsigned short _y, unsigned char _z)
{
	if(_z < MAP_LAYER){
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

void Map::setTile(unsigned short _x, unsigned short _y, unsigned char _z, unsigned short groundId)
{
  Tile* tile = getTile(_x, _y, _z);

	if(tile != NULL){
		if(tile->ground)
			//delete tile->ground;
			tile->ground->releaseThing2();

		if(groundId != 0 && Item::items[groundId].isGroundTile()){
			tile->ground = Item::CreateItem(groundId);
			tile->ground->setParent(tile);
		}
		else{
			tile = new Tile(_x, _y, _z);

			if(groundId != 0 && Item::items[groundId].isGroundTile()){
				tile->ground = Item::CreateItem(groundId);
				tile->ground->setParent(tile);
			}
		}
	}
}

Tile* Map::setTile(unsigned short _x, unsigned short _y, unsigned char _z)
{
	Tile *tile = getTile(_x, _y, _z);
	
	if(!tile){
    	tile = new Tile(_x, _y,_z);
		//tileMaps[_x & 0x1F][_y & 0x1F][_z][(_x << 16) | _y] = tile;
		//tileMaps[_x & 0xFF][_y & 0xFF][ _x & 0xFF00) << 8 | (_y & 0xFF00) | _z] = tile;
		tileMaps[_x & 0x7F][_y & 0x7F][ (_x & 0xFF80) << 16 | (_y & 0xFF80) << 1 | _z] = tile;
  	} 
  	return tile;
}

bool Map::placeCreature(const Position& pos, Creature* creature)
{
	Tile* tile = getTile(pos.x, pos.y, pos.z);

	if(tile){
		ReturnValue ret = tile->__queryAdd(0, creature, 0);

		if(ret == RET_NOERROR){
			tile->__internalAddThing(creature);
			return true;
		}
		else if(tile->hasFlag(TILESTATE_HOUSE) && ret == RET_PLAYERISNOTINVITED){
			int32_t index = 0;
			Item* toItem = NULL;
			Cylinder* cylinder = tile->__queryDestination(index, creature, &toItem);
			cylinder->__internalAddThing(creature);
			return true;
		}
	}

	for(int cx = pos.x - 1; cx <= pos.x + 1; cx++){
		for(int cy = pos.y - 1; cy <= pos.y + 1; cy++){
			tile = getTile(cx, cy, pos.z);

			//isSuccess= (tile && !tile->getTeleportItem() && !tile->floorChange() && tile->__queryAdd(0, creature, 0) == RET_NOERROR);

			if(tile && tile->__queryAdd(0, creature, 0) == RET_NOERROR){
				tile->__internalAddThing(creature);
				return true;
			}
		}
	}

#ifdef __DEBUG__
	std::cout << "Failed to place creature onto map!" << std::endl;
#endif

	return false;

	/*
	Tile* tile = getTile(pos.x, pos.y, pos.z);

	bool success = (tile && !tile->getTeleportItem() && !tile->floorChange() && tile->__queryAdd(0, creature, 0) == RET_NOERROR);
	if(!success){
		for(int cx = pos.x - 1; cx <= pos.x + 1 && !success; cx++){
			for(int cy = pos.y - 1; cy <= pos.y + 1 && !success; cy++){
				tile = getTile(cx, cy, pos.z);
				success = success = (tile && !tile->getTeleportItem() && !tile->floorChange() && tile->__queryAdd(0, creature, 0) == RET_NOERROR);
				if(success){
					break;
				}
			}
		}
	}

	if(success){
		tile->__internalAddThing(creature);
		return true;
	}

#ifdef __DEBUG__
	std::cout << "Failed to place creature onto map!" << std::endl;
#endif

	return false;
	*/
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

void Map::getSpectators(const Range& range, SpectatorVec& list)
{
/*
#ifdef __DEBUG__
	std::cout << "Viewer position at x: " << range.centerpos.x
		<< ", y: " << range.centerpos.y
		<< ", z: " << range.centerpos.z << std::endl;
	std::cout << "Min Range x: " << range.minRange.x
		<< ", y: " << range.minRange.y
		<< ", z: " << range.minRange.z << std::endl;
    std::cout << "Max Range x: " << range.maxRange.x
		<< ", y: " << range.maxRange.y
		<< ", z: " << range.maxRange.z << std::endl;    	
#endif
*/

	int offsetz;
	CreatureVector::iterator cit;
	Tile *tile;

	//for(int nz = range.minRange.z; nz != range.maxRange.z + range.zstep; nz++) {
	for(int nz = range.minRange.z; nz != range.maxRange.z + range.zstep; nz += range.zstep){
		offsetz = range.centerpos.z - nz;
		for(int nx = range.minRange.x + offsetz; nx <= range.maxRange.x + offsetz; ++nx){
			for(int ny = range.minRange.y + offsetz; ny <= range.maxRange.y + offsetz; ++ny){
				tile = getTile(nx + range.centerpos.x, ny + range.centerpos.y, nz);
				if(tile){
					for(cit = tile->creatures.begin(); cit != tile->creatures.end(); ++cit){
/*
#ifdef __DEBUG__
						std::cout << "Found " << (*cit)->getName() << " at x: " << (*cit)->pos.x << ", y: " << (*cit)->pos.y << ", z: " << (*cit)->pos.z << ", offset: " << offsetz << std::endl;
#endif
*/					
						if(std::find(list.begin(), list.end(), *cit) == list.end()){
							list.push_back(*cit);
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
	
	if(start.z - end.z > 3)
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
		case 0:
			rx = x; ry = y; rz = z;
			break;
		case 1:
			rx = y; ry = x; rz = z;
			break;
		case 2:
			rx = z; ry = y; rz = x;
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

bool Map::isPathValid(Creature* creature, const std::list<Position>& path, int pathSize,
	bool ignoreMoveableBlockingItems /*= false)*/)
{
	int pathCount = 0;
	std::list<Position>::const_iterator iit;
	for(iit = path.begin(); iit != path.end(); ++iit) {

		Tile* tile = getTile(iit->x, iit->y, iit->z);
		if(tile){
			if(creature->getTile() != tile){
				ReturnValue ret = tile->__queryAdd(0, creature, 1);

				if(ret != RET_NOERROR)
					continue;
			}

			/*
			if(!tile->creatures.empty() && creature->getTile() != tile)
				continue;

			if(ignoreMoveableBlockingItems){
				if(tile->hasProperty(NOTMOVEABLEBLOCKSOLID) || tile->hasProperty(NOTMOVEABLEBLOCKPATHFIND))
					continue;
			}
			else{
				if(tile->hasProperty(BLOCKSOLID) || tile->hasProperty(BLOCKPATHFIND))
					continue;
			}
			*/
		}
		else
			return false;

		if(pathCount++ >= pathSize)
			return RET_NOERROR;
	}

	return true;
}

std::list<Position> Map::getPathTo(Creature* creature, Position start, Position to,
	bool creaturesBlock /*=true*/, bool ignoreMoveableBlockingItems /*= false*/, int maxNodSize /*= 100*/)
{
	std::list<Position> path;
	AStarNodes nodes;
	AStarNode* found = NULL;
	int z = start.z;
	AStarNode* startNode = nodes.createOpenNode();
	startNode->parent = NULL;
	startNode->h = 0;
	startNode->x = start.x;
	startNode->y = start.y;
	
	while(!found && nodes.countClosedNodes() < maxNodSize){		
		AStarNode* current = nodes.getBestNode();
		if(!current)
			return path; //no path
		
		nodes.closeNode(current);
		
		for(int dx=-1; dx <= 1; dx++){
			for(int dy=-1; dy <= 1; dy++){
				if(std::abs(dx) != std::abs(dy)){
					int x = current->x + dx;
					int y = current->y + dy;

					Tile* tile = getTile(x, y, z);
					if(tile){
						if(creature->getTile() != tile){
							ReturnValue ret = tile->__queryAdd(0, creature, 1);

							if(ret != RET_NOERROR)
								continue;
						}

						/*
						if(!tile->creatures.empty() && creature->getTile() != tile)
							continue;

						if(ignoreMoveableBlockingItems){
							if(tile->hasProperty(NOTMOVEABLEBLOCKSOLID) || tile->hasProperty(NOTMOVEABLEBLOCKPATHFIND))
								continue;
						}
						else{
							if(tile->hasProperty(BLOCKSOLID) || tile->hasProperty(BLOCKPATHFIND))
								continue;
						}
						*/
					}
					else
						continue;
					
					if(!nodes.isInList(x,y)){
						AStarNode* n = nodes.createOpenNode();
						if(n){
							n->x = x;
							n->y = y;
							n->h = abs(n->x - to.x)*abs(n->x - to.x) + abs(n->y - to.y)*abs(n->y - to.y);
							n->parent = current;
							if(x == to.x && y == to.y){
								found = n;
							}
						}
					}
/*					else{
						if(current->g + 1 < child->g)
							child->parent = current;
							child->g=current->g+1;
					}*/
				}
			}
		}
	}
	//cleanup the mess
	while(found){
		Position p;
		p.x = found->x;
		p.y = found->y;
		p.z = z;
		path.push_front(p);
		found = found->parent;
	}

	return path;
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

	for(int i = 0; i < curNode; i++){
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
	for(int i = 0; i < curNode; i++){
		if(openNodes[i] == 0){
			counter++;
		}
	}
	return counter;
}

unsigned long AStarNodes::countOpenNodes()
{
	unsigned long counter = 0;
	for(int i = 0; i < curNode; i++){
		if(openNodes[i] == 1){
			counter++;
		}
	}
	return counter;
}


bool AStarNodes::isInList(unsigned long x, unsigned long y)
{
	for(int i = 0; i < curNode; i++){
		if(nodes[i].x == x && nodes[i].y == y){
			return true;
		}
	}
	return false;
}

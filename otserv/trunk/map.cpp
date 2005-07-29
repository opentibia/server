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

using namespace std;

#include "iomap.h"

#ifdef _SQLMAP_
#include "iomapsql.h"
#elif _BINMAP_
#include "iomapbin.h"
#else
#include "iomapxml.h"
#endif

#include "otsystem.h"
#include <stdio.h>

#include "items.h"
#include "map.h"
#include "tile.h"

#include "player.h"
#include "tools.h"

#include "npc.h"
#include "spells.h"


#include "luascript.h"
#include <ctype.h>

#define EVENT_CHECKPLAYER          123
#define EVENT_CHECKPLAYERATTACKING 124

extern LuaScript g_config;
extern Spells spells;
extern std::map<long, Creature*> channel;

Map::Map() :
spawnfile(""),
mapwidth(0),
mapheight(0)
{
/*	//first we fill the map with
  for(int y = 0; y < MAP_HEIGHT; y++)
  {
    for(int x = 0; x < MAP_WIDTH; x++)
    {
      //				setTile(x,y,7,102);
    }
  }
*/

  //OTSYS_THREAD_LOCKVARINIT(mapLock);
}


Map::~Map()
{
}


bool Map::loadMap(std::string filename) {
#ifdef _SQLMAP_
	IOMap* loader = new IOMapSQL();
#elif _BINMAP_
	IOMap* loader = new IOMapBin();
#else
	IOMap* loader = new IOMapXML();
#endif

	std::cout << ":: Loading map from: " << loader->getSourceDescription() << std::endl;
	bool success = loader->loadMap(this, filename);
	delete loader;
	return success;
}

Tile* Map::getTile(unsigned short _x, unsigned short _y, unsigned char _z)
{
  if (_z < MAP_LAYER)
  {
	// _x & 0x7F  is like _x % 128
	//TileMap *tm = &tileMaps[_x & 0x1F][_y & 0x1F][_z];
	//TileMap *tm = &tileMaps[_x & 0xFF][_y & 0xFF];
    TileMap *tm = &tileMaps[_x & 0x7F][_y & 0x7F];
	if(!tm)
		return NULL;
	
    // search in the stl map for the requested tile
    //TileMap::iterator it = tm->find((_x << 16) | _y);
    //TileMap::iterator it = tm->find(_x & 0xFF00) << 8 | (_y & 0xFF00) | _z);
    TileMap::iterator it = tm->find((_x & 0xFF80) << 16 | (_y & 0xFF80) << 1 | _z);

    // ... found
    if (it != tm->end())
      return it->second;
  }
	
	 // or not
  return NULL;
}

Tile* Map::getTile(const Position &pos)
{
	return getTile(pos.x, pos.y, pos.z);
}


void Map::setTile(unsigned short _x, unsigned short _y, unsigned char _z, unsigned short groundId)
{
  Tile *tile = getTile(_x, _y, _z);

  if (tile != NULL)
  {
		if(tile->ground)
			//delete tile->ground;
			tile->ground->releaseThing();

		tile->ground = Item::CreateItem(groundId);
		tile->ground->pos.x = _x;
		tile->ground->pos.y = _y;
		tile->ground->pos.z = _z;
  }
  else
  {
    tile = new Tile();
		if(groundId != 0 && Item::items[groundId].groundtile) {
			tile->ground = Item::CreateItem(groundId);

			tile->ground->pos.x = _x;
			tile->ground->pos.y = _y;
			tile->ground->pos.z = _z;
		}

		//tileMaps[_x & 0x1F][_y & 0x1F][_z][(_x << 16) | _y] = tile;
		//tileMaps[_x & 0xFF][_y & 0xFF][ _x & 0xFF00) << 8 | (_y & 0xFF00) | _z] = tile;
		tileMaps[_x & 0x7F][_y & 0x7F][ (_x & 0xFF80) << 16 | (_y & 0xFF80) << 1 | _z] = tile;
  } 
}

bool Map::placeCreature(Position &pos, Creature* c)
{
	Tile* tile = getTile(pos.x, pos.y, pos.z);
	bool success = tile && c->canMovedTo(tile);
	//bool success = tile!=NULL;
	if(!success)
	{   
		for(int cx =pos.x - 1; cx <= pos.x + 1 && !success; cx++) {
			for(int cy = pos.y - 1; cy <= pos.y + 1 && !success; cy++){
#ifdef __DEBUG__
				std::cout << "search pos x: " << cx <<" y: "<< cy << std::endl;
#endif

				tile = getTile(cx, cy, pos.z);
				success = tile && c->canMovedTo(tile);

				if(success) {
					pos.x = cx;
					pos.y = cy;
				}
			}
		}

		if(!success){
			Player *player = dynamic_cast<Player*>(c);
			if(player) {
				pos.x = c->masterPos.x;
				pos.y = c->masterPos.y;
				pos.z = c->masterPos.z;

				tile = getTile(pos.x, pos.y, pos.z);
				success = tile && player->canMovedTo(tile);
			}
		}    

	}

	if(!success || !tile) {
#ifdef __DEBUG__
	std::cout << "Failed to place creature onto map!" << std::endl;
#endif
		return false;
	}
    #ifdef __DEBUG__
	std::cout << "POS: " << c->pos << std::endl;
	#endif
	tile->addThing(c);
	c->pos = pos;

	return true;
}

bool Map::removeCreature(Creature* c)
{
	//OTSYS_THREAD_LOCK(mapLock)
	bool ret = true;

	Tile *tile = getTile(c->pos.x, c->pos.y, c->pos.z);
	if(!tile || !tile->removeThing(c))
		return false;

	//OTSYS_THREAD_UNLOCK(mapLock)
	return true;
}

void Map::getSpectators(const Range& range, std::vector<Creature*>& list)
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

	for(int nz = range.minRange.z; nz != range.maxRange.z + range.zstep; nz += range.zstep) {
		offsetz = range.centerpos.z - nz;
		//negative offset means that the player is on a lower floor than ourself

		for (int nx = range.minRange.x + offsetz; nx <= range.maxRange.x + offsetz; ++nx)
		{
			for (int ny = range.minRange.y + offsetz; ny <= range.maxRange.y + offsetz; ++ny)
			{
				tile = getTile(nx + range.centerpos.x, ny + range.centerpos.y, nz);
				if (tile)
				{
					for (cit = tile->creatures.begin(); cit != tile->creatures.end(); ++cit) {
/*
#ifdef __DEBUG__
						std::cout << "Found " << (*cit)->getName() << " at x: " << (*cit)->pos.x << ", y: " << (*cit)->pos.y << ", z: " << (*cit)->pos.z << ", offset: " << offsetz << std::endl;
#endif
*/
						list.push_back((*cit));
					}
				}
			}
		}
	}	
}

bool Map::canThrowItemTo(Position from, Position to, bool creaturesBlock /* = true*/, bool isProjectile /*= false*/)
{
	if(from.x > to.x) {
		swap(from.x, to.x);
		swap(from.y, to.y);
	}

	bool steep = std::abs(to.y - from.y) > abs(to.x - from.x);

	if(steep) {
		swap(from.x, from.y);
		swap(to.x, to.y);
	}
	
	int deltax = abs(to.x - from.x);
	int deltay = abs(to.y - from.y);
	int error = 0;
	int deltaerr = deltay;
	int y = from.y;
	Tile *t = NULL;
	int xstep = ((from.x < to.x) ? 1 : -1);
	int ystep = ((from.y < to.y) ? 1 : -1);

	for(int x = from.x; x != to.x; x += xstep) {
		//cout << "x: " << (steep ? y : x) << ", y: " << (steep ? x : y) << std::endl;
		t = getTile((steep ? y : x), (steep ? x : y), from.z);

		if(t) {
			if(isProjectile) {
				if(t->isBlockingProjectile())
					return false;
			}
			else if(creaturesBlock && !t->creatures.empty())
				return false;
			else if(!(from.x == x && from.y == y) && t->isBlocking())
				return false;
		}

		error += deltaerr;

		if(2 * error >= deltax) {
			y += ystep;
			error -= deltax;
		}
	}

	return true;
}

bool Map::isPathValid(Creature *creature, const std::list<Position>& path, bool ignoreMoveableBlockingItems /*= false)*/)
{
	std::list<Position>::const_iterator iit;
	for(iit = path.begin(); iit != path.end(); ++iit) {

		Tile *t = getTile(iit->x, iit->y, iit->z);		
		if(!t || t->isBlocking(false, ignoreMoveableBlockingItems) || (!t->creatures.empty() && (t->getCreature() != creature || t->creatures.size() > 1)))
			return false;
	}

	return true;
}

std::list<Position> Map::getPathTo(Creature *creature, Position start, Position to, bool creaturesBlock /*=true*/, bool ignoreMoveableBlockingItems /*= false*/){
	std::list<Position> path;
/*	if(start.z != to.z)
		return path;
*/

	std::list<AStarNode*> openNodes;
	std::list<AStarNode*> closedNodes;
	int z = start.z;

	AStarNode* startNode = new AStarNode;
	startNode->parent=NULL;
	startNode->h=0;
	startNode->x=start.x;
	startNode->y=start.y;
	AStarNode* found = NULL;
	openNodes.push_back(startNode);
	while(!found && closedNodes.size() < 100){
		//get best node from open list
		openNodes.sort(lessPointer<AStarNode>());
		if(openNodes.size() == 0)
			return path; //no path
		AStarNode* current = openNodes.front();
		openNodes.pop_front();
		closedNodes.push_back(current);
		for(int dx=-1; dx <= 1; dx++){
			for(int dy=-1; dy <= 1; dy++){
				if(abs(dx) != abs(dy)){
					int x = current->x + dx;
					int y = current->y + dy;

					Tile *t = getTile(x, y, z);
					if(!t || t->isBlocking(false,ignoreMoveableBlockingItems) ||
						(creaturesBlock && !t->creatures.empty() && (t->getCreature() != creature || t->creatures.size() > 1)) ||
							t->floorChange() || t->getTeleportItem())
					continue;

					bool isInClosed = false;
					for(std::list<AStarNode*>::iterator it = closedNodes.begin();
						it != closedNodes.end(); it++){
						AStarNode* n = *it;
						if(n->x == x && n->y == y){
							isInClosed = true;
							break;
						}
					}
					if(isInClosed)
						continue;

					bool isInOpen = false;
					AStarNode* child = NULL;
					for(std::list<AStarNode*>::iterator it = openNodes.begin();
						it != openNodes.end(); it++){
						AStarNode* n = *it;
						if(n->x == x && n->y == y){
							isInOpen = true;
							child = *it;
							break;
						}
					}

					if(!isInOpen){
						AStarNode* n = new AStarNode;
						n->x=x;
						n->y=y;
						n->h = abs(n->x - to.x)*abs(n->x - to.x) + abs(n->y - to.y)*abs(n->y - to.y);
						//n->h = (float)abs(n->x - to.x) + (float)abs(n->y - to.y);
						//n->g = current->g + 1;
						n->parent = current;
						if(n->x == to.x && n->y == to.y){
							found = n;
						}
						openNodes.push_front(n);
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

	for(std::list<AStarNode*>::iterator it = openNodes.begin();
		it != openNodes.end(); it++){
		delete *it;
	}

	for(std::list<AStarNode*>::iterator it = closedNodes.begin();
		it != closedNodes.end(); it++){
		delete *it;
	}
	
	for(std::list<Position>::iterator it = path.begin(); it != path.end(); it++){
		Position p = *it;
	}
	return path;
}

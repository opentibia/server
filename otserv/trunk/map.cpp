//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// the map of OpenTibia
//////////////////////////////////////////////////////////////////////
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundumpion; either version 2
// of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundumpion,
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
#include "iomapxml.h"

#include "otsystem.h"
#include <stdio.h>

#include "items.h"
#include "map.h"
#include "tile.h"

#include "player.h"
#include "tools.h"

#include "networkmessage.h"

#include "npc.h"
#include "spells.h"

#include "luascript.h"
#include <ctype.h>

#define EVENT_CHECKPLAYER          123
#define EVENT_CHECKPLAYERATTACKING 124

extern LuaScript g_config;
extern Spells spells;
extern std::map<long, Creature*> channel;

MapState::MapState(Map* imap)
: map(imap)
{
}


bool MapState::isTileStored(const Tile *t) const
{
	bool ret = false;

	TileExDataMap::const_iterator preChangeItemMapIt;
	for(preChangeItemMapIt = preChangeItemMap.begin(); preChangeItemMapIt != preChangeItemMap.end(); ++preChangeItemMapIt) {
		if(preChangeItemMapIt->first == t) {
			ret = true;
		}
	}

	return ret;
}

void MapState::addThing(Tile *t, Thing *thing)
{
	addThingInternal(t, thing, false);
}

bool MapState::removeThing(Tile *t, Thing *thing)
{
	return removeThingInternal(t, thing, false);
}

bool MapState::removeThingInternal(Tile *t, Thing *thing, bool onlyRegister)
{
	//First change to this tile?
	if(!isTileStored(t)) {
		addTile(t, thing->pos);
	}

	std::vector<tilechangedata>& vec = changesItemMap[t];

	int stackpos = t->getThingStackPos(thing);
	if(onlyRegister || t->removeThing(thing)) {
		tilechangedata tc;
		tc.thing = thing;
		tc.remove = true;
		tc.stackpos = stackpos;

		vec.push_back(tc);
		return true;
	}

	return false;
}

void MapState::addThingInternal(Tile *t, Thing *thing, bool onlyRegister)
{
	//First change to this tile?
	if(!isTileStored(t)) {
		addTile(t, thing->pos);
	}

	std::vector<tilechangedata>& vec = changesItemMap[t];

	if(!onlyRegister && thing != t->splash)
		t->addThing(thing);

	int stackpos = t->getThingStackPos(thing);

	tilechangedata tc;
	tc.thing = thing;
	tc.remove = false;
	tc.stackpos = stackpos;
	vec.push_back(tc);
}

//Basically "fake" remove/add an item to register the change to the client
void MapState::refreshThing(Tile *t, Thing *thing)
{
	removeThingInternal(t, thing, true);
	addThingInternal(t, thing, true);
}

void MapState::getMapChanges(Player *spectator, NetworkMessage &msg)
{
	std::vector<Tile*> tileUpdatedVec; //keep track of tiles that already been updated

	TileExDataMap::const_iterator preChangeItemMapIt;
	for(preChangeItemMapIt = preChangeItemMap.begin(); preChangeItemMapIt != preChangeItemMap.end(); ++preChangeItemMapIt) {
		
		/*
		if(std::find(tileUpdatedVec.begin(), tileUpdatedVec.end(), preChangeItemMap->first) != tileUpdatedVec.end()) {
			//Tile has already been updated
			continue;
		}
		*/

		if(!spectator->CanSee(preChangeItemMapIt->second.pos.x,  preChangeItemMapIt->second.pos.y))
			continue;

		Tile *targettile = map->getTile(preChangeItemMapIt->second.pos.x,  preChangeItemMapIt->second.pos.y, preChangeItemMapIt->second.pos.z);

		if(!targettile)
			continue;

		//if(preChangeItemMapIt->second.thingCount > 9 && tileThingCountMap[targettile] > 0) {
		if(preChangeItemMapIt->second.thingCount > 9 && changesItemMap[targettile].size() > 0) {
			if(std::find(tileUpdatedVec.begin(), tileUpdatedVec.end(), targettile) == tileUpdatedVec.end()) {
				tileUpdatedVec.push_back(targettile);
				((Creature*)spectator)->onTileUpdated(&preChangeItemMapIt->second.pos);
			}

			#if __DEBUG__
			std::cout << "pop-up item" << std::endl;
			#endif
		}
	}

	//Add/remove items
	TileChangeDataVecMap::const_iterator changesItemMapIt;
	for(changesItemMapIt = changesItemMap.begin(); changesItemMapIt != changesItemMap.end(); ++changesItemMapIt) {
		
		if(std::find(tileUpdatedVec.begin(), tileUpdatedVec.end(), changesItemMapIt->first) != tileUpdatedVec.end()) {
			//Tile has already been updated
			continue;
		}

		TileChangeDataVec::const_iterator thIt;
		for(thIt = changesItemMapIt->second.begin(); thIt != changesItemMapIt->second.end(); ++thIt) {
			
			if(!spectator->CanSee(thIt->thing->pos.x,  thIt->thing->pos.y))
				continue;

			if(thIt->remove) {
				if(thIt->stackpos < 10) {
					msg.AddByte(0x6c);
					msg.AddPosition(thIt->thing->pos);
					msg.AddByte(thIt->stackpos);
				}
				else {
					//This will cause some problem, we remove an item (example: a player gets removed due to death) from the map, but the client cant see it
					//(above the 9 limit), real tibia has the same problem so I don't think there is a way to fix this.
					//Problem: The client won't be informed that the player has been killed
					//and will show the player as alive (0 hp).
					//Solution: re-log.
				}
			}
			else {
				Item *item = dynamic_cast<Item*>(thIt->thing);

				if(item) {
					msg.AddByte(0x6a);
					msg.AddPosition(item->pos);
					msg.AddItem(item);
				}
			}
		}
	}
}

void MapState::addTile(Tile *t, Position& tilepos)
{
	if(t) {
		preChangeItemMap[t].pos = tilepos;
		preChangeItemMap[t].thingCount = t->getThingCount();
	}
}

Map::Map()
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

  OTSYS_THREAD_LOCKVARINIT(mapLock);
}


Map::~Map()
{
}


bool Map::loadMap(std::string filename) {

	IOMap* loader = new IOMapXML();
	std::cout << "Loading map from: " << loader->getSourceDescription() << std::endl;
	return loader->loadMap(this, filename);
}

Tile* Map::getTile(unsigned short _x, unsigned short _y, unsigned char _z)
{
  if (_z < MAP_LAYER)
  {
    // _x & 0x3F  is like _x % 64
    TileMap *tm = &tileMaps[_x & 1][_y & 1][_z];

    // search in the stl map for the requested tile
    TileMap::iterator it = tm->find((_x << 16) | _y);

    // ... found
    if (it != tm->end())
      return it->second;
  }
	
	 // or not
  return NULL;
}


void Map::setTile(unsigned short _x, unsigned short _y, unsigned char _z, unsigned short groundId)
{
  Tile *tile = getTile(_x, _y, _z);

  if (tile != NULL)
  {
    tile->ground = groundId;
  }
  else
  {
    tile = new Tile();
    tile->ground = groundId;
    tileMaps[_x & 1][_y & 1][_z][(_x << 16) | _y] = tile;
  }  
}

Position Map::placeCreature(Creature* c){
	Position pos = c->pos;
	/*if (!c->canMovedTo(getTile(pos.x, pos.y, pos.z)))
	{   
		bool found =false;
		for(int cx =pos.x-1; cx <= pos.x+1 && !found; cx++){
			for(int cy = pos.y-1; cy <= pos.y+1 && !found; cy++){
				std::cout << "search pos x:" <<cx <<" y: "<< cy << std::endl;                
				if (c->canMovedTo(getTile(cx, cy, pos.z))){
					pos.x = cx;
					pos.y = cy;
					found = true;
				}
			}
		}
		if(!found){
			pos.x = c->masterPos.x;
			pos.y = c->masterPos.y;
			pos.z = c->masterPos.z;
		}    
	}*/
	Tile* tile=getTile(pos.x, pos.y, pos.z);
	if (!tile){
         pos = Position();
         tile=getTile(pos.x, pos.y, pos.z);
               }
	tile->addThing(c);
	c->pos = pos;

	return pos;
         

}

bool Map::removeCreature(Creature* c)
{
	OTSYS_THREAD_LOCK(mapLock)
	/* int stackpos = */ getTile(c->pos.x, c->pos.y, c->pos.z)->getCreatureStackPos(c);
	getTile(c->pos.x, c->pos.y, c->pos.z)->removeThing(c);

	OTSYS_THREAD_UNLOCK(mapLock)

    return true;
}

void Map::getSpectators(const Range& range, std::vector<Creature*>& list)
{
	CreatureVector::iterator cit;
	for (int z = range.startz; z <= range.endz; ++z) {
		for (int x = range.startx; x <= range.endx; ++x)
		{
			for (int y = range.starty; y <= range.endy; ++y)
			{
				Tile *tile = getTile(x, y, z);
				if (tile)
				{
					for (cit = tile->creatures.begin(); cit != tile->creatures.end(); ++cit) {					
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

		if(t && (isProjectile ? t->isBlockingProjectile() : t->isBlocking()) || (creaturesBlock ? !t->creatures.empty() : false)) {
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


std::list<Position> Map::getPathTo(Position start, Position to, bool creaturesBlock){
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

					Tile *t;
					if((!(t = getTile(x,y,z))) || t->isBlocking() || (t->creatures.size() && x != to.x && y != to.y))
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
						n->h = (float)abs(n->x - to.x) + (float)abs(n->y - to.y);
						n->g = current->g + 1;
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

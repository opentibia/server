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
Map::Map()
{
	//first we fill the map with
  for(int y = 0; y < MAP_HEIGHT; y++)
  {
    for(int x = 0; x < MAP_WIDTH; x++)
    {
      //				setTile(x,y,7,102);
    }
  }


  OTSYS_THREAD_LOCKVARINIT(mapLock);
  OTSYS_THREAD_LOCKVARINIT(eventLock);
  OTSYS_THREAD_SIGNALVARINIT(eventSignal);

  OTSYS_CREATE_THREAD(eventThread, this);
}


Map::~Map()
{
}


bool Map::LoadMap(std::string filename) {

  // get maximum number of players allowed...
  max_players = atoi(g_config.getGlobalString("maxplayers").c_str());
  std::cout << ":: Player Limit: " << max_players << std::endl;

  FILE* f;
  std::cout << ":: Loading Map from " << filename << " ... ";
  f=fopen(filename.c_str(),"r");
  if(f){
    fclose(f);
    loadMapXml(filename.c_str());
    std::cout << "[done]" << std::endl;
    return true;
  }
  else{
    //this code is ugly but works
    //TODO improve this code to support things like
    //a quadtree to speed up everything
#ifdef __DEBUG__
    std::cout << "Loading map" << std::endl;
#endif
    FILE* dump=fopen("otserv.map", "rb");
    if(!dump){
#ifdef __DEBUG__
      std::cout << "Loading old format mapfile failed" << std::endl;
#endif
      exit(1);
    }
    Position topleft, bottomright, now;


    topleft.x=fgetc(dump)*256;	topleft.x+=fgetc(dump);
    topleft.y=fgetc(dump)*256;	topleft.y+=fgetc(dump);
    topleft.z=fgetc(dump);

    bottomright.x=fgetc(dump)*256;	bottomright.x+=fgetc(dump);
    bottomright.y=fgetc(dump)*256;	bottomright.y+=fgetc(dump);
    bottomright.z=fgetc(dump);

    int xsize= bottomright.x-topleft.x;
    int ysize= bottomright.y-topleft.y;
    int xorig=((MAP_WIDTH)-xsize)/2;
    int yorig=((MAP_HEIGHT)-ysize)/2;
    //TODO really place this map patch where it belongs

    for(int y=0; y < ysize; y++){
      for(int x=0; x < xsize; x++){
        while(true)
        {
          int id=fgetc(dump)*256;id+=fgetc(dump);
          if(id==0x00FF)
            break;
          //now.x=x+MINX;now.y=y+MINY;now.z=topleft.z;

          Item *item = new Item(id);
          if (item->isGroundTile())
          {
            setTile(xorig+x, yorig+y, 7, id);
            delete item;
          }
          else
          {
            Tile *t = getTile(xorig+x, yorig+y, 7);
            if (t)
              t->addThing(item);
          }
          //tiles[x][y]->push_back(new Item(id));
        }
      }
    }
    fclose(dump);
  }

  return true;
}



/*****************************************************************************/


OTSYS_THREAD_RETURN Map::eventThread(void *p)
{
  Map* _this = (Map*)p;

  // basically what we do is, look at the first scheduled item,
  // and then sleep until it's due (or if there is none, sleep until we get an event)
  // of course this means we need to get a notification if there are new events added
  while (true)
  {
#ifdef __DEBUG__EVENTSCHEDULER__
    std::cout << "schedulercycle start..." << std::endl;
#endif

    SchedulerTask* task = NULL;

    // check if there are events waiting...
    OTSYS_THREAD_LOCK(_this->eventLock)

      int ret;
    if (_this->eventList.size() == 0) {
      // unlock mutex and wait for signal
      ret = OTSYS_THREAD_WAITSIGNAL(_this->eventSignal, _this->eventLock);
    } else {
      // unlock mutex and wait for signal or timeout
      ret = OTSYS_THREAD_WAITSIGNAL_TIMED(_this->eventSignal, _this->eventLock, _this->eventList.top()->getCycle());
    }
    // the mutex is locked again now...
    if (ret == OTSYS_THREAD_TIMEOUT) {
      // ok we had a timeout, so there has to be an event we have to execute...
#ifdef __DEBUG__EVENTSCHEDULER__
      std::cout << "event found at " << OTSYS_TIME() << " which is to be scheduled at: " << _this->eventList.top()->getCycle() << std::endl;
#endif
      task = _this->eventList.top();
      _this->eventList.pop();
    }

    OTSYS_THREAD_UNLOCK(_this->eventLock);
    if (task) {
      (*task)(_this);
      delete task;
    }
  }

}

void Map::addEvent(SchedulerTask* event) {
  bool do_signal = false;
  OTSYS_THREAD_LOCK(eventLock)

    eventList.push(event);
  if (eventList.empty() || *event < *eventList.top())
    do_signal = true;

  OTSYS_THREAD_UNLOCK(eventLock)

    if (do_signal)
      OTSYS_THREAD_SIGNAL_SEND(eventSignal);

}

/*****************************************************************************/



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



int Map::loadMapXml(const char *filename){
	xmlDocPtr doc;
	xmlNodePtr root, tile, p;
	int width, height;

	xmlLineNumbersDefault(1);
	doc=xmlParseFile(filename);
	if (!doc) {
    std::cout << "FATAL: couldnt load map. exiting" << std::endl;
    exit(1);
  }
	root=xmlDocGetRootElement(doc);
	if(xmlStrcmp(root->name,(const xmlChar*) "map")){
    xmlFreeDoc(doc);
    std::cout << "FATAL: couldnt load map. exiting" << std::endl;
    exit(1);
  }

	width=atoi((const char*)xmlGetProp(root, (const xmlChar *) "width"));
	height=atoi((const char*)xmlGetProp(root, (const xmlChar *) "height"));

	int xorig=((MAP_WIDTH)-width)/2;
	int yorig=((MAP_HEIGHT)-height)/2;
	tile=root->children;
	int numpz = 0;
	for(int y=0; y < height; y++){
	    for(int x=0; x < width; x++){
		    if (!tile) {
		    std::cout << "no tile for " << x << " / " << y << std::endl;
		    exit(1);
	    }
			const char* pz = (const char*)xmlGetProp(tile, (const xmlChar *) "pz");
			p=tile->children;
	
			while(p)
			{
				if(xmlStrcmp(p->name,(const xmlChar*) "item")==0){
					Item* myitem=new Item();
					myitem->unserialize(p);

					if (myitem->isGroundTile())
					{
						setTile(xorig+x, yorig+y, 7, myitem->getID());
						delete myitem;

						if (pz && (strcmp(pz, "1") == 0)) {
							numpz++;
							getTile(xorig+x, yorig+y, 7)->setPz();
						}
				    }
					else
					{
						Tile *t = getTile(xorig+x, yorig+y, 7);
						if (t)
						{
							if (myitem->isAlwaysOnTop())
								t->topItems.push_back(myitem);
							else
								t->downItems.push_back(myitem);
						}
					}

				}
				if(xmlStrcmp(p->name,(const xmlChar*) "npc")==0){
					std::string name = (const char*)xmlGetProp(p, (const xmlChar *) "name");
					Npc* mynpc = new Npc(name.c_str(), this);
					//first we have to set the position of our creature...
					mynpc->pos.x=xorig+x;
					mynpc->pos.y=yorig+y;
					if(!this->placeCreature(mynpc)){
						//tinky winky: "oh oh"
					}
				}
				p=p->next;
			}
			tile=tile->next;
		}
	}
	
  xmlFreeDoc(doc);
	
	return 0;
}



Creature* Map::getCreatureByID(unsigned long id)
{
  std::map<long, Creature*>::iterator i;
  for( i = playersOnline.begin(); i != playersOnline.end(); i++ )
  {
    if((i->second)->getID() == id )
    {
      return i->second;
    }
  }
  return NULL; //just in case the player doesnt exist
}

Creature* Map::getCreatureByName(const char* s)
{
  std::map<long, Creature*>::iterator i;
	std::string txt1 = s;
	std::transform(txt1.begin(), txt1.end(), txt1.begin(), upchar);

  for( i = playersOnline.begin(); i != playersOnline.end(); i++ )
	{
		std::string txt2 = (i->second)->getName();
		std::transform(txt2.begin(), txt2.end(), txt2.begin(), upchar);
		if(txt1 == txt2)
    {
      return i->second;
    }
  }
  return NULL; //just in case the player doesnt exist
}

bool Map::placeCreature(Creature* c)
{
  if (c->access == 0 && playersOnline.size() >= max_players)
    return false;

	OTSYS_THREAD_LOCK(mapLock)

	// add player to the online list
	playersOnline[c->getID()] = c;
	Player* player = dynamic_cast<Player*>(c);
	if (player) {
		player->usePlayer();
	}
	std::cout << (uint32_t)playersOnline.size() << " players online." << std::endl;

	addEvent(makeTask(1000, std::bind2nd(std::mem_fun(&Map::checkPlayer), c->id)));
	addEvent(makeTask(2000, std::bind2nd(std::mem_fun(&Map::checkPlayerAttacking), c->id)));

	if (!c->canMovedTo(getTile(c->pos.x, c->pos.y, c->pos.z)))
  {   
      bool found =false;
      for(int cx =c->pos.x-1; cx <= c->pos.x+1 && !found; cx++){
                    for(int cy = c->pos.y-1; cy <= c->pos.y+1 && !found; cy++){
                        std::cout << "search pos x:" <<cx <<" y: "<< cy << std::endl;                
						if (c->canMovedTo(getTile(cx, cy, c->pos.z))){
                                            c->pos.x = cx;
                                            c->pos.y = cy;
                                            found = true;
                                        }
                                        // crap we need to find another spot
                                        
                                    }
                                }
      if(!found){
          c->pos.x = c->masterPos.x;
          c->pos.y = c->masterPos.y;
          c->pos.z = c->masterPos.z;
      }    
	}

	Tile* tile=getTile(c->pos.x, c->pos.y, c->pos.z);
	if(!tile){
		this->setTile(c->pos.x, c->pos.y, c->pos.z, 0);
		tile=getTile(c->pos.x, c->pos.y, c->pos.z);
	}
  tile->addThing(c);

	std::vector<Creature*> list;
	getSpectators(Range(c->pos, true), list);

	for(int i = 0; i < list.size(); ++i)
	{
		list[i]->onCreatureAppear(c);
	}

	OTSYS_THREAD_UNLOCK(mapLock)

    return true;
}

bool Map::removeCreature(Creature* c)
{
  OTSYS_THREAD_LOCK(mapLock)
    //removeCreature from the online list

    std::map<long, Creature*>::iterator pit = playersOnline.find(c->getID());
  if (pit != playersOnline.end()) {
    playersOnline.erase(pit);


#ifdef __DEBUG__
    std::cout << "removing creature "<< std::endl;
#endif

    int stackpos = getTile(c->pos.x, c->pos.y, c->pos.z)->getCreatureStackPos(c);
    getTile(c->pos.x, c->pos.y, c->pos.z)->removeThing(c);
		
		std::vector<Creature*> list;
		getSpectators(Range(c->pos, true), list);

		for(int i = 0; i < list.size(); ++i)
		{
			list[i]->onCreatureDisappear(c, stackpos);
		}
  }

  std::cout << playersOnline.size() << " players online." << std::endl;

  Player* player = dynamic_cast<Player*>(c);

  if (player){
    std::string charName = c->getName();
    player->savePlayer(charName);                    
    player->releasePlayer();
  }

   OTSYS_THREAD_UNLOCK(mapLock)

    return true;
}

void Map::thingMove(Creature *player, Thing *thing,
                    unsigned short to_x, unsigned short to_y, unsigned char to_z)
{
  OTSYS_THREAD_LOCK(mapLock)
  Tile *fromTile = getTile(thing->pos.x, thing->pos.y, thing->pos.z);

  if (fromTile)
  {
    int oldstackpos = fromTile->getThingStackPos(thing);

    thingMoveInternal(player, thing->pos.x, thing->pos.y, thing->pos.z, oldstackpos, to_x, to_y, to_z);
  }

   OTSYS_THREAD_UNLOCK(mapLock)
}


void Map::thingMove(Creature *player,
                    unsigned short from_x, unsigned short from_y, unsigned char from_z,
                    unsigned char stackPos,
                    unsigned short to_x, unsigned short to_y, unsigned char to_z)
{
  OTSYS_THREAD_LOCK(mapLock)
  
	thingMoveInternal(player, from_x, from_y, from_z, stackPos, to_x, to_y, to_z);
  
	OTSYS_THREAD_UNLOCK(mapLock)
}

void Map::thingMoveInternal(Creature *player,
                    unsigned short from_x, unsigned short from_y, unsigned char from_z,
                    unsigned char stackPos,
                    unsigned short to_x, unsigned short to_y, unsigned char to_z)
{
	Thing *thing = NULL;
  Tile *fromTile = NULL;
  Tile *toTile   = NULL;
	Item* fromContainer = NULL;
	Item* toContainer = NULL;

	if(from_x == 0xFFFF && to_x == 0xFFFF) {
		Player* p = dynamic_cast<Player*>(player);
		if(!p)
			return;

		unsigned char from_id = from_y & 0x0F;

		if(0x40 & to_y) {
			unsigned char to_id = to_y & 0x0F;
			toContainer = p->getContainer(to_id);
		}
		else
			toContainer = p->items[to_y];

		fromContainer = p->getContainer(from_id);

		if(!(fromContainer && toContainer) || !(fromContainer->isContainer() && toContainer->isContainer()) || 
			from_z >= fromContainer->getContainerItemCount())
			return;

		Item* item = fromContainer->getItem(from_z);
		if(!item)
			return;

		if(0x40 & to_y) {
			Item *toSlot = toContainer->getItem(to_z);

			if(toSlot && toSlot->isContainer()) {
				toContainer = toSlot;
			}
		}

		bool isItemHolding = false;
		item->isContainerHolding(toContainer, isItemHolding);
		if(isItemHolding || toContainer == item || fromContainer == item /*|| item == toSlot*/) {
      player->sendCancel("This is impossible.");
			return ;
		}

		//move around an item in a container
		if(fromContainer == toContainer) {
			fromContainer->moveItem(from_z, to_z);
		}
		//move around an item between different containers
		else {
			fromContainer->removeItem(item);
			toContainer->addItem(item);
		}

		thing = item;
		
		/*
		Item* container = NULL;
		for(int i = 0; i < p->getContainerCount(); i++) {
			container  = p->getContainer(i);
			if(container && container == fromContainer) {
				player->onContainerUpdated(item, i, (toContainer == fromContainer ? i : 0xFF), from_z, to_z, true);
			}

			if(container && container == toContainer && toContainer != fromContainer) {
				player->onContainerUpdated(item, 0xFF, i, from_z, to_z, false);
			}
		}
		*/
		//return;
	}
	else if(from_x == 0xFFFF && to_x != 0xFFFF) {
		/*
		Player* p = dynamic_cast<Player*>(player);
		if(!p)
			return;

		toTile = getTile(to_x, to_y, to_z);

		if(0x40 & from_y) {
			unsigned char from_id = from_y & 0x0F;
			fromContainer = p->getContainer(from_id);
			if(!fromContainer || !fromContainer->isContainer())
				return;
			
			thing = fromContainer->getItem(from_z);
		}
		else {
			thing = p->items[from_y];
			fromContainer = p->items[from_y];
		}
		*/
		return;
	}
	else if(from_x != 0xFFFF && to_x == 0xFFFF) {
		/*
		Player* p = dynamic_cast<Player*>(player);
		if(!p)
			return;

		fromTile = getTile(from_x, from_y, from_z);
		thing = fromTile->getThingByStackPos(stackPos);

		if(0x40 & to_y) {
			unsigned char to_id = to_y & 0x0F;
			toContainer = p->getContainer(to_id);
			if(!toContainer || !toContainer->isContainer())
				return;

			Item *toSlot = toContainer->getItem(to_z);

			if(toSlot && toSlot->isContainer()) {
				toContainer = toSlot;
			}
		}
		else {
			toContainer = p->items[to_y];
		}
		*/
		return;
	}
	else {
		thing = getTile(from_x, from_y, from_z)->getThingByStackPos(stackPos);
    fromTile = getTile(from_x, from_y, from_z);
    toTile   = getTile(to_x, to_y, to_z);
	}

#ifdef __DEBUG__
								std::cout << "moving"
				/*
				<< ": from_x: "<< (int)from_x << ", from_y: "<< (int)from_y << ", from_z: "<< (int)from_z
				<< ", stackpos: "<< (int)stackPos
				<< ", to_x: "<< (int)to_x << ", to_y: "<< (int)to_y << ", to_z: "<< (int)to_z
				*/
				<< std::endl;
#endif

  if (thing)
  {
		Creature* creature = dynamic_cast<Creature*>(thing);
		Item* item = dynamic_cast<Item*>(thing);
    if (player->access == 0 && creature && creature->access != 0) {
      player->sendCancel("Better dont touch him...");
      return;
    }
    Player* playerMoving = dynamic_cast<Player*>(creature);
    Position oldPos;
    oldPos.x = from_x;
    oldPos.y = from_y;
    oldPos.z = from_z;


    if ((fromTile != NULL || fromContainer) && (toTile != NULL || toContainer))
    {
      if (fromTile && ((abs(from_x - player->pos.x) > 1) ||
          (abs(from_y - player->pos.y) > 1)))
      {
        player->sendCancel("To far away...");
      }
			else if ((abs((fromContainer ? player->pos.x : oldPos.x) - (toContainer ? player->pos.x : to_x)) > thing->throwRange) ||
               (abs((fromContainer ? player->pos.y : oldPos.y) - (toContainer ? player->pos.y : to_y)) > thing->throwRange))
      {
        player->sendCancel("Not there...");
      }
			else if(!canThrowItemTo((fromContainer ? player->pos : Position(from_x, from_y, from_z)),
															(toContainer ? player->pos : Position(to_x, to_y, to_z)), false)) {
				player->sendCancel("You cannot throw there.");
			}
			else if (toTile && !thing->canMovedTo(toTile))
      {
        if (player == thing)
          player->sendCancelWalk("Sorry, not possible...");
        else
          player->sendCancel("Sorry, not possible...");
      }
      else if (playerMoving && toTile->isPz() && playerMoving->pzLocked) {
          if (player == thing && player->pzLocked)
            player->sendCancelWalk("You can't enter a protection zone after attacking another creature.");
          else if (playerMoving->pzLocked)
            player->sendCancel("Sorry, not possible...");
      }
      else if (playerMoving && fromTile->isPz() && player != thing) {
            player->sendCancel("Sorry, not possible...");
      } 
			else if (fromTile && fromTile->splash == thing && fromTile->splash->isNotMoveable()) {
				player->sendCancel("You cannot move this object.");
#ifdef __DEBUG__
				cout << player->Creature::getName() << " is trying to move a splash item!" << std::endl;
#endif
			}
			else if (item && item->isNotMoveable()) {
				player->sendCancel("You cannot move this object.");
#ifdef __DEBUG__
				cout << player->Creature::getName() << " is trying to move an unmoveable item!" << std::endl;
#endif
			}
      else
      {
				if(fromContainer || toContainer) {
					Player* p = dynamic_cast<Player*>(player);
						if(!p)
							return;

					thing->pos.x = to_x;
					thing->pos.y = to_y;
					thing->pos.z = to_z;

					//Throw equipment item on the floor
					if(toTile && !(fromTile && toContainer) && (fromContainer == thing))
					{
						/*
						p->items[from_y] = NULL;
						toTile->addThing(thing);

						creatureBroadcastTileUpdated(thing->pos);
						player->sendInventory();
						*/
						return;
					}
					//Drop item on floor
					else if(toTile && fromContainer && !toContainer)
					{
						fromContainer->removeItem(item);
						toTile->addThing(thing);

						for(int i = 0; i < p->getContainerCount(); i++) {
							if(p->getContainer(i) == fromContainer) {
								player->onContainerUpdated(item, i, 0xFF, from_z, 0xFF, true);
							}
						}

						creatureBroadcastTileUpdated(thing->pos);
						return;
					}
					//Pickup equipment item from the floor
					else if(fromTile && !(toTile && fromContainer) && !(0x40 & to_y) && p->items[to_y] == toContainer && !toContainer->isContainer())
					{
						/*
						NetworkMessage msg;
						if(p->items[to_y] != NULL) {
							fromTile->removeThing(thing);

							Thing* fromThing = p->items[to_y];
							fromThing->pos.x = from_x;
							fromThing->pos.y = from_y;
							fromThing->pos.z = from_z;
							fromTile->addThing(fromThing);

							p->items[to_y] = NULL;
							msg.AddPlayerInventoryItem(p, to_y);
							msg.AddByte(0x6A);
							msg.AddPosition(fromThing->pos);
							//Item item = Item(target->lookcorpse);
							//msg.AddItem(&item);
						}

						p->items[to_y] = item;
						creatureBroadcastTileUpdated(Position(from_x, from_y, from_z));
						//player->sendInventory();
						*/
						return;
					}
					//Pickup item from the floor to container
					else if(fromTile && toContainer && !fromContainer)
					{
						if(fromTile->removeThing(thing)) {
							toContainer->addItem(item);
						
							/*
							for(int i = 0; i < p->getContainerCount(); i++) {
								if(p->getContainer(i) == toContainer) {
									player->onContainerUpdated(item, 0xFF, i, 0xFF, 0, false);
								}
							}
							
							creatureBroadcastTileUpdated(Position(from_x, from_y, from_z));
							*/
						}
						return;
					}
					else if(toContainer && fromContainer) {
						Item* container = NULL;
						for(int i = 0; i < p->getContainerCount(); i++) {
							container  = p->getContainer(i);
							if(container && container == fromContainer) {
								player->onContainerUpdated(item, i, (toContainer == fromContainer ? i : 0xFF), from_z, to_z, true);
							}

							if(container && container == toContainer && toContainer != fromContainer) {
								player->onContainerUpdated(item, 0xFF, i, from_z, to_z, false);
							}
						}
						return;
					}
					else
						return;

					/*
					if(fromContainer && toTile) {
						fromContainer->removeItem(item);
						toTile->addThing(thing);
					}
					else if(fromTile && toContainer) {
						std::vector<Creature*> list;
						getSpectators(thing->pos, list, &Range(thing->pos.x - 1, thing->pos.x + 1, 
																									thing->pos.y - 1, thing->pos.y + 1));
						for(int i = 0; i < list.size(); i++) {
							
							Player* spec = dynamic_cast<Player*>(list[i]);
							if(!spec)
								continue;
							
							if(spec != player) {
								unsigned char containerid =  spec->getContainerID(toContainer);
								//if(containerid != 0xFF)
							 	//	spec->onContainerClosed(containerid);
							}
						}

						if(fromTile->removeThing(thing))
							toContainer->addItem(item);

					}

					Item* container = NULL;
					for(int i = 0; i < p->getContainerCount(); i++) {
						container  = p->getContainer(i);
						if(container && container == fromContainer) {
							player->onContainerUpdated(item, i, (toContainer == fromContainer ? i : 0xFF), from_z, to_z, true);
						}

						if(container && container == toContainer && toContainer != fromContainer) {
							player->onContainerUpdated(item, 0xFF, i, from_z, to_z, false);
						}
					}

					creatureBroadcastTileUpdated((fromTile ? Position(from_x, from_y, from_z) : Position(to_x, to_y, to_z)));

					thing->pos.x = to_x;
          thing->pos.y = to_y;
          thing->pos.z = to_z;

					return;
					*/
				}

        int oldstackpos = fromTile->getThingStackPos(thing);
        if (fromTile && fromTile->removeThing(thing))
        {
					toTile->addThing(thing);

          thing->pos.x = to_x;
          thing->pos.y = to_y;
          thing->pos.z = to_z;
					
					if (creature) {
            // we need to update the direction the player is facing to...
            // otherwise we are facing some problems in turning into the
            // direction we were facing before the movement
            // check y first cuz after a diagonal move we lock to east or west
            if (to_y < oldPos.y) ((Player*)thing)->direction = NORTH;
            if (to_y > oldPos.y) ((Player*)thing)->direction = SOUTH;
            if (to_x > oldPos.x) ((Player*)thing)->direction = EAST;
            if (to_x < oldPos.x) ((Player*)thing)->direction = WEST;
            
            if(playerMoving && creature->attackedCreature != 0){
             Creature* c = getCreatureByID(creature->attackedCreature);
             if(c){      
             if((std::abs(creature->pos.x-c->pos.x) > 8) ||
				(std::abs(creature->pos.y-c->pos.y) > 5) ||
				(creature->pos.z != c->pos.z)){                      
             playerMoving->sendCancelAttacking();
             }
             }
            }
          }

					std::vector<Creature*> list;
					getSpectators(Range(min(oldPos.x, (int)to_x) - 9, max(oldPos.x, (int)to_x) + 9,
														  min(oldPos.y, (int)to_y) - 7, max(oldPos.y, (int)to_y) + 7, oldPos.z, true), list);

					for(int i = 0; i < list.size(); ++i)
					{
						list[i]->onThingMove(player, thing, &oldPos, oldstackpos);
					}

					if(fromTile->getThingCount() > 8) {
						cout << "Pop-up item from below..." << std::endl;

						//We need to pop up this item
						Thing *newthing = fromTile->getThingByStackPos(9);

						if(newthing != NULL) {
							creatureBroadcastTileUpdated(newthing->pos /*&oldPos*/);
						}
					}
				}
			}
    }
  }
}

void Map::getSpectators(const Range& range, std::vector<Creature*>& list)
{
	CreatureVector::iterator cit;
	//for (int z = range.startz; z <= range.endz; z++) {
		for (int x = range.startx; x <= range.endx; x++)
		{
			for (int y = range.starty; y <= range.endy; y++)
			{
				Tile *tile = getTile(x, y, range.endz);
				if (tile)
				{
					for (cit = tile->creatures.begin(); cit != tile->creatures.end(); cit++) {					
						list.push_back((*cit));
					}
				}
			}
		}
	//}
}

void Map::creatureBroadcastTileUpdated(const Position& pos)
{
	std::vector<Creature*> list;
	getSpectators(Range(pos, true), list);

	for(int i = 0; i < list.size(); ++i)
	{
		list[i]->onTileUpdated(&pos);
	}
}

void Map::creatureTurn(Creature *creature, Direction dir)
{
	OTSYS_THREAD_LOCK(mapLock)

    if (creature->direction != dir)
    {
      creature->direction = dir;

      int stackpos = getTile(creature->pos.x, creature->pos.y, creature->pos.z)->getThingStackPos(creature);

			std::vector<Creature*> list;
			getSpectators(Range(creature->pos, true), list);

			for(int i = 0; i < list.size(); ++i)
			{
				list[i]->onCreatureTurn(creature, stackpos);
			}
    }

   OTSYS_THREAD_UNLOCK(mapLock)
}


void Map::creatureSay(Creature *creature, unsigned char type, const std::string &text)
{
  OTSYS_THREAD_LOCK(mapLock)
	// First, check if this was a GM command
	if(text[0] == '/' && creature->access > 0)
	{
		// Get the command
		switch(text[1])
		{
			default:break;
			// Summon?
			case 's':
			{
				// Create a non-const copy of the command
				std::string cmd = text;
				// Erase the first 2 bytes
				cmd.erase(0,3);
				// The string contains the name of the NPC we want.
				Npc *npc = new Npc(cmd.c_str(), (Map *)this);
				if(!npc->isLoaded()){
					delete npc;
					break;
				}
				// Set the NPC pos
				if(creature->direction == NORTH)
				{
					npc->pos.x = creature->pos.x;
					npc->pos.y = creature->pos.y - 1;
					npc->pos.z = creature->pos.z;
				}
				// South
				if(creature->direction == SOUTH)
				{
					npc->pos.x = creature->pos.x;
					npc->pos.y = creature->pos.y + 1;
					npc->pos.z = creature->pos.z;
				}
				// East
				if(creature->direction == EAST)
				{
					npc->pos.x = creature->pos.x + 1;
					npc->pos.y = creature->pos.y;
					npc->pos.z = creature->pos.z;
				}
				// West
				if(creature->direction == WEST)
				{
					npc->pos.x = creature->pos.x - 1;
					npc->pos.y = creature->pos.y;
					npc->pos.z = creature->pos.z;
				}
				// Place the npc
				placeCreature(npc);
			} break; // case 's':
		}
	}

	// It was no command, or it was just a player
	else {
		std::vector<Creature*> list;
		getSpectators(Range(creature->pos), list);

		for(int i = 0; i < list.size(); ++i)
		{
			list[i]->onCreatureSay(creature, type, text);
		}

    /*
		CreatureVector::iterator cit;

		for (int x = creature->pos.x - 8; x <= creature->pos.x + 8; x++)
			for (int y = creature->pos.y - 6; y <= creature->pos.y + 6; y++)
			{
				Tile *tile = getTile(x, y, 7);
				if (tile)
				{
					for (cit = tile->creatures.begin(); cit != tile->creatures.end(); cit++)
					{
						(*cit)->onCreatureSay(creature, type, text);
					}
				}
			}
			*/
	}


	OTSYS_THREAD_UNLOCK(mapLock)
}


void Map::creatureChangeOutfit(Creature *creature)
{
  OTSYS_THREAD_LOCK(mapLock)

	std::vector<Creature*> list;
	getSpectators(Range(creature->pos, true), list);

	for(int i = 0; i < list.size(); ++i)
	{
		list[i]->onCreatureChangeOutfit(creature);
	}

  OTSYS_THREAD_UNLOCK(mapLock)
}

void Map::creatureWhisper(Creature *creature, const std::string &text)
{
  OTSYS_THREAD_LOCK(mapLock)

	std::vector<Creature*> list;
	getSpectators(Range(creature->pos), list);

	for(int i = 0; i < list.size(); ++i)
	{
		if(abs(creature->pos.x - list[i]->pos.x) > 1 || abs(creature->pos.y - list[i]->pos.y) > 1)
			list[i]->onCreatureSay(creature, 2, std::string("pspsps"));
		else
			list[i]->onCreatureSay(creature, 2, text);
	}

	/*
  CreatureVector::iterator cit;
  for (int x = creature->pos.x - 8; x <= creature->pos.x + 8; x++)
    for (int y = creature->pos.y - 6; y <= creature->pos.y + 6; y++)
    {
      Tile *tile = getTile(x, y, 7);
      if (tile)
      {
        for (cit = tile->creatures.begin(); cit != tile->creatures.end(); cit++)
        {
            if(abs(creature->pos.x - x) >1 || abs(creature->pos.y - y) >1)
                      (*cit)->onCreatureSay(creature, 2, std::string("pspsps"));
            else (*cit)->onCreatureSay(creature, 2, text);
        }
      }
    }
	*/
  OTSYS_THREAD_UNLOCK(mapLock)
}

void Map::creatureYell(Creature *creature, std::string &text)
{
  OTSYS_THREAD_LOCK(mapLock)

		Player* player = dynamic_cast<Player*>(creature);
  if(player && player->access == 0 && player->exhaustedTicks >=1000) {
      player->exhaustedTicks += (long)g_config.getGlobalNumber("exhaustedadd", 0);
      NetworkMessage msg;
      msg.AddTextMessage(MSG_SMALLINFO, "You are exhausted.");
      player->sendNetworkMessage(&msg);
  }
  else {
      creature->exhaustedTicks = (long)g_config.getGlobalNumber("exhausted", 0);
      std::transform(text.begin(), text.end(), text.begin(), upchar);

			std::vector<Creature*> list;
			getSpectators(Range(creature->pos.x - 18, creature->pos.x + 18,
										 		  creature->pos.y - 14, creature->pos.y + 14,
													max(creature->pos.z - 3, 0), min(creature->pos.z + 3, MAP_LAYER - 1)), list);

			for(int i = 0; i < list.size(); ++i)
			{
				list[i]->onCreatureSay(creature, 3, text);
			}
  }    
  
	OTSYS_THREAD_UNLOCK(mapLock)
}


void Map::creatureSpeakTo(Creature *creature, const std::string &receiver, const std::string &text)
{
  OTSYS_THREAD_LOCK(mapLock) 
  Creature* c = getCreatureByName(receiver.c_str());
  if(c)
  c->onCreatureSay(creature, 4, text);
  OTSYS_THREAD_UNLOCK(mapLock)
}


void Map::creatureBroadcastMessage(Creature *creature, const std::string &text)
{
  if(creature->access == 0) 
	  return;

  OTSYS_THREAD_LOCK(mapLock)

	std::map<long, Creature*>::iterator cit;
  for (cit = playersOnline.begin(); cit != playersOnline.end(); cit++)
  {
		cit->second->onCreatureSay(creature, 9, text);
	}

	OTSYS_THREAD_UNLOCK(mapLock)
}

void Map::creatureToChannel(Creature *creature, unsigned char type, const std::string &text, unsigned short channelId)
{

  OTSYS_THREAD_LOCK(mapLock)

	std::map<long, Creature*>::iterator cit;
  for (cit = channel.begin(); cit != channel.end(); cit++)
  {
        Player* player = dynamic_cast<Player*>(cit->second);
        if(player)
		player->sendToChannel(creature, type, text, channelId);
	}

	OTSYS_THREAD_UNLOCK(mapLock)
}

void Map::getAreaTiles(Position pos, const unsigned char area[14][18], unsigned char dir, std::list<tiletargetdata>& list)
{
	tiletargetdata tt;

	Position tpos = pos;
	tpos.x -= 8;
	tpos.y -= 6;

	for(int y = 0; y < 14; y++) {
		for(int x = 0; x < 18; x++) {
			if(area[y][x] == dir) {
				Tile *t = getTile(tpos.x, tpos.y, tpos.z);
				if(t && !t->isBlocking() && canThrowItemTo(pos, tpos, false)) {
					tt.pos = tpos;
					tt.targetCount = t->creatures.size();
					tt.thingCount = t->getThingCount();
					list.push_back(tt);
				}
			}
			tpos.x += 1;
		}
		
		tpos.x -= 18;
		tpos.y += 1;
	}
}

bool Map::creatureMakeMagic(Creature *creature, const MagicEffectClass* me)
{
	//const MagicSpellConditionClass* magicCondition = dynamic_cast<const MagicSpellConditionClass*>(me);
	const MagicEffectInstantSpellClass* magicInstant = dynamic_cast<const MagicEffectInstantSpellClass*>(me);
	const MagicEffectRuneClass* magicRune = dynamic_cast<const MagicEffectRuneClass*>(me);
	const MagicEffectAreaClass* magicArea = dynamic_cast<const MagicEffectAreaClass*>(me);
	const MagicEffectGroundClass* magicGround = dynamic_cast<const MagicEffectGroundClass*>(me);

	if(/*!magicCondition &&*/ me->offensive && !creatureOnPrepareAttack(creature, me->centerpos))
		return false;

	if(!((std::abs(creature->pos.x-me->centerpos.x) <= 8) && (std::abs(creature->pos.y-me->centerpos.y) <= 6) &&
		(creature->pos.z == me->centerpos.z)))
		return false;

#ifdef __DEBUG__
	cout << "creatureMakeMagic: " << creature->getName() << ", x: " << me->centerpos.x << ", y: " << me->centerpos.y << ", z: " << me->centerpos.z << std::endl;
#endif

	Player* player = dynamic_cast<Player*>(creature);
	if(player) {
		if(player->access == 0) {
			if(/*!magicCondition &&*/ player->exhaustedTicks >= 1000) {
				NetworkMessage msg;
				msg.AddMagicEffect(player->pos, NM_ME_PUFF);
				msg.AddTextMessage(MSG_SMALLINFO, "You are exhausted.");
				player->sendNetworkMessage(&msg);
				player->exhaustedTicks += (long)g_config.getGlobalNumber("exhaustedadd", 0);
				return false;
			}
			else if(magicInstant) {
				if(player->mana < magicInstant->manaCost) {
					NetworkMessage msg;
					msg.AddMagicEffect(player->pos, NM_ME_PUFF);
					msg.AddTextMessage(MSG_SMALLINFO, "You do not have enough mana.");
					player->sendNetworkMessage(&msg);
					return false;
				}
				else
					player->mana -= magicInstant->manaCost;
					
					//for debug only:
					//cout << player->getName() << " casted a spell that costed " << magicInstant->manaCost << " mana.\n";
					
					//for magic level advances
					player->manaspent += magicInstant->manaCost;
			}
		}
	}
	
	if(magicGround) {
		Tile *groundtile = getTile(me->centerpos.x, me->centerpos.y, me->centerpos.z);
		if(!groundtile)
			return false;

		if(Item::items[magicGround->groundID].blocking && (groundtile->isBlocking() || !groundtile->creatures.empty())) {
			if(player) {
				if(!groundtile->creatures.empty()) {
					player->sendCancel("There is not enough room.");

					NetworkMessage msg;
					msg.AddMagicEffect(player->pos, NM_ME_PUFF);
					player->sendNetworkMessage(&msg);
				}
				else {
					player->sendCancel("You cannot throw there.");
				}
			}

			return false;
		}
	}
	
	if(player && player->access == 0 && (magicArea || getTile(me->centerpos.x, me->centerpos.y, me->centerpos.z)->creatures.size() > 0)) {
		if(me->offensive && creature->pos != me->centerpos)
			player->pzLocked = true;

		creature->exhaustedTicks = (long)g_config.getGlobalNumber("exhausted", 0);
	}

	if (creature->pos != me->centerpos && me->offensive && player && player->access == 0) {
	    player->inFightTicks = (long)g_config.getGlobalNumber("pzlocked", 0);
	    player->sendIcons();
	}

	std::list<tiletargetdata> tilelist;
	std::list<tiletargetdata>::const_iterator tl;

	if(magicArea || magicInstant) {
		getAreaTiles(magicArea->centerpos, magicArea->area, magicArea->direction, tilelist);
	}
	else {
		Tile *t = getTile(me->centerpos.x, me->centerpos.y, me->centerpos.z);
		if(t) {
			tiletargetdata tt;
			tt.pos = me->centerpos;
			tt.targetCount = t->creatures.size();
			tt.thingCount = t->getThingCount();
			tilelist.push_back(tt);
		}
	}

	std::vector<Creature*> spectatorlist;
	getSpectators(Range(min(creature->pos.x, me->centerpos.x) - 14, max(creature->pos.x, me->centerpos.x) + 14,
											min(creature->pos.y, me->centerpos.y) - 11, max(creature->pos.y, me->centerpos.y) + 11,
											creature->pos.z), spectatorlist);

	Tile *attackertile = getTile(creature->pos.x,  creature->pos.y, creature->pos.z);
	Tile *targettile = NULL;

	CreatureVector::iterator cit;
	typedef std::pair<Creature*, struct targetdata> targetitem;
	std::vector<targetitem> targetvec;
	std::vector<targetitem>::const_iterator tv;
	targetitem ti;

	//Apply the permanent effect to the map
	for(tl = tilelist.begin(); tl != tilelist.end(); tl++) {
		targettile = getTile(tl->pos.x,  tl->pos.y, tl->pos.z);

		if(!targettile)
			continue;
		
		if(creature->access == 0 && targettile->isPz())
			continue;

		for(cit = targettile->creatures.begin(); cit != targettile->creatures.end(); cit++) {
			Creature* target = (*cit);
			Player* targetPlayer = dynamic_cast<Player*>(target);

			int damage = 0;
			int manaDamage = 0;

			if(!me->offensive || (target != creature || magicGround)) {
				if(target->access == 0)
					damage = random_range(me->minDamage, me->maxDamage);

				if(!me->offensive)
					damage = -damage;

				if(creature->access != 0 && target->access == 0)
					damage += (me->offensive ? 1337 : -1337);

				if (damage > 0) {
					if(targetPlayer && me->offensive && target->access ==0){
						targetPlayer->inFightTicks = (long)g_config.getGlobalNumber("pzlocked", 0);
						targetPlayer->sendIcons();
					}
						
					if (target->manaShieldTicks >= 1000 && (damage < target->mana) ){
						manaDamage = damage;
						damage = 0;
					}
					else if (target->manaShieldTicks >= 1000 && (damage > target->mana) ){
						manaDamage = target->mana;
						damage -= manaDamage;
					}
					else if((target->manaShieldTicks < 1000) && (damage > target->health))
						damage = target->health;
					else if (target->manaShieldTicks >= 1000 && (damage > (target->health + target->mana))){
						damage = target->health;
						manaDamage = target->mana;
					}

					if(target->manaShieldTicks < 1000)
						target->drainHealth(damage);
					else if(manaDamage >0){
						target->drainHealth(damage);
						target->drainMana(manaDamage);
					}
					else
						target->drainMana(damage);
				} else {
					int newhealth = target->health - damage;
					if(newhealth > target->healthmax)
						newhealth = target->healthmax;
						
					target->health = newhealth;

					damage = target->health - newhealth;
					manaDamage = 0;
				}
			}

			ti.first = target;
			ti.second.damage = damage;
			ti.second.manaDamage = manaDamage;
			ti.second.stackpos = targettile->getCreatureStackPos(target);
			ti.second.hadSplash = targettile->splash != NULL;
			
			targetvec.push_back(ti);
		}

		if(magicGround) {
			//if(!targettile->isBlocking()) {
			Item* item = new Item(magicGround->groundID);
			item->pos = tl->pos;
			targettile->addThing(item);

			unsigned short decayTime = Item::items[magicGround->groundID].decayTime;
			addEvent(makeTask(decayTime * 1000, std::bind2nd(std::mem_fun(&Map::decayItem), item)));
			//}
		}
	}
	
	//Remove player from tile, add bodies/blood to the map
	std::map<Tile*, int> tileBodyCountMap; //optimization to save bandwidth
	for(tv = targetvec.begin(); tv != targetvec.end(); tv++) {
		Creature* target = tv->first;
		Player* targetPlayer = dynamic_cast<Player*>(target);
		Tile *targettile = getTile(target->pos.x, target->pos.y, target->pos.z);

		if(me->physical && tv->second.damage > 0) {
			if (!targettile->splash)
			{
				Item *item = new Item(1437, 2);
				item->pos = target->pos;
				targettile->splash = item;

				unsigned short decayTime = Item::items[1437].decayTime;
				targettile->decaySplashAfter = OTSYS_TIME() + decayTime*1000;
				addEvent(makeTask(decayTime*1000, std::bind2nd(std::mem_fun(&Map::decaySplash), targettile->splash)));
			}
		}

		if (target->health <= 0) {
			if(tv->second.stackpos < 10) {
				if(tileBodyCountMap.find(targettile) == tileBodyCountMap.end())
					tileBodyCountMap[targettile] = 1;
				else {
					tileBodyCountMap[targettile]++;
				}
			}			
			
			playersOnline.erase(playersOnline.find(target->getID()));
			targettile->removeThing(target);
			NetworkMessage msg;
			creature->experience += (int)(target->experience * 0.1);
			if(player){
        msg.AddPlayerStats(player);
			  player->sendNetworkMessage(&msg);
      }
			Item *item = new Item(target->lookcorpse);
			/*
			if(item->isContainer() && targetPlayer) {
				item->addItem(targetPlayer->items[SLOT_BACKPACK]);
				targetPlayer->items[SLOT_BACKPACK] = NULL;
			}
			*/

			item->pos = target->pos;
			targettile->addThing(item);

			unsigned short decayTime = Item::items[item->getID()].decayTime;
			addEvent(makeTask(decayTime*1000, std::bind2nd(std::mem_fun(&Map::decayItem), item)));
		}
	}

	std::vector<Tile*> tileBloodVec; //keep track of the blood splashes (only 1 msg / tile / spectator 
	std::vector<Tile*> tileUpdatedVec;

	NetworkMessage msg;
	//Create a network message for each spectator
	for(int i = 0; i < spectatorlist.size(); ++i) {
		Player* spectator = dynamic_cast<Player*>(spectatorlist[i]);
		
		if(!spectator)
			continue;

		msg.Reset();
		tileBloodVec.clear();
		tileUpdatedVec.clear();

		if((!magicRune || targetvec.size() > 0) && me->animationEffect > 0 && (spectator->CanSee(player->pos.x, player->pos.y) || spectator->CanSee(me->centerpos.x, me->centerpos.y)))
			msg.AddDistanceShoot(creature->pos, me->centerpos, me->animationEffect);

		for(tl = tilelist.begin(); tl != tilelist.end(); tl++) {
			Tile *targettile = getTile(tl->pos.x,  tl->pos.y, tl->pos.z);

			if(!targettile)
				continue;

			if(tl->thingCount > 9 && tileBodyCountMap[targettile] > 0) {
				if(std::find(tileUpdatedVec.begin(), tileUpdatedVec.end(), targettile) == tileUpdatedVec.end()) {
					tileUpdatedVec.push_back(targettile);
					spectatorlist[i]->onTileUpdated(&tl->pos);
				}

#if __DEBUG__
				std::cout << "pop-up item" << std::endl;
#endif
			}

			if(magicGround && spectator->CanSee(tl->pos.x, tl->pos.y)) {
				if(std::find(tileUpdatedVec.begin(), tileUpdatedVec.end(), targettile) == tileUpdatedVec.end()) {

					msg.AddByte(0x6a);
					msg.AddPosition(tl->pos);
					Item item = Item(magicGround->groundID);
					msg.AddItem(&item);
				}

				if(magicGround->maxDamage == 0) {
					continue;
				}
			}
			if(tl->targetCount == 0 && spectator->CanSee(tl->pos.x, tl->pos.y)) {
				if(magicArea && (creature->access != 0 || !targettile->isPz())) {
					if(magicArea->areaEffect != 0xFF)
						msg.AddMagicEffect(tl->pos, magicArea->areaEffect);
				}
				else if(magicRune)
					msg.AddMagicEffect(creature->pos, NM_ME_PUFF);
			}
		}

		for(tv = targetvec.begin(); tv != targetvec.end(); tv++) {
			Creature *target = tv->first;
			Tile *targettile = getTile(target->pos.x, target->pos.y, target->pos.z);
			int damage = tv->second.damage;
			int manaDamage = tv->second.manaDamage;
			unsigned char targetstackpos = tv->second.stackpos;
			bool hadSplash = tv->second.hadSplash;

			if(spectator->CanSee(target->pos.x, target->pos.y))
			{
				if(me->physical && damage > 0)
					msg.AddMagicEffect(target->pos, NM_ME_DRAW_BLOOD);
					
				msg.AddMagicEffect(target->pos, me->damageEffect);

				if(damage != 0) {
					std::stringstream dmg;
					dmg << std::abs(damage);
					msg.AddAnimatedText(target->pos, me->animationcolor, dmg.str());
				}

				if(manaDamage > 0){
					msg.AddMagicEffect(target->pos, NM_ME_LOOSE_ENERGY);
					std::stringstream manaDmg;
					manaDmg << std::abs(manaDamage);
					msg.AddAnimatedText(target->pos, 2, manaDmg.str());
				}

				if (target->health <= 0)
				{            
          std::stringstream exp;
          exp << (int)(target->experience * 0.1);
          msg.AddAnimatedText(creature->pos, 983, exp.str());               

					if(std::find(tileUpdatedVec.begin(), tileUpdatedVec.end(), targettile) == tileUpdatedVec.end()) {
#if __DEBUG__
						std::cout << "remove character " << "targetstackpos: "<< (int) targetstackpos << std::endl;
#endif
						if(targetstackpos < 10) {
							// remove character
							msg.AddByte(0x6c);
							msg.AddPosition(target->pos);
							msg.AddByte(targetstackpos);
							msg.AddByte(0x6a);
							msg.AddPosition(target->pos);
							Item item = Item(target->lookcorpse);
							msg.AddItem(&item);
						}
					}
				}
				else {
					msg.AddCreatureHealth(target);
        }

				if(me->physical && damage > 0)
				{
					if(std::find(tileUpdatedVec.begin(), tileUpdatedVec.end(), targettile) == tileUpdatedVec.end()) {
						if(std::find(tileBloodVec.begin(), tileBloodVec.end(), targettile) == tileBloodVec.end()) {
							tileBloodVec.push_back(targettile);

							// fresh blood, first remove od
							if (hadSplash)
							{
								msg.AddByte(0x6c);
								msg.AddPosition(target->pos);
								msg.AddByte(1);
								targettile->splash->setID(1437);
							}

							msg.AddByte(0x6a);
							msg.AddPosition(target->pos);
							Item item = Item(1437, 2);
							msg.AddItem(&item);
						}
					}
				}

				if (spectator == target){
					CreateManaDamageUpdate(target, creature, manaDamage, msg);
					CreateDamageUpdate(target, creature, damage, msg);
				}
			}
		}

		spectator->sendNetworkMessage(&msg);
	}

	return true;
}

void Map::creatureCastSpell(Creature *creature, const MagicEffectClass& me) {
  OTSYS_THREAD_LOCK(mapLock)

	creatureMakeMagic(creature, &me);

	OTSYS_THREAD_UNLOCK(mapLock)
}


//We assume we are on the same floor
bool Map::canThrowItemTo(Position from, Position to, bool creaturesBlock /* = true*/)
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
		if(steep)
			t = getTile(y, x, from.z);
		else
			t = getTile(x, y, from.z);

		//cout << "x: " << (steep ? y : x) << ", y: " << (steep ? x : y) << std::endl;

		if(t && t->isBlocking() || (creaturesBlock ? !t->creatures.empty() : false)) {
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

bool Map::creatureThrowRune(Creature *creature, const MagicEffectClass& me) {
  OTSYS_THREAD_LOCK(mapLock)

	bool ret = false;

	if(creature->pos.z != me.centerpos.z) {
		creature->sendCancel("You need to be on the same floor.");
	}
	else if(!canThrowItemTo(creature->pos, me.centerpos, false)) {
		creature->sendCancel("You cannot throw there.");
	}
	else
		ret = creatureMakeMagic(creature, &me);

	OTSYS_THREAD_UNLOCK(mapLock)

	return ret;
}

bool Map::creatureOnPrepareAttack(Creature *creature, Position pos)
{
    if(creature){ 
	Player* player = dynamic_cast<Player*>(creature);

	Tile* tile = (Tile*)getTile(creature->pos.x, creature->pos.y, creature->pos.z);
	Tile* targettile = getTile(pos.x, pos.y, pos.z);

	if(creature->access == 0) {
		if(tile && tile->isPz()) {
			if(player) {
				NetworkMessage msg;
				msg.AddTextMessage(MSG_SMALLINFO, "You may not attack a person while your in a protection zone.");
				player->sendNetworkMessage(&msg);
				player->sendCancelAttacking();
			}

			return false;
		}
		else if(targettile && targettile->isPz()) {
			if(player) {
				NetworkMessage msg;
				msg.AddTextMessage(MSG_SMALLINFO, "You may not attack a person in a protection zone.");
				player->sendNetworkMessage(&msg);
				player->sendCancelAttacking();
			}

			return false;
		}
	}

	return true;
    }
    return false;
}

void Map::creatureMakeDamage(Creature *creature, Creature *attackedCreature, fight_t damagetype)
{
	if(!creatureOnPrepareAttack(creature, attackedCreature->pos))
		return;
	
	Player* player = dynamic_cast<Player*>(creature);
	Player* attackedPlayer = dynamic_cast<Player*>(attackedCreature);

	Tile* tile = getTile(creature->pos.x, creature->pos.y, creature->pos.z);
	Tile* targettile = getTile(attackedCreature->pos.x, attackedCreature->pos.y, attackedCreature->pos.z);

	NetworkMessage msg;
	//can the attacker reach the attacked?
	bool inReach = false;

	switch(damagetype){
		case FIGHT_MELEE:
			if((std::abs(creature->pos.x-attackedCreature->pos.x) <= 1) &&
				(std::abs(creature->pos.y-attackedCreature->pos.y) <= 1) &&
				(creature->pos.z == attackedCreature->pos.z))
					inReach = true;
		break;
		case FIGHT_DIST:
			if((std::abs(creature->pos.x-attackedCreature->pos.x) <= 8) &&
				(std::abs(creature->pos.y-attackedCreature->pos.y) <= 5) &&
				(creature->pos.z == attackedCreature->pos.z))
					inReach = true;
		break;
		case FIGHT_MAGICDIST:
			if((std::abs(creature->pos.x-attackedCreature->pos.x) <= 8) &&
				(std::abs(creature->pos.y-attackedCreature->pos.y) <= 5) &&
				(creature->pos.z == attackedCreature->pos.z))
					inReach = true;	
			break;
	}	
					
	if (player && player->access == 0) {
	    player->inFightTicks = (long)g_config.getGlobalNumber("pzlocked", 0);
	    player->sendIcons();
	    if(attackedPlayer)
 	         player->pzLocked = true;	    
	}
	if(attackedPlayer && attackedPlayer->access ==0){
	 attackedPlayer->inFightTicks = (long)g_config.getGlobalNumber("pzlocked", 0);
	 attackedPlayer->sendIcons();
  }
    if(attackedCreature->access != 0){
        if(player)
        player->sendCancelAttacking();
        return;
         }
	if(!inReach){         
		return;
    }
	int damage = creature->getWeaponDamage();
	int manaDamage = 0;

	if (creature->access != 0)
		damage += 1337;

	if (damage < -50 || attackedCreature->access != 0)
		damage = 0;
	if (attackedCreature->manaShieldTicks <1000 && damage > 0)
		attackedCreature->drainHealth(damage);
	else if (attackedCreature->manaShieldTicks >= 1000 && damage < attackedCreature->mana){
         manaDamage = damage;
         damage = 0;
         attackedCreature->drainMana(manaDamage);
         }
    else if(attackedCreature->manaShieldTicks >= 1000 && damage > attackedCreature->mana){
         manaDamage = attackedCreature->mana;
         damage -= manaDamage;
         attackedCreature->drainHealth(damage);
         attackedCreature->drainMana(manaDamage);
         }
	else
		attackedCreature->health += min(-damage, attackedCreature->healthmax - attackedCreature->health);

	std::vector<Creature*> list;
	getSpectators(Range(min(creature->pos.x, attackedCreature->pos.x) - 9,
										  max(creature->pos.x, attackedCreature->pos.x) + 9,
											min(creature->pos.y, attackedCreature->pos.y) - 7,
											max(creature->pos.y, attackedCreature->pos.y) + 7, creature->pos.z), list);

	for(int i = 0; i < list.size(); ++i)
	{
		Player* p = dynamic_cast<Player*>(list[i]);

		if (p) {
			msg.Reset();
			if(damagetype == FIGHT_DIST)
				msg.AddDistanceShoot(creature->pos, attackedCreature->pos, NM_ANI_POWERBOLT);
			if(damagetype == FIGHT_MAGICDIST)
				msg.AddDistanceShoot(creature->pos, attackedCreature->pos, NM_ANI_ENERGY);

			if (attackedCreature->manaShieldTicks < 1000 && (damage == 0) && (p->CanSee(attackedCreature->pos.x, attackedCreature->pos.y))) {
				msg.AddMagicEffect(attackedCreature->pos, NM_ME_PUFF);
			}
			else if (attackedCreature->manaShieldTicks < 1000 && (damage < 0) && (p->CanSee(attackedCreature->pos.x, attackedCreature->pos.y)))
			{
				msg.AddMagicEffect(attackedCreature->pos, NM_ME_BLOCKHIT);
			}
			else
			{
				if (p->CanSee(attackedCreature->pos.x, attackedCreature->pos.y))
				{
					std::stringstream dmg, manaDmg;
					dmg << std::abs(damage);
					manaDmg << std::abs(manaDamage);
					
					if(damage > 0){
					msg.AddAnimatedText(attackedCreature->pos, 0xB4, dmg.str());
					msg.AddMagicEffect(attackedCreature->pos, NM_ME_DRAW_BLOOD);
													}
					if(manaDamage >0){
													msg.AddMagicEffect(attackedCreature->pos, NM_ME_LOOSE_ENERGY);
													msg.AddAnimatedText(attackedCreature->pos, 2, manaDmg.str());
													}

					if (attackedCreature->health <= 0)
					{
                        std::stringstream exp;
                        exp << (int)(attackedCreature->experience * 0.1);
                        msg.AddAnimatedText(creature->pos, 983, exp.str());                         
						// remove character
						msg.AddByte(0x6c);
						msg.AddPosition(attackedCreature->pos);
						msg.AddByte(targettile->getThingStackPos(attackedCreature));
						msg.AddByte(0x6a);
						msg.AddPosition(attackedCreature->pos);
						Item item = Item(attackedCreature->lookcorpse);
						msg.AddItem(&item);
					}
					else
					{
						msg.AddCreatureHealth(attackedCreature);
					}
					if(damage > 0){				
					// fresh blood, first remove od
					if (targettile->splash)
					{
						msg.AddByte(0x6c);
						msg.AddPosition(attackedCreature->pos);
						msg.AddByte(1);
						targettile->splash->setID(1437);
					}
					msg.AddByte(0x6a);
					msg.AddPosition(attackedCreature->pos);
					Item item = Item(1437, 2);
					msg.AddItem(&item);
					}
				}
			}

			if (p == attackedCreature){
				CreateManaDamageUpdate(p, creature, manaDamage, msg);
				CreateDamageUpdate(p, creature, damage, msg);
			}
	
			p->sendNetworkMessage(&msg);
		}
	}
	
	if(damage > 0){
		if (!targettile->splash)
		{
			Item *item = new Item(1437, 2);
			item->pos = attackedCreature->pos;
			targettile->splash = item;
		}
        
		unsigned short decayTime = Item::items[1437].decayTime;
		targettile->decaySplashAfter = OTSYS_TIME() + decayTime*1000;
		addEvent(makeTask(decayTime*1000, std::bind2nd(std::mem_fun(&Map::decaySplash), targettile->splash)));
	}
   if(player && (damage > 0 || manaDamage >0)){
        player->addSkillTry(1);
        }
   else if(player)
   player->addSkillTry(1);
   
   
	if (attackedCreature->health <= 0) {
		targettile->removeThing(attackedCreature);
		playersOnline.erase(playersOnline.find(attackedCreature->getID()));
		NetworkMessage msg;
        creature->experience += (int)(attackedCreature->experience * 0.1);
        if(player){
             msg.AddPlayerStats(player);           
			 player->sendNetworkMessage(&msg);
            }
    Item *item = new Item(attackedCreature->lookcorpse);
    item->pos = attackedCreature->pos;
		targettile->addThing(item);

		unsigned short decayTime = Item::items[item->getID()].decayTime;
    addEvent(makeTask(decayTime*1000, std::bind2nd(std::mem_fun(&Map::decayItem), item)));
	}
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
						n->h = abs(n->x - to.x) + abs(n->y - to.y);
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



void Map::checkPlayer(unsigned long id)
{
  OTSYS_THREAD_LOCK(mapLock)
  Creature *creature = getCreatureByID(id);

  if (creature != NULL)
  {
	 creature->onThink();

	 Player* player = dynamic_cast<Player*>(creature);
	 if(player){
		 addEvent(makeTask(1000, std::bind2nd(std::mem_fun(&Map::checkPlayer), id)));

		 player->mana += min(10, player->manamax - player->mana);
		 NetworkMessage msg;
		 int requiredExp = player->getExpForLv(player->level+1);
		 
		  if (player->experience >= requiredExp)
          {
          int lastLv = player->level;

          player->level += 1;
          player->healthmax = player->healthmax+player->HPGain[player->voc];
          player->health = player->health+player->HPGain[player->voc];
          player->manamax = player->manamax+player->ManaGain[player->voc];
          player->mana = player->mana+player->ManaGain[player->voc];
          player->cap = player->cap+player->CapGain[player->voc];
          player->setNormalSpeed();
          changeSpeed(player->getID(), player->getSpeed());
          std::stringstream lvMsg;
          lvMsg << "You advanced from level " << lastLv << " to level " << player->level << ".";
          msg.AddTextMessage(MSG_ADVANCE, lvMsg.str().c_str());
          }
		 
		 msg.AddPlayerStats(player);
		 player->sendNetworkMessage(&msg);


         //Magic Level Advance
         unsigned long int reqMana = (unsigned long int) ( 400 * pow(player->ManaMultiplier[player->voc], player->maglevel) );       //will calculate mana for _NEXT_ magic level

         if (reqMana % 20 < 10)
              reqMana = reqMana - (reqMana % 20);
         else reqMana = reqMana - (reqMana % 20) + 20;


         if (player->manaspent >= reqMana) {
            player->manaspent -= reqMana;
            player->maglevel++;
            
            std::stringstream MaglvMsg;
            MaglvMsg << "You advanced from magic level " << (player->maglevel - 1) << " to magic level " << player->maglevel << ".";
            msg.AddTextMessage(MSG_ADVANCE, MaglvMsg.str().c_str());
            player->sendNetworkMessage(&msg);
         }
         //End Magic Level Advance


		 if(player->inFightTicks >= 1000) {
			player->inFightTicks -= 1000;
            if(player->inFightTicks < 1000)
				player->pzLocked = false;
                player->sendIcons(); 
          }
          if(player->exhaustedTicks >=1000){
            player->exhaustedTicks -=1000;
            } 
          if(player->manaShieldTicks >=1000){
            player->manaShieldTicks -=1000;
            if(player->manaShieldTicks  < 1000)
            player->sendIcons();
            }
            if(player->hasteTicks >=1000){
            player->hasteTicks -=1000;
            }    
	 }
	 else{
		 addEvent(makeTask(300, std::bind2nd(std::mem_fun(&Map::checkPlayer), id)));
		 if(creature->manaShieldTicks >=1000){
         creature->manaShieldTicks -=300;
         }
         if(creature->hasteTicks >=1000){
            creature->hasteTicks -=300;
            }  
	 }
  }
   OTSYS_THREAD_UNLOCK(mapLock)
}

void Map::changeOutfit(unsigned long id, int looktype){
     OTSYS_THREAD_LOCK(mapLock)
     
     Creature *creature = getCreatureByID(id);
     if(creature){
     creature->looktype = looktype;
     creatureChangeOutfit(creature);
     }
     
     OTSYS_THREAD_UNLOCK(mapLock)
     }

void Map::changeOutfitAfter(unsigned long id, int looktype, long time){

     addEvent(makeTask(time, 
     boost::bind(
     &Map::changeOutfit, this,
     id, looktype)));
     
}

void Map::changeSpeed(unsigned long id, unsigned short speed)
{
	Creature *creature = getCreatureByID(id);
	if(creature){
		if(creature->hasteTicks >= 1000 || creature->speed == speed){
			return;
		}
	
		creature->speed = speed;
		Player* player = dynamic_cast<Player*>(creature);
		if(player){
			player->sendChangeSpeed(creature);
			player->sendIcons();
		}

		std::vector<Creature*> list;
		getSpectators(Range(creature->pos), list);

		/*
		CreatureVector::iterator cit;
		for (int x = creature->pos.x - 8; x <= creature->pos.x + 8; x++)
			for (int y = creature->pos.y - 6; y <= creature->pos.y + 6; y++)
			{
				Tile *tile = getTile(x, y, 7);
				if (tile)
				{
					for (cit = tile->creatures.begin(); cit != tile->creatures.end(); cit++)*/

		for(int i = 0; i < list.size(); i++)
		{
			Player* p = dynamic_cast<Player*>(list[i]);
			if(p)
				p->sendChangeSpeed(creature);
		}
	}
}


void Map::checkPlayerAttacking(unsigned long id)
{
	OTSYS_THREAD_LOCK(mapLock)

  Creature *creature = getCreatureByID(id);
  if (creature != NULL && creature->health > 0)
  {
    addEvent(makeTask(2000, std::bind2nd(std::mem_fun(&Map::checkPlayerAttacking), id)));

	  if (creature->attackedCreature != 0)
    {
      Creature *attackedCreature = getCreatureByID(creature->attackedCreature);
      if (attackedCreature)
      {
	      Tile* fromtile = getTile(creature->pos.x, creature->pos.y, creature->pos.z);
        if (!attackedCreature->isAttackable() == 0 && fromtile->isPz() && creature->access == 0)
        {
					Player* player = dynamic_cast<Player*>(creature);
          if (player) {
	          NetworkMessage msg;
            msg.AddTextMessage(MSG_STATUS, "You may not attack a person in a protection zone.");
            player->sendNetworkMessage(&msg);
            player->sendCancelAttacking();
          }
        }
        else
        {
          if (attackedCreature != NULL && attackedCreature->health > 0)
          {
	          this->creatureMakeDamage(creature, attackedCreature, creature->getFightType());
          }
        }
      }
	  }
	}

   OTSYS_THREAD_UNLOCK(mapLock)
}

void Map::decayItem(Item* item)
{
  OTSYS_THREAD_LOCK(mapLock)

	if(item) {
		Tile *t = getTile(item->pos.x, item->pos.y, item->pos.z);
		unsigned short decayTo   = Item::items[item->getID()].decayTo;
		unsigned short decayTime = Item::items[item->getID()].decayTime;

		if (decayTo == 0)
		{
			t->removeThing(item);
		}
		else
		{
			item->setID(decayTo);
			addEvent(makeTask(decayTime*1000, std::bind2nd(std::mem_fun(&Map::decayItem), item)));
		}

		creatureBroadcastTileUpdated(item->pos);

		if (decayTo == 0)
			delete item;
	}
  
	OTSYS_THREAD_UNLOCK(mapLock)
}


void Map::decaySplash(Item* item)
{
  OTSYS_THREAD_LOCK(mapLock)

	if (item) {
		Tile *t = getTile(item->pos.x, item->pos.y, item->pos.z);

		if ((t) && (t->decaySplashAfter <= OTSYS_TIME()))
		{
			unsigned short decayTo   = Item::items[item->getID()].decayTo;
			unsigned short decayTime = Item::items[item->getID()].decayTime;

			if (decayTo == 0)
			{
				t->splash = NULL;
			}
			else
			{
				item->setID(decayTo);
				t->decaySplashAfter = OTSYS_TIME() + decayTime*1000;
				addEvent(makeTask(decayTime*1000, std::bind2nd(std::mem_fun(&Map::decaySplash), item)));
			}
			
			creatureBroadcastTileUpdated(item->pos);

			if (decayTo == 0)
				delete item;
		}
	}
  
	OTSYS_THREAD_UNLOCK(mapLock)
}


void Map::CreateDamageUpdate(Creature* creature, Creature* attackCreature, int damage, NetworkMessage& msg)
{
			Player* player = dynamic_cast<Player*>(creature);
			if(!player)
				return;
			msg.AddPlayerStats(player);
			if (damage > 0) {
				std::stringstream dmgmesg;

				if(damage == 1)
				dmgmesg << "You lose 1 hitpoint due to an attack by ";
				else
				dmgmesg << "You lose " << damage << " hitpoints due to an attack by ";
				
				dmgmesg << attackCreature->getName();

				msg.AddTextMessage(MSG_EVENT, dmgmesg.str().c_str());
			}
			if (player->health <= 0){
                msg.AddTextMessage(MSG_ADVANCE, "You are dead.");             
				msg.AddTextMessage(MSG_EVENT, "Own3d!");
                
                int reqExp =  player->getExpForLv(player->level);
                if(player->experience < reqExp)
                {
                std::stringstream lvMsg;
                lvMsg << "You were downgraded from level " << player->level << " to level " << player->level-1 << ".";
                msg.AddTextMessage(MSG_ADVANCE, lvMsg.str().c_str());
                
                
                /* Magic Level downgrade
                for (int i = 0, i <= player->level, i++) {
                    //
                }
                
                End Magic Level downgrade*/
                }
            }
}

void Map::CreateManaDamageUpdate(Creature* creature, Creature* attackCreature, int damage, NetworkMessage& msg)
{
	Player* player = dynamic_cast<Player*>(creature);
	if(!player)
		return;

	msg.AddPlayerStats(player);
	if (damage > 0) {
		std::stringstream dmgmesg;
		dmgmesg << "You lose " << damage << " mana blocking an attack by ";	
		dmgmesg << attackCreature->getName();
		msg.AddTextMessage(MSG_EVENT, dmgmesg.str().c_str());
	}
}

bool Map::creatureSaySpell(Creature *creature, const std::string &text)
{
  OTSYS_THREAD_LOCK(mapLock)
	bool ret = false;

	Player* player = dynamic_cast<Player*>(creature);
	std::string temp, var;
  unsigned int loc = text.find( "\"", 0 );
  if( loc != string::npos && loc >= 0){
      temp = std::string(text, 0, loc-1);
      var = std::string(text, (loc+1), text.size()-loc-1);
	}
  else {
    temp = text;
    var = std::string(""); 
	}
  
	if(creature->access != 0 || !player){
		std::map<std::string, Spell*>::iterator sit = spells.getAllSpells()->find(temp);
		if( sit != spells.getAllSpells()->end() ) {
				sit->second->getSpellScript()->castSpell(creature, var);
				ret = true;
		}
	}
  else if(player){
		std::map<std::string, Spell*>* tmp = spells.getVocSpells(player->voc);
		if(tmp){
			std::map<std::string, Spell*>::iterator sit = tmp->find(temp);
			if( sit != tmp->end() ) {
				if(player->maglevel >= sit->second->getMagLv()){
					sit->second->getSpellScript()->castSpell(creature, var);
					ret = true;
				}
			}
		}
	}

	OTSYS_THREAD_UNLOCK(mapLock)
	return ret;
}

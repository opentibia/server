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
    if( (i->second)->getID() == id )
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
      for(int cx =c->pos.x-3; cx <= c->pos.x+3 && !found; cx++){
                    for(int cy = c->pos.y-3; cy <= c->pos.y+3 && !found; cy++){
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

  CreatureVector::iterator cit;
  for (int x = c->pos.x - 9; x <= c->pos.x + 9; x++)
    for (int y = c->pos.y - 7; y <= c->pos.y + 7; y++)
    {
      Tile *tile = getTile(x, y, 7);
      if (tile)
      {
        for (cit = tile->creatures.begin(); cit != tile->creatures.end(); cit++)
        {
          (*cit)->onCreatureAppear(c);
        }
      }
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

    CreatureVector::iterator cit;
    for (int x = c->pos.x - 9; x <= c->pos.x + 9; x++)
      for (int y = c->pos.y - 7; y <= c->pos.y + 7; y++)
      {
        Tile *tile = getTile(x, y, 7);
        if (tile)
        {
          for (cit = tile->creatures.begin(); cit != tile->creatures.end(); cit++)
          {
            (*cit)->onCreatureDisappear(c, stackpos);
          }
        }
      }

  }

  std::cout << playersOnline.size() << " players online." << std::endl;

	Player* player = dynamic_cast<Player*>(c);
  if (player)
    player->releasePlayer();

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
	if(from_x == 0xFFFF && to_x == 0xFFFF) {
		Player* p = dynamic_cast<Player*>(player);
		if(!p)
			return;

		unsigned char from_id = from_y & 0x0F;
		unsigned char to_id = to_y & 0x0F;

		Item* from_container = p->getContainer(from_id);
		Item* to_container = p->getContainer(to_id);

		if(!(from_container && to_container) || !(from_container->isContainer() && to_container->isContainer()) || 
			from_z >= from_container->getContainerItemCount())
			return;

		Item* item = from_container->getItem(from_z);
		Item *to_slot_item = to_container->getItem(to_z);

		if(!item)
			return;

		if(to_slot_item && to_slot_item->isContainer()) {
			to_container = to_slot_item;
		}

		bool isItemHolding = false;
		item->isContainerHolding(to_container, isItemHolding);
		if(isItemHolding || to_container == item || from_container == item || item == to_slot_item) {
      player->sendCancel("This is impossible.");
			return ;
		}

		//move around an item in a container
		if(from_container == to_container) {
			from_container->moveItem(from_z, to_z);
		}
		//move around an item between different containers
		else {
			from_container->removeItem(item);
			to_container->addItem(item);
		}
		
		Item* container = NULL;
		for(int i = 0; i < p->getContainerCount(); i++) {
			container  = p->getContainer(i);
			if(container && container == from_container) {
				player->onContainerUpdated(item, i, (to_container == from_container ? i : 0xFF), from_z, to_z, true);
			}

			if(container && container == to_container && to_container != from_container) {
				player->onContainerUpdated(item, 0xFF, i, from_z, to_z, false);
			}
		}

		return;
	}
	else if(from_x == 0xFFFF && to_x != 0xFFFF) {
		return;
	}
	else if(from_x != 0xFFFF && to_x == 0xFFFF) {
		return;
	}

#ifdef __DEBUG__
				/*				std::cout << "moving"

				<< ": from_x: "<< (int)from_x << ", from_y: "<< (int)from_y << ", from_z: "<< (int)from_z
				<< ", stackpos: "<< (int)stackPos
				<< ", to_x: "<< (int)to_x << ", to_y: "<< (int)to_y << ", to_z: "<< (int)to_z
				
				<< std::endl;*/
#endif

	Thing *thing = getTile(from_x, from_y, from_z)->getThingByStackPos(stackPos);

  if (thing)
  {
		Creature* creature = dynamic_cast<Creature*>(thing);
		Item* item = dynamic_cast<Item*>(thing); //testing
    if (player->access == 0 && creature && creature->access != 0) {
      player->sendCancel("Better dont touch him...");
      return;
    }
    Player* playerMoving = dynamic_cast<Player*>(creature);
    Position oldPos;
    oldPos.x = from_x;
    oldPos.y = from_y;
    oldPos.z = from_z;

    Tile *fromTile = getTile(from_x, from_y, from_z);
    Tile *toTile   = getTile(to_x, to_y, to_z);

    if ((fromTile != NULL) && (toTile != NULL))
    {
      if ((abs(from_x - player->pos.x) > 1) ||
          (abs(from_y - player->pos.y) > 1))
      {
        player->sendCancel("To far away...");
      }
      else if ((abs(oldPos.x - to_x) > thing->throwRange) ||
               (abs(oldPos.y - to_y) > thing->throwRange))
      {
        player->sendCancel("Not there...");
      }
			else if (!thing->canMovedTo(toTile) || (player == thing && toTile->ground.getID () == 475))
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
			else if (fromTile->splash == thing && fromTile->splash->isNotMoveable()) {
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
        int oldstackpos = fromTile->getThingStackPos(thing);

        if (fromTile->removeThing(thing))
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
          }

          CreatureVector::iterator cit;
          for (int x = min(oldPos.x, (int)to_x) - 9; x <= max(oldPos.x, (int)to_x) + 9; x++)
            for (int y = min(oldPos.y, (int)to_y) - 7; y <= max(oldPos.y, (int)to_y) + 7; y++)
            {
              Tile *tile = getTile(x, y, 7);
              if (tile)
              {
				   
                for (cit = tile->creatures.begin(); cit != tile->creatures.end(); cit++)
                {
                  (*cit)->onThingMove(player, thing, &oldPos, oldstackpos);
                }
              }
						}

						if(fromTile->getThingCount() > 8) {
							cout << "Pop-up item from below..." << std::endl;

							//We need to pop up this item
							Thing *newthing = fromTile->getThingByStackPos(9);

							if(newthing != NULL) {
								CreatureVector::iterator cit;
								for (int x = newthing->pos.x - 9; x <= newthing->pos.x + 9; x++)
									for (int y = newthing->pos.y - 7; y <= newthing->pos.y + 7; y++)
									{
										Tile *tile = getTile(x, y, newthing->pos.z);
										if (tile)
										{
											for (cit = tile->creatures.begin(); cit != tile->creatures.end(); cit++)
											{
												(*cit)->onTileUpdated(&oldPos);
											}
										}
									}
							}
						}
				}
			}
    }
  }
}



void Map::creatureTurn(Creature *creature, Direction dir)
{
	OTSYS_THREAD_LOCK(mapLock)

    if (creature->direction != dir)
    {
      creature->direction = dir;

      int stackpos = getTile(creature->pos.x, creature->pos.y, creature->pos.z)->getThingStackPos(creature);

      CreatureVector::iterator cit;
      for (int x = creature->pos.x - 9; x <= creature->pos.x + 9; x++)
        for (int y = creature->pos.y - 7; y <= creature->pos.y + 7; y++)
        {
          Tile *tile = getTile(x, y, 7);
          if (tile)
          {
            for (cit = tile->creatures.begin(); cit != tile->creatures.end(); cit++)
            {
              (*cit)->onCreatureTurn(creature, stackpos);
            }
          }
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
	}

	OTSYS_THREAD_UNLOCK(mapLock)
}


void Map::creatureChangeOutfit(Creature *creature)
{
  OTSYS_THREAD_LOCK(mapLock)
  CreatureVector::iterator cit;
  for (int x = creature->pos.x - 9; x <= creature->pos.x + 9; x++)
    for (int y = creature->pos.y - 7; y <= creature->pos.y + 7; y++)
    {
      Tile *tile = getTile(x, y, 7);
      if (tile)
      {
        for (cit = tile->creatures.begin(); cit != tile->creatures.end(); cit++)
        {
          (*cit)->onCreatureChangeOutfit(creature);
        }
      }
    }

   OTSYS_THREAD_UNLOCK(mapLock)
}

void Map::creatureWhisper(Creature *creature, const std::string &text)
{
  OTSYS_THREAD_LOCK(mapLock)
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
      CreatureVector::iterator cit;
      std::transform(text.begin(), text.end(), text.begin(), upchar);

			for (int x = creature->pos.x - 18; x <= creature->pos.x + 18; x++)
				for (int y = creature->pos.y - 14; y <= creature->pos.y + 14; y++)
				{
					Tile *tile = getTile(x, y, 7);
					if (tile)
					{
						for (cit = tile->creatures.begin(); cit != tile->creatures.end(); cit++)
						{
							(*cit)->onCreatureSay(creature, 3, text);
						}
					}
				}
  }    
   OTSYS_THREAD_UNLOCK(mapLock)
}


void Map::creatureSpeakTo(Creature *creature, const std::string &receiver, const std::string &text)
{
  OTSYS_THREAD_LOCK(mapLock) 
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

void Map::creatureMakeAreaEffect(Creature *spectator, Creature *attacker, const EffectInfo &ei, NetworkMessage& msg)
{
	CreatureVector::iterator cit;

	Player* pattacker = dynamic_cast<Player*>(attacker);
	Player* pspectator = dynamic_cast<Player*>(spectator);
	Position pos = ei.centerpos;
	bool distshootadded = false;

	pos.x -= 8;
	pos.y -= 6;

	for(int y = 0; y < 14; y++) {
		for(int x = 0; x < 18; x++) {
			Tile* tile = getTile(pos.x, pos.y, pos.z);

			if (tile && pspectator && pspectator->CanSee(pos.x, pos.y)) {
				if(tile->creatures.empty())
				{
					if(ei.area[y][x] == ei.direction) {
						if(ei.needtarget) {
							msg.AddMagicEffect(attacker->pos, NM_ME_PUFF);
							return;
						}
					else
						if((!tile->isPz() || !ei.offensive || attacker->access != 0) && ei.areaEffect > 0) {
							if(!distshootadded && !ei.needtarget) {
								msg.AddDistanceShoot(attacker->pos, ei.centerpos, ei.animationEffect);
								distshootadded = true;
		
							}

							msg.AddMagicEffect(pos, ei.areaEffect);
						}
					}
				}
				else 
				for (cit = tile->creatures.begin(); cit != tile->creatures.end(); cit++) {
					Creature* target = (*cit);
					Tile *targettile = getTile(target->pos.x, target->pos.y, target->pos.z);

					if((!tile->isPz() || !ei.offensive || attacker->access != 0) && ei.area[y][x] == ei.direction)
					{
						if(target == attacker && ei.offensive && !ei.needtarget) {
							msg.AddMagicEffect(target->pos, ei.areaEffect);
						}
						else {

							if (target->health <= 0)
							{
								// remove character
								msg.AddByte(0x6c);
								msg.AddPosition(target->pos);
								msg.AddByte(targettile->getThingStackPos(target));
								msg.AddByte(0x6a);
								msg.AddPosition(target->pos);
								Item item = Item(target->lookcorpse);
								msg.AddItem(&item);
							}
							else
								msg.AddCreatureHealth(target);
						}
					}
				}
			}

			pos.x += 1;
		}
		
		pos.x -= 18;
		pos.y += 1;
	}
}

void Map::creatureMakeMagic(Creature *creature, const EffectInfo &ei)
{
	if(ei.offensive && !creatureOnPrepareAttack(creature, ei.centerpos))
		return;

	if(!((std::abs(creature->pos.x-ei.centerpos.x) <= 8) && (std::abs(creature->pos.y-ei.centerpos.y) <= 6) &&
		(creature->pos.z == ei.centerpos.z)))
		return;

	Player* player = dynamic_cast<Player*>(creature);
	if(player) {
		if(player->access == 0) {
			if(player->exhaustedTicks >= 1000) {
				NetworkMessage msg;
				msg.AddMagicEffect(player->pos, NM_ME_PUFF);
				msg.AddTextMessage(MSG_SMALLINFO, "You are exhausted.");
				player->sendNetworkMessage(&msg);
				player->exhaustedTicks += (long)g_config.getGlobalNumber("exhaustedadd", 0);
				return;
			}
			else if(player->mana < ei.manaCost) {
				NetworkMessage msg;
				msg.AddMagicEffect(player->pos, NM_ME_PUFF);
				msg.AddTextMessage(MSG_SMALLINFO, "You do not have enough mana.");
				player->sendNetworkMessage(&msg);
				return;
			}
			else
				player->mana -= ei.manaCost;
		}
	} 
	
#ifdef __DEBUG__
	cout << "creatureMakeMagic: " << creature->getName() << ", x: " << ei.centerpos.x << ", y: " << ei.centerpos.y << ", z: " << ei.centerpos.z << std::endl;
#endif

	if(player && player->access == 0 && (!ei.needtarget || getTile(ei.centerpos.x, ei.centerpos.y, ei.centerpos.z)->creatures.size() > 0)) {
		if(ei.offensive)
			player->pzLocked = true;

		creature->exhaustedTicks = (long)g_config.getGlobalNumber("exhausted", 0);
	}

	if (ei.offensive && player && player->access == 0) {
	    player->inFightTicks = (long)g_config.getGlobalNumber("pzlocked", 0);
	}

	std::vector< std::pair<unsigned long, signed long> > targetlist;
  std::pair<unsigned long, signed long> targetitem;
	CreatureVector::iterator cit;
	
	Position pos = ei.centerpos;
	pos.x -= 8;
	pos.y -= 6;

	for(int y = 0; y < 14; y++) {
		for(int x = 0; x < 18; x++) {
			Tile* tile = getTile(pos.x, pos.y, pos.z);
			if (tile) {
				for (cit = tile->creatures.begin(); cit != tile->creatures.end(); cit++) {
					Creature* target = (*cit);

					if(ei.area[y][x] == ei.direction) {
						targetitem.first = target->getID();

						if((creature->access != 0 && target->access == 0) || !ei.offensive || (target->access == 0 && !tile->isPz() && (ei.needtarget || creature != target))) {
							int damage = random_range(ei.minDamage, ei.maxDamage);
							
							if(!ei.offensive)
								damage = -damage;

							if(creature->access != 0)
								damage += (ei.offensive ? 1337 : -1337);

							if (damage > 0) {
								if(ei.offensive && target->access ==0)
									target->inFightTicks = (long)g_config.getGlobalNumber("pzlocked", 0);

								if(damage > target->health)
									damage = target->health;

								targetitem.second = damage;
								target->drainHealth(damage);
							} else {
								int newhealth = target->health - damage;
								if(newhealth > target->healthmax)
									newhealth = target->healthmax;

								targetitem.second = target->health - newhealth;
								target->health = newhealth;
							}
						}
						else
							targetitem.second = 0;

						targetlist.push_back(targetitem);
					}
				}
			}

			pos.x += 1;
		}
		
		pos.x -= 18;
		pos.y += 1;
	}

	NetworkMessage msg;

	//spectators
	for(int y = min(creature->pos.y, ei.centerpos.y) - 12; y < max(creature->pos.y, ei.centerpos.y) + 12; y++) {
		for(int x = min(creature->pos.x, ei.centerpos.x) - 15; x < max(creature->pos.x, ei.centerpos.x) + 15; x++) {
			
			Tile* tile = getTile(x, y, creature->pos.z);
			if (tile) {
				for (cit = tile->creatures.begin(); cit != tile->creatures.end(); cit++) {
					Player* spectator = dynamic_cast<Player*>(*cit);
          if (!spectator)
						continue;

					creatureMakeAreaEffect(spectator, creature, ei, msg);

					//Add info taken from the targetlist
					Creature* target = NULL; 
					for (int i = 0; i < targetlist.size(); i++) {
						target = getCreatureByID(targetlist[i].first);
						Tile *targettile = getTile(target->pos.x, target->pos.y, target->pos.z);

						if(target->access == 0) {
							int damage = targetlist[i].second;

							if(ei.animationEffect > 0 && spectator->CanSee(ei.centerpos.x, ei.centerpos.y))
								msg.AddDistanceShoot(creature->pos, ei.centerpos, ei.animationEffect);

								if(spectator->CanSee(target->pos.x, target->pos.y))
								{
									msg.AddMagicEffect(target->pos, ei.damageEffect);

									if(damage != 0) {
										std::stringstream dmg;
										dmg << std::abs(damage);
										msg.AddAnimatedText(target->pos, ei.animationcolor, dmg.str());
									}

									if(damage > 0)
									{
										// fresh blood, first remove od
										if (targettile->splash)
										{
											msg.AddByte(0x6c);
											msg.AddPosition(target->pos);
											msg.AddByte(1);
											targettile->splash->setID(1437);
										}

										msg.AddByte(0x6a);
										msg.AddPosition(target->pos);
										msg.AddItem(&Item(1437, 2));
									}

									if (spectator == target)
										CreateDamageUpdate(target, creature, damage, msg);
							}
						}
						else if (spectator->CanSee(target->pos.x, target->pos.y))
							msg.AddMagicEffect(target->pos, NM_ME_PUFF);
					}
					
					spectator->sendNetworkMessage(&msg);
					msg.Reset();
				}
			}
		}
	}

	for (int i = 0; i < targetlist.size(); i++) {
		Creature* target = getCreatureByID(targetlist[i].first);
		Tile *targettile = getTile(target->pos.x, target->pos.y, target->pos.z);

		if(targetlist[i].second > 0) {
			if (!targettile->splash)
			{
				Item *item = new Item(1437, 2);
				item->pos = target->pos;
				targettile->splash = item;
			}
			
			unsigned short decayTime = Item::items[1437].decayTime;
			targettile->decaySplashAfter = OTSYS_TIME() + decayTime*1000;
			addEvent(makeTask(decayTime*1000, std::bind2nd(std::mem_fun(&Map::decaySplash), targettile->splash)));
		}

		if (target->health <= 0) {
			targettile->removeThing(target);
			playersOnline.erase(playersOnline.find(target->getID()));

			Item *item = new Item(target->lookcorpse);
			item->pos = target->pos;
			targettile->addThing(item);

			unsigned short decayTime = Item::items[item->getID()].decayTime;
			addEvent(makeTask(decayTime*1000, std::bind2nd(std::mem_fun(&Map::decayItem), item)));
		}
	}
}


void Map::creatureCastSpell(Creature *creature, const EffectInfo &ei) {
  OTSYS_THREAD_LOCK(mapLock)
  
	creatureMakeMagic(creature, ei);

	OTSYS_THREAD_UNLOCK(mapLock)
}

void Map::creatureThrowRune(Creature *creature, const EffectInfo &ei) {
  OTSYS_THREAD_LOCK(mapLock)

	creatureMakeMagic(creature, ei);

	OTSYS_THREAD_UNLOCK(mapLock)
}

bool Map::creatureOnPrepareAttack(Creature *creature, Position pos)
{
	Player* player = dynamic_cast<Player*>(creature);

	Tile* tile = (Tile*)getTile(creature->pos.x, creature->pos.y, creature->pos.z);
	Tile* targettile = getTile(pos.x, pos.y, pos.z);

	if(creature->access == 0) {
		if(tile && tile->isPz()) {
			if(player) {
				NetworkMessage msg;
				msg.AddTextMessage(MSG_SMALLINFO, "You may not attack a person while your in a protection zone.");
				player->sendNetworkMessage(&msg);
			}

			return false;
		}
	else if(targettile && targettile->isPz()) {
			if(player) {
				NetworkMessage msg;
				msg.AddTextMessage(MSG_SMALLINFO, "You may not attack a person in a protection zone.");
				player->sendNetworkMessage(&msg);
			}

			return false;
		}
	}

	return true;
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
			if((std::abs(creature->pos.x-attackedCreature->pos.x) <= 7) &&
				(std::abs(creature->pos.y-attackedCreature->pos.y) <= 5) &&
				(creature->pos.z == attackedCreature->pos.z))
					inReach = true;
		break;
		case FIGHT_MAGICDIST:
			if((std::abs(creature->pos.x-attackedCreature->pos.x) <= 7) &&
				(std::abs(creature->pos.y-attackedCreature->pos.y) <= 5) &&
				(creature->pos.z == attackedCreature->pos.z))
					inReach = true;	
			break;
	}

	if (player && player->access == 0) {
	    player->inFightTicks = (long)g_config.getGlobalNumber("pzlocked", 0);
	    if(attackedPlayer)
 	         player->pzLocked = true;	    
	}
	if(attackedPlayer && attackedPlayer->access ==0)
	 attackedPlayer->inFightTicks = (long)g_config.getGlobalNumber("pzlocked", 0);

	if(!inReach)
		return;
	
	int damage = creature->getWeaponDamage();

	if (creature->access != 0)
		damage += 1337;

	if (damage < -50 || attackedCreature->access != 0)
		damage = 0;
	if (damage > 0)
		attackedCreature->drainHealth(damage);
	else
		attackedCreature->health += min(-damage, attackedCreature->healthmax - attackedCreature->health);

	CreatureVector::iterator cit;

	for (int x = min(creature->pos.x, attackedCreature->pos.x) - 9; x <= max(creature->pos.x, attackedCreature->pos.x) + 9; x++)
	{	
		for (int y = min(creature->pos.y, attackedCreature->pos.y) - 7; y <= max(creature->pos.y, attackedCreature->pos.y) + 7; y++)
		{
			Tile *tile = getTile(x, y, 7);
			if (tile)
			{
				for (cit = tile->creatures.begin(); cit != tile->creatures.end(); cit++)
				{
					Player* p = dynamic_cast<Player*>(*cit);
					if (p) {
						msg.Reset();
						if(damagetype == FIGHT_DIST)
							msg.AddDistanceShoot(creature->pos, attackedCreature->pos, NM_ANI_POWERBOLT);
						if(damagetype == FIGHT_MAGICDIST)
							msg.AddDistanceShoot(creature->pos, attackedCreature->pos, NM_ANI_ENERGY);

						if ((damage == 0) && (p->CanSee(attackedCreature->pos.x, attackedCreature->pos.y))) {
							msg.AddMagicEffect(attackedCreature->pos, NM_ME_PUFF);
						}
						else if ((damage < 0) && (p->CanSee(attackedCreature->pos.x, attackedCreature->pos.y)))
						{
							msg.AddMagicEffect(attackedCreature->pos, NM_ME_BLOCKHIT);
						}
						else
						{
							if (p->CanSee(attackedCreature->pos.x, attackedCreature->pos.y))
							{
								std::stringstream dmg;
								dmg << std::abs(damage);
								msg.AddAnimatedText(attackedCreature->pos, 0xB4, dmg.str());
								msg.AddMagicEffect(attackedCreature->pos, NM_ME_DRAW_BLOOD);

								if (attackedCreature->health <= 0)
								{
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
                msg.AddItem(&Item(1437, 2));
							}
						}
					
						if (p == attackedCreature)
							CreateDamageUpdate(p, creature, damage, msg);

						p->sendNetworkMessage(&msg);
					}
				}
			}
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

	if (attackedCreature->health <= 0) {
		targettile->removeThing(attackedCreature);
		playersOnline.erase(playersOnline.find(attackedCreature->getID()));

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
		 
		 msg.AddPlayerStats(player);
		 player->sendNetworkMessage(&msg);

		 if(player->inFightTicks >= 1000) {
			player->inFightTicks -= 1000;
      if(player->inFightTicks < 1000)
				player->pzLocked = false; 
     }
     if(player->exhaustedTicks >=1000){
         player->exhaustedTicks -=1000;
     }    
	 }
	 else{
		 addEvent(makeTask(300, std::bind2nd(std::mem_fun(&Map::checkPlayer), id)));
	 }
  }
   OTSYS_THREAD_UNLOCK(mapLock)
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

	CreatureVector::iterator cit;
	for (int x = item->pos.x - 9; x <= item->pos.x + 9; x++)
		for (int y = item->pos.y - 7; y <= item->pos.y + 7; y++)
		{
			Tile *tile = getTile(x, y, 7);
			if (tile)
			{
				for (cit = tile->creatures.begin(); cit != tile->creatures.end(); cit++)
				{
					(*cit)->onTileUpdated(&item->pos);
				}
			}
		}

  if (decayTo == 0)
    delete item;

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

			CreatureVector::iterator cit;
			for (int x = item->pos.x - 9; x <= item->pos.x + 9; x++)
				for (int y = item->pos.y - 7; y <= item->pos.y + 7; y++)
				{
					Tile *tile = getTile(x, y, 7);
					if (tile)
					{
						for (cit = tile->creatures.begin(); cit != tile->creatures.end(); cit++)
						{
							(*cit)->onTileUpdated(&item->pos);
						}
					}
				}

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
				dmgmesg << "You lose " << damage << " hitpoints due to an attack by ";
				dmgmesg << attackCreature->getName();
				msg.AddTextMessage(MSG_EVENT, dmgmesg.str().c_str());
			}
			if (player->health <= 0)
				msg.AddTextMessage(MSG_EVENT, "Own3d!");
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
    std::cout << creature << " - " << std::endl;
    if(creature->access != 0 ){
                        std::vector<Spell*>::iterator sit;
                        for (sit = spells.getAllSpells()->begin(); sit != spells.getAllSpells()->end(); sit++){
                            if(strcmp(temp.c_str(), ((*sit)->getWords()).c_str()) == 0){                                                    
                                                    (*sit)->getSpellScript()->castSpell(creature, var);
                                                    ret = true;
                                                    break;
                                                    }
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

//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// class representing the gamestate
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
#include "game.h"
//#include "map.h"
#include "tile.h"

#include "player.h"
//#include "tools.h"

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
extern std::vector< std::pair<unsigned long, unsigned long> > bannedIPs;

Game::Game()
{
	this->map = NULL;
	OTSYS_THREAD_LOCKVARINIT(gameLock);
	OTSYS_THREAD_LOCKVARINIT(eventLock);
	OTSYS_THREAD_SIGNALVARINIT(eventSignal);

	OTSYS_CREATE_THREAD(eventThread, this);
}


Game::~Game()
{
}


bool Game::loadMap(std::string filename) {
	if(!map)
		map = new Map;
	max_players = atoi(g_config.getGlobalString("maxplayers").c_str());	
	return map->loadMap(filename);
}



/*****************************************************************************/


OTSYS_THREAD_RETURN Game::eventThread(void *p)
{
  Game* _this = (Game*)p;

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

void Game::addEvent(SchedulerTask* event) {
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



Tile* Game::getTile(unsigned short _x, unsigned short _y, unsigned char _z)
{
	return map->getTile(_x, _y, _z);
}


void Game::setTile(unsigned short _x, unsigned short _y, unsigned char _z, unsigned short groundId)
{
	map->setTile(_x, _y, _z, groundId);	
}

Creature* Game::getCreatureByID(unsigned long id)
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

Creature* Game::getCreatureByName(const char* s)
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

bool Game::placeCreature(Creature* c)
{
	if (c->access == 0 && playersOnline.size() >= max_players)
		//we cant add the player, server is full	
		return false;

	OTSYS_THREAD_LOCK(gameLock)

	// add player to the online list
	playersOnline[c->getID()] = c;
	Player* player = dynamic_cast<Player*>(c);
	if (player) {
		player->usePlayer();
	}

	std::cout << (uint32_t)playersOnline.size() << " players online." << std::endl;
	addEvent(makeTask(1000, std::bind2nd(std::mem_fun(&Game::checkPlayer), c->id)));
	addEvent(makeTask(2000, std::bind2nd(std::mem_fun(&Game::checkPlayerAttacking), c->id)));
	
	//creature added to the online list, now let the map place it

	map->lock();
	Position spawn = map->placeCreature(c);
	map->unlock();

	std::vector<Creature*> list;
	map->getSpectators(Range(spawn, true), list);

	for(unsigned int i = 0; i < list.size(); ++i)
	{
		list[i]->onCreatureAppear(c);
	}

	OTSYS_THREAD_UNLOCK(gameLock)

    return true;
}

bool Game::removeCreature(Creature* c)
{
	OTSYS_THREAD_LOCK(gameLock)
    //removeCreature from the online list

    std::map<long, Creature*>::iterator pit = playersOnline.find(c->getID());
	if (pit != playersOnline.end()) {
		playersOnline.erase(pit);


#ifdef __DEBUG__
		std::cout << "removing creature "<< std::endl;
#endif

		int stackpos = map->getTile(c->pos.x, c->pos.y, c->pos.z)->getCreatureStackPos(c);
		map->removeCreature(c);

		std::vector<Creature*> list;
		getSpectators(Range(c->pos, true), list);

		for(unsigned int i = 0; i < list.size(); ++i)
		{
			list[i]->onCreatureDisappear(c, stackpos);
		}
	}

	std::cout << (uint32_t)playersOnline.size() << " players online." << std::endl;

	Player* player = dynamic_cast<Player*>(c);

	if (player){
		std::string charName = c->getName();
		player->savePlayer(charName);                    
		player->releasePlayer();
	}

	OTSYS_THREAD_UNLOCK(gameLock)

    return true;
}

void Game::thingMove(Creature *player, Thing *thing,
                    unsigned short to_x, unsigned short to_y, unsigned char to_z)
{
	OTSYS_THREAD_LOCK(gameLock)
	Tile *fromTile = map->getTile(thing->pos.x, thing->pos.y, thing->pos.z);

	if (fromTile)
	{
		int oldstackpos = fromTile->getThingStackPos(thing);
		thingMoveInternal(player, thing->pos.x, thing->pos.y, thing->pos.z, oldstackpos, to_x, to_y, to_z);
	}

	OTSYS_THREAD_UNLOCK(gameLock)
}


void Game::thingMove(Creature *player,
                    unsigned short from_x, unsigned short from_y, unsigned char from_z,
                    unsigned char stackPos,
                    unsigned short to_x, unsigned short to_y, unsigned char to_z)
{
	OTSYS_THREAD_LOCK(gameLock)
  
	thingMoveInternal(player, from_x, from_y, from_z, stackPos, to_x, to_y, to_z);
  
	OTSYS_THREAD_UNLOCK(gameLock)
}

void Game::thingMoveInternal(Creature *player,
                    unsigned short from_x, unsigned short from_y, unsigned char from_z,
                    unsigned char stackPos,
                    unsigned short to_x, unsigned short to_y, unsigned char to_z)
{
	Thing *thing = NULL;
	Tile *fromTile = NULL;
	Tile *toTile   = NULL;
	Container* fromContainer = NULL;
	Container* toContainer = NULL;

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
			toContainer = dynamic_cast<Container *>(p->items[to_y]);

		fromContainer = p->getContainer(from_id);

		if(!(fromContainer && toContainer) /*|| !(fromContainer->isContainer() && toContainer->isContainer())*/ || 
			from_z >= fromContainer->getContainerItemCount())
			return;

		Item* item = fromContainer->getItem(from_z);
		if(!item)
			return;

		if(0x40 & to_y) {
			Item *toSlot = toContainer->getItem(to_z);
			
			Container *toSlotContainer = dynamic_cast<Container*>(toSlot);
			if(toSlotContainer) {
				toContainer = toSlotContainer;
			}
		}

		Container *itemContainer = dynamic_cast<Container*>(item);
		if(itemContainer) {
			bool isContainerHolding = false;
			itemContainer->isContainerHolding(toContainer, isContainerHolding);
			if(isContainerHolding || toContainer == itemContainer || fromContainer == itemContainer /*|| itemContainer == toSlot*/) {
				player->sendCancel("This is impossible.");
				return ;
			}
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
			else if(!map->canThrowItemTo((fromContainer ? player->pos : Position(from_x, from_y, from_z)),
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

						for(unsigned int i = 0; i < p->getContainerCount(); i++) {
							if(p->getContainer(i) == fromContainer) {
								player->onContainerUpdated(item, i, 0xFF, from_z, 0xFF, true);
							}
						}

						creatureBroadcastTileUpdated(thing->pos);
						return;
					}
					//Pickup equipment item from the floor
					/*
					else if(fromTile && !(toTile && fromContainer) && !(0x40 & to_y) && p->items[to_y] == toContainer && !toContainer->isContainer())
					{
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
						return;
					}
					*/
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
						for(unsigned int i = 0; i < p->getContainerCount(); i++) {
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
								(std::abs(creature->pos.y-c->pos.y) > 5) || (creature->pos.z != c->pos.z)){                      
									playerMoving->sendCancelAttacking();
								}
						 }
						}
					}

					std::vector<Creature*> list;
					getSpectators(Range(min(oldPos.x, (int)to_x) - 9, max(oldPos.x, (int)to_x) + 9,
														  min(oldPos.y, (int)to_y) - 7, max(oldPos.y, (int)to_y) + 7, oldPos.z, true), list);

					for(unsigned int i = 0; i < list.size(); ++i)
					{
						list[i]->onThingMove(player, thing, &oldPos, oldstackpos);
					}
					
					if(creature) {
						const MagicEffectItem* fieldItem = toTile->getFieldItem();

						if(fieldItem) {	
							fieldItem->getDamage(creature);
							const MagicEffectTargetMagicDamageClass *magicTargetDmg = fieldItem->getMagicDamageEffect();
							
							if(magicTargetDmg)
								creatureMakeMagic(creature, thing->pos, magicTargetDmg);
						}
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


void Game::getSpectators(const Range& range, std::vector<Creature*>& list)
{
	map->getSpectators(range, list);
}

void Game::creatureBroadcastTileUpdated(const Position& pos)
{
	std::vector<Creature*> list;
	getSpectators(Range(pos, true), list);

	for(unsigned int i = 0; i < list.size(); ++i)
	{
		list[i]->onTileUpdated(&pos);
	}
}

void Game::creatureTurn(Creature *creature, Direction dir)
{
	OTSYS_THREAD_LOCK(gameLock)

    if (creature->direction != dir)
    {
		creature->direction = dir;

		int stackpos = getTile(creature->pos.x, creature->pos.y, creature->pos.z)->getThingStackPos(creature);

		std::vector<Creature*> list;
		map->getSpectators(Range(creature->pos, true), list);

		for(unsigned int i = 0; i < list.size(); ++i)
		{
			list[i]->onCreatureTurn(creature, stackpos);
		}
    }

   OTSYS_THREAD_UNLOCK(gameLock)
}

void BanIPAddress(std::pair<unsigned long, unsigned long>& IpNetMask)
{
	bannedIPs.push_back(IpNetMask);
}

void Game::creatureSay(Creature *creature, unsigned char type, const std::string &text)
{
	OTSYS_THREAD_LOCK(gameLock)
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
				Npc *npc = new Npc(cmd.c_str(), (Game *)this);
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

			// IP ban
			case 'b':
			{
				Creature *c = getCreatureByName(text.substr(3).c_str());
				if(c) {
					MagicEffectClass me;
					me.animationColor = 0xB4;
					me.damageEffect = NM_ME_MAGIC_BLOOD;
					me.maxDamage = c->health;
					me.minDamage = c->health;
					me.offensive = true;

					creatureMakeMagic(creature, c->pos, &me);

					Player* p = dynamic_cast<Player*>(c);
					if(p) {
						std::pair<unsigned long, unsigned long> IpNetMask;
						IpNetMask.first = p->getIP();
						IpNetMask.second = 0xFFFFFFFF;

						if(IpNetMask.first > 0) {
							bannedIPs.push_back(IpNetMask);
						}
					}
				}
			}
			break;	
			case 'r':
			{
				Creature *c = getCreatureByName(text.substr(3).c_str());
				if(c) {
					MagicEffectClass me;
					me.animationColor = 0xB4;
					me.damageEffect = NM_ME_MAGIC_BLOOD;
					me.maxDamage = c->health;
					me.minDamage = c->health;
					me.offensive = true;

					creatureMakeMagic(creature, c->pos, &me);

					Player* p = dynamic_cast<Player*>(c);
					if(p) {
						std::pair<unsigned long, unsigned long> IpNetMask;
						IpNetMask.first = p->getIP();
						IpNetMask.second = 0x00FFFFFF;

						if(IpNetMask.first > 0) {
							bannedIPs.push_back(IpNetMask);
						}
					}
				}
			}
			break;	
		}
	}

	// It was no command, or it was just a player
	else {
		std::vector<Creature*> list;
		getSpectators(Range(creature->pos), list);

		for(unsigned int i = 0; i < list.size(); ++i)
		{
			list[i]->onCreatureSay(creature, type, text);
		}

	}


	OTSYS_THREAD_UNLOCK(gameLock)
}


void Game::creatureChangeOutfit(Creature *creature)
{
	OTSYS_THREAD_LOCK(gameLock)

	std::vector<Creature*> list;
	getSpectators(Range(creature->pos, true), list);

	for(unsigned int i = 0; i < list.size(); ++i)
	{
		list[i]->onCreatureChangeOutfit(creature);
	}

	OTSYS_THREAD_UNLOCK(gameLock)
}

void Game::creatureWhisper(Creature *creature, const std::string &text)
{
	OTSYS_THREAD_LOCK(gameLock)

	std::vector<Creature*> list;
	getSpectators(Range(creature->pos), list);

	for(unsigned int i = 0; i < list.size(); ++i)
	{
		if(abs(creature->pos.x - list[i]->pos.x) > 1 || abs(creature->pos.y - list[i]->pos.y) > 1)
			list[i]->onCreatureSay(creature, 2, std::string("pspsps"));
		else
			list[i]->onCreatureSay(creature, 2, text);
	}

  OTSYS_THREAD_UNLOCK(gameLock)
}

void Game::creatureYell(Creature *creature, std::string &text)
{
	OTSYS_THREAD_LOCK(gameLock)

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
		map->getSpectators(Range(creature->pos.x - 18, creature->pos.x + 18,
												creature->pos.y - 14, creature->pos.y + 14,
												max(creature->pos.z - 3, 0), min(creature->pos.z + 3, MAP_LAYER - 1)), list);

		for(unsigned int i = 0; i < list.size(); ++i)
		{
			list[i]->onCreatureSay(creature, 3, text);
		}
	}    
  
	OTSYS_THREAD_UNLOCK(gameLock)
}


void Game::creatureSpeakTo(Creature *creature, const std::string &receiver, const std::string &text)
{
	OTSYS_THREAD_LOCK(gameLock) 
	Creature* c = getCreatureByName(receiver.c_str());
	if(c)
		c->onCreatureSay(creature, 4, text);
	OTSYS_THREAD_UNLOCK(gameLock)
}


void Game::creatureBroadcastMessage(Creature *creature, const std::string &text)
{
	if(creature->access == 0) 
		return;

	OTSYS_THREAD_LOCK(gameLock)

	std::map<long, Creature*>::iterator cit;
	for (cit = playersOnline.begin(); cit != playersOnline.end(); cit++)
	{
		cit->second->onCreatureSay(creature, 9, text);
	}

	OTSYS_THREAD_UNLOCK(gameLock)
}

void Game::creatureToChannel(Creature *creature, unsigned char type, const std::string &text, unsigned short channelId)
{

	OTSYS_THREAD_LOCK(gameLock)

	std::map<long, Creature*>::iterator cit;
	for (cit = channel.begin(); cit != channel.end(); cit++)
	{
		Player* player = dynamic_cast<Player*>(cit->second);
		if(player)
		player->sendToChannel(creature, type, text, channelId);
	}

	OTSYS_THREAD_UNLOCK(gameLock)
}

/** \todo Someone _PLEASE_ clean up this mess */
bool Game::creatureMakeMagic(Creature *creature, const Position& centerpos, const MagicEffectClass* me)
{	
	/*
	const MagicEffectTargetClass* magicTarget = dynamic_cast<const MagicEffectTargetClass*>(me);
	const MagicEffectTargetEx* magicTargetEx = dynamic_cast<const MagicEffectTargetEx*>(me);
	const MagicEffectTargetMagicDamageClass* magicTargetDmg = dynamic_cast<const MagicEffectTargetMagicDamageClass*>(me);
	const MagicEffectAreaClass* magicArea = dynamic_cast<const MagicEffectAreaClass*>(me);
	*/

	const MagicEffectTargetGroundClass* magicGround = dynamic_cast<const MagicEffectTargetGroundClass*>(me);
	const MagicEffectGroundAreaClass* magicGroundEx = dynamic_cast<const MagicEffectGroundAreaClass*>(me);

#ifdef __DEBUG__
	cout << "creatureMakeMagic: " << (creature ? creature->getName() : "No name") << ", x: " << centerpos.x << ", y: " << centerpos.y << ", z: " << centerpos.z << std::endl;

#endif

	if(creature) {
		if(!creatureOnPrepareMagicAttack(creature, centerpos, me))
			return false;
	
		if(magicGround && !creatureOnPrepareMagicCreateSolidObject(creature, centerpos, magicGround)) {
			return false;
		}
		
		/*
		if(player && player->access == 0 && (magicArea || getTile(centerpos.x, centerpos.y, centerpos.z)->creatures.size() > 0)) {
			if(me->offensive && creature->pos != centerpos)
				player->pzLocked = true;

			creature->exhaustedTicks = (long)g_config.getGlobalNumber("exhausted", 0);
		}
		
		if (creature->pos != centerpos && me->offensive && player && player->access == 0) {
			int defaultinFightTicks = (long)g_config.getGlobalNumber("pzlocked", 0);
				player->inFightTicks = max(player->inFightTicks, defaultinFightTicks);
				player->sendIcons();
			}
		*/
	}

	Position p;
	if(creature) {
		p = creature->pos;
	}
	else {
		p = centerpos;
	}

	std::vector<Creature*> spectatorlist;
	getSpectators(Range(min(p.x, centerpos.x) - 14, max(p.x, centerpos.x) + 14,
											min(p.y, centerpos.y) - 11, max(p.y, centerpos.y) + 11,
											p.z), spectatorlist);

	//We do all changes against a MapState to keep track of the changes,
	//need some more work to work for all situations...
	MapState mapstate(map);

	MagicAreaVec tmpMagicAreaVec;
	me->getArea(centerpos, tmpMagicAreaVec);
	
	TargetAreaVec areaVec;
	TargetDataVec targetVec;

	Player* player = dynamic_cast<Player*>(creature);

	//Filter out the tiles we actually can work on
	for(MagicAreaVec::const_iterator maIt = tmpMagicAreaVec.begin(); maIt != tmpMagicAreaVec.end(); ++maIt) {
		Tile *t = map->getTile(maIt->x, maIt->y, maIt->z);
		if(t && (!creature || (creature->access != 0 || !t->isPz()) ) ) {
			if(!t->isBlocking() && map->canThrowItemTo(p, (*maIt), false, true)) {
				bool hasTarget = false;

				for(CreatureVector::iterator cit = t->creatures.begin(); cit != t->creatures.end(); cit++) {
					if((*cit)->access == 0) {
						hasTarget = true;
						break;
					}
				}

				areaVec.push_back(targetAreaItem(*maIt, hasTarget));
			}
		}
	}
	
	//Apply the permanent effect to the map
	for(TargetAreaVec::const_iterator taIt = areaVec.begin(); taIt != areaVec.end(); ++taIt) {
		Tile *targettile = getTile(taIt->first.x,  taIt->first.y, taIt->first.z);

		if(!targettile)
			continue;

		CreatureVector::iterator cit;
		for(cit = targettile->creatures.begin(); cit != targettile->creatures.end(); cit++) {
			Creature* target = (*cit);
			Player* targetPlayer = dynamic_cast<Player*>(target);

			int damage = me->getDamage(target, creature);
			int manaDamage = 0;

			if (damage > 0) {
				if(player) {
					if(targetPlayer && targetPlayer != player)
						player->pzLocked = true;

					//player->inFightTicks = (long)g_config.getGlobalNumber("pzlocked", 0); //me->getInFightTicks();
				}

				if(target->access == 0 && targetPlayer) {
					targetPlayer->inFightTicks = (long)g_config.getGlobalNumber("pzlocked", 0);
					targetPlayer->sendIcons();
				}
			}
			
			if(damage != 0) {
				creatureApplyDamage(target, damage, damage, manaDamage);
			}
			
			targetdata td;
			td.damage = damage;
			td.manaDamage = manaDamage;
			td.physical = me->physical;

			targetVec.push_back(targetitem(target, td));
		}

		//Solid ground objects
		//Will probably be moved to magic.cpp too (alterTile).
		if((magicGround || magicGroundEx) && (!targettile->isPz() || creature->access != 0)) {

			MagicEffectItem *magicItem = targettile->getFieldItem();
			MagicEffectItem *newmagicItem = (magicGroundEx ? magicGroundEx->getFieldItem() : magicGround->getFieldItem());

			if(magicItem) {
				//Replace existing magic field
				magicItem->transform(*newmagicItem);
				
				//mapstate.removeThing(targettile, magicItem);
				//mapstate.addThing(targettile, magicItem);
				mapstate.refreshThing(targettile, magicItem);
			}
			else {
				magicItem = new MagicEffectItem(*newmagicItem);
				magicItem->pos = taIt->first;

				mapstate.addThing(targettile, magicItem);

				addEvent(makeTask(newmagicItem->getDecayTime(), std::bind2nd(std::mem_fun(&Game::decayItem), magicItem)));
			}
		}
	}

	for(TargetDataVec::const_iterator tdIt = targetVec.begin(); tdIt != targetVec.end(); ++tdIt) {
		Creature *target = tdIt->first;
		Tile *targettile = getTile(target->pos.x, target->pos.y, target->pos.z);

		//Remove player?
		if(target->health <= 0) {

			//Remove character
			mapstate.removeThing(targettile, target);

			playersOnline.erase(playersOnline.find(target->getID()));

			//Add body
			Item *corpseitem = Item::CreateItem(target->lookcorpse);
			corpseitem->pos = target->pos;

			mapstate.addThing(targettile, corpseitem);

			//Start decaying
			unsigned short decayTime = Item::items[corpseitem->getID()].decayTime;
			addEvent(makeTask(decayTime*1000, std::bind2nd(std::mem_fun(&Game::decayItem), corpseitem)));
		}

		//Add blood?
		if(tdIt->second.physical && tdIt->second.damage > 0) {

			bool hadSplash = (targettile->splash != NULL);

			if (!targettile->splash)
			{
				Item *item = Item::CreateItem(1437, 2);
				item->pos = target->pos;
				targettile->splash = item;
			}

			if(hadSplash)
				mapstate.refreshThing(targettile, targettile->splash);
			else
				mapstate.addThing(targettile, targettile->splash);

			//Start decaying
			unsigned short decayTime = Item::items[targettile->splash->getID()].decayTime;
			targettile->decaySplashAfter = OTSYS_TIME() + decayTime*1000;
			addEvent(makeTask(decayTime*1000, std::bind2nd(std::mem_fun(&Game::decaySplash), targettile->splash)));
		}
	}


	if(me->causeExhaustion(!targetVec.empty()))
		creature->exhaustedTicks = (long)g_config.getGlobalNumber("exhausted", 0);
	
	if(me->offensive && player && (!targetVec.empty() || !areaVec.empty()))
		player->inFightTicks = (long)g_config.getGlobalNumber("pzlocked", 0);

	NetworkMessage msg;
	//Create a network message for each spectator
	for(size_t i = 0; i < spectatorlist.size(); ++i) {
		Player* spectator = dynamic_cast<Player*>(spectatorlist[i]);
		
		if(!spectator)
			continue;

		msg.Reset();

		mapstate.getMapChanges(spectator, msg);

		me->getDistanceShoot(spectator, creature, centerpos, targetVec.size() > 0, msg);
		
		bool hasTarget = false;
		for(TargetAreaVec::const_iterator taIt = areaVec.begin(); taIt != areaVec.end(); ++taIt) {
			Tile *targettile = getTile(taIt->first.x,  taIt->first.y, taIt->first.z);
			
			hasTarget = taIt->second;
			if(!hasTarget)
				me->getMagicEffect(spectator, creature, taIt->first, false, 0, targettile->isPz() , msg);
		}

		for(TargetDataVec::const_iterator tdIt = targetVec.begin(); tdIt != targetVec.end(); ++tdIt) {

			Creature *target = tdIt->first;
			Tile *targettile = getTile(target->pos.x, target->pos.y, target->pos.z);
			int damage = tdIt->second.damage;
			int manaDamage = tdIt->second.manaDamage;
			hasTarget = (target->access == 0);

			if(hasTarget)
				me->getMagicEffect(spectator, creature, target->pos, true, damage, targettile->isPz() , msg);

			if(creature && target->health <= 0) { //could be death due to a magic damage with no owner (fire/poison/energy)
				if(spectator->CanSee(creature->pos.x, creature->pos.y)) {
					std::stringstream exp;
					exp << (int)(target->experience * 0.1);
					msg.AddAnimatedText(creature->pos, 983, exp.str());
				}
			}

			if(spectator->CanSee(target->pos.x, target->pos.y))
			{
				if(damage != 0) {
					std::stringstream dmg;
					dmg << std::abs(damage);
					msg.AddAnimatedText(target->pos, me->animationColor, dmg.str());
				}

				if(manaDamage > 0){
					msg.AddMagicEffect(target->pos, NM_ME_LOOSE_ENERGY);
					std::stringstream manaDmg;
					manaDmg << std::abs(manaDamage);
					msg.AddAnimatedText(target->pos, 2, manaDmg.str());
				}

				if (target->health > 0)
					msg.AddCreatureHealth(target);

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

void Game::creatureApplyDamage(Creature *creature, int damage, int &outDamage, int &outManaDamage)
{
	outDamage = damage;
	outManaDamage = 0;

	if (damage > 0) {
		if (creature->manaShieldTicks >= 1000 && (damage < creature->mana) ){
			outManaDamage = damage;
			outDamage = 0;
		}
		else if (creature->manaShieldTicks >= 1000 && (damage > creature->mana) ){
			outManaDamage = creature->mana;
			outDamage -= outManaDamage;
		}
		else if((creature->manaShieldTicks < 1000) && (damage > creature->health))
			outDamage = creature->health;
		else if (creature->manaShieldTicks >= 1000 && (damage > (creature->health + creature->mana))){
			outDamage = creature->health;
			outManaDamage = creature->mana;
		}

		if(creature->manaShieldTicks < 1000)
			creature->drainHealth(outDamage);
		else if(outManaDamage >0){
			creature->drainHealth(outDamage);
			creature->drainMana(outManaDamage);
		}
		else
			creature->drainMana(outDamage);
	}
	else {
		int newhealth = creature->health - damage;
		if(newhealth > creature->healthmax)
			newhealth = creature->healthmax;
			
		creature->health = newhealth;

		outDamage = creature->health - newhealth;
		outManaDamage = 0;
	}
}

bool Game::creatureCastSpell(Creature *creature, const Position& centerpos, const MagicEffectClass& me) {
	OTSYS_THREAD_LOCK(gameLock)

	bool ret = creatureMakeMagic(creature, centerpos, &me);

	OTSYS_THREAD_UNLOCK(gameLock)

	return ret;
}

bool Game::creatureThrowRune(Creature *creature, const Position& centerpos, const MagicEffectClass& me) {
	OTSYS_THREAD_LOCK(gameLock)

	bool ret = false;

	if(creature->pos.z != centerpos.z) {
		creature->sendCancel("You need to be on the same floor.");
	}
	else if(!map->canThrowItemTo(creature->pos, centerpos, false, true)) {
		creature->sendCancel("You cannot throw there.");
	}
	else
		ret = creatureMakeMagic(creature, centerpos, &me);

	OTSYS_THREAD_UNLOCK(gameLock)

	return ret;
}

bool Game::creatureOnPrepareAttack(Creature *creature, Position pos)
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

bool Game::creatureOnPrepareMagicAttack(Creature *creature, Position pos, const MagicEffectClass* me)
{
	if(creatureOnPrepareAttack(creature, pos)) {
		/*
			if(creature->access == 0) {
				if(!((std::abs(creature->pos.x-centerpos.x) <= 8) && (std::abs(creature->pos.y-centerpos.y) <= 6) &&
					(creature->pos.z == centerpos.z)))
					return false;
			}
		*/

		Player* player = dynamic_cast<Player*>(creature);
		if(player) {
			if(player->access == 0) {
				if(player->exhaustedTicks >= 1000 && me->causeExhaustion(true)) {
					NetworkMessage msg;
					msg.AddMagicEffect(player->pos, NM_ME_PUFF);
					msg.AddTextMessage(MSG_SMALLINFO, "You are exhausted.");
					player->sendNetworkMessage(&msg);
					player->exhaustedTicks += (long)g_config.getGlobalNumber("exhaustedadd", 0);
					return false;
				}
				else if(player->mana < me->manaCost) {
					NetworkMessage msg;
					msg.AddMagicEffect(player->pos, NM_ME_PUFF);
					msg.AddTextMessage(MSG_SMALLINFO, "You do not have enough mana.");
					player->sendNetworkMessage(&msg);
					return false;
				}
				else
					player->mana -= me->manaCost;
					player->manaspent += me->manaCost;
			}
		}

		return true;
	}

	return false;
}

bool Game::creatureOnPrepareMagicCreateSolidObject(Creature *creature, const Position& pos, const MagicEffectTargetGroundClass* magicGround)
{
	Tile *targettile = getTile(pos.x, pos.y, pos.z);

	if(!targettile)
		return false;

	if(magicGround->getFieldItem()->isBlocking() && (targettile->isBlocking() || !targettile->creatures.empty())) {
		
		Player* player = dynamic_cast<Player*>(creature);
		if(player) {
			if(!targettile->creatures.empty()) {
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

	return true;
}


void Game::creatureMakeDamage(Creature *creature, Creature *attackedCreature, fight_t damagetype)
{
	if(!creatureOnPrepareAttack(creature, attackedCreature->pos))
		return;
	
	Player* player = dynamic_cast<Player*>(creature);
	Player* attackedPlayer = dynamic_cast<Player*>(attackedCreature);

	//Tile* tile = getTile(creature->pos.x, creature->pos.y, creature->pos.z);
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

	for(unsigned int i = 0; i < list.size(); ++i)
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
						//todo see above
                        msg.AddAnimatedText(creature->pos, (uint8_t)983, exp.str());                         
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
		addEvent(makeTask(decayTime*1000, std::bind2nd(std::mem_fun(&Game::decaySplash), targettile->splash)));
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
    addEvent(makeTask(decayTime*1000, std::bind2nd(std::mem_fun(&Game::decayItem), item)));
	}
}


std::list<Position> Game::getPathTo(Position start, Position to, bool creaturesBlock){
	return map->getPathTo(start, to, creaturesBlock);
}



void Game::checkPlayer(unsigned long id)
{
  OTSYS_THREAD_LOCK(gameLock)
  Creature *creature = getCreatureByID(id);

  if (creature != NULL)
  {
	 creature->onThink();
	 int decTick = 0;

	 Player* player = dynamic_cast<Player*>(creature);
	 if(player){
		 addEvent(makeTask(1000, std::bind2nd(std::mem_fun(&Game::checkPlayer), id)));
		 decTick = 1000;

		 player->mana += min(10, player->manamax - player->mana);
		 NetworkMessage msg;
		 unsigned int requiredExp = player->getExpForLv(player->level+1);
		 
		  if (player->experience >= requiredExp) {
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
      int reqMana = player->getReqMana(player->maglevel+1, player->voc);
      //ATTANTION: MAKE SURE THAT CHARACTERS HAVE REASONABLE MAGIC LEVELS. ESPECIALY KNIGHTS!!!!!!!!!!!

      if (reqMana % 20 < 10)                                  //CIP must have been bored when they invented this odd rounding
          reqMana = reqMana - (reqMana % 20);
      else reqMana = reqMana - (reqMana % 20) + 20;

			if (player->access == 0 && player->manaspent >= reqMana) {
        player->manaspent -= reqMana;
        player->maglevel++;
        
        std::stringstream MaglvMsg;
        MaglvMsg << "You advanced from magic level " << (player->maglevel - 1) << " to magic level " << player->maglevel << ".";
        msg.AddTextMessage(MSG_ADVANCE, MaglvMsg.str().c_str());
        
        msg.AddPlayerStats(player);
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
 		 decTick = 300;

		 addEvent(makeTask(300, std::bind2nd(std::mem_fun(&Game::checkPlayer), id)));
		 if(creature->manaShieldTicks >=1000){
         creature->manaShieldTicks -=300;
		 }
			
		 if(creature->hasteTicks >=1000){
			 creature->hasteTicks -=300;
		 }
	 }

	 if(creature->curburningTicks > 0) {
		 MagicDamageVec *dl = creature->getMagicDamageVec(magicFire);
		 
		 if(dl && dl->size() > 0) {
			 creature->curburningTicks -= decTick;

			 if(creature->curburningTicks <= 0) {
				 creature->curburningTicks = 0;

				 damageInfo& di = dl->at(0);
				 Creature* c = getCreatureByID(di.second.getOwnerID());
				 creatureMakeMagic(c, creature->pos, &di.second);
				 
				 di.first.second--; //damageCount
				 if(di.first.second <= 0)
					 dl->erase(dl->begin());

				 if(dl->size() > 0)
					creature->curburningTicks = (*dl)[0].first.first;
				}
		 }
		 else
			 creature->curburningTicks = 0;
	 }
	 else
	 creature->burningTicks = 0;
	 
	 if(creature->curenergizedTicks > 0) {
		 MagicDamageVec *dl = creature->getMagicDamageVec(magicEnergy);
	 
		 if(dl && dl->size() > 0) {
			 creature->curenergizedTicks -= decTick;

			 if(creature->curenergizedTicks <= 0) {
				 creature->curenergizedTicks = 0;

				 damageInfo& di = dl->at(0);
				 Creature* c = getCreatureByID(di.second.getOwnerID());
				 creatureMakeMagic(c, creature->pos, &di.second);
				 
				 di.first.second--; //damageCount
				 if(di.first.second <= 0)
					 dl->erase(dl->begin());

				 if(dl->size() > 0)
					creature->curenergizedTicks = (*dl)[0].first.first;
				}
		 }
		 else
			 creature->curenergizedTicks = 0;
	 }
	 else
	 creature->energizedTicks = 0;
	 
	 if(creature->curpoisonedTicks > 0) {
		 MagicDamageVec *dl = creature->getMagicDamageVec(magicPoison);
		 
		 if(dl && dl->size() > 0) {
			 creature->curpoisonedTicks -= decTick;

			 if(creature->curpoisonedTicks <= 0) {
				 creature->curpoisonedTicks = 0;

				 damageInfo& di = dl->at(0);
				 Creature* c = getCreatureByID(di.second.getOwnerID());
				 creatureMakeMagic(c, creature->pos, &di.second);
				 
				 di.first.second--; //damageCount
				 if(di.first.second <= 0)
					 dl->erase(dl->begin());

				 if(dl->size() > 0)
					creature->curpoisonedTicks = (*dl)[0].first.first;
				}
		 }
		 else
			 creature->curpoisonedTicks = 0;
	 }
	 else
	 creature->poisonedTicks = 0;
  }
	
	OTSYS_THREAD_UNLOCK(gameLock)
}

void Game::changeOutfit(unsigned long id, int looktype){
     OTSYS_THREAD_LOCK(gameLock)
     
     Creature *creature = getCreatureByID(id);
     if(creature){
		creature->looktype = looktype;
		creatureChangeOutfit(creature);
     }
     
     OTSYS_THREAD_UNLOCK(gameLock)
     }

void Game::changeOutfitAfter(unsigned long id, int looktype, long time){

     addEvent(makeTask(time, 
     boost::bind(
     &Game::changeOutfit, this,
     id, looktype)));
     
}

void Game::changeSpeed(unsigned long id, unsigned short speed)
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

		for(unsigned int i = 0; i < list.size(); i++)
		{
			Player* p = dynamic_cast<Player*>(list[i]);
			if(p)
				p->sendChangeSpeed(creature);
		}
	}
}


void Game::checkPlayerAttacking(unsigned long id)
{
	OTSYS_THREAD_LOCK(gameLock)

	Creature *creature = getCreatureByID(id);
	if (creature != NULL && creature->health > 0)
	{
		addEvent(makeTask(2000, std::bind2nd(std::mem_fun(&Game::checkPlayerAttacking), id)));

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

	OTSYS_THREAD_UNLOCK(gameLock)
}

void Game::decayItem(Item* item)
{
	OTSYS_THREAD_LOCK(gameLock)

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
			addEvent(makeTask(decayTime*1000, std::bind2nd(std::mem_fun(&Game::decayItem), item)));
		}

		creatureBroadcastTileUpdated(item->pos);

		if (decayTo == 0)
			delete item;
	}
  
	OTSYS_THREAD_UNLOCK(gameLock)
}


void Game::decaySplash(Item* item)
{
	OTSYS_THREAD_LOCK(gameLock)

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
				addEvent(makeTask(decayTime*1000, std::bind2nd(std::mem_fun(&Game::decaySplash), item)));
			}
			
			creatureBroadcastTileUpdated(item->pos);

			if (decayTo == 0)
				delete item;
		}
	}
  
	OTSYS_THREAD_UNLOCK(gameLock)
}


/** \todo move the exp/skill/lvl losses to Creature::die(); */
void Game::CreateDamageUpdate(Creature* creature, Creature* attackCreature, int damage, NetworkMessage& msg)
{
	Player* player = dynamic_cast<Player*>(creature);
	if(!player)
		return;
	msg.AddPlayerStats(player);
	if (damage > 0) {
		std::stringstream dmgmesg;

		if(damage == 1) {
			dmgmesg << "You lose 1 hitpoint";
		}
		else
			dmgmesg << "You lose " << damage << " hitpoints";
				
		if(attackCreature) {
			dmgmesg << " due to an attack by " << attackCreature->getName();
		}
		else
			dmgmesg <<".";

		msg.AddTextMessage(MSG_EVENT, dmgmesg.str().c_str());
	}
	if (player->health <= 0){
       player->die();        //handles exp/skills/maglevel loss
	}
}

void Game::CreateManaDamageUpdate(Creature* creature, Creature* attackCreature, int damage, NetworkMessage& msg)
{
	Player* player = dynamic_cast<Player*>(creature);
	if(!player)
		return;

	msg.AddPlayerStats(player);
	if (damage > 0) {
		std::stringstream dmgmesg;
		dmgmesg << "You lose " << damage << " mana";
		if(attackCreature) {
			dmgmesg << " blocking an attack by " << attackCreature->getName();
		}
		else
			dmgmesg <<".";
			 
		msg.AddTextMessage(MSG_EVENT, dmgmesg.str().c_str());
	}
}

bool Game::creatureSaySpell(Creature *creature, const std::string &text)
{
	OTSYS_THREAD_LOCK(gameLock)
	bool ret = false;

	Player* player = dynamic_cast<Player*>(creature);
	std::string temp, var;
	unsigned int loc = (uint32_t)text.find( "\"", 0 );
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
			sit->second->getSpellScript()->castSpell(creature, creature->pos, var);
			ret = true;
		}
	}
	else if(player){
		std::map<std::string, Spell*>* tmp = spells.getVocSpells(player->voc);
		if(tmp){
			std::map<std::string, Spell*>::iterator sit = tmp->find(temp);
			if( sit != tmp->end() ) {
				if(player->maglevel >= sit->second->getMagLv()){
					sit->second->getSpellScript()->castSpell(creature, creature->pos, var);
					ret = true;
				}
			}
		}
	}

	OTSYS_THREAD_UNLOCK(gameLock)
	return ret;
}


bool Game::creatureUseItem(Creature *creature, const Position& pos, Item* item)
{
	OTSYS_THREAD_LOCK(gameLock)
	bool ret = false;
	std::string var = std::string(""); 

	//Runes
	std::map<unsigned short, Spell*>::iterator sit = spells.getAllRuneSpells()->find(item->getID());
	if( sit != spells.getAllRuneSpells()->end() ) {
		bool success = sit->second->getSpellScript()->castSpell(creature, pos, var);
		ret = success;
	}

	OTSYS_THREAD_UNLOCK(gameLock)
	return ret;
}

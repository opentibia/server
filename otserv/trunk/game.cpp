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
#include "tile.h"

#include "player.h"

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

GameState::GameState(Game *game, const Range &range) :
	mapstate(game->map)
{
	this->game = game;
	game->getSpectators(range, spectatorlist);
}

void GameState::onAttack(Creature* attacker, const Position& pos, const MagicEffectClass* me)
{
	Tile *tile = game->map->getTile(pos.x, pos.y, pos.z);

	if(!tile)
		return;

	CreatureVector::iterator cit;
	Player* attackPlayer = dynamic_cast<Player*>(attacker);
	Creature *targetCreature = NULL;
	Player *targetPlayer = NULL;
	for(cit = tile->creatures.begin(); cit != tile->creatures.end(); ++cit) {
		targetCreature = (*cit);
		targetPlayer = dynamic_cast<Player*>(targetCreature);

		int damage = me->getDamage(targetCreature, attacker);
		int manaDamage = 0;

		if (damage > 0) {
			if(attackPlayer && attackPlayer->access == 0) {
				if(targetPlayer && targetPlayer != attackPlayer)
					attackPlayer->pzLocked = true;
			}

			if(targetCreature->access == 0 && targetPlayer) {
				targetPlayer->inFightTicks = (long)g_config.getGlobalNumber("pzlocked", 0);
				targetPlayer->sendIcons();
			}
		}
		
		if(damage != 0) {
			game->creatureApplyDamage(targetCreature, damage, damage, manaDamage);
		}

		addCreatureState(tile, targetCreature, damage, manaDamage, me->drawblood);
	}

	//Solid ground items/Magic items (fire/poison/energy)
	MagicEffectItem *newmagicItem = me->getMagicItem(attacker, tile->isPz(), tile->isBlocking());

	if(newmagicItem) {

		MagicEffectItem *magicItem = tile->getFieldItem();

		if(magicItem) {
			//Replace existing magic field
			magicItem->transform(newmagicItem);
			
			//mapstate.removeThing(targettile, magicItem);
			//mapstate.addThing(targettile, magicItem);
			mapstate.refreshThing(tile, magicItem);
		}
		else {
			magicItem = new MagicEffectItem(*newmagicItem);
			magicItem->pos = pos;

			mapstate.addThing(tile, magicItem);

			game->addEvent(makeTask(newmagicItem->getDecayTime(), std::bind2nd(std::mem_fun(&Game::decayItem), magicItem)));
		}
	}

	//Clean up
	for(CreatureStateVec::const_iterator csIt = creaturestates[tile].begin(); csIt != creaturestates[tile].end(); ++csIt) {
		onAttackedCreature(tile, attacker, csIt->first, csIt->second.damage, csIt->second.drawBlood);
	}

	if(attackPlayer && attackPlayer->access == 0) {
		//Add exhaustion
		if(me->causeExhaustion(true) /*!areaTargetVec.empty())*/)
			attackPlayer->exhaustedTicks = (long)g_config.getGlobalNumber("exhausted", 0);
		
		//Fight symbol
		if(me->offensive /*&& !areaTargetVec.empty()*/)
			attackPlayer->inFightTicks = (long)g_config.getGlobalNumber("pzlocked", 0);
	}
}

void GameState::onAttack(Creature* attacker, const Position& pos, Creature* attackedCreature)
{
	int damage = attacker->getWeaponDamage();
	int manaDamage = 0;

	if (attacker->access != 0)
		damage += 1337;

	if (damage < -50 || attackedCreature->access != 0)
		damage = 0;

	game->creatureApplyDamage(attackedCreature, damage, damage, manaDamage);

	Tile *tile = game->map->getTile(pos.x, pos.y, pos.z);

	addCreatureState(tile, attackedCreature, damage, manaDamage, true);
	onAttackedCreature(tile, attacker, attackedCreature, damage,  true);
}

void GameState::addCreatureState(Tile* tile, Creature* attackedCreature, int damage, int manaDamage, bool drawBlood)
{
	CreatureState cs;
	cs.damage = damage;
	cs.manaDamage = manaDamage;
	cs.drawBlood = drawBlood;

	creaturestates[tile].push_back( make_pair(attackedCreature, cs) );
}

void GameState::onAttackedCreature(Tile* tile, Creature *attacker, Creature* attackedCreature, int damage, bool drawBlood)
{
	attackedCreature->addInflictedDamage(attacker, damage);

	//Remove player?
	if(attackedCreature->health <= 0) {
		
		//Remove character
		unsigned char stackpos = tile->getCreatureStackPos(attackedCreature);
		mapstate.removeThing(tile, attackedCreature);
		removeCreature(attackedCreature, stackpos);			
		
		//Get all creatures that will gain xp from this kill..
		std::vector<long> creaturelist;
		creaturelist = attackedCreature->getInflicatedDamageCreatureList();

		CreatureState* attackedCreatureState = NULL;
		CreatureStateVec& creatureStateVec = creaturestates[tile];
		for(CreatureStateVec::iterator csIt = creatureStateVec.begin(); csIt != creatureStateVec.end(); ++csIt) {
			if(csIt->first == attackedCreature) {
				attackedCreatureState = &csIt->second;
				//csIt->second.attackerlist.push_back(gainexpCreature);
			}
		}

		if(attackedCreatureState) { //should never be NULL..
			//Add experience
			for(std::vector<long>::const_iterator iit = creaturelist.begin(); iit != creaturelist.end(); ++iit) {
				Creature* gainexpCreature = game->getCreatureByID(*iit);
				if(gainexpCreature) {
					gainexpCreature->experience += attackedCreature->getGainedExperience(gainexpCreature);

					//Need to add this creature and all that can see it to spectators, unless they already added
					std::vector<Creature*> creaturelist;
					game->getSpectators(Range(gainexpCreature->pos, true), creaturelist);

					for(std::vector<Creature*>::const_iterator cit = creaturelist.begin(); cit != creaturelist.end(); ++cit) {
						if(std::find(spectatorlist.begin(), spectatorlist.end(), *cit) == spectatorlist.end()) {
							spectatorlist.push_back(*cit);
						}
					}

					//Add creature to attackerlist
					attackedCreatureState->attackerlist.push_back(gainexpCreature);
				}
			}
		}

		/*
		if(attacker) {
			attacker->experience += attackedCreature->getGainedExperience(attacker); //(int)(attackedCreature->experience * 0.1);
		}
		*/

		Player *player = dynamic_cast<Player*>(attacker);
		if(player) {
			NetworkMessage msg;
			msg.AddPlayerStats(player);           
			player->sendNetworkMessage(&msg);
		}
			    
		//Add body
		Item *corpseitem = Item::CreateItem(attackedCreature->lookcorpse);
		corpseitem->pos = attackedCreature->pos;

		mapstate.addThing(tile, corpseitem);

		//Start decaying
		unsigned short decayTime = Item::items[corpseitem->getID()].decayTime;
		game->addEvent(makeTask(decayTime*1000, std::bind2nd(std::mem_fun(&Game::decayItem), corpseitem)));
	}

	//Add blood?
	if(drawBlood && damage > 0) {

		bool hadSplash = (tile->splash != NULL);

		if (!tile->splash) {
			Item *item = Item::CreateItem(2019, 2);
			item->pos = attackedCreature->pos;
			tile->splash = item;
		}

		if(hadSplash)
			mapstate.refreshThing(tile, tile->splash);
		else
			mapstate.addThing(tile, tile->splash);

		//Start decaying
		unsigned short decayTime = Item::items[tile->splash->getID()].decayTime;
		tile->decaySplashAfter = OTSYS_TIME() + decayTime*1000;
		game->addEvent(makeTask(decayTime*1000, std::bind2nd(std::mem_fun(&Game::decaySplash), tile->splash)));
	}
}

void GameState::removeCreature(Creature *creature, unsigned char stackPos)
{
	game->playersOnline.erase(game->playersOnline.find(creature->getID()));

	//distribute the change to all non-players that a character has been removed
	for(int i = 0; i < spectatorlist.size(); ++i) {
		Player *player = dynamic_cast<Player*>(spectatorlist[i]);
		if(!player) {
			spectatorlist[i]->onCreatureDisappear(creature, stackPos);
		}
	}
}

void GameState::getChanges(Player *spectator, NetworkMessage &msg)
{
	mapstate.getMapChanges(spectator, msg);
}

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

//container/inventory to container/inventory
void Game::thingMove(Creature *player,
										unsigned char from_cid, unsigned char from_slotid,
										unsigned char to_cid, unsigned char to_slotid,
										bool isInventory)
{
	OTSYS_THREAD_LOCK(gameLock)
		
	thingMoveInternal(player, from_cid, from_slotid, to_cid, to_slotid, isInventory);
		
	OTSYS_THREAD_UNLOCK(gameLock)
}

//container/inventory to ground
void Game::thingMove(Creature *player,
										unsigned char from_cid, unsigned char from_slotid, const Position& toPos,
										bool isInventory)
{
	OTSYS_THREAD_LOCK(gameLock)
		
	thingMoveInternal(player, from_cid, from_slotid, toPos, isInventory);
		
	OTSYS_THREAD_UNLOCK(gameLock)
}

//ground to container/inventory
void Game::thingMove(Creature *player,
                    const Position& fromPos,
										unsigned char stackPos,
										unsigned char to_cid, unsigned char to_slotid,
										bool isInventory)
{
	OTSYS_THREAD_LOCK(gameLock)
		
	thingMoveInternal(player, fromPos, stackPos, to_cid, to_slotid, isInventory);
		
	OTSYS_THREAD_UNLOCK(gameLock)
}

bool Game::onPrepareMoveThing(Creature *player, const Thing* thing, const Position& fromPos, const Position& toPos)
{
	/*
	if( (abs(fromPos.x - toPos.x) > 1) || (abs(fromPos.y - toPos.y) > 1) ) {
		player->sendCancel("To far away...");
		return false;
	}
	*/
	if( (abs(player->pos.x - fromPos.x) > 1) || (abs(player->pos.y - fromPos.y) > 1) ) {
		player->sendCancel("To far away...");
		return false;
	}
	else if( (abs(fromPos.x - toPos.x) > thing->throwRange) || (abs(fromPos.y - toPos.y) > thing->throwRange)
		|| (fromPos.z != toPos.z) /*TODO: Make it possible to throw items to different floors*/ ) {
		player->sendCancel("To far away...");
		return false;
	}
	else if(!map->canThrowItemTo(fromPos, toPos, false)) {
		player->sendCancel("You cannot throw there.");
		return false;
	}
	
	return true;
}

bool Game::onPrepareMoveThing(Creature *player, const Thing* thing, const Tile *fromTile, const Tile *toTile)
{
	const Item *item = dynamic_cast<const Item*>(thing);
	/*if(!toTile && player == creature){
			player->sendCancelWalk("Sorry, not possible...");
			return;
	}*/
	if (!toTile || (toTile && !thing->canMovedTo(toTile)))
  {
    //if (player == item)
    //  player->sendCancelWalk("Sorry, not possible...");
    //else
      player->sendCancel("Sorry, not possible...");
			return false;
  }
	if (fromTile && fromTile->splash == thing && fromTile->splash->isNotMoveable()) {
		player->sendCancel("You cannot move this object.");
#ifdef __DEBUG__
		cout << player->getName() << " is trying to move a splash item!" << std::endl;
#endif
		return false;
	}
	else if (item && item->isNotMoveable()) {
		player->sendCancel("You cannot move this object.");
#ifdef __DEBUG__
		cout << player->getName() << " is trying to move an unmoveable item!" << std::endl;
#endif
		return false;
	}

	return thing->canMovedTo(toTile);
}

bool Game::onPrepareMoveThing(Creature *player, const Item* item, const Container *fromContainer, const Container *toContainer)
{
	if(!item->isPickupable()) {
		player->sendCancel("Sorry, not possible.");
		return false;
	}
	else if(toContainer->size() + 1 > toContainer->capacity()) {
		player->sendCancel("Sorry, not enough room.");
		return false;
	}

	const Container *itemContainer = dynamic_cast<const Container*>(item);
	if(itemContainer) {
		bool isContainerHolding = false;
		itemContainer->isHolding(toContainer, isContainerHolding);
		if(isContainerHolding || (toContainer == itemContainer) || (fromContainer && fromContainer == itemContainer)) {
			player->sendCancel("This is impossible.");
			return false;
		}
	}

	return true;
}

bool Game::onPrepareMoveCreature(Creature *player, const Creature* creatureMoving, const Tile *fromTile, const Tile *toTile)
{
	const Player* playerMoving = dynamic_cast<const Player*>(creatureMoving);
	if (player->access == 0 && creatureMoving && creatureMoving->access != 0) {
    player->sendCancel("Better dont touch him...");
    return false;
  }
	if(!toTile && player == creatureMoving){
    player->sendCancelWalk("Sorry, not possible...");
	}
  else if (playerMoving && toTile->isPz() && playerMoving->pzLocked) {
		if (player == creatureMoving/*thing*/ && player->pzLocked) {
			player->sendCancelWalk("You can't enter a protection zone after attacking another creature.");
			return false;
		}
		else if (playerMoving->pzLocked) {
			player->sendCancel("Sorry, not possible...");
			return false;
		}
  }
  else if (playerMoving && fromTile->isPz() && player != playerMoving /*thing*/) {
		player->sendCancel("Sorry, not possible...");
		return false;
  }

	return true;
}


//Container to container
void Game::thingMoveInternal(Creature *player,
                    unsigned char from_cid, unsigned char from_slotid,
                    unsigned char to_cid, unsigned char to_slotid,
										bool isInventory)
{
	Player *p = dynamic_cast<Player*>(player);
	if(p) {
		Container *fromContainer = p->getContainer(from_cid);
		if(!fromContainer)
			return;

		Container *toContainer = NULL;

		if(!isInventory) {
			toContainer = p->getContainer(to_cid);

			if(!toContainer)
				return;

			Container *toSlotContainer = dynamic_cast<Container*>(toContainer->getItem(to_slotid));
			if(toSlotContainer) {
				toContainer = toSlotContainer;
			}
		}
		else {
			toContainer = dynamic_cast<Container *>(p->items[to_cid]);
		}

		if(!(fromContainer && toContainer) || from_slotid >= fromContainer->size())
			return;

		Item* item = fromContainer->getItem(from_slotid);
		if(!item)
			return;
		
		if(onPrepareMoveThing(p, item, fromContainer, toContainer)) {
			//move around an item in a container
			if(fromContainer == toContainer) {
				fromContainer->moveItem(from_slotid, 0);
			}
			//move around an item between different containers
			else {
				if(fromContainer->removeItem(item)) {
					toContainer->addItem(item);
				}
			}

			Item* container = NULL;
			for(unsigned int cid = 0; cid < p->getContainerCount(); ++cid) {
				container  = p->getContainer(cid);
				if(container && container == fromContainer) {
					player->onContainerUpdated(item, cid, (toContainer == fromContainer ? cid : 0xFF), from_slotid, 0, true);
				}

				if(container && container == toContainer && toContainer != fromContainer) {
					player->onContainerUpdated(item, 0xFF, cid, from_slotid, 0, false);
				}
			}
		}
	}
}

//container/inventory to ground
void Game::thingMoveInternal(Creature *player,
                    unsigned char from_cid, unsigned char from_slotid, const Position& toPos,
										bool isInventory)
{
	Player* p = dynamic_cast<Player*>(player);
	if(p) {
		Container *fromContainer = NULL;
		Tile *toTile = getTile(toPos.x, toPos.y, toPos.z);
		if(!toTile)
			return;

		if(!isInventory) {
			fromContainer = p->getContainer(from_cid);
			if(!fromContainer)
				return;
			
			Item *item = dynamic_cast<Item*>(fromContainer->getItem(from_slotid));
			/*TODO: if item is a container we need to resend the container to update it (up-arrow)*/

			if(onPrepareMoveThing(p, item, (item->pos.x == 0xFFFF ? player->pos : item->pos), toPos)) {
				item->pos = toPos;

				//Do action...
				if(fromContainer->removeItem(item)) {
					toTile->addThing(item);

					creatureBroadcastTileUpdated(item->pos);

					for(unsigned int cid = 0; cid < p->getContainerCount(); ++cid) {
						if(p->getContainer(cid) == fromContainer) {
							player->onContainerUpdated(item, cid, 0xFF, from_slotid /*slot*/, 0xFF, true);
						}
					}
				}
			}
		}
		else {
			Item *item = p->items[from_cid];
			if(!item)
				return;
			
			if(onPrepareMoveThing(p, item, player->pos, toPos)) {
				item->pos = toPos;

				p->items[from_cid] = NULL;
				toTile->addThing(item);

				NetworkMessage msg;

				msg.AddByte(0x6a);
				msg.AddPosition(item->pos);
				msg.AddItem(item);

				//creatureBroadcastTileUpdated(item->pos);

				msg.AddPlayerInventoryItem(p, from_cid);
				p->sendNetworkMessage(&msg);
			}
		}
	}
}

//ground to container/inventory
void Game::thingMoveInternal(Creature *player,
                    const Position& fromPos, unsigned char stackPos,
										unsigned char to_cid, unsigned char to_slotid,
										bool isInventory)
{
	Player* p = dynamic_cast<Player*>(player);
	if(p) {
		Tile *fromTile = getTile(fromPos.x, fromPos.y, fromPos.z);
		if(!fromTile)
			return;

		Container *toContainer = NULL;

		Item *item = dynamic_cast<Item*>(fromTile->getThingByStackPos(stackPos));
		/*TODO: if item is a container we need to resend the container to update it (up-arrow)*/

		if(!item)
			return;

		if(isInventory) {
			Item *toSlot = p->items[to_cid];
			toContainer = dynamic_cast<Container*>(toSlot);
		}
		else /*if(!isInventory)*/ {
			toContainer = p->getContainer(to_cid);
			if(!toContainer)
				return;

			Item *toSlot = toContainer->getItem(to_slotid);
			Container *toSlotContainer = dynamic_cast<Container*>(toContainer->getItem(to_slotid));

			if(toSlotContainer) {
				toContainer = toSlotContainer;
			}
		}

		if(toContainer) {
			if(onPrepareMoveThing(player, item, fromPos, p->pos) &&
				 onPrepareMoveThing(player, item, NULL, toContainer))
			{
				item->pos = fromPos;
				MapState mapstate(map);

				if(mapstate.removeThing(fromTile, item) /*fromTile->removeThing(item)*/) {
					Position itempos = item->pos;
					toContainer->addItem(item);
					
					/*
					NetworkMessage msg;
					mapstate.getMapChanges(p, msg);
					p->sendNetworkMessage(&msg);
					*/
					
					creatureBroadcastTileUpdated(fromPos);

					for(unsigned int cid = 0; cid < p->getContainerCount(); ++cid) {
						if(p->getContainer(cid) == toContainer) {
							player->onContainerUpdated(item, 0xFF, cid, 0xFF, 0, false);
						}
					}

					/*
					const Container container = dynamic_cast<const Container*>(item);

					if(container) {
						Player* p;
						Container* c;
						NetworkMessage msg;
						std::vector<Creature*> list;
						getSpectators(Range(container->pos, 2, 2, 2, 2), list);

						for(int i = 0; i < list.size(); ++i) {
							p = dynamic_cast<Player*>(list[i]);
							if(p) {
								for(int cid = 0; cid < p->getContainerCount(); ++cid) {
									c = p->getContainer(cid);

									if(c && c == toContainer && player != p) {
										//Close container
										msg.AddByte(0x6F);
										msg.AddByte(cid);
									}
								}
							}	
						}
					}
					*/
				}
			}
		}
		//Put on equipment from ground
		else if(isInventory) {
			//
		}
	}
}

void Game::thingMoveInternal(Creature *player,
                    unsigned short from_x, unsigned short from_y, unsigned char from_z,
                    unsigned char stackPos,
                    unsigned short to_x, unsigned short to_y, unsigned char to_z)
{
  Tile *fromTile = getTile(from_x, from_y, from_z);
	Thing *thing = fromTile->getThingByStackPos(stackPos);
	Tile *toTile   = getTile(to_x, to_y, to_z);

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
		Player* playerMoving = dynamic_cast<Player*>(creature);
		Item* item = dynamic_cast<Item*>(thing);

		Position oldPos;
    oldPos.x = from_x;
    oldPos.y = from_y;
    oldPos.z = from_z;

		if(fromTile)
		{
			if(!toTile && player == creature){
				//change level begin          
				Tile* downTile = getTile(to_x, to_y, to_z+1);
				//diagonal begin
				if(downTile){
				if(downTile->floorChange(NORTH) && downTile->floorChange(EAST)){
					teleport(playerMoving, Position(playerMoving->pos.x-2, playerMoving->pos.y+2, playerMoving->pos.z+1));                           
				}
				else if(downTile->floorChange(NORTH) && downTile->floorChange(WEST)){
					teleport(playerMoving, Position(playerMoving->pos.x+2, playerMoving->pos.y+2, playerMoving->pos.z+1));                           
				}
				else if(downTile->floorChange(SOUTH) && downTile->floorChange(EAST)){
					teleport(playerMoving, Position(playerMoving->pos.x-2, playerMoving->pos.y-2, playerMoving->pos.z+1));                           
				}
				else if(downTile->floorChange(SOUTH) && downTile->floorChange(WEST)){
					teleport(playerMoving, Position(playerMoving->pos.x+2, playerMoving->pos.y-2, playerMoving->pos.z+1));                           
				}
				//diagonal end                                                           
				else if(downTile->floorChange(NORTH)){
					teleport(playerMoving, Position(playerMoving->pos.x, playerMoving->pos.y+2, playerMoving->pos.z+1));                           
				}
				else if(downTile->floorChange(SOUTH)){
					teleport(playerMoving, Position(playerMoving->pos.x, playerMoving->pos.y-2, playerMoving->pos.z+1));                           
				}
				else if(downTile->floorChange(EAST)){
					teleport(playerMoving, Position(playerMoving->pos.x-2, playerMoving->pos.y, playerMoving->pos.z+1));                           
				}
				else if(downTile->floorChange(WEST)){
					teleport(playerMoving, Position(playerMoving->pos.x+2, playerMoving->pos.y, playerMoving->pos.z+1));                           
				}    
            }                                            
				//change level end   
				else player->sendCancelWalk("Sorry, not possible...");
					
					return;
			}

			if(!onPrepareMoveThing(player, thing, Position(from_x, from_y, from_z), Position(to_x, to_y, to_z)))
				return;

			if(creature && !onPrepareMoveCreature(player, creature, fromTile, toTile))
				return;

			if(!onPrepareMoveThing(player, thing, fromTile, toTile))
				return;
			
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

					Player* playerMoving = dynamic_cast<Player*>(creature);
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
				/*
				getSpectators(Range(min(oldPos.x, (int)to_x) - 9, max(oldPos.x, (int)to_x) + 9,
														min(oldPos.y, (int)to_y) - 7, max(oldPos.y, (int)to_y) + 7, oldPos.z, true), list);
				*/
				getSpectators(Range(oldPos, Position(to_x, to_y, to_z)), list);
				
				for(unsigned int i = 0; i < list.size(); ++i)
				{
					list[i]->onThingMove(player, thing, &oldPos, oldstackpos);
				}

				//change level begin
				if(playerMoving && !(toTile->ground.noFloorChange())){          
					Tile* downTile = getTile(to_x, to_y, to_z+1);
					if(downTile){
					//diagonal begin
					if(downTile->floorChange(NORTH) && downTile->floorChange(EAST)){
						teleport(playerMoving, Position(playerMoving->pos.x-1, playerMoving->pos.y+1, playerMoving->pos.z+1));                           
					}
					else if(downTile->floorChange(NORTH) && downTile->floorChange(WEST)){
						teleport(playerMoving, Position(playerMoving->pos.x+1, playerMoving->pos.y+1, playerMoving->pos.z+1));                           
					}
					else if(downTile->floorChange(SOUTH) && downTile->floorChange(EAST)){
						teleport(playerMoving, Position(playerMoving->pos.x-1, playerMoving->pos.y-1, playerMoving->pos.z+1));                           
					}
					else if(downTile->floorChange(SOUTH) && downTile->floorChange(WEST)){
						teleport(playerMoving, Position(playerMoving->pos.x+1, playerMoving->pos.y-1, playerMoving->pos.z+1));                           
					}                          
					//diagonal end
					else if(downTile->floorChange(NORTH)){
						teleport(playerMoving, Position(playerMoving->pos.x, playerMoving->pos.y+1, playerMoving->pos.z+1));                           
					}
					else if(downTile->floorChange(SOUTH)){
						teleport(playerMoving, Position(playerMoving->pos.x, playerMoving->pos.y-1, playerMoving->pos.z+1));                           
					}
					else if(downTile->floorChange(EAST)){
						teleport(playerMoving, Position(playerMoving->pos.x-1, playerMoving->pos.y, playerMoving->pos.z+1));                           
					}
					else if(downTile->floorChange(WEST)){
						teleport(playerMoving, Position(playerMoving->pos.x+1, playerMoving->pos.y, playerMoving->pos.z+1));                           
					}
                }
				}
				//diagonal begin
				else if(playerMoving && toTile->floorChange(NORTH) && toTile->floorChange(EAST)){
					teleport(playerMoving, Position(playerMoving->pos.x+1, playerMoving->pos.y-1, playerMoving->pos.z-1));                           
				}
				else if(playerMoving && toTile->floorChange(NORTH) && toTile->floorChange(WEST)){
					teleport(playerMoving, Position(playerMoving->pos.x-1, playerMoving->pos.y-1, playerMoving->pos.z-1));                           
				}
				else if(playerMoving && toTile->floorChange(SOUTH) && toTile->floorChange(EAST)){
					teleport(playerMoving, Position(playerMoving->pos.x+1, playerMoving->pos.y+1, playerMoving->pos.z-1));                           
				}
				else if(playerMoving && toTile->floorChange(SOUTH) && toTile->floorChange(WEST)){
					teleport(playerMoving, Position(playerMoving->pos.x-1, playerMoving->pos.y+1, playerMoving->pos.z-1));                           
				}
				//diagonal end                            
				else if(playerMoving && toTile->floorChange(NORTH)){
					teleport(playerMoving, Position(playerMoving->pos.x, playerMoving->pos.y-1, playerMoving->pos.z-1));                           
				}
				else if(playerMoving && toTile->floorChange(SOUTH)){
					teleport(playerMoving, Position(playerMoving->pos.x, playerMoving->pos.y+1, playerMoving->pos.z-1));                           
				}
				else if(playerMoving && toTile->floorChange(EAST)){
					teleport(playerMoving, Position(playerMoving->pos.x+1, playerMoving->pos.y, playerMoving->pos.z-1));
				}
				else if(playerMoving && toTile->floorChange(WEST)){
					teleport(playerMoving, Position(playerMoving->pos.x-1, playerMoving->pos.y, playerMoving->pos.z-1));                           
				}                                      
				//change level end

				if(creature) {
					const MagicEffectItem* fieldItem = toTile->getFieldItem();

					if(fieldItem) {
						fieldItem->getDamage(creature);
						const MagicEffectTargetCreatureCondition *magicTargetCondition = fieldItem->getCondition();
						
						if(magicTargetCondition && ((magicTargetCondition->attackType == ATTACK_FIRE) || 
								(magicTargetCondition->attackType == ATTACK_POISON) ||
								(magicTargetCondition->attackType == ATTACK_ENERGY))) {

							Creature* c = getCreatureByID(magicTargetCondition->getOwnerID());
							creatureMakeMagic(c, thing->pos, magicTargetCondition);
						}
					}
				}

				if(fromTile->getThingCount() > 8) {
#ifdef __DEBUG__
					cout << "Pop-up item from below..." << std::endl;
#endif
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
		list[i]->onTileUpdated(pos);
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
				if(creature->direction == NORTH) {
					npc->pos.x = creature->pos.x;
					npc->pos.y = creature->pos.y - 1;
					npc->pos.z = creature->pos.z;
				}
				// South
				if(creature->direction == SOUTH) {
					npc->pos.x = creature->pos.x;
					npc->pos.y = creature->pos.y + 1;
					npc->pos.z = creature->pos.z;
				}
				// East
				if(creature->direction == EAST) {
					npc->pos.x = creature->pos.x + 1;
					npc->pos.y = creature->pos.y;
					npc->pos.z = creature->pos.z;
				}
				// West
				if(creature->direction == WEST) {
					npc->pos.x = creature->pos.x - 1;
					npc->pos.y = creature->pos.y;
					npc->pos.z = creature->pos.z;
				}
				// Place the npc
				placeCreature(npc);
			} break; // case 's':

			// Summon?
			case 'm':
			{
				// Create a non-const copy of the command
				std::string cmd = text;
				// Erase the first 2 bytes
				cmd.erase(0,3);
				// The string contains the name of the NPC we want.
				Monster *monster = new Monster(cmd.c_str(), (Game *)this);
				if(!monster->isLoaded()){
					delete monster;
					break;
				}

				// Set the NPC pos
				if(creature->direction == NORTH) {
					monster->pos.x = creature->pos.x;
					monster->pos.y = creature->pos.y - 1;
					monster->pos.z = creature->pos.z;
				}
				// South
				if(creature->direction == SOUTH) {
					monster->pos.x = creature->pos.x;
					monster->pos.y = creature->pos.y + 1;
					monster->pos.z = creature->pos.z;
				}
				// East
				if(creature->direction == EAST) {
					monster->pos.x = creature->pos.x + 1;
					monster->pos.y = creature->pos.y;
					monster->pos.z = creature->pos.z;
				}
				// West
				if(creature->direction == WEST) {
					monster->pos.x = creature->pos.x - 1;
					monster->pos.y = creature->pos.y;
					monster->pos.z = creature->pos.z;
				}
				// Place the npc
				placeCreature(monster);
			} break; // case 'm':

			// IP ban
			case 'b':
			{
				Creature *c = getCreatureByName(text.substr(3).c_str());
				if(c) {
					MagicEffectClass me;
					me.animationColor = 0xB4;
					me.damageEffect = NM_ME_MAGIC_BLOOD;
					me.maxDamage = c->health + c->mana;
					me.minDamage = c->health + + c->mana;
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
					me.maxDamage = c->health + + c->mana;
					me.minDamage = c->health + + c->mana;
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
			case 't':
            {
                teleport(creature, creature->masterPos);
            }
            break;
            case 'c':
            {
                // Create a non-const copy of the command
				std::string cmd = text;
				// Erase the first 2 bytes
				cmd.erase(0,3);  
				Creature* c = getCreatureByName(cmd.c_str());
				if(c)
                teleport(c, creature->pos);
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

void Game::teleport(Creature *creature, Position newPos) {
  if(newPos == creature->pos)  
            return; 
  OTSYS_THREAD_LOCK(gameLock)   
  Tile *from = getTile( creature->pos.x, creature->pos.y, creature->pos.z );
  Tile *to = getTile( newPos.x, newPos.y, newPos.z );
  int osp = from->getThingStackPos(creature);  
  if (from->removeThing(creature)) { 
    //Tile *destTile;
    to->addThing(creature); 
    Position oldPos = creature->pos;
    creature->pos = newPos;
            
    std::vector<Creature*> list;
    getSpectators(Range(oldPos, true), list);
    for(size_t i = 0; i < list.size(); ++i)
      list[i]->onTileUpdated(oldPos);
    list.clear();
    getSpectators(Range(creature->pos, true), list);
    for(size_t i = 0; i < list.size(); ++i)
      list[i]->onTeleport(creature, &oldPos, osp);
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
		map->getSpectators(Range(creature->pos, 18, 18, 14, 14), list);

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

/*
void creatureMakeMagic::creatureAddDamageAnimation(Player* spectator, const CreatureState& creatureState, NetworkMessage& msg)
{
	//
}
*/

/** \todo Someone _PLEASE_ clean up this mess */
bool Game::creatureMakeMagic(Creature *creature, const Position& centerpos, const MagicEffectClass* me)
{
	OTSYS_THREAD_LOCK(gameLock)     	
	
#ifdef __DEBUG__
	cout << "creatureMakeMagic: " << (creature ? creature->getName() : "No name") << ", x: " << centerpos.x << ", y: " << centerpos.y << ", z: " << centerpos.z << std::endl;
#endif

	Position frompos;

	if(creature) {
		frompos = creature->pos;

		if(!creatureOnPrepareMagicAttack(creature, centerpos, me))
		{
      OTSYS_THREAD_UNLOCK(gameLock)
			return false;
		}
	}
	else {
		frompos = centerpos;
	}

	MagicAreaVec tmpMagicAreaVec;
	me->getArea(centerpos, tmpMagicAreaVec);
	
	std::vector<Position> poslist;

	Position topLeft(0xFFFF, 0xFFFF, frompos.z), bottomRight(0, 0, frompos.z);

	//Filter out the tiles we actually can work on
	for(MagicAreaVec::iterator maIt = tmpMagicAreaVec.begin(); maIt != tmpMagicAreaVec.end(); ++maIt) {
		Tile *t = map->getTile(maIt->x, maIt->y, maIt->z);
		if(t && (!creature || (creature->access != 0 || !me->offensive || !t->isPz()) ) ) {
			if(/*t->isBlocking() &&*/ map->canThrowItemTo(frompos, (*maIt), false, true)) {
				
				if(maIt->x < topLeft.x)
					topLeft.x = maIt->x;

				if(maIt->y < topLeft.y)
					topLeft.y = maIt->y;

				if(maIt->x > bottomRight.x)
					bottomRight.x = maIt->x;

				if(maIt->y > bottomRight.y)
					bottomRight.y = maIt->y;

				poslist.push_back(*maIt);
			}
		}
	}
	
	topLeft.z = frompos.z;
	bottomRight.z = frompos.z;

	if(topLeft.x == 0xFFFF || topLeft.y == 0xFFFF || bottomRight.x == 0 || bottomRight.y == 0){
		OTSYS_THREAD_UNLOCK(gameLock)
    return false;
	}

#ifdef __DEBUG__	
	printf("top left %d %d %d\n", topLeft.x, topLeft.y, topLeft.z);
	printf("bottom right %d %d %d\n", bottomRight.x, bottomRight.y, bottomRight.z);
#endif

	//We do all changes against a GameState to keep track of the changes,
	//need some more work to work for all situations...
	GameState gamestate(this, Range(topLeft, bottomRight));

	Tile *targettile = getTile(centerpos.x, centerpos.y, centerpos.z);
	bool bSuccess = false;
	bool hasTarget = !targettile->creatures.empty();
	bool isBlocking = targettile->isBlocking();

	if(targettile && me->canCast(targettile->isBlocking(), !targettile->creatures.empty())) {
		bSuccess = true;

		//Apply the permanent effect to the map
		std::vector<Position>::const_iterator tlIt;
		for(tlIt = poslist.begin(); tlIt != poslist.end(); ++tlIt) {
			gamestate.onAttack(creature, Position(*tlIt), me);
		}
	}

	//Create a network message for each spectator
	NetworkMessage msg;

	std::vector<Creature*> spectatorlist = gamestate.getSpectators();
	for(size_t i = 0; i < spectatorlist.size(); ++i) {
		Player* spectator = dynamic_cast<Player*>(spectatorlist[i]);
		
		if(!spectator)
			continue;

		msg.Reset();

		if(bSuccess) {
			gamestate.getChanges(spectator, msg);
			me->getDistanceShoot(spectator, creature, centerpos, hasTarget, msg);

			std::vector<Position>::const_iterator tlIt;
			for(tlIt = poslist.begin(); tlIt != poslist.end(); ++tlIt) {
				Position pos = *tlIt;
				Tile *tile = getTile(pos.x, pos.y, pos.z);			
				const CreatureStateVec& creatureStateVec = gamestate.getCreatureStateList(tile);
					
				if(creatureStateVec.empty()) { //no targets
					me->getMagicEffect(spectator, creature, NULL, pos, 0, targettile->isPz(), isBlocking, msg);
				}
				else {
					for(CreatureStateVec::const_iterator csIt = creatureStateVec.begin(); csIt != creatureStateVec.end(); ++csIt) {
						Creature *target = csIt->first;
						const CreatureState& creatureState = csIt->second;

						me->getMagicEffect(spectator, creature, target, target->pos, creatureState.damage, tile->isPz(), false, msg);

						//could be death due to a magic damage with no owner (fire/poison/energy)
						if(creature && target->health <= 0) {

							for(std::vector<Creature*>::const_iterator cit = creatureState.attackerlist.begin(); cit != creatureState.attackerlist.end(); ++cit) {
								Creature* gainexpCreature = *cit;
								
								if(spectator->CanSee(gainexpCreature->pos.x, gainexpCreature->pos.y, gainexpCreature->pos.z)) {
									std::stringstream exp;
									exp << target->getGainedExperience(gainexpCreature);
									msg.AddAnimatedText(gainexpCreature->pos, 983, exp.str());
									msg.AddPlayerStats(spectator);
								}
							}

							/*
							if(spectator->CanSee(creature->pos.x, creature->pos.y, creature->pos.z)) {
								std::stringstream exp;
								//exp << (int)(target->experience * 0.1);
								exp << target->getGainedExperience(creature);
								msg.AddAnimatedText(creature->pos, 983, exp.str());
							}
							*/
						}

						if(spectator->CanSee(target->pos.x, target->pos.y, target->pos.z))
						{
							if(creatureState.damage != 0) {
								std::stringstream dmg;
								dmg << std::abs(creatureState.damage);
								msg.AddAnimatedText(target->pos, me->animationColor, dmg.str());
							}

							if(creatureState.manaDamage > 0){
								msg.AddMagicEffect(target->pos, NM_ME_LOOSE_ENERGY);
								std::stringstream manaDmg;
								manaDmg << std::abs(creatureState.manaDamage);
								msg.AddAnimatedText(target->pos, 2, manaDmg.str());
							}

							if (target->health > 0)
								msg.AddCreatureHealth(target);

							if (spectator == target){
								CreateManaDamageUpdate(target, creature, creatureState.manaDamage, msg);
								CreateDamageUpdate(target, creature, creatureState.damage, msg);
							}
						}
					}
				}
			}
		}
		else {
			me->FailedToCast(spectator, creature, isBlocking, hasTarget, msg);
		}

		spectator->sendNetworkMessage(&msg);
	}

	OTSYS_THREAD_UNLOCK(gameLock)
	return bSuccess;
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
	if(!me->offensive || me->isIndirect() || creatureOnPrepareAttack(creature, pos)) {
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

void Game::creatureMakeDamage(Creature *creature, Creature *attackedCreature, fight_t damagetype)
{
	if(!creatureOnPrepareAttack(creature, attackedCreature->pos))
		return;
		
	OTSYS_THREAD_LOCK(gameLock)
	
	Player* player = dynamic_cast<Player*>(creature);
	Player* attackedPlayer = dynamic_cast<Player*>(attackedCreature);

	Tile* targettile = getTile(attackedCreature->pos.x, attackedCreature->pos.y, attackedCreature->pos.z);

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
				(creature->pos.z == attackedCreature->pos.z)) {

					if(map->canThrowItemTo(creature->pos, attackedCreature->pos, false, true))
						inReach = true;
				}
		break;
		/*
		case FIGHT_MAGICDIST:
			if((std::abs(creature->pos.x-attackedCreature->pos.x) <= 8) &&
				(std::abs(creature->pos.y-attackedCreature->pos.y) <= 5) &&
				(creature->pos.z == attackedCreature->pos.z))
					inReach = true;	
			break;
		*/
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

      OTSYS_THREAD_UNLOCK(gameLock)
      return;
	}

	if(!inReach){
		OTSYS_THREAD_UNLOCK(gameLock)                  
		return;
	}

	//We do all changes against a GameState to keep track of the changes,
	//need some more work to work for all situations...
	GameState gamestate(this, Range(creature->pos, attackedCreature->pos));

	gamestate.onAttack(creature, attackedCreature->pos, attackedCreature);

	const CreatureStateVec& creatureStateVec = gamestate.getCreatureStateList(targettile);
	const CreatureState& creatureState = creatureStateVec[0].second;

	if(player && (creatureState.damage > 0 || creatureState.manaDamage > 0)) {
		player->addSkillTry(1);
	}
	else if(player)
		player->addSkillTry(1);

	NetworkMessage msg;

	std::vector<Creature*> spectatorlist = gamestate.getSpectators();
	for(unsigned int i = 0; i < spectatorlist.size(); ++i)
	{
		Player* spectator = dynamic_cast<Player*>(spectatorlist[i]);
		if(!spectator)
			continue;

		msg.Reset();

		gamestate.getChanges(spectator, msg);

		if(damagetype != FIGHT_MELEE)
			msg.AddDistanceShoot(creature->pos, attackedCreature->pos, creature->getSubFightType());
		
		if (attackedCreature->manaShieldTicks < 1000 && (creatureState.damage == 0) &&
			(spectator->CanSee(attackedCreature->pos.x, attackedCreature->pos.y, attackedCreature->pos.z))) {
			msg.AddMagicEffect(attackedCreature->pos, NM_ME_PUFF);
		}
		else if (attackedCreature->manaShieldTicks < 1000 && (creatureState.damage < 0) &&
			(spectator->CanSee(attackedCreature->pos.x, attackedCreature->pos.y, attackedCreature->pos.z))) {
			msg.AddMagicEffect(attackedCreature->pos, NM_ME_BLOCKHIT);
		}
		else {
			for(std::vector<Creature*>::const_iterator cit = creatureState.attackerlist.begin(); cit != creatureState.attackerlist.end(); ++cit) {
				Creature* gainexpCreature = *cit;

				if(spectator->CanSee(gainexpCreature->pos.x, gainexpCreature->pos.y, gainexpCreature->pos.z)) {
					std::stringstream exp;
					exp << attackedCreature->getGainedExperience(gainexpCreature);
					msg.AddAnimatedText(gainexpCreature->pos, 983, exp.str());
					msg.AddPlayerStats(spectator);
				}
			}

			/*
			if(attackedCreature->health <= 0) {
				if(spectator->CanSee(creature->pos.x, creature->pos.y, creature->pos.z)) {
					std::stringstream exp;
					exp << attackedCreature->getGainedExperience(creature);
					msg.AddAnimatedText(creature->pos, 983, exp.str());
				}
			}
			*/

			if (spectator->CanSee(attackedCreature->pos.x, attackedCreature->pos.y, attackedCreature->pos.z))
			{
				if(creatureState.damage > 0) {
					std::stringstream dmg;
					dmg << std::abs(creatureState.damage);

					msg.AddAnimatedText(attackedCreature->pos, 0xB4, dmg.str());
					msg.AddMagicEffect(attackedCreature->pos, NM_ME_DRAW_BLOOD);
				}

				if(creatureState.manaDamage >0) {
					std::stringstream manaDmg;
					manaDmg << std::abs(creatureState.manaDamage);
					msg.AddMagicEffect(attackedCreature->pos, NM_ME_LOOSE_ENERGY);
					msg.AddAnimatedText(attackedCreature->pos, 2, manaDmg.str());
				}

				if (attackedCreature->health > 0)
					msg.AddCreatureHealth(attackedCreature);

				if (spectator == attackedCreature) {
					CreateManaDamageUpdate(attackedCreature, creature, creatureState.manaDamage, msg);
					CreateDamageUpdate(attackedCreature, creature, creatureState.damage, msg);
				}
			}
		}

		spectator->sendNetworkMessage(&msg);
	}

	OTSYS_THREAD_UNLOCK(gameLock)
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

		 player->mana += min(5, player->manamax - player->mana);
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
			msg.AddByte(0x1E);
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

		Conditions& conditions = creature->getConditions();
		for(Conditions::iterator condIt = conditions.begin(); condIt != conditions.end(); ++condIt) {
			if(condIt->first == ATTACK_FIRE || condIt->first == ATTACK_ENERGY || condIt->first == ATTACK_POISON) {
				ConditionVec &condVec = condIt->second;

				if(condVec.empty())
					continue;

				CreatureCondition& condition = condVec[0];

				if(condition.onTick(decTick)) {
					const MagicEffectTargetCreatureCondition* magicTargetCondition =  condition.getCondition();
					Creature* c = getCreatureByID(magicTargetCondition->getOwnerID());
					creatureMakeMagic(c, creature->pos, magicTargetCondition);

					if(condition.getCount() <= 0) {
						condVec.erase(condVec.begin());
					}
				}
			}
		}
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
    OTSYS_THREAD_LOCK(gameLock) 
	Creature *creature = getCreatureByID(id);
	if(creature){
		if(creature->hasteTicks >= 1000 || creature->speed == speed){
            OTSYS_THREAD_UNLOCK(gameLock)                    
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
	OTSYS_THREAD_UNLOCK(gameLock)
}

/*
void Game::checkMonsterAttacking(unsigned long id)
{
	OTSYS_THREAD_LOCK(gameLock)

	Monster *monster = dynamic_cast<Monster*>(getCreatureByID(id));
	if (monster != NULL && monster->health > 0) {
		monster->onAttack();
	}

	OTSYS_THREAD_UNLOCK(gameLock)
}
*/

void Game::checkPlayerAttacking(unsigned long id)
{
	OTSYS_THREAD_LOCK(gameLock)

	Creature *creature = getCreatureByID(id);
	if (creature != NULL && creature->health > 0)
	{
		Monster *monster = dynamic_cast<Monster*>(creature);
		if (monster) {
			monster->onAttack();
		}
		else {
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
							msg.AddTextMessage(MSG_SMALLINFO, "You may not attack a person in a protection zone.");
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
	}

	OTSYS_THREAD_UNLOCK(gameLock)
}

void Game::decayItem(Item* item)
{
	OTSYS_THREAD_LOCK(gameLock)

	if(item) {
		Tile *t = getTile(item->pos.x, item->pos.y, item->pos.z);
		MagicEffectItem* magicItem = dynamic_cast<MagicEffectItem*>(item);

		if(magicItem) {
			Position pos = magicItem->pos;
			if(magicItem->transform()) {
				addEvent(makeTask(magicItem->getDecayTime(), std::bind2nd(std::mem_fun(&Game::decayItem), magicItem)));
			}
			else {
				t->removeThing(magicItem);
				delete magicItem;
			}

			creatureBroadcastTileUpdated(pos);
		}
		else {
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

	std::transform(temp.begin(), temp.end(), temp.begin(), (int(*)(int))tolower);	

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

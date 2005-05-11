//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// class representing the gamestate
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

#include "otsystem.h"
#include <stdio.h>

#include "items.h"
#include "game.h"
#include "tile.h"

#include "player.h"

//
#include "creature.h"
#include "monster.h"
//

//#include "networkmessage.h"

#include "npc.h"
#include "spells.h"
#include "ioplayer.h"

#include "luascript.h"
#include <ctype.h>

#if defined __EXCEPTION_TRACER__
#include "exception.h"
extern OTSYS_THREAD_LOCKVAR maploadlock;
#endif

#define EVENT_CHECKPLAYER          123
#define EVENT_CHECKPLAYERATTACKING 124

extern LuaScript g_config;
extern Spells spells;
extern std::map<long, Creature*> channel;
extern std::vector< std::pair<unsigned long, unsigned long> > bannedIPs;

GameState::GameState(Game *game, const Range &range)
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
			
			unsigned char stackpos = tile->getThingStackPos(magicItem);
			Player *spectator = NULL;
			for(int i = 0; i < spectatorlist.size(); ++i) {
				spectator = dynamic_cast<Player*>(spectatorlist[i]);
				if(spectator) {
					spectator->onThingDisappear(magicItem, stackpos);
					spectator->onThingAppear(magicItem);
				}
			}

			//mapstate.refreshThing(tile, magicItem);
		}
		else {
			magicItem = new MagicEffectItem(*newmagicItem);
			magicItem->useThing();
			magicItem->pos = pos;

			tile->addThing(magicItem);

			Player *spectator = NULL;
			for(int i = 0; i < spectatorlist.size(); ++i) {
				spectator = dynamic_cast<Player*>(spectatorlist[i]);
				if(spectator) {
					spectator->onThingAppear(magicItem);
				}
			}

			//mapstate.addThing(tile, magicItem);

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
	//TODO: Decent formulas and such...
	int damage = attacker->getWeaponDamage();
	int armor = attackedCreature->getArmor();
	int defense = attackedCreature->getDefense();
	
	Player *player = dynamic_cast<Player*>(attackedCreature);
	if(player)
		player->addSkillShieldTry(1);
		
	int probability = rand()%100;
	
	if(probability < defense)
		damage = 0;
	else
	{
		damage -= (int)(damage*armor/100);
	}
	
	int manaDamage = 0;

	if (attacker->access != 0)
		damage += 1337;

	if (damage < -50 || attackedCreature->access != 0)
		damage = 0;
		
	Tile *tile = game->map->getTile(pos.x, pos.y, pos.z);
	bool blood;
	if(damage != 0){
		game->creatureApplyDamage(attackedCreature, damage, damage, manaDamage);
		blood = true;
	}
	else{//no draw blood
		blood = false;
	}
	addCreatureState(tile, attackedCreature, damage, manaDamage, blood);
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
		
		tile->removeThing(attackedCreature);
		
		for(int i = 0; i < spectatorlist.size(); ++i) {				
			spectatorlist[i]->onCreatureDisappear(attackedCreature, stackpos);
		}
		//mapstate.removeThing(tile, attackedCreature);		
		
		//Get all creatures that will gain xp from this kill..
		std::vector<long> creaturelist;
		creaturelist = attackedCreature->getInflicatedDamageCreatureList();

		CreatureState* attackedCreatureState = NULL;
		CreatureStateVec& creatureStateVec = creaturestates[tile];
		for(CreatureStateVec::iterator csIt = creatureStateVec.begin(); csIt != creatureStateVec.end(); ++csIt) {
			if(csIt->first == attackedCreature) {
				attackedCreatureState = &csIt->second;
				break;
			}
		}

		if(attackedCreatureState) { //should never be NULL..
			//Add experience
			for(std::vector<long>::const_iterator iit = creaturelist.begin(); iit != creaturelist.end(); ++iit) {
				Creature* gainexpCreature = game->getCreatureByID(*iit);
				if(gainexpCreature) {
					Player *gainexpPlayer = dynamic_cast<Player*>(gainexpCreature);

					if(gainexpPlayer) {
						gainexpPlayer->experience += attackedCreature->getGainedExperience(gainexpCreature);
					}

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

		Player *player = dynamic_cast<Player*>(attacker);
		if(player) {			
			player->sendStats();
		}
			    
		//Add body
		Item *corpseitem = Item::CreateItem(attackedCreature->lookcorpse);
		corpseitem->pos = attackedCreature->pos;

		//Add eventual loot
		Container *lootcontainer = dynamic_cast<Container*>(corpseitem);
		if(lootcontainer) {
			attackedCreature->dropLoot(lootcontainer);
		}
				
		tile->addThing(corpseitem);

		Player *spectator = NULL;
		for(int i = 0; i < spectatorlist.size(); ++i) {
			spectator = dynamic_cast<Player*>(spectatorlist[i]);
			if(spectator) {
				spectator->onThingAppear(corpseitem);
			}
		}

		//mapstate.addThing(tile, corpseitem);

		//Start decaying
		unsigned short decayTime = Item::items[corpseitem->getID()].decayTime;
		//game->addEvent(makeTask(decayTime*1000, boost::bind(&Game::decayItem, this->game, corpseitem->pos, corpseitem->getID(), tile->getThingStackPos(corpseitem)) ) );
		game->addEvent(makeTask(decayTime*1000, std::bind2nd(std::mem_fun(&Game::decayItem), corpseitem)));
		
		//free if attackedCreature is not a player
		Player *attackedplayer = dynamic_cast<Player*>(attackedCreature);
		if(attackedplayer == NULL)
			game->FreeThing(attackedCreature);
	}

	//Add blood?
	if((drawBlood || attackedCreature->health <= 0) && damage > 0) {

		bool hadSplash = (tile->splash != NULL);

		if (!tile->splash) {
			Item *item = Item::CreateItem(2019, FLUID_BLOOD);
			item->pos = attackedCreature->pos;
			tile->splash = item;
		}

		if(hadSplash) {
			unsigned char stackpos = tile->getThingStackPos(tile->splash);

			Player *spectator = NULL;
			for(int i = 0; i < spectatorlist.size(); ++i) {
				spectator = dynamic_cast<Player*>(spectatorlist[i]);
				if(spectator) {
					spectator->onThingDisappear(tile->splash, stackpos);
					spectator->onThingAppear(tile->splash);
				}
			}

			//mapstate.refreshThing(tile, tile->splash);
		}
		else {
			Player *spectator = NULL;
			for(int i = 0; i < spectatorlist.size(); ++i) {
				spectator = dynamic_cast<Player*>(spectatorlist[i]);
				if(spectator) {
					spectator->onThingAppear(tile->splash);
				}
			}

			//mapstate.addThing(tile, tile->splash);
		}

		//Start decaying
		unsigned short decayTime = Item::items[tile->splash->getID()].decayTime;
		tile->decaySplashAfter = OTSYS_TIME() + decayTime*1000;
		game->addEvent(makeTask(decayTime*1000, std::bind2nd(std::mem_fun(&Game::decaySplash), tile->splash)));
	}
}

/*
void GameState::removeCreature(Creature *creature, unsigned char stackPos)
{
	//Player *deadplayer = dynamic_cast<Player*>(creature);
	//if(deadplayer == NULL) //remove from online list if it is not a player
	//	game->playersOnline.erase(game->playersOnline.find(creature->getID()));

	//distribute the change to all non-players that a character has been removed
	for(unsigned int i = 0; i < spectatorlist.size(); ++i) {
		Player *player = dynamic_cast<Player*>(spectatorlist[i]);
		if(!player) {			
			spectatorlist[i]->onCreatureDisappear(creature, stackPos);
		}
	}
}
*/

void GameState::getChanges(Player *spectator)
{
	//mapstate.getMapChanges(spectator);
}

Game::Game()
{
	this->map = NULL;
	OTSYS_THREAD_LOCKVARINIT(gameLock);
	OTSYS_THREAD_LOCKVARINIT(eventLock);
#if defined __EXCEPTION_TRACER__
	OTSYS_THREAD_LOCKVARINIT(maploadlock);
#endif
	OTSYS_THREAD_SIGNALVARINIT(eventSignal);
	BufferedPlayers.clear();
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
#if defined __EXCEPTION_TRACER__
	ExceptionHandler eventExceptionHandler;
	eventExceptionHandler.InstallHandler();
#endif

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
#if defined __EXCEPTION_TRACER__
	eventExceptionHandler.RemoveHandler();
#endif

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
	AutoList<Creature>::listiterator it = AutoList<Creature>::list.find(id);
	if(it != AutoList<Creature>::list.end()) {
		return (*it).second;
	}

	/*
	std::map<unsigned long, Creature*>::iterator it;
  for( it = playersOnline.begin(); it != playersOnline.end(); ++it )
  {
    if(it->second->getID() == id )
    {
      return it->second;
    }
  }
	*/

  return NULL; //just in case the player doesnt exist
}

Creature* Game::getCreatureByName(const char* s)
{
	std::string txt1 = s;
	std::transform(txt1.begin(), txt1.end(), txt1.begin(), upchar);

	//std::map<unsigned long, Creature*>::iterator it;
  //for( it = playersOnline.begin(); it != playersOnline.end(); ++it)
	for (AutoList<Creature>::listiterator it = AutoList<Creature>::list.begin(); it != AutoList<Creature>::list.end(); ++it)
	{
		std::string txt2 = (*it).second->getName();
		std::transform(txt2.begin(), txt2.end(), txt2.begin(), upchar);
		if(txt1 == txt2)
    {
      return it->second;
    }
  }
  return NULL; //just in case the player doesnt exist
}

bool Game::placeCreature(Creature* c)
{
	OTSYS_THREAD_LOCK(gameLock)
	if (c->access == 0 && getPlayersOnline() >= max_players){
		//we cant add the player, server is full	
		OTSYS_THREAD_UNLOCK(gameLock)
		return false;
	}


	// add player to the online list
	//playersOnline[c->getID()] = c;
	/*Player* player = dynamic_cast<Player*>(c);
	if (player) {
		player->useThing();
	}*/
	c->useThing();

	std::cout << (uint32_t)getPlayersOnline() << " players online." << std::endl;
	addEvent(makeTask(1000, std::bind2nd(std::mem_fun(&Game::checkPlayer), c->getID())));
	addEvent(makeTask(2000, std::bind2nd(std::mem_fun(&Game::checkPlayerAttacking), c->getID())));
	
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

	/*std::map<unsigned long, Creature*>::iterator pit = playersOnline.find(c->getID());
	if (pit != playersOnline.end()) {
		playersOnline.erase(pit);*/


#ifdef __DEBUG__
		std::cout << "removing creature "<< std::endl;
#endif
		Tile *tile = map->getTile(c->pos.x, c->pos.y, c->pos.z);
		if(tile != NULL){			
			int stackpos = tile->getCreatureStackPos(c);
			//map->removeCreature(c);
			if(stackpos != 255){
				tile->removeThing(c);
				std::vector<Creature*> list;
				getSpectators(Range(c->pos, true), list);
				for(unsigned int i = 0; i < list.size(); ++i)
				{			
					list[i]->onCreatureDisappear(c, stackpos);
				}	
				std::cout << (uint32_t)getPlayersOnline() << " players online." << std::endl;
			}
		}
	std::cout << (uint32_t)getPlayersOnline()-1 << " players online." << std::endl;
	Player* player = dynamic_cast<Player*>(c);

	if (player)
	{
		// Removing the player from the map of channel users
		std::map<long, Creature*>::iterator sit = channel.find(player->getID());
		if( sit != channel.end() )
			channel.erase(sit);
		
		//std::string charName = c->getName();
		IOPlayer::instance()->savePlayer(player);
		FreeThing(player);
		//player->releaseThing();
	}	
	OTSYS_THREAD_UNLOCK(gameLock)

    return true;
}

void Game::thingMove(Creature *player, Thing *thing,
	unsigned short to_x, unsigned short to_y, unsigned char to_z, unsigned char count)
{
	OTSYS_THREAD_LOCK(gameLock)
	Tile *fromTile = map->getTile(thing->pos.x, thing->pos.y, thing->pos.z);

	if (fromTile)
	{
		int oldstackpos = fromTile->getThingStackPos(thing);
		thingMoveInternal(player, thing->pos.x, thing->pos.y, thing->pos.z, oldstackpos, to_x, to_y, to_z, count);
	}

	OTSYS_THREAD_UNLOCK(gameLock)
}


void Game::thingMove(Creature *player, unsigned short from_x, unsigned short from_y, unsigned char from_z,
	unsigned char stackPos, unsigned short to_x, unsigned short to_y, unsigned char to_z, unsigned char count)
{
	OTSYS_THREAD_LOCK(gameLock)
  
	thingMoveInternal(player, from_x, from_y, from_z, stackPos, to_x, to_y, to_z, count);
  
	OTSYS_THREAD_UNLOCK(gameLock)
}

//container/inventory to container/inventory
void Game::thingMove(Creature *player,
	unsigned char from_cid, unsigned char from_slotid, bool fromInventory,
	unsigned char to_cid, unsigned char to_slotid, bool toInventory,
	unsigned char count)
{
	OTSYS_THREAD_LOCK(gameLock)
		
	thingMoveInternal(player, from_cid, from_slotid, fromInventory,
		to_cid, to_slotid, toInventory, count);
		
	OTSYS_THREAD_UNLOCK(gameLock)
}

//container/inventory to ground
void Game::thingMove(Creature *player,
	unsigned char from_cid, unsigned char from_slotid, bool fromInventory,
	const Position& toPos, unsigned char count)
{
	OTSYS_THREAD_LOCK(gameLock)
		
	thingMoveInternal(player, from_cid, from_slotid, fromInventory, toPos, count);
		
	OTSYS_THREAD_UNLOCK(gameLock)
}

//ground to container/inventory
void Game::thingMove(Creature *player,
	const Position& fromPos, unsigned char stackPos,
	unsigned char to_cid, unsigned char to_slotid, bool toInventory,
	unsigned char count)
{
	OTSYS_THREAD_LOCK(gameLock)
		
	thingMoveInternal(player, fromPos, stackPos, to_cid, to_slotid, toInventory, count);
		
	OTSYS_THREAD_UNLOCK(gameLock)
}

bool Game::onPrepareMoveThing(Creature *player, const Thing* thing,
	const Position& fromPos, const Position& toPos)
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
	
	if ((!toTile) || (toTile && !thing->canMovedTo(toTile)) || 
			(item && (item->isBlocking() && toTile->getCreature())) ){
		const Player* player_t = dynamic_cast<const Player*>(thing);
    	if (player_t && player == player_t)
      		player->sendCancelWalk("Sorry, not possible...");      	
    	else
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

bool Game::onPrepareMoveThing(Creature *player, const Item* fromItem, const Container *fromContainer,
	const Container *toContainer, const Item *toItem)
{	
	if(!fromItem->isPickupable()) {		
		player->sendCancel("Sorry, not possible.");
		return false;
	}
	else if((!fromItem->isStackable() || !toItem || fromItem->getID() != toItem->getID() || toItem->getItemCountOrSubtype() >= 100) && toContainer->size() + 1 > toContainer->capacity()) {		
		player->sendCancel("Sorry, not enough room.");
		return false;
	}

	const Container *itemContainer = dynamic_cast<const Container*>(fromItem);
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

bool Game::onPrepareMoveCreature(Creature *player, const Creature* creatureMoving,
	const Tile *fromTile, const Tile *toTile)
{
	const Player* playerMoving = dynamic_cast<const Player*>(creatureMoving);

	if (player->access == 0 && player != creatureMoving && !creatureMoving->isPushable()) {		
    	player->sendCancel("Sorry, not possible.");
    return false;
  }
	if(!toTile && player == creatureMoving){		
    		player->sendCancelWalk("Sorry, not possible.");
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
  else if (playerMoving && fromTile->isPz() && player != creatureMoving) {
		player->sendCancel("Sorry, not possible...");
		return false;
  }

	return true;
}

bool Game::onPrepareMoveThing(Player *player, const Position& fromPos, const Item *item, slots_t toSlot)
{
	if( (abs(player->pos.x - fromPos.x) > 1) || (abs(player->pos.y - fromPos.y) > 1) ) {
		player->sendCancel("To far away...");
		return false;
	}
	else if(!item->isPickupable()) {
		player->sendCancel("You cannot move this object.");
		return false;
	}

	return true;
}

bool Game::onPrepareMoveThing(Player *player, slots_t fromSlot, const Item *fromItem, slots_t toSlot, const Item *toItem)
{
	if(toItem && (!toItem->isStackable() || toItem->getID() != fromItem->getID())) {
		player->sendCancel("Sorry, not enough room.");
		return false;
	}

	return true;
}

bool Game::onPrepareMoveThing(Player *player, const Item *item, slots_t toSlot)
{
	switch(toSlot)
	{
	case SLOT_HEAD:
		if(item->getSlotPosition() & SLOTP_HEAD)
			return true;
		break;
	case SLOT_NECKLACE:
		if(item->getSlotPosition() & SLOTP_NECKLACE)
			return true;
		break;
	case SLOT_BACKPACK:
		if(item->getSlotPosition() & SLOTP_BACKPACK)
			return true;
		break;
	case SLOT_ARMOR:
		if(item->getSlotPosition() & SLOTP_ARMOR)
			return true;
		break;
	case SLOT_RIGHT:
		if(item->getSlotPosition() & SLOTP_RIGHT){
			if(item->getSlotPosition() & SLOTP_TWO_HAND){
				if(player->items[SLOT_LEFT] != NULL){
					player->sendCancel("Two handed item .");
					return false;
				}
				return true	;				
			}
			else{
				if(player->items[SLOT_LEFT]){
					if(player->items[SLOT_LEFT]->getSlotPosition() & SLOTP_TWO_HAND){
						player->sendCancel("Two handed item .");
						return false;
					}
					return true;
				}
				return true;
			}
		}
		break;
	case SLOT_LEFT:
		if(item->getSlotPosition() & SLOTP_LEFT){
			if(item->getSlotPosition() & SLOTP_TWO_HAND){
				if(player->items[SLOT_RIGHT] != NULL){
					player->sendCancel("Two handed item .");
					return false;
				}
				return true	;				
			}
			else{
				if(player->items[SLOT_RIGHT]){
					if(player->items[SLOT_RIGHT]->getSlotPosition() & SLOTP_TWO_HAND){
						player->sendCancel("Two handed item .");
						return false;
					}
					return true;
				}
				return true;
			}
		}
		break;
	case SLOT_LEGS:
		if(item->getSlotPosition() & SLOTP_LEGS)
			return true;
		break;
	case SLOT_FEET:
		if(item->getSlotPosition() & SLOTP_FEET)
			return true;
		break;
	case SLOT_RING:
		if(item->getSlotPosition() & SLOTP_RING)
			return true;
		break;
	case SLOT_AMMO:
		if(item->getSlotPosition() & SLOTP_AMMO)
			return true;
		break;
	}
	player->sendCancel("You cannot put that object in that place.");
	return false;
}

//container/inventory to container/inventory
void Game::thingMoveInternal(Creature *player,
	unsigned char from_cid, unsigned char from_slotid, bool fromInventory,
	unsigned char to_cid, unsigned char to_slotid, bool toInventory,
	unsigned char count)
{
	Player *p = dynamic_cast<Player*>(player);
	if(p) {
		Container *fromContainer = NULL;
		Container *toContainer = NULL;
		Item *fromItem = NULL;
		Item *toItem = NULL;

		if(fromInventory) {
			fromItem = p->getItem(from_cid);
			fromContainer = dynamic_cast<Container *>(fromItem);
		}
		else {
			fromContainer = p->getContainer(from_cid);

			if(fromContainer) {
				if(from_slotid >= fromContainer->size())
					return;

				fromItem = fromContainer->getItem(from_slotid);
			}
		}

		if(toInventory) {
			toItem = p->getItem(to_cid);
			toContainer = dynamic_cast<Container *>(toItem);
		}
		else {
			toContainer = p->getContainer(to_cid);

			if(toContainer) {
				if(to_slotid >= toContainer->capacity())
					return;

				toItem = toContainer->getItem(to_slotid);
				Container *toSlotContainer = dynamic_cast<Container*>(toItem);
				if(toSlotContainer) {
					toContainer = toSlotContainer;
					toItem = NULL;
				}
			}
		}

		if(!fromItem || (toItem == fromItem) || (fromItem->isStackable() && count > fromItem->getItemCountOrSubtype()))
			return;

		//Container to container
		if(!fromInventory && fromContainer && toContainer) {
			if(onPrepareMoveThing(p, fromItem, fromContainer, toContainer, toItem)) {
				int oldFromCount = fromItem->getItemCountOrSubtype();
				int oldToCount = 0;

				//move around an item in a container
				if(fromItem->isStackable()) {
					if(toItem && toItem != fromItem && toItem->getID() == fromItem->getID())
					{
						oldToCount = toItem->getItemCountOrSubtype();

						int newToCount = std::min(100, oldToCount + count);
						toItem->setItemCountOrSubtype(newToCount);

						int subcount = std::max(oldToCount + count - 100, oldFromCount - count);
						fromItem->setItemCountOrSubtype(subcount);

						if(oldToCount + count  > 100) {
							int surplusCount = oldToCount + count  - 100;
							if(surplusCount == fromItem->getItemCountOrSubtype()) {
								if(onPrepareMoveThing(p, fromItem, fromContainer, toContainer, NULL)) {
									if(fromContainer->removeItem(fromItem)) {
										toContainer->addItem(fromItem);
									}
								}
								else
									count -= surplusCount; //re-define the actual amount we move.
							} else {
								Item *surplusItem = Item::CreateItem(fromItem->getID(), surplusCount);
								if(onPrepareMoveThing(p, surplusItem, fromContainer, toContainer, NULL)) {
									toContainer->addItem(surplusItem);
								}
								else {
									delete surplusItem;
									count -= surplusCount; //re-define the actual amount we move.
								}
							}
						}
					}
					else if(count < oldFromCount) {
						int subcount = oldFromCount - count;
						fromItem->setItemCountOrSubtype(subcount);
		
						toContainer->addItem(Item::CreateItem(fromItem->getID(), count));
					}
					else {
						if(fromContainer == toContainer) {
							fromContainer->moveItem(from_slotid, 0);
						}
						else if(fromContainer->removeItem(fromItem)) {
							toContainer->addItem(fromItem);
						}
					}

					if(fromItem->isStackable() && fromItem->getItemCountOrSubtype() == 0) {
						fromContainer->removeItem(fromItem);
					}
				}
				else {
					if(fromContainer == toContainer) {
						fromContainer->moveItem(from_slotid, 0);
					}
					else if(fromContainer->removeItem(fromItem)) {
						toContainer->addItem(fromItem);
					}
				}

				std::vector<Creature*> list;

				Position fromPos = (fromContainer->pos.x == 0xFFFF ? player->pos : fromContainer->pos);
				Position toPos = (toContainer->pos.x == 0xFFFF ? player->pos : toContainer->pos);

				if(fromPos == toPos) {
					getSpectators(Range(fromPos, false), list);
				}
				else {
					getSpectators(Range(fromPos, toPos), list);
				}

				if(!list.empty()) {
					for(int i = 0; i < list.size(); ++i) {
						list[i]->onThingMove(player, fromContainer, from_slotid, fromItem, oldFromCount, toContainer, to_slotid, toItem, oldToCount, count);
					}
				}
				else
					player->onThingMove(player, fromContainer, from_slotid, fromItem, oldFromCount, toContainer, to_slotid, toItem, oldToCount, count);

				if(fromItem->isStackable() && fromItem->getItemCountOrSubtype() == 0) {
					delete fromItem;
				}
			}
		}
		else {
			//inventory to inventory
			if(fromInventory && toInventory && !toContainer) {
				if(onPrepareMoveThing(p, fromItem, (slots_t)to_cid) && onPrepareMoveThing(p, (slots_t)from_cid, fromItem, (slots_t)to_cid, toItem)) {

					int oldFromCount = fromItem->getItemCountOrSubtype();
					int oldToCount = 0;

					if(fromItem->isStackable()) {
						if(toItem && toItem != fromItem && toItem->getID() == fromItem->getID())
						{
							oldToCount = toItem->getItemCountOrSubtype();

							int newToCount = std::min(100, oldToCount + count);
							toItem->setItemCountOrSubtype(newToCount);

							int subcount = std::max(oldToCount + count - 100, oldFromCount - count);
							fromItem->setItemCountOrSubtype(subcount);

							if(oldToCount + count > 100) {
								player->sendCancel("Sorry not enough room.");
							}

							if(fromItem->isStackable() && fromItem->getItemCountOrSubtype() == 0) {
								p->removeItemInventory(from_cid, true);
							}
						}
						else if(count < oldFromCount) {
							int subcount = oldFromCount - count;
							fromItem->setItemCountOrSubtype(subcount);
			
							p->removeItemInventory(to_cid, true);
							p->addItemInventory(Item::CreateItem(fromItem->getID(), count), to_cid, true);

							if(fromItem->isStackable() && fromItem->getItemCountOrSubtype() == 0) {
								p->removeItemInventory(from_cid, true);
							}
						}
						else {
							if(p->removeItemInventory(from_cid, true)) {
								p->removeItemInventory(to_cid, true);
								p->addItemInventory(fromItem, to_cid, true);
							}
						}
					}
					else if(p->removeItemInventory(from_cid, true)) {
						p->removeItemInventory(to_cid, true);
						p->addItemInventory(fromItem, to_cid, true);
					}

					player->onThingMove(player, (slots_t)from_cid, fromItem, oldFromCount, (slots_t)to_cid, toItem, oldToCount, count);

					if(fromItem->isStackable() && fromItem->getItemCountOrSubtype() == 0) {
						delete fromItem;
					}
				}
			}
			//container to inventory
			else if(!fromInventory && fromContainer && toInventory) {
				if(onPrepareMoveThing(p, fromItem, (slots_t)to_cid)) {
					int oldFromCount = fromItem->getItemCountOrSubtype();
					int oldToCount = 0;

					if(fromItem->isStackable()) {
						if(toItem && toItem != fromItem && toItem->getID() == fromItem->getID())
						{
							oldToCount = toItem->getItemCountOrSubtype();

							int newToCount = std::min(100, oldToCount + count);
							toItem->setItemCountOrSubtype(newToCount);

							int subcount = std::max(oldToCount + count - 100, oldFromCount - count);
							fromItem->setItemCountOrSubtype(subcount);

							if(oldToCount + count > 100) {
								player->sendCancel("Sorry not enough room.");
							}

							if(fromItem->isStackable() && fromItem->getItemCountOrSubtype() == 0) {
								fromContainer->removeItem(fromItem);
							}
						}
						else if(count < oldFromCount) {
							int subcount = oldFromCount - count;
							fromItem->setItemCountOrSubtype(subcount);
			
							p->removeItemInventory(to_cid, true);
							p->addItemInventory(Item::CreateItem(fromItem->getID(), count), to_cid, true);

							if(toItem) {
								fromContainer->addItem(toItem);
							}

							if(fromItem->isStackable() && fromItem->getItemCountOrSubtype() == 0) {
								fromContainer->removeItem(fromItem);
							}
						}
						else {
							if(fromContainer->removeItem(fromItem)) {
								p->removeItemInventory(to_cid, true);
								p->addItemInventory(fromItem, to_cid, true);

								if(toItem) {
									fromContainer->addItem(toItem);
								}
							}
						}
					}
					else if(fromContainer->removeItem(fromItem)) {
						p->removeItemInventory(to_cid, true);
						p->addItemInventory(fromItem, to_cid, true);

						if(toItem) {
							fromContainer->addItem(toItem);
						}
					}

					if(fromContainer->pos.x != 0xFFFF) {
						std::vector<Creature*> list;
						getSpectators(Range(fromContainer->pos, false), list);

						for(int i = 0; i < list.size(); ++i) {
							list[i]->onThingMove(player, fromContainer, from_slotid, fromItem, oldFromCount, (slots_t)to_cid, toItem, oldToCount, count);
						}
					}
					else
						player->onThingMove(player, fromContainer, from_slotid, fromItem, oldFromCount, (slots_t)to_cid, toItem, oldToCount, count);

					if(fromItem->isStackable() && fromItem->getItemCountOrSubtype() == 0) {
						delete fromItem;
					}
				}
			}
			//inventory to container
			else if(fromInventory && toContainer) {
				int oldFromCount = fromItem->getItemCountOrSubtype();
				int oldToCount = 0;

				if(onPrepareMoveThing(player, fromItem, NULL, toContainer, toItem)) {

					if(fromItem->isStackable()) {
						if(toItem && toItem != fromItem && toItem->getID() == fromItem->getID())
						{
							oldToCount = toItem->getItemCountOrSubtype();

							int newToCount = std::min(100, oldToCount + count);
							toItem->setItemCountOrSubtype(newToCount);

							int subcount = std::max(oldToCount + count - 100, oldFromCount - count);
							fromItem->setItemCountOrSubtype(subcount);

							if(oldToCount + count > 100) {
								int surplusCount = oldToCount + count  - 100;
								if(surplusCount == fromItem->getItemCountOrSubtype()) {
									if(onPrepareMoveThing(player, fromItem, NULL, toContainer, NULL)) {
										p->removeItemInventory(from_cid, true);
										toContainer->addItem(fromItem);
									}
									else
										count -= surplusCount; //re-define the actual amount we move.
								}
								else {
									Item *surplusItem = Item::CreateItem(fromItem->getID(), surplusCount);

									if(onPrepareMoveThing(player, surplusItem, NULL, toContainer, NULL)) {
										toContainer->addItem(surplusItem);
									}
									else {
										delete surplusItem;
										count -= surplusCount; //re-define the actual amount we move.
									}
								}
							}
						}
						else if(count < oldFromCount) {
							int subcount = oldFromCount - count;
							fromItem->setItemCountOrSubtype(subcount);
			
							toContainer->addItem(Item::CreateItem(fromItem->getID(), count));
						}
						else {
							if(p->removeItemInventory((slots_t)from_cid, true)) {
								toContainer->addItem(fromItem);
							}
						}

						if(fromItem->isStackable() && fromItem->getItemCountOrSubtype() == 0) {
							p->removeItemInventory(from_cid, true);
						}
					}
					else if(p->removeItemInventory(from_cid, true)) {
						toContainer->addItem(fromItem);
					}

					if(toContainer->pos.x != 0xFFFF) {
						std::vector<Creature*> list;
						getSpectators(Range(toContainer->pos, false), list);

						for(int i = 0; i < list.size(); ++i) {
							list[i]->onThingMove(player, (slots_t)from_cid, fromItem, oldFromCount, toContainer, to_slotid, toItem, oldToCount, count);
						}
					}
					else
						player->onThingMove(player, (slots_t)from_cid, fromItem, oldFromCount, toContainer, to_slotid, toItem, oldToCount, count);

					if(fromItem->isStackable() && fromItem->getItemCountOrSubtype() == 0) {
						delete fromItem;
					}
				}
			}
		}
	}
}

//container/inventory to ground (100%)
void Game::thingMoveInternal(Creature *player,
	unsigned char from_cid, unsigned char from_slotid, bool fromInventory,
	const Position& toPos, unsigned char count)
{
	Player* p = dynamic_cast<Player*>(player);
	if(p) {
		Container *fromContainer = NULL;
		Tile *toTile = getTile(toPos.x, toPos.y, toPos.z);
		if(!toTile)
			return;

		if(!fromInventory) {
			fromContainer = p->getContainer(from_cid);
			if(!fromContainer)
				return;
			
			Position fromPos = (fromContainer->pos.x == 0xFFFF ? player->pos : fromContainer->pos);			
			Item *fromItem = dynamic_cast<Item*>(fromContainer->getItem(from_slotid));

			if(!fromItem)
				return;

			if(onPrepareMoveThing(p, fromItem, fromPos, toPos) && onPrepareMoveThing(p, fromItem, NULL, toTile)) {
				Item *toItem = dynamic_cast<Item*>(toTile->getThingByStackPos(toTile->getThingCount() - 1));
				int oldFromCount = fromItem->getItemCountOrSubtype();
				int oldToCount = 0;

				//Do action...
				if(fromItem->isStackable()) {
					if(toItem && toItem->getID() == fromItem->getID())
					{
						oldToCount = toItem->getItemCountOrSubtype();

						int newToCount = std::min(100, oldToCount + count);
						toItem->setItemCountOrSubtype(newToCount);

						int subcount = std::max(oldToCount + count - 100, oldFromCount - count);
						fromItem->setItemCountOrSubtype(subcount);

						if(oldToCount + count > 100) {
							int surplusCount = oldToCount + count  - 100;
							if(surplusCount == fromItem->getItemCountOrSubtype()) {
								if(fromContainer->removeItem(fromItem)) {
									toTile->addThing(fromItem);
								}
							} else {
								Item *surplusItem = Item::CreateItem(fromItem->getID(), surplusCount);
								surplusItem->pos = toPos;
								
								toTile->addThing(surplusItem);
							}
						}

						if(fromItem->isStackable() && fromItem->getItemCountOrSubtype() == 0) {
							fromContainer->removeItem(fromItem);
						}
					}
					else if(count < oldFromCount) {
						int subcount = oldFromCount - count;
						fromItem->setItemCountOrSubtype(subcount);

						Item *moveItem = Item::CreateItem(fromItem->getID(), count);
						moveItem->pos = toPos;
						toTile->addThing(moveItem);

						if(fromItem->isStackable() && fromItem->getItemCountOrSubtype() == 0) {
							fromContainer->removeItem(fromItem);
						}
					}
					else if(fromContainer->removeItem(fromItem)) {
						fromItem->pos = toPos;
						toTile->addThing(fromItem);
					}
				}
				else if(fromContainer->removeItem(fromItem)) {
					fromItem->pos = toPos;
					toTile->addThing(fromItem);
				}

				std::vector<Creature*> list;
				getSpectators(Range(fromPos, false), list);

				std::vector<Creature*> tolist;
				getSpectators(Range(toPos, true), tolist);

				for(std::vector<Creature*>::const_iterator it = tolist.begin(); it != tolist.end(); ++it) {
					if(std::find(list.begin(), list.end(), *it) == list.end()) {
						list.push_back(*it);
					}
				}

				for(int i = 0; i < list.size(); ++i) {
					list[i]->onThingMove(player, fromContainer, from_slotid, fromItem, oldFromCount, toPos, toItem, oldToCount, count);
				}

				if(fromItem->isStackable() && fromItem->getItemCountOrSubtype() == 0) {
					delete fromItem;
				}
			}
		}
		else {
			Item *fromItem = p->getItem(from_cid);
			if(!fromItem)
				return;
			
			if(onPrepareMoveThing(p, fromItem, player->pos, toPos) && onPrepareMoveThing(p, fromItem, NULL, toTile)) {
				Item *toItem = dynamic_cast<Item*>(toTile->getThingByStackPos(toTile->getThingCount() - 1));
				int oldFromCount = fromItem->getItemCountOrSubtype();
				int oldToCount = 0;

				//Do action...
				if(fromItem->isStackable()) {
					if(toItem && toItem->getID() == fromItem->getID())
					{
						oldToCount = toItem->getItemCountOrSubtype();

						int newToCount = std::min(100, oldToCount + count);
						toItem->setItemCountOrSubtype(newToCount);

						int subcount = std::max(oldToCount + count - 100, oldFromCount - count);
						fromItem->setItemCountOrSubtype(subcount);

						if(oldToCount + count > 100) {
							int surplusCount = oldToCount + count  - 100;
							if(surplusCount == fromItem->getItemCountOrSubtype()) {
								if(fromContainer->removeItem(fromItem)) {
									toTile->addThing(fromItem);
								}
							} else {
								Item *surplusItem = Item::CreateItem(fromItem->getID(), surplusCount);
								surplusItem->pos = toPos;
								
								toTile->addThing(surplusItem);
							}
						}

						if(fromItem->isStackable() && fromItem->getItemCountOrSubtype() == 0) {
							p->removeItemInventory(from_cid, true);
						}
					}
					else if(count < oldFromCount) {
						int subcount = oldFromCount - count;
						fromItem->setItemCountOrSubtype(subcount);

						Item *moveItem = Item::CreateItem(fromItem->getID(), count);
						moveItem->pos = toPos;
						toTile->addThing(moveItem);

						if(fromItem->isStackable() && fromItem->getItemCountOrSubtype() == 0) {
							p->removeItemInventory(from_cid, true);
						}
					}
					else if(p->removeItemInventory(from_cid, true)) {
						fromItem->pos = toPos;
						toTile->addThing(fromItem);
					}
				}
				else if(p->removeItemInventory(from_cid, true)) {
					fromItem->pos = toPos;
					toTile->addThing(fromItem);
				}

				std::vector<Creature*> list;
				getSpectators(Range(player->pos, false), list);

				std::vector<Creature*> tolist;
				getSpectators(Range(toPos, true), tolist);

				for(std::vector<Creature*>::const_iterator it = tolist.begin(); it != tolist.end(); ++it) {
					if(std::find(list.begin(), list.end(), *it) == list.end()) {
						list.push_back(*it);
					}
				}

				for(int i = 0; i < list.size(); ++i) {
					list[i]->onThingMove(player, (slots_t)from_cid, fromItem, oldFromCount, toPos, toItem, oldToCount, count);
				}

				if(fromItem->isStackable() && fromItem->getItemCountOrSubtype() == 0) {
					delete fromItem;
				}
			}
		}
	}
}

//ground to container/inventory
//ground to inventory (100%)
//ground to container (100%)
void Game::thingMoveInternal(Creature *player, const Position& fromPos, unsigned char stackPos,
	unsigned char to_cid, unsigned char to_slotid, bool toInventory, unsigned char count)
{
	Player* p = dynamic_cast<Player*>(player);
	if(p) {
		Tile *fromTile = getTile(fromPos.x, fromPos.y, fromPos.z);
		if(!fromTile)
			return;

		Container *toContainer = NULL;

		Item *fromItem = dynamic_cast<Item*>(fromTile->getThingByStackPos(stackPos));

		if(!fromItem)
			return;

		if(toInventory) {
			Item *toSlot = p->getItem(to_cid);
			toContainer = dynamic_cast<Container*>(toSlot);
		}
		else {
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
			Item *toItem = toContainer->getItem(to_slotid);

			if(onPrepareMoveThing(player, fromItem, fromPos, p->pos) &&
				 onPrepareMoveThing(player, fromItem, NULL, toContainer, toItem))
			{
				int oldFromCount = fromItem->getItemCountOrSubtype();
				int oldToCount = 0;
				int stackpos = fromTile->getThingStackPos(fromItem);

				if(fromItem->isStackable()) {
					if(toItem && toItem->getID() == fromItem->getID()) {
						oldToCount = toItem->getItemCountOrSubtype();

						int newToCount = std::min(100, oldToCount + count);
						toItem->setItemCountOrSubtype(newToCount);

						int subcount = std::max(oldToCount + count - 100, oldFromCount - count);
						fromItem->setItemCountOrSubtype(subcount);

						if(oldToCount + count > 100) {
							int surplusCount = oldToCount + count  - 100;
							if(surplusCount == fromItem->getItemCountOrSubtype()) {
								if(onPrepareMoveThing(player, fromItem, NULL, toContainer, NULL)) {
									if(fromTile->removeThing(fromItem)) {
										toContainer->addItem(fromItem);
									}
								}
								else
									count -= surplusCount; //re-define the actual amount we move.
							}
							else {
								Item *surplusItem = Item::CreateItem(fromItem->getID(), surplusCount);

								if(onPrepareMoveThing(player, surplusItem, NULL, toContainer, NULL)) {
									toContainer->addItem(surplusItem);
								}
								else {
									delete surplusItem;
									count -= surplusCount; //re-define the actual amount we move.
								}
							}
						}

						if(fromItem->isStackable() && fromItem->getItemCountOrSubtype() == 0) {
							fromTile->removeThing(fromItem);
						}
					}
					else if(count < oldFromCount) {
						int subcount = oldFromCount - count;
						fromItem->setItemCountOrSubtype(subcount);

						toContainer->addItem(Item::CreateItem(fromItem->getID(), count));

						if(fromItem->isStackable() && fromItem->getItemCountOrSubtype() == 0) {
							fromTile->removeThing(fromItem);
						}
					}
					else if(fromTile->removeThing(fromItem)) {
						toContainer->addItem(fromItem);
					}
				}
				else {
					if(fromTile->removeThing(fromItem)) {
						toContainer->addItem(fromItem);
					}
				}
					
				std::vector<Creature*> list;
				getSpectators(Range(fromPos, true), list);
				for(int i = 0; i < list.size(); ++i) {
					list[i]->onThingMove(player, fromPos, stackpos, fromItem, oldFromCount, toContainer, to_slotid, toItem, oldToCount, count);
				}

				if(fromItem->isStackable() && fromItem->getItemCountOrSubtype() == 0) {
					delete fromItem;
				}
			}
		}
		//Put on equipment from ground
		else if(toInventory) {
			if(onPrepareMoveThing(p, fromPos, fromItem, (slots_t)to_cid) && onPrepareMoveThing(p, fromItem, (slots_t)to_cid)) {
				Item *toItem = p->getItem(to_cid);
				int oldFromCount = fromItem->getItemCountOrSubtype();
				int oldToCount = 0;
				int stackpos = fromTile->getThingStackPos(fromItem);

				if(fromItem->isStackable()) {
					if(toItem && toItem->getID() == fromItem->getID()) {
						oldToCount = toItem->getItemCountOrSubtype();

						int newToCount = std::min(100, oldToCount + count);
						toItem->setItemCountOrSubtype(newToCount);

						int subcount = std::max(oldToCount + count - 100, oldFromCount - count);
						fromItem->setItemCountOrSubtype(subcount);

						if(oldToCount + count > 100) {
							p->sendCancel("Sorry not enough room.");
						}

						if(fromItem && fromItem->getItemCountOrSubtype() == 0) {
							fromTile->removeThing(fromItem);
						}
					}
					else if(count < oldFromCount) {
						int subcount = oldFromCount - count;
						fromItem->setItemCountOrSubtype(subcount);

						p->removeItemInventory(to_cid, true);
						p->addItemInventory(Item::CreateItem(fromItem->getID(), count), to_cid, true);

						if(toItem) {
							fromTile->addThing(toItem);
							toItem->pos = fromPos;
						}

						if(fromItem->isStackable() && fromItem->getItemCountOrSubtype() == 0) {
							fromTile->removeThing(fromItem);
						}
					}
					else {
						if(fromTile->removeThing(fromItem)) {
							p->removeItemInventory(to_cid, true);
							p->addItemInventory(fromItem, to_cid, true);

							if(toItem) {
								fromTile->addThing(toItem);
								toItem->pos = fromPos;
							}
						}
					}
				}
				else {
					if(fromTile->removeThing(fromItem)) {
						p->removeItemInventory(to_cid, true);
						p->addItemInventory(fromItem, to_cid, true);

						if(toItem) {
							fromTile->addThing(toItem);
							toItem->pos = fromPos;
						}
					}
				}

				std::vector<Creature*> list;
				getSpectators(Range(fromPos, true), list);
				for(int i = 0; i < list.size(); ++i) {
					list[i]->onThingMove(player, fromPos, stackpos, fromItem, oldFromCount, (slots_t)to_cid, toItem, oldToCount, count);
				}

				if(fromItem->isStackable() && fromItem->getItemCountOrSubtype() == 0) {
					delete fromItem;
				}
			}
		}
	}
}

//ground to ground
void Game::thingMoveInternal(Creature *player, unsigned short from_x, unsigned short from_y, unsigned char from_z,
	unsigned char stackPos, unsigned short to_x, unsigned short to_y, unsigned char to_z, unsigned char count)
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
		if(creature){
			// we need to update the direction the player is facing to...
			// otherwise we are facing some problems in turning into the
			// direction we were facing before the movement
			// check y first cuz after a diagonal move we lock to east or west
			if (to_y < oldPos.y) ((Player*)thing)->direction = NORTH;
			if (to_y > oldPos.y) ((Player*)thing)->direction = SOUTH;
			if (to_x > oldPos.x) ((Player*)thing)->direction = EAST;
			if (to_x < oldPos.x) ((Player*)thing)->direction = WEST;
		}
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
					else 
						player->sendCancelWalk("Sorry, not possible...");  
				}                                            
				//change level end   
				else 
					player->sendCancelWalk("Sorry, not possible...");
				
				return;
			}

			if(!onPrepareMoveThing(player, thing, Position(from_x, from_y, from_z), Position(to_x, to_y, to_z)))
				return;
			
			if(creature && !onPrepareMoveCreature(player, creature, fromTile, toTile))
				return;
			
			if(!onPrepareMoveThing(player, thing, fromTile, toTile))
				return;
			
			Teleport *teleportitem = toTile->getTeleportItem();
			if(teleportitem) {
				teleport(thing, teleportitem->getDestPos());
				return;
			}
			
			int oldstackpos = fromTile->getThingStackPos(thing);
			if (fromTile && fromTile->removeThing(thing))
			{
				toTile->addThing(thing);
				
				thing->pos.x = to_x;
				thing->pos.y = to_y;
				thing->pos.z = to_z;
				
				if (creature) 
				{
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
					list[i]->onThingMove(player, thing, &oldPos, oldstackpos, 1, 1);
				}

				//change level begin
				if(playerMoving && toTile->ground && !(toTile->ground->noFloorChange())){          
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

void Game::creatureSay(Creature *creature, SpeakClasses type, const std::string &text)
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
							if(c) {
								teleport(c, creature->pos);
							}
						}
            break;
		  case 'i': // Create new items in the ground ;)
            {
			std::string cmd = text;
			cmd.erase(0,3);
			std::string::size_type pos = cmd.find(0x20, 0);
			if(pos == std::string::npos)
				break;
			
			int type = atoi(cmd.substr(0, pos).c_str());
			cmd.erase(0, pos+1);
			int count = std::min(atoi(cmd.c_str()), 100);
			
			Item *newItem = Item::CreateItem(type, count);
			if(!newItem)
				break;
			
			Tile *t = getTile(creature->pos.x, creature->pos.y, creature->pos.z);
			if(!t)
			{
				delete newItem;
				break;
			}
			newItem->pos = creature->pos;
			t->addThing(newItem);
			
			Game::creatureBroadcastTileUpdated(creature->pos);
			
            }
            break;
		  case 'q': // Testing command to see your money and to substract too.
            {
			std::string cmd = text;
			cmd.erase(0,3);
			
			Player *p = dynamic_cast<Player *>(creature);
			if(!p)
				break;
			
			int count = atoi(cmd.c_str());
			unsigned long money = p->getMoney();
			if(!count)
			{
				std::stringstream info;
				info << "You have " << money << " of money.";
				p->sendCancel(info.str().c_str());
				break;
			}
			else if(count > money)
			{
				std::stringstream info;
				info << "You have " << money << " of money and is not suficient.";
				p->sendCancel(info.str().c_str());
				break;
			}
			
			p->substractMoney(count);
            }
            break;
          case 'z': //protocol command
			std::string cmd = text;
			cmd.erase(0,3);
			int color = atoi(cmd.c_str());
			Player *p = dynamic_cast<Player *>(creature);			
			if(p)		
				p->sendMagicEffect(p->pos,color);					
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

void Game::teleport(Thing *thing, Position newPos) {

	if(newPos == thing->pos)  
		return; 
	OTSYS_THREAD_LOCK(gameLock)
	Creature *creature;
	Tile *from = getTile( thing->pos.x, thing->pos.y, thing->pos.z );
	Tile *to = getTile( newPos.x, newPos.y, newPos.z );
	if(!to) {
		OTSYS_THREAD_UNLOCK(gameLock)
		return;
	}

	int osp = from->getThingStackPos(thing);  
	if (from->removeThing(thing)) { 
		to->addThing(thing); 
		Position oldPos = thing->pos;
            
		std::vector<Creature*> list;
		getSpectators(Range(oldPos, true), list);
		for(size_t i = 0; i < list.size(); ++i) {
			creature = dynamic_cast<Creature*>(thing);
			if(creature)
				list[i]->onCreatureDisappear(creature, osp, true);
			else
				creatureBroadcastTileUpdated(oldPos);
			//list[i]->onTileUpdated(oldPos);
		}

		thing->pos = newPos;

		list.clear();
		getSpectators(Range(thing->pos, true), list);
		for(size_t i = 0; i < list.size(); ++i)
		{
			creature = dynamic_cast<Creature*>(thing);
			if(creature)
				list[i]->onTeleport(creature, &oldPos, osp);
			else
				creatureBroadcastTileUpdated(newPos);
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
			list[i]->onCreatureSay(creature, SPEAK_WHISPER, std::string("pspsps"));
		else
			list[i]->onCreatureSay(creature, SPEAK_WHISPER, text);
	}

  OTSYS_THREAD_UNLOCK(gameLock)
}

void Game::creatureYell(Creature *creature, std::string &text)
{
	OTSYS_THREAD_LOCK(gameLock)

	Player* player = dynamic_cast<Player*>(creature);
	if(player && player->access == 0 && player->exhaustedTicks >=1000) {
		player->exhaustedTicks += (long)g_config.getGlobalNumber("exhaustedadd", 0);		
		player->sendTextMessage(MSG_SMALLINFO, "You are exhausted.");		
	}
	else {
		creature->exhaustedTicks = (long)g_config.getGlobalNumber("exhausted", 0);
		std::transform(text.begin(), text.end(), text.begin(), upchar);

		std::vector<Creature*> list;
		map->getSpectators(Range(creature->pos, 18, 18, 14, 14), list);

		for(unsigned int i = 0; i < list.size(); ++i)
		{
			list[i]->onCreatureSay(creature, SPEAK_YELL, text);
		}
	}    
  
	OTSYS_THREAD_UNLOCK(gameLock)
}

void Game::creatureSpeakTo(Creature *creature, const std::string &receiver, const std::string &text)
{
	OTSYS_THREAD_LOCK(gameLock) 
	Creature* c = getCreatureByName(receiver.c_str());
	if(c)
		c->onCreatureSay(creature, SPEAK_PRIVATE, text);
	OTSYS_THREAD_UNLOCK(gameLock)
}

void Game::creatureMonsterYell(Monster* monster, const std::string& text) 
{
	OTSYS_THREAD_LOCK(gameLock)

	std::vector<Creature*> list;
	map->getSpectators(Range(monster->pos, 18, 18, 14, 14), list);

	for(unsigned int i = 0; i < list.size(); ++i) {
		list[i]->onCreatureSay(monster, SPEAK_MONSTER1, text);
	}

  OTSYS_THREAD_UNLOCK(gameLock)
}

void Game::creatureBroadcastMessage(Creature *creature, const std::string &text)
{
	if(creature->access == 0) 
		return;

	OTSYS_THREAD_LOCK(gameLock)

	//for (cit = playersOnline.begin(); cit != playersOnline.end(); cit++)
	for (AutoList<Player>::listiterator it = AutoList<Player>::list.begin(); it != AutoList<Player>::list.end(); ++it)
	{
		(*it).second->onCreatureSay(creature, SPEAK_BROADCAST, text);
	}

	OTSYS_THREAD_UNLOCK(gameLock)
}

void Game::creatureToChannel(Creature *creature, SpeakClasses type, const std::string &text, unsigned short channelId)
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
	//NetworkMessage msg;

	std::vector<Creature*> spectatorlist = gamestate.getSpectators();
	for(size_t i = 0; i < spectatorlist.size(); ++i) {
		Player* spectator = dynamic_cast<Player*>(spectatorlist[i]);
		
		if(!spectator)
			continue;

		//msg.Reset();

		if(bSuccess) {
			gamestate.getChanges(spectator);
			me->getDistanceShoot(spectator, creature, centerpos, hasTarget);

			std::vector<Position>::const_iterator tlIt;
			for(tlIt = poslist.begin(); tlIt != poslist.end(); ++tlIt) {
				Position pos = *tlIt;
				Tile *tile = getTile(pos.x, pos.y, pos.z);			
				const CreatureStateVec& creatureStateVec = gamestate.getCreatureStateList(tile);
					
				if(creatureStateVec.empty()) { //no targets
					me->getMagicEffect(spectator, creature, NULL, pos, 0, targettile->isPz(), isBlocking);
				}
				else {
					for(CreatureStateVec::const_iterator csIt = creatureStateVec.begin(); csIt != creatureStateVec.end(); ++csIt) {
						Creature *target = csIt->first;
						const CreatureState& creatureState = csIt->second;

						me->getMagicEffect(spectator, creature, target, target->pos, creatureState.damage, tile->isPz(), false);

						//could be death due to a magic damage with no owner (fire/poison/energy)
						if(creature && target->health <= 0) {

							for(std::vector<Creature*>::const_iterator cit = creatureState.attackerlist.begin(); cit != creatureState.attackerlist.end(); ++cit) {
								Creature* gainexpCreature = *cit;
								
								if(spectator->CanSee(gainexpCreature->pos.x, gainexpCreature->pos.y, gainexpCreature->pos.z)) {
									std::stringstream exp;
									exp << target->getGainedExperience(gainexpCreature);
									spectator->sendAnimatedText(gainexpCreature->pos, 983, exp.str());
									//msg.AddAnimatedText(gainexpCreature->pos, 983, exp.str());
									spectator->sendStats();
									//msg.AddPlayerStats(spectator);
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
								spectator->sendAnimatedText(target->pos, me->animationColor, dmg.str());
								//msg.AddAnimatedText(target->pos, me->animationColor, dmg.str());
							}

							if(creatureState.manaDamage > 0){
								spectator->sendMagicEffect(target->pos, NM_ME_LOOSE_ENERGY);
								//msg.AddMagicEffect(target->pos, NM_ME_LOOSE_ENERGY);
								std::stringstream manaDmg;
								manaDmg << std::abs(creatureState.manaDamage);
								spectator->sendAnimatedText(target->pos, 2, manaDmg.str());
								//msg.AddAnimatedText(target->pos, 2, manaDmg.str());
							}

							if (target->health > 0)
								spectator->sendCreatureHealth(target);
								//msg.AddCreatureHealth(target);

							if (spectator == target){
								CreateManaDamageUpdate(target, creature, creatureState.manaDamage);
								CreateDamageUpdate(target, creature, creatureState.damage);
							}
						}
					}
				}
			}
		}
		else {
			me->FailedToCast(spectator, creature, isBlocking, hasTarget);
		}

		//spectator->sendNetworkMessage(&msg);
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
					player->sendTextMessage(MSG_SMALLINFO, "You may not attack a person while your in a protection zone.");	
					player->sendCancelAttacking();
				}

				return false;
			}
			else if(targettile && targettile->isPz()) {
				if(player) {					
					player->sendTextMessage(MSG_SMALLINFO, "You may not attack a person in a protection zone.");					
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
					player->sendTextMessage(MSG_SMALLINFO, "You are exhausted.",player->pos, NM_ME_PUFF);
					player->exhaustedTicks += (long)g_config.getGlobalNumber("exhaustedadd", 0);
					return false;
				}
				else if(player->mana < me->manaCost) {															
					player->sendTextMessage(MSG_SMALLINFO, "You do not have enough mana.",player->pos, NM_ME_PUFF);					
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
		case FIGHT_MAGICDIST:
			if((std::abs(creature->pos.x-attackedCreature->pos.x) <= 8) &&
				(std::abs(creature->pos.y-attackedCreature->pos.y) <= 5) &&
				(creature->pos.z == attackedCreature->pos.z)) {

					if(map->canThrowItemTo(creature->pos, attackedCreature->pos, false, true))
						inReach = true;
				}	
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
	
	//NetworkMessage msg;

	std::vector<Creature*> spectatorlist = gamestate.getSpectators();
	for(unsigned int i = 0; i < spectatorlist.size(); ++i)
	{
		Player* spectator = dynamic_cast<Player*>(spectatorlist[i]);
		if(!spectator)
			continue;

		//msg.Reset();

		gamestate.getChanges(spectator);

		if(damagetype != FIGHT_MELEE){
			spectator->sendDistanceShoot(creature->pos, attackedCreature->pos, creature->getSubFightType());
			//msg.AddDistanceShoot(creature->pos, attackedCreature->pos, creature->getSubFightType());
		}
		
		if (attackedCreature->manaShieldTicks < 1000 && (creatureState.damage == 0) &&
			(spectator->CanSee(attackedCreature->pos.x, attackedCreature->pos.y, attackedCreature->pos.z))) {
				spectator->sendMagicEffect(attackedCreature->pos, NM_ME_PUFF);
			//msg.AddMagicEffect(attackedCreature->pos, NM_ME_PUFF);
		}
		else if (attackedCreature->manaShieldTicks < 1000 && (creatureState.damage < 0) &&
			(spectator->CanSee(attackedCreature->pos.x, attackedCreature->pos.y, attackedCreature->pos.z))) {
				spectator->sendMagicEffect(attackedCreature->pos, NM_ME_BLOCKHIT);
			//msg.AddMagicEffect(attackedCreature->pos, NM_ME_BLOCKHIT);
		}
		else {
			for(std::vector<Creature*>::const_iterator cit = creatureState.attackerlist.begin(); cit != creatureState.attackerlist.end(); ++cit) {
				Creature* gainexpCreature = *cit;

				if(spectator->CanSee(gainexpCreature->pos.x, gainexpCreature->pos.y, gainexpCreature->pos.z)) {
					std::stringstream exp;
					exp << attackedCreature->getGainedExperience(gainexpCreature);
					spectator->sendAnimatedText(gainexpCreature->pos, 983, exp.str());
					//msg.AddAnimatedText(gainexpCreature->pos, 983, exp.str());
					spectator->sendStats();
					//msg.AddPlayerStats(spectator);
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
					spectator->sendAnimatedText(attackedCreature->pos, 0xB4, dmg.str());
					//msg.AddAnimatedText(attackedCreature->pos, 0xB4, dmg.str());
					spectator->sendMagicEffect(attackedCreature->pos, NM_ME_DRAW_BLOOD);
					//msg.AddMagicEffect(attackedCreature->pos, NM_ME_DRAW_BLOOD);
				}

				if(creatureState.manaDamage >0) {
					std::stringstream manaDmg;
					manaDmg << std::abs(creatureState.manaDamage);
					//msg.AddMagicEffect(attackedCreature->pos, NM_ME_LOOSE_ENERGY);
					spectator->sendMagicEffect(attackedCreature->pos, NM_ME_LOOSE_ENERGY);
					//msg.AddAnimatedText(attackedCreature->pos, 2, manaDmg.str());
					spectator->sendAnimatedText(attackedCreature->pos, 2, manaDmg.str());
				}

				if (attackedCreature->health > 0)
					spectator->sendCreatureHealth(attackedCreature);
					//msg.AddCreatureHealth(attackedCreature);

				if (spectator == attackedCreature) {
					CreateManaDamageUpdate(attackedCreature, creature, creatureState.manaDamage);
					CreateDamageUpdate(attackedCreature, creature, creatureState.damage);
				}
			}
		}

		//spectator->sendNetworkMessage(&msg);
	}

	if(damagetype != FIGHT_MELEE)
		creature->RemoveDistItem();
		
	OTSYS_THREAD_UNLOCK(gameLock)
}

std::list<Position> Game::getPathTo(Creature *creature, Position start, Position to, bool creaturesBlock){
	return map->getPathTo(creature, start, to, creaturesBlock);
}

void Game::checkPlayer(unsigned long id)
{
	OTSYS_THREAD_LOCK(gameLock)
	Creature *creature = getCreatureByID(id);

	if (creature && creature->health > 0)
	{
		int thinkTicks = 0;
		int oldThinkTicks = creature->onThink(thinkTicks);
		
		if(thinkTicks > 0) {
			addEvent(makeTask(thinkTicks, std::bind2nd(std::mem_fun(&Game::checkPlayer), id)));
		}

		Player* player = dynamic_cast<Player*>(creature);
		if(player){
			//addEvent(makeTask(1000, std::bind2nd(std::mem_fun(&Game::checkPlayer), id)));
			
			Tile *tile = getTile(player->pos.x, player->pos.y, player->pos.z);
			if(tile == NULL){
				std::cout << "CheckPlayer NULL tile: " << player->getName() << std::endl;
				return;
			}
				
			if(!tile->isPz())
				player->mana += min(5, player->manamax - player->mana);						
			
			int lastLv = player->level;
			
			while (player->experience >= player->getExpForLv(player->level+1)) {
				player->level++;
				player->healthmax += player->HPGain[player->voc];
				player->health += player->HPGain[player->voc];
				player->manamax += player->ManaGain[player->voc];
				player->mana += player->ManaGain[player->voc];
				player->cap += player->CapGain[player->voc];
			}
			if(lastLv != player->level)
			{
				player->setNormalSpeed();
				changeSpeed(player->getID(), player->getSpeed());
				std::stringstream lvMsg;
				lvMsg << "You advanced from level " << lastLv << " to level " << player->level << ".";				
				player->sendTextMessage(MSG_ADVANCE,lvMsg.str().c_str());
			}
			//send stast only if have changed
			if(player->NeedUpdateStats())
				player->sendStats();
			
			player->sendPing();
			
			//Magic Level Advance
			int reqMana = player->getReqMana(player->maglevel+1, player->voc);
			//ATTANTION: MAKE SURE THAT CHARACTERS HAVE REASONABLE MAGIC LEVELS. ESPECIALY KNIGHTS!!!!!!!!!!!
			/*
			if (reqMana % 20 < 10) //CIP must have been bored when they invented this odd rounding
				reqMana = reqMana - (reqMana % 20);
			else
				reqMana = reqMana - (reqMana % 20) + 20;
			*/
			if (player->access == 0 && player->manaspent >= reqMana) {
				player->manaspent -= reqMana;
				player->maglevel++;
				std::stringstream MaglvMsg;
				MaglvMsg << "You advanced from magic level " << (player->maglevel - 1) << " to magic level " << player->maglevel << ".";				
				player->sendTextMessage(MSG_ADVANCE, MaglvMsg.str().c_str());
				player->sendStats();
				//msg.AddPlayerStats(player);
				//player->sendNetworkMessage(&msg);
			}
			//End Magic Level Advance

			if(player->inFightTicks >= 1000) {
				player->inFightTicks -= thinkTicks; /*1000;*/
				
				if(player->inFightTicks < 1000)
					player->pzLocked = false;
					player->sendIcons(); 
			}
			
			if(player->exhaustedTicks >=1000){
				player->exhaustedTicks -= thinkTicks; /*1000;*/
			}
			
			if(player->manaShieldTicks >=1000){
				player->manaShieldTicks -= thinkTicks; /*1000;*/
				
				if(player->manaShieldTicks  < 1000)
					player->sendIcons();
			}
			
			if(player->hasteTicks >=1000){
				player->hasteTicks -= thinkTicks; /*1000*/;
			}	
		}
		else {
 			/*
			addEvent(makeTask(300, std::bind2nd(std::mem_fun(&Game::checkPlayer), id)));
			*/

			if(creature->manaShieldTicks >=1000){
					creature->manaShieldTicks -= thinkTicks; /*300*/;
			}
				
			if(creature->hasteTicks >=1000){
				creature->hasteTicks -= thinkTicks; /*300*/;
			}
		}

		Conditions& conditions = creature->getConditions();
		for(Conditions::iterator condIt = conditions.begin(); condIt != conditions.end(); ++condIt) {
			if(condIt->first == ATTACK_FIRE || condIt->first == ATTACK_ENERGY || condIt->first == ATTACK_POISON) {
				ConditionVec &condVec = condIt->second;

				if(condVec.empty())
					continue;

				CreatureCondition& condition = condVec[0];

				if(condition.onTick(oldThinkTicks /*decTick*/)) {
					const MagicEffectTargetCreatureCondition* magicTargetCondition =  condition.getCondition();
					Creature* c = getCreatureByID(magicTargetCondition->getOwnerID());
					creatureMakeMagic(c, creature->pos, magicTargetCondition);

					if(condition.getCount() <= 0) {
						condVec.erase(condVec.begin());
					}
				}
			}
		}
		flushSendBuffers();
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
					if(fromtile == NULL){
						std::cout << "CheckAttackingPlayer NULL tile: " << creature->getName() << std::endl;
						return;
					}						
					if (!attackedCreature->isAttackable() == 0 && fromtile->isPz() && creature->access == 0)
					{
						Player* player = dynamic_cast<Player*>(creature);
						if (player) {							
							player->sendTextMessage(MSG_SMALLINFO, "You may not attack a person in a protection zone.");
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
		flushSendBuffers();
	}

	OTSYS_THREAD_UNLOCK(gameLock)
}

//void Game::decayItem(Position& pos, unsigned short id, unsigned char stackpos)
void Game::decayItem(Item *item)
{
	OTSYS_THREAD_LOCK(gameLock)

	/*todo: Decaying item could be in a  container carried by a player,
		should all items have a pointer to their parent (like containers)?*/
	if(item && item->pos.x != 0xFFFF) {
		Tile *tile = getTile(item->pos.x, item->pos.y, item->pos.z);
		MapState mapstate(this->map);
		Position pos;
		
		MagicEffectItem* magicItem = dynamic_cast<MagicEffectItem*>(item);

		if(magicItem) {
			pos = magicItem->pos;
			if(magicItem->transform()) {
				mapstate.replaceThing(tile, magicItem, magicItem);
				//addEvent(makeTask(magicItem->getDecayTime(), boost::bind(&Game::decayItem, this, magicItem->pos, magicItem->getID(), tile->getThingStackPos(magicItem)) ) );
				addEvent(makeTask(magicItem->getDecayTime(), std::bind2nd(std::mem_fun(&Game::decayItem), magicItem)));
			}
			else {
				mapstate.removeThing(tile, item);
				//tile->removeThing(magicItem);				
				//delete magicItem;
				FreeThing(magicItem);
			}			
			//creatureBroadcastTileUpdated(pos);
		}
		else {
			Item* newitem = item->tranform();

			pos = item->pos;			

			if (newitem == NULL /*decayTo == 0*/) {
				mapstate.removeThing(tile, item);
				//t->removeThing(item);
			}
			else {
				mapstate.replaceThing(tile, item, newitem);

				unsigned short decayTime = Item::items[newitem->getID()].decayTime;
				addEvent(makeTask(decayTime*1000, std::bind2nd(std::mem_fun(&Game::decayItem), newitem)));
				//mapstate.refreshThing(tile, item);
			}
			FreeThing(item);
			//delete item;
		}
		std::vector<Creature*> list;
		getSpectators(Range(pos, true), list);
			
		for(unsigned int i = 0; i < list.size(); ++i) {
			Player *spectator = dynamic_cast<Player*>(list[i]);
			if(!spectator)
				continue;				
			mapstate.getMapChanges(spectator);
		}
		
		flushSendBuffers();
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
				FreeThing(item);
				//delete item;
		}
	}
    flushSendBuffers();
	OTSYS_THREAD_UNLOCK(gameLock)
}


/*
void Game::checkSpawns(int n)
{
	SpawnManager::instance()->checkSpawns(n);
	this->addEvent(makeTask(5000, std::bind2nd(std::mem_fun(&Game::checkSpawns), 0)));
}
*/

void Game::CreateDamageUpdate(Creature* creature, Creature* attackCreature, int damage)
{
	Player* player = dynamic_cast<Player*>(creature);
	if(!player)
		return;
	player->sendStats();
	//msg.AddPlayerStats(player);
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
		player->sendTextMessage(MSG_EVENT, dmgmesg.str().c_str());
		//msg.AddTextMessage(MSG_EVENT, dmgmesg.str().c_str());
	}
	if (player->health <= 0){
       player->die();        //handles exp/skills/maglevel loss
	}
}

void Game::CreateManaDamageUpdate(Creature* creature, Creature* attackCreature, int damage)
{
	Player* player = dynamic_cast<Player*>(creature);
	if(!player)
		return;
	player->sendStats();
	//msg.AddPlayerStats(player);
	if (damage > 0) {
		std::stringstream dmgmesg;
		dmgmesg << "You lose " << damage << " mana";
		if(attackCreature) {
			dmgmesg << " blocking an attack by " << attackCreature->getName();
		}
		else
			dmgmesg <<".";
		player->sendTextMessage(MSG_EVENT, dmgmesg.str().c_str());
		//msg.AddTextMessage(MSG_EVENT, dmgmesg.str().c_str());
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
	if(sit != spells.getAllRuneSpells()->end()) {
		if(sit->second->getMagLv() <= creature->maglevel || creature->access != 0)
		{
			bool success = sit->second->getSpellScript()->castSpell(creature, pos, var);
			ret = success;
		}
		else
		{
			Player *p = dynamic_cast<Player *>(creature);
			if(p)
				p->sendCancel("You don't have the required magic level to use that rune.");
		}
	}

	OTSYS_THREAD_UNLOCK(gameLock)
	return ret;
}


void Game::flushSendBuffers(){
	OTSYS_THREAD_LOCK(gameLock)

	for(std::vector<Player*>::iterator it = BufferedPlayers.begin(); it != BufferedPlayers.end(); ++it) {
		(*it)->flushMsg();
		(*it)->releaseThing();
/*
#ifdef __DEBUG__
		std::cout << "flushSendBuffers() - releaseThing()" << std::endl;
#endif
*/
		}	
	BufferedPlayers.clear();
	
	//free memory
	for(std::vector<Thing*>::iterator it = ToReleaseThings.begin(); it != ToReleaseThings.end(); ++it){		
		(*it)->releaseThing();
	}
	ToReleaseThings.clear();
	
	OTSYS_THREAD_UNLOCK(gameLock)
	return;
}

void Game::addPlayerBuffer(Player* p){
	OTSYS_THREAD_LOCK(gameLock)	
/*
#ifdef __DEBUG__
	std::cout << "addPlayerBuffer() - useThing()" << std::endl;
#endif
*/

	p->useThing();

	BufferedPlayers.push_back(p);
	OTSYS_THREAD_UNLOCK(gameLock)
	return;
}

void Game::FreeThing(Thing* thing){
	OTSYS_THREAD_LOCK(gameLock)
	//std::cout << "freeThings() " << (long)thing <<std::endl;
	ToReleaseThings.push_back(thing);
	OTSYS_THREAD_UNLOCK(gameLock)
	return;
}

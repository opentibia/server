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
#include "commands.h"
#include "creature.h"
#include "player.h"
#include "monster.h"
#include "npc.h"
#include "game.h"
#include "tile.h"

#include "spells.h"
#include "actions.h"
#include "ioplayer.h"

#include "luascript.h"
#include <ctype.h>

#if defined __EXCEPTION_TRACER__
#include "exception.h"
extern OTSYS_THREAD_LOCKVAR maploadlock;
#endif

#define EVENT_CHECKCREATURE          123
#define EVENT_CHECKCREATUREATTACKING 124

extern LuaScript g_config;
extern Spells spells;
extern Actions actions;
extern Commands commands;
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
				if(targetPlayer && targetPlayer != attackPlayer && game->getWorldType() != WORLD_TYPE_NO_PVP)
					attackPlayer->pzLocked = true;
			}

			if(targetCreature->access == 0 && targetPlayer && game->getWorldType() != WORLD_TYPE_NO_PVP) {
				targetPlayer->inFightTicks = (long)g_config.getGlobalNumber("pzlocked", 0);
				targetPlayer->sendIcons();
			}

			if(game->getWorldType() == WORLD_TYPE_NO_PVP && attackPlayer && targetPlayer && attackPlayer->access == 0){
				damage = 0;
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

			magicItem->isRemoved = false;
			game->startDecay(magicItem);
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
		damage -= (int)((damage*armor/100)*(rand()/(RAND_MAX+1.0)));
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
	Player *attackedplayer = dynamic_cast<Player*>(attackedCreature);
	Position CreaturePos = attackedCreature->pos;
	
	attackedCreature->addInflictedDamage(attacker, damage);
	
	if(attackedplayer){
		attackedplayer->sendStats();
	}
	//Remove player?
	if(attackedCreature->health <= 0 && attackedCreature->isRemoved == false) {
		unsigned char stackpos = tile->getThingStackPos(attackedCreature);
		/*//Remove character
		unsigned char stackpos = tile->getCreatureStackPos(attackedCreature);
		
		game->sendRemoveThing(NULL,attackedCreature->pos,attackedCreature,stackpos);
		tile->removeThing(attackedCreature);*/
		
		//Prepare body
		Item *corpseitem = Item::CreateItem(attackedCreature->getLookCorpse());
		corpseitem->pos = CreaturePos;
		tile->addThing(corpseitem);
		
		//Add eventual loot
		Container *lootcontainer = dynamic_cast<Container*>(corpseitem);
		if(lootcontainer) {
			attackedCreature->dropLoot(lootcontainer);
		}
		
		if(attackedplayer){
			attackedplayer->onThingDisappear(attackedplayer,stackpos);
			attackedplayer->die();        //handles exp/skills/maglevel loss
		}
		//remove creature
		game->removeCreature(attackedCreature);
		// Update attackedCreature pos because contains
		//  temple position for players
		attackedCreature->pos = CreaturePos;
		//add body
		game->sendAddThing(NULL,corpseitem->pos,corpseitem);
		
		if(attackedplayer){
			//set body special description
			std::stringstream s;
			s << "a dead human. You recognize " 
				<< attackedplayer->getName() << ". ";
			if(attacker){
				if(attackedplayer->getSex() == PLAYERSEX_FEMALE)
					s << "She";
				else
					s << "He";
				s << " was killed by ";
				Player *attackerplayer = dynamic_cast<Player*>(attacker);
				if(attackerplayer)
					s << attacker->getName();
				else
					s << "a " << attacker->getName();
			}				
			corpseitem->setSpecialDescription(s.str());
			//send corpse to the dead player. It is not in spectator list
			// because was removed
			attackedplayer->onThingAppear(corpseitem);
		}
		game->startDecay(corpseitem);
		
		/*
		for(int i = 0; i < spectatorlist.size(); ++i) {				
			spectatorlist[i]->onCreatureDisappear(attackedCreature, stackpos);
		}*/
		//mapstate.removeThing(tile, attackedCreature);		
		
		//Get all creatures that will gain xp from this kill..
		CreatureState* attackedCreatureState = NULL;
		std::vector<long> creaturelist;
		if(!(dynamic_cast<Player*>(attackedCreature) && game->getWorldType() != WORLD_TYPE_PVP_ENFORCED)){
			creaturelist = attackedCreature->getInflicatedDamageCreatureList();
			CreatureStateVec& creatureStateVec = creaturestates[tile];
			for(CreatureStateVec::iterator csIt = creatureStateVec.begin(); csIt != creatureStateVec.end(); ++csIt) {
				if(csIt->first == attackedCreature) {
					attackedCreatureState = &csIt->second;
					break;
				}
			}
		}

		if(attackedCreatureState) { //should never be NULL..
			//Add experience
			for(std::vector<long>::const_iterator iit = creaturelist.begin(); iit != creaturelist.end(); ++iit) {
				Creature* gainExpCreature = game->getCreatureByID(*iit);
				if(gainExpCreature) {
					int gainedExperience = attackedCreature->getGainedExperience(gainExpCreature);
					if(gainedExperience <= 0)
						continue;

					Player *gainExpPlayer = dynamic_cast<Player*>(gainExpCreature);

					if(gainExpPlayer) {
						gainExpPlayer->addExp(gainedExperience);
					}

					//Need to add this creature and all that can see it to spectators, unless they already added
					std::vector<Creature*> creaturelist;
					game->getSpectators(Range(gainExpCreature->pos, true), creaturelist);

					for(std::vector<Creature*>::const_iterator cit = creaturelist.begin(); cit != creaturelist.end(); ++cit) {
						if(std::find(spectatorlist.begin(), spectatorlist.end(), *cit) == spectatorlist.end()) {
							spectatorlist.push_back(*cit);
						}
					}

					//Add creature to attackerlist
					attackedCreatureState->attackerlist.push_back(gainExpCreature);
				}
			}
		}

		Player *player = dynamic_cast<Player*>(attacker);
		if(player){
			player->sendStats();
		}
		
		/*
		Player *spectator = NULL;
		for(int i = 0; i < spectatorlist.size(); ++i) {
			spectator = dynamic_cast<Player*>(spectatorlist[i]);
			if(spectator) {
				spectator->onThingAppear(corpseitem);
			}
		}
		*/
		//mapstate.addThing(tile, corpseitem);

		//Start decaying
		//unsigned short decayTime = Item::items[corpseitem->getID()].decayTime;
		//game->addEvent(makeTask(decayTime*1000, boost::bind(&Game::decayItem, _1, corpseitem)));
		
		if(attackedCreature && attackedCreature->getMaster() != NULL) {
			attackedCreature->getMaster()->removeSummon(attackedCreature);
			//game->FreeThing(attackedCreature);
		}

		//free if attackedCreature is not a player		
		/*if(attackedplayer == NULL)
			game->FreeThing(attackedCreature);*/
	}

	//Add blood?
	if((drawBlood || attackedCreature->health <= 0) && damage > 0) {

		bool hadSplash = (tile->splash != NULL);

		if (!tile->splash) {
			Item *item = Item::CreateItem(2019, FLUID_BLOOD);
			item->pos = CreaturePos;
			tile->splash = item;
			game->startDecay(tile->splash);
		}

		if(hadSplash) {
			unsigned char stackpos = tile->getThingStackPos(tile->splash);
			Item *splash;
			splash = tile->splash;
			splash->setID(2019);
			splash->setItemCountOrSubtype(FLUID_BLOOD);
			tile->splash = NULL;
			game->sendRemoveThing(NULL,CreaturePos,splash,stackpos);
			tile->splash = splash;
			game->sendAddThing(NULL,CreaturePos,tile->splash);
			/*
			Player *spectator = NULL;
			for(int i = 0; i < spectatorlist.size(); ++i) {
				spectator = dynamic_cast<Player*>(spectatorlist[i]);
				if(spectator) {
					spectator->onThingDisappear(tile->splash, stackpos);
					spectator->onThingAppear(tile->splash);
				}
			}
			*/
			//mapstate.refreshThing(tile, tile->splash);
		}
		else {
			/*Player *spectator = NULL;
			for(int i = 0; i < spectatorlist.size(); ++i) {
				spectator = dynamic_cast<Player*>(spectatorlist[i]);
				if(spectator) {
					spectator->onThingAppear(tile->splash);
				}
			}*/
			game->sendAddThing(NULL,CreaturePos,tile->splash);
			//mapstate.addThing(tile, tile->splash);
		}

		//Start decaying
		//unsigned short decayTime = Item::items[tile->splash->getID()].decayTime;
		//tile->decaySplashAfter = OTSYS_TIME() + decayTime*1000;
		//game->addEvent(makeTask(decayTime*1000, std::bind2nd(std::mem_fun(&Game::decaySplash), tile->splash)));
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
	eventIdCount = 1000;
	this->game_state = GAME_STATE_NORMAL;
	this->map = NULL;
	this->worldType = WORLD_TYPE_PVP;
	OTSYS_THREAD_LOCKVARINIT(gameLock);
	OTSYS_THREAD_LOCKVARINIT(eventLock);
	OTSYS_THREAD_LOCKVARINIT(AutoID::autoIDLock);
#if defined __EXCEPTION_TRACER__
	OTSYS_THREAD_LOCKVARINIT(maploadlock);
#endif
	OTSYS_THREAD_SIGNALVARINIT(eventSignal);
	BufferedPlayers.clear();
	OTSYS_CREATE_THREAD(eventThread, this);

	addEvent(makeTask(DECAY_INTERVAL, boost::bind(&Game::checkDecay,this,DECAY_INTERVAL)));	
}


Game::~Game()
{
}

void Game::setWorldType(enum_world_type type)
{
	this->worldType = type;
}

enum_game_state Game::getGameState()
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);
	return game_state;
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
		bool runtask = false;

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

		if(task) {
			std::map<unsigned long, SchedulerTask*>::iterator it = _this->eventIdMap.find(task->getEventId());
			if(it != _this->eventIdMap.end()) {
				_this->eventIdMap.erase(it);
				runtask = true;
			}
		}

		OTSYS_THREAD_UNLOCK(_this->eventLock);
    if (task) {
			if(runtask) {
				(*task)(_this);
			}
			delete task;
    }
  }
#if defined __EXCEPTION_TRACER__
	eventExceptionHandler.RemoveHandler();
#endif

}

unsigned long Game::addEvent(SchedulerTask* event) {
  bool do_signal = false;
  OTSYS_THREAD_LOCK(eventLock)

	if(event->getEventId() == 0) {
		++eventIdCount;
		event->setEventId(eventIdCount);
	}
	else {
#ifdef __DEBUG__EVENTSCHEDULER__
		std::cout << "addEvent - " << event->getEventId() << std::endl;
#endif
	}

#ifdef __DEBUG__EVENTSCHEDULER__
		std::cout << "addEvent - " << event->getEventId() << std::endl;
#endif

	eventIdMap[event->getEventId()] = event;
	
	if (eventList.empty() ||  *event < *eventList.top())
    do_signal = true;

	eventList.push(event);

	/*
	if (eventList.empty() ||  *event < *eventList.top())
    do_signal = true;
	*/

  OTSYS_THREAD_UNLOCK(eventLock)

	if (do_signal)
		OTSYS_THREAD_SIGNAL_SEND(eventSignal);

	return event->getEventId();
}

bool Game::stopEvent(unsigned long eventid) {
	//return false;

	if(eventid == 0)
		return false;

  OTSYS_THREAD_LOCK(eventLock)

	std::map<unsigned long, SchedulerTask*>::iterator it = eventIdMap.find(eventid);
	if(it != eventIdMap.end()) {

#ifdef __DEBUG__EVENTSCHEDULER__
		std::cout << "stopEvent - eventid: " << eventid << "/" << it->second->getEventId() << std::endl;
#endif

		//it->second->setEventId(0); //invalidate the event
		eventIdMap.erase(it);

	  OTSYS_THREAD_UNLOCK(eventLock)
		return true;
	}

  OTSYS_THREAD_UNLOCK(eventLock)
	return false;
}

/*****************************************************************************/

uint32_t Game::getPlayersOnline() {return (uint32_t)Player::listPlayer.list.size();};
uint32_t Game::getMonstersOnline() {return (uint32_t)Monster::listMonster.list.size();};
uint32_t Game::getNpcsOnline() {return (uint32_t)Npc::listNpc.list.size();};
uint32_t Game::getCreaturesOnline() {return (uint32_t)listCreature.list.size();};

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
	if(id == 0)
		return NULL;
	
	AutoList<Creature>::listiterator it = listCreature.list.find(id);
	if(it != listCreature.list.end()) {
		return (*it).second;
	}

	return NULL; //just in case the player doesnt exist
}

Creature* Game::getCreatureByName(const std::string &s)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);
	std::string txt1 = s;
	std::transform(txt1.begin(), txt1.end(), txt1.begin(), upchar);
	for(AutoList<Creature>::listiterator it = listCreature.list.begin(); it != listCreature.list.end(); ++it){
		std::string txt2 = (*it).second->getName();
		std::transform(txt2.begin(), txt2.end(), txt2.begin(), upchar);
		if(txt1 == txt2)
			return it->second;
	}
	return NULL; //just in case the player doesnt exist
}

bool Game::placeCreature(Position &pos, Creature* c)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);
	
	bool success = false;
	Player *p = dynamic_cast<Player*>(c);

	if (!p || c->access != 0 || getPlayersOnline() < max_players) {
		success = map->placeCreature(pos, c);		
		if(success) {
			c->useThing();
			
			c->setID();
			//std::cout << "place: " << c << " " << c->getID() << std::endl;
			listCreature.addList(c);
			c->addList();
			c->isRemoved = false;
			
			sendAddThing(NULL,c->pos,c);
			if(p) {
				std::cout << (uint32_t)getPlayersOnline() << " players online." << std::endl;
			}
			
			if(p){
				c->eventCheck = addEvent(makeTask(1000, std::bind2nd(std::mem_fun(&Game::checkCreature), c->getID())));
			}
			else{
				c->eventCheck = addEvent(makeTask(500, std::bind2nd(std::mem_fun(&Game::checkCreature), c->getID())));
			}
			//c->eventCheckAttacking = addEvent(makeTask(2000, std::bind2nd(std::mem_fun(&Game::checkCreatureAttacking), c->getID())));
		}
	}
	else {
		//we cant add the player, server is full	
		success = false;
	}
	
  return success;
}

bool Game::removeCreature(Creature* c)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);
	if(c->isRemoved == true)
		return false;
#ifdef __DEBUG__
	std::cout << "removing creature "<< std::endl;
#endif

	//std::cout << "remove: " << c << " " << c->getID() << std::endl;
	listCreature.removeList(c->getID());
	c->removeList();
	
	removeThing(NULL,c->pos,c);
	c->isRemoved = true;
	
	for(std::vector<Creature*>::iterator cit = c->summons.begin(); cit != c->summons.end(); ++cit) {
		removeCreature(*cit);
	}
		
	stopEvent(c->eventCheck);
	stopEvent(c->eventCheckAttacking);

	Player* player = dynamic_cast<Player*>(c);
	if(player){
		if(player->tradePartner != 0) {
			playerCloseTrade(player);
		}
		if(player->eventAutoWalk)
			stopEvent(player->eventAutoWalk);
		// Removing the player from the map of channel users
		std::map<long, Creature*>::iterator sit = channel.find(player->getID());
		if( sit != channel.end() )
			channel.erase(sit);
		
		IOPlayer::instance()->savePlayer(player);
		std::cout << (uint32_t)getPlayersOnline() << " players online." << std::endl;
	}
	
	this->FreeThing(c);
	return true;
}

void Game::thingMove(Creature *creature, Thing *thing,
	unsigned short to_x, unsigned short to_y, unsigned char to_z, unsigned char count)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);

	Tile *fromTile = map->getTile(thing->pos.x, thing->pos.y, thing->pos.z);

	if (fromTile)
	{
		int oldstackpos = fromTile->getThingStackPos(thing);
		thingMoveInternal(creature, thing->pos.x, thing->pos.y, thing->pos.z, oldstackpos, to_x, to_y, to_z, count);
	}
}


void Game::thingMove(Creature *creature, unsigned short from_x, unsigned short from_y, unsigned char from_z,
	unsigned char stackPos, unsigned short to_x, unsigned short to_y, unsigned char to_z, unsigned char count)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);

	thingMoveInternal(creature, from_x, from_y, from_z, stackPos, to_x, to_y, to_z, count);
}

//container/inventory to container/inventory
void Game::thingMove(Player *player,
	unsigned char from_cid, unsigned char from_slotid, bool fromInventory,
	unsigned char to_cid, unsigned char to_slotid, bool toInventory,
	unsigned char count)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);
		
	thingMoveInternal(player, from_cid, from_slotid, fromInventory,
		to_cid, to_slotid, toInventory, count);
}

//container/inventory to ground
void Game::thingMove(Player *player,
	unsigned char from_cid, unsigned char from_slotid, bool fromInventory,
	const Position& toPos, unsigned char count)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);

	thingMoveInternal(player, from_cid, from_slotid, fromInventory, toPos, count);
}

//ground to container/inventory
void Game::thingMove(Player *player,
	const Position& fromPos, unsigned char stackPos,
	unsigned char to_cid, unsigned char to_slotid, bool toInventory,
	unsigned char count)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);		
	thingMoveInternal(player, fromPos, stackPos, to_cid, to_slotid, toInventory, count);
}

/*ground -> ground*/
bool Game::onPrepareMoveThing(Creature *player, const Thing* thing,
	const Position& fromPos, const Position& toPos, int count)
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

/*ground -> ground*/
bool Game::onPrepareMoveThing(Creature *creature, const Thing* thing,
	const Tile *fromTile, const Tile *toTile, int count)
{
	const Item *item = dynamic_cast<const Item*>(thing);
	const Player* player = dynamic_cast<const Player*>(thing);
	
	if(item && (!toTile || !item->canMovedTo(toTile))) {
	 	creature->sendCancel("Sorry, not possible.");
		return false;
	}
	else if(player && (!toTile || !thing->canMovedTo(toTile)) ) {
		player->sendTextMessage(MSG_SMALLINFO, "Sorry, not possible.");
		player->sendCancelWalk();

		//player->sendCancelWalk("Sorry, not possible.");
		return false;
	}
	else if(!player && toTile && toTile->ground && !toTile->ground->noFloorChange()) {
		creature->sendCancel("Sorry, not possible.");
		return false;
	}

	if (fromTile && fromTile->splash == thing && fromTile->splash->isNotMoveable()) {
			creature->sendCancel("You cannot move this object.");
#ifdef __DEBUG__
		cout << creature->getName() << " is trying to move a splash item!" << std::endl;
#endif
		return false;
	}
	else if (item && item->isNotMoveable()) {
			creature->sendCancel("You cannot move this object.");
#ifdef __DEBUG__
		cout << creature->getName() << " is trying to move an unmoveable item!" << std::endl;
#endif
		return false;
	}

	return thing->canMovedTo(toTile);
}

/*inventory -> container*/
bool Game::onPrepareMoveThing(Player *player, const Item* fromItem, slots_t fromSlot,
	const Container *toContainer, const Item *toItem, int count)
{
	if(!fromItem->isPickupable()) {		
		player->sendCancel("Sorry, not possible.");
		return false;
	}
	else {
		const Container *itemContainer = dynamic_cast<const Container*>(fromItem);
		if(itemContainer) {
			if(itemContainer->isHoldingItem(toContainer) || (toContainer == itemContainer)) {
				player->sendCancel("This is impossible.");
				return false;
			}
		}
		
		if((!fromItem->isStackable() || !toItem || fromItem->getID() != toItem->getID() || toItem->getItemCountOrSubtype() >= 100) && toContainer->size() + 1 > toContainer->capacity()) {
			player->sendCancel("Sorry not enough room.");
			return false;
		}
		
		Container const *topContainer = toContainer->getTopParent();
		if(!topContainer)
			topContainer = toContainer;
		if(topContainer->depot != 0 && player->max_depot_items != 0 && topContainer->getItemHoldingCount() >= player->max_depot_items){
			player->sendCancel("You can not put more items in this depot.");
			return false;
		}
	}

	return true;
}

/*container -> container*/
/*ground -> container*/
bool Game::onPrepareMoveThing(Player *player, const Item* fromItem, const Container *fromContainer,
	const Container *toContainer, const Item *toItem, int count)
{	
	if(!fromItem->isPickupable()) {		
		player->sendCancel("Sorry, not possible.");
		return false;
	}
	else {		
		double itemWeight = (fromItem->isStackable() ? Item::items[fromItem->getID()].weight * std::max(1, count) : fromItem->getWeight());
		if((!fromContainer || !player->isHoldingContainer(fromContainer)) && player->isHoldingContainer(toContainer)) {
			if(player->access == 0 && player->getFreeCapacity() < itemWeight) {
				player->sendCancel("This object is too heavy.");
				return false;
			}
		}
		const Container *itemContainer = dynamic_cast<const Container*>(fromItem);
		if(itemContainer) {
			if(itemContainer->isHoldingItem(toContainer) || (toContainer == itemContainer) || (fromContainer && fromContainer == itemContainer)) {
				player->sendCancel("This is impossible.");
				return false;
			}
		}
		
		if((!fromItem->isStackable() || !toItem || fromItem->getID() != toItem->getID() || toItem->getItemCountOrSubtype() >= 100) && toContainer->size() + 1 > toContainer->capacity()) {		
			player->sendCancel("Sorry not enough room.");
			return false;
		}
		
		Container const *topContainer = toContainer->getTopParent();
		if(!topContainer)
			topContainer = toContainer;
		if(topContainer->depot != 0 && player->max_depot_items != 0 && topContainer->getItemHoldingCount() >= player->max_depot_items){
			player->sendCancel("You can not put more items in this depot.");
			return false;
		}
	}

	return true;
}

/*ground -> ground*/
bool Game::onPrepareMoveCreature(Creature *creature, const Creature* creatureMoving,
	const Tile *fromTile, const Tile *toTile)
{
	const Player* playerMoving = dynamic_cast<const Player*>(creatureMoving);
	Player* player = dynamic_cast<Player*>(creature);

	if (creature->access == 0 && creature != creatureMoving && !creatureMoving->isPushable()) {		
		creature->sendCancel("Sorry, not possible.");
    return false;
  }
	if(!toTile){
		if(creature == creatureMoving && player) {
			player->sendTextMessage(MSG_SMALLINFO, "Sorry, not possible.");
			player->sendCancelWalk();
		}

		return false;
	}
  else if (playerMoving && toTile->isPz() && playerMoving->pzLocked) {
		if (creature == creatureMoving && creature->pzLocked) {

			if(player) {
				player->sendTextMessage(MSG_SMALLINFO, "You can not enter a protection zone after attacking another player.");
				player->sendCancelWalk();
			}

			//player->sendCancelWalk("You can not enter a protection zone after attacking another player.");
			return false;
		}
		else if (playerMoving->pzLocked) {			
			creature->sendCancel("Sorry, not possible.");
			return false;
		}
  }
  else if (playerMoving && fromTile->isPz() && !toTile->isPz() && creature != creatureMoving) {
		creature->sendCancel("Sorry, not possible.");
		return false;
  }

	return true;
}

/*ground -> inventory*/
bool Game::onPrepareMoveThing(Player *player, const Position& fromPos, const Item *item,
	slots_t toSlot, int count)
{
	if( (abs(player->pos.x - fromPos.x) > 1) || (abs(player->pos.y - fromPos.y) > 1) ) {
		player->sendCancel("To far away...");
		return false;
	}
	else if(!item->isPickupable()) {
		player->sendCancel("You cannot move this object.");
		return false;
	}

	double itemWeight = (item->isStackable() ? Item::items[item->getID()].weight * std::max(1, count) : item->getWeight());
	if(player->access == 0 && player->getFreeCapacity() < itemWeight) {
		player->sendCancel("This object is too heavy.");
		return false;
	}

	return true;
}

/*inventory -> inventory*/
bool Game::onPrepareMoveThing(Player *player, slots_t fromSlot, const Item *fromItem,
	slots_t toSlot, const Item *toItem, int count)
{
	if(toItem && (!toItem->isStackable() || toItem->getID() != fromItem->getID())) {
		player->sendCancel("Sorry, not enough room.");
		return false;
	}

	return true;
}

/*container -> inventory*/
bool Game::onPrepareMoveThing(Player *player, const Container *fromContainer, const Item *fromItem,
	slots_t toSlot, const Item *toItem, int count)
{
	double itemWeight = (fromItem->isStackable() ? Item::items[fromItem->getID()].weight * std::max(1, count) : fromItem->getWeight());
	if(player->access == 0 && !player->isHoldingContainer(fromContainer) &&
		player->getFreeCapacity() < itemWeight) {
		player->sendCancel("This object is too heavy.");
		return false;
	}

	if(toItem && (!toItem->isStackable() || toItem->getID() != fromItem->getID())) {
		player->sendCancel("Sorry, not enough room.");
		return false;
	}

	return true;
}

/*->inventory*/
bool Game::onPrepareMoveThing(Player *player, const Item *item,
	slots_t toSlot, int count)
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
					player->sendCancel("First remove the two-handed item.");
					return false;
				}
				return true	;				
			}
			else{
				if(player->items[SLOT_LEFT]){
					if(player->items[SLOT_LEFT]->getSlotPosition() & SLOTP_TWO_HAND){
						player->sendCancel("First remove the two-handed item.");
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
					player->sendCancel("First remove the two-handed item.");
					return false;
				}
				return true	;				
			}
			else{
				if(player->items[SLOT_RIGHT]){
					if(player->items[SLOT_RIGHT]->getSlotPosition() & SLOTP_TWO_HAND){
						player->sendCancel("First remove the two-handed item.");
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
void Game::thingMoveInternal(Player *player,
	unsigned char from_cid, unsigned char from_slotid, bool fromInventory,
	unsigned char to_cid, unsigned char to_slotid, bool toInventory,
	unsigned char count)
{
	Container *fromContainer = NULL;
	Container *toContainer = NULL;
	Item *fromItem = NULL;
	Item *toItem = NULL;

	if(fromInventory) {
		fromItem = player->getItem(from_cid);
		fromContainer = dynamic_cast<Container *>(fromItem);
	}
	else {
		fromContainer = player->getContainer(from_cid);

		if(fromContainer) {
			if(from_slotid >= fromContainer->size())
				return;

			fromItem = fromContainer->getItem(from_slotid);
		}
	}

	if(toInventory) {
		toItem = player->getItem(to_cid);
		toContainer = dynamic_cast<Container *>(toItem);
	}
	else {
		toContainer = player->getContainer(to_cid);

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
		if(onPrepareMoveThing(player, fromItem, fromContainer, toContainer, toItem, count)) {

			autoCloseTrade(fromItem);
			int oldFromCount = fromItem->getItemCountOrSubtype();
			int oldToCount = 0;

			//move around an item in a container
			if(fromItem->isStackable()) {
				if(toItem && toItem != fromItem && toItem->getID() == fromItem->getID())
				{
					oldToCount = toItem->getItemCountOrSubtype();
					int newToCount = std::min(100, oldToCount + count);
					toItem->setItemCountOrSubtype(newToCount);

					if(oldToCount != newToCount) {
						autoCloseTrade(toItem);
					}

					int subcount = oldFromCount - count;
					fromItem->setItemCountOrSubtype(subcount);

					int surplusCount = oldToCount + count  - 100;
					if(surplusCount > 0) {
						Item *surplusItem = Item::CreateItem(fromItem->getID(), surplusCount);
						if(onPrepareMoveThing(player, surplusItem, fromContainer, toContainer, NULL, count)) {
							autoCloseTrade(toContainer);
							toContainer->addItem(surplusItem);
						}
						else {
							delete surplusItem;
							count -= surplusCount; //re-define the actual amount we move.
							fromItem->setItemCountOrSubtype(fromItem->getItemCountOrSubtype() + surplusCount);
						}
					}
				}
				else if(count < oldFromCount) {
					int subcount = oldFromCount - count;
					fromItem->setItemCountOrSubtype(subcount);
	
					Item* moveItem = Item::CreateItem(fromItem->getID(), count);
					toContainer->addItem(moveItem);
					autoCloseTrade(toContainer);
				}
				else {
					if(fromContainer == toContainer) {
						fromContainer->moveItem(from_slotid, 0);
					}
					else if(fromContainer->removeItem(fromItem)) {
						autoCloseTrade(toContainer);
						toContainer->addItem(fromItem);
					}
				}

				if(fromItem->getItemCountOrSubtype() == 0) {
					fromContainer->removeItem(fromItem);
					this->FreeThing(fromItem);
				}
			}
			else {
				if(fromContainer == toContainer) {
					fromContainer->moveItem(from_slotid, 0);
				}
				else if(fromContainer->removeItem(fromItem)) {
					autoCloseTrade(toContainer);
					toContainer->addItem(fromItem);
				}
			}

			if(player->isHoldingContainer(fromContainer) != player->isHoldingContainer(toContainer)) {
				player->updateInventoryWeigth();
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
		}
	}
	else {
		//inventory to inventory
		if(fromInventory && toInventory && !toContainer) {
			if(onPrepareMoveThing(player, fromItem, (slots_t)to_cid, count) && onPrepareMoveThing(player, (slots_t)from_cid, fromItem, (slots_t)to_cid, toItem, count)) {

				autoCloseTrade(fromItem);
				int oldFromCount = fromItem->getItemCountOrSubtype();
				int oldToCount = 0;

				if(fromItem->isStackable()) {
					if(toItem && toItem != fromItem && toItem->getID() == fromItem->getID())
					{
						oldToCount = toItem->getItemCountOrSubtype();
						int newToCount = std::min(100, oldToCount + count);
						toItem->setItemCountOrSubtype(newToCount);

						if(oldToCount != newToCount) {
							autoCloseTrade(toItem);
						}

						int subcount = oldFromCount - count;
						fromItem->setItemCountOrSubtype(subcount);

						int surplusCount = oldToCount + count  - 100;
						if(surplusCount > 0) {
							fromItem->setItemCountOrSubtype(fromItem->getItemCountOrSubtype() + surplusCount);
							player->sendCancel("Sorry not enough room.");
						}

						if(fromItem->getItemCountOrSubtype() == 0) {
							player->removeItemInventory(from_cid, true);
							this->FreeThing(fromItem);
						}
					}
					else if(count < oldFromCount) {
						int subcount = oldFromCount - count;
						fromItem->setItemCountOrSubtype(subcount);
		
						autoCloseTrade(toItem);
						player->removeItemInventory(to_cid, true);
						player->addItemInventory(Item::CreateItem(fromItem->getID(), count), to_cid, true);

						if(fromItem->getItemCountOrSubtype() == 0) {
							player->removeItemInventory(from_cid, true);
							this->FreeThing(fromItem);
						}
					}
					else {
						if(player->removeItemInventory(from_cid, true)) {
							player->removeItemInventory(to_cid, true);
							player->addItemInventory(fromItem, to_cid, true);
						}
					}
				}
				else if(player->removeItemInventory(from_cid, true)) {
					player->removeItemInventory(to_cid, true);
					player->addItemInventory(fromItem, to_cid, true);
				}

				player->onThingMove(player, (slots_t)from_cid, fromItem, oldFromCount, (slots_t)to_cid, toItem, oldToCount, count);
			}
		}
		//container to inventory
		else if(!fromInventory && fromContainer && toInventory) {
			if(onPrepareMoveThing(player, fromItem, (slots_t)to_cid, count) && onPrepareMoveThing(player, fromContainer, fromItem, (slots_t)to_cid, toItem, count)) {
				autoCloseTrade(fromItem);
				int oldFromCount = fromItem->getItemCountOrSubtype();
				int oldToCount = 0;

				if(fromItem->isStackable()) {
					if(toItem && toItem != fromItem && toItem->getID() == fromItem->getID())
					{
						oldToCount = toItem->getItemCountOrSubtype();
						int newToCount = std::min(100, oldToCount + count);
						toItem->setItemCountOrSubtype(newToCount);

						if(oldToCount != newToCount) {
							autoCloseTrade(toItem);
						}

						int subcount = oldFromCount - count;
						fromItem->setItemCountOrSubtype(subcount);

						int surplusCount = oldToCount + count  - 100;
						if(surplusCount > 0) {
							fromItem->setItemCountOrSubtype(fromItem->getItemCountOrSubtype() + surplusCount);
							player->sendCancel("Sorry not enough room.");
						}

						if(fromItem->getItemCountOrSubtype() == 0) {
							fromContainer->removeItem(fromItem);
							this->FreeThing(fromItem);
						}
					}
					else if(count < oldFromCount) {
						int subcount = oldFromCount - count;
						fromItem->setItemCountOrSubtype(subcount);
		
						player->removeItemInventory(to_cid, true);
						player->addItemInventory(Item::CreateItem(fromItem->getID(), count), to_cid, true);

						if(toItem) {
							fromContainer->addItem(toItem);
						}

						if(fromItem->getItemCountOrSubtype() == 0) {
							fromContainer->removeItem(fromItem);
							this->FreeThing(fromItem);
						}
					}
					else {
						if(fromContainer->removeItem(fromItem)) {
							player->removeItemInventory(to_cid, true);
							player->addItemInventory(fromItem, to_cid, true);

							if(toItem) {
								fromContainer->addItem(toItem);
							}
						}
					}
				}
				else if(fromContainer->removeItem(fromItem)) {
					player->removeItemInventory(to_cid, true);
					player->addItemInventory(fromItem, to_cid, true);

					if(toItem) {
						fromContainer->addItem(toItem);
					}
				}

				if(!player->isHoldingContainer(fromContainer)) {
					player->updateInventoryWeigth();
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
			}
		}
		//inventory to container
		else if(fromInventory && toContainer) {
			int oldFromCount = fromItem->getItemCountOrSubtype();
			int oldToCount = 0;

			if(onPrepareMoveThing(player, fromItem, (slots_t)from_cid, toContainer, toItem, count)) {
				autoCloseTrade(fromItem);
				if(fromItem->isStackable()) {
					if(toItem && toItem != fromItem && toItem->getID() == fromItem->getID())
					{
						oldToCount = toItem->getItemCountOrSubtype();
						int newToCount = std::min(100, oldToCount + count);
						toItem->setItemCountOrSubtype(newToCount);

						if(oldToCount != newToCount) {
							autoCloseTrade(toItem);
						}

						int subcount = oldFromCount - count;
						fromItem->setItemCountOrSubtype(subcount);

						int surplusCount = oldToCount + count  - 100;
						if(surplusCount > 0) {
							Item *surplusItem = Item::CreateItem(fromItem->getID(), surplusCount);

							if(onPrepareMoveThing(player, surplusItem, NULL, toContainer, NULL, count)) {
								autoCloseTrade(toContainer);
								toContainer->addItem(surplusItem);
							}
							else {
								delete surplusItem;
								count -= surplusCount; //re-define the actual amount we move.
								fromItem->setItemCountOrSubtype(fromItem->getItemCountOrSubtype() + surplusCount);
							}
						}
					}
					else if(count < oldFromCount) {
						int subcount = oldFromCount - count;
						fromItem->setItemCountOrSubtype(subcount);
						autoCloseTrade(toContainer);
						toContainer->addItem(Item::CreateItem(fromItem->getID(), count));
					}
					else {
						if(player->removeItemInventory((slots_t)from_cid, true)) {
							autoCloseTrade(toContainer);
							toContainer->addItem(fromItem);
						}
					}

					if(fromItem->getItemCountOrSubtype() == 0) {
						player->removeItemInventory(from_cid, true);
						this->FreeThing(fromItem);
					}
				}
				else if(player->removeItemInventory(from_cid, true)) {
					autoCloseTrade(toContainer);
					toContainer->addItem(fromItem);
				}

				if(!player->isHoldingContainer(toContainer)) {
					player->updateInventoryWeigth();
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
			}
		}
	}
}

//container/inventory to ground
void Game::thingMoveInternal(Player *player,
	unsigned char from_cid, unsigned char from_slotid, bool fromInventory,
	const Position& toPos, unsigned char count)
{
	Container *fromContainer = NULL;
	Tile *toTile = getTile(toPos.x, toPos.y, toPos.z);
	if(!toTile)
		return;

	/*container to ground*/
	if(!fromInventory) {
		fromContainer = player->getContainer(from_cid);
		if(!fromContainer)
			return;
		
		Position fromPos = (fromContainer->pos.x == 0xFFFF ? player->pos : fromContainer->pos);			
		Item *fromItem = dynamic_cast<Item*>(fromContainer->getItem(from_slotid));
		Item *toItem = dynamic_cast<Item*>(toTile->getThingByStackPos(toTile->getThingCount() - 1));

		if(!fromItem || (toItem == fromItem) || (fromItem->isStackable() && count > fromItem->getItemCountOrSubtype()))
			return;

		if(onPrepareMoveThing(player, fromItem, fromPos, toPos, count) && onPrepareMoveThing(player, fromItem, NULL, toTile, count)) {
			autoCloseTrade(fromItem);
			int oldFromCount = fromItem->getItemCountOrSubtype();
			int oldToCount = 0;

			//Do action...
			if(fromItem->isStackable()) {
				if(toItem && toItem->getID() == fromItem->getID())
				{
					oldToCount = toItem->getItemCountOrSubtype();
					int newToCount = std::min(100, oldToCount + count);
					toItem->setItemCountOrSubtype(newToCount);

					if(oldToCount != newToCount) {
						autoCloseTrade(toItem);
					}

					int subcount = oldFromCount - count;
					fromItem->setItemCountOrSubtype(subcount);

					int surplusCount = oldToCount + count  - 100;
					if(surplusCount > 0) {
						Item *surplusItem = Item::CreateItem(fromItem->getID(), surplusCount);
						surplusItem->pos = toPos;
						
						toTile->addThing(surplusItem);
					}

					if(fromItem->getItemCountOrSubtype() == 0) {
						fromContainer->removeItem(fromItem);
						this->FreeThing(fromItem);
					}
				}
				else if(count < oldFromCount) {
					int subcount = oldFromCount - count;
					fromItem->setItemCountOrSubtype(subcount);

					Item *moveItem = Item::CreateItem(fromItem->getID(), count);
					moveItem->pos = toPos;
					toTile->addThing(moveItem);

					if(fromItem->getItemCountOrSubtype() == 0) {
						fromContainer->removeItem(fromItem);
						this->FreeThing(fromItem);
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

			if(player->isHoldingContainer(fromContainer)) {
				player->updateInventoryWeigth();
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
		}
	}
	else /*inventory to ground*/{
		Item *fromItem = player->getItem(from_cid);
		if(!fromItem)
			return;
		
		if(onPrepareMoveThing(player, fromItem, player->pos, toPos, count) && onPrepareMoveThing(player, fromItem, NULL, toTile, count)) {
			autoCloseTrade(fromItem);
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

					if(oldToCount != newToCount) {
						autoCloseTrade(toItem);
					}

					int subcount = oldFromCount - count;
					fromItem->setItemCountOrSubtype(subcount);

					int surplusCount = oldToCount + count  - 100;
					if(surplusCount > 0) {
						Item *surplusItem = Item::CreateItem(fromItem->getID(), surplusCount);
						surplusItem->pos = toPos;
						
						toTile->addThing(surplusItem);
					}

					if(fromItem->getItemCountOrSubtype() == 0) {
						player->removeItemInventory(from_cid, true);
						this->FreeThing(fromItem);
					}
				}
				else if(count < oldFromCount) {
					int subcount = oldFromCount - count;
					fromItem->setItemCountOrSubtype(subcount);

					Item *moveItem = Item::CreateItem(fromItem->getID(), count);
					moveItem->pos = toPos;
					toTile->addThing(moveItem);

					if(fromItem->getItemCountOrSubtype() == 0) {
						player->removeItemInventory(from_cid, true);
						this->FreeThing(fromItem);
					}
				}
				else if(player->removeItemInventory(from_cid, true)) {
					fromItem->pos = toPos;
					toTile->addThing(fromItem);
				}
			}
			else if(player->removeItemInventory(from_cid, true)) {
				fromItem->pos = toPos;
				toTile->addThing(fromItem);
			}

			player->updateInventoryWeigth();

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
		}
	}
}

//ground to container/inventory
void Game::thingMoveInternal(Player *player, const Position& fromPos, unsigned char stackPos,
	unsigned char to_cid, unsigned char to_slotid, bool toInventory, unsigned char count)
{
	Tile *fromTile = getTile(fromPos.x, fromPos.y, fromPos.z);
	if(!fromTile)
		return;

	Container *toContainer = NULL;

	Item *fromItem = dynamic_cast<Item*>(fromTile->getThingByStackPos(stackPos));
	Item *toItem = NULL;

	if(!fromItem)
		return;

	if(toInventory) {
		toItem = player->getItem(to_cid);
		toContainer = dynamic_cast<Container*>(toItem);
	}
	else {
		toContainer = player->getContainer(to_cid);
		if(!toContainer)
			return;

		toItem = toContainer->getItem(to_slotid);
		Container *toSlotContainer = dynamic_cast<Container*>(toItem);

		if(toSlotContainer) {
			toContainer = toSlotContainer;
			toItem = NULL;
		}
	}

	if(!fromItem || (toItem == fromItem) || (fromItem->isStackable() && count > fromItem->getItemCountOrSubtype()))
		return;

	/*ground to container*/
	if(toContainer) {
		if(onPrepareMoveThing(player, fromItem, fromPos, player->pos, count) &&
				onPrepareMoveThing(player, fromItem, NULL, toContainer, toItem, count))
		{
			autoCloseTrade(fromItem);
			int oldFromCount = fromItem->getItemCountOrSubtype();
			int oldToCount = 0;
			int stackpos = fromTile->getThingStackPos(fromItem);

			if(fromItem->isStackable()) {
				if(toItem && toItem->getID() == fromItem->getID()) {
					oldToCount = toItem->getItemCountOrSubtype();
					int newToCount = std::min(100, oldToCount + count);
					toItem->setItemCountOrSubtype(newToCount);

					if(oldToCount != newToCount) {
						autoCloseTrade(toItem);
					}

					int subcount = oldFromCount - count;
					fromItem->setItemCountOrSubtype(subcount);

					int surplusCount = oldToCount + count  - 100;
					if(surplusCount > 0) {
						Item *surplusItem = Item::CreateItem(fromItem->getID(), surplusCount);

						if(onPrepareMoveThing(player, surplusItem, NULL, toContainer, NULL, count)) {
							autoCloseTrade(toContainer);
							toContainer->addItem(surplusItem);
						}
						else {
							delete surplusItem;
							count -= surplusCount; //re-define the actual amount we move.
							fromItem->setItemCountOrSubtype(fromItem->getItemCountOrSubtype() + surplusCount);
						}
					}

					if(fromItem->getItemCountOrSubtype() == 0) {
						fromTile->removeThing(fromItem);
						this->FreeThing(fromItem);
					}
				}
				else if(count < oldFromCount) {
					int subcount = oldFromCount - count;
					fromItem->setItemCountOrSubtype(subcount);

					autoCloseTrade(toContainer);
					toContainer->addItem(Item::CreateItem(fromItem->getID(), count));

					if(fromItem->getItemCountOrSubtype() == 0) {
						fromTile->removeThing(fromItem);
						this->FreeThing(fromItem);
					}
				}
				else if(fromTile->removeThing(fromItem)) {
					autoCloseTrade(toContainer);
					toContainer->addItem(fromItem);
				}
			}
			else {
				if(fromTile->removeThing(fromItem)) {
					autoCloseTrade(toContainer);
					toContainer->addItem(fromItem);
				}
			}
				
			if(player->isHoldingContainer(toContainer)) {
				player->updateInventoryWeigth();
			}

			std::vector<Creature*> list;
			getSpectators(Range(fromPos, true), list);
			for(int i = 0; i < list.size(); ++i) {
				list[i]->onThingMove(player, fromPos, stackpos, fromItem, oldFromCount, toContainer, to_slotid, toItem, oldToCount, count);
			}
		}
	}
	//ground to inventory
	else if(toInventory) {
		if(onPrepareMoveThing(player, fromPos, fromItem, (slots_t)to_cid, count) && onPrepareMoveThing(player, fromItem, (slots_t)to_cid, count)) {
			autoCloseTrade(fromItem);
			int oldFromCount = fromItem->getItemCountOrSubtype();
			int oldToCount = 0;
			int stackpos = fromTile->getThingStackPos(fromItem);

			if(fromItem->isStackable()) {
				if(toItem && toItem->getID() == fromItem->getID()) {
					oldToCount = toItem->getItemCountOrSubtype();
					int newToCount = std::min(100, oldToCount + count);
					toItem->setItemCountOrSubtype(newToCount);

					if(oldToCount != newToCount) {
						autoCloseTrade(toItem);
					}

					int subcount = oldFromCount - count;
					fromItem->setItemCountOrSubtype(subcount);

					int surplusCount = oldToCount + count  - 100;
					if(surplusCount > 0) {
						fromItem->setItemCountOrSubtype(fromItem->getItemCountOrSubtype() + surplusCount);
						player->sendCancel("Sorry not enough room.");
					}

					if(fromItem->getItemCountOrSubtype() == 0) {
						fromTile->removeThing(fromItem);
						this->FreeThing(fromItem);
					}
				}
				else if(count < oldFromCount) {
					int subcount = oldFromCount - count;
					fromItem->setItemCountOrSubtype(subcount);

					player->removeItemInventory(to_cid, true);
					player->addItemInventory(Item::CreateItem(fromItem->getID(), count), to_cid, true);

					if(toItem) {
						autoCloseTrade(toItem);
						fromTile->addThing(toItem);
						toItem->pos = fromPos;
					}

					if(fromItem->getItemCountOrSubtype() == 0) {
						fromTile->removeThing(fromItem);
						this->FreeThing(fromItem);
					}
				}
				else {
					if(fromTile->removeThing(fromItem)) {
						player->removeItemInventory(to_cid, true);
						player->addItemInventory(fromItem, to_cid, true);

						if(toItem) {
							autoCloseTrade(toItem);
							fromTile->addThing(toItem);
							toItem->pos = fromPos;
						}
					}
				}
			}
			else {
				if(fromTile->removeThing(fromItem)) {
					player->removeItemInventory(to_cid, true);
					player->addItemInventory(fromItem, to_cid, true);

					if(toItem) {
						autoCloseTrade(toItem);
						fromTile->addThing(toItem);
						toItem->pos = fromPos;
					}
				}
			}

			player->updateInventoryWeigth();

			std::vector<Creature*> list;
			getSpectators(Range(fromPos, true), list);
			for(int i = 0; i < list.size(); ++i) {
				list[i]->onThingMove(player, fromPos, stackpos, fromItem, oldFromCount, (slots_t)to_cid, toItem, oldToCount, count);
			}
		}
	}
}

//ground to ground
void Game::thingMoveInternal(Creature *creature, unsigned short from_x, unsigned short from_y, unsigned char from_z,
	unsigned char stackPos, unsigned short to_x, unsigned short to_y, unsigned char to_z, unsigned char count)
{
	Tile *fromTile = getTile(from_x, from_y, from_z);
	if(!fromTile)
		return;
	Tile *toTile   = getTile(to_x, to_y, to_z);
	/*if(!toTile){
		if(dynamic_cast<Player*>(player))
			dynamic_cast<Player*>(player)->sendCancelWalk("Sorry, not possible...");
		return;
	}*/
	Thing *thing = fromTile->getThingByStackPos(stackPos);

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
		Item* item = dynamic_cast<Item*>(thing);
		Creature* creatureMoving = dynamic_cast<Creature*>(thing);
		Player* playerMoving = dynamic_cast<Player*>(creatureMoving);
		Player* player = dynamic_cast<Player*>(creature);
		
		Position oldPos;
		oldPos.x = from_x;
		oldPos.y = from_y;
		oldPos.z = from_z;

		if(fromTile)
		{
			if(!toTile && creature == creatureMoving){      
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
						//teleport(playerMoving, Position(playerMoving->pos.x, playerMoving->pos.y+2, playerMoving->pos.z+1));
						teleport(playerMoving, Position(to_x, to_y + 1, playerMoving->pos.z+1));
					}
					else if(downTile->floorChange(SOUTH)){
						//teleport(playerMoving, Position(playerMoving->pos.x, playerMoving->pos.y-2, playerMoving->pos.z+1));
						teleport(playerMoving, Position(to_x, to_y - 1, playerMoving->pos.z+1));
					}
					else if(downTile->floorChange(EAST)){
						//teleport(playerMoving, Position(playerMoving->pos.x-2, playerMoving->pos.y, playerMoving->pos.z+1));
						teleport(playerMoving, Position(to_x - 1, to_y, playerMoving->pos.z+1));
					}
					else if(downTile->floorChange(WEST)){
						//teleport(playerMoving, Position(playerMoving->pos.x+2, playerMoving->pos.y, playerMoving->pos.z+1));
						teleport(playerMoving, Position(to_x + 1, to_y, playerMoving->pos.z+1));
					}
					else {
						if(player) {
							player->sendTextMessage(MSG_SMALLINFO, "Sorry, not possible.");
							player->sendCancelWalk();
						}

						//player->sendCancelWalk("Sorry, not possible.");
						return;
					}
				}
				//change level end   
				else if(player) {
					player->sendTextMessage(MSG_SMALLINFO, "Sorry, not possible.");
					player->sendCancelWalk();
					//creature->sendCancelWalk("Sorry, not possible.");
				}
				
				return;
			}

			if(!onPrepareMoveThing(creature, thing, Position(from_x, from_y, from_z), Position(to_x, to_y, to_z), count))
				return;
			
			if(creatureMoving && !onPrepareMoveCreature(creature, creatureMoving, fromTile, toTile))
				return;
			
			if(!onPrepareMoveThing(creature, thing, fromTile, toTile, count))
				return;

			Teleport *teleportitem = toTile->getTeleportItem();
			if(teleportitem) {
				teleport(thing, teleportitem->getDestPos());
				return;
			}
			
			if(creatureMoving){
				// we need to update the direction the player is facing to...
				// otherwise we are facing some problems in turning into the
				// direction we were facing before the movement
				// check y first cuz after a diagonal move we lock to east or west
				if (to_y < oldPos.y) creatureMoving->direction = NORTH;
				if (to_y > oldPos.y) creatureMoving->direction = SOUTH;
				if (to_x > oldPos.x) creatureMoving->direction = EAST;
				if (to_x < oldPos.x) creatureMoving->direction = WEST;
			}

			int oldstackpos = fromTile->getThingStackPos(thing);
			if (fromTile && fromTile->removeThing(thing))
			{
				toTile->addThing(thing);
				
				thing->pos.x = to_x;
				thing->pos.y = to_y;
				thing->pos.z = to_z;
				
				if (playerMoving) {
					if(playerMoving->attackedCreature != 0) {
						Creature* attackedCreature = getCreatureByID(creatureMoving->attackedCreature);
						if(attackedCreature){      
							if((std::abs(creatureMoving->pos.x - attackedCreature->pos.x) > 8) ||
							(std::abs(creatureMoving->pos.y - attackedCreature->pos.y) > 5) || (creatureMoving->pos.z != attackedCreature->pos.z)){
								player->sendTextMessage(MSG_SMALLINFO, "Target lost.");
								playerSetAttackedCreature(playerMoving, 0);
							}
						}
					}
					else if(playerMoving->tradePartner != 0) {
						Creature* tradePartner = getCreatureByID(playerMoving->tradePartner);
						if(tradePartner) {
							if((std::abs(playerMoving->pos.x - tradePartner->pos.x) > 2) ||
							(std::abs(playerMoving->pos.y - tradePartner->pos.y) > 2) || (playerMoving->pos.z != tradePartner->pos.z)){
								playerCloseTrade(playerMoving);
							}
						}
					}
				}

				autoCloseTrade(item);
				std::vector<Creature*> list;
				getSpectators(Range(oldPos, Position(to_x, to_y, to_z)), list);
				
				for(unsigned int i = 0; i < list.size(); ++i)
				{
					list[i]->onThingMove(creature, thing, &oldPos, oldstackpos, 1, 1);
				}

				//change level begin
				if(playerMoving && toTile->ground && !(toTile->ground->noFloorChange())){          
					Tile* downTile = getTile(to_x, to_y, to_z+1);
					if(downTile){
						//diagonal begin
						if(downTile->floorChange(NORTH) && downTile->floorChange(EAST)){
							teleport(playerMoving, Position(playerMoving->pos.x-1, playerMoving->pos.y+1, playerMoving->pos.z+1));
							//teleport(thing, Position(from_x-1, from_y+1, from_z+1));
						}
						else if(downTile->floorChange(NORTH) && downTile->floorChange(WEST)){
							teleport(playerMoving, Position(playerMoving->pos.x+1, playerMoving->pos.y+1, playerMoving->pos.z+1));
							//teleport(thing, Position(from_x+1, from_y+1, from_z+1));
						}
						else if(downTile->floorChange(SOUTH) && downTile->floorChange(EAST)){
							teleport(playerMoving, Position(playerMoving->pos.x-1, playerMoving->pos.y-1, playerMoving->pos.z+1));
							//teleport(thing, Position(from_x-1, from_y-1, from_z+1));
						}
						else if(downTile->floorChange(SOUTH) && downTile->floorChange(WEST)){
							teleport(playerMoving, Position(playerMoving->pos.x+1, playerMoving->pos.y-1, playerMoving->pos.z+1));
							//teleport(thing, Position(from_x+1, from_y-1, from_z+1));
						}                          
						//diagonal end
						else if(downTile->floorChange(NORTH)){
							teleport(playerMoving, Position(playerMoving->pos.x, playerMoving->pos.y+1, playerMoving->pos.z+1));
							//teleport(thing, Position(from_x, from_y+1, from_z+1));
						}
						else if(downTile->floorChange(SOUTH)){
							teleport(playerMoving, Position(playerMoving->pos.x, playerMoving->pos.y-1, playerMoving->pos.z+1));
							//teleport(thing, Position(from_x, from_y-1, from_z+1));
						}
						else if(downTile->floorChange(EAST)){
							teleport(playerMoving, Position(playerMoving->pos.x-1, playerMoving->pos.y, playerMoving->pos.z+1));
							//teleport(thing, Position(from_x-1, from_y, from_z+1));
						}
						else if(downTile->floorChange(WEST)){
							teleport(playerMoving, Position(playerMoving->pos.x+1, playerMoving->pos.y, playerMoving->pos.z+1));
							//teleport(thing, Position(from_x+1, from_y, from_z+1));
						}
					}
				}
				//diagonal begin
				else if(playerMoving && toTile->floorChange(NORTH) && toTile->floorChange(EAST)){
					teleport(playerMoving, Position(playerMoving->pos.x+1, playerMoving->pos.y-1, playerMoving->pos.z-1));
					//teleport(thing, Position(from_x+1, from_y-1, from_z-1));
				}
				else if(playerMoving && toTile->floorChange(NORTH) && toTile->floorChange(WEST)){
					teleport(playerMoving, Position(playerMoving->pos.x-1, playerMoving->pos.y-1, playerMoving->pos.z-1));
					//teleport(thing, Position(from_x-1, from_y-1, from_z-1));
				}
				else if(playerMoving && toTile->floorChange(SOUTH) && toTile->floorChange(EAST)){
					teleport(playerMoving, Position(playerMoving->pos.x+1, playerMoving->pos.y+1, playerMoving->pos.z-1));
					//teleport(thing, Position(from_x+1, from_y+1, from_z-1));
				}
				else if(playerMoving && toTile->floorChange(SOUTH) && toTile->floorChange(WEST)){
					teleport(playerMoving, Position(playerMoving->pos.x-1, playerMoving->pos.y+1, playerMoving->pos.z-1));
					//teleport(thing, Position(from_x-1, from_y+1, from_z-1));
				}
				//diagonal end                            
				else if(playerMoving && toTile->floorChange(NORTH)){
					teleport(playerMoving, Position(playerMoving->pos.x, playerMoving->pos.y-1, playerMoving->pos.z-1));
					//teleport(thing, Position(from_x, from_y-1, from_z-1));
				}
				else if(playerMoving && toTile->floorChange(SOUTH)){
					teleport(playerMoving, Position(playerMoving->pos.x, playerMoving->pos.y+1, playerMoving->pos.z-1));
					//teleport(thing, Position(from_x, from_y+1, from_z-1));
				}
				else if(playerMoving && toTile->floorChange(EAST)){
					teleport(playerMoving, Position(playerMoving->pos.x+1, playerMoving->pos.y, playerMoving->pos.z-1));
					//teleport(thing, Position(from_x+1, from_y, from_z-1));
				}
				else if(playerMoving && toTile->floorChange(WEST)){
					teleport(playerMoving, Position(playerMoving->pos.x-1, playerMoving->pos.y, playerMoving->pos.z-1));
					//teleport(thing, Position(from_x-1, from_y, from_z-1));
				}                                      
				//change level end

				if(creatureMoving) {
					const MagicEffectItem* fieldItem = toTile->getFieldItem();
					
					if(fieldItem) {
						const MagicEffectTargetCreatureCondition *magicTargetCondition = fieldItem->getCondition();

						if(!(getWorldType() == WORLD_TYPE_NO_PVP && playerMoving && magicTargetCondition && magicTargetCondition->getOwnerID() != 0)) {
							fieldItem->getDamage(creatureMoving);
						}
						
						if(magicTargetCondition && ((magicTargetCondition->attackType == ATTACK_FIRE) || 
								(magicTargetCondition->attackType == ATTACK_POISON) ||
								(magicTargetCondition->attackType == ATTACK_ENERGY))) {	
							Creature *c = getCreatureByID(magicTargetCondition->getOwnerID());
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
	//OTSYS_THREAD_LOCK(gameLock)
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);

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

   //OTSYS_THREAD_UNLOCK(gameLock)
}

void BanIPAddress(std::pair<unsigned long, unsigned long>& IpNetMask)
{
	bannedIPs.push_back(IpNetMask);
}

void Game::addCommandTag(std::string tag){
	bool found = false;
	for(int i=0;i< commandTags.size() ;i++){
		if(commandTags[i] == tag){
			found = true;
			break;
		}
	}
	if(!found){
		commandTags.push_back(tag);
	}
}

void Game::resetCommandTag(){
	commandTags.clear();
}

void Game::creatureSay(Creature *creature, SpeakClasses type, const std::string &text)
{
	//OTSYS_THREAD_LOCK(gameLock)
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);
	bool GMcommand = false;
	// First, check if this was a GM command
	for(int i=0;i< commandTags.size() ;i++){
		if(commandTags[i] == text.substr(0,1)){
			if(commands.exeCommand(creature,text)){
				GMcommand = true;
			}
			break;
		}
	}
	if(!GMcommand){
		// It was no command, or it was just a player
		std::vector<Creature*> list;
		getSpectators(Range(creature->pos), list);

		for(unsigned int i = 0; i < list.size(); ++i)
		{
			list[i]->onCreatureSay(creature, type, text);
		}

	}
	//OTSYS_THREAD_UNLOCK(gameLock)
}

void Game::teleport(Thing *thing, const Position& newPos) {

	if(newPos == thing->pos)  
		return; 
	
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);
	
	Tile *toTile = getTile( newPos.x, newPos.y, newPos.z );
	if(toTile){
		Creature *creature = dynamic_cast<Creature*>(thing); 
		if(creature){
			Tile *fromTile = getTile( thing->pos.x, thing->pos.y, thing->pos.z );
			if(!fromTile)
				return;
			
			int osp = fromTile->getThingStackPos(thing);  
			fromTile->removeThing(thing);
			toTile->addThing(thing);
			Position oldPos = thing->pos;
			
			std::vector<Creature*> list;
			getSpectators(Range(thing->pos, true), list);
			for(size_t i = 0; i < list.size(); ++i) {
				list[i]->onCreatureDisappear(creature, osp, true);
			}
			
			if(newPos.y < oldPos.y)
				creature->direction = NORTH;
			if(newPos.y > oldPos.y)
				creature->direction = SOUTH;
			if(newPos.x > oldPos.x && (std::abs(newPos.x - oldPos.x) >= std::abs(newPos.y - oldPos.y)) )
				creature->direction = EAST;
			if(newPos.x < oldPos.x && (std::abs(newPos.x - oldPos.x) >= std::abs(newPos.y - oldPos.y)))
				creature->direction = WEST;
			
			Player *player = dynamic_cast<Player*>(creature);
			if(player && player->attackedCreature != 0){
				Creature* attackedCreature = getCreatureByID(player->attackedCreature);
				if(attackedCreature){
					if((std::abs(newPos.x - attackedCreature->pos.x) > 8) ||
					(std::abs(newPos.y - attackedCreature->pos.y) > 5) || (newPos.z != attackedCreature->pos.z)){
						player->sendTextMessage(MSG_SMALLINFO, "Target lost.");
						playerSetAttackedCreature(player, 0);
					}
				}
			}
			
			thing->pos = newPos;
			list.clear();
			getSpectators(Range(thing->pos, true), list);
			for(size_t i = 0; i < list.size(); ++i){
				list[i]->onTeleport(creature, &oldPos, osp);
			}
		}
		else{
			removeThing(NULL, thing->pos, thing, false);
			addThing(NULL,newPos,thing);
		}
	}//if(toTile)

}


void Game::creatureChangeOutfit(Creature *creature)
{
	//OTSYS_THREAD_LOCK(gameLock)
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);

	std::vector<Creature*> list;
	getSpectators(Range(creature->pos, true), list);

	for(unsigned int i = 0; i < list.size(); ++i)
	{
		list[i]->onCreatureChangeOutfit(creature);
	}

	//OTSYS_THREAD_UNLOCK(gameLock)
}

void Game::creatureWhisper(Creature *creature, const std::string &text)
{
	//OTSYS_THREAD_LOCK(gameLock)
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);

	std::vector<Creature*> list;
	getSpectators(Range(creature->pos), list);

	for(unsigned int i = 0; i < list.size(); ++i)
	{
		if(abs(creature->pos.x - list[i]->pos.x) > 1 || abs(creature->pos.y - list[i]->pos.y) > 1)
			list[i]->onCreatureSay(creature, SPEAK_WHISPER, std::string("pspsps"));
		else
			list[i]->onCreatureSay(creature, SPEAK_WHISPER, text);
	}

  //OTSYS_THREAD_UNLOCK(gameLock)
}

void Game::creatureYell(Creature *creature, std::string &text)
{
	//OTSYS_THREAD_LOCK(gameLock)
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);

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
  
	//OTSYS_THREAD_UNLOCK(gameLock)
}

void Game::creatureSpeakTo(Creature *creature, const std::string &receiver, const std::string &text)
{
	//OTSYS_THREAD_LOCK(gameLock) 
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);
	Creature* c = getCreatureByName(receiver);
	if(c)
		c->onCreatureSay(creature, SPEAK_PRIVATE, text);
	//OTSYS_THREAD_UNLOCK(gameLock)
}

void Game::creatureMonsterYell(Monster* monster, const std::string& text) 
{
	//OTSYS_THREAD_LOCK(gameLock)
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);

	std::vector<Creature*> list;
	map->getSpectators(Range(monster->pos, 18, 18, 14, 14), list);

	for(unsigned int i = 0; i < list.size(); ++i) {
		list[i]->onCreatureSay(monster, SPEAK_MONSTER1, text);
	}

  //OTSYS_THREAD_UNLOCK(gameLock)
}

void Game::creatureBroadcastMessage(Creature *creature, const std::string &text)
{
	if(creature->access == 0) 
		return;

	//OTSYS_THREAD_LOCK(gameLock)
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);

	//for (cit = playersOnline.begin(); cit != playersOnline.end(); cit++)
	for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it)
	{
		(*it).second->onCreatureSay(creature, SPEAK_BROADCAST, text);
	}

	//OTSYS_THREAD_UNLOCK(gameLock)
}

void Game::creatureToChannel(Creature *creature, SpeakClasses type, const std::string &text, unsigned short channelId)
{

	//OTSYS_THREAD_LOCK(gameLock)
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);

	if(creature->access == 0){
		type = SPEAK_CHANNEL_Y;
	}
	std::map<long, Creature*>::iterator cit;
	for (cit = channel.begin(); cit != channel.end(); cit++)
	{
		Player* player = dynamic_cast<Player*>(cit->second);
		if(player){
			player->sendToChannel(creature, type, text, channelId);
		}
	}

	//OTSYS_THREAD_UNLOCK(gameLock)
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
	//OTSYS_THREAD_LOCK(gameLock)
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);
	
#ifdef __DEBUG__
	cout << "creatureMakeMagic: " << (creature ? creature->getName() : "No name") << ", x: " << centerpos.x << ", y: " << centerpos.y << ", z: " << centerpos.z << std::endl;
#endif

	Position frompos;

	if(creature) {
		frompos = creature->pos;

		if(!creatureOnPrepareMagicAttack(creature, centerpos, me))
		{
      //OTSYS_THREAD_UNLOCK(gameLock)
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
			if(!t->isBlocking() && map->canThrowItemTo(frompos, (*maIt), false, true) && !t->floorChange()) {
				
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
		//OTSYS_THREAD_UNLOCK(gameLock)
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
	bool hasTarget = false;
	bool isBlocking = true;
	if(targettile){
		hasTarget = !targettile->creatures.empty();
		isBlocking = targettile->isBlocking();
	}

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
						if(creature && target->isRemoved == true) {

							for(std::vector<Creature*>::const_iterator cit = creatureState.attackerlist.begin(); cit != creatureState.attackerlist.end(); ++cit) {
								Creature* gainExpCreature = *cit;
								if(dynamic_cast<Player*>(gainExpCreature))
									dynamic_cast<Player*>(gainExpCreature)->sendStats();
								
								if(spectator->CanSee(gainExpCreature->pos.x, gainExpCreature->pos.y, gainExpCreature->pos.z)) {
									std::stringstream exp;
									exp << target->getGainedExperience(gainExpCreature);
									spectator->sendAnimatedText(gainExpCreature->pos, 983, exp.str());
									//msg.AddAnimatedText(gainexpCreature->pos, 983, exp.str());
									//spectator->sendStats();
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

	//OTSYS_THREAD_UNLOCK(gameLock)
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

		if(creature->manaShieldTicks < 1000 || (creature->mana == 0))
			creature->drainHealth(outDamage);
		else if(outManaDamage > 0){
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
	//OTSYS_THREAD_LOCK(gameLock)
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);

	bool ret = creatureMakeMagic(creature, centerpos, &me);

	//OTSYS_THREAD_UNLOCK(gameLock)

	return ret;
}

bool Game::creatureThrowRune(Creature *creature, const Position& centerpos, const MagicEffectClass& me) {
	//OTSYS_THREAD_LOCK(gameLock)
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);

	bool ret = false;	
	if(creature->pos.z != centerpos.z) {	
		creature->sendCancel("You need to be on the same floor.");
	}
	else if(!map->canThrowItemTo(creature->pos, centerpos, false, true)) {		
		creature->sendCancel("You cannot throw there.");
	}
	else
		ret = creatureMakeMagic(creature, centerpos, &me);

	//OTSYS_THREAD_UNLOCK(gameLock)

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
					playerSetAttackedCreature(player, 0);
					//player->sendCancelAttacking();
				}

				return false;
			}
			else if(targettile && targettile->isPz()) {
				if(player) {					
					player->sendTextMessage(MSG_SMALLINFO, "You may not attack a person in a protection zone.");
					playerSetAttackedCreature(player, 0);
					//player->sendCancelAttacking();
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
					if(me->offensive) {
						player->sendTextMessage(MSG_SMALLINFO, "You are exhausted.",player->pos, NM_ME_PUFF);
						player->exhaustedTicks += (long)g_config.getGlobalNumber("exhaustedadd", 0);
					}

					return false;
				}
				else if(player->mana < me->manaCost) {															
					player->sendTextMessage(MSG_SMALLINFO, "You do not have enough mana.",player->pos, NM_ME_PUFF);					
					return false;
				}
				else
					player->mana -= me->manaCost;
					//player->manaspent += me->manaCost;
					player->addManaSpent(me->manaCost);
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
			
	//OTSYS_THREAD_LOCK(gameLock)
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);
	
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
	
	if(!inReach){
		/*
		if(player) {
			player->sendTextMessage(MSG_SMALLINFO, "Target lost.");
			playerSetAttackedCreature(player, 0);
		}
		*/

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
				if(dynamic_cast<Player*>(gainexpCreature))
					dynamic_cast<Player*>(gainexpCreature)->sendStats();
				
				if(spectator->CanSee(gainexpCreature->pos.x, gainexpCreature->pos.y, gainexpCreature->pos.z)) {
					std::stringstream exp;
					exp << attackedCreature->getGainedExperience(gainexpCreature);
					spectator->sendAnimatedText(gainexpCreature->pos, 983, exp.str());
					//msg.AddAnimatedText(gainexpCreature->pos, 983, exp.str());
					//spectator->sendStats();
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

	if(damagetype != FIGHT_MELEE && player) {
		player->removeDistItem();
	}
		
	//OTSYS_THREAD_UNLOCK(gameLock)
}

std::list<Position> Game::getPathTo(Creature *creature, Position start, Position to, bool creaturesBlock){
	return map->getPathTo(creature, start, to, creaturesBlock);
}

void Game::checkPlayerWalk(unsigned long id)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);
	Player *player = dynamic_cast<Player*>(getCreatureByID(id));

	if(!player)
		return;

	Position pos = player->pos;
	Direction dir = player->pathlist.front();
	player->pathlist.pop_front();

	switch (dir) {
		case NORTH:
			pos.y--;
			break;
		case EAST:
			pos.x++;
			break;
		case SOUTH:
			pos.y++;
			break;
		case WEST:
			pos.x--;
			break;
		case NORTHEAST:
			pos.x++;
			pos.y--;
			break;
		case NORTHWEST:
			pos.x--;
			pos.y--;
			break;
		case SOUTHWEST:
			pos.x--;
			pos.y++;
			break;
		case SOUTHEAST:
			pos.x++;
			pos.y++;
			break;
	}

/*
#ifdef __DEBUG__
	std::cout << "move to: " << dir << std::endl;
#endif
*/

	player->lastmove = OTSYS_TIME();
	this->thingMove(player, player, pos.x, pos.y, pos.z, 1);
	flushSendBuffers();

	if(!player->pathlist.empty()) {
		int ticks = (int)player->getSleepTicks();
/*
#ifdef __DEBUG__
		std::cout << "checkPlayerWalk - " << ticks << std::endl;
#endif
*/
		player->eventAutoWalk = addEvent(makeTask(ticks, std::bind2nd(std::mem_fun(&Game::checkPlayerWalk), id)));
	}
	else
		player->eventAutoWalk = 0;
}

void Game::checkCreature(unsigned long id)
{
	//OTSYS_THREAD_LOCK(gameLock)
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);
	Creature *creature = getCreatureByID(id);

	if (creature && creature->isRemoved == false)
	{
		creature->eventCheck = 0;
		
		int thinkTicks = 0;
		int oldThinkTicks = creature->onThink(thinkTicks);
		
		if(thinkTicks > 0) {
			creature->eventCheck = addEvent(makeTask(thinkTicks, std::bind2nd(std::mem_fun(&Game::checkCreature), id)));
		}

		Player* player = dynamic_cast<Player*>(creature);
		if(player){
			Tile *tile = getTile(player->pos.x, player->pos.y, player->pos.z);
			if(tile == NULL){
				std::cout << "CheckPlayer NULL tile: " << player->getName() << std::endl;
				return;
			}
				
			if(!tile->isPz()){
				if(player->food > 1000){
					player->mana += min(5, player->manamax - player->mana);
					player->food -= thinkTicks;
					if(player->healthmax - player->health > 0){
						player->health += min(5, player->healthmax - player->health);
						std::vector<Creature*> list;
						getSpectators(Range(creature->pos), list);

						for(unsigned int i = 0; i < list.size(); i++){
							Player* p = dynamic_cast<Player*>(list[i]);
							if(p)
								p->sendCreatureHealth(player);
						}
					}
				}				
			}

			//send stast only if have changed
			if(player->NeedUpdateStats()){
				player->sendStats();
			}
			
			player->sendPing();

			if(player->inFightTicks >= 1000) {
				player->inFightTicks -= thinkTicks;
				
				if(player->inFightTicks < 1000)
					player->pzLocked = false;
					player->sendIcons(); 
			}
			
			if(player->exhaustedTicks >=1000){
				player->exhaustedTicks -= thinkTicks;
			}
			
			if(player->manaShieldTicks >=1000){
				player->manaShieldTicks -= thinkTicks;
				
				if(player->manaShieldTicks  < 1000)
					player->sendIcons();
			}
			
			if(player->hasteTicks >=1000){
				player->hasteTicks -= thinkTicks;
			}	
		}
		else {
			if(creature->manaShieldTicks >=1000){
				creature->manaShieldTicks -= thinkTicks;
			}
				
			if(creature->hasteTicks >=1000){
				creature->hasteTicks -= thinkTicks;
			}
		}

		Conditions& conditions = creature->getConditions();
		for(Conditions::iterator condIt = conditions.begin(); condIt != conditions.end(); ++condIt) {
			if(condIt->first == ATTACK_FIRE || condIt->first == ATTACK_ENERGY || condIt->first == ATTACK_POISON) {
				ConditionVec &condVec = condIt->second;

				if(condVec.empty())
					continue;

				CreatureCondition& condition = condVec[0];

				if(condition.onTick(oldThinkTicks)) {
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
	
	//OTSYS_THREAD_UNLOCK(gameLock)
}

void Game::changeOutfit(unsigned long id, int looktype){
     //OTSYS_THREAD_LOCK(gameLock)
     OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);
     
     Creature *creature = getCreatureByID(id);
     if(creature){
		creature->looktype = looktype;
		creatureChangeOutfit(creature);
     }
     
     //OTSYS_THREAD_UNLOCK(gameLock)
     }

void Game::changeOutfitAfter(unsigned long id, int looktype, long time){

     addEvent(makeTask(time, 
     boost::bind(
     &Game::changeOutfit, this,
     id, looktype)));
     
}

void Game::changeSpeed(unsigned long id, unsigned short speed)
{
    //OTSYS_THREAD_LOCK(gameLock) 
    OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);
	Creature *creature = getCreatureByID(id);
	if(creature && creature->hasteTicks < 1000 && creature->speed != speed)
	{
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

	//OTSYS_THREAD_UNLOCK(gameLock)
}

void Game::checkCreatureAttacking(unsigned long id)
{
	//OTSYS_THREAD_LOCK(gameLock)
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);

	Creature *creature = getCreatureByID(id);
	if (creature != NULL && creature->isRemoved == false)
	{
		creature->eventCheckAttacking = 0;
		Monster *monster = dynamic_cast<Monster*>(creature);
		if (monster) {
			monster->onAttack();
		}
		else {
			if (creature->attackedCreature != 0)
			{
				Creature *attackedCreature = getCreatureByID(creature->attackedCreature);
				if (attackedCreature)
				{
					Tile* fromtile = getTile(creature->pos.x, creature->pos.y, creature->pos.z);
					if(fromtile == NULL) {
						std::cout << "checkCreatureAttacking NULL tile: " << creature->getName() << std::endl;
						//return;
					}
					if (!attackedCreature->isAttackable() == 0 && fromtile && fromtile->isPz() && creature->access == 0)
					{
						Player* player = dynamic_cast<Player*>(creature);
						if (player) {							
							player->sendTextMessage(MSG_SMALLINFO, "You may not attack a person in a protection zone.");
							//player->sendCancelAttacking();
							playerSetAttackedCreature(player, 0);
							return;
						}
					}
					else
					{
						if (attackedCreature != NULL && attackedCreature->isRemoved == false)
						{
							this->creatureMakeDamage(creature, attackedCreature, creature->getFightType());
						}
					}

					creature->eventCheckAttacking = addEvent(makeTask(2000, std::bind2nd(std::mem_fun(&Game::checkCreatureAttacking), id)));
				}
			}
		}
		flushSendBuffers();
	}

	//OTSYS_THREAD_UNLOCK(gameLock)
}

//void Game::decayItem(Position& pos, unsigned short id, unsigned char stackpos)
/*void Game::decayItem(Item *item)
{
	OTSYS_THREAD_LOCK(gameLock)

	/*todo: Decaying item could be in a  container carried by a player,
		should all items have a pointer to their parent (like containers)?*/
	/*if(item && item->pos.x != 0xFFFF) {
		Tile *tile = getTile(item->pos.x, item->pos.y, item->pos.z);
		//MapState mapstate(this->map);
		Position pos;
		
		MagicEffectItem* magicItem = dynamic_cast<MagicEffectItem*>(item);

		if(magicItem) {
			pos = magicItem->pos;
			int stackpos = tile->getThingStackPos(magicItem);
			if(magicItem->transform()) {
				//mapstate.replaceThing(tile, magicItem, magicItem);				
				sendUpdateThing(NULL,pos,magicItem,stackpos);
				//addEvent(makeTask(magicItem->getDecayTime(), boost::bind(&Game::decayItem, this, magicItem->pos, magicItem->getID(), tile->getThingStackPos(magicItem)) ) );
				addEvent(makeTask(magicItem->getDecayTime(), std::bind2nd(std::mem_fun(&Game::decayItem), magicItem)));
			}
			else {
				sendRemoveThing(NULL,pos,magicItem,stackpos);
				removeThing(NULL,pos,magicItem);
				//mapstate.removeThing(tile, item);
				//tile->removeThing(magicItem);	
				//delete magicItem;
				FreeThing(magicItem);
			}			
			//creatureBroadcastTileUpdated(pos);
		}
		else {
			Item* newitem = item->tranform();

			pos = item->pos;
			int stackpos = tile->getThingStackPos(item);
			
			if (newitem == NULL /*decayTo == 0*//*) {
				sendRemoveThing(NULL,pos,item,stackpos,true);
				removeThing(NULL,pos,item);
				//mapstate.removeThing(tile, item);
				//t->removeThing(item);
			}
			else {
				//mapstate.replaceThing(tile, item, newitem);
				tile->removeThing(item);
				//autoclose.
				if(dynamic_cast<Container*>(item)){
					std::vector<Creature*> list;
					getSpectators(Range(pos, true), list);			
					for(unsigned int i = 0; i < list.size(); ++i) {
						Player *spectator = dynamic_cast<Player*>(list[i]);
						if(spectator)
							spectator->onThingRemove(item);
					}
				}
				
				tile->insertThing(newitem, stackpos);
				sendUpdateThing(NULL,pos,newitem,stackpos);
				
				//unsigned short decayTime = Item::items[newitem->getID()].decayTime;
				addEvent(makeTask(newitem->getDecayTime(), std::bind2nd(std::mem_fun(&Game::decayItem), newitem)));
				//mapstate.refreshThing(tile, item);
			}
			FreeThing(item);
			//delete item;
		}
		/*std::vector<Creature*> list;
		getSpectators(Range(pos, true), list);
			
		for(unsigned int i = 0; i < list.size(); ++i) {
			Player *spectator = dynamic_cast<Player*>(list[i]);
			if(!spectator)
				continue;				
			//mapstate.getMapChanges(spectator);
		}*/
		/*
		flushSendBuffers();
	}

	OTSYS_THREAD_UNLOCK(gameLock)
}
*/
/*
void Game::decaySplash(Item* item)
{
	OTSYS_THREAD_LOCK(gameLock)

	if (item) {
		Tile *t = getTile(item->pos.x, item->pos.y, item->pos.z);

		if ((t) && (t->decaySplashAfter <= OTSYS_TIME()))
		{
			unsigned short decayTo   = Item::items[item->getID()].decayTo;
			unsigned short decayTime = Item::items[item->getID()].decayTime;
			int stackpos = t->getThingStackPos(item);

			if (decayTo == 0)
			{				
				t->splash = NULL;
				sendRemoveThing(NULL,item->pos,item,stackpos);
				FreeThing(item);				
			}
			else
			{
				item->setID(decayTo);
				t->decaySplashAfter = OTSYS_TIME() + decayTime*1000;
				addEvent(makeTask(decayTime*1000, std::bind2nd(std::mem_fun(&Game::decaySplash), item)));
				sendUpdateThing(NULL,item->pos,item,stackpos);
			}
			
			//creatureBroadcastTileUpdated(item->pos);

			/*if (decayTo == 0)
				FreeThing(item);
				//delete item;*//*
		}
	}
  
	flushSendBuffers();
	
	OTSYS_THREAD_UNLOCK(gameLock)
}
*/
void Game::checkDecay(int t){
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);
	addEvent(makeTask(DECAY_INTERVAL, boost::bind(&Game::checkDecay,this,DECAY_INTERVAL)));
		
	list<decayBlock*>::iterator it;
	for(it = decayVector.begin();it != decayVector.end();){
		(*it)->decayTime -= t;
		if((*it)->decayTime <= 0){
			list<Item*>::iterator it2;
			for(it2 = (*it)->decayItems.begin(); it2 != (*it)->decayItems.end(); it2++){
				/*todo: Decaying item could be in a  container carried by a player,
				should all items have a pointer to their parent (like containers)?*/
				Item* item = *it2;
				item->isDecaying = false;
				if(item->canDecay()){
					if(item->pos.x != 0xFFFF){
						Item* newitem = item->decay();
						Tile *tile = getTile(item->pos.x, item->pos.y, item->pos.z);
						Position pos = item->pos;
						
						if(newitem){
							int stackpos = tile->getThingStackPos(item);
							if(newitem == item){
								sendUpdateThing(NULL,pos,newitem,stackpos);
							}
							else{
								tile->removeThing(item);
								//autoclose containers
								if(dynamic_cast<Container*>(item)){
									std::vector<Creature*> list;
									getSpectators(Range(pos, true), list);
									for(unsigned int j = 0; j < list.size(); ++j){
										Player *spectator = dynamic_cast<Player*>(list[j]);
										if(spectator)
											spectator->onThingRemove(item);
									}
								}
								tile->insertThing(newitem, stackpos);
								sendUpdateThing(NULL,pos,newitem,stackpos);
								FreeThing(item);
							}
							startDecay(newitem);
						}
						else{
							removeThing(NULL,pos,item);
							FreeThing(item);
						}//newitem
					}//pos != 0xFFFF
				}//item->canDecay()
				FreeThing(item);
			}//for it2
			delete *it;
			it = decayVector.erase(it);
		}//(*it)->decayTime <= 0
		else{
			it++;
		}
	}//for it
		
	flushSendBuffers();	
}

void Game::startDecay(Item* item){
	if(item->isDecaying)
		return;//dont add 2 times the same item
	//get decay time
	item->isDecaying = true;
	unsigned long dtime = item->getDecayTime();
	//round time
	if(dtime < DECAY_INTERVAL)
		dtime = DECAY_INTERVAL;
	dtime = (dtime/DECAY_INTERVAL)*DECAY_INTERVAL;
	item->useThing();
	//search if there are any block with this time
	list<decayBlock*>::iterator it;
	for(it = decayVector.begin();it != decayVector.end();it++){
		if((*it)->decayTime == dtime){			
			(*it)->decayItems.push_back(item);
			return;
		}
	}
	//we need a new decayBlock
	decayBlock* db = new decayBlock;
	db->decayTime = dtime;
	db->decayItems.clear();
	db->decayItems.push_back(item);
	decayVector.push_back(db);
}

void Game::checkSpawns(int t)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);
	
	SpawnManager::instance()->checkSpawns(t);
	this->addEvent(makeTask(t, std::bind2nd(std::mem_fun(&Game::checkSpawns), t)));
}

void Game::CreateDamageUpdate(Creature* creature, Creature* attackCreature, int damage)
{
	Player* player = dynamic_cast<Player*>(creature);
	if(!player)
		return;
	//player->sendStats();
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
		dmgmesg <<".";

		player->sendTextMessage(MSG_EVENT, dmgmesg.str().c_str());
		//msg.AddTextMessage(MSG_EVENT, dmgmesg.str().c_str());
	}
	if (player->isRemoved == true){
		player->sendTextMessage(MSG_ADVANCE, "You are dead.");	
	}
}

void Game::CreateManaDamageUpdate(Creature* creature, Creature* attackCreature, int damage)
{
	Player* player = dynamic_cast<Player*>(creature);
	if(!player)
		return;
	//player->sendStats();
	//msg.AddPlayerStats(player);
	if (damage > 0) {
		std::stringstream dmgmesg;
		dmgmesg << "You lose " << damage << " mana";
		if(attackCreature) {
			dmgmesg << " blocking an attack by " << attackCreature->getName();
		}
		dmgmesg <<".";

		player->sendTextMessage(MSG_EVENT, dmgmesg.str().c_str());
		//msg.AddTextMessage(MSG_EVENT, dmgmesg.str().c_str());
	}
}

bool Game::creatureSaySpell(Creature *creature, const std::string &text)
{
	//OTSYS_THREAD_LOCK(gameLock)
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);
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
		std::map<std::string, Spell*>* tmp = spells.getVocSpells(player->vocation);
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

	//OTSYS_THREAD_UNLOCK(gameLock)
	return ret;
}

void Game::playerAutoWalk(Player* player, std::list<Direction>& path)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);
	stopEvent(player->eventAutoWalk);

  // then we schedule the movement...
  // the interval seems to depend on the speed of the char?
	//player->eventAutoWalk = addEvent(makeTask<Direction>(0, MovePlayer(player->getID()), path, 400, StopMovePlayer(player->getID())));
	player->pathlist = path;
	int ticks = (int)player->getSleepTicks();
#ifdef __DEBUG__
		std::cout << "playerAutoWalk - " << ticks << std::endl;
#endif

	player->eventAutoWalk = addEvent(makeTask(ticks, std::bind2nd(std::mem_fun(&Game::checkPlayerWalk), player->getID())));
}

bool Game::playerUseItemEx(Player *player, const Position& posFrom,const unsigned char  stack_from,
		const Position &posTo,const unsigned char stack_to, const unsigned short itemid)
{
	//OTSYS_THREAD_LOCK(gameLock)
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);
	bool ret = false;

	Position thingpos = getThingMapPos(player, posFrom);
	Item *item = dynamic_cast<Item*>(getThing(posFrom, stack_from, player));

	if(item) {
		//Runes
		std::map<unsigned short, Spell*>::iterator sit = spells.getAllRuneSpells()->find(item->getID());
		if(sit != spells.getAllRuneSpells()->end()) {
			if( (abs(thingpos.x - player->pos.x) > 1) || (abs(thingpos.y - player->pos.y) > 1) ) {
				player->sendCancel("To far away...");
				ret = false;
			}
			else {
				std::string var = std::string("");
				if(player->access != 0 || sit->second->getMagLv() <= player->maglevel)
				{
					bool success = sit->second->getSpellScript()->castSpell(player, posTo, var);
					ret = success;
					if(success) {
						autoCloseTrade(item);
						item->setItemCharge(std::max((int)item->getItemCharge() - 1, 0) );
						if(item->getItemCharge() == 0) {
							removeThing(player,posFrom,item);
						}
					}
				}
				else
				{			
					player->sendCancel("You don't have the required magic level to use that rune.");
				}
			}
		}
		else{
			actions.UseItemEx(player,posFrom,stack_from,posTo,stack_to,itemid);
			ret = true;
		}
	}

	//OTSYS_THREAD_UNLOCK(gameLock)
	return ret;
}


bool Game::playerUseItem(Player *player, const Position& pos, const unsigned char stackpos, const unsigned short itemid, unsigned char index)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);
	actions.UseItem(player,pos,stackpos,itemid,index);
	return true;
}

void Game::playerRequestTrade(Player *player, const Position& pos,
	const unsigned char stackpos, const unsigned short itemid, unsigned long playerid)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);

	Player *tradePartner = dynamic_cast<Player*>(getCreatureByID(playerid));
	if(!tradePartner || tradePartner == player) {
		player->sendTextMessage(MSG_INFO, "Sorry, not possible.");
		return;
	}

	Item *tradeItem = dynamic_cast<Item*>(getThing(pos, stackpos, player));
	if(!tradeItem || tradeItem->getID() != itemid || !tradeItem->isPickupable()) {
		player->sendCancel("Sorry, not possible.");
		return;
	}

	
	if(!player->removeItem(tradeItem, true)) {
		/*if( (abs(player->pos.x - pos.x) > 1) || (abs(player->pos.y - pos.y) > 1) ) {
			player->sendCancel("To far away...");
			return;
		}*/
		player->sendCancel("Sorry, not possible.");
		return;
	}

	if(tradeItems.find(tradeItem) != tradeItems.end()){
		player->sendTextMessage(MSG_INFO, "This item is already beeing traded.");
		//player->sendCancel("Sorry, not possible.");
		return;
	}

	if(player->tradePartner != 0 && tradePartner->tradePartner != player->getID()){
		player->sendTextMessage(MSG_INFO, "This player is already trading.");
		return;
	}

	Container *tradeContainer = dynamic_cast<Container*>(tradeItem);
	if(tradeContainer && tradeContainer->getItemHoldingCount() + 1 > 100){
		player->sendTextMessage(MSG_INFO, "You can not trade more than 100 items.");
		return;
	}

	player->tradePartner = playerid;
	player->tradeItem = tradeItem;
	//tradeItems.insert(tradeItems.begin(), tradeItem);
	tradeItems[tradeItem] = player->getID();

	player->sendTradeItemRequest(player, tradeItem, true);

	if (tradePartner->tradePartner == 0){
		std::stringstream trademsg;
		trademsg << player->getName() <<" wants to trade with you.";
		tradePartner->sendTextMessage(MSG_INFO, trademsg.str().c_str());
	}
	else {
		Item* counterOfferItem = tradePartner->tradeItem;
		player->sendTradeItemRequest(tradePartner, counterOfferItem, false);
		tradePartner->sendTradeItemRequest(player, tradeItem, false);
	}
}

void Game::playerAcceptTrade(Player* player)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);
	
	player->setAcceptTrade(true);
	Player *tradePartner = dynamic_cast<Player*>(getCreatureByID(player->tradePartner));
	if(tradePartner && tradePartner->getAcceptTrade()) {
		Item *tradeItem1 = player->tradeItem;
		Item *tradeItem2 = tradePartner->tradeItem;

		std::map<Item*, unsigned long>::iterator it;

		it = tradeItems.find(tradeItem1);
		if(it != tradeItems.end()) {
			tradeItems.erase(it);
		}

		it = tradeItems.find(tradeItem2);
		if(it != tradeItems.end()) {
			tradeItems.erase(it);
		}
		
		player->setAcceptTrade(false);
		tradePartner->setAcceptTrade(false);
		player->sendCloseTrade();
		tradePartner->sendCloseTrade();
		
		if(player->addItem(tradeItem2, true) && tradePartner->addItem(tradeItem1, true) && 
			player->removeItem(tradeItem1, true) && tradePartner->removeItem(tradeItem2, true)){
			//this->removeThing(player, tradeItem1->pos, tradeItem1);
			//this->removeThing(tradePartner, tradeItem2->pos, tradeItem2);
			player->removeItem(tradeItem1);
			tradePartner->removeItem(tradeItem2);
			
			player->onThingRemove(tradeItem1);
			tradePartner->onThingRemove(tradeItem2);
			
			player->addItem(tradeItem2);
			tradePartner->addItem(tradeItem1);
		}
		else{
			player->sendTextMessage(MSG_SMALLINFO, "Sorry not possible.");
			tradePartner->sendTextMessage(MSG_SMALLINFO, "Sorry not possible.");
		}
	}
}

void Game::playerLookInTrade(Player* player, bool lookAtCounterOffer, int index)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);

	Player *tradePartner = dynamic_cast<Player*>(getCreatureByID(player->tradePartner));
	if(!tradePartner)
		return;

	Item *tradeItem = NULL;

	if(lookAtCounterOffer)
		tradeItem = tradePartner->getTradeItem();
	else
		tradeItem = player->getTradeItem();

	if(!tradeItem)
		return;

	if(index == 0) {
		player->sendTextMessage(MSG_INFO, tradeItem->getDescription(true).c_str());
		return;
	}

	Container *tradeContainer = dynamic_cast<Container*>(tradeItem);
	if(!tradeContainer || index > tradeContainer->getItemHoldingCount())
		return;

	bool foundItem = false;
	std::list<const Container*> stack;
	for (ContainerList::const_iterator it = tradeContainer->getItems(); it != tradeContainer->getEnd(); ++it) {
		Container *container = dynamic_cast<Container*>(*it);
		if(container) {
			stack.push_back(container);
		}

		--index;
		if(index == 0) {
			tradeItem = *it;
			foundItem = true;
			break;
		}
	}
	
	while(!foundItem && stack.size() > 0) {
		const Container *container = stack.front();
		stack.pop_front();

		for (ContainerList::const_iterator it = container->getItems(); it != container->getEnd(); ++it) {
			Container *container = dynamic_cast<Container*>(*it);
			if(container) {
				stack.push_back(container);
			}

			--index;
			if(index == 0) {
				tradeItem = *it;
				foundItem = true;
				break;
			}
		}
	}
	
	if(foundItem) {
		player->sendTextMessage(MSG_INFO, tradeItem->getDescription(true).c_str());
	}
}

void Game::playerCloseTrade(Player* player)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);
	
	Player *tradePartner = dynamic_cast<Player*>(getCreatureByID(player->tradePartner));

	std::vector<Item*>::iterator it;
	if(player->getTradeItem()) {
		std::map<Item*, unsigned long>::iterator it = tradeItems.find(player->getTradeItem());
		if(it != tradeItems.end()) {
			tradeItems.erase(it);
		}
	}

	player->setAcceptTrade(false);
	player->sendTextMessage(MSG_SMALLINFO, "Trade cancelled.");
	player->sendCloseTrade();

	if(tradePartner) {
		if(tradePartner->getTradeItem()) {
			std::map<Item*, unsigned long>::iterator it = tradeItems.find(tradePartner->getTradeItem());
			if(it != tradeItems.end()) {
				tradeItems.erase(it);
			}
		}

		tradePartner->setAcceptTrade(false);
		tradePartner->sendTextMessage(MSG_SMALLINFO, "Trade cancelled.");
		tradePartner->sendCloseTrade();
	}
}

void Game::autoCloseTrade(const Item* item){
	if(!item)
		return;

	std::map<Item*, unsigned long>::const_iterator it;
	const Container* container = NULL;
	for(it = tradeItems.begin(); it != tradeItems.end(); it++) {
		if(item == it->first || 
			((container = dynamic_cast<const Container*>(item)) && container->getSlotNumberByItem(it->first) == 0xFF && container->isHoldingItem(it->first)) ||
			((container = dynamic_cast<const Container*>(it->first)) && container->isHoldingItem(item)))
		{
			Player* player = dynamic_cast<Player*>(getCreatureByID(it->second));
			if(player) {
					playerCloseTrade(player);
			}

			break;
		}
	}
}

void Game::playerSetAttackedCreature(Player* player, unsigned long creatureid)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);
		
	if(player->attackedCreature != 0 && creatureid == 0) {
		player->sendCancelAttacking();
	}

	Creature* attackedCreature = NULL;
	if(creatureid != 0) {
		attackedCreature = getCreatureByID(creatureid);
	}

	if(attackedCreature) {
		if(attackedCreature->access != 0 || (getWorldType() == WORLD_TYPE_NO_PVP && player->access == 0 && dynamic_cast<Player*>(attackedCreature))) {
			player->sendTextMessage(MSG_SMALLINFO, "You may not attack this player.");
			player->sendCancelAttacking();
			player->setAttackedCreature(0);
			stopEvent(player->eventCheckAttacking);
			player->eventCheckAttacking = 0;
			return;
		}
	}

	player->setAttackedCreature(creatureid);
	stopEvent(player->eventCheckAttacking);

	if(creatureid != 0) {
		player->eventCheckAttacking = addEvent(makeTask(2000, std::bind2nd(std::mem_fun(&Game::checkCreatureAttacking), player->getID())));
	}
	else
		player->eventCheckAttacking = 0;
}

void Game::flushSendBuffers(){
	//OTSYS_THREAD_LOCK(gameLock)
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);

	for(std::vector<Player*>::iterator it = BufferedPlayers.begin(); it != BufferedPlayers.end(); ++it) {
		(*it)->flushMsg();
		(*it)->SendBuffer = false;
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
	
	//OTSYS_THREAD_UNLOCK(gameLock)
	return;
}

void Game::addPlayerBuffer(Player* p){
	//OTSYS_THREAD_LOCK(gameLock)	
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);
/*
#ifdef __DEBUG__
	std::cout << "addPlayerBuffer() - useThing()" << std::endl;
#endif
*/
	if(p->SendBuffer == false){
		p->useThing();
		BufferedPlayers.push_back(p);
		p->SendBuffer = true;
	}
	//OTSYS_THREAD_UNLOCK(gameLock)
	return;
}

void Game::FreeThing(Thing* thing){
	//OTSYS_THREAD_LOCK(gameLock)
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock);
	//std::cout << "freeThing() " << thing <<std::endl;
	ToReleaseThings.push_back(thing);
	//OTSYS_THREAD_UNLOCK(gameLock)
	return;
}
/*
ADD
container(player,pos-cid,thing)
inventory(player,pos-i,[ignored])
ground([ignored],postion,thing)

REMOVE
container(player,pos-cid,thing,autoclose?)
inventory(player,pos-i,thing,autoclose?)
ground([ignored],postion,thing,autoclose?,stackpos)

UPDATE
container(player,pos-cid,thing)
inventory(player,pos-i,[ignored])
ground([ignored],postion,thing,stackpos)
*/
void Game::sendAddThing(Player* player,const Position &pos,const Thing* thing){
	if(pos.x == 0xFFFF) {		
		if(!player)
			return;
		if(pos.y & 0x40) { //container
			if(!thing)
				return;
								
			const Item *item = dynamic_cast<const Item*>(thing);
			if(!item)
				return;
								
			unsigned char containerid = pos.y & 0x0F;
			Container* container = player->getContainer(containerid);
			if(!container)
				return;
			
			std::vector<Creature*> list;
			Position centerpos = (container->pos.x == 0xFFFF ? player->pos : container->pos);
			map->getSpectators(Range(centerpos,2,2,2,2,false), list);
			
			if(!list.empty()) {
				for(int i = 0; i < list.size(); ++i) {
					Player *spectator = dynamic_cast<Player*>(list[i]);
					if(spectator)
						spectator->onItemAddContainer(container,item);
				}
			}
			else
				player->onItemAddContainer(container,item);

		}
		else //inventory
		{
			player->sendInventory(pos.y);
		}
	}
	else //ground
	{
		if(!thing)
			return;
		
		std::vector<Creature*> list;
		map->getSpectators(Range(pos,true), list);		
		for(unsigned int i = 0; i < list.size(); ++i) {
			list[i]->onThingAppear(thing);
		}			
	}	
}

void Game::sendRemoveThing(Player* player,const Position &pos,const Thing* thing,const unsigned char stackpos /*=1*/ ,const bool autoclose/* =false*/){
	if(!thing)
		return;
	
	const Item *item = dynamic_cast<const Item*>(thing);
	bool perform_autoclose = false;
	if(autoclose && item){
		const Container *container = dynamic_cast<const Container*>(item);
		if(container)
			perform_autoclose = true;		
	}
	
	if(pos.x == 0xFFFF) {
		if(!player)
			return;		
		if(pos.y & 0x40) { //container									
			if(!item)
				return;
									
			unsigned char containerid = pos.y & 0x0F;
			Container* container = player->getContainer(containerid);
			if(!container)
				return;
			//check that item is in the container
			unsigned char slot = container->getSlotNumberByItem(item);
			
			std::vector<Creature*> list;
			Position centerpos = (container->pos.x == 0xFFFF ? player->pos : container->pos);
			map->getSpectators(Range(centerpos,2,2,2,2,false), list);
			
			if(!list.empty()) {
				for(int i = 0; i < list.size(); ++i) {					
					Player *spectator = dynamic_cast<Player*>(list[i]);
					if(spectator){
						spectator->onItemRemoveContainer(container,slot);
						if(perform_autoclose){
							spectator->onThingRemove(thing);
						}
					}
				}
			}
			else{
				player->onItemRemoveContainer(container,slot);
				if(perform_autoclose){
					player->onThingRemove(thing);
				}
			}

		}
		else //inventory
		{
			player->removeItemInventory(pos.y);	
			if(perform_autoclose){
				player->onThingRemove(thing);
			}
		}
	}
	else //ground
	{		
		std::vector<Creature*> list;
		map->getSpectators(Range(pos,true), list);
		for(unsigned int i = 0; i < list.size(); ++i) {			
			list[i]->onThingDisappear(thing,stackpos);
			Player *spectator = dynamic_cast<Player*>(list[i]);
			if(perform_autoclose && spectator){
				spectator->onThingRemove(thing);
			}
		}			
	}
}

void Game::sendUpdateThing(Player* player,const Position &pos,const Thing* thing,const unsigned char stackpos/*=1*/){
	
	if(pos.x == 0xFFFF) {		
		if(!player)
			return;
		if(pos.y & 0x40) { //container									
			if(!thing)
				return;
				
			const Item *item = dynamic_cast<const Item*>(thing);
			if(!item)
				return;
									
			unsigned char containerid = pos.y & 0x0F;
			Container* container = player->getContainer(containerid);
			if(!container)
				return;
			//check that item is in the container
			unsigned char slot = container->getSlotNumberByItem(item);
			
			std::vector<Creature*> list;
			Position centerpos = (container->pos.x == 0xFFFF ? player->pos : container->pos);
			map->getSpectators(Range(centerpos,2,2,2,2,false), list);
			
			if(!list.empty()) {
				for(int i = 0; i < list.size(); ++i) {					
					Player *spectator = dynamic_cast<Player*>(list[i]);
					if(spectator)
						spectator->onItemUpdateContainer(container,item,slot);
				}
			}
			else{
				//never should be here
				std::cout << "Error: sendUpdateThing" << std::endl;
				//player->onItemUpdateContainer(container,item,slot);
			}

		}
		else //inventory
		{
			player->sendInventory(pos.y);
		}
	}
	else //ground
	{		
		if(!thing)
			return;
		std::vector<Creature*> list;
		map->getSpectators(Range(pos,true), list);
		for(unsigned int i = 0; i < list.size(); ++i){
			list[i]->onThingTransform(thing,stackpos);
		}			
	}
}

void Game::addThing(Player* player,const Position &pos,Thing* thing)
{
	if(!thing)
		return;
	Item *item = dynamic_cast<Item*>(thing);
	
	if(pos.x == 0xFFFF) {
		if(!player || !item)
			return;
		
		if(pos.y & 0x40) { //container								
			unsigned char containerid = pos.y & 0x0F;
			Container* container = player->getContainer(containerid);
			if(!container)
				return;
			container->addItem(item);
			sendAddThing(player,pos,thing);
		}
		else //inventory
		{
			player->addItemInventory(item,pos.y,true);
			sendAddThing(player,pos,thing);
		}
	}
	else //ground
	{
		if(!thing)
			return;
		Tile *tile = map->getTile(pos.x, pos.y, pos.z);
		if(tile){
			thing->pos = pos;
			if(item && item->isSplash()){
				if(tile->splash){
					int oldstackpos = tile->getThingStackPos(tile->splash);
					Item *oldsplash = tile->splash;

					oldsplash->isRemoved = true;
					FreeThing(oldsplash);

					tile->splash = item;

					sendUpdateThing(NULL, pos, item, oldstackpos);

					/*
					unsigned char stackpos = tile->getThingStackPos(tile->splash);
					Item *oldsplash = tile->splash;
					tile->splash = NULL;
					sendRemoveThing(NULL,pos,oldsplash,stackpos);
					tile->splash = item;
					sendAddThing(NULL,pos,tile->splash);
					oldsplash->isRemoved = true;
					FreeThing(oldsplash);
					*/
				}
				else{
					tile->splash = item;
					sendAddThing(NULL,pos,tile->splash);
				}
			}
			else if(item && item->isGroundTile()){
				tile->ground = item;
				Game::creatureBroadcastTileUpdated(thing->pos);
			}
			else if(item && item->isStackable()){
				Item *topitem = tile->getTopDownItem();
				if(topitem && topitem->getID() == item->getID() && 
				  topitem->getItemCountOrSubtype() + item->getItemCountOrSubtype() <= 100){
					topitem->setItemCountOrSubtype(topitem->getItemCountOrSubtype() + item->getItemCountOrSubtype());
					int stackpos = tile->getThingStackPos(topitem);
					sendUpdateThing(NULL,topitem->pos,topitem,stackpos);
					item->pos.x = 0xFFFF;
					FreeThing(item);
				}
				else{
					tile->addThing(thing);
					sendAddThing(player,pos,thing);
				}
			}
			else{
				tile->addThing(thing);
				sendAddThing(player,pos,thing);
			}
		}
	}		
}

void Game::removeThing(Player* player,const Position &pos,Thing* thing,  bool setRemoved /*= true*/)
{
	if(!thing)
		return;
	Item *item = dynamic_cast<Item*>(thing);
	
	if(pos.x == 0xFFFF) {
		if(!player || !item)
			return;
			
		if(pos.y & 0x40) { //container
			unsigned char containerid = pos.y & 0x0F;
			Container* container = player->getContainer(containerid);
			if(!container)
				return;

			sendRemoveThing(player,pos,thing,0,true);
			container->removeItem(item);
			if(player && player->isHoldingContainer(container)) {
				player->updateInventoryWeigth();
				player->sendStats();
			}
		}
		else //inventory
		{
			sendRemoveThing(player,pos,thing,0,true);
			//player->removeItemInventory(pos.y,true);	
		}
		if(setRemoved)
			item->isRemoved = true;
	}
	else //ground
	{		
		Tile *tile = map->getTile(pos.x, pos.y, pos.z);
		if(tile){
			unsigned char stackpos = tile->getThingStackPos(thing);
			tile->removeThing(thing);
			sendRemoveThing(NULL,pos,thing,stackpos,true);
		}
		if(item && setRemoved){
			item->isRemoved = true;
		}
	}
}

Position Game::getThingMapPos(Player *player, const Position &pos)
{
	if(pos.x == 0xFFFF){
		Position dummyPos(0,0,0);
		if(!player)
			return dummyPos;
		if(pos.y & 0x40) { //from container						
			unsigned char containerid = pos.y & 0x0F;
			const Container* container = player->getContainer(containerid);
			if(!container){
				return dummyPos;
			}			
			while(container->getParent() != NULL) {
				container = container->getParent();				
			}			
			if(container->pos.x == 0xFFFF)				
				return player->pos;			
			else
				return container->pos;
		}
		else //from inventory
		{
			return player->pos;
		}
	}
	else{
		return pos;
	}
}

Thing* Game::getThing(const Position &pos,unsigned char stack, Player* player /*=NULL*/)
{	
	if(pos.x == 0xFFFF) {
		if(!player)
			return NULL;
		if(pos.y & 0x40) { //from container
			unsigned char containerid = pos.y & 0x0F;
			Container* container = player->getContainer(containerid);
			if(!container)
				return NULL;
							
			return container->getItem(pos.z);
		}
		else //from inventory
		{
			return player->getItem(pos.y);
		}
	}
	else //from ground
	{
		Tile *t = getTile(pos.x, pos.y, pos.z);
		if(!t)
			return NULL;
		
		return t->getThingByStackPos(stack);
	}	
}

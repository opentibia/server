//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// 
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
#include <iostream>
#include <algorithm>

using namespace std;

#include <stdlib.h>

#include "protocol.h"
#include "player.h"


Player::Player(const char *name, Protocol *p) : Creature(name)
{
  client     = p;

	exhaustedTicks  = 0;
	pzLocked = false;
	inFightTicks = 0;
	looktype   = PLAYER_MALE_1;
	voc        = 0;

  cap        = 300;

  mana       = 0;
  manamax    = 0;
  manaspent  = 0;

  food       = 0;

  level      = 1;
  experience = 180;

  maglevel   = 20;

  access     = 0;
  cancelMove = false;
  fightMode = followMode = 0;
  for(int i = 0; i < 7; i++)
  {
    skills[i][SKILL_LEVEL] = 1;
    skills[i][SKILL_TRIES] = 0;
  }

  //set item pointers to NULL
	for(int i = 0; i < 11; i++)
		items[i] = NULL;

  useCount = 0;
} 


Player::~Player()
{
	for (int i = 0; i < 11; i++)
		if (items[i])
      delete items[i];

  delete client;
}


Item* Player::getItem(int pos)
{
	if(pos>0 && pos <11)
		return items[pos];
	return NULL;
}

int Player::getWeaponDamage() const
{
	double damagemax = 0;
  for (int slot = SLOT_RIGHT; slot <= SLOT_LEFT; slot++)
	  if (items[slot])
    {
			if ((items[slot]->isWeapon()))
      {
				// check which kind of skill we use...
				// and calculate the damage dealt
				switch (items[slot]->getWeaponType())
        {
					case SWORD:
						damagemax = 3*skills[SKILL_SWORD][SKILL_LEVEL] + 2*Item::items[items[slot]->getID()].attack;
						break;
					case CLUB:
						damagemax = 3*skills[SKILL_CLUB][SKILL_LEVEL] + 2*Item::items[items[slot]->getID()].attack;
						break;
					case AXE:
						damagemax = 3*skills[SKILL_AXE][SKILL_LEVEL] + 2*Item::items[items[slot]->getID()].attack;
						break;
					case DIST:
						damagemax = 4*skills[SKILL_DIST][SKILL_LEVEL];
						break;
					case MAGIC:
						damagemax = (level*2+maglevel*3) * 1.25;
						break;
			  }
		  }
    }

	// no weapon found -> fist fighting
	if (damagemax == 0)
		damagemax = 2*skills[SKILL_FIST][SKILL_LEVEL] + 5;

	// return it
	return 1+(int)(damagemax*rand()/(RAND_MAX+1.0));
}

void Player::speak(const std::string &text)
{
}

void Player::sendIcons()
{
     int icons = 0;
     if(inFightTicks >= 6000 || inFightTicks ==4000 || inFightTicks == 2000){
                     icons |= ICON_SWORDS;
                     }
     if(manaShieldTicks >= 1000){
                     icons |= ICON_MANASHIELD;
                     }
     if(speed != getNormalSpeed()){
                     icons |= ICON_HASTE;
                     }                   
     client->sendIcons(icons);             
}

int Player::sendInventory(){
	client->sendInventory();
	return true;
}


int Player::addItem(Item* item, int pos){
#ifdef __DEBUG__
	std::cout << "Should add item at " << pos <<std::endl;
#endif

	if(pos>0 && pos <11)
  {
    if (items[pos])
      delete items[pos];
		items[pos]=item;
  }
	client->sendInventory();
	return true;
}

void Player::addSkillTry(int skilltry)
{
int skill;
std::string skillname;
for (int slot = SLOT_RIGHT; slot <= SLOT_LEFT; slot++)
    if (items[slot])
    {
     if (items[slot]->isWeapon())
     {
      switch (items[slot]->getWeaponType())
            {
            case SWORD: skill = 2; skillname = "sword fighting"; break;
            case CLUB: skill = 1; skillname = "club fighting"; break;
            case AXE: skill = 3; skillname = "axe fighting"; break;
            case DIST: skill = 4; skillname = "distance fighting"; break;
            default: skill = 0; skillname = "fist fighting"; break;
            }
      }
    }

skills[skill][SKILL_TRIES] += skilltry;

int oldSkill = skills[skill][SKILL_LEVEL];
int tries = skills[skill][SKILL_TRIES];

int checkSkill = oldSkill+1;
int reqTries = 50;
for(int i = 10;i<=checkSkill;i++){
        if(i == 11)
        reqTries = 50;
        else
        reqTries = (int)(reqTries*1.1);
}
//Need skill up?
printf("%d -- %d\n", tries, reqTries);
if (tries >= reqTries)
{
   skills[skill][SKILL_LEVEL] = checkSkill;
   skills[skill][SKILL_TRIES] = 0;

   NetworkMessage msg;
   std::stringstream advMsg;
   advMsg << "You advanced in " << skillname << ".";
   msg.AddTextMessage(MSG_ADVANCE, advMsg.str().c_str());
   msg.AddPlayerSkills(this);
   sendNetworkMessage(&msg);
}


}

Item* Player::getContainer(unsigned char containerid)
{
  for(containerLayout::iterator cl = vcontainers.begin(); cl != vcontainers.end(); ++cl)
  {
	  if(cl->first == containerid)
			return cl->second;
	}

	return NULL;
}

unsigned char Player::getContainerID(Item* container)
{
  for(containerLayout::iterator cl = vcontainers.begin(); cl != vcontainers.end(); ++cl)
  {
	  if(cl->second == container)
			return cl->first;
	}

	return 0xFF;
}

void Player::addContainer(unsigned char containerid, Item *container)
{
#ifdef __DEBUG__
	cout << Creature::getName() << ", addContainer: " << (int)containerid << std::endl;
#endif
	if(containerid > 0xF)
		return;

	for(containerLayout::iterator cl = vcontainers.begin(); cl != vcontainers.end(); ++cl) {
		if(cl->first == containerid) {
			cl->second = container;
			return;
		}
	}
	
	//id doesnt exist, create it
	containerItem vItem;
	vItem.first = containerid;
	vItem.second = container;

	vcontainers.push_back(vItem);
}

void Player::closeContainer(unsigned char containerid)
{
  for(containerLayout::iterator cl = vcontainers.begin(); cl != vcontainers.end(); ++cl)
  {
	  if(cl->first == containerid)
	  {
		  vcontainers.erase(cl);
			break;
		}
	}

#ifdef __DEBUG__
	cout << Creature::getName() << ", closeContainer: " << (int)containerid << std::endl;
#endif
}

fight_t Player::getFightType()
{
  for (int slot = SLOT_RIGHT; slot <= SLOT_LEFT; slot++)
  {
    if (items[slot])
    {
			if ((items[slot]->isWeapon())) {
				switch (items[slot]->getWeaponType())
        {
					case DIST:
						return FIGHT_DIST;
					case MAGIC:
						return FIGHT_MAGICDIST;
				}
			}
    }
  }
  return FIGHT_MELEE;
}


bool Player::CanSee(int x, int y)
{
  return client->CanSee(x, y);
}


void Player::sendNetworkMessage(NetworkMessage *msg)
{
  client->sendNetworkMessage(msg);
}

void Player::sendCancel(const char *msg)
{
  client->sendCancel(msg);
}
void Player::sendChangeSpeed(Creature* creature){
     client->sendChangeSpeed(creature);
     }

void Player::sendToChannel(Creature *creature, unsigned char type, const std::string &text, unsigned short channelId){
     client->sendToChannel(creature, type, text, channelId);
     }

void Player::sendCancelAttacking()
{
  attackedCreature = 0;   
  client->sendCancelAttacking();
}

void Player::sendCancelWalk(const char *msg)
{
  client->sendCancelWalk(msg);
}

void Player::onThingMove(const Creature *creature, const Thing *thing, const Position *oldPos, unsigned char oldstackpos)
{
  client->sendThingMove(creature, thing, oldPos, oldstackpos);
}

void Player::setAttackedCreature(unsigned long id){
     attackedCreature = id;
        }

void Player::onCreatureAppear(const Creature *creature)
{
  client->sendCreatureAppear(creature);
}


void Player::onCreatureDisappear(const Creature *creature, unsigned char stackPos)
{
  client->sendCreatureDisappear(creature, stackPos);
}


void Player::onCreatureTurn(const Creature *creature, unsigned char stackPos)
{
  client->sendCreatureTurn(creature, stackPos);
}


void Player::onCreatureSay(const Creature *creature, unsigned char type, const std::string &text)
{
  client->sendCreatureSay(creature, type, text);
}

void Player::onCreatureChangeOutfit(const Creature* creature) {
		  client->sendSetOutfit(creature);
}

void Player::onThink(){}

void Player::onTileUpdated(const Position *Pos)
{
  client->sendTileUpdated(Pos);
}

void Player::onContainerUpdated(Item *item, unsigned char from_id, unsigned char to_id,
																unsigned char from_slot, unsigned char to_slot, bool remove)
{
	client->sendContainerUpdated(item, from_id, to_id, from_slot, to_slot, remove);
}



//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// base class for every creature
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


#ifndef __creature_h
#define __creature_h


#include "thing.h"
#include "position.h"


enum playerLooks
{
	PLAYER_MALE_1=0x80,
	PLAYER_MALE_2=0x81,
	PLAYER_MALE_3=0x82,
	PLAYER_MALE_4=0x83,
	PLAYER_FEMALE_1=0x88,
	PLAYER_FEMALE_2=0x89,
	PLAYER_FEMALE_3=0x8A,
	PLAYER_FEMALE_4=0x8B,
};


class Map;

class Item;

class Thing;
class Player;


//////////////////////////////////////////////////////////////////////
// Defines the Base class for all creatures and base functions which 
// every creature has


class Creature : public Thing
{
public:
  Creature(const char *name);
  virtual ~Creature() {};

  virtual bool isCreature() const { return true; };

  virtual const std::string& getName() const {return name; };

  unsigned long getID() const { return id; }

  Direction getDirection() const { return direction; }
  void setDirection(Direction dir) { direction = dir; }


  virtual void drainHealth(int);




		virtual int sendInventory(){return 0;};
		virtual int addItem(Item* item, int pos){return 0;};
		virtual Item* getItem(int pos){return NULL;}




  int lookhead, lookbody, looklegs, lookfeet, looktype;

  int speed;

  int health, healthmax;

  int lastDamage;


private:
  virtual void onThingMove(const Player *player, const Thing *thing, const Position *oldPos, unsigned char oldstackpos) { };
  virtual void onCreatureAppear(const Creature *creature) { };
  virtual void onCreatureDisappear(const Creature *creature, unsigned char stackPos) { };
  virtual void onCreatureTurn(const Creature *creature, unsigned char stackPos) { };
  virtual void onCreatureSay(const Creature *creature, unsigned char type, const std::string &text) { };

  virtual void onCreatureChangeOutfit(const Creature* creature) { };

  friend class Map;

  Direction direction;

  unsigned long id;
  std::string        name;
};


#endif // __creature_h

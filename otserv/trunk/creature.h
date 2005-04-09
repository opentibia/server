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
#include "networkmessage.h"
#include "container.h"
#include "magic.h"
#include <vector>

enum subfight_t {
	DIST_NONE = 0,
	DIST_BOLT = NM_ANI_BOLT,
  DIST_ARROW = NM_ANI_ARROW, 
  DIST_FIRE = NM_ANI_FIRE,
  DIST_ENERGY = NM_ANI_ENERGY,
  DIST_POISONARROW = NM_ANI_POISONARROW,
  DIST_BURSTARROW = NM_ANI_BURSTARROW,
  DIST_THROWINGSTAR = NM_ANI_THROWINGSTAR,
  DIST_THROWINGKNIFE = NM_ANI_THROWINGKNIFE,
  DIST_SMALLSTONE = NM_ANI_SMALLSTONE,
  DIST_SUDDENDEATH = NM_ANI_SUDDENDEATH,
  DIST_LARGEROCK = NM_ANI_LARGEROCK,
  DIST_SNOWBALL = NM_ANI_SNOWBALL,
  DIST_POWERBOLT = NM_ANI_POWERBOLT,
  DIST_SPEAR = NM_ANI_SPEAR
};

enum slots_t {
	SLOT_WHEREEVER=0,
	SLOT_HEAD=1,
	SLOT_NECKLACE=2,
	SLOT_BACKPACK=3,
	SLOT_ARMOR=4,
	SLOT_RIGHT=5,
	SLOT_LEFT=6,
	SLOT_LEGS=7,
	SLOT_FEET=8,
	SLOT_RING=9,
	SLOT_AMMO=10,
	SLOT_DEPOT=11
};

enum fight_t {
	FIGHT_MELEE,
	FIGHT_DIST,
	FIGHT_MAGICDIST
};

// Macros
#define CREATURE_SET_OUTFIT(c, type, head, body, legs, feet) c->looktype = type; \
c->lookhead = head; \
c->lookbody = body; \
c->looklegs = legs; \
c->lookfeet = feet;

enum playerLooks
{
	PLAYER_MALE_1=0x80,
	PLAYER_MALE_2=0x81,
	PLAYER_MALE_3=0x82,
	PLAYER_MALE_4=0x83,
	PLAYER_MALE_5=0x84,
	PLAYER_MALE_6=0x85,
	PLAYER_MALE_7=0x86,
	PLAYER_FEMALE_1=0x88,
	PLAYER_FEMALE_2=0x89,
	PLAYER_FEMALE_3=0x8A,
	PLAYER_FEMALE_4=0x8B,
	PLAYER_FEMALE_5=0x8C,
	PLAYER_FEMALE_6=0x8D,
	PLAYER_FEMALE_7=0x8E,
};


class Map;

class Item;

class Thing;
class Player;
class Monster;

class Conditions : public std::map<attacktype_t, ConditionVec>
{
public:
	bool hasCondition(attacktype_t type)
	{
		Conditions::iterator condIt = this->find(type);
		if(condIt != this->end() && !condIt->second.empty()) {
			return true;
		}

		return false;
	}
};

//////////////////////////////////////////////////////////////////////
// Defines the Base class for all creatures and base functions which 
// every creature has


class Creature : public Thing
{
public:
  Creature(const char *name);
  virtual ~Creature() {};

  virtual const std::string& getName() const {return name; };

  void setID(int id){this->id=id;}
  unsigned long getID() const { return id; }
  unsigned long getExpForLv(const int& lv) const { 
		return (int)((50*lv*lv*lv)/3 - 100 * lv * lv + (850*lv) / 3 - 200);
	}
  Direction getDirection() const { return direction; }
  void setDirection(Direction dir) { direction = dir; }

  virtual fight_t getFightType(){return FIGHT_MELEE;};
	virtual subfight_t getSubFightType() {return DIST_NONE;}
	virtual int getImmunities() const
	{
		if(access != 0) 
			return  ATTACK_ENERGY | ATTACK_BURST | ATTACK_FIRE |
			ATTACK_PHYSICAL | ATTACK_POISON | ATTACK_PARALYZE | ATTACK_DRUNKNESS;
		else
			return immunities;
	};
  virtual void drainHealth(int);
  virtual void drainMana(int);
  virtual void die(){};
  virtual std::string getDescription(bool self = false) const;
  virtual void setAttackedCreature(unsigned long id);

  virtual int getWeaponDamage() const {
	return 1+(int)(10.0*rand()/(RAND_MAX+1.0));
  }

  unsigned long attackedCreature;

  virtual bool isAttackable() const { return true; };
	virtual bool isPushable() const {return true;}
	virtual void dropLoot(Container *corpse) {return;};

  virtual int sendInventory(){return 0;};
  virtual int addItem(Item* item, int pos){return 0;};
  virtual Item* getItem(int pos){return NULL;}
  virtual Direction getDirection(){return direction;}
	void addCondition(const CreatureCondition& condition, bool refresh);
	Conditions& getConditions() {return conditions;};

  int lookhead, lookbody, looklegs, lookfeet, looktype, lookcorpse, lookmaster;
  int mana, manamax, manaspent;
  bool pzLocked;
  
  long inFightTicks, exhaustedTicks;
	long manaShieldTicks, hasteTicks, paralyzeTicks;
	int immunities;

  unsigned long experience;
  Position masterPos;

  int health, healthmax;

  uint64_t lastmove;

  //int lastDamage;

  unsigned short getSpeed() const {            
           return speed; 
           };

  virtual int getStepDuration(int underground) { return (1000*120*100)/(getSpeed()*underground); };
  void setNormalSpeed()
  {
		if(access!=0){
			speed = 900;     
			return;    
		}
		
		speed = 220 + (2* (level - 1)); 
  }
  
	int getNormalSpeed()
  {
		if(access!=0){     
			return 900;    
		}
		return 220 + (2* (level - 1)); 
  }

  int access;		//access level
  int maglevel;	// magic level
  int level;		// level
  int speed;

  virtual bool canMovedTo(const Tile *tile) const;

  virtual void sendCancel(const char *msg) { };
  virtual void sendCancelWalk(const char *msg) { };

	virtual void addInflictedDamage(Creature* attacker, int damage);
	virtual int getGainedExperience(Creature* attacker);
	virtual std::vector<long> getInflicatedDamageCreatureList();
	virtual int getLostExperience();
	virtual int getInflicatedDamage(Creature* attacker);
	virtual int getTotalInflictedDamage();
	virtual int getInflicatedDamage(unsigned long id);

protected:
	Conditions conditions;
	typedef std::vector< std::pair<uint64_t, long> > DamageList;
	typedef std::map<long, DamageList > TotalDamageList;
	TotalDamageList totaldamagelist;



protected:
	virtual int onThink(int& newThinkTicks){newThinkTicks = 300; return 300;};
  virtual void onThingMove(const Creature *player, const Thing *thing, const Position *oldPos,
		unsigned char oldstackpos, unsigned char oldcount, unsigned char count) { };

  virtual void onCreatureAppear(const Creature *creature) { };
  virtual void onCreatureDisappear(const Creature *creature, unsigned char stackPos) { };
  virtual void onCreatureTurn(const Creature *creature, unsigned char stackPos) { };
  virtual void onCreatureSay(const Creature *creature, unsigned char type, const std::string &text) { };

  virtual void onCreatureChangeOutfit(const Creature* creature) { };
	virtual void onTileUpdated(const Position &pos) { };

	//virtual void onContainerUpdated(Item *item, unsigned char from_id, unsigned char to_id, unsigned char from_slot, unsigned char to_slot, bool remove) {};
  virtual void onTeleport(const Creature *creature, const Position *oldPos, unsigned char oldstackpos) { };

	//container to container
	virtual void onThingMove(const Creature *creature, const Container *fromContainer, Container *toContainer,
		const Item* item, unsigned char from_slotid, unsigned char to_slotid, unsigned char oldcount, unsigned char count) {};

	//container to ground
	virtual void onThingMove(const Creature *creature, const Container *fromContainer, const Position *newPos,
		const Item* item, unsigned char from_slotid, unsigned char oldcount, unsigned char count) {};

	//inventory to ground
	virtual void onThingMove(const Creature *creature, slots_t fromSlot, const Position *newPos,
		const Item* item, unsigned char oldcount, unsigned char count) {};
	
	//ground to container
	virtual void onThingMove(const Creature *creature, const Position *oldPos, const Container *toContainer,
		const Item* item, unsigned char stackpos, unsigned char to_slotid, unsigned char oldcount, unsigned char count) {};
	
	//ground to inventory
	virtual void onThingMove(const Creature *creature, const Position *oldPos, slots_t toSlot,
		const Item* item, unsigned char stackpos, unsigned char oldcount, unsigned char count) {};

  friend class Game;
  friend class Map;
	friend class MapState;
	friend class GameState;

  Direction direction;

  unsigned long id;
  std::string        name;
};


#endif // __creature_h

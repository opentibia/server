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


#ifndef __monster_h_
#define __monster_h_


#include "creature.h"
#include "game.h"
#include "tile.h"

class TimeProbabilityClass {
public:
	TimeProbabilityClass()
	{
		setDefault();
	}

	TimeProbabilityClass(int _cycleTicks, int _probability, int _exhaustionticks)
	{
		setDefault();
		init(_cycleTicks, _probability, _exhaustionticks);
	};

	~TimeProbabilityClass() {};

	bool onTick(int ticks)
	{
		ticksleft -= ticks;
		
		if(ticksleft <= 0) {
			ticksleft = cycleTicks;
			bool ret = (random_range(probability, 100) >= probability);
			return ret;
		}

		return false;
	}
	
	void init(int _cycleTicks, int _probability, int _exhaustionticks)
	{
		if(_cycleTicks >= 0) {
			this->ticksleft = _cycleTicks;
			this->cycleTicks = _cycleTicks;
		}

		_probability = std::max(_probability, 0);

		if(_probability >= 0)
			probability = std::min(100, probability);

		if(_exhaustionticks >= 0)
			exhaustionTicks = _exhaustionticks;
	}

	int getExhaustion() const {return exhaustionTicks;}

private:
	void setDefault()
	{
		cycleTicks = 5000;
		ticksleft = cycleTicks;
		probability = 50;
		exhaustionTicks = 0;
	}

	int ticksleft;
	int cycleTicks;
	int probability;
	int exhaustionTicks;
};

class PhysicalAttackClass {
public:
	PhysicalAttackClass()
	{
		//fighttype = FIGHT_NONE;
		disttype = DIST_NONE;
		minWeapondamage = 0;
		maxWeapondamage = 1;
	};

	~PhysicalAttackClass() {};

	fight_t fighttype;
	subfight_t disttype;

	int minWeapondamage;
	int maxWeapondamage;
};

class Monster : public Creature
{
public:
  Monster(const char *name, Game* game);
  virtual ~Monster();

	virtual void onAttack();
	bool isLoaded() const {return loaded;}

private:
	int getCurrentDistanceToTarget();
	void calcMovePosition();
	bool isInRange(const Position &pos);
	std::list<Position> route;

protected:
	Game* game;
	PhysicalAttackClass	*curPhysicalAttack;

	int targetDistance;
	int runawayHealth;

	bool doAttacks(Player* attackedPlayer);

	typedef std::vector<TimeProbabilityClass> TimeProbabilityClassVec;

	typedef std::map<std::string, TimeProbabilityClassVec> InstantAttackSpells;
	InstantAttackSpells instantSpells;
	
	typedef std::map<unsigned short, TimeProbabilityClassVec> RuneAttackSpells;
	RuneAttackSpells runeSpells;

	typedef std::map<PhysicalAttackClass*, TimeProbabilityClass> PhysicalAttacks;
	PhysicalAttacks physicalAttacks;

	typedef std::vector<std::pair<std::string, TimeProbabilityClass> > YellingSentences;
	YellingSentences yellingSentences;
	
	Position targetPos;
	Position moveToPos;
	void doMoveTo(const Position &target);

	virtual fight_t getFightType() {return curPhysicalAttack->fighttype; /*curPhysicalAttack != NULL ? curPhysicalAttack->fighttype : FIGHT_MELEE);*/};
	virtual subfight_t getSubFightType()
	{return curPhysicalAttack->disttype; /*return (curPhysicalAttack != NULL ? curPhysicalAttack->disttype : DIST_NONE);*/}
	virtual int getWeaponDamage() const;

	void OnCreatureEnter(const Creature *creature);
	void OnCreatureLeave(const Creature *creature);

	virtual int getLostExperience() {return experience;};

	virtual void onThingMove(const Creature *creature, const Thing *thing, const Position *oldPos, unsigned char oldstackpos);
  virtual void onCreatureAppear(const Creature *creature);
  virtual void onCreatureDisappear(const Creature *creature, unsigned char stackPos);
  virtual void onTeleport(const Creature *creature, const Position *oldPos, unsigned char oldstackpos);

  //virtual void onCreatureTurn(const Creature *creature, unsigned char stackpos);
  //virtual void onCreatureSay(const Creature *creature, unsigned char type, const std::string &text);
  //virtual void onCreatureChangeOutfit(const Creature* creature);
	virtual void onThink();
  virtual void setAttackedCreature(unsigned long id);
  virtual std::string getDescription() const;
  std::string monstername;
	bool loaded;
};

#endif // __monster_h_

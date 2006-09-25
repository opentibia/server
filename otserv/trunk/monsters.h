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

#ifndef __monsters_h_
#define __monsters_h_

#include <string>

#include "creature.h"

class TimeProbabilityClass{
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
			bool ret = (random_range(1, 100) <= probability);
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
		
		if(_probability >= 0)
			probability = std::min(100, _probability);
		
		if(_exhaustionticks >= 0)
			exhaustionTicks = _exhaustionticks;
	}
	
	int getExhaustion() const {return exhaustionTicks;}
	
private:
	void setDefault()
	{
		cycleTicks = 2000;
		ticksleft = cycleTicks;
		probability = 80;
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

#define CHANCE_MAX  100000
struct LootBlock{
	unsigned short id;
	unsigned short countmax;
	unsigned long chance1;
	unsigned long chancemax;
	typedef std::list<LootBlock> LootItems;
	LootItems childLoot;
	LootBlock(){
		id = 0;
		countmax = 0;
		chance1 = 0;
		chancemax = 0;
	}
};	

struct summonBlock{
	std::string name;
	unsigned long summonChance;
};

typedef std::list<LootBlock> LootItems;
typedef std::vector<TimeProbabilityClass> TimeProbabilityClassVec;	
typedef std::map<std::string, TimeProbabilityClassVec> InstantAttackSpells;
typedef std::map<unsigned short, TimeProbabilityClassVec> RuneAttackSpells;
typedef std::map<PhysicalAttackClass*, TimeProbabilityClass> PhysicalAttacks;
typedef std::vector<std::pair<std::string, TimeProbabilityClass> > YellingSentences;
typedef std::list<summonBlock> SummonSpells;

class MonsterType{
public:
	MonsterType();
	~MonsterType();
	
	void reset();
	
	std::string name;
	int experience;
	int armor;
	int defense;
	bool hasDistanceAttack;
	bool canPushItems;
	unsigned long staticLook;
	unsigned long staticAttack;
	unsigned short changeTargetChance;  
	int maxSummons;
	int targetDistance;
	int runAwayHealth;
	bool pushable;
	int base_speed;
	int level;
	int maglevel;
	int health;
	int health_max;
	int lookhead, lookbody, looklegs, lookfeet, looktype, lookcorpse, lookmaster, lookaddons;
	int immunities;
	
	int lightLevel;
	int lightColor;
	
	InstantAttackSpells instantSpells;
	RuneAttackSpells runeSpells;
	PhysicalAttacks physicalAttacks;
	YellingSentences yellingSentences;
	SummonSpells summonSpells; 

	LootItems lootItems;
	
	void createLoot(Container* corpse);
	void createLootContainer(Container* parent, const LootBlock& lootblock);
	Item* createLootItem(const LootBlock& lootblock);
};

class Monsters{
public:
	Monsters();
	~Monsters();
	
	bool loadFromXml(const std::string &_datadir, bool reloading = false);
	bool isLoaded(){return loaded;}	
	bool reload();
	
	MonsterType* getMonsterType(unsigned long mid);
	unsigned long getIdByName(const std::string& name);
	
private:
	MonsterType* loadMonster(const std::string& file, const std::string& monster_name, bool reloading = false);
	bool loadLootContainer(xmlNodePtr, LootBlock&);
	bool loadLootItem(xmlNodePtr, LootBlock&);

	typedef std::map<std::string, unsigned long> MonsterNameMap;
	MonsterNameMap monsterNames;
	
	typedef std::map<unsigned long, MonsterType*> MonsterMap;
	MonsterMap monsters;
	
	bool loaded;
	std::string datadir;
		
};

#endif

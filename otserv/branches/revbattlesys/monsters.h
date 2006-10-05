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

#ifndef __OTSERV_MONSTERS_H__
#define __OTSERV_MONSTERS_H__

#include <string>
#include "creature.h"

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

struct summonBlock_t{
	std::string name;
	uint32_t chance;
};

class Spell;

struct spellBlock_t{
	Spell* spell;
	uint32_t chance;
};

/*
struct voiceBlock_t{
	std::string text;
	uint32_t chance;
};
*/

typedef std::list<LootBlock> LootItems;
typedef std::list<summonBlock_t> SummonList;
typedef std::list<spellBlock_t> SpellList;
typedef std::vector<std::string> voiceVector;

class MonsterType{
public:
	MonsterType();
	~MonsterType();
	
	void reset();
	
	std::string name;
	std::string nameDescription;
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
	int health;
	int health_max;
	int lookhead, lookbody, looklegs, lookfeet, looktype, lookcorpse, lookmaster;
	int immunities;
	RaceType_t race;
	
	int lightLevel;
	int lightColor;
	
	SummonList summonList;
	LootItems lootItems;
	SpellList spellList;

	uint32_t yellChance;
	voiceVector voiceVector;
	
	void createLoot(Container* corpse);
	void createLootContainer(Container* parent, const LootBlock& lootblock);
	Item* createLootItem(const LootBlock& lootblock);
};

class Monsters{
public:
	Monsters();
	~Monsters();
	
	bool loadFromXml(const std::string& _datadir, bool reloading = false);
	bool isLoaded(){return loaded;}	
	bool reload();
	
	MonsterType* getMonsterType(unsigned long mid);
	unsigned long getIdByName(const std::string& name);

	static unsigned long getRandom();
	
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

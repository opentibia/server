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

#define MAX_LOOTCHANCE 100000
#define MAX_STATICWALK 100

struct LootBlock{
	unsigned short id;
	unsigned short countmax;
	unsigned long chance;

	//optional
	int subType;
	int actionId;
	std::string text;

	typedef std::list<LootBlock> LootItems;
	LootItems childLoot;
	LootBlock(){
		id = 0;
		countmax = 0;
		chance = 0;

		subType = -1;
		actionId = -1;
		text = "";
	}
};	

struct summonBlock_t{
	std::string name;
	uint32_t chance;
	uint32_t speed;
};

class BaseSpell;

struct spellBlock_t{
	BaseSpell* spell;
	uint32_t chance;
	uint32_t speed;
	uint32_t range;
	int32_t minCombatValue;
	int32_t maxCombatValue;
	bool combatSpell;
};

struct voiceBlock_t{
	std::string text;
	bool yellText;
};

typedef std::list<LootBlock> LootItems;
typedef std::list<summonBlock_t> SummonList;
typedef std::list<spellBlock_t> SpellList;
typedef std::vector<voiceBlock_t> VoiceVector;

class MonsterType{
public:
	MonsterType();
	~MonsterType();
	
	void reset();
	
	std::string name;
	std::string nameDescription;
	int experience;

	int defense;
	int armor;

	bool canPushItems;
	unsigned long staticAttackChance;
	int maxSummons;
	int targetDistance;
	int runAwayHealth;
	bool pushable;
	int base_speed;
	int health;
	int health_max;
	
	Outfit_t outfit;
	int32_t lookcorpse;
	int conditionImmunities;
	int damageImmunities;
	RaceType_t race;
	bool isSummonable;
	bool isIllusionable;
	bool isConvinceable;
	bool isAttackable;
	bool isHostile;
	
	int lightLevel;
	int lightColor;
		
	uint32_t manaCost;
	SummonList summonList;
	LootItems lootItems;
	SpellList spellAttackList;
	SpellList spellDefenseList;

	uint32_t yellChance;
	uint32_t yellSpeedTicks;
	VoiceVector voiceVector;

	int32_t changeTargetSpeed;
	int32_t changeTargetChance;

	int32_t attackStrength;
	int32_t defenseStrength;

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
	
	MonsterType* getMonsterType(const std::string& name);
	MonsterType* getMonsterType(unsigned long mid);
	unsigned long getIdByName(const std::string& name);

	static uint32_t getLootRandom();
	
private:
	ConditionDamage* getDamageCondition(ConditionType_t conditionType, int32_t maxDamage, int32_t minDamage, int32_t startDamage);
	bool deserializeSpell(xmlNodePtr node, spellBlock_t& sb);

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

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

#include <libxml/parser.h>

#define MAX_LOOTCHANCE 100000
#define MAX_STATICWALK 100

struct LootBlock{
	unsigned short id;
	unsigned short countmax;
	uint32_t chance;

	//optional
	int subType;
	int actionId;
	std::string text;

	typedef std::list<LootBlock> LootItems;
	LootItems childLoot;
	bool dropEmpty;

	LootBlock(){
		id = 0;
		countmax = 0;
		chance = 0;

		subType = -1;
		actionId = -1;
		text = "";
		dropEmpty = false;
	}
};

struct summonBlock_t{
	std::string name;
	uint32_t chance;
	uint32_t speed;
};

struct spellBlock_t{
	//BaseSpell* spell;
	uint32_t chance;
	uint32_t speed;
	uint32_t range;
	int32_t minCombatValue;
	int32_t maxCombatValue;
	bool combatSpell;
	bool isMelee;
};

struct voiceBlock_t{
	std::string text;
	bool yellText;
};

typedef std::list<LootBlock> LootItems;
typedef std::list<summonBlock_t> SummonList;
typedef std::list<spellBlock_t> SpellList;
typedef std::vector<voiceBlock_t> VoiceVector;
//typedef std::list<std::string> MonsterScriptList;
typedef std::map<CombatType, int32_t> ElementMap;


class InternalCreatureType;

class CreatureType{
	CreatureType& operator=(const CreatureType& ct);
public:
	CreatureType();
	CreatureType(const CreatureType& ct);
	~CreatureType();

	static uint32_t getLootChance();
	void createLoot(Container* corpse) const;

	#define DECLARE_PROPERTY(proptype, propname) \
		proptype& propname(); \
		const proptype& propname() const; \
		void propname(const proptype& v);

	DECLARE_PROPERTY(std::string, name)
	DECLARE_PROPERTY(std::string, nameDescription)
	DECLARE_PROPERTY(std::string, fileLoaded)
	
	DECLARE_PROPERTY(uint64_t, experience)
	DECLARE_PROPERTY(int, defense)
	DECLARE_PROPERTY(int, armor)
	DECLARE_PROPERTY(bool, canPushItems)
	DECLARE_PROPERTY(bool, canPushCreatures)
	DECLARE_PROPERTY(uint32_t, staticAttackChance)
	DECLARE_PROPERTY(int, maxSummons)
	DECLARE_PROPERTY(int, targetDistance)
	DECLARE_PROPERTY(int, fleeHealth)
	DECLARE_PROPERTY(bool, pushable)
	DECLARE_PROPERTY(int, base_speed)
	DECLARE_PROPERTY(int, health)
	DECLARE_PROPERTY(int, health_max)
	
	DECLARE_PROPERTY(OutfitType, outfit)
	DECLARE_PROPERTY(int32_t, corpseId)
	DECLARE_PROPERTY(ConditionType, conditionImmunities)
	DECLARE_PROPERTY(CombatType, damageImmunities)
	DECLARE_PROPERTY(RaceType, race)
	
	DECLARE_PROPERTY(bool, isSummonable)
	DECLARE_PROPERTY(bool, isIllusionable)
	DECLARE_PROPERTY(bool, isConvinceable)
	DECLARE_PROPERTY(bool, isAttackable)
	DECLARE_PROPERTY(bool, isHostile)
	DECLARE_PROPERTY(bool, isLureable)
	DECLARE_PROPERTY(int, lightLevel)
	DECLARE_PROPERTY(int, lightColor)
	DECLARE_PROPERTY(uint32_t, manaCost)

	DECLARE_PROPERTY(SummonList, summonList)
	DECLARE_PROPERTY(LootItems, lootItems)
	DECLARE_PROPERTY(ElementMap, elementMap)

	DECLARE_PROPERTY(SpellList, spellAttackList)
	DECLARE_PROPERTY(SpellList, spellDefenseList)

	DECLARE_PROPERTY(uint32_t, yellChance)
	DECLARE_PROPERTY(uint32_t, yellSpeedTicks)
	DECLARE_PROPERTY(VoiceVector, voiceVector)
	DECLARE_PROPERTY(int32_t, changeTargetSpeed)
	DECLARE_PROPERTY(int32_t, changeTargetChance)

	#undef DECLARE_PROPERTY
private:
	void self_copy();
	InternalCreatureType* impl;
};

#endif

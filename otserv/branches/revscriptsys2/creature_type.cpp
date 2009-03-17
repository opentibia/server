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
#include "otpch.h"

#include "tools.h"
#include "creature_type.h"
#include "actor.h"
#include "configmanager.h"
#include "container.h"

extern ConfigManager g_config;

// Internal creature type

class InternalCreatureType{
public:
	InternalCreatureType();
	~InternalCreatureType();

	void reset();

	int reference_count;

	std::string name;
	std::string nameDescription;
	std::string fileLoaded;
	uint64_t experience;

	int defense;
	int armor;

	bool canPushItems;
	bool canPushCreatures;
	uint32_t staticAttackChance;
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
	bool isLureable;

	int lightLevel;
	int lightColor;

	uint32_t manaCost;
	SummonList summonList;
	LootItems lootItems;
	ElementMap elementMap;
	SpellList spellAttackList;
	SpellList spellDefenseList;

	uint32_t yellChance;
	uint32_t yellSpeedTicks;
	VoiceVector voiceVector;

	int32_t changeTargetSpeed;
	int32_t changeTargetChance;

	//MonsterScriptList scriptList;

	void createLoot(Container* corpse);
	void createLootContainer(Container* parent, const LootBlock& lootblock);
	Item* createLootItem(const LootBlock& lootblock);
};

InternalCreatureType::InternalCreatureType() : reference_count(0)
{
	reset();
}

void InternalCreatureType::reset()
{
	experience = 0;

	defense = 0;
	armor = 0;

	canPushItems = false;
	canPushCreatures = false;
	staticAttackChance = 95;
	maxSummons = 0;
	targetDistance = 1;
	runAwayHealth = 0;
	pushable = true;
	base_speed = 200;
	health = 100;
	health_max = 100;

	outfit.lookHead   = 130; // Default man
	outfit.lookBody   = 0;
	outfit.lookLegs   = 0;
	outfit.lookFeet   = 0;
	outfit.lookType   = 0;
	outfit.lookTypeEx = 0;
	outfit.lookAddons = 0;
	lookcorpse = 0;

	conditionImmunities = 0;
	damageImmunities = 0;
	race = RACE_BLOOD;
	isSummonable = false;
	isIllusionable = false;
	isConvinceable = false;
	isAttackable = true;
	isHostile = true;
	isLureable = false;

	lightLevel = 0;
	lightColor = 0;

	manaCost = 0;
	summonList.clear();
	lootItems.clear();
	elementMap.clear();

	spellAttackList.clear();
	spellDefenseList.clear();

	yellSpeedTicks = 0;
	yellChance = 0;
	voiceVector.clear();

	changeTargetSpeed = 0;
	changeTargetChance = 0;

	//scriptList.clear();
}

InternalCreatureType::~InternalCreatureType()
{
	reset();
}

void InternalCreatureType::createLoot(Container* corpse)
{
	for(LootItems::const_iterator it = lootItems.begin(); it != lootItems.end() && (corpse->capacity() - corpse->size() > 0); it++){
		Item* tmpItem = createLootItem(*it);
		if(tmpItem){
			//check containers
			if(Container* container = tmpItem->getContainer()){
				createLootContainer(container, *it);
				if(container->size() == 0){
					delete container;
				}
				else{
					corpse->__internalAddThing(tmpItem);
				}
			}
			else{
				corpse->__internalAddThing(tmpItem);
			}
		}
	}
}

Item* InternalCreatureType::createLootItem(const LootBlock& lootBlock)
{
	Item* tmpItem = NULL;
	if(Item::items[lootBlock.id].stackable){
		uint32_t randvalue = CreatureType::getLootChance();
		if(randvalue < lootBlock.chance){
			uint32_t n = randvalue % lootBlock.countmax + 1;
			tmpItem = Item::CreateItem(lootBlock.id, n);
		}
	}
	else{
		if(CreatureType::getLootChance() < lootBlock.chance){
			tmpItem = Item::CreateItem(lootBlock.id, 0);
		}
	}

	if(tmpItem){
		if(lootBlock.subType != -1){
			tmpItem->setSubType(lootBlock.subType);
		}

		if(lootBlock.actionId != -1){
			tmpItem->setActionId(lootBlock.actionId);
		}

		if(lootBlock.text != ""){
			tmpItem->setText(lootBlock.text);
		}

		return tmpItem;
	}

	return NULL;
}

void InternalCreatureType::createLootContainer(Container* parent, const LootBlock& lootblock)
{
	if(parent->size() < parent->capacity()){
		LootItems::const_iterator it;
		for(it = lootblock.childLoot.begin(); it != lootblock.childLoot.end(); it++){
			Item* tmpItem = createLootItem(*it);
			if(tmpItem){
				if(Container* container = tmpItem->getContainer()){
					createLootContainer(container, *it);
					if(container->size() == 0 && it->dropEmpty == false){
						delete container;
					}
					else{
						parent->__internalAddThing(container);
					}
				}
				else{
					parent->__internalAddThing(tmpItem);
				}
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// Creature type implementation

CreatureType::CreatureType(){
	impl = new InternalCreatureType();
	impl->reference_count = 1;
}

CreatureType::CreatureType(const CreatureType& ct){
	impl = ct.impl;
	impl->reference_count += 1;
}

CreatureType::~CreatureType(){
	impl->reference_count -= 1;
	if(impl->reference_count == 0)
		delete impl;
}

uint32_t CreatureType::getLootChance()
{
	return random_range(0, MAX_LOOTCHANCE)/g_config.getNumber(ConfigManager::RATE_LOOT);
}

void CreatureType::self_copy(){
	if(impl->reference_count == 1){
		// We are the only owner, no need to copy anything
	}
	else{
		// Make a copy of the type, and make us the only owner
		InternalCreatureType* oimpl = impl;
		impl = new InternalCreatureType(*oimpl);
		impl->reference_count = 1;
		oimpl->reference_count -= 1; // No need to check for deletion, will always be >= 1
	}
}

void CreatureType::createLoot(Container* corpse) const {
}

#define DEFINE_PROPERTY(proptype, propname) \
	const proptype& CreatureType::propname() const { \
		return impl->propname; \
	} \
	\
	proptype& CreatureType::propname() { \
		return impl->propname; \
	} \
	\
	void CreatureType::propname(const proptype& v){ \
		self_copy(); \
		impl->propname = v; \
	}

DEFINE_PROPERTY(std::string, name)
DEFINE_PROPERTY(std::string, nameDescription)
DEFINE_PROPERTY(std::string, fileLoaded)
DEFINE_PROPERTY(uint64_t, experience)
DEFINE_PROPERTY(int, defense)
DEFINE_PROPERTY(int, armor)
DEFINE_PROPERTY(bool, canPushItems)
DEFINE_PROPERTY(bool, canPushCreatures)
DEFINE_PROPERTY(uint32_t, staticAttackChance)
DEFINE_PROPERTY(int, maxSummons)
DEFINE_PROPERTY(int, targetDistance)
DEFINE_PROPERTY(int, runAwayHealth)
DEFINE_PROPERTY(bool, pushable)
DEFINE_PROPERTY(int, base_speed)
DEFINE_PROPERTY(int, health)
DEFINE_PROPERTY(int, health_max)
DEFINE_PROPERTY(Outfit_t, outfit)
DEFINE_PROPERTY(int32_t, lookcorpse)
DEFINE_PROPERTY(int, conditionImmunities)
DEFINE_PROPERTY(int, damageImmunities)
DEFINE_PROPERTY(RaceType_t, race)
DEFINE_PROPERTY(bool, isSummonable)
DEFINE_PROPERTY(bool, isIllusionable)
DEFINE_PROPERTY(bool, isConvinceable)
DEFINE_PROPERTY(bool, isAttackable)
DEFINE_PROPERTY(bool, isHostile)
DEFINE_PROPERTY(bool, isLureable)
DEFINE_PROPERTY(int, lightLevel)
DEFINE_PROPERTY(int, lightColor)
DEFINE_PROPERTY(uint32_t, manaCost)
DEFINE_PROPERTY(SummonList, summonList)
DEFINE_PROPERTY(LootItems, lootItems)
DEFINE_PROPERTY(ElementMap, elementMap)
DEFINE_PROPERTY(SpellList, spellAttackList)
DEFINE_PROPERTY(SpellList, spellDefenseList)
DEFINE_PROPERTY(uint32_t, yellChance)
DEFINE_PROPERTY(uint32_t, yellSpeedTicks)
DEFINE_PROPERTY(VoiceVector, voiceVector)
DEFINE_PROPERTY(int32_t, changeTargetSpeed)
DEFINE_PROPERTY(int32_t, changeTargetChance)

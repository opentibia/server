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

#ifndef __OTSERV_VOCATION_H__
#define __OTSERV_VOCATION_H__

#include <string>
#include <map>
#include <stdint.h>
#include "enums.h"
#include "const.h"

class Vocation
{
	Vocation(uint32_t id);
	~Vocation();

public:
	uint32_t getID() const;
	const std::string& getVocName() const;
	const std::string& getVocDescription() const;
	uint32_t getReqSkillTries(SkillType skill, int32_t level);
	uint32_t getReqMana(int32_t magLevel);

	uint32_t getHPGain() const;
	uint32_t getManaGain() const;
	uint32_t getCapGain() const;

	uint32_t getManaGainTicks() const;
	uint32_t getManaGainAmount() const;
	uint32_t getHealthGainTicks() const;
	uint32_t getHealthGainAmount() const;

	uint16_t getSoulMax() const;
	uint16_t getSoulGainTicks() const;

	float getMeleeBaseDamage(WeaponType weaponType) const;

	float getMagicBaseDamage() const;
	float getWandBaseDamage() const;
	float getHealingBaseDamage() const;

	float getBaseDefense() const;
	float getArmorDefense() const;

	void debugVocation();

protected:
	uint32_t id;
	std::string name;
	std::string description;

	uint32_t gainHealthTicks;
	uint32_t gainHealthAmount;
	uint32_t gainManaTicks;
	uint32_t gainManaAmount;

	uint32_t gainCap;
	uint32_t gainMana;
	uint32_t gainHP;

	uint16_t maxSoul;
	uint16_t gainSoulTicks;

	uint32_t skillBases[SkillType::size];
	float skillMultipliers[SkillType::size];
	float manaMultiplier;

	float swordBaseDamage;
	float axeBaseDamage;
	float clubBaseDamage;
	float distBaseDamage;
	float fistBaseDamage;

	float magicBaseDamage;
	float wandBaseDamage;
	float healingBaseDamage;

	float baseDefense;
	float armorDefense;

	typedef std::map<uint32_t, uint32_t> cacheMap;
	cacheMap cacheMana;
	cacheMap cacheSkill[SkillType::size];

	friend class Vocations;
};

class Vocations
{
public:
	Vocations();
	~Vocations();

	bool loadFromXml(const std::string& datadir);
	Vocation* getVocation(uint32_t vocId);
	int32_t getVocationId(const std::string& name);

private:
	typedef std::map<uint32_t, Vocation*> VocationsMap;
	VocationsMap vocationsMap;
};

#endif

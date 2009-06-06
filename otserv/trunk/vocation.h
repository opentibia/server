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

#include "enums.h"
#include "const.h"
#include <string>
#include <map>

class Vocation
{
public:
	~Vocation();
	const std::string& getVocName() const {return name;}
	const std::string& getVocDescription() const {return description;}
	uint32_t getReqSkillTries(int skill, int level);
	uint32_t getReqMana(int magLevel);

	uint32_t getHPGain() const {return gainHP;};
	uint32_t getManaGain() const {return gainMana;};
	uint32_t getCapGain() const {return gainCap;};

	uint32_t getManaGainTicks() const {return gainManaTicks;};
	uint32_t getManaGainAmount() const {return gainManaAmount;};
	uint32_t getHealthGainTicks() const {return gainHealthTicks;};
	uint32_t getHealthGainAmount() const {return gainHealthAmount;};

	uint16_t getSoulMax() const {return maxSoul;};
	uint16_t getSoulGainTicks() const {return gainSoulTicks;};

	float getMeleeBaseDamage(WeaponType_t weaponType) const
	{
		if(weaponType == WEAPON_SWORD)
			return swordBaseDamage;
		else if(weaponType == WEAPON_AXE)
			return axeBaseDamage;
		else if(weaponType == WEAPON_CLUB)
			return clubBaseDamage;
		else if(weaponType == WEAPON_DIST)
			return distBaseDamage;
		else
			return fistBaseDamage;
	};

	float getMagicBaseDamage() const {return magicBaseDamage;};
	float getWandBaseDamage() const {return wandBaseDamage;};
	float getHealingBaseDamage() const {return healingBaseDamage;};

	float getBaseDefense() const {return baseDefense;};
	float getArmorDefense() const {return armorDefense;};

	void debugVocation();

protected:
	friend class Vocations;
	Vocation();

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

	static uint32_t skillBase[SKILL_LAST + 1];
	float skillMultipliers[SKILL_LAST + 1];
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
	cacheMap cacheSkill[SKILL_LAST + 1];
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
	Vocation def_voc;
};

#endif

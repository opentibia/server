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


#ifndef __OTSERV_WEAPONS_H__
#define __OTSERV_WEAPONS_H__

#include "game.h"
#include "player.h"
#include "combat.h"
#include "const.h"

class Weapon;
class WeaponMelee;
class WeaponDistance;
class WeaponWand;

class Weapon
{
public:
	Weapon();
	virtual ~Weapon();

	virtual bool configureWeapon(const ItemType& it);
	virtual bool interruptSwing() const {return false;}

	static int32_t getMaxWeaponDamage(int32_t level, int32_t attackSkill, int32_t attackValue, float attackFactor);
	static int32_t getMaxMeleeDamage(int32_t attackSkill, int32_t attackValue);

	virtual int32_t playerWeaponCheck(Player* player, Creature* target) const;
	virtual bool useWeapon(Player* player, Item* item, Creature* target) const;

	void setCombatParam(const CombatParams& _params);

	uint16_t getID() const {return id;}

	static bool useFist(Player* player, Creature* target);
	virtual int32_t getWeaponDamage(const Player* player, const Creature* target, const Item* item, bool maxDamage = false) const = 0;
	virtual int32_t getElementDamage(const Player* player, const Creature* target) const {return 0;}

	const uint32_t getReqLevel() const {return level;}
	const uint32_t getReqMagLv() const {return magLevel;}
	const bool hasExhaustion() const {return (exhaustion != 0);}
	const bool isWieldedUnproperly() const {return wieldUnproperly;}
	const bool isPremium() const {return premium;}

protected:
	bool executeUseWeapon(Player* player) const;
	bool internalUseWeapon(Player* player, Item* item, Creature* target, int32_t damageModifier) const;
	bool internalUseWeapon(Player* player, Item* item, Tile* tile) const;

	virtual void onUsedWeapon(Player* player, Item* item, Tile* destTile) const;
	virtual void onUsedAmmo(Player* player, Item* item, Tile* destTile) const;
	virtual bool getSkillType(const Player* player, const Item* item, SkillType& skill, uint32_t& skillpoint) const {return false;};

	int32_t getManaCost(const Player* player) const;

	uint16_t id;
	bool scripted;
	bool enabled;
	bool premium;
	int32_t exhaustion;
	bool wieldUnproperly;
	int32_t level;
	int32_t magLevel;
	int32_t mana;
	int32_t manaPercent;
	int32_t soul;
	int32_t range;
	AmmoAction_t ammoAction;
	CombatParams params;

private:
	typedef std::map<int32_t, bool> VocWeaponMap;
	VocWeaponMap vocWeaponMap;
};

class WeaponMelee : public Weapon
{
public:
	WeaponMelee();
	~WeaponMelee() {};

	bool configureWeapon(const ItemType& it);

	bool useWeapon(Player* player, Item* item, Creature* target) const;
	int32_t getWeaponDamage(const Player* player, const Creature* target, const Item* item, bool maxDamage = false) const;
	int32_t getElementDamage(const Player* player, const Item* item) const;

protected:
	void onUsedWeapon(Player* player, Item* item, Tile* destTile) const;
	void onUsedAmmo(Player* player, Item* item, Tile* destTile) const;
	bool getSkillType(const Player* player, const Item* item, SkillType& skill, uint32_t& skillpoint) const;

	CombatType elementType;
	int16_t elementDamage;
};

class WeaponDistance : public Weapon
{
public:
	WeaponDistance();
	~WeaponDistance() {};

	virtual bool configureWeapon(const ItemType& it);

	//virtual int32_t playerWeaponCheck(Player* player, Creature* target) const;
	virtual bool useWeapon(Player* player, Item* item, Creature* target) const;
	virtual int32_t getWeaponDamage(const Player* player, const Creature* target, const Item* item, bool maxDamage = false) const;

protected:
	void onUsedWeapon(Player* player, Item* item, Tile* destTile) const;
	void onUsedAmmo(Player* player, Item* item, Tile* destTile) const;
	bool getSkillType(const Player* player, const Item* item, SkillType& skill, uint32_t& skillpoint) const;

	int32_t hitChance;
	int32_t maxHitChance;
	int32_t breakChance;
	int32_t ammuAttackValue;
};

class WeaponWand : public Weapon
{
public:
	WeaponWand();
	virtual ~WeaponWand() {};
	
	bool configureWeapon(const ItemType& it);

	virtual int32_t getWeaponDamage(const Player* player, const Creature* target, const Item* item, bool maxDamage = false) const;

protected:
	bool getSkillType(const Player* player, const Item* item, SkillType& skill, uint32_t& skillpoint) const {return false;};

	int32_t minChange;
	int32_t maxChange;
};

#endif

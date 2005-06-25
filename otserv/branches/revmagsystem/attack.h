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


#ifndef __attacks_h_
#define __attacks_h_

#include "const74.h"
#include "creature.h"

enum eDamgeType{
	DAMAGE_HEALTH,
	DAMAGE_MANA,
};

enum eATextColor{
	ATEXT_NONE = 255,
	ATEXT_WHITE = 215,
	ATEXT_BLUE = 2,
	ATEXT_RED = 180,
	ATEXT_GREEN = 50,
};

enum eBloodColor{
	BLOOD_RED = FLUID_RED,
	BLOOD_GREEN = FLUID_GREEN,
	BLOOD_NONE = 255,
};

enum eAttackTypes{
	ATTACK_TYPE_NONE,
	ATTACK_TYPE_MELEE,
	ATTACK_TYPE_DISTANCE,
	ATTACK_TYPE_MAGIC, //energy,poison,fire,explosion,sd
	ATTACK_TYPE_HEALTH,
	ATTACK_TYPE_MANA,
	ATTACK_TYPE_LIFE,
	ATTACK_TYPE_PARALYZE,
};

class Attack{
public:
	Attack();
	virtual ~Attack();
	
	unsigned long getOwnerId(){return ownerId;}
	Creature* getOwner() {return owner;}
	eAttackTypes getAttackType(){return attack_type;}
	
	virtual long getDrawInfo(MagicEffectClasses &me, eATextColor text_color,
					eBloodColor blood_color, subfight_t shoot) = 0;
	virtual long getSkillId() = 0;
	virtual bool addShieldTry() = 0;
	virtual long getDamage(Creature *AttackedCreature, eDamgeType &type) = 0;
	
protected:
	
	bool checkManaShield(Creature *attackedCreature, long damage);
	void getDrawInfoManaShield(MagicEffectClasses &me, eATextColor text_color);
	
	bool manaAttack;
	unsigned long ownerId;
	Creature *owner;
	eAttackTypes attack_type;
};

class AttackMelee : public Attack {
	AttackMelee();
	virtual ~AttackMelee();
	virtual long getDrawInfo(MagicEffectClasses &me, eATextColor text_color,
					eBloodColor blood_color, subfight_t shoot);
	virtual long getSkillId();
	virtual bool addShieldTry();
	virtual long getDamage(Creature *AttackedCreature, eDamgeType &type);
};

#endif

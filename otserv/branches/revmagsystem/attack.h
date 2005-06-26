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
	ATEXT_WHITE_EXP = 215,
	ATEXT_WHITE = 208,
	ATEXT_BLUE = 2,
	ATEXT_RED = 180,
	ATEXT_GREEN = 50,
};

enum eBloodColor{
	BLOOD_RED = FLUID_RED,
	BLOOD_GREEN = FLUID_GREEN,
	BLOOD_NONE = 255,
};

enum eSquareColor{
	SQ_COLOR_NONE = 256,
	SQ_COLOR_BLACK = 0,
};

enum eAttackDistanceBlockType{
	ATTACK_DIST_BLOCK_ARMOR = 1,
	ATTACK_DIST_BLOCK_SHIELD = 2,
	ATTACK_DIST_BLOCK_NO_BLOCK = 0,
};

enum eAttackTypes{
	ATTACK_TYPE_NONE,
	ATTACK_TYPE_MELEE,
	ATTACK_TYPE_DISTANCE_PHYSICAL,
	ATTACK_TYPE_DISTANCE_MAGIC,
	ATTACK_TYPE_MAGIC, //energy,poison,fire,explosion,sd
	ATTACK_TYPE_HEALTH,
	ATTACK_TYPE_MANA,
	ATTACK_TYPE_LIFE,
	ATTACK_TYPE_PARALYZE,
};

class Attack{
public:
	Attack();
	virtual ~Attack(){};
	
	unsigned long getOwnerId(){return ownerId;}
	Creature* getOwner() {return owner;}
	eAttackTypes getAttackType(){return attack_type;}
	
	//static Attack* createAttack(.....);
	
	virtual void getDrawInfo(MagicEffectClasses &me, eATextColor &text_color,
					eBloodColor &blood_color, subfight_t &shoot, eSquareColor &sqcolor) = 0;
	virtual long getSkillId() = 0;
	virtual bool addShieldTry() = 0;
	virtual long getDamage(Creature *AttackedCreature, eDamgeType &type) = 0;
	
protected:
	
	bool checkManaShield(Creature *attackedCreature, long damage);
	void getDrawInfoManaShield(MagicEffectClasses &me, eATextColor &text_color
			,eBloodColor &blood_color, subfight_t &shoot);
	void getDrawInfoRace(race_t race ,MagicEffectClasses &me, eATextColor &text_color
			,eBloodColor &blood_color);
	
	bool manaAttack;
	unsigned long ownerId;
	Creature *owner;
	eAttackTypes attack_type;
};

class AttackMelee : public Attack {
public:
	AttackMelee();
	virtual ~AttackMelee(){};
	
	void initialize(Creature *attacker, Item* weapon);
	void initialize(Creature *attacker, long damage);
	
	virtual void getDrawInfo(MagicEffectClasses &me, eATextColor &text_color,
					eBloodColor &blood_color, subfight_t &shoot, eSquareColor &sqcolor);
	virtual long getSkillId();
	virtual bool addShieldTry();
	virtual long getDamage(Creature *AttackedCreature, eDamgeType &type);
protected:
	long base_damage;
	long skill;
	MagicEffectClasses internal_me;
	eSquareColor internal_sq;
	race_t attacked_race;
	bool addSkillTry;
	//condition *condition;
};

class AttackDistancePhysical : public Attack {
public:
	AttackDistancePhysical();
	virtual ~AttackDistancePhysical(){};
	
	void initialize(Creature *attacker, Item* weapon);
	void initialize(Creature *attacker, long damage, subfight_t type, long block_type = 3);
	
	virtual void getDrawInfo(MagicEffectClasses &me, eATextColor &text_color,
					eBloodColor &blood_color, subfight_t &shoot, eSquareColor &sqcolor);
	virtual long getSkillId();
	virtual bool addShieldTry();
	virtual long getDamage(Creature *AttackedCreature, eDamgeType &type);
protected:
	long base_damage;
	MagicEffectClasses internal_me;
	eSquareColor internal_sq;
	subfight_t internal_shoot;
	race_t attacked_race;
	long internal_block_type;
	long hitchance;
	bool addSkillTry;
	Item *internal_dist_item;
	//condition *condition;
};


#endif

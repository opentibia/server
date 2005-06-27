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

#ifndef __MAGIC_H__
#define __MAGIC_H__

class Creature;
class Player;
class Position;
class Item;
class Game;

/*
MagicEffectClass
|
|	
|----->	MagicEffectTargetClass : public MagicEffectClass
|       |
|       |-----> MagicEffectTargetEx : public MagicEffectTargetClass //ie. soul fire
|       |
|       |-----> MagicEffectTargetCreatureCondition : public MagicEffectTarget //ie. burning, energized etc.
|       |
|       |-----> MagicEffectTargetGroundClass //ie. m-wall, wild growth
|								(Holds a MagicEffectItem*)
|       
|-----> MagicEffectAreaClass : public MagicEffectClass //ie. gfb
|       |
|       |-----> MagicEffectAreaExClass : public MagicEffectAreaClass //ie. poison storm
|       |
|	      |-----> MagicEffectAreaGroundClass : public MagicEffectArea //ie. fire bomb
|                 (Holds a MagicEffectItem*)
|
|----->	MagicEffectItem : public Item, public MagicEffectClass
*/
//------------------------------------------------------------------------------

class MagicSpell {
public:
	MagicSpell(Spell *ispell);
	virtual ~MagicSpell();
	virtual bool doCastSpell(Creature* spellCastCreature, const Position& pos, const std::string& var) const = 0;
	virtual bool doCastSpell(Creature* spellCastCreature, Creature* targetCreature) const = 0;
	virtual bool causeExhaustion() {return true;};

protected:
	virtual void setArea(const AreaVector& vec) = 0;
	virtual void getArea(const Position& centerpos, std::vector<Position>& vec, unsigned char direction) const = 0;
	Spell *spell;
};

class ConjureItemSpell : public MagicSpell {
public:
	ConjureItemSpell(Spell *ispell, const std::vector<unsigned short>& items, int icount, unsigned char imagicEffect);
	virtual ~ConjureItemSpell() {};
	virtual bool doCastSpell(Creature* spellCastCreature, const Position& pos, const std::string& var) const;
	virtual bool doCastSpell(Creature* spellCastCreature, Creature* targetCreature) const {return false;};

protected:
	virtual void setArea(const AreaVector& vec) {};
	virtual void getArea(const Position& centerpos, std::vector<Position>& vec, unsigned char direction) const {};

	std::vector<unsigned short> items;
	int count;
	unsigned char magicEffect;
};

class ChangeSpeedSpell : public MagicSpell {
public:
	ChangeSpeedSpell(Spell *ispell, int itime, int iaddspeed, unsigned char imagicEffect);
	virtual ~ChangeSpeedSpell() {};
	virtual bool doCastSpell(Creature* spellCastCreature, const Position& pos, const std::string& var) const;
	virtual bool doCastSpell(Creature* spellCastCreature, Creature* targetCreature) const {return false;};

protected:
	virtual void setArea(const AreaVector& vec) {};
	virtual void getArea(const Position& centerpos, std::vector<Position>& vec, unsigned char direction) const {};

	int time;
	int addspeed;
	unsigned char magicEffect;
};

class LightSpell : public MagicSpell {
public:
	LightSpell(Spell *ispell, int itime, unsigned char ilightlevel, unsigned char imagicEffect);
	virtual ~LightSpell() {};
	virtual bool doCastSpell(Creature* spellCastCreature, const Position& pos, const std::string& var) const {return false;};
	virtual bool doCastSpell(Creature* spellCastCreature, Creature* targetCreature) const;

protected:
	virtual void setArea(const AreaVector& vec) {};
	virtual void getArea(const Position& centerpos, std::vector<Position>& vec, unsigned char direction) const {};

	int time;
	unsigned char lightlevel;
	unsigned char magicEffect;
};

class MagicTargetSpell : public MagicSpell {
public:
	MagicTargetSpell(Spell *ispell, attacktype_t iattackType, unsigned char idistanceEffect,
		amuEffect_t iamuInfo);
	virtual ~MagicTargetSpell() {};
	virtual bool doCastSpell(Creature* spellCastCreature, const Position& pos, const std::string& var) const;
	virtual bool doCastSpell(Creature* spellCastCreature, Creature* targetCreature) const;

protected:
	virtual void setArea(const AreaVector& vec) {};
	virtual void getArea(const Position& centerpos, std::vector<Position>& vec, unsigned char direction) const;

	unsigned short distanceEffect;
	amuEffect_t amuInfo;
	attacktype_t attackType;
};

class MagicAreaSpell : public MagicSpell {
public:
	MagicAreaSpell(Spell *ispell, attacktype_t iattackType, unsigned char idistanceEffect,
		const AreaVector& ivec, bool ineedDirection, amuEffect_t iamuInfo);
	virtual ~MagicAreaSpell() {};
	virtual bool doCastSpell(Creature* spellCastCreature, const Position& pos, const std::string& var) const;
	virtual bool doCastSpell(Creature* spellCastCreature, Creature* targetCreature) const;

protected:
	virtual void setArea(const AreaVector& vec) {areaVec = vec;};
	virtual void getArea(const Position& centerpos, std::vector<Position>& vec, unsigned char direction) const;

	amuEffect_t amuInfo;
	attacktype_t attackType;
	unsigned char distanceEffect;
	bool needDirection;
	std::vector< std::vector<unsigned char> > areaVec;

//std::vector<Position> areaVec;

	//std::vector< std::vector<unsigned char> > areaVec;

	/*
	//virtual bool causeExhaustion(bool hasTarget) const;

	//virtual void getMagicEffect(Player* spectator, Creature *spellCastCreature, Creature *targetCreature) const;
	//virtual void getMagicEffect(Player* spectator, Creature *spellCastCreature, const Position& pos) const;

	virtual int getDamage(Creature* target, const Creature* attacker = NULL) const;

	virtual void getMagicEffect(Player* spectator, const Creature* attacker, const Creature* target,
		const Position& pos, int damage, bool isPz, bool isBlocking) const;

	virtual void getDistanceShoot(Player* spectator, const Creature* c, const Position& to,
		bool hasTarget) const;

	virtual void getArea(const Position& rcenterpos, MagicAreaVec& list) const;

	virtual MagicEffectItem* getMagicItem(const Creature* attacker, bool isPz, bool isBlocking) const;

	virtual void FailedToCast(Player* spectator, const Creature* attacker,
		bool isBlocking, bool hasTarget) const;
	*/

	/*
	int minDamage;
	int maxDamage;
	bool offensive;
	bool drawblood; //causes blood splashes
	long manaCost;

	attacktype_t attackType;

	unsigned char animationColor;
	unsigned char animationEffect;
	unsigned char hitEffect;
	unsigned char damageEffect;
	*/
};

/*
//Need a target. Example sudden death
class MagicEffectTargetClass : public MagicEffectClass {
public:
	MagicEffectTargetClass();
	virtual ~MagicEffectTargetClass() {};

	virtual void getMagicEffect(Player* spectator, const Creature* attacker, const Creature* target,
		const Position& pos, int damage, bool isPz, bool isBlocking) const;
	
	virtual void getDistanceShoot(Player* spectator, const Creature* attacker, const Position& to,
		bool hasTarget) const;

	virtual bool canCast(bool isBlocking, bool hasCreature) const
	{
		return !isBlocking && hasCreature;
	}
};
*/

/*
//Is created indirectly, need a target and make magic damage (burning/poisoned/energized)
class MagicEffectTargetCreatureCondition : public MagicEffectTargetClass
{
public:
	MagicEffectTargetCreatureCondition() {};
	//MagicEffectTargetCreatureCondition(unsigned long creatureid);
	virtual ~MagicEffectTargetCreatureCondition() {};

	virtual bool isIndirect() const{return true;}
	virtual bool causeExhaustion(bool hasTarget) const {return false;}

	virtual int getDamage(Creature *target, const Creature *attacker = NULL) const;

	virtual void getMagicEffect(Player* spectator, const Creature* attacker, const Creature* target,
		const Position& pos, int damage, bool isPz, bool isBlocking) const;

	virtual void getDistanceShoot(const Creature* c, const Position& to,
		bool hasTarget) const
	{
		//this class shouldn't have any distance shoots, just return.
	}

	const unsigned long getOwnerID() const {return ownerid;}
	void setOwnerID(unsigned long owner) { ownerid=owner;}

protected:
	//unsigned long ownerid;
};
*/


/*
class CreatureCondition {
public:
	CreatureCondition(long ticks, long count, const MagicEffectTargetCreatureCondition& magicTargetCondition) {
		this->delayTicks = ticks;
		this->count = count;
		this->magicTargetCondition = magicTargetCondition;
		this->internalTicks = delayTicks;
	}

	const MagicEffectTargetCreatureCondition* getCondition() const {return &magicTargetCondition; }

	bool onTick(long tick)
	{
		internalTicks -= tick;
		if(internalTicks <= 0 && count > 0) {
			internalTicks = delayTicks;
			--count;
			return true;
		}

		return false;
	}

	long getCount() const {return count;};

private:
	long delayTicks;
	long count;
	long internalTicks;
	MagicEffectTargetCreatureCondition magicTargetCondition;
};
*/

//<<delayTicks, conditionCount>, MagicEffectTargetCreatureCondition>
//typedef std::vector<CreatureCondition> ConditionVec;

//<duration, ConditionVec>
//typedef std::pair<long, ConditionVec> TransformItem;;

//<type, <duration, <<delayTicks, conditionCount>, MagicEffectTargetCreatureCondition>> >
//typedef std::map<unsigned short, TransformItem> TransformMap;

/*
//Needs target, holds a damage list. Example: Soul fire.
class MagicEffectTargetExClass : public MagicEffectTargetClass
{
public:
	MagicEffectTargetExClass(const ConditionVec& condition);
	virtual ~MagicEffectTargetExClass() {};

	virtual int getDamage(Creature *target, const Creature *attacker = NULL) const;

protected:
	ConditionVec condition;
};
*/

/*
//magic field (Fire/Energy/Poison) and solid objects (Magic-wall/Wild growth)
class MagicEffectItem : public Item, public MagicEffectClass
{
public:
	MagicEffectItem(const TransformMap& transformMap);

	virtual void useThing() {
		//std::cout << "Magic: useThing() " << this << std::endl;
		useCount++;
	};
	
	virtual void releaseThing() {
		//std::cout << "Magic: releaseThing() " << this << std::endl;
		useCount--;
		//if (useCount == 0)
		if (useCount <= 0)
			delete this;
	};
	
	const MagicEffectTargetCreatureCondition* getCondition() const;

	virtual bool causeExhaustion(bool hasTarget) const {return false;}

	virtual int getDamage(Creature *target, const Creature *attacker = NULL) const;

	virtual Item* decay();
	bool transform(const MagicEffectItem *rhs);
	long getDecayTime();
	
protected:
	int useCount;
	void buildCondition();
	TransformMap transformMap;
	ConditionVec condition;
};
*/

/*
//Create a solid object. Example: Magic wall, Wild growth
class MagicEffectTargetGroundClass : public MagicEffectTargetClass {
public:
	MagicEffectTargetGroundClass(MagicEffectItem* fieldItem);
	virtual ~MagicEffectTargetGroundClass();

	virtual bool causeExhaustion(bool hasTarget) const
	{
		return true;
	}

	virtual int getDamage(Creature *target, const Creature *attacker = NULL) const
	{
		return 0;
	}

	virtual void getMagicEffect(Player* spectator, const Creature* attacker, const Creature* target,
		const Position& pos, int damage, bool isPz, bool isBlocking) const;

	virtual void getDistanceShoot(Player* spectator, const Creature* attacker, const Position& to,
		bool hasTarget) const;

	virtual MagicEffectItem* getMagicItem(const Creature* attacker, bool isPz, bool isBlocking) const;

	virtual bool canCast(bool isBlocking, bool hasCreature) const;

	virtual void FailedToCast(Player* spectator, const Creature* attacker,
		bool isBlocking, bool hasTarget) const;

protected:
	MagicEffectItem* magicItem;
};
*/

/*
//Don't need a target. Example: GFB
class MagicEffectAreaClass : public MagicEffectClass {
public:
	MagicEffectAreaClass();
	virtual ~MagicEffectAreaClass() {};

	virtual bool causeExhaustion(bool hasTarget) const
	{
		return true;
	}

	virtual void getMagicEffect(Player* spectator, const Creature* attacker, const Creature* target,
		const Position& pos, int damage, bool isPz, bool isBlocking) const;

	virtual void getArea(const Position& rcenterpos, MagicAreaVec& list) const;

	unsigned char direction;
	unsigned char areaEffect;
	
	std::vector< std::vector<unsigned char> > areaVec;
};
*/

/*
//Dont need target. Example: Poison storm
class MagicEffectAreaExClass : public MagicEffectAreaClass
{
public:
	MagicEffectAreaExClass(const ConditionVec& dmglist);
	virtual ~MagicEffectAreaExClass() {};

	virtual int getDamage(Creature *target, const Creature *attacker = NULL) const;

protected:
	ConditionVec condition;
};
*/

/*
//Don't need a target. Example: Fire bomb
class MagicEffectAreaGroundClass : public MagicEffectAreaClass
{
public:
	MagicEffectAreaGroundClass(MagicEffectItem* fieldItem);
	virtual ~MagicEffectAreaGroundClass();

	virtual int getDamage(Creature *target, const Creature *attacker = NULL) const;

	virtual MagicEffectItem* getMagicItem(const Creature* attacker, bool isPz, bool isBlocking) const;
	
protected:
	MagicEffectItem* magicItem;
};
*/

//#include "creature.h"
#endif //__MAGIC_H__


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

#include "position.h"
#include "item.h"

#include "networkmessage.h"
#include "tools.h"

/*
MagicEffectClass
|
|	
|----->	MagicEffectTargetClass : public MagicEffectClass
|       |
|       |-----> MagicEffectTargetEx : public MagicEffectTargetClass //ie. soul fire
|       |
|       |-----> MagicEffectTargetMagicDamageClass : public MagicEffectTarget //burning, energized etc.
|       |
|       |-----> MagicEffectTargetGroundClass //m-wall, wild growth
|								(Holds a MagicEffectItem*)
|       
|-----> MagicEffectAreaClass : public MagicEffectClass //gfb
|       |
|       |-----> MagicEffectAreaExClass : public MagicEffectAreaClass //ie. poison storm
|       |
|	      |-----> MagicEffectGroundAreaClass : public MagicEffectArea //fire bomb
|                 (Holds a MagicEffectItem*)
|
|----->	MagicEffectItem : public Item, public MagicEffectClass
*/
//------------------------------------------------------------------------------

enum MagicDamageType {
	magicNone,
	magicFire,
	magicPoison,
	magicEnergy
};

typedef std::vector<Position> MagicAreaVec;

//<delayTicks, damageCount>
typedef std::pair<long, long> damageTimeCount;

class MagicEffectClass {
public:
	MagicEffectClass();
	virtual ~MagicEffectClass() {};
	
	virtual bool causeExhaustion(bool hasTarget) const;
	virtual int getDamage(Creature *target, const Creature *attacker = NULL) const;
	virtual void getMagicEffect(const Creature* c, const Position& pos, bool hasTarget, int damage, bool isPz, NetworkMessage &msg) const;
	virtual void getDistanceShoot(const Creature* c, const Position& to, bool hasTarget, NetworkMessage &msg) const;
	virtual void getArea(const Position& rcenterpos, MagicAreaVec& list) const;
	//virtual void alterTile(MapState *mapstate, Tile *t);

	int minDamage;
	int maxDamage;
	bool offensive;
	bool physical; //causes blood splashes
	long manaCost;

	unsigned char animationColor;
	unsigned char animationEffect;
	unsigned char damageEffect;
};

//Need a target. Example sudden death
class MagicEffectTargetClass : public MagicEffectClass {
public:
	MagicEffectTargetClass();
	virtual ~MagicEffectTargetClass() {};

	virtual bool causeExhaustion(bool hasTarget) const
	{
		return MagicEffectClass::causeExhaustion(hasTarget);
	}

	virtual int getDamage(Creature *target, const Creature *attacker = NULL) const
	{
		return MagicEffectClass::getDamage(target, attacker);
	}
	
	virtual void getMagicEffect(const Creature* c, const Position& pos, bool hasTarget, int damage, bool isPz, NetworkMessage &msg) const;
	virtual void getDistanceShoot(const Creature* c, const Position& to, bool hasTarget, NetworkMessage &msg) const;
	virtual void getArea(const Position& rcenterpos, MagicAreaVec& list) const
	{
		MagicEffectClass::getArea(rcenterpos, list);
	}
};

//Is created indirectly, need a target and make magic damage (burning/poisoned/energized)
class MagicEffectTargetMagicDamageClass : public MagicEffectTargetClass
{
public:
	MagicEffectTargetMagicDamageClass(const unsigned long creatureid);
	virtual ~MagicEffectTargetMagicDamageClass() {};

	virtual bool causeExhaustion(bool hasTarget) const
	{
		return false;
	}

	virtual int getDamage(Creature *target, const Creature *attacker = NULL) const;

	virtual void getMagicEffect(const Creature* c, const Position& pos, bool hasTarget, int damage, bool isPz, NetworkMessage &msg) const
	{
		MagicEffectTargetClass::getMagicEffect(c, pos, hasTarget, damage, isPz, msg);
	}

	virtual void getDistanceShoot(const Creature* c, const Position& to, bool hasTarget, NetworkMessage &msg) const
	{
		//this class shouldn't have any distance shoots, just return.
	}

	virtual void getArea(const Position& rcenterpos, MagicAreaVec& list) const
	{
		MagicEffectTargetClass::getArea(rcenterpos, list);
	}

	const unsigned long getOwnerID() const {return ownerid;}

protected:
	unsigned long ownerid;
};


//<<delayTicks, damageCount>, MagicEffectTargetMagicDamageClass>
typedef std::pair<damageTimeCount, MagicEffectTargetMagicDamageClass> damageInfo;
typedef std::vector<damageInfo> MagicDamageVec;

//<duration, MagicDamageVectorClass>
typedef std::pair<long, MagicDamageVec> transformInfo;

//<type, <duration, <<delayTicks, damageCount>, MagicEffectTargetMagicDamageClass>> >
typedef std::map<unsigned short, transformInfo> damageMapClass;

//Holds the magic damage (burning/energized/poisoned)
class MagicDamageContainer : public MagicDamageVec {
public:
	MagicDamageContainer(MagicDamageType md);
	MagicDamageContainer(MagicDamageType md, MagicDamageVec list);
	virtual ~MagicDamageContainer() {};

	MagicDamageType getMagicType() const {return this->magictype;}

protected:
	MagicDamageType magictype;
};

//Needs target, holds a damage list. Example: Soul fire.
class MagicEffectTargetEx : public MagicEffectTargetClass
{
public:
	MagicEffectTargetEx(MagicDamageType md, const MagicDamageVec& dmglist);
	virtual ~MagicEffectTargetEx() {};

	virtual bool causeExhaustion(bool hasTarget) const
	{
		return MagicEffectTargetClass::causeExhaustion(hasTarget);	
	}

	virtual int getDamage(Creature *target, const Creature *attacker = NULL) const;

	virtual void getMagicEffect(const Creature* c, const Position& pos, bool hasTarget, int damage, bool isPz, NetworkMessage &msg) const
	{
		MagicEffectTargetClass::getMagicEffect(c, pos, hasTarget, damage, isPz, msg);
	}

	virtual void getDistanceShoot(const Creature* c, const Position& to, bool hasTarget, NetworkMessage &msg) const
	{
		MagicEffectTargetClass::getDistanceShoot(c, to, hasTarget, msg);
	}

	virtual void getArea(const Position& rcenterpos, MagicAreaVec& list) const
	{
		MagicEffectTargetClass::getArea(rcenterpos, list);
	}
	
protected:
		MagicDamageContainer dmgContainer;
};

//magic field (Fire/Energy/Poison) and solid objects (Magic-wall/Wild growth)
class MagicEffectItem : public Item, public MagicEffectClass
{
public:
	MagicEffectItem(const damageMapClass& dmgmap);
	MagicEffectItem(MagicDamageType md, const damageMapClass& dmgmap);

	const MagicEffectTargetMagicDamageClass* getMagicDamageEffect() const;

	virtual bool causeExhaustion(bool hasTarget) const
	{
		return false;
	}

	virtual int getDamage(Creature *target, const Creature *attacker = NULL) const;

	virtual void getMagicEffect(const Creature* c, const Position& pos, bool hasTarget, int damage, bool isPz, NetworkMessage &msg) const
	{
		MagicEffectClass::getMagicEffect(c, pos, hasTarget, damage, isPz, msg);
	}

	virtual void getDistanceShoot(const Creature* c, const Position& to, bool hasTarget, NetworkMessage &msg) const
	{
		MagicEffectClass::getDistanceShoot(c, to, hasTarget, msg);
	}

	virtual void getArea(const Position& rcenterpos, MagicAreaVec& list) const
	{
		MagicEffectClass::getArea(rcenterpos, list);
	}

	bool transform();
	bool transform(const MagicEffectItem &rhs);
	long getDecayTime();
protected:
	void buildDamageList();
	damageMapClass dmgMap;
	MagicDamageContainer dmgContainer;
};

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

	virtual void getMagicEffect(const Creature* c, const Position& pos, bool hasTarget, int damage, bool isPz, NetworkMessage &msg) const
	{
		//Nothing
	}

	virtual void getDistanceShoot(const Creature* c, const Position& to, bool hasTarget, NetworkMessage &msg) const;

	virtual void getArea(const Position& rcenterpos, MagicAreaVec& list) const
	{
		MagicEffectTargetClass::getArea(rcenterpos, list);
	}

	MagicEffectItem* getFieldItem() const {return fieldItem;}

protected:
	MagicEffectItem* fieldItem;
};

//Don't need a target. Example: GFB
class MagicEffectAreaClass : public MagicEffectClass {
public:
	MagicEffectAreaClass();
	virtual ~MagicEffectAreaClass() {};

	virtual bool causeExhaustion(bool hasTarget) const
	{
		return true;
	}

	virtual int getDamage(Creature *target, const Creature *attacker = NULL) const
	{
		return MagicEffectClass::getDamage(target, attacker);
	}

	virtual void getMagicEffect(const Creature* c, const Position& pos, bool hasTarget, int damage, bool isPz, NetworkMessage &msg) const;

	virtual void getDistanceShoot(const Creature* c, const Position& to, bool hasTarget, NetworkMessage &msg) const
	{
		MagicEffectClass::getDistanceShoot(c, to, hasTarget, msg);
	}

	virtual void getArea(const Position& rcenterpos, MagicAreaVec& list) const;

	unsigned char direction;
	unsigned char areaEffect;
	
	std::vector< std::vector<unsigned char> > areaVec;
	//unsigned char area[14][18];
};

//Dont need target. Example: Poison storm
class MagicEffectAreaExClass : public MagicEffectAreaClass
{
public:
	MagicEffectAreaExClass(MagicDamageType md, const MagicDamageVec& dmglist);
	virtual ~MagicEffectAreaExClass() {};

	virtual bool causeExhaustion(bool hasTarget) const
	{
		return MagicEffectAreaClass::causeExhaustion(hasTarget);
	}

	virtual int getDamage(Creature *target, const Creature *attacker = NULL) const;

	virtual void getMagicEffect(const Creature* c, const Position& pos, bool hasTarget, int damage, bool isPz, NetworkMessage &msg) const
	{
		MagicEffectAreaClass::getMagicEffect(c, pos, hasTarget, damage, isPz, msg);
	}

	virtual void getDistanceShoot(const Creature* c, const Position& to, bool hasTarget, NetworkMessage &msg) const
	{
		MagicEffectAreaClass::getDistanceShoot(c, to, hasTarget, msg);
	}

	virtual void getArea(const Position& rcenterpos, MagicAreaVec& list) const
	{
		MagicEffectAreaClass::getArea(rcenterpos, list);
	}

protected:
	MagicDamageContainer dmgContainer;
};

//Don't need a target. Example: Fire bomb
class MagicEffectGroundAreaClass : public MagicEffectAreaClass
{
public:
	MagicEffectGroundAreaClass(MagicEffectItem* fieldItem);
	virtual ~MagicEffectGroundAreaClass();

	virtual bool causeExhaustion(bool hasTarget) const
	{
		return MagicEffectAreaClass::causeExhaustion(hasTarget);
	}

	virtual int getDamage(Creature *target, const Creature *attacker = NULL) const;

	virtual void getMagicEffect(const Creature* c, const Position& pos, bool hasTarget, int damage, bool isPz, NetworkMessage &msg) const
	{
		MagicEffectAreaClass::getMagicEffect(c, pos, hasTarget, damage, isPz, msg);
	}

	virtual void getDistanceShoot(const Creature* c, const Position& to, bool hasTarget, NetworkMessage &msg) const
	{
		MagicEffectAreaClass::getDistanceShoot(c, to, hasTarget, msg);
	}

	virtual void getArea(const Position& rcenterpos, MagicAreaVec& list) const
	{
		MagicEffectAreaClass::getArea(rcenterpos, list);
	}

	MagicEffectItem* getFieldItem() const {return fieldItem;}
protected:
	MagicEffectItem* fieldItem;
};
#endif //__MAGIC_H__


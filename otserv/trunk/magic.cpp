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

#include "definitions.h"
#include "creature.h"
#include "magic.h"

#include "player.h"


MagicEffectClass::MagicEffectClass()
{
	animationColor = 0;
	animationEffect = 0;
	damageEffect = 0xFF;
	minDamage = 0;
	maxDamage = 0;
	offensive = false;
	physical = false;
	manaCost = 0;
}

bool MagicEffectClass::causeExhaustion(bool hasTarget) const
{
	return hasTarget;
}

int MagicEffectClass::getDamage(Creature *target, const Creature *attacker /*= NULL*/) const
{
	if((!offensive || (target != attacker)) && target->access == 0) {
		int damage = (int)random_range(minDamage, maxDamage);

		if(!offensive)
			damage = -damage;
		else {
			if(attacker && attacker->access != 0)
				return damage;

			const Player* p = dynamic_cast<const Player*>(target);

			if(p) {
				damage = (int)floor(damage / 2.0);
			}
		}

		return damage;
	}

	return 0;
}

void MagicEffectClass::getMagicEffect(const Player* spectator, const Creature* attacker, const Position& pos,
			bool hasTarget, int damage, bool isPz, bool isBlocking, NetworkMessage &msg) const
{
	if(!isBlocking && hasTarget) {
		if(spectator->CanSee(pos.x, pos.y, pos.z)) {
			if(physical && damage > 0)
				msg.AddMagicEffect(pos, NM_ME_DRAW_BLOOD);

			if(damageEffect != 0xFF)
				msg.AddMagicEffect(pos, damageEffect);
		}
	}
}

void MagicEffectClass::getDistanceShoot(const Player* spectator, const Creature* attacker, const Position& to,
			bool hasTarget, NetworkMessage &msg) const
{
	if(animationEffect > 0) {
		if(spectator->CanSee(attacker->pos.x, attacker->pos.y, attacker->pos.z) || spectator->CanSee(to.x, to.y, to.z)) {
			msg.AddDistanceShoot(attacker->pos, to, animationEffect);
		}
	}
}

void MagicEffectClass::getArea(const Position& rcenterpos, MagicAreaVec& list) const
{
	list.push_back(rcenterpos);
}

MagicEffectItem* MagicEffectClass::getMagicItem(const Creature* attacker, bool isPz, bool isBlocking) const
{
	return NULL;
}

bool MagicEffectClass::canCast(bool isBlocking, bool hasCreature) const
{
	return !isBlocking;
}

//Need a target
MagicEffectTargetClass::MagicEffectTargetClass()
{
	//
}

void MagicEffectTargetClass::getMagicEffect(const Player* spectator, const Creature* attacker, const Position& pos,
			bool hasTarget, int damage, bool isPz, bool isBlocking, NetworkMessage &msg) const
{
	if(hasTarget) {
		//default
		MagicEffectClass::getMagicEffect(spectator, attacker, pos, hasTarget, damage, isPz, isBlocking, msg);
	}
	else {
		if(attacker) {
			if(spectator->CanSee(attacker->pos.x, attacker->pos.y, attacker->pos.z)) {
				msg.AddMagicEffect(attacker->pos, NM_ME_PUFF);
			}
		}
	}
}

void MagicEffectTargetClass::getDistanceShoot(const Player* spectator, const Creature* attacker, const Position& to,
			bool hasTarget, NetworkMessage &msg) const
{
	if(animationEffect > 0 && hasTarget) {
		if(spectator->CanSee(attacker->pos.x, attacker->pos.y, attacker->pos.z) || spectator->CanSee(to.x, to.y, to.z)) {
			msg.AddDistanceShoot(attacker->pos, to, animationEffect);
		}
	}
}

MagicDamageContainer::MagicDamageContainer(enum MagicDamageType md) 
: magictype(md)
{

}

MagicDamageContainer::MagicDamageContainer(MagicDamageType md, MagicDamageVec list)
: magictype(md)
{
	this->assign(list.begin(), list.end());
}


MagicEffectTargetExClass::MagicEffectTargetExClass(MagicDamageType md, const MagicDamageVec& dmglist)
: dmgContainer(md, dmglist)
{

}

int MagicEffectTargetExClass::getDamage(Creature *target, const Creature *attacker /*= NULL*/) const
{
	if((!offensive || (target != attacker)) && target->access == 0) {
		target->addMagicDamage(dmgContainer, true);

		int damage = (int)random_range(minDamage, maxDamage);

		if(!offensive)
			damage = -damage;

		return damage;
	}

	return 0;
}

//Burning/poisoned/energized
MagicEffectTargetMagicDamageClass::MagicEffectTargetMagicDamageClass(const unsigned long creatureid)
: ownerid(creatureid)
{
	//
}

int MagicEffectTargetMagicDamageClass::getDamage(Creature *target, const Creature *attacker /*= NULL*/) const
{
	if(target->access == 0) {
		int damage = (int)random_range(minDamage, maxDamage);

		if(!offensive)
			damage = -damage;

		return damage;
	}

	return 0;
}


//magic wall, wild growth
MagicEffectTargetGroundClass::MagicEffectTargetGroundClass(MagicEffectItem* item)
{
	magicItem = item;
}

MagicEffectTargetGroundClass::~MagicEffectTargetGroundClass()
{
	if(magicItem) {
		delete magicItem;
		magicItem = NULL;
	}
}

MagicEffectItem* MagicEffectTargetGroundClass::getMagicItem(const Creature* attacker, bool isPz, bool isBlocking) const
{
	if(!isBlocking && (!isPz || attacker->access != 0)) {
		return magicItem;
	}
	else
		return NULL;
}

bool MagicEffectTargetGroundClass::canCast(bool isBlocking, bool hasCreature) const
{
	if(magicItem) {
		if(magicItem->isBlocking() && (isBlocking || hasCreature)) {
			return false;
		}
	}

	return true;
}

void MagicEffectTargetGroundClass::getMagicEffect(const Player* spectator, const Creature* attacker, const Position& pos,
			bool hasTarget, int damage, bool isPz, bool isBlocking, NetworkMessage &msg) const
{
	const Player* player = dynamic_cast<const Player*>(attacker);

	if(isBlocking || hasTarget) {
		if(hasTarget) {
			if(player && player == spectator) {
				msg.AddTextMessage(MSG_SMALLINFO, "There is not enough room.");
			}

			msg.AddMagicEffect(player->pos, NM_ME_PUFF);
		}
		else if(player && player == spectator) {
			msg.AddTextMessage(MSG_SMALLINFO, "You cannot throw there.");
		}
	}
}

void MagicEffectTargetGroundClass::getDistanceShoot(const Player* spectator, const Creature* attacker, const Position& to,
			bool hasTarget, NetworkMessage &msg) const
{
	if(hasTarget && animationEffect > 0) {
		if(spectator->CanSee(attacker->pos.x, attacker->pos.y, attacker->pos.z) || spectator->CanSee(to.x, to.y, to.z)) {
			msg.AddDistanceShoot(attacker->pos, to, animationEffect);
		}
	}
}

MagicEffectAreaClass::MagicEffectAreaClass()
{
	direction = 0;
	areaEffect = 0xFF;
	//memset(area, 0, sizeof(area));
}

void MagicEffectAreaClass::getMagicEffect(const Player* spectator, const Creature* attacker, const Position& pos,
			bool hasTarget, int damage, bool isPz, bool isBlocking, NetworkMessage &msg) const
{
	if(hasTarget) {
		//default
		MagicEffectClass::getMagicEffect(spectator, attacker, pos, hasTarget, damage, isPz, isBlocking, msg);
	}
	else {
		if(!isBlocking && areaEffect != 0xFF && (attacker->access != 0 || !isPz)) {
			if(spectator->CanSee(pos.x, pos.y, pos.z)) {
				msg.AddMagicEffect(pos, areaEffect);
			}
		}
	}
}

void MagicEffectAreaClass::getArea(const Position& rcenterpos, MagicAreaVec& list) const
{
	int rows = (int)areaVec.size();
	int cols = (int)(rows > 0 ? areaVec[0].size() : 0);

	if(rows < 3 || cols < 3)
		return;

	Position tpos = rcenterpos;
	tpos.x -= (cols - 1) / 2; //8;
	tpos.y -= (rows - 1) / 2; //6;

	for(int y = 0; y < rows /*14*/; y++) {
		for(int x = 0; x < cols /*18*/; x++) {
			//if(area[y][x] == direction) {
			if(areaVec[y][x] == direction) {

				list.push_back(tpos);
			}
			tpos.x += 1;
		}
		
		tpos.x -= cols /*18*/;
		tpos.y += 1;
	}
}

MagicEffectAreaExClass::MagicEffectAreaExClass(MagicDamageType md, const MagicDamageVec& dmglist)
: dmgContainer(md, dmglist)
{

}

int MagicEffectAreaExClass::getDamage(Creature *target, const Creature *attacker /*= NULL*/) const
{
	if(target->access == 0) {
		target->addMagicDamage(dmgContainer, true);
		int damage = (int)random_range(minDamage, maxDamage);

		if(!offensive)
			damage = -damage;

		return damage;
	}

	return 0;
}

MagicEffectAreaGroundClass::MagicEffectAreaGroundClass(MagicEffectItem* item)
{
	magicItem = item;
}

MagicEffectAreaGroundClass::~MagicEffectAreaGroundClass()
{
	if(magicItem) {
		delete magicItem;
		magicItem = NULL;
	}
}

int MagicEffectAreaGroundClass::getDamage(Creature *target, const Creature *attacker /*= NULL*/) const
{
	if(target->access == 0) {
		if(magicItem) {
			return magicItem->getDamage(target, attacker);
		}
		else
			return 0;

		/*
		c->addMagicDamage(dmgContainer, true);
		int damage = (int)random_range(minDamage, maxDamage);

		if(!offensive)
			damage = -damage;

		return damage;
		*/
	}

	return 0;
}

MagicEffectItem* MagicEffectAreaGroundClass::getMagicItem(const Creature* attacker, bool isPz, bool isBlocking) const
{
	if(!isBlocking && (!isPz || attacker->access != 0)) {
		return magicItem;
	}
	else
		return NULL;
}

//Constructor for solid objects.
MagicEffectItem::MagicEffectItem(const damageMapClass& dmgmap)
: dmgContainer(magicNone)
{
	this->dmgMap = dmgmap;

	unsigned short type = 0;
	damageMapClass::iterator dm = dmgMap.begin();
	if(dm != dmgMap.end()) {
		type = dm->first;
	}

	setID(type);
}

//Fire/Poison/Energy fields.
MagicEffectItem::MagicEffectItem(MagicDamageType md, const damageMapClass& dmgmap)
: dmgContainer(md)
{
	this->dmgMap = dmgmap;

	unsigned short type = 0;
	damageMapClass::iterator dm = dmgMap.begin();
	if(dm != dmgMap.end()) {
		type = dm->first;
	}

	setID(type);
	buildDamageList();
}

bool MagicEffectItem::transform(const MagicEffectItem *rhs)
{
	this->dmgMap = rhs->dmgMap;
	setID(rhs->getID());

	buildDamageList();
	return true;
}

long MagicEffectItem::getDecayTime()
{
	damageMapClass::iterator dm = dmgMap.find(getID());
	if(dm != dmgMap.end()) {
		return dm->second.first;
	}
	
	return 0;
}

bool MagicEffectItem::transform()
{
	damageMapClass::iterator dm = dmgMap.find(getID());
	if(dm != dmgMap.end()) {

		//get next id to transform to
		dm++;

		if(dm != dmgMap.end()) {
			setID(dm->first);
			buildDamageList();
			return true;
		}
	}

	return false;
}

void MagicEffectItem::buildDamageList()
{
	dmgContainer.clear();

	damageMapClass::iterator dm = dmgMap.find(getID());
	if(dm != dmgMap.end()) {
		while(dm != dmgMap.end()) {
			for(MagicDamageVec::iterator di = dm->second.second.begin(); di != dm->second.second.end(); di++) {

				dmgContainer.push_back(*di);
			}

			dm++;
		}
	}
}

int MagicEffectItem::getDamage(Creature *target, const Creature *attacker /*= NULL*/) const
{
	if(target->access == 0) {
		target->addMagicDamage(dmgContainer, true);
		const MagicEffectTargetMagicDamageClass *magicTargetDmg = getMagicDamageEffect();
		
		if(magicTargetDmg)
			return magicTargetDmg->getDamage(target, attacker);
		else
			return 0;
	}

	return 0;
}

const MagicEffectTargetMagicDamageClass* MagicEffectItem::getMagicDamageEffect() const
{
	if(dmgContainer.size() > 0) {
		return &dmgContainer[0].second;
	}

	return NULL;
}


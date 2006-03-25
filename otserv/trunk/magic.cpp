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
#include "game.h"
#include "creature.h"
#include "monster.h"
#include "player.h"

#include "magic.h"

extern Game g_game;

MagicEffectClass::MagicEffectClass()
{
	animationColor = 0;
	animationEffect = 0;
	hitEffect = 0xFF;
	damageEffect = 0xFF;
	minDamage = 0;
	maxDamage = 0;
	offensive = false;
	drawblood = false;
	manaCost = 0;
	attackType = ATTACK_NONE;
}
 
bool MagicEffectClass::isIndirect() const
{
	return false;
}

bool MagicEffectClass::causeExhaustion(bool hasTarget) const
{
	return hasTarget;
}

int MagicEffectClass::getDamage(Creature *target, const Creature *attacker /*= NULL*/) const
{
	if((attackType != ATTACK_NONE) && (target->getImmunities() & attackType) == attackType)
		return 0;

	if((!offensive || (target != attacker)) && target->access == 0) {
		int damage = (int)random_range(minDamage, maxDamage);

		if(!offensive)
			damage = -damage;
		else {
			if(attacker && attacker->access != 0)
				return damage;

			const Monster *monster = dynamic_cast<const Monster*>(attacker);
			if(monster) {
				//no reductation of damage if attack came from a monster
				return damage;
			}

			const Player *targetPlayer = dynamic_cast<const Player*>(target);

			if(targetPlayer) {
				damage = (int)floor(damage / 2.0);
			}
		}

		return damage;
	}

	return 0;
}

void MagicEffectClass::getMagicEffect(Player* spectator, const Creature* attacker, const Creature* target,
	const Position& pos, /*bool hasTarget,*/ int damage, bool isPz, bool isBlocking) const
{
	if(!isBlocking && target != NULL /*hasTarget*/) {
		if(spectator->CanSee(pos.x, pos.y, pos.z)) {
			if(damageEffect != 0xFF) {
				if(!offensive || !(g_game.getWorldType() == WORLD_TYPE_NO_PVP && dynamic_cast<const Player*>(attacker) &&
					dynamic_cast<const Player*>(target) && target->access == 0 && attacker->access == 0) || target->access != 0) {
						if(offensive && (target->getImmunities() & attackType) == attackType) {
							spectator->sendMagicEffect(pos, NM_ME_BLOCKHIT);
						}
						else {
							spectator->sendMagicEffect(pos, damageEffect);
						}
					}
			}

			if(hitEffect != 0xFF)
				spectator->sendMagicEffect(pos, hitEffect);
		}
	}
}

void MagicEffectClass::getDistanceShoot(Player* spectator, const Creature* attacker, const Position& to,
			bool hasTarget) const
{
	if(animationEffect > 0) {
		if(spectator->CanSee(attacker->getPosition().x, attacker->getPosition().y, attacker->getPosition().z) ||
			spectator->CanSee(to.x, to.y, to.z)) {
			spectator->sendDistanceShoot(attacker->getPosition(), to, animationEffect);
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

void MagicEffectClass::FailedToCast(Player* spectator, const Creature* attacker,
		bool isBlocking, bool hasTarget) const
{
	if(!hasTarget && attacker) {
		if(attacker == spectator) {
			spectator->sendTextMessage(MSG_STATUS_SMALL, "You can only use this rune on creatures.");
		}
		spectator->sendMagicEffect(attacker->getPosition(), NM_ME_PUFF);
	}
}

//Need a target
MagicEffectTargetClass::MagicEffectTargetClass()
{
	//
}
void MagicEffectTargetClass::getMagicEffect(Player* spectator, const Creature* attacker, const Creature* target,
	const Position& pos, int damage, bool isPz, bool isBlocking) const
{
	if(target != NULL) {
		//default
		MagicEffectClass::getMagicEffect(spectator, attacker, target, pos, /*hasTarget,*/ damage, isPz, isBlocking);
	}
	else {
		if(attacker) {
			if(spectator->CanSee(attacker->getPosition().x, attacker->getPosition().y, attacker->getPosition().z)) {
				spectator->sendMagicEffect(attacker->getPosition(), NM_ME_PUFF);
			}
		}
	}
}

void MagicEffectTargetClass::getDistanceShoot(Player* spectator, const Creature* attacker, const Position& to,
			bool hasTarget) const
{
	if(animationEffect > 0 && hasTarget) {
		if(spectator->CanSee(attacker->getPosition().x, attacker->getPosition().y, attacker->getPosition().z) || spectator->CanSee(to.x, to.y, to.z)) {
			spectator->sendDistanceShoot(attacker->getPosition(), to, animationEffect);
		}
	}
}

MagicEffectTargetExClass::MagicEffectTargetExClass(const ConditionVec& dmglist) :
condition(dmglist)
{

}

int MagicEffectTargetExClass::getDamage(Creature *target, const Creature *attacker /*= NULL*/) const
{
	if((attackType != ATTACK_NONE) && (target->getImmunities() & attackType) == attackType)
		return 0;

	if((!offensive || (target != attacker)) && target->access == 0) {
		
		//target->addMagicDamage(dmgContainer, true);

		bool refresh = true;
		for(ConditionVec::const_iterator condIt = condition.begin(); condIt != condition.end(); ++condIt) {
			/*if(condIt == condition.begin()) //skip first
				continue;*/
			
			if((condIt->getCondition()->attackType != ATTACK_NONE) &&
				(target->getImmunities() & condIt->getCondition()->attackType) != condIt->getCondition()->attackType) {
				target->addCondition(*condIt, refresh);
				refresh = false; //only set refresh flag on first "new event"
			}
		}

		int damage = (int)random_range(minDamage, maxDamage);

		if(!offensive)
			damage = -damage;

		return damage;
	}

	return 0;
}

//Burning/poisoned/energized
MagicEffectTargetCreatureCondition::MagicEffectTargetCreatureCondition(const unsigned long creatureid)
: ownerid(creatureid)
{
	//
}

int MagicEffectTargetCreatureCondition::getDamage(Creature *target, const Creature *attacker /*= NULL*/) const
{
	if((attackType != ATTACK_NONE) && (target->getImmunities() & attackType) == attackType)
		return 0;

	if(target->access == 0) {
		int damage = (int)random_range(minDamage, maxDamage);

		if(!offensive)
			damage = -damage;

		return damage;
	}

	return 0;
}

void MagicEffectTargetCreatureCondition::getMagicEffect(Player* spectator, const Creature* attacker, const Creature* target,
	const Position& pos, int damage, bool isPz, bool isBlocking) const
{
	if(target != NULL) {
		//default
		MagicEffectClass::getMagicEffect(spectator, attacker, target, pos, /*hasTarget,*/ damage, isPz, isBlocking);
	}
	else {
		if(spectator->CanSee(pos.x, pos.y, pos.z)) {
			spectator->sendMagicEffect(pos, NM_ME_PUFF);
		}
	}

	//MagicEffectTargetClass::getMagicEffect(spectator, attacker, pos, hasTarget, damage, isPz, isBlocking, msg);
}


//magic wall, wild growth
MagicEffectTargetGroundClass::MagicEffectTargetGroundClass(MagicEffectItem* item)
{
	magicItem = item;
}

MagicEffectTargetGroundClass::~MagicEffectTargetGroundClass()
{
	if(magicItem) {
		magicItem->releaseThing2();
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

void MagicEffectTargetGroundClass::FailedToCast(Player* spectator, const Creature* attacker,
		bool isBlocking, bool hasTarget) const
{
	const Player* player = dynamic_cast<const Player*>(attacker);

	if(isBlocking || hasTarget) {
		if(hasTarget) {
			if(player && player == spectator) {
				spectator->sendTextMessage(MSG_STATUS_SMALL, "There is not enough room.");
			}
			spectator->sendMagicEffect(player->getPosition(), NM_ME_PUFF);
		}
		else if(player && player == spectator) {
			spectator->sendTextMessage(MSG_STATUS_SMALL, "You cannot throw there.");
		}
	}
}

void MagicEffectTargetGroundClass::getMagicEffect(Player* spectator, const Creature* attacker, const Creature* target,
		const Position& pos, int damage, bool isPz, bool isBlocking) const
{
	//Default: nothing
}

void MagicEffectTargetGroundClass::getDistanceShoot(Player* spectator, const Creature* attacker, const Position& to,
			bool hasTarget) const
{
	if(!hasTarget && animationEffect > 0) {
		if(spectator->CanSee(attacker->getPosition().x, attacker->getPosition().y, attacker->getPosition().z) || spectator->CanSee(to.x, to.y, to.z)) {
			spectator->sendDistanceShoot(attacker->getPosition(), to, animationEffect);
		}
	}
}

MagicEffectAreaClass::MagicEffectAreaClass()
{
	direction = 0;
	areaEffect = 0xFF;
}

void MagicEffectAreaClass::getMagicEffect(Player* spectator, const Creature* attacker, const Creature* target,
	const Position& pos, int damage, bool isPz, bool isBlocking) const
{
	if(target != NULL) {
		//default
		MagicEffectClass::getMagicEffect(spectator, attacker, target, pos, /*hasTarget,*/ damage, isPz, isBlocking);
	}
	else {
		if(!isBlocking && areaEffect != 0xFF && (attacker->access != 0 || !isPz)) {
			if(spectator->CanSee(pos.x, pos.y, pos.z)) {
				spectator->sendMagicEffect(pos, areaEffect);
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

MagicEffectAreaExClass::MagicEffectAreaExClass(const ConditionVec& dmglist) :
condition(dmglist)
{
	//
}

int MagicEffectAreaExClass::getDamage(Creature *target, const Creature *attacker /*= NULL*/) const
{
	if((attackType != ATTACK_NONE) && (target->getImmunities() & attackType) == attackType)
		return 0;

	if(target->access == 0) {
		//target->addMagicDamage(dmgContainer, true);

		bool refresh = true;
		for(ConditionVec::const_iterator condIt = condition.begin(); condIt != condition.end(); ++condIt) {
			if(condIt == condition.begin()) //skip first
				continue;

			if((condIt->getCondition()->attackType != ATTACK_NONE) &&
				(target->getImmunities() & condIt->getCondition()->attackType) != condIt->getCondition()->attackType) {
				target->addCondition(*condIt, refresh);
				refresh = false; //only set refresh flag on first "new event"
			}
		}

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
		//delete magicItem;
		magicItem->releaseThing2();
		magicItem = NULL;
	}
}

int MagicEffectAreaGroundClass::getDamage(Creature *target, const Creature *attacker /*= NULL*/) const
{
	if((attackType != ATTACK_NONE) && (target->getImmunities() & attackType) == attackType)
		return 0;

	if(target->access == 0) {
		if(magicItem) {
			return magicItem->getDamage(target, attacker);
		}
		else
			return 0;
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
MagicEffectItem::MagicEffectItem(const TransformMap& transformMap)
{
	this->transformMap = transformMap;
	unsigned short type = 0;
	TransformMap::const_iterator dm = transformMap.begin();
	if(dm != transformMap.end()) {
		type = dm->first;		
	}	
	setID(type);
	buildCondition();
}

bool MagicEffectItem::transform(const MagicEffectItem *rhs)
{
	this->transformMap = rhs->transformMap;
	setID(rhs->getID());
	buildCondition();
	return true;
}

long MagicEffectItem::getDecayTime()
{
	TransformMap::iterator dm = transformMap.find(getID());
	
	if(dm != transformMap.end()) {
		return dm->second.first;
	}
	
	return Item::getDecayTime();
}

void MagicEffectItem::setID(unsigned short newid)
{
	Item::setID(newid);
	buildCondition();
}

void MagicEffectItem::buildCondition()
{
	condition.clear();

	TransformMap::iterator dm = transformMap.find(getID());
	if(dm != transformMap.end()) {
		while(dm != transformMap.end()) {
			for(ConditionVec::iterator di = dm->second.second.begin(); di != dm->second.second.end(); di++) {

				condition.push_back(*di);
			}

			dm++;
		}
	}
}

int MagicEffectItem::getDamage(Creature *target, const Creature *attacker /*= NULL*/) const
{

	if(target->access == 0) {

		bool refresh = true;
		for(ConditionVec::const_iterator condIt = condition.begin(); condIt != condition.end(); ++condIt) {
			if(condIt == condition.begin()) //skip first
				continue;

			if((condIt->getCondition()->attackType != ATTACK_NONE) &&
				(target->getImmunities() & condIt->getCondition()->attackType) != condIt->getCondition()->attackType) {
				target->addCondition(*condIt, refresh);
				refresh = false; //only set refresh flag on first "new event"
			}
		}

		const MagicEffectTargetCreatureCondition *magicTargetCondition = getCondition();
		
		if(magicTargetCondition)
			return magicTargetCondition->getDamage(target, attacker);
		else
			return 0;
	}

	return 0;
}

const MagicEffectTargetCreatureCondition* MagicEffectItem::getCondition() const
{
	if(condition.size() > 0) {
		return condition[0].getCondition();
	}

	return NULL;
}


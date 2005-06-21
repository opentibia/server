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
#include "spells.h"
#include "creature.h"
#include "player.h"
#include "tile.h"

#include "magic.h"


MagicSpell::MagicSpell(Spell *ispell) :
 spell(ispell)
{
	//
}

MagicSpell::~MagicSpell()
{
	//
}

ConjureItemSpell::ConjureItemSpell(Spell *ispell, const std::vector<unsigned short>& iitems, int icount, unsigned char imagicEffect) : 
 MagicSpell(ispell), items(iitems), count(icount), magicEffect(imagicEffect)
{
	//
}

bool ConjureItemSpell::doCastSpell(Creature* spellCastCreature, const Position& pos, const std::string& var) const
{
	int n = random_range(0, items.size() - 1);
	unsigned short itemtype = items[n];

	Item* newItem = NULL;
	newItem = Item::CreateItem(itemtype, count);

	Player *spellCastPlayer = dynamic_cast<Player*>(spellCastCreature);
	if(spellCastPlayer && spellCastPlayer->addItem(newItem)) {
		spell->game->addMagicEffect(spellCastPlayer->pos, magicEffect);
		return true;
	}

	//add item on the ground
	spell->game->addThing(NULL, pos, newItem);
	spell->game->addMagicEffect(pos, magicEffect);

	return true;
}
//---------------------------------------------------------------
ChangeSpeedSpell::ChangeSpeedSpell(Spell *ispell, int itime, int iaddspeed, unsigned char imagicEffect) :
 MagicSpell(ispell), time(itime), addspeed(iaddspeed), magicEffect(imagicEffect)
{
	//
}

bool ChangeSpeedSpell::doCastSpell(Creature* spellCastCreature, const Position& pos, const std::string& var) const
{
	if(addspeed == 0) {
		int addspeed = spell->script->onUse(spellCastCreature, spellCastCreature, var);
	}
	
	if(addspeed > 0) {
		spell->game->addCondition(spellCastCreature, CONDITION_HASTE, time, addspeed);
	}
	else
		spell->game->addCondition(spellCastCreature, CONDITION_SLOWED, time, addspeed);

	return true;
}
//---------------------------------------------------------------
LightSpell::LightSpell(Spell *ispell, int itime, unsigned char ilightlevel, unsigned char imagicEffect) :
 MagicSpell(ispell), time(itime), lightlevel(ilightlevel), magicEffect(imagicEffect)
{
	//
}

bool LightSpell::doCastSpell(Creature* spellCastCreature, const Position& pos, const std::string& var) const
{
	Player* spellCastPlayer = dynamic_cast<Player*>(spellCastCreature);
	if(spellCastPlayer) {
		spell->game->creatureChangeLight(spellCastPlayer, time, lightlevel);
		return 1;
	}

	return 0;
}

//---------------------------------------------------------------
MagicAttackSpell::MagicAttackSpell(Spell *ispell, attacktype_t iattackType, unsigned char idistanceEffect,
	const AreaVector& ivec, bool ineedDirection, amuEffect_t iamuInfo) : 
 MagicSpell(ispell), attackType(iattackType), needDirection(ineedDirection), amuInfo(iamuInfo)
{
	setArea(ivec);
}

bool MagicAttackSpell::doCastSpell(Creature* spellCastCreature, const Position& pos, const std::string& var) const
{
	CreatureVector targetlist;
	CreatureVector::iterator cit;

	unsigned char direction = 1;
	if(needDirection){
		switch(spellCastCreature->getDirection()) {
			case NORTH: direction = 1; break;
			case WEST: direction = 2; break;
			case EAST: direction = 3; break;
			case SOUTH: direction = 4; break;
		}
	}

	std::vector<Position> poslist;
	getArea(pos, poslist, direction);

	for(std::vector<Position>::const_iterator it = poslist.begin(); it != poslist.end(); ++it) {
		Tile *tile = spell->game->getTile(it->x, it->y, it->z);
		if(tile && ((spellCastCreature->access != 0 || !tile->isPz()) ) ) {
			if(spell->game->canThrowTo(spellCastCreature->pos, *it, false, true)) {

				if(tile->creatures.empty()) {
					spell->game->addMagicEffect(*it, amuInfo.areaEffect);
				}
				else {
					for(cit = tile->creatures.begin(); cit != tile->creatures.end(); ++cit) {
						int damage = spell->script->onUse(spellCastCreature, *cit, var);
						spell->game->internalCreatureAttackCreature(spellCastCreature, *cit, attackType, amuInfo, damage);
						targetlist.push_back(*cit);
					}
				}
			}
		}
	}

	for(CreatureVector::iterator cit = targetlist.begin(); cit != targetlist.end(); ++cit) {
		spell->game->internalCreatureAttackedCreature(spellCastCreature, *cit);
	}

	return true;
}

void MagicAttackSpell::getArea(const Position& centerpos, std::vector<Position>& vec, unsigned char direction) const
{
	int rows = (int)areaVec.size();
	int cols = (int)(rows > 0 ? areaVec[0].size() : 0);

	if(rows < 3 || cols < 3 && !(rows == 1 && cols == 1))
		return;

	Position tpos = centerpos;
	tpos.x -= (cols - 1) / 2; //8;
	tpos.y -= (rows - 1) / 2; //6;

	for(int y = 0; y < rows; y++) {
		for(int x = 0; x < cols; x++) {
			if(areaVec[y][x] == direction) {
				vec.push_back(tpos);
			}

			tpos.x += 1;
		}
		
		tpos.x -= cols;
		tpos.y += 1;
	}
}

/*
MagicAttackSpell::MagicAttackSpell()
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
*/

/*
bool MagicEffectClass::isIndirect() const
{
	return false;
}
*/

/*
bool MagicEffectClass::causeExhaustion(bool hasTarget) const
{
	return hasTarget;
}
*/

/*
void MagicEffectClass::getMagicEffect(Player* spectator, Creature *spellCastCreature, Creature *targetCreature) const
{
	//
}
*/

/*
void MagicEffectClass::getMagicEffect(Player* spectator, Creature *spellCastCreature, const Position& pos) const
{
	if(spectator->CanSee(pos.x, pos.y, pos.z)) {
		spectator->sendMagicEffect(pos, areaEffect);
}
*/

/*
int MagicEffectClass::getDamage(Creature *target, const Creature *attacker = NULL) const
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
*/

/*
void MagicEffectClass::getMagicEffect(Player* spectator, const Creature* attacker, const Creature* target,
	const Position& pos, int damage, bool isPz, bool isBlocking) const
{
	if(!isBlocking && target != NULL) {
		if(spectator->CanSee(pos.x, pos.y, pos.z)) {
			if(target->access == 0) {
				if(damageEffect != 0xFF) {
					if(offensive && (target->getImmunities() & attackType) == attackType) {
						spectator->sendMagicEffect(pos, NM_ME_BLOCKHIT);
					}
					else {
						spectator->sendMagicEffect(pos, damageEffect);
					}
				}

				if(hitEffect != 0xFF)
					spectator->sendMagicEffect(pos, hitEffect);
			}
			else
				spectator->sendMagicEffect(pos, NM_ME_PUFF);
		}
	}
}
*/

/*
void MagicEffectClass::getDistanceShoot(Player* spectator, const Creature* attacker, const Position& to,
			bool hasTarget) const
{
	if(animationEffect > 0) {
		if(spectator->CanSee(attacker->pos.x, attacker->pos.y, attacker->pos.z) || spectator->CanSee(to.x, to.y, to.z)) {
			spectator->sendDistanceShoot(attacker->pos, to, animationEffect);
		}
	}
}
*/

/*
void MagicEffectClass::getArea(const Position& rcenterpos, MagicAreaVec& list) const
{
	list.push_back(rcenterpos);
}
*/

/*
MagicEffectItem* MagicEffectClass::getMagicItem(const Creature* attacker, bool isPz, bool isBlocking) const
{
	return NULL;
}
*/

/*
void MagicEffectClass::FailedToCast(Player* spectator, const Creature* attacker,
		bool isBlocking, bool hasTarget) const
{
	if(!hasTarget && attacker) {
		if(attacker == spectator) {
			spectator->sendTextMessage(MSG_SMALLINFO, "You can only use this rune on creatures.");
		}
		spectator->sendMagicEffect(attacker->pos, NM_ME_PUFF);
	}
}
*/

/*
//Need a target
MagicEffectTargetClass::MagicEffectTargetClass()
{
	//
}
*/

/*
void MagicEffectTargetClass::getMagicEffect(Player* spectator, const Creature* attacker, const Creature* target,
	const Position& pos, int damage, bool isPz, bool isBlocking) const
{
	if(target != NULL) {
		//default
		MagicEffectClass::getMagicEffect(spectator, attacker, target, pos, damage, isPz, isBlocking);
	}
	else {
		if(attacker) {
			if(spectator->CanSee(attacker->pos.x, attacker->pos.y, attacker->pos.z)) {
				spectator->sendMagicEffect(attacker->pos, NM_ME_PUFF);
			}
		}
	}
}
*/

/*
void MagicEffectTargetClass::getDistanceShoot(Player* spectator, const Creature* attacker, const Position& to,
			bool hasTarget) const
{
	if(animationEffect > 0 && hasTarget) {
		if(spectator->CanSee(attacker->pos.x, attacker->pos.y, attacker->pos.z) || spectator->CanSee(to.x, to.y, to.z)) {
			spectator->sendDistanceShoot(attacker->pos, to, animationEffect);
			//msg.AddDistanceShoot(attacker->pos, to, animationEffect);
		}
	}
}
*/

/*
MagicEffectTargetExClass::MagicEffectTargetExClass(const ConditionVec& dmglist) :
condition(dmglist)
{

}
*/

/*
int MagicEffectTargetExClass::getDamage(Creature *target, const Creature *attacker = NULL) const
{
	if((attackType != ATTACK_NONE) && (target->getImmunities() & attackType) == attackType)
		return 0;

	if((!offensive || (target != attacker)) && target->access == 0) {
		
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
*/

/*
//Burning/poisoned/energized
MagicEffectTargetCreatureCondition::MagicEffectTargetCreatureCondition(const unsigned long creatureid)
: ownerid(creatureid)
{
	//
}
*/

/*
int MagicEffectTargetCreatureCondition::getDamage(Creature *target, const Creature *attacker = NULL) const
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
*/

/*
void MagicEffectTargetCreatureCondition::getMagicEffect(Player* spectator, const Creature* attacker, const Creature* target,
	const Position& pos, int damage, bool isPz, bool isBlocking) const
{
	if(target != NULL) {
		//default
		MagicEffectClass::getMagicEffect(spectator, attacker, target, pos, damage, isPz, isBlocking);
	}
	else {
		if(spectator->CanSee(pos.x, pos.y, pos.z)) {
			spectator->sendMagicEffect(pos, NM_ME_PUFF);
			//msg.AddMagicEffect(pos, NM_ME_PUFF);
		}
	}

	//MagicEffectTargetClass::getMagicEffect(spectator, attacker, pos, hasTarget, damage, isPz, isBlocking, msg);
}
*/

/*
//magic wall, wild growth
MagicEffectTargetGroundClass::MagicEffectTargetGroundClass(MagicEffectItem* item)
{
	magicItem = item;
}
*/

/*
MagicEffectTargetGroundClass::~MagicEffectTargetGroundClass()
{
	if(magicItem) {
		//delete magicItem;
		magicItem->releaseThing();
		magicItem = NULL;
	}
}
*/

/*
MagicEffectItem* MagicEffectTargetGroundClass::getMagicItem(const Creature* attacker, bool isPz, bool isBlocking) const
{
	if(!isBlocking && (!isPz || attacker->access != 0)) {
		return magicItem;
	}
	else
		return NULL;
}
*/

/*
bool MagicEffectTargetGroundClass::canCast(bool isBlocking, bool hasCreature) const
{
	if(magicItem) {
		if(magicItem->isBlocking() && (isBlocking || hasCreature)) {
			return false;
		}
	}

	return true;
}
*/

/*
void MagicEffectTargetGroundClass::FailedToCast(Player* spectator, const Creature* attacker,
		bool isBlocking, bool hasTarget) const
{
	const Player* player = dynamic_cast<const Player*>(attacker);

	if(isBlocking || hasTarget) {
		if(hasTarget) {
			if(player && player == spectator) {
				spectator->sendTextMessage(MSG_SMALLINFO, "There is not enough room.");
				//msg.AddTextMessage(MSG_SMALLINFO, "There is not enough room.");
			}
			spectator->sendMagicEffect(player->pos, NM_ME_PUFF);
			//msg.AddMagicEffect(player->pos, NM_ME_PUFF);
		}
		else if(player && player == spectator) {
			spectator->sendTextMessage(MSG_SMALLINFO, "You cannot throw there.");
			//msg.AddTextMessage(MSG_SMALLINFO, "You cannot throw there.");
		}
	}
}
*/

/*
void MagicEffectTargetGroundClass::getMagicEffect(Player* spectator, const Creature* attacker, const Creature* target,
		const Position& pos, int damage, bool isPz, bool isBlocking) const
{
	//Default: nothing
}
*/

/*
void MagicEffectTargetGroundClass::getDistanceShoot(Player* spectator, const Creature* attacker, const Position& to,
			bool hasTarget) const
{
	if(!hasTarget && animationEffect > 0) {
		if(spectator->CanSee(attacker->pos.x, attacker->pos.y, attacker->pos.z) || spectator->CanSee(to.x, to.y, to.z)) {
			spectator->sendDistanceShoot(attacker->pos, to, animationEffect);
			//msg.AddDistanceShoot(attacker->pos, to, animationEffect);
		}
	}
}
*/

/*
MagicEffectAreaClass::MagicEffectAreaClass()
{
	direction = 0;
	areaEffect = 0xFF;
}
*/

/*
void MagicEffectAreaClass::getMagicEffect(Player* spectator, const Creature* attacker, const Creature* target,
	const Position& pos, int damage, bool isPz, bool isBlocking) const
{
	if(target != NULL) {
		//default
		MagicEffectClass::getMagicEffect(spectator, attacker, target, pos, damage, isPz, isBlocking);
	}
	else {
		if(!isBlocking && areaEffect != 0xFF && (attacker->access != 0 || !isPz)) {
			if(spectator->CanSee(pos.x, pos.y, pos.z)) {
				spectator->sendMagicEffect(pos, areaEffect);
				//msg.AddMagicEffect(pos, areaEffect);
			}
		}
	}
}
*/

/*
void MagicEffectAreaClass::getArea(const Position& rcenterpos, MagicAreaVec& list) const
{
	int rows = (int)areaVec.size();
	int cols = (int)(rows > 0 ? areaVec[0].size() : 0);

	if(rows < 3 || cols < 3)
		return;

	Position tpos = rcenterpos;
	tpos.x -= (cols - 1) / 2; //8;
	tpos.y -= (rows - 1) / 2; //6;

	for(int y = 0; y < rows; y++) {
		for(int x = 0; x < cols; x++) {
			//if(area[y][x] == direction) {
			if(areaVec[y][x] == direction) {

				list.push_back(tpos);
			}
			tpos.x += 1;
		}
		
		tpos.x -= cols;
		tpos.y += 1;
	}
}
*/

/*
MagicEffectAreaExClass::MagicEffectAreaExClass(const ConditionVec& dmglist) :
condition(dmglist)
{
	//
}
*/

/*
int MagicEffectAreaExClass::getDamage(Creature *target, const Creature *attacker = NULL) const
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
*/

/*
MagicEffectAreaGroundClass::MagicEffectAreaGroundClass(MagicEffectItem* item)
{
	magicItem = item;
}
*/

/*
MagicEffectAreaGroundClass::~MagicEffectAreaGroundClass()
{
	if(magicItem) {
		//delete magicItem;
		magicItem->releaseThing();
		magicItem = NULL;
	}
}
*/

/*
int MagicEffectAreaGroundClass::getDamage(Creature *target, const Creature *attacker = NULL) const
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
*/

/*
MagicEffectItem* MagicEffectAreaGroundClass::getMagicItem(const Creature* attacker, bool isPz, bool isBlocking) const
{
	if(!isBlocking && (!isPz || attacker->access != 0)) {
		return magicItem;
	}
	else
		return NULL;
}
*/

/*
//Constructor for solid objects.
MagicEffectItem::MagicEffectItem(const TransformMap& transformMap)
{
	this->transformMap = transformMap;
	useCount = 0;
	unsigned short type = 0;
	//decaytime = 0;
	//updateDecay = false;
	TransformMap::const_iterator dm = transformMap.begin();
	if(dm != transformMap.end()) {
		type = dm->first;		
	}	
	setID(type);
	buildCondition();
}
*/

/*
bool MagicEffectItem::transform(const MagicEffectItem *rhs)
{
	this->transformMap = rhs->transformMap;
	setID(rhs->getID());
	if(transformMap.begin() != transformMap.end()){
		//decaytime = OTSYS_TIME() + transformMap.begin()->second.first;
		//updateDecay = true;
	}
	else{
		//decaytime = 0;
		//updateDecay = false;
	}	
	buildCondition();
	return true;
}
*/

/*
long MagicEffectItem::getDecayTime()
{
	TransformMap::iterator dm = transformMap.find(getID());
	//if(!updateDecay){
		if(dm != transformMap.end()) {
			return dm->second.first;
		}
	//}
	else{
		uint64_t ret = decaytime - OTSYS_TIME();
		if(ret < 0){
			ret = 0;
		}
		decaytime = 0;
		updateDecay = false;
		return ret;
	}
	
	return 0;
}
*/

/*
Item* MagicEffectItem::decay()
{
	TransformMap::iterator dm = transformMap.find(getID());
	//if(updateDecay){
	//	return this;
	//}
	if(dm != transformMap.end()) {

		//get next id to transform to
		dm++;

		if(dm != transformMap.end()) {
			setID(dm->first);
			buildCondition();
			return this;
		}
	}

	return NULL;
}
*/

/*
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
*/

/*
int MagicEffectItem::getDamage(Creature *target, const Creature *attacker = NULL) const
{
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

		const MagicEffectTargetCreatureCondition *magicTargetCondition = getCondition();
		
		if(magicTargetCondition)
			return magicTargetCondition->getDamage(target, attacker);
		else
			return 0;
	}

	return 0;
}
*/

/*
const MagicEffectTargetCreatureCondition* MagicEffectItem::getCondition() const
{
	if(condition.size() > 0) {
		return condition[0].getCondition();
	}

	return NULL;
}
*/


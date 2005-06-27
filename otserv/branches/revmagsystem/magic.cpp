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
		//spell->game->addCondition(spellCastCreature, CONDITION_HASTE, time, addspeed);
	}
	else
		//spell->game->addCondition(spellCastCreature, CONDITION_SLOWED, time, addspeed);

	return true;
}
//---------------------------------------------------------------
LightSpell::LightSpell(Spell *ispell, int itime, unsigned char ilightlevel, unsigned char imagicEffect) :
 MagicSpell(ispell), time(itime), lightlevel(ilightlevel), magicEffect(imagicEffect)
{
	//
}

//bool LightSpell::doCastSpell(Creature* spellCastCreature, const Position& pos, const std::string& var) const
bool LightSpell::doCastSpell(Creature* spellCastCreature, Creature* targetCreature) const
{
	Player* spellCastPlayer = dynamic_cast<Player*>(spellCastCreature);
	if(spellCastPlayer) {
		spell->game->creatureChangeLight(spellCastPlayer, time, lightlevel);
		return 1;
	}

	return 0;
}

//---------------------------------------------------------------
MagicTargetSpell::MagicTargetSpell(Spell *ispell, attacktype_t iattackType, unsigned char idistanceEffect, amuEffect_t iamuInfo) : 
 MagicSpell(ispell), attackType(iattackType), distanceEffect(idistanceEffect), amuInfo(iamuInfo)
{
	//
}

bool MagicTargetSpell::doCastSpell(Creature* spellCastCreature, const Position& pos, const std::string& var) const
{
	Player*	spellCastPlayer = dynamic_cast<Player*>(spellCastCreature);
	if(spellCastCreature->access != 0 || spell->game->canThrowTo(spellCastCreature->pos, pos, false, true)) {
		Tile *tile = spell->game->getTile(pos.x, pos.y, pos.z);
		if(tile) {
			if(tile->creatures.empty()) {
				if(spellCastPlayer) {
					spellCastPlayer->sendTextMessage(MSG_SMALLINFO, "You can only use this rune on creatures.");
				}

				spell->game->addMagicEffect(spellCastCreature->pos, NM_ME_PUFF);
				return false;
			}
			else {
				if(distanceEffect != 0xFF) {
					spell->game->addAnimationShoot(spellCastCreature, pos, distanceEffect);
				}

				CreatureVector targetlist;
				CreatureVector::iterator cit;
				for(cit = tile->creatures.begin(); cit != tile->creatures.end(); ++cit) {
					int damage = spell->script->onUse(spellCastCreature, *cit, var);
					//spell->game->internalCreatureAttackCreature(spellCastCreature, *cit, attackType, amuInfo, damage);
					targetlist.push_back(*cit);
				}

				for(CreatureVector::iterator cit = targetlist.begin(); cit != targetlist.end(); ++cit) {
					//spell->game->internalCreatureAttackedCreature(spellCastCreature, *cit);
				}
			}

			return true;
		}
	}

	if(spellCastPlayer) {
		spellCastPlayer->sendCancel("Sorry not possible.");
	}

	return false;
}

bool MagicTargetSpell::doCastSpell(Creature* spellCastCreature, Creature* targetCreature) const
{
	if(spell->game->canThrowTo(spellCastCreature->pos, targetCreature->pos, false, true)) {
		
		spell->game->addAnimationShoot(spellCastCreature, targetCreature->pos, distanceEffect);

		Player*	spellCastPlayer = dynamic_cast<Player*>(spellCastCreature);
		int damage = spell->script->onUse(spellCastCreature, targetCreature, "");
		//spell->game->creatureAttackCreature(spellCastCreature, targetCreature, attackType, amuInfo, damage);

		return true;
	}

	return false;
}

void MagicTargetSpell::getArea(const Position& centerpos, std::vector<Position>& vec, unsigned char direction) const
{
	vec.push_back(centerpos);
}

//---------------------------------------------------------------
MagicAreaSpell::MagicAreaSpell(Spell *ispell, attacktype_t iattackType, unsigned char idistanceEffect,
	const AreaVector& ivec, bool ineedDirection, amuEffect_t iamuInfo) : 
 MagicSpell(ispell), attackType(iattackType), distanceEffect(idistanceEffect), needDirection(ineedDirection), amuInfo(iamuInfo)
{
	setArea(ivec);
}

bool MagicAreaSpell::doCastSpell(Creature* spellCastCreature, const Position& pos, const std::string& var) const
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

				if(distanceEffect != 0xFF) {
					spell->game->addAnimationShoot(spellCastCreature, pos, distanceEffect);
				}

				if(tile->creatures.empty()) {
					spell->game->addMagicEffect(*it, amuInfo.areaEffect);
				}
				else {
					for(cit = tile->creatures.begin(); cit != tile->creatures.end(); ++cit) {
						int damage = spell->script->onUse(spellCastCreature, *cit, var);
						//spell->game->internalCreatureAttackCreature(spellCastCreature, *cit, attackType, amuInfo, damage);
						targetlist.push_back(*cit);
					}
				}
			}
		}
	}

	for(CreatureVector::iterator cit = targetlist.begin(); cit != targetlist.end(); ++cit) {
		//spell->game->internalCreatureAttackedCreature(spellCastCreature, *cit);
	}

	return true;
}

bool MagicAreaSpell::doCastSpell(Creature* spellCastCreature, Creature* targetCreature) const
{
	return doCastSpell(spellCastCreature, targetCreature->pos, "");
}

void MagicAreaSpell::getArea(const Position& centerpos, std::vector<Position>& vec, unsigned char direction) const
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


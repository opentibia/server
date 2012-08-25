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

#ifndef __OTSERV_COMBAT_H__
#define __OTSERV_COMBAT_H__

#include "classes.h"
#include "map.h"
#include "otsystem.h"

class CombatSource{
public:
	CombatSource();
	CombatSource(Creature* creature, Item* item, bool condition);
	CombatSource(Creature* creature);
	CombatSource(Item* item);
	CombatSource(const CombatSource& rhs);
	~CombatSource();

	bool isSourceCreature() const;
	bool isSourceItem() const;
	bool isSourceCondition() const;

	Creature* getSourceCreature() const;
	Item* getSourceItem() const;

	void setSourceCreature(Creature* _creature);
	void setSourceItem(Item* _item);
	void setSourceIsCondition(bool b);

private:
	Creature* creature;
	Item* item;
	bool condition;
};

struct CombatEffect{
	CombatEffect(bool showEffect) : showEffect(showEffect)
	{
		hitEffect = MAGIC_EFFECT_UNK;
		hitTextColor = TEXTCOLOR_UNK;
	}

	CombatEffect() {
		hitEffect = MAGIC_EFFECT_UNK;
		hitTextColor = TEXTCOLOR_UNK;
		showEffect = true;
	}

	MagicEffect hitEffect;
	TextColor hitTextColor;
	bool showEffect;
};

struct CombatParams{
	CombatParams() {
		combatType = COMBAT_NONE;
		blockedByArmor = false;
		blockedByShield = false;
		targetCasterOrTopMost = false;
		aggressive = true;
	}

	CombatType combatType;
	bool blockedByArmor;
	bool blockedByShield;
	bool targetCasterOrTopMost;
	bool aggressive;
	CombatEffect effects;
};

namespace Combat {

bool isInPvpZone(const Creature* attacker, const Creature* target);
bool isUnjustKill(const Creature* attacker, const Creature* target);
bool isPlayerCombat(const Creature* target);
ReturnValue canTargetCreature(const Player* attacker, const Creature* target);
ReturnValue canDoCombat(const Creature* attacker, const Tile* tile, bool isAggressive);
ReturnValue canDoCombat(const Creature* attacker, const Creature* target);
Position getCasterPosition(const Creature* creature, Direction dir);

} // namespace Combat

class MagicField : public Item {
public:
	MagicField(uint16_t _type);
	~MagicField();

	virtual MagicField* getMagicField();
	virtual const MagicField* getMagicField() const;
};

#endif

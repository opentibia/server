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

#ifndef __OTSERV_ENUMS_H__
#define __OTSERV_ENUMS_H__

enum RaceType_t {
	RACE_NONE = 0,
	RACE_VENOM,
	RACE_BLOOD,
	RACE_UNDEAD
};

enum CombatType_t {
	COMBAT_NONE = 0,
	COMBAT_HITPOINTS = 1,
	COMBAT_MANAPOINTS = 2,
	COMBAT_CONDITION = 3
};

enum DamageType_t {
	DAMAGE_NONE = 0,
	DAMAGE_PHYSICAL = 1,
	DAMAGE_ENERGY = 2,
	DAMAGE_POISON = 4 ,
	DAMAGE_FIRE = 8,
	DAMAGE_MANA = 16,
};

enum CombatParam_t{
	COMBATPARAM_DAMAGETYPE = 1,
	COMBATPARAM_EFFECT = 2,
	COMBATPARAM_DISTANCEEFFECT = 3,
	COMBATPARAM_BLOCKEDBYSHIELD = 4,
	COMBATPARAM_BLOCKEDBYARMOR = 5,
	COMBATPARAM_TARGETCASTERORTOPMOST = 6,
	COMBATPARAM_CREATEITEM = 7,
	COMBATPARAM_MINMAXCALLBACK = 8,
};

enum ConditionParam_t{
	CONDITIONPARAM_OWNER = 1,
	CONDITIONPARAM_TICKS = 2,
	CONDITION_PARAM_OUTFIT = 3,
	CONDITIONPARAM_DELAYED = 4,
	CONDITIONPARAM_SPEED = 5,
};

enum BlockType_t {
	BLOCK_NONE = 0,
	BLOCK_DEFENSE,
	BLOCK_ARMOR,
	BLOCK_IMMUNITY
};

enum Vocation_t {
	VOCATION_NONE = 0,
	VOCATION_SORCERER = 1,
	VOCATION_DRUID = 2,
	VOCATION_PALADIN = 3,
	VOCATION_KNIGHT = 4
};

enum skills_t {
	SKILL_FIRST = 0,
	SKILL_FIST = SKILL_FIRST,
	SKILL_CLUB = 1,
	SKILL_SWORD = 2,
	SKILL_AXE = 3,
	SKILL_DIST = 4,
	SKILL_SHIELD = 5,
	SKILL_FISH = 6,
	SKILL_LAST = SKILL_FISH
};

#endif

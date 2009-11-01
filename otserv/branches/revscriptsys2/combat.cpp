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
#include "otpch.h"

#include "combat.h"
#include "player.h"
#include "condition.h"
#include "configmanager.h"
#include "game.h"
#include "weapons.h"

extern Game g_game;
extern ConfigManager g_config;

CombatSource::CombatSource(Creature* creature, Item* item, bool condition) :
	creature(creature), item(item), condition(condition)
{
	if(creature) creature->addRef();
	if(item) item->addRef();
}

CombatSource::CombatSource(Creature* creature) :
	creature(creature), item(NULL), condition(false)
{
	if(creature) creature->addRef();
}

CombatSource::CombatSource(Item* item) :
	creature(NULL), item(item), condition(false)
{
	if(item) item->addRef();
}

CombatSource::CombatSource(const CombatSource& rhs) :
	creature(NULL),
	item(NULL)
{
	setSourceCreature(rhs.creature);
	setSourceItem(rhs.item);
	setSourceIsCondition(rhs.condition);
}

CombatSource::~CombatSource()
{
	setSourceCreature(NULL);
	setSourceItem(NULL);
}

void CombatSource::setSourceCreature(Creature* _creature)
{
	if(_creature != creature){
		if(creature){
			creature->unRef();
		}
		creature = _creature;

		if(creature){
			creature->addRef();
		}
	}
}

void CombatSource::setSourceItem(Item* _item)
{
	if(_item != item){
		if(item){
			item->unRef();
		}

		item = _item;

		if(item){
			item->addRef();
		}
	}
}

Combat::Combat()
{
	//
}

Combat::~Combat()
{
	//
}

/*
bool Combat::internalCombat(CombatSource& combatSource, CombatParams& params, Creature* target,
	const SpectatorVec* spectators = NULL) const
{
	Creature* attacker = combatSource.getSourceCreature();
	if(!params.aggressive || (attacker != target && Combat::canDoCombat(attacker, target) == RET_NOERROR)){
		if(params.combatType != COMBAT_NONE){
			int32_t minChange = 0;
			int32_t maxChange = 0;
			//getMinMaxValues(attacker, target, minChange, maxChange);
			int32_t value = random_range(minChange, maxChange, DISTRO_NORMAL);

			if(g_game.combatBlockHit(params.combatType, combatSource, target, value, params.blockedByShield, params.blockedByArmor)){
				return true;
			}

			if(params.combatType != COMBAT_MANADRAIN){
				if(changeHealth(combatSource, params, target, value)){
					return true;
				}
			}
			else{
				if(changeMana(combatSource, params, target, value)){
					return true;
				}
			}
		}
		else{
			return true;
		}
	}

	return false;
}
*/

bool Combat::isPlayerCombat(const Creature* target)
{
	if(target->getPlayer()){
		return true;
	}

	if(target->isPlayerSummon()){
		return true;
	}

	return false;
}

ReturnValue Combat::canTargetCreature(const Player* player, const Creature* target)
{
	if(player == target){
		return RET_YOUMAYNOTATTACKTHISPERSON;
	}

	if(!player->hasFlag(PlayerFlag_IgnoreProtectionZone)){
		//pz-zone
		if(player->getZone() == ZONE_PROTECTION){
			return RET_YOUMAYNOTATTACKAPERSONWHILEINPROTECTIONZONE;
		}
		if(target->getZone() == ZONE_PROTECTION){
			return RET_YOUMAYNOTATTACKAPERSONINPROTECTIONZONE;
		}

		//nopvp-zone
		if(isPlayerCombat(target)){
			if(player->getZone() == ZONE_NOPVP){
				return RET_ACTIONNOTPERMITTEDINANONPVPZONE;
			}
			if(target->getZone() == ZONE_NOPVP){
				return RET_YOUMAYNOTATTACKAPERSONINPROTECTIONZONE;
			}
		}
	}

	if(player->hasFlag(PlayerFlag_CannotUseCombat) || !target->isAttackable()){
		if(target->getPlayer()){
			return RET_YOUMAYNOTATTACKTHISPERSON;
		}
		else{
			return RET_YOUMAYNOTATTACKTHISCREATURE;
		}
	}

#ifdef __SKULLSYSTEM__
	if(const Player* targetPlayer = target->getPlayer()){
		if(player->hasSafeMode()){
			if(player->isPartner(targetPlayer)){
				return Combat::canDoCombat(player, targetPlayer);
			}
			if(targetPlayer->getSkull() == SKULL_NONE){
				if(!Combat::isInPvpZone(player, targetPlayer)){
					return RET_TURNSECUREMODETOATTACKUNMARKEDPLAYERS;
				}
			}
		}
		else if(player->getSkull() == SKULL_BLACK){
			if(targetPlayer->getSkull() == SKULL_NONE && !targetPlayer->hasAttacked(player)){
				return RET_YOUMAYNOTATTACKTHISPERSON;
			}
		}
	}
#endif

	return Combat::canDoCombat(player, target);
}

ReturnValue Combat::canDoCombat(const Creature* attacker, const Tile* tile, bool isAggressive)
{
	if(tile->blockProjectile()){
		return RET_NOTENOUGHROOM;
	}

	if(tile->floorChange()){
		return RET_NOTENOUGHROOM;
	}

	if(tile->getTeleportItem()){
		return RET_NOTENOUGHROOM;
	}

	if(attacker){
		if(attacker->getPosition().z < tile->getPosition().z){
			return RET_FIRSTGODOWNSTAIRS;
		}

		if(attacker->getPosition().z > tile->getPosition().z){
			return RET_FIRSTGOUPSTAIRS;
		}

		if(const Player* player = attacker->getPlayer()){
			if(player->hasFlag(PlayerFlag_IgnoreProtectionZone)){
				return RET_NOERROR;
			}
		}
	}

	//pz-zone
	if(isAggressive && tile->hasFlag(TILEPROP_PROTECTIONZONE)){
		return RET_ACTIONNOTPERMITTEDINPROTECTIONZONE;
	}

	return RET_NOERROR;
}

bool Combat::isInPvpZone(const Creature* attacker, const Creature* target)
{
	if(attacker->getZone() != ZONE_PVP){
		return false;
	}

	if(target->getZone() != ZONE_PVP){
		return false;
	}

	return true;
}

bool Combat::isUnjustKill(const Creature* attacker, const Creature* target)
{
#ifdef __SKULLSYSTEM__
	const Player* attackerPlayer = attacker->getPlayer();
	const Player* targetPlayer = target->getPlayer();

	if(attacker->isPlayerSummon()){
		attackerPlayer = attacker->getPlayerMaster();
	}

	if(	attackerPlayer == NULL ||
		targetPlayer == NULL ||
		attackerPlayer->isPartner(targetPlayer) ||
		Combat::isInPvpZone(attackerPlayer, targetPlayer) ||
		targetPlayer->hasAttacked(attackerPlayer) ||
		targetPlayer->getSkull() != SKULL_NONE ||
		targetPlayer == attackerPlayer){
		return false;
	}

	return true;

#else
	return false;
#endif
}

ReturnValue Combat::canDoCombat(const Creature* attacker, const Creature* target)
{
	if(attacker){
		if(const Player* targetPlayer = target->getPlayer()){
			if(targetPlayer->hasFlag(PlayerFlag_CannotBeAttacked)){
				return RET_YOUMAYNOTATTACKTHISPERSON;
			}

			if(const Player* attackerPlayer = attacker->getPlayer()){
				if(attackerPlayer->hasFlag(PlayerFlag_CannotAttackPlayer) ||
					attackerPlayer->isLoginAttackLocked(targetPlayer->getID())){
					return RET_YOUMAYNOTATTACKTHISPERSON;
				}
#ifdef __SKULLSYSTEM__
				if(attackerPlayer->getSkull() == SKULL_BLACK){
					if(targetPlayer->getSkull() == SKULL_NONE && !targetPlayer->hasAttacked(attackerPlayer)){
						return RET_YOUMAYNOTATTACKTHISPERSON;
					}
				}
#endif
			}

			if(const Player* masterAttackerPlayer = attacker->getPlayerMaster()){
				if(masterAttackerPlayer->hasFlag(PlayerFlag_CannotAttackPlayer)){
					return RET_YOUMAYNOTATTACKTHISPERSON;
				}
			}
		}
		else if(target->getActor()){
			if(const Player* attackerPlayer = attacker->getPlayer()){
				if(attackerPlayer->hasFlag(PlayerFlag_CannotAttackMonster)){
					return RET_YOUMAYNOTATTACKTHISCREATURE;
				}
			}
		}

		if(attacker->getPlayer() || attacker->isPlayerSummon()){
			//nopvp-zone
			if(target->getPlayer() && target->getParentTile()->hasFlag(TILEPROP_NOPVPZONE)){
				return RET_ACTIONNOTPERMITTEDINANONPVPZONE;
			}

			if(g_game.getWorldType() == WORLD_TYPE_NOPVP){
				if(target->getPlayer()){
					if(!isInPvpZone(attacker, target)){
						return RET_YOUMAYNOTATTACKTHISPERSON;
					}
				}

				if(target->isPlayerSummon()){
					if(!isInPvpZone(attacker, target)){
						return RET_YOUMAYNOTATTACKTHISCREATURE;
					}
				}
			}
		}
	}

	return RET_NOERROR;
}

Position Combat::getCasterPosition(const Creature* creature, Direction dir)
{
	Position pos = creature->getPosition();

	switch(dir.value()){
		case enums::NORTH:
			pos.y -= 1;
			break;

		case enums::SOUTH:
			pos.y += 1;
			break;

		case enums::EAST:
			pos.x += 1;
			break;

		case enums::WEST:
			pos.x -= 1;
			break;

		case enums::SOUTHWEST:
			pos.x -= 1;
			pos.y += 1;
		break;

		case enums::NORTHWEST:
			pos.x -= 1;
			pos.y -= 1;
		break;

		case enums::NORTHEAST:
			pos.x += 1;
			pos.y -= 1;
		break;

		case enums::SOUTHEAST:
			pos.x += 1;
			pos.y += 1;
		break;

		default:
			break;
	}

	return pos;
}

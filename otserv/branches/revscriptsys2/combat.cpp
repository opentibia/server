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

void Combat::combatToTarget(CombatSource& combatSource, CombatParams& params, Creature* target) const
{
	bool result = internalCombat(combatSource, params, target);
	if(result){
		if(params.effects.impactEffect != MAGIC_EFFECT_NONE){
			g_game.addMagicEffect(target->getPosition(), params.effects.impactEffect);
		}

		Creature* attacker = combatSource.getSourceCreature();
		if(attacker && params.effects.distanceEffect != MAGIC_EFFECT_NONE){
			g_game.addDistanceEffect(attacker, attacker->getPosition(), target->getPosition(), params.effects.distanceEffect);
		}
	}
}

void Combat::combatToArea(CombatSource& combatSource, CombatParams& params,
	const Position& pos, const CombatArea* area) const
{
	std::list<Tile*> tileList;
	getCombatArea(pos, pos, area, tileList);

	SpectatorVec spectators;
	getSpectators(pos, tileList, spectators);

	Creature* attacker = combatSource.getSourceCreature();

	Tile* iter_tile;
	for(std::list<Tile*>::iterator it = tileList.begin(); it != tileList.end(); ++it){
		iter_tile = *it;
		bool bContinue = true;

		if(canDoCombat(attacker, iter_tile, params.isAggressive) == RET_NOERROR){
			CreatureIterator cit;
			CreatureIterator cend = iter_tile->creatures_end();
			for(cit = iter_tile->creatures_begin(); bContinue && cit != cend; ++cit){
				if(params.targetCasterOrTopMost){
					if(attacker && attacker->getTile() == iter_tile){
						if(*cit == attacker){
							bContinue = false;
						}
					}
					else if(*cit == iter_tile->getTopCreature()){
						bContinue = false;
					}

					if(bContinue){
						continue;
					}
				}

				internalCombat(combatSource, params, *cit, &spectators);
			}
		}
	}

	if(attacker){
		g_game.addDistanceEffect(attacker, attacker->getPosition(), pos, params.effects.distanceEffect);
	}
}

bool Combat::internalCombat(CombatSource& combatSource, CombatParams& params, Creature* target,
	const SpectatorVec* spectators /*= NULL*/) const
{
	Creature* attacker = combatSource.getSourceCreature();
	if(!params.isAggressive || (attacker != target && Combat::canDoCombat(attacker, target) == RET_NOERROR)){
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
					applyCondition(combatSource, params, target);
					applyDispel(combatSource, params, target);
					return true;
				}
			}
			else{
				if(changeMana(combatSource, params, target, value)){
					applyCondition(combatSource, params, target);
					applyDispel(combatSource, params, target);
					return true;
				}
			}

			defaultCombat(combatSource, params, target, spectators);
		}
		else{
			applyCondition(combatSource, params, target);
			applyDispel(combatSource, params, target);
			defaultCombat(combatSource, params, target, spectators);
			return true;
		}
	}

	return false;
}

bool Combat::defaultCombat(CombatSource& combatSource, CombatParams& params, Creature* target, 
	const SpectatorVec* spectators) const
{
	if(params.itemId != 0){
		Creature* attacker = combatSource.getSourceCreature();
		if(spectators){
			addTileItem(*spectators, attacker, target->getTile(), params);
		}
		else{
			const SpectatorVec& list = g_game.getSpectators(target->getTile()->getPosition());
			addTileItem(list, attacker, target->getTile(), params);
		}
	}
	return true;
}

bool Combat::changeHealth(CombatSource& combatSource, CombatParams& params, Creature* target, int32_t healthChange) const
{
	Creature* attacker = combatSource.getSourceCreature();
	if(healthChange < 0){
#ifdef __SKULLSYSTEM__
		if(attacker && attacker->getPlayer() && target->getPlayer() && target->getPlayer()->getSkull() != SKULL_BLACK){
#else
	if(attacker && attacker->getPlayer() && target->getPlayer()){
#endif
			healthChange = healthChange/2;
		}
	}

	return g_game.combatChangeHealth(params.combatType, combatSource, params.effects, target, healthChange);  
}

bool Combat::changeMana(CombatSource& combatSource, CombatParams& params, Creature* target, int32_t manaChange) const
{
	Creature* attacker = combatSource.getSourceCreature();
	if(manaChange < 0){
#ifdef __SKULLSYSTEM__
		if(attacker && attacker->getPlayer() && target->getPlayer() && target->getPlayer()->getSkull() != SKULL_BLACK){
#else
		if(attacker && attacker->getPlayer() && target->getPlayer()){
#endif
			manaChange = manaChange/2;
		}
	}

	return g_game.combatChangeMana(combatSource, params.effects, target, manaChange);
}

bool Combat::applyCondition(CombatSource& combatSource, CombatParams& params, Creature* target) const
{
	Creature* attacker = combatSource.getSourceCreature();
	for(std::list<const Condition*>::const_iterator it = params.conditionList.begin(); it != params.conditionList.end(); ++it){
		const Condition* condition = *it;

		if(attacker == target || !target->isImmune(condition)){
			Condition* conditionCopy = condition->clone();
			conditionCopy->setSource(combatSource);
			target->addCombatCondition(conditionCopy);
		}
	}

	return true;
}

bool Combat::applyDispel(CombatSource& combatSource, CombatParams& params, Creature* target) const
{
	Creature* attacker = combatSource.getSourceCreature();
	if(target->hasCondition(params.dispelName)){
		target->removeCondition(params.dispelName, attacker);
		return true;
	}

	return false;
}

void Combat::addTileItem(const SpectatorVec& list, Creature* attacker, Tile* tile, CombatParams& params) const
{
	if(params.itemId != 0){
		uint32_t itemId = params.itemId;
		if(attacker && (attacker->getPlayer() || attacker->isPlayerSummon())){
			Player* player = attacker->getPlayer();
			if(!player){
				player = attacker->getPlayerMaster();
			}

			if(g_game.getWorldType() == WORLD_TYPE_NOPVP || tile->hasFlag(TILEPROP_NOPVPZONE)){
				if(itemId == ITEM_FIREFIELD_PVP){
					itemId = ITEM_FIREFIELD_NOPVP;
				}
				else if(itemId == ITEM_POISONFIELD_PVP){
					itemId = ITEM_POISONFIELD_NOPVP;
				}
				else if(itemId == ITEM_ENERGYFIELD_PVP){
					itemId = ITEM_ENERGYFIELD_NOPVP;
				}
			} else if(params.isAggressive){
				const ItemType& it = Item::items[itemId];
				if(!it.blockSolid){
					player->addInFightTicks(g_game.getInFightTicks(), true);
				}
				else{
					player->addInFightTicks(g_game.getInFightTicks());
				}
			}
		}
		Item* item = Item::CreateItem(itemId);

		if(attacker){
			item->setOwner(attacker->getID());
		}

		ReturnValue ret = g_game.internalAddItem(attacker, tile, item);
		if(ret == RET_NOERROR){
			g_game.startDecay(item);
		}
		else{
			delete item;
		}
	}
}

void Combat::getCombatArea(const Position& centerPos, const Position& targetPos, const CombatArea* area,
	std::list<Tile*>& list) const
{
	if(area){
		area->getList(centerPos, targetPos, list);
	}
	else if(targetPos.x >= 0 && targetPos.x < 0xFFFF &&
			targetPos.y >= 0 && targetPos.y < 0xFFFF &&
			targetPos.z >= 0 && targetPos.z < MAP_MAX_LAYERS)
	{
		Tile* tile = g_game.getTile(targetPos.x, targetPos.y, targetPos.z);
		if(!tile){
			// These tiles will never have anything on them
			tile = new StaticTile(targetPos.x, targetPos.y, targetPos.z);
			g_game.setTile(tile);
		}
		list.push_back(tile);
	}
}

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
			if(target->getPlayer() && target->getTile()->hasFlag(TILEPROP_NOPVPZONE)){
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

void Combat::getSpectators(const Position& pos, const std::list<Tile*>& tile_list, SpectatorVec& spectators) const
{
	uint32_t maxX = 0;
	uint32_t maxY = 0;
	uint32_t diff;

	//calculate the max viewable range
	for(std::list<Tile*>::const_iterator it = tile_list.begin(); it != tile_list.end(); ++it){
		diff = std::abs((*it)->getPosition().x - pos.x);
		if(diff > maxX){
			maxX = diff;
		}

		diff = std::abs((*it)->getPosition().y - pos.y);
		if(diff > maxY){
			maxY = diff;
		}
	}

	g_game.getSpectators(spectators, pos, false, true, maxX + Map::maxViewportX, maxX + Map::maxViewportX,
		maxY + Map::maxViewportY, maxY + Map::maxViewportY);
}

void CombatArea::clear()
{
	for(CombatAreaMap::iterator it = areas.begin(); it != areas.end(); ++it){
		delete it->second;
	}

	areas.clear();
}

CombatArea::CombatArea(const CombatArea& rhs)
{
	hasExtArea = rhs.hasExtArea;

	for(CombatAreaMap::const_iterator it = rhs.areas.begin(); it != rhs.areas.end(); ++it){
		areas[it->first] = new MatrixArea(*it->second);
	}
}

bool CombatArea::getList(const Position& centerPos, const Position& targetPos, std::list<Tile*>& list) const
{
	Tile* tile = g_game.getTile(targetPos.x, targetPos.y, targetPos.z);

	const MatrixArea* area = getArea(centerPos, targetPos);
	if(!area){
		return false;
	}

	int32_t tmpPosX = targetPos.x;
	int32_t tmpPosY = targetPos.y;
	int32_t tmpPosZ = targetPos.z;

	size_t cols = area->getCols();
	size_t rows = area->getRows();

	uint32_t centerY, centerX;
	area->getCenter(centerY, centerX);

	tmpPosX -= centerX;
	tmpPosY -= centerY;

	for(size_t y = 0; y < rows; ++y){
		for(size_t x = 0; x < cols; ++x){

			if(area->getValue(y, x) != 0){
				if(	tmpPosX >= 0 && tmpPosX < 0xFFFF &&
					tmpPosY >= 0 && tmpPosY < 0xFFFF &&
					tmpPosZ >= 0 && tmpPosZ < MAP_MAX_LAYERS)
				{
					if(g_game.isSightClear(targetPos, Position(tmpPosX, tmpPosY, tmpPosZ), true)){
						tile = g_game.getTile(tmpPosX, tmpPosY, tmpPosZ);
						if(!tile){
							// This tile will never have anything on it
							tile = new StaticTile(tmpPosX, tmpPosY, tmpPosZ);
							g_game.setTile(tile);
						}
						list.push_back(tile);
					}
					list.push_back(tile);
				}
			}

			tmpPosX += 1;
		}

		tmpPosX -= cols;
		tmpPosY += 1;
	}

	return true;
}

int32_t round(float v)
{
	int32_t t = (int32_t)std::floor(v);
	if((v - t) > 0.5){
		return t + 1;
	}
	else{
		return t;
	}
}

void CombatArea::copyArea(const MatrixArea* input, MatrixArea* output, MatrixOperation_t op) const
{
	uint32_t centerY, centerX;
	input->getCenter(centerY, centerX);

	if(op == MATRIXOPERATION_COPY){
		for(unsigned int y = 0; y < input->getRows(); ++y){
			for(unsigned int x = 0; x < input->getCols(); ++x){
				(*output)[y][x] = (*input)[y][x];
			}
		}

		output->setCenter(centerY, centerX);
	}
	else if(op == MATRIXOPERATION_MIRROR){
		for(unsigned int y = 0; y < input->getRows(); ++y){
			int rx = 0;
			for(int x = input->getCols() - 1; x >= 0; --x){
				(*output)[y][rx++] = (*input)[y][x];
			}
		}

		output->setCenter(centerY, (input->getRows() - 1) - centerX);
	}
	else if(op == MATRIXOPERATION_FLIP){
		for(unsigned int x = 0; x < input->getCols(); ++x){
			int ry = 0;
			for(int y = input->getRows() - 1; y >= 0; --y){
				(*output)[ry++][x] = (*input)[y][x];
			}
		}

		output->setCenter((input->getCols() - 1) - centerY, centerX);
	}
	//rotation
	else{
		uint32_t centerX, centerY;
		input->getCenter(centerY, centerX);

		int32_t rotateCenterX = (output->getCols() / 2) - 1;
		int32_t rotateCenterY = (output->getRows() / 2) - 1;
		int32_t angle = 0;

		switch(op){
			case MATRIXOPERATION_ROTATE90:
				angle = 90;
				break;

			case MATRIXOPERATION_ROTATE180:
				angle = 180;
				break;

			case MATRIXOPERATION_ROTATE270:
				angle = 270;
				break;

			default:
				angle = 0;
				break;
		}
		double angleRad = 3.1416 * angle / 180.0;

		float a = std::cos(angleRad);
		float b = -std::sin(angleRad);
		float c = std::sin(angleRad);
		float d = std::cos(angleRad);

		for(int32_t x = 0; x < (int32_t)input->getCols(); ++x){
			for(int32_t y = 0; y < (int32_t)input->getRows(); ++y){
				//calculate new coordinates using rotation center
				int32_t newX = x - centerX;
				int32_t newY = y - centerY;

				//perform rotation
				int32_t rotatedX = round(newX * a + newY * b);
				int32_t rotatedY = round(newX * c + newY * d);

				//write in the output matrix using rotated coordinates
				(*output)[rotatedY + rotateCenterY][rotatedX + rotateCenterX] = (*input)[y][x];
			}
		}

		output->setCenter(rotateCenterY, rotateCenterX);
	}
}

MatrixArea* CombatArea::createArea(const std::list<uint32_t>& list, uint32_t rows)
{
	unsigned int cols = list.size() / rows;
	MatrixArea* area = new MatrixArea(rows, cols);

	uint32_t x = 0;
	uint32_t y = 0;

	for(std::list<uint32_t>::const_iterator it = list.begin(); it != list.end(); ++it){
		if(*it == 1 || *it == 3){
			area->setValue(y, x, true);
		}

		if(*it == 2 || *it == 3){
			area->setCenter(y, x);
		}

		++x;

		if(cols == x){
			x = 0;
			++y;
		}
	}

	return area;
}

void CombatArea::setupArea(const std::list<uint32_t>& list, uint32_t rows)
{
	MatrixArea* area = createArea(list, rows);

	//NORTH
	areas[NORTH] = area;

	uint32_t maxOutput = std::max(area->getCols(), area->getRows()) * 2;

	//SOUTH
	MatrixArea* southArea = new MatrixArea(maxOutput, maxOutput);
	copyArea(area, southArea, MATRIXOPERATION_ROTATE180);
	areas[SOUTH] = southArea;

	//EAST
	MatrixArea* eastArea = new MatrixArea(maxOutput, maxOutput);
	copyArea(area, eastArea, MATRIXOPERATION_ROTATE90);
	areas[EAST] = eastArea;

	//WEST
	MatrixArea* westArea = new MatrixArea(maxOutput, maxOutput);
	copyArea(area, westArea, MATRIXOPERATION_ROTATE270);
	areas[WEST] = westArea;
}

void CombatArea::setupArea(int32_t length, int32_t spread)
{
	std::list<uint32_t> list;

	uint32_t rows = length;
	int32_t cols = 1;

	if(spread != 0){
		cols = ((length - length % spread) / spread) * 2 + 1;
	}

	int32_t colSpread = cols;

	for(uint32_t y = 1; y <= rows; ++y){
		int32_t mincol = cols - colSpread + 1;
		int32_t maxcol = cols - (cols - colSpread);
		for(int32_t x = 1; x <= cols; ++x){
			if(y == rows && x == ((cols - cols % 2) / 2) + 1){
				list.push_back(3);
			}
			else if(x >= mincol && x <= maxcol){
				list.push_back(1);
			}
			else{
				list.push_back(0);
			}
		}

		if(spread > 0 && y % spread == 0){
			--colSpread;
		}
	}

	setupArea(list, rows);
}

void CombatArea::setupArea(int32_t radius)
{
	int32_t area[13][13] = {
		{0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 8, 8, 7, 8, 8, 0, 0, 0, 0},
		{0, 0, 0, 8, 7, 6, 6, 6, 7, 8, 0, 0, 0},
		{0, 0, 8, 7, 6, 5, 5, 5, 6, 7, 8, 0, 0},
		{0, 8, 7, 6, 5, 4, 4, 4, 5, 6, 7, 8, 0},
		{0, 8, 6, 5, 4, 3, 2, 3, 4, 5, 6, 8, 0},
		{8, 7, 6, 5, 4, 2, 1, 2, 4, 5, 6, 7, 8},
		{0, 8, 6, 5, 4, 3, 2, 3, 4, 5, 6, 8, 0},
		{0, 8, 7, 6, 5, 4, 4, 4, 5, 6, 7, 8, 0},
		{0, 0, 8, 7, 6, 5, 5, 5, 6, 7, 8, 0, 0},
		{0, 0, 0, 8, 7, 6, 6, 6, 7, 8, 0, 0, 0},
		{0, 0, 0, 0, 8, 8, 7, 8, 8, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0}
	};

	std::list<uint32_t> list;

	for(int32_t y = 0; y < 13; ++y){
		for(int32_t x = 0; x < 13; ++x){
			if(area[y][x] == 1){
				list.push_back(3);
			}
			else if(area[y][x] > 0 && area[y][x] <= radius){
				list.push_back(1);
			}
			else{
				list.push_back(0);
			}
		}
	}

	setupArea(list, 13);
}

void CombatArea::setupExtArea(const std::list<uint32_t>& list, uint32_t rows)
{
	if(list.empty()){
		return;
	}

	hasExtArea = true;
	MatrixArea* area = createArea(list, rows);

	//NORTH-WEST
	areas[NORTHWEST] = area;

	uint32_t maxOutput = std::max(area->getCols(), area->getRows()) * 2;

	//NORTH-EAST
	MatrixArea* neArea = new MatrixArea(maxOutput, maxOutput);
	copyArea(area, neArea, MATRIXOPERATION_MIRROR);
	areas[NORTHEAST] = neArea;

	//SOUTH-WEST
	MatrixArea* swArea = new MatrixArea(maxOutput, maxOutput);
	copyArea(area, swArea, MATRIXOPERATION_FLIP);
	areas[SOUTHWEST] = swArea;

	//SOUTH-EAST
	MatrixArea* seArea = new MatrixArea(maxOutput, maxOutput);
	copyArea(swArea, seArea, MATRIXOPERATION_MIRROR);
	areas[SOUTHEAST] = seArea;
}

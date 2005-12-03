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

#include <vector>
#include <string>
#include <sstream>

#include "monster.h"
#include "monsters.h"
#include "spells.h"

extern Spells spells;

AutoList<Monster>Monster::listMonster;

extern Monsters g_monsters;

Monster* Monster::createMonster(const std::string& name, Game* game)
{
	unsigned long id = g_monsters.getIdByName(name);
	if(!id){
		return NULL;
	}
	
	MonsterType* mtype = g_monsters.getMonsterType(id);
	if(!mtype)
		return NULL;
	
	Monster* new_monster = new Monster(mtype, game);
	return new_monster;
}

Monster::Monster(MonsterType* _mtype, Game* game) : 
Creature()
{
	mType = _mtype;
	useCount = 0;
	oldThinkTicks = 0;
	state = STATE_IDLE;
	updateMovePos = false;
	this->game = game;
	this->speed = 220;
	this->level = 8;
	this->maglevel = 0;
	curPhysicalAttack = NULL;
	hasLostMaster = false;
	
	lookhead = mType->lookhead;
	lookbody = mType->lookbody;
	looklegs = mType->looklegs;
	lookfeet = mType->lookfeet;
	looktype = mType->looktype;
	lookcorpse = mType->lookcorpse;
	lookmaster = mType->lookmaster;
	health     = mType->health;
	healthmax  = mType->health_max;
	lookcorpse = mType->lookcorpse;
	immunities = mType->immunities;
	speed = mType->base_speed;
}

unsigned long Monster::getRandom(){
	return (unsigned long)((rand()<< 16 | rand()) % CHANCE_MAX);
}


Monster::~Monster()
{

}

int Monster::getArmor() const
{
	return mType->armor;
}

int Monster::getDefense() const
{
	return mType->defense;
}

bool Monster::isPushable() const
{
	return mType->pushable;
}

const std::string& Monster::getName() const
{
	return mType->name;
}

bool Monster::isCreatureAttackable(const Creature* creature)
{
	if(!creature || creature->access != 0)
		return false;

	if(!dynamic_cast<const Player*>(creature) && !(const_cast<Creature*>(creature))->getMaster())
		return false;

	Creature* topMaster = (const_cast<Creature*>(creature))->getMaster();
	while(topMaster && topMaster->getMaster()) {
		topMaster = const_cast<Creature*>(topMaster->getMaster());
	}

	if(topMaster && (!dynamic_cast<const Player*>(topMaster) || topMaster->access > 0))
		return false;
	
	return true;
}

int Monster::getLostExperience()
{
	return (isSummon() ? 0 : mType->experience);	
}

bool Monster::validateDistanceAttack(const Creature *target)
{
	//return isInRange(pos) && game->map->canThrowItemTo(this->pos, target->pos, false, true);
	return isInRange(getPosition()) && (game->map->canThrowObjectTo(getPosition(), target->getPosition(), BLOCK_PROJECTILE) == RET_NOERROR);
}

bool Monster::validateDistanceAttack(const Position &pos)
{
	return isInRange(pos) && (game->map->canThrowObjectTo(getPosition(), pos, BLOCK_PROJECTILE) == RET_NOERROR);
}

bool Monster::isCreatureReachable(const Creature* creature)
{
	Tile* tile = game->getTile(creature->getPosition().x, creature->getPosition().y, creature->getPosition().z);
	if(!tile || tile->isPz()) {
		return false;
	}

	if(mType->hasDistanceAttack && validateDistanceAttack(creature)) {
		return true;
	}
	else {
		if(getCurrentDistanceToTarget(creature->getPosition()) <= 1) {
			return true;
		}

		Position closePos;
		return getCloseCombatPosition(creature->getPosition(), closePos);

		/*
		//to slow
		if(getCloseCombatPosition(creature->pos, closePos)) {
			return !game->map->getPathTo(this, this->pos, closePos, true, canPushItems).empty();
		}
		*/

		return false;
	}
}

Creature* Monster::findTarget(long range, bool &canReach, const Creature *ignoreCreature /*= NULL*/)
{
	SpectatorVec tmplist;
	if(range == 0)
		game->getSpectators(Range(getPosition(), false), tmplist);
	else
		game->getSpectators(Range(getPosition(), range, range, range, range, false), tmplist);

	std::vector<Creature*> targetlist;
	std::vector<Creature*> unreachablelist;

	for(SpectatorVec::const_iterator cit = tmplist.begin(); cit != tmplist.end(); ++cit) {
		if(ignoreCreature == *cit || (*cit)->access > 0)
			continue;

		if(!isCreatureAttackable(*cit))
			continue;

		if(!isCreatureReachable(*cit)) {
			unreachablelist.push_back(*cit);
			continue;
		}

		targetlist.push_back(*cit);
	}

	if(targetlist.empty()) {
		if(!unreachablelist.empty()) {
			size_t index = random_range(0, unreachablelist.size() - 1);
			canReach = false;
			return unreachablelist[index];
		}

		canReach = false;
		return NULL;
	}

	size_t index = random_range(0, targetlist.size() - 1);
	canReach = true;
	return targetlist[index];
}

int Monster::onThink(int& newThinkTicks)
{
	bool yelled = false;
	for(YellingSentences::iterator ysIt = mType->yellingSentences.begin(); ysIt != mType->yellingSentences.end(); ++ysIt) {
		if(ysIt->second.onTick(oldThinkTicks) && !yelled) {
			yelled = true;
			game->creatureMonsterYell(this, ysIt->first);
		}
	}

	long long delay;
	delay = getSleepTicks();
	
	if(state == STATE_TARGETNOTREACHABLE || attackedCreature != 0) {
		if(delay > 0) {
			newThinkTicks = (int)delay;
			int ret = oldThinkTicks;
			oldThinkTicks = newThinkTicks;
			return ret;
		}

		this->lastmove = OTSYS_TIME();
	}
	
	reThink(false);

	//check/update/calc route
	if(state != STATE_IDLE && state != STATE_TARGETNOTREACHABLE && !(isSummon() && hasLostMaster))
	{
		if(updateMovePos || !game->map->isPathValid(this, route, mType->canPushItems))
		{
			updateMovePos = false;

			if(calcMovePosition()) {
				route = game->map->getPathTo(this, getPosition(), moveToPos, true, mType->canPushItems);

				if(route.empty() && !(state == STATE_IDLESUMMON) && (!mType->hasDistanceAttack || !validateDistanceAttack(targetPos))) {
					state = STATE_TARGETNOTREACHABLE;
				}

				if(!route.empty() && route.front() == getPosition()) {
					route.pop_front();
				}
			}
		}
	}

	//process movement
	if(state != STATE_IDLE && !(state == STATE_IDLESUMMON && hasLostMaster)) {
		if(state == STATE_TARGETNOTREACHABLE) {
			Position newMovePos;
			if(getRandomPosition(targetPos, newMovePos)) {
				int dx = newMovePos.x - getPosition().x;
				int dy = newMovePos.y - getPosition().y;

				doMoveTo(dx, dy);
			}
			else {
				updateLookDirection();
			}
		}
		else if(!route.empty()){
			Position nextStep = route.front();
			route.pop_front();

			int dx = nextStep.x - getPosition().x;
			int dy = nextStep.y - getPosition().y;

			doMoveTo(dx, dy);
		}
		else {
			updateLookDirection();
		}

		newThinkTicks = getStepDuration();
		int ret = oldThinkTicks;
		oldThinkTicks = newThinkTicks;
		return ret;
	}

	eventCheck = 0;
	stopThink();
	newThinkTicks = 0;
	return oldThinkTicks;
}

int Monster::getTargetDistance()
{
	return mType->targetDistance;
}

int Monster::getCurrentDistanceToTarget(const Position& target)
{
	return std::max(std::abs(getPosition().x - target.x), std::abs(getPosition().y - target.y));
}

void Monster::updateLookDirection()
{
	if(isSummon() && state == STATE_IDLESUMMON) {
		return;
	}

	int deltax = targetPos.x - getPosition().x;
	int deltay = targetPos.y - getPosition().y;

	if(!(deltax == 0 && deltay == 0)) {
		Direction newdir = this->getDirection();

		//SE
		if(deltax < 0 && deltay < 0) {
			if(std::abs(deltax) > std::abs(deltay)) {
				newdir = WEST;
			}
			else if(std::abs(deltay) > std::abs(deltax)) {
				newdir = NORTH;
			}
			else if(getDirection() != NORTH && getDirection() != WEST) {
				newdir = (rand() % 2 == 0 ? NORTH : WEST);
			}
		}
		//SW
		else if(deltax > 0 && deltay < 0) {
			if(std::abs(deltax) > std::abs(deltay)) {
				newdir = EAST;
			}
			else if(std::abs(deltay) > std::abs(deltax)) {
				newdir = NORTH;
			}
			else if(getDirection() != NORTH && getDirection() != EAST) {
				newdir = (rand() % 2 == 0 ? NORTH : EAST);
			}
		}
		//NW
		else if(deltax > 0 && deltay > 0) {
			if(std::abs(deltax) > std::abs(deltay)) {
				newdir = EAST;
			}
			else if(std::abs(deltay) > std::abs(deltax)) {
				newdir = SOUTH;
			}
			else if(getDirection() != SOUTH && getDirection() != EAST) {
				newdir = (rand() % 2 == 0 ? SOUTH : EAST);
			}
		}
		//NE
		else if(deltax < 0 && deltay > 0) {
			if(std::abs(deltax) > std::abs(deltay)) {
				newdir = WEST;
			}
			else if(std::abs(deltay) > std::abs(deltax)) {
				newdir = SOUTH;
			}
			else if(getDirection() != SOUTH && getDirection() != EAST) {
				newdir = (rand() % 2 == 0 ? SOUTH : WEST);
			}
		}
		//N
		else if(deltax == 0 && deltay > 0)
			newdir = SOUTH;
		//S
		else if(deltax == 0 && deltay < 0)
			newdir = NORTH;
		//W
		else if(deltax > 0 && deltay == 0)
			newdir = EAST;
		//E
		else if(deltax < 0 && deltay == 0)
			newdir = WEST;

		if(newdir != this->getDirection()) {
			game->creatureTurn(this, newdir);
		}
	}
}

void Monster::setUpdateMovePos() {
	updateMovePos = true;
	route.clear();
}

bool Monster::calcMovePosition()
{
	bool foundDistPath = false;
	Position newMovePos = moveToPos;
	if(state == STATE_IDLESUMMON && isSummon()) {
		getCloseCombatPosition(getMaster()->getPosition(), newMovePos);
	}
	else {
		if(state == STATE_FLEEING) {
			getDistancePosition(targetPos, 10, true, newMovePos);
			foundDistPath = true;
		}
		else {
			bool fullPathSearch = !validateDistanceAttack(targetPos);
			if(getTargetDistance() > 1 /*&& hasDistanceAttack*/) {
				foundDistPath = getDistancePosition(targetPos, getTargetDistance(), fullPathSearch, newMovePos);
			}
		}

		//Close combat
		if(!foundDistPath) {
			int currentDist = getCurrentDistanceToTarget(targetPos);
			if(currentDist > getTargetDistance() || currentDist == 0) {
				getCloseCombatPosition(targetPos, newMovePos);
			}
		}
	}

	if(newMovePos == getPosition()) {
		route.clear();
		return false;
	}
	else if(newMovePos != moveToPos) {
		moveToPos = newMovePos;
		route.clear();
		return true;
	}
	else {
		//same move position as before
		return true;
	}
}

bool Monster::getDistancePosition(const Position &target, const int& maxTryDist, bool fullPathSearch, Position &dest)
{
	int minTryDist = 0;
	int currentDistance = getCurrentDistanceToTarget(target);
	if(currentDistance <= maxTryDist && validateDistanceAttack(target)) {

		if(!fullPathSearch) {
			minTryDist = currentDistance;
		}

		if(currentDistance == maxTryDist) {
			dest = getPosition();
			return true;
		}
	}

	std::vector<Position> positionList;
	Position tmpPos;
	int tryDist = 0;
	int prevDist = 0;

	int tryWalkDist = 0;
	int minWalkDist = 0;

	int xmindelta = ((fullPathSearch || (getPosition().x - target.x) <= 0) ? maxTryDist : 0);
	int xmaxdelta = ((fullPathSearch || (getPosition().x - target.x) >= 0) ? maxTryDist : 0);
	int ymindelta = ((fullPathSearch || (getPosition().y - target.y) <= 0) ? maxTryDist : 0);
	int ymaxdelta = ((fullPathSearch || (getPosition().y - target.y) >= 0) ? maxTryDist : 0);

	for (int y = target.y - ymindelta; y <= target.y + ymaxdelta; ++y) {
		for (int x = target.x - xmindelta; x <= target.x + xmaxdelta; ++x) {

			if((target.x == x && target.y == y))
				continue;

			//tryDist = std::sqrt( std::pow(std::abs(target.x - x), 2) + std::pow(std::abs(target.y - y), 2));
			tryDist = std::abs(target.x - x)*std::abs(target.x - x) + std::abs(target.y - y)*std::abs(target.y - y);

			tmpPos.x = x;
			tmpPos.y = y;
			tmpPos.z = getPosition().z;

			if(tryDist <= (maxTryDist * maxTryDist) && (tryDist >= prevDist) && (tryDist > (minTryDist * minTryDist))) {

				//if(!game->map->canThrowItemTo(tmpPos, target, false, true))
				if(game->map->canThrowObjectTo(tmpPos, target, BLOCK_PROJECTILE) != RET_NOERROR)
					continue;

				if(tmpPos != getPosition()) {
					if(!canMoveTo(x, y, getPosition().z))
						continue;
				}

				tryWalkDist = std::abs(getPosition().x - x)*std::abs(getPosition().x - x) + 
									std::abs(getPosition().y - y)*std::abs(getPosition().y - y);

				if(tryWalkDist > 0 && (tryWalkDist < minWalkDist || minWalkDist == 0)) {
					positionList.clear();
					minWalkDist = tryWalkDist;
				}
				
				else if(tryDist > prevDist) {
					positionList.clear();
				}

				prevDist = tryDist;
				positionList.push_back(tmpPos);
				//#ifdef __DEBUG__
				//				std::cout << "CalcMovePosition()" << ", x: " << tmpPos.x << ", y: " << tmpPos.y  << std::endl;
				//#endif
			}
		}
	}

	if(positionList.empty()){
		dest = getPosition();
		return currentDistance <= maxTryDist && (game->map->canThrowObjectTo(getPosition(), target, BLOCK_PROJECTILE) == RET_NOERROR);
		//return currentDistance <= maxTryDist && game->map->canThrowItemTo(this->pos, target, false, true);
	}

	size_t index = random_range(0, positionList.size() - 1);
	dest = positionList[index];
	return true;
}

bool Monster::getCloseCombatPosition(const Position &target, Position &dest)
{
	//Close combat
	std::vector<Position> positionList;
	Position tmpPos;
	int prevdist = 0;
	for(int y = target.y - 1; y <= target.y + 1; ++y) {
		for(int x = target.x - 1; x <= target.x + 1; ++x) {
			if((target.x == x && target.y == y))
				continue;

			int dist = std::max(std::abs(getPosition().x - x), std::abs(getPosition().y - y));

			tmpPos.x = x;
			tmpPos.y = y;
			tmpPos.z = getPosition().z;

			if(dist <= prevdist || (prevdist == 0)) {

				if(tmpPos != getPosition()) {
					if(!canMoveTo(x,y,getPosition().z))
						continue;
				}

				if(dist < prevdist)
					positionList.clear();

				prevdist = dist;
				positionList.push_back(tmpPos);
			}
		}
	}

	if(positionList.empty()){
		dest = getPosition();
		//dest = target;
		return false;
	}

	size_t index = random_range(0, positionList.size() - 1);
	dest = positionList[index];
	return true;
}

bool Monster::getRandomPosition(const Position &target, Position &dest)
{
	if(state == STATE_TARGETNOTREACHABLE) {
		std::vector<Position>	positionList;
		Position tmppos;

		tmppos.x = this->getPosition().x + 1;
		tmppos.y = this->getPosition().y;
		tmppos.z = this->getPosition().z;

		if(((std::abs(target.x - tmppos.x) <= 9) && (std::abs(target.y - tmppos.y) <= 7)) &&
			canMoveTo(tmppos.x, tmppos.y, tmppos.z))
			positionList.push_back(tmppos);

		tmppos.x = getPosition().x - 1;
		tmppos.y = getPosition().y;
		tmppos.z = getPosition().z;

		if(((std::abs(target.x - tmppos.x) <= 9) && (std::abs(target.y - tmppos.y) <= 7)) &&
			canMoveTo(tmppos.x, tmppos.y, tmppos.z))
			positionList.push_back(tmppos);

		tmppos.x = getPosition().x;
		tmppos.y = getPosition().y + 1;
		tmppos.z = getPosition().z;

		if(((std::abs(target.x - tmppos.x) <= 9) && (std::abs(target.y - tmppos.y) <= 7)) &&
			canMoveTo(tmppos.x, tmppos.y, tmppos.z))
			positionList.push_back(tmppos);

		tmppos.x = getPosition().x;
		tmppos.y = getPosition().y - 1;
		tmppos.z = getPosition().z;

		if(((std::abs(target.x - tmppos.x) <= 9) && (std::abs(target.y - tmppos.y) <= 7)) &&
			canMoveTo(tmppos.x, tmppos.y, tmppos.z))
			positionList.push_back(tmppos);

		if(!positionList.empty()) {
			size_t index = random_range(0, positionList.size() - 1);
			dest = positionList[index];
			return true;
		}
		else
			return false;
	}
	else if(getCurrentDistanceToTarget(target) <= 1) {
		//only close combat
		std::vector<Position>	positionList;
		Position randMovePos;
		for(int y = getPosition().y - 1; y <= getPosition().y + 1; ++y){
			for(int x = getPosition().x - 1; x <= getPosition().x + 1; ++x){
				if(std::abs(y - targetPos.y) <= 1 && std::abs(x - targetPos.x) <= 1 &&
					((std::abs(y - getPosition().y) == 1)^(std::abs(x - getPosition().x) == 1)) &&
					canMoveTo(x,y,getPosition().z)){
						randMovePos.x = x;
						randMovePos.y = y;
						randMovePos.z = getPosition().z;
						positionList.push_back(randMovePos);
					}
			}
		}

		if(positionList.empty())
			return false;

		size_t index = random_range(0, positionList.size() - 1);
		dest = positionList[index];
		return true;
	}

	return false;
}

bool Monster::isInRange(const Position &p)
{
	return ((std::abs(p.x - getPosition().x) <= 9) && (std::abs(p.y - getPosition().y) <= 7) &&
		(p.z == getPosition().z));
}

void Monster::onCreatureAppear(const Creature* creature, bool isLogin)
{
	//
}

void Monster::onCreatureDisappear(const Creature* creature, uint32_t stackpos, bool isLogout)
{
	//
}

void Monster::onCreatureMove(const Creature* creature, const Position& oldPos, uint32_t oldStackPos)
{
	//
}

void Monster::onTeleport(const Creature* creature, const Position& oldPos, uint32_t oldStackPos)
{
	//
}

/*
void Monster::onThingMove(const Creature *creature, const Thing* thing,
	const Position* oldPos, unsigned char oldstackpos, unsigned char oldcount, unsigned char count)
{
	const Creature* moveCreature = dynamic_cast<const Creature *>(thing);
	if(moveCreature && (moveCreature != this || creature != moveCreature)) {
		onCreatureMove(moveCreature, oldPos);
	}
	else
		reThink();
}
*/

/*
void Monster::onCreatureAppear(const Creature* creature)
{
	if(creature == this) {
		return;
	}

	if(isInRange(creature->getPosition())) {
		bool canReach = isCreatureReachable(creature);
		onCreatureEnter(creature, canReach);
	}
}
*/

/*
void Monster::onCreatureDisappear(const Creature* creature, unsigned char stackPos, bool tele)
{
	if(creature == this) {
		stopThink();
		return;
	}

	onCreatureLeave(creature);
}
*/

void Monster::onThingDisappear(const Thing* thing, unsigned char stackPos)
{
	const Creature *creature = dynamic_cast<const Creature*>(thing);

	if(creature == this) {
		stopThink();
		return;
	}

	if(creature) {
		creatureLeave(creature);
	}
	else {
		reThink();
	}
}

/*
void Monster::onThingTransform(const Thing* thing,int stackpos)
{
	const Item* item = dynamic_cast<const Item*>(thing);
	if(item && item->isSplash())
		return;

	reThink();
}
*/

void Monster::onThingAppear(const Thing* thing)
{
	const Creature *creature = dynamic_cast<const Creature*>(thing);
	if(creature && isInRange(creature->getPosition())){
		bool canReach = isCreatureReachable(creature);
		creatureEnter(creature, canReach);
	}	
	else {
		reThink();
		setUpdateMovePos();
	}
}

/*
void Monster::onTeleport(const Creature	*creature, const Position* oldPos, unsigned char oldstackpos)
{
	if(creature == this) {
		Creature* attackedCreature = game->getCreatureByID(this->attackedCreature);
		if(!attackedCreature || !isCreatureReachable(attackedCreature)) {
			state = STATE_TARGETNOTREACHABLE;
		}

		reThink();
	}
	else {
		onCreatureMove(creature, oldPos);
	}
}
*/

void Monster::creatureMove(const Creature* creature, const Position& oldPos)
{
	if(creature == this) {
		//We been pushed
		reThink();

	}
	else if(isInRange(creature->getPosition()) && isInRange(oldPos)) {
		//Creature just moving around in-range
		if(attackedCreature == creature->getID()) {
			if(state == STATE_ATTACKING && !isCreatureReachable(creature)) {

				if(isSummon()) {
					state = STATE_TARGETNOTREACHABLE;
					route.clear();
				}
				else {
					state = STATE_TARGETNOTREACHABLE;
					route.clear();

					reThink();
				}
			}
			else {
				targetPos = creature->getPosition();
				setUpdateMovePos();
			}
		}
		else if(isSummon()) {
			if(state == STATE_IDLESUMMON && getMaster() == creature) {
				startThink();
			}
			else
				reThink();
		}
		else if((state == STATE_IDLE || state == STATE_TARGETNOTREACHABLE)) {
			if(isCreatureAttackable(creature)) {
				bool canReach = isCreatureReachable(creature);
				if(canReach || (state == STATE_IDLE)) {
					selectTarget(creature, canReach);
				}
			}
		}
	}
	else if(isInRange(creature->getPosition()) && !isInRange(oldPos)) {
		bool canReach = isCreatureReachable(creature);
		creatureEnter(creature, canReach);
	}
	else if(!isInRange(creature->getPosition()) && isInRange(oldPos)) {
		creatureLeave(creature);
	}
}

void Monster::creatureEnter(const Creature* creature, bool canReach /* = true*/)
{
	if(isSummon()) {
		//master found
		if(creature == getMaster()) {
			hasLostMaster = false;
			startThink();
		}
		//target enters
		else if(attackedCreature == creature->getID()) {
			startThink();
		}
		else
			reThink();
	}
	else if(state == STATE_IDLE || (state == STATE_TARGETNOTREACHABLE && canReach)) {
		if(isCreatureAttackable(creature)) {
			selectTarget(creature, canReach);
		}
	}
}

void Monster::creatureLeave(const Creature *creature)
{
	if(creature == this) {
		stopThink();
	}
	else if(isSummon()) {
		//master out of reach
		if(creature == getMaster()) {
			hasLostMaster = true;
			stopThink();
		}
		//target lost
		else if(attackedCreature == creature->getID()) {
			stopThink();
		}
		else
			reThink();
	}
	else {
		if(creature && attackedCreature == creature->getID()) {
			//try find a new target
			bool canReach;
			Creature *target = findTarget(0, canReach, creature);
			if(target) {
				selectTarget(target, canReach);
			}
			else {
				stopThink();
			}
		}
		else
			reThink();
	}
}

void Monster::selectTarget(const Creature* creature, bool canReach /* = true*/)
{
	Creature::setAttackedCreature(creature);
	targetPos = creature->getPosition();

	//start fleeing?
	if(!isSummon() && mType->runAwayHealth > 0 && this->health <= mType->runAwayHealth) {
		state = STATE_FLEEING;
	}
	else if(canReach) {
		state = STATE_ATTACKING;

		updateLookDirection();
		Creature *attackedCreature = const_cast<Creature*>(creature);

		if(attackedCreature) {
			if(validateDistanceAttack(attackedCreature)){
				doAttacks(attackedCreature, (getMaster() ? MODE_NORMAL : MODE_AGGRESSIVE));
			}
		}
	}
	else {
		state = STATE_TARGETNOTREACHABLE;
	}

	startThink();
}

//something changed, check if we should change state
void Monster::reThink(bool updateOnlyState /* = true*/)
{
	if(isSummon()) {
		//try find a path to the target
		if(state == STATE_TARGETNOTREACHABLE) {
			if(updateOnlyState) {
				setUpdateMovePos();
			}
			else if(calcMovePosition()) {
				state = STATE_ATTACKING;
				return;
			}
		}

		if(state == STATE_ATTACKING) {
			Creature *attackedCreature = game->getCreatureByID(this->attackedCreature);
			if (!attackedCreature || !isCreatureReachable(attackedCreature)) {
				state = STATE_TARGETNOTREACHABLE;
				route.clear();
			}
		}

		if(!updateMovePos && route.empty()) {
			//to far away from master?
			if(state == STATE_IDLESUMMON && getCurrentDistanceToTarget(getMaster()->getPosition()) > 1) {
				setUpdateMovePos();
			}
			//
			//to far away from target?
			else if(state != STATE_IDLESUMMON && getCurrentDistanceToTarget(targetPos) > getTargetDistance()) {
				setUpdateMovePos();
			}
			//
		}
	}
	else {
		//change target
		if(state != STATE_IDLE) {
			if(mType->changeTargetChance > rand()*10000/(RAND_MAX+1)){
				bool canReach;
				Creature *newtarget = findTarget(3, canReach);
				if(newtarget && (canReach || state == STATE_TARGETNOTREACHABLE) && newtarget->getID() != attackedCreature){
					selectTarget(newtarget, canReach);
				}
			}
		}

		if(state == STATE_FLEEING) {
			if(this->health > mType->runAwayHealth || !isInRange(targetPos)) {
				bool canReach;
				Creature *newtarget = findTarget(0, canReach);
				if(newtarget) {
					selectTarget(newtarget, canReach);
				}
				else {
					setAttackedCreature(NULL);
				}
			}
		}

		if(state == STATE_ATTACKING) {
			if(mType->runAwayHealth > 0 && this->health <= mType->runAwayHealth) {
				state = STATE_FLEEING;
				setUpdateMovePos();
			}
		}

		if(state == STATE_ATTACKING) {
			Creature *attackedCreature = game->getCreatureByID(this->attackedCreature);
			if (!attackedCreature || !isCreatureReachable(attackedCreature)) {
				state = STATE_TARGETNOTREACHABLE;
				route.clear();
			}
		}

		if(state == STATE_ATTACKING) {
			//summons
			if(summons.size() < mType->maxSummons) {
				SummonSpells::const_iterator it;
				for(it = mType->summonSpells.begin(); it != mType->summonSpells.end(); ++it) {
					if(getRandom() < (*it).summonChance) {
						//new Monster((*it).name, game);
						Monster *summon = createMonster((*it).name, game);
						if(summon){
							Position summonPos = getPosition();

							if(!game->placeCreature(summonPos, summon)){
								delete summon;
							}
							else{
								addSummon(summon);
							}
						}
					}
				}
			}

			//static walking
			if(getTargetDistance() <= 1 && getCurrentDistanceToTarget(targetPos) == 1) {
				if(rand() % mType->staticAttack == 0) {
					Position newMovePos;
					if(getRandomPosition(targetPos, newMovePos)) {
						moveToPos = newMovePos;
					}
				}
			}
			//to far away from target?
			else if(route.empty() && getCurrentDistanceToTarget(targetPos) > getTargetDistance()) {
				setUpdateMovePos();
			}
		}

		if(state == STATE_TARGETNOTREACHABLE) {
			//try find a target
			bool canReach;
			Creature* target = findTarget(0, canReach);
			if(target) {
				if(canReach || target->getID() != attackedCreature) {
					selectTarget(target, canReach);
				}
			}
			else {
				setAttackedCreature(NULL);
			}
		}
	}
}

void Monster::startThink()
{
	//Update move	position
	setUpdateMovePos();

	if(!eventCheck){
		eventCheck = game->addEvent(makeTask(500, std::bind2nd(std::mem_fun(&Game::checkCreature), getID())));
	}

	if(attackedCreature != 0 && !(isSummon() && hasLostMaster)) {
		if(!eventCheckAttacking){
			eventCheckAttacking = game->addEvent(makeTask(500, std::bind2nd(std::mem_fun(&Game::checkCreatureAttacking), getID())));
		}
	}
}

void Monster::stopThink()
{
	if(isSummon()) {
		state = STATE_IDLESUMMON;
		setUpdateMovePos();
	}
	else
		state = STATE_IDLE;

	std::list<Creature*>::iterator cit;
	for(cit = summons.begin(); cit != summons.end(); ++cit) {
		(*cit)->setAttackedCreature(NULL);
	}

	//route.clear();
	//updateRoute = true;
	setUpdateMovePos();
	attackedCreature = 0;

	targetPos.x = 0;
	targetPos.y = 0;
	targetPos.z = 0;

	if(state == STATE_IDLE) {
		game->stopEvent(eventCheck);
		eventCheck = 0;
	}

	game->stopEvent(eventCheckAttacking);
	eventCheckAttacking = 0;
}

void Monster::setMaster(Creature* creature)
{
	Creature::setMaster(creature);

	if(creature) {
		Creature* attackedCreature = dynamic_cast<Creature*>(game->getCreatureByID(creature->attackedCreature));
		if(attackedCreature) {
			bool canReach = isCreatureReachable(attackedCreature);
			selectTarget(attackedCreature, canReach);
		}
		else {
			state = STATE_IDLESUMMON;
			startThink();
		}
	}
	else
		state = STATE_IDLE;
}

void Monster::setAttackedCreature(const Creature* creature)
{
	if(creature == NULL || creature == this) {
		//Creature::setAttackedCreature(NULL);
		stopThink();
	}
	else {
		//Creature::setAttackedCreature(creature);

		if(creature) {
			bool canReach = isCreatureReachable(creature);
			selectTarget(creature, canReach);
		}
		else {
			Creature::setAttackedCreature(NULL);
		}
	}
}

std::string Monster::getDescription(uint32_t lookDistance) const
{
	std::stringstream ss;
	std::string str;

	str = getName();
	std::transform(str.begin(), str.end(), str.begin(), tolower);

	ss << "a " << str << ".";
	str = ss.str();

	return str;
}

int Monster::getWeaponDamage() const
{
	if(curPhysicalAttack != NULL)
		return random_range(curPhysicalAttack->minWeapondamage, curPhysicalAttack->maxWeapondamage);
	else
		return 0;
}

void Monster::dropLoot(Container *corpse)
{
	if(isSummon())
		return;

	mType->createLoot(corpse);
}

bool Monster::doAttacks(Creature* attackedCreature, monstermode_t mode /*= MODE_NORMAL*/)
{
	int modeProb = 0;
	switch(mode) {
		case MODE_NORMAL: modeProb = 0; break;
		case MODE_AGGRESSIVE: modeProb = 50; break;
	}

	bool ret = false;

	bool trymelee = getCurrentDistanceToTarget(targetPos) <= 1;
	bool hasmelee = false;
	if(trymelee){
		for(PhysicalAttacks::iterator paIt = mType->physicalAttacks.begin(); paIt != mType->physicalAttacks.end(); ++paIt) {
			if(paIt->first->fighttype == FIGHT_MELEE) {
				hasmelee = true;
				break;
			}
		}
	}

	for(PhysicalAttacks::iterator paIt = mType->physicalAttacks.begin(); paIt != mType->physicalAttacks.end(); ++paIt) {
		TimeProbabilityClass& timeprobsystem = paIt->second;
		if(timeprobsystem.onTick(500) || (random_range(1, 100) <= modeProb)) {
			if(!hasmelee || (hasmelee && paIt->first->fighttype == FIGHT_MELEE)) {
				curPhysicalAttack = paIt->first;
				//game->creatureMakeDamage(this, attackedCreature, getFightType());
			}
		}
	}

	if(exhaustedTicks <= 0) {
		for(RuneAttackSpells::iterator raIt = mType->runeSpells.begin(); raIt != mType->runeSpells.end(); ++raIt) {
			for(TimeProbabilityClassVec::iterator asIt = raIt->second.begin(); asIt != raIt->second.end(); ++asIt) {
				TimeProbabilityClass& timeprobsystem = *asIt;
				if(timeprobsystem.onTick(500) || (random_range(1, 100) <= modeProb)) {

					std::map<unsigned short, Spell*>::iterator rit = spells.getAllRuneSpells()->find(raIt->first);
					if(rit != spells.getAllRuneSpells()->end()) {
						bool success = rit->second->getSpellScript()->castSpell(this, attackedCreature->getPosition(), "");

						if(success) {
							ret = true;
							exhaustedTicks = timeprobsystem.getExhaustion();
							if(exhaustedTicks > 0) {
								return true;
							}
						}
					}
				}
			}
		}

		for(InstantAttackSpells::iterator iaIt = mType->instantSpells.begin(); iaIt != mType->instantSpells.end(); ++iaIt) {
			for(TimeProbabilityClassVec::iterator asIt = iaIt->second.begin(); asIt != iaIt->second.end(); ++asIt) {
				TimeProbabilityClass& timeprobsystem = *asIt;
				if(timeprobsystem.onTick(500) || (random_range(1, 100) <= modeProb)) {

					std::map<std::string, Spell*>::iterator rit = spells.getAllSpells()->find(iaIt->first);
					if(rit != spells.getAllSpells()->end()) {
						bool success = rit->second->getSpellScript()->castSpell(this, getPosition(), "");

						if(success) {
							ret = true;
							exhaustedTicks = timeprobsystem.getExhaustion();
							if(exhaustedTicks > 0) {
								return true;
							}
						}
					}
				}
			}
		}
	}

	return ret;
}

void Monster::onAttack()
{
	if (attackedCreature != 0 && !(isSummon() && hasLostMaster)) {
		this->eventCheckAttacking = game->addEvent(makeTask(500, std::bind2nd(std::mem_fun(&Game::checkCreatureAttacking), getID())));

		exhaustedTicks -= 500;

		if(exhaustedTicks < 0)
			exhaustedTicks = 0;

		Creature* attackedCreature = game->getCreatureByID(this->attackedCreature);

		if(attackedCreature && attackedCreature->isAttackable()) {
			if(validateDistanceAttack(attackedCreature)){
				doAttacks(attackedCreature);
			}
		}
		else
			setAttackedCreature(NULL);
	}
}

void Monster::doMoveTo(int dx, int dy)
{
	if(mType->canPushItems){
		//move/destroy blocking moveable items
		Tile *tile =game->getTile(getPosition().x + dx ,getPosition().y + dy ,getPosition().z);
		if(tile){
			int countItems = 0;
			countItems = 0;
			while(Item* blockItem = tile->getMoveableBlockingItem()){
				if(countItems < 2){
					if(!monsterMoveItem(blockItem, 3)){
						//destroy it
						//if(game->removeThing(NULL, blockItem->getPosition(), blockItem)){
						//	game->FreeThing(blockItem);
						//}
					}
				}
				else{
					//destroy items directly
					//if(game->removeThing(NULL, blockItem->getPosition(), blockItem)){
					//	game->FreeThing(blockItem);
					//}
				}
				countItems++;
			}
		}
	}

	Position moveTo = getPosition();
	moveTo.x += dx;
	moveTo.y += dy;

	//this->game->thingMove(this, this, moveTo.x, moveTo.y, moveTo.z, 1);

	if(moveTo != getPosition()) {
		setUpdateMovePos();
	}
	else if(state == STATE_ATTACKING && getPosition() == moveToPos) {
		updateLookDirection();
	}
}

bool Monster::monsterMoveItem(Item* item, int radius)
{
	Position centerPos = item->getPosition();
	Position tryPos;

	int itemCount;
	if(item->isStackable()){
		itemCount = item->getItemCountOrSubtype();
	}
	else{
		itemCount = 1;
	}
	//try random position in radius
	tryPos.z = item->getPosition().z;
	for(int i = 0; i < 4*radius; i++){
		tryPos.x = item->getPosition().x + rand() % radius;
		tryPos.y = item->getPosition().y + rand() % radius;
		//if(game->map->canThrowItemTo(item->pos,tryPos)){
		if(game->map->canThrowObjectTo(item->getPosition(), tryPos, BLOCK_SOLID) == RET_NOERROR){
			Tile* fromTile = game->getTile(item->getPosition().x, item->getPosition().y, item->getPosition().z);
			//int oldstackpos = fromTile->getThingStackPos(item);
			//game->thingMoveInternal(this, item->getPosition().x, item->getPosition().y, item->getPosition().z,
			//	oldstackpos, item->getID(), tryPos.x, tryPos.y, tryPos.z, count);

			if(item->getPosition() == tryPos){
				return true;
			}
		}
	}
	return false;
}

bool Monster::canMoveTo(unsigned short x, unsigned short y, unsigned char z)
{
	Tile *t = game->map->getTile(x, y, getPosition().z);
	if(t) {
		return (t->isBlocking(BLOCK_SOLID | BLOCK_PATHFIND | BLOCK_PZ, false, mType->canPushItems) == RET_NOERROR);
	}

	return false;

	/*
	Tile *t;
	if((!(t = game->map->getTile(x, y, pos.z))) ||
		t->isBlocking() ||
		t->isPz() || 
		!t->creatures.empty() ||
		t->floorChange() ||
		t->getTeleportItem()){
			return false;
		}
	else{
		return true;
	}
	*/
}

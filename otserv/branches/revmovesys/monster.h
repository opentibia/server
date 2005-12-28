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


#ifndef __MONSTER_H__
#define __MONSTER_H__

#include "creature.h"
#include "game.h"
#include "tile.h"
#include "templates.h"
#include "monsters.h"

class Creature;

enum monsterstate_t{
	STATE_IDLE,
	STATE_IDLESUMMON,
	STATE_TARGETNOTREACHABLE,
	STATE_ATTACKING,
	STATE_FLEEING,
};

enum monstermode_t{
	MODE_NORMAL,
	MODE_AGGRESSIVE
};

class Monster : public Creature
{
private:
	Monster(MonsterType *mtype, Game* game);
public:
	static Monster* createMonster(const std::string& name, Game* game);

	virtual ~Monster();
	//const Monster& operator=(const Monster& rhs);

	virtual Monster* getMonster() {return this;};
	virtual const Monster* getMonster() const {return this;};

	virtual const std::string& getName() const;
	virtual bool isPushable() const;

	virtual unsigned long idRange(){ return 0x40000000;}
	static AutoList<Monster> listMonster;
	void removeList() {listMonster.removeList(getID());}
	void addList() {listMonster.addList(this);}
	
	virtual int getArmor() const;
	virtual int getDefense() const;
	
	virtual void setMaster(Creature* creature);
	bool isSummon() {return (getMaster() != NULL);}
	virtual void onAttack();
	
	static unsigned long getRandom();
	
private:
	Game* game;
	std::list<Position> route;
	monsterstate_t state;
	bool updateMovePos;
	int oldThinkTicks;
	Position targetPos;
	Position moveToPos;
	bool hasLostMaster;
	bool isYielding;
	MonsterType *mType;
	
	void doMoveTo(int dx, int dy);
	
	int getCurrentDistanceToTarget(const Position &target);
	int getTargetDistance();
	void setUpdateMovePos();
	bool calcMovePosition();
	void updateLookDirection();
	
	bool getRandomPosition(const Position &target, Position &dest);
	bool getDistancePosition(const Position &target, const int& maxTryDist, bool fullPathSearch, Position &dest);
	bool getCloseCombatPosition(const Position &target, Position &dest);
	bool canMoveTo(unsigned short x, unsigned short y, unsigned char z);
	bool isInRange(const Position &pos);
	bool isCreatureReachable(const  Creature* creature);
	Creature* findTarget(long range, bool &canReach, const Creature *ignoreCreature = NULL);
	void stopAttack();
	void startThink();
	void stopThink();
	void reThink(bool updateOnlyState = true);
	
	void selectTarget(const Creature* creature, bool canReach /* = true*/);

protected:
	PhysicalAttackClass	*curPhysicalAttack;
		
	bool doAttacks(Creature* attackedCreature, monstermode_t mode = MODE_NORMAL);
		
	virtual fight_t getFightType() {return curPhysicalAttack->fighttype;};
	virtual subfight_t getSubFightType()  {return curPhysicalAttack->disttype;}
	virtual int getWeaponDamage() const;
	
	void creatureEnter(const Creature *creature, bool canReach = true);
	void creatureLeave(const Creature *creature);
	void creatureMove(const Creature *creature, const Position& oldPos);
	
	bool validateDistanceAttack(const Creature *creature);
	bool validateDistanceAttack(const Position &pos);
	bool monsterMoveItem(Item* item, int radius);
	bool isCreatureAttackable(const Creature* creature);
	
	virtual int getLostExperience();
	virtual void dropLoot(Container *corpse);
	
	virtual void onAddTileItem(const Position& pos, const Item* item);
	virtual void onUpdateTileItem(const Position& pos, uint32_t stackpos, const Item* oldItem, const Item* newItem);
	virtual void onRemoveTileItem(const Position& pos, uint32_t stackpos, const Item* item);
	virtual void onUpdateTile(const Position& pos);

	virtual void onCreatureAppear(const Creature* creature, bool isLogin);
	virtual void onCreatureDisappear(const Creature* creature, uint32_t stackpos, bool isLogout);
	virtual void onCreatureMove(const Creature* creature, const Position& oldPos, uint32_t oldStackPos, bool teleport);

	virtual bool isAttackable() const { return true; };
	
	virtual int onThink(int& newThinkTicks);
	virtual void setAttackedCreature(const Creature* creature);
	
	virtual std::string getDescription(int32_t lookDistance) const;
};

#endif

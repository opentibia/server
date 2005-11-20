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


#ifndef __monster_h_
#define __monster_h_

#include "creature.h"
#include "game.h"
#include "tile.h"
#include "templates.h"
#include "monsters.h"

class Creature;

enum monsterstate_t {
	STATE_IDLE,
	STATE_IDLESUMMON,
	STATE_TARGETNOTREACHABLE,
	STATE_ATTACKING,
	STATE_FLEEING,
};

enum monstermode_t {
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
	int useCount;
	PhysicalAttackClass	*curPhysicalAttack;
		
	bool doAttacks(Creature* attackedCreature, monstermode_t mode = MODE_NORMAL);
		
	virtual fight_t getFightType() {return curPhysicalAttack->fighttype;};
	virtual subfight_t getSubFightType()  {return curPhysicalAttack->disttype;}
	virtual int getWeaponDamage() const;
	
	void onCreatureEnter(const Creature *creature, bool canReach = true);
	void onCreatureLeave(const Creature *creature);
	void onCreatureMove(const Creature *creature, const Position *oldPos);
	
	bool validateDistanceAttack(const Creature *creature);
	bool validateDistanceAttack(const Position &pos);
	bool monsterMoveItem(Item* item, int radius);
	bool isCreatureAttackable(const Creature* creature);
	
	virtual int getLostExperience();
	virtual void dropLoot(Container *corpse);
	
	virtual void onThingMove(const Creature *creature, const Thing *thing, const Position *oldPos,
		unsigned char oldstackpos, unsigned char oldcount, unsigned char count);
	
	virtual void onCreatureAppear(const Creature *creature);
	virtual void onCreatureDisappear(const Creature *creature, unsigned char stackPos, bool tele);
	virtual void onThingDisappear(const Thing* thing, unsigned char stackPos);
	virtual void onThingTransform(const Thing* thing,int stackpos);
	virtual void onThingAppear(const Thing* thing);
	virtual void onTeleport(const Creature *creature, const Position *oldPos, unsigned char oldstackpos);
	
	virtual bool isAttackable() const { return true; };
	
	virtual int onThink(int& newThinkTicks);
	virtual void setAttackedCreature(const Creature* creature);
	
	virtual std::string getDescription(uint32_t lookDistance) const;
};

#endif // __monster_h_

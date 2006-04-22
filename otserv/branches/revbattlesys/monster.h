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


#ifndef __OTSERV_MONSTER_H__
#define __OTSERV_MONSTER_H__

#include "tile.h"
#include "monsters.h"

class Creature;
class Game;

/*
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
*/

class Monster : public Creature
{
private:
	Monster(MonsterType* mtype);
	//const Monster& operator=(const Monster& rhs);

public:
	static Monster* createMonster(const std::string& name);
	virtual ~Monster();

	virtual Monster* getMonster() {return this;};
	virtual const Monster* getMonster() const {return this;};

	virtual unsigned long idRange(){ return 0x40000000;}
	static AutoList<Monster> listMonster;
	void removeList() {listMonster.removeList(getID());}
	void addList() {listMonster.addList(this);}
	
	virtual const std::string& getName() const {return mType->name;}
	virtual const std::string& getNameDescription() const {return mType->nameDescription;}
	virtual std::string getDescription(int32_t lookDistance) const;

	virtual int getArmor() const {return mType->armor;}
	virtual int getDefense() const {return mType->defense;}
	virtual bool isPushable() const {return mType->pushable;}

	virtual void setNormalCreatureLight();

	virtual void onAddTileItem(const Position& pos, const Item* item);
	virtual void onUpdateTileItem(const Position& pos, uint32_t stackpos, const Item* oldItem, const Item* newItem);
	virtual void onRemoveTileItem(const Position& pos, uint32_t stackpos, const Item* item);
	virtual void onUpdateTile(const Position& pos);

	virtual void onCreatureAppear(const Creature* creature, bool isLogin);
	virtual void onCreatureDisappear(const Creature* creature, uint32_t stackpos, bool isLogout);
	virtual void onCreatureMove(const Creature* creature, const Position& oldPos, uint32_t oldStackPos, bool teleport);

	virtual void onThink(uint32_t interval);

	bool canPushItems() const {return mType->canPushItems;}
	bool isSummon() const { return isSummon; }

private:
	std::string strDescription;

	typedef std::list<Creature*> TargetList;
	Creature* targetCreature;
	TargetList targetList;
	MonsterType* mType;

	void startThink();
	void stopThink();
	int getTargetDistance() {return mType->targetDistance;}

	virtual int32_t getLostExperience() const {return (isSummon() ? 0 : mType->experience);}
	virtual void dropLoot(Container* corpse);
};

#endif

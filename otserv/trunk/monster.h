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


class Monster : public Creature
{
public:
  Monster(const char *name, Game* game);
  virtual ~Monster();

	virtual void onAttack();
	bool isLoaded() const {return loaded;}

private:
	bool isInRange(const Position &pos);
	std::list<Position> route;

protected:
	Game* game;

	fight_t fighttype;
	subfight_t disttype;
	int minWeapondamage;
	int maxWeapondamage;

	std::vector<std::string> instantSpells;
	std::vector<unsigned short> runeSpells;
	
	Position targetPos;
	void doMoveTo(const Position &target);

	virtual fight_t getFightType(){return fighttype;};
	virtual subfight_t getSubFightType() {return disttype;}
	virtual int getWeaponDamage() const;

	void OnCreatureEnter(const Creature *creature);
	void OnCreatureLeave(const Creature *creature);

	virtual void onThingMove(const Creature *creature, const Thing *thing, const Position *oldPos, unsigned char oldstackpos);
  virtual void onCreatureAppear(const Creature *creature);
  virtual void onCreatureDisappear(const Creature *creature, unsigned char stackPos);
  virtual void onTeleport(const Creature *creature, const Position *oldPos, unsigned char oldstackpos);

  //virtual void onCreatureTurn(const Creature *creature, unsigned char stackpos);
  //virtual void onCreatureSay(const Creature *creature, unsigned char type, const std::string &text);
  //virtual void onCreatureChangeOutfit(const Creature* creature);
	virtual void onThink();
  virtual void setAttackedCreature(unsigned long id);
  virtual std::string getDescription() const;
  std::string monstername;
	bool loaded;
};

#endif // __monster_h_

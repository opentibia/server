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

#ifndef __player_h_
#define __player_h_


#include "creature.h"


class Protocol;


enum skills_t {
    SKILL_FIST,
    SKILL_CLUB,
    SKILL_SWORD,
    SKILL_AXE,
    SKILL_DIST,
    SKILL_SHIELD,
    SKILL_FISH
};

enum skillsid_t {
    SKILL_LEVEL,
    SKILL_TRIES
};

class NetworkMessage;

//////////////////////////////////////////////////////////////////////
// Defines a player...

class Player : public Creature
{
public:
	Player(const char *name, Protocol* p);
	virtual ~Player();

  virtual bool isPlayer() const { return true; };

  void speak(const std::string &text);

	int addItem(Item* item, int pos);
	int sendInventory();

	Item* getItem(int pos);

	std::string getName(){return name;};

  int access; //access level
  int sex, voc;
  int cap;

  int mana, manamax, manaspent;

  int food;


  // level
  int level;
  // experience
  unsigned long experience;
  // magic level
  int maglevel;

  int skills[7][2];

  void    usePlayer() { useCount++; };
  void    releasePlayer() { useCount++; if (useCount == 0) delete this; };

  void    setAttackedCreature(unsigned long id);

  unsigned long attackedCreature;

  bool CanSee(int x, int y);
  void sendNetworkMessage(NetworkMessage *msg);

protected:
  bool    useCount;

  virtual void onThingMove(const Player *player, const Thing *thing, const Position *oldPos, unsigned char oldstackpos);
  virtual void onCreatureAppear(const Creature *creature);
  virtual void onCreatureDisappear(const Creature *creature, unsigned char stackPos);
  virtual void onCreatureTurn(const Creature *creature, unsigned char stackpos);
  virtual void onCreatureSay(const Creature *creature, unsigned char type, const std::string &text);
  virtual void onCreatureChangeOutfit(const Creature* creature);

	Protocol *client;

	// we need our name and password...
	std::string name, password;
};


#endif // __player_h_

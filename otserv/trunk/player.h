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


enum slots_t {
	SLOT_WHEREEVER=0,
	SLOT_HEAD=1,
	SLOT_NECKLACE=2,
	SLOT_BACKPACK=3,
	SLOT_ARMOR=4,
	SLOT_RIGHT=5,
	SLOT_LEFT=6,
	SLOT_LEGS=7,
	SLOT_FEET=8,
	SLOT_RING=9,
	SLOT_AMMO=10
};

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

  void speak(const std::string &text);

	int addItem(Item* item, int pos);
	unsigned int getContainerCount() {return vcontainers.size();}; //returns the current number of containers open
	Item* getContainer(unsigned char containerid);
	unsigned char getContainerID(Item* container);
	void addContainer(unsigned char containerid, Item *container);
	void closeContainer(unsigned char containerid);
	int sendInventory();

	Item* getItem(int pos);

	std::string getName(){return name;};
	
  int sex, voc;
  int cap;

  int food;

  virtual int getWeaponDamage() const;
  virtual unsigned short getSpeed() const;

  // experience
  unsigned long experience;

  int skills[7][2];

  //items
  Item* items[11]; //equipement of the player
	typedef std::pair<unsigned char, Item*> containerItem;
	typedef std::vector<containerItem> containerLayout;
	containerLayout vcontainers;

  void    usePlayer() { useCount++; };
  void    releasePlayer() { useCount--; if (useCount == 0) delete this; };

  fight_t getFightType();

  bool CanSee(int x, int y);
  void sendNetworkMessage(NetworkMessage *msg);

  virtual void sendCancel(const char *msg);
  virtual void sendCancelWalk(const char *msg);

  virtual bool isAttackable() { return (access == 0); };

protected:
  int useCount;

  virtual void onThingMove(const Creature *creature, const Thing *thing, const Position *oldPos, unsigned char oldstackpos);
  virtual void onCreatureAppear(const Creature *creature);
  virtual void onCreatureDisappear(const Creature *creature, unsigned char stackPos);
  virtual void onCreatureTurn(const Creature *creature, unsigned char stackpos);
  virtual void onCreatureSay(const Creature *creature, unsigned char type, const std::string &text);
  virtual void onCreatureChangeOutfit(const Creature* creature);
  virtual void onThink();
	virtual void onTileUpdated(const Position *Pos);
	virtual void onContainerUpdated(Item *item, unsigned char from_id, unsigned char to_id, unsigned char from_slot, unsigned char to_slot, bool sameview);
	Protocol *client;

	// we need our name and password...
	std::string name, password;
};


#endif // __player_h_

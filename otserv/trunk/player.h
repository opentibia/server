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
#include "container.h"
#include <vector>
#include <algorithm>

class Protocol;

enum skills_t {
    SKILL_FIST=0,
    SKILL_CLUB=1,
    SKILL_SWORD=2,
    SKILL_AXE=3,
    SKILL_DIST=4,
    SKILL_SHIELD=5,
    SKILL_FISH=6
};

enum skillsid_t {
    SKILL_LEVEL=0,
    SKILL_TRIES=1
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
	unsigned char getContainerCount() {return (unsigned char)vcontainers.size();}; //returns the current number of containers open
	Container* getContainer(unsigned char containerid);
	unsigned char getContainerID(const Container* container) const;
	void addContainer(unsigned char containerid, Container *container);
	void closeContainer(unsigned char containerid);
	int sendInventory();

	Item* getItem(int pos) const;

	std::string getName(){return name;};
	
  int sex, voc;
  int cap;
  int food;
  bool cancelMove;
  virtual int getWeaponDamage() const;
  char fightMode, followMode;
  int accountNumber;
  
  std::string password;

  Item* items[11]; //equipement of the player
  unsigned int skills[7][2];
  
  //reminder: 0 = None, 1 = Sorcerer, 2 = Druid, 3 = Paladin, 4 = Knight
  unsigned short CapGain[5];          //for level advances
  unsigned short ManaGain[5];
  unsigned short HPGain[5];
  
  //for skill advances
  unsigned int getReqSkilltries (int skill, int level, int voc);
  
  //for magic level advances
  unsigned int getReqMana(int maglevel, int voc); 
  
  //items
	typedef std::pair<unsigned char, Container*> containerItem;
	typedef std::vector<containerItem> containerLayout;
	containerLayout vcontainers;
  void preSave();
  void    usePlayer() { useCount++; };
  void    releasePlayer() { useCount--; if (useCount == 0) delete this; };
	unsigned long getIP() const;

  fight_t getFightType();
	subfight_t getSubFightType();

  void sendIcons();
  bool CanSee(int x, int y, int z) const;
  void addSkillTry(int skilltry);
  void sendNetworkMessage(NetworkMessage *msg);
  void sendCancelAttacking();
  void sendChangeSpeed(Creature* creature);
  void sendToChannel(Creature *creature, unsigned char type, const std::string &text, unsigned short channelId);
  void die();      //player loses exp/skills/maglevel on death
  
  virtual void sendCancel(const char *msg);
  virtual void sendCancelWalk(const char *msg);
  virtual void setAttackedCreature(unsigned long id);
  virtual bool isAttackable() const { return (access == 0); };
  virtual bool isPushable() const { return (access == 0); };
	virtual void dropLoot(Container *corpse);

protected:
  int useCount;

  virtual void onCreatureAppear(const Creature *creature);
  virtual void onCreatureDisappear(const Creature *creature, unsigned char stackPos);
  virtual void onCreatureTurn(const Creature *creature, unsigned char stackpos);
  virtual void onCreatureSay(const Creature *creature, unsigned char type, const std::string &text);
  virtual void onCreatureChangeOutfit(const Creature* creature);
  virtual void onTeleport(const Creature *creature, const Position *oldPos, unsigned char oldstackpos); 
  virtual int onThink(int& newThinkTicks);
  virtual std::string getDescription(bool self = false) const;
	virtual void onTileUpdated(const Position &pos);
	//virtual void onContainerUpdated(Item *item, unsigned char from_id, unsigned char to_id, unsigned char from_slot, unsigned char to_slot, bool remove);

	//container to container
	virtual void onThingMove(const Creature *creature, const Container *fromContainer, Container *toContainer,
		const Item* item, unsigned char from_slotid, unsigned char to_slotid, unsigned char oldcount, unsigned char count);

	//container to ground
	virtual void onThingMove(const Creature *creature, const Container *fromContainer, const Position *newPos,
		const Item* item, unsigned char from_slotid, unsigned char oldcount, unsigned char count);

	//inventory to ground
	virtual void onThingMove(const Creature *creature, slots_t fromSlot, const Position *newPos,
		const Item* item, unsigned char oldcount, unsigned char count);

	//ground to container
	virtual void onThingMove(const Creature *creature, const Position *oldPos, const Container *toContainer,
		const Item* item, unsigned char stackpos, unsigned char to_slotid, unsigned char oldcount, unsigned char count);

	//ground to inventory
	virtual void onThingMove(const Creature *creature, const Position *oldPos, slots_t toSlot,
		const Item* item, unsigned char stackpos, unsigned char oldcount, unsigned char count);

  //ground to ground
	virtual void onThingMove(const Creature *creature, const Thing *thing, const Position *oldPos,
		unsigned char oldstackpos, unsigned char oldcount, unsigned char count);

	Protocol *client;

	// we need our name
	std::string name;
};


#endif // __player_h_

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
    SKILL_TRIES=1,
    SKILL_PERCENT=2
};

class NetworkMessage;

typedef std::pair<unsigned char, Container*> containerItem;
typedef std::vector<containerItem> containerLayout;
typedef std::map<unsigned long, Container*> DepotMap;

//////////////////////////////////////////////////////////////////////
// Defines a player...

class Player : protected AutoID<Player>, public AutoList<Player>, public Creature
{
public:
	Player(const char *name, Protocol* p);
	virtual ~Player();
	void setGUID(unsigned long _guid) {guid = _guid;};
	unsigned long getGUID() const { return guid;};


	static const unsigned long min_id = 16777217U;
	static const unsigned long max_id = 4294967295U;

  void speak(const std::string &text);

	int addItem(Item* item, int pos, bool isloading = false);

	containerLayout::const_iterator getContainers() const { return vcontainers.begin();}
	containerLayout::const_iterator getEndContainer() const { return vcontainers.end();}

	Container* getContainer(unsigned char containerid);
	unsigned char getContainerID(const Container* container) const;
	void addContainer(unsigned char containerid, Container *container);
	void closeContainer(unsigned char containerid);

	Item* getItem(int pos) const;
	Item* GetDistWeapon() const;
	
	std::string getName(){return name;};
	
  int sex, voc;
  int cap;
  int food;
  unsigned char level_percent;
  unsigned char maglevel_percent;
  bool cancelMove;
  bool SendBuffer;
  long internal_ping;
  long npings;
  virtual int getWeaponDamage() const;
  virtual int getArmor() const;
  virtual int getDefense() const;
  unsigned long getMoney();
  unsigned long getMoneyContainer(Container *container);
  bool substractMoney(unsigned long money);
  bool substractMoneyContainer(Container *container, unsigned long *money);
  char fightMode, followMode;
  int accountNumber;
  
  std::string password;

  Item* items[11]; //equipement of the player
  unsigned int skills[7][3];
  
  //reminder: 0 = None, 1 = Sorcerer, 2 = Druid, 3 = Paladin, 4 = Knight
  unsigned short CapGain[5];          //for level advances
  unsigned short ManaGain[5];
  unsigned short HPGain[5];
  
  //for skill advances
  unsigned int getReqSkilltries (int skill, int level, int voc);
  
  //for magic level advances
  unsigned int getReqMana(int maglevel, int voc); 
  
  //items
	containerLayout vcontainers;
  void preSave();
  void    usePlayer() {
		useCount++;
	};
	
	void    releasePlayer() {
		useCount--;
		if (useCount == 0)
			delete this;
	};

	unsigned long getIP() const;
	Container* getDepot(unsigned long depotId);
	bool addDepot(Container* depot,unsigned long depotIs);
	//depots	
	DepotMap depots;

	void RemoveDistItem();
  fight_t getFightType();
	subfight_t getSubFightType();

  bool CanSee(int x, int y, int z) const;

  void sendIcons();  
  void sendNetworkMessage(NetworkMessage *msg);
  void sendCancelAttacking();
  void sendChangeSpeed(Creature* creature);
  void sendToChannel(Creature *creature, unsigned char type, const std::string &text, unsigned short channelId);
  virtual void sendCancel(const char *msg);
  virtual void sendCancelWalk(const char *msg);  
  int sendInventory(unsigned char sl_id);
  void sendStats();
  void sendTextMessage(MessageClasses mclass, const char* message);
  void sendTextMessage(MessageClasses mclass, const char* message,const Position &pos, unsigned char type);
  void sendPing();
  void receivePing();
  void flushMsg();

  void addSkillTry(int skilltry);
  void addSkillShieldTry(int skilltry);
  void die();      //player loses exp/skills/maglevel on death

  virtual void setAttackedCreature(unsigned long id);
  virtual bool isAttackable() const { return (access == 0); };
  virtual bool isPushable() const { return (access == 0); };
	virtual void dropLoot(Container *corpse);
	bool NeedUpdateStats();	
	void onThingDisappear(const Thing* thing, unsigned char stackPos);
	void onThingAppear(const Thing* thing);  
	void onThingRemove(const Thing* thing);
	void sendDistanceShoot(const Position &from, const Position &to, unsigned char type);
	void sendMagicEffect(const Position &pos, unsigned char type);
	void sendAnimatedText(const Position &pos, unsigned char color, std::string text);
	void sendCreatureHealth(const Creature *creature);

protected:
  int useCount;

  virtual void onCreatureAppear(const Creature *creature);
  virtual void onCreatureDisappear(const Creature *creature, unsigned char stackPos, bool tele);
  virtual void onCreatureTurn(const Creature *creature, unsigned char stackpos);
  virtual void onCreatureSay(const Creature *creature, unsigned char type, const std::string &text);
  virtual void onCreatureChangeOutfit(const Creature* creature);
  virtual void onTeleport(const Creature *creature, const Position *oldPos, unsigned char oldstackpos); 
  virtual int onThink(int& newThinkTicks);
  virtual std::string getDescription(bool self = false) const;
	virtual void onTileUpdated(const Position &pos);

	//container to container
	virtual void onThingMove(const Creature *creature, const Container *fromContainer, const Container *toContainer,
		const Item* item, unsigned char from_slotid, unsigned char to_slotid, unsigned char oldcount, unsigned char count);

	//inventory to container
	virtual void onThingMove(const Creature *creature, slots_t fromSlot, const Container *toContainer,
		const Item* item, unsigned char oldcount, unsigned char count);

	//container to inventory
	virtual void onThingMove(const Creature *creature, const Container *fromContainer, slots_t toSlot,
		const Item* item, unsigned char from_slotid, unsigned char oldcount, unsigned char count);

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

	void addSkillTryInternal(int skilltry,int skill,std::string &skillname);

  friend class Game;
  friend class Map;

	Protocol *client;
	//cache some data
	struct SkillCache{
		unsigned int tries;
		int level;
		int voc;
	};
	SkillCache SkillAdvanceCache[7][2];
	struct SentStats{
		int health;
		int healthmax;
		unsigned long experience;
		int level;
		int cap;
		int mana;
		int manamax;
		int manaspent;
		int maglevel;
	};
	SentStats lastSentStats;
	// we need our name
	std::string name;	
	unsigned long guid;	
};


#endif // __player_h_

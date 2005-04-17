//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// base class to be implemented by each protocoll to use
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


#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__


#include "definitions.h"
#include "game.h"
#include "texcept.h"

#include <string>

#include "player.h"


// base class to represent different protokolls...
class Protocol
{
public:
  Protocol();
  virtual ~Protocol();

	virtual int sendInventory(){return 0;}

  void setPlayer(Player* p);
	unsigned long getIP() const;
	//SOCKET getSocket() const {return s;}

  virtual bool CanSee(int x, int y, int z) const = 0;

  virtual void sendNetworkMessage(NetworkMessage *msg) = 0;

  //container to container
	virtual void sendThingMove(const Creature *creature, const Container *fromContainer, const Container *toContainer,
		const Item* item, unsigned char from_slotid, unsigned char to_slotid, unsigned char oldcount, unsigned char count) = 0;

	//inventory to container
	virtual void sendThingMove(const Creature *creature, slots_t fromSlot, const Container *toContainer,
		const Item* item, unsigned char oldcount, unsigned char count) = 0;

	//container to inventory
	virtual void sendThingMove(const Creature *creature, const Container *fromContainer, slots_t toSlot,
		const Item* item, unsigned char from_slotid, unsigned char oldcount, unsigned char count) = 0;

	//container to ground
	virtual void sendThingMove(const Creature *creature, const Container *fromContainer, const Position *newPos,
		const Item* item, unsigned char from_slotid, unsigned char oldcount, unsigned char count) = 0;

	//inventory to ground
	virtual void sendThingMove(const Creature *creature, slots_t fromSlot, const Position *newPos,
		const Item* item, unsigned char oldcount, unsigned char count) = 0;

	//ground to container
	virtual void sendThingMove(const Creature *creature, const Position *oldPos, const Container *toContainer,
		const Item* item, unsigned char stackpos, unsigned char to_slotid, unsigned char oldcount, unsigned char count) = 0;

	//ground to inventory
	virtual void sendThingMove(const Creature *creature, const Position *oldPos, slots_t toSlot,
		const Item* item, unsigned char stackpos, unsigned char oldcount, unsigned char count) = 0;

	//ground to ground
	virtual void sendThingMove(const Creature *creature, const Thing *thing,
		const Position *oldPos, unsigned char oldStackPos, unsigned char oldcount,
		unsigned char count, bool tele = false) = 0;

  virtual void sendCreatureAppear(const Creature *creature) = 0;
  virtual void sendCreatureDisappear(const Creature *creature, unsigned char stackPos) = 0;
  virtual void sendCreatureTurn(const Creature *creature, unsigned char stackPos) = 0;
  virtual void sendCreatureSay(const Creature *creature, unsigned char type, const std::string &text) = 0;
  virtual void sendSetOutfit(const Creature* creature) = 0;
	virtual void sendTileUpdated(const Position &pos) = 0;
	//virtual void sendContainerUpdated(Item *item, unsigned char from_id, unsigned char to_id, unsigned char from_slot, unsigned char to_slot, bool remove) = 0;
  virtual void sendIcons(int icons) = 0;
  virtual void sendCancel(const char *msg) = 0;
  virtual void sendCancelWalk(const char *msg) = 0;
  virtual void sendChangeSpeed(const Creature* creature) = 0;
  virtual void sendCancelAttacking() = 0;
  virtual void sleepTillMove();
  virtual void sendChannels() = 0;
  virtual void sendChannel(unsigned short channelId) = 0;
  virtual void sendToChannel(const Creature * creature, unsigned char type, const std::string &text, unsigned short channelId) = 0;
  virtual void sendOpenPriv(std::string &receiver) =0;


protected:
  Game   *game;
  Player *player;
  SOCKET s;
};


#endif  // #ifndef __PROTOCOL_H__

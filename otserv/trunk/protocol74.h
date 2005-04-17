//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// The protocoll for the clients version 7.4
// the newer clients still use the same protocoll just with a 
// prequel defined in the 6.5 protocoll
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


#ifndef tprot74_h
#define tprot74_h

#include "protocol.h"
#include "creature.h"
#include "texcept.h"
#include "item.h"
#include "container.h"
#include <string>

/*
enum MoveParam {
 MOVE_FROM_INVENTORY = 1,
 MOVE_TO_INVENTORY = 2, 
 MOVE_FROM_CONTAINER =  4, 
 MOVE_TO_CONTAINER = 8, 
 MOVE_FROM_GROUND = 16, 
 MOVE_TO_GROUND = 32
 };
 */

class NetworkMessage;

class Protocol74 : public Protocol
{
public:
  // our constructor get's the socket of the client and the initial
  // message the client sent
  Protocol74(SOCKET s);
  virtual ~Protocol74();

  bool ConnectPlayer();
  void ReceiveLoop();


private:
  // the socket the player is on...

  std::list<unsigned long> knownPlayers;
  void checkCreatureAsKnown(unsigned long id, bool &known, unsigned long &removedKnown);

	// we have all the parse methods
  void parsePacket(NetworkMessage &msg);

  void parseLogout(NetworkMessage &msg);
  
  void parseCancelMove(NetworkMessage &msg);
  void parseModes(NetworkMessage &msg);
  void parseDebug(NetworkMessage &msg);
  
  void parseMoveByMouse(NetworkMessage &msg);

	void parseMoveNorth(NetworkMessage &msg);
	void parseMoveEast(NetworkMessage &msg);
	void parseMoveSouth(NetworkMessage &msg);
	void parseMoveWest(NetworkMessage &msg);
  
	void parseTurnNorth(NetworkMessage &msg);
	void parseTurnEast(NetworkMessage &msg);
	void parseTurnSouth(NetworkMessage &msg);
	void parseTurnWest(NetworkMessage &msg);

	void parseRequestOutfit(NetworkMessage &msg);
	void parseSetOutfit(NetworkMessage &msg);

	void parseSay(NetworkMessage &msg);

	void parseLookAt(NetworkMessage &msg);

	void parseAttack(NetworkMessage &msg);

	void parseThrow(NetworkMessage &msg);
	void parseUseItemEx(NetworkMessage &msg);
	void parseUseItem(NetworkMessage &msg);
	void parseCloseContainer(NetworkMessage &msg);
	void parseUpArrowContainer(NetworkMessage &msg);

/*
	void sendPlayerItemAppear(Action* action);
	void sendPlayerItemChange(Action* action);
	void sendPlayerItemDisappear(Action* action);
	void sendPlayerAppearance(Action* action);
	void sendPlayerChangeAppearance(Action* action);
*/

	void sendPlayerLookAt(std::string);

	// channel tabs
	void parseGetChannels(NetworkMessage &msg);
	void parseOpenChannel(NetworkMessage &msg);
	void parseOpenPriv(NetworkMessage &msg);
	void parseCloseChannel(NetworkMessage &msg);
	virtual void sendChannels();
	virtual void sendChannel(unsigned short channelId);
	virtual void sendOpenPriv(std::string &receiver);
	virtual void sendToChannel(const Creature *creature, unsigned char type, const std::string &text, unsigned short channelId);
	//void sendPlayerChangeGround(Action* action);

  virtual void sendNetworkMessage(NetworkMessage *msg);
  virtual void sendIcons(int icons);

	//container to container
	virtual void sendThingMove(const Creature *creature, const Container *fromContainer, const Container *toContainer,
		const Item* item, unsigned char from_slotid, unsigned char to_slotid, unsigned char oldcount, unsigned char count);

	//inventory to container
	virtual void sendThingMove(const Creature *creature, slots_t fromSlot, const Container *toContainer,
		const Item* item, unsigned char oldcount, unsigned char count);

	//container to inventory
	virtual void sendThingMove(const Creature *creature, const Container *fromContainer, slots_t toSlot,
		const Item* item, unsigned char from_slotid, unsigned char oldcount, unsigned char count);

	//container to ground
	virtual void sendThingMove(const Creature *creature, const Container *fromContainer, const Position *newPos,
		const Item* item, unsigned char from_slotid, unsigned char oldcount, unsigned char count);

	//inventory to ground
	virtual void sendThingMove(const Creature *creature, slots_t fromSlot, const Position *newPos,
		const Item* item, unsigned char oldcount, unsigned char count);

	//ground to container
	virtual void sendThingMove(const Creature *creature, const Position *oldPos, const Container *toContainer,
		const Item* item, unsigned char stackpos, unsigned char to_slotid, unsigned char oldcount, unsigned char count);

	//ground to inventory
	virtual void sendThingMove(const Creature *creature, const Position *oldPos, slots_t toSlot,
		const Item* item, unsigned char stackpos, unsigned char oldcount, unsigned char count);

	//ground to ground
  virtual void sendThingMove(const Creature *creature, const Thing *thing,
		const Position *oldPos, unsigned char oldstackpos, unsigned char oldcount,
		unsigned char count, bool tele = false);

	void autoCloseContainers(const Container *container, NetworkMessage &msg);

  virtual void sendCreatureAppear(const Creature *creature);
  virtual void sendCreatureDisappear(const Creature *creature, unsigned char stackPos);
  virtual void sendCreatureTurn(const Creature *creature, unsigned char stackpos);
  virtual void sendCreatureSay(const Creature *creature, unsigned char type, const std::string &text);

  virtual void sendCancel(const char *msg);
  virtual void sendCancelWalk(const char *msg);
  virtual void sendChangeSpeed(const Creature* creature);
  virtual void sendCancelAttacking();
  void sendSetOutfit(const Creature* creature);
	virtual void sendTileUpdated(const Position &Pos);
	//virtual void sendContainerUpdated(Item *item, unsigned char from_id, unsigned char to_id, unsigned char from_slot, unsigned char to_slot, bool remove);

  virtual bool CanSee(int x, int y, int z) const;

	void GetTileUpdated(const Position &pos, NetworkMessage &msg);
	void sendContainer(unsigned char index, Container *container);
	void sendCloseContainer(unsigned char containerid);

	// translate a tile to clientreadable format
  void GetTileDescription(const Tile* tile, NetworkMessage &msg);

	// translate a map area to clientreadable format
  void GetMapDescription(unsigned short x, unsigned short y, unsigned char z,
                         unsigned short width, unsigned short height,
                         NetworkMessage &msg);


};



#endif

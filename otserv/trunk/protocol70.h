//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// The protocoll for the clients up to version 6.4
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


#ifndef tprot70_h
#define tprot70_h

#include "protocol.h"
#include "creature.h"
#include "texcept.h"
#include "item.h"
#include "container.h"
#include <string>


class NetworkMessage;

class Protocol70 : public Protocol
{
public:
  // our constructor get's the socket of the client and the initial
  // message the client sent
  Protocol70(SOCKET s);
  virtual ~Protocol70();

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

/*	void sendPlayerItemAppear(Action* action);
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
virtual void sendToChannel(const Creature * creature, unsigned char type, const std::string &text, unsigned short channelId);
//	void sendPlayerChangeGround(Action* action);

  virtual void sendNetworkMessage(NetworkMessage *msg);
  virtual void sendIcons(int icons);
  virtual void sendThingMove(const Creature *creature, const Thing *thing, const Position *oldPos, unsigned char oldstackpos);
  virtual void sendCreatureAppear(const Creature *creature);
  virtual void sendCreatureDisappear(const Creature *creature, unsigned char stackPos);
  virtual void sendCreatureTurn(const Creature *creature, unsigned char stackpos);
  virtual void sendCreatureSay(const Creature *creature, unsigned char type, const std::string &text);

  virtual void sendCancel(const char *msg);
  virtual void sendCancelWalk(const char *msg);
  virtual void sendChangeSpeed(const Creature* creature);
  virtual void sendCancelAttacking();
  void sendSetOutfit(const Creature* creature);
	virtual void sendTileUpdated(const Position *Pos);
	virtual void sendContainerUpdated(Item *item, unsigned char from_id, unsigned char to_id, unsigned char from_slot, unsigned char to_slot, bool remove);

  virtual bool CanSee(int x, int y) const;

  // translate a map area to clientreadable format
  void GetMapDescription(unsigned short x, unsigned short y, unsigned char z,
                         unsigned short width, unsigned short height,
                         NetworkMessage &msg);


};



#endif

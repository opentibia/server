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
  SOCKET s;

  std::list<unsigned long> knownPlayers;
  bool setCreatureAsKnown(unsigned long id);

	// we have all the parse methods
  void parsePacket(NetworkMessage &msg);

  void parseLogout(NetworkMessage &msg);

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
//	void parseUseItem(Action* action, NetworkMessage &msg);

/*	void sendPlayerItemAppear(Action* action);
	void sendPlayerItemChange(Action* action);
	void sendPlayerItemDisappear(Action* action);
	void sendPlayerAppearance(Action* action);
	void sendPlayerChangeAppearance(Action* action);
*/

	void sendPlayerSorry();
	void sendPlayerSorry(tmapEnum);
	void sendPlayerSorry(std::string);
	void sendPlayerLookAt(std::string);



//	void sendPlayerChangeGround(Action* action);

  virtual void sendNetworkMessage(NetworkMessage *msg);

  
  virtual void sendThingMove(const Creature *creature, const Thing *thing, const Position *oldPos, unsigned char oldstackpos);
  virtual void sendCreatureAppear(const Creature *creature);
  virtual void sendCreatureDisappear(const Creature *creature, unsigned char stackPos);
  virtual void sendCreatureTurn(const Creature *creature, unsigned char stackpos);
  virtual void sendCreatureSay(const Creature *creature, unsigned char type, const std::string &text);

  void sendSetOutfit(const Creature* creature);
	virtual void sendTileUpdated(const Position *Pos);

  virtual bool CanSee(int x, int y);

  // translate a map area to clientreadable format
  void GetMapDescription(unsigned short x, unsigned short y, unsigned short z,
                         unsigned short width, unsigned short height,
                         NetworkMessage &msg);


};



#endif

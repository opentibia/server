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

#include <string>

#include "player.h"

class NetworkMessage;
// base class to represent different protokolls...
class Protocol
{
public:
	Protocol();
	virtual ~Protocol();
	
	void setPlayer(Player* p);
	unsigned long getIP() const;
	
	virtual bool CanSee(int x, int y, int z) const = 0;
	virtual bool CanSee(const Creature*) const = 0;
	
	virtual void sleepTillMove();
	virtual void flushOutputBuffer() = 0;
	virtual void logout() = 0;
	virtual void reinitializeProtocol() = 0;

	//ground to ground
	virtual void sendThingMove(const Creature *creature, const Thing *thing,
		const Position *oldPos, unsigned char oldStackPos, unsigned char oldcount,
		unsigned char count, bool tele = false) = 0;
	
	//tiles
	virtual void sendThingAppear(const Thing *thing) = 0;
	virtual void sendThingRemove(const Thing *thing) = 0;
	virtual void sendThingTransform(const Thing* thing,int stackpos) = 0;
	virtual void sendThingDisappear(const Thing *thing, unsigned char stackPos, bool tele) = 0;
	virtual void sendTileUpdated(const Position &pos) = 0;

	//containers
	virtual void sendAddContainerItem(const Container* container, const Item* item) = 0;
	virtual void sendUpdateContainerItem(const Container *container, uint8_t slot, const Item* item) = 0;
	virtual void sendRemoveContainerItem(const Container* container, uint8_t slot) = 0;

	virtual void sendContainer(uint32_t cid, Container* container) = 0;
	virtual void sendCloseContainer(uint32_t cid) = 0;

	//inventory
	virtual void sendAddInventoryItem(slots_t slot, const Item* item) = 0;
	virtual void sendUpdateInventoryItem(slots_t slot, const Item* item) = 0;
	virtual void sendRemoveInventoryItem(slots_t slot) = 0;

	virtual void sendDistanceShoot(const Position &from, const Position &to, unsigned char type) = 0;
	virtual void sendMagicEffect(const Position &pos, unsigned char type) = 0;
	virtual void sendAnimatedText(const Position &pos, unsigned char color, std::string text) = 0;
	virtual void sendCreatureHealth(const Creature *creature) = 0;
	virtual void sendSkills() = 0;
	virtual void sendPing() = 0;
	virtual void sendTradeItemRequest(const Player* player, const Item* item, bool ack) = 0;
	virtual void sendCloseTrade() = 0;
	virtual void sendTextWindow(Item* item,const unsigned short maxlen, const bool canWrite) = 0;
	virtual void sendCreatureTurn(const Creature *creature, unsigned char stackPos) = 0;
	virtual void sendCreatureSay(const Creature *creature, SpeakClasses type, const std::string &text) = 0;
	virtual void sendSetOutfit(const Creature* creature) = 0;
	virtual void sendIcons(int icons) = 0;
	virtual void sendCancel(const char *msg) = 0;
	virtual void sendCancelWalk() = 0;
	virtual void sendStats() = 0;
	virtual void sendChangeSpeed(const Creature* creature) = 0;
	virtual void sendCancelAttacking() = 0;
	virtual void sendTextMessage(MessageClasses mclass, const char* message) = 0;
	virtual void sendTextMessage(MessageClasses mclass, const char* message,const Position &pos, unsigned char type) = 0;
	virtual void sendChannelsDialog() = 0;
	virtual void sendChannel(unsigned short channelId, std::string channelName) = 0;
	virtual void sendToChannel(const Creature * creature, SpeakClasses type, const std::string &text, unsigned short channelId) = 0;
	virtual void sendOpenPriv(std::string &receiver) =0;
	virtual void sendVIPLogIn(unsigned long guid) = 0;
	virtual void sendVIPLogOut(unsigned long guid) = 0;
	virtual void sendVIP(unsigned long guid, const std::string &name, bool isOnline) = 0;	
	
protected:
	
	bool pendingLogout;
	Game   *game;
	Player *player;
	SOCKET s;
	friend OTSYS_THREAD_RETURN ConnectionHandler(void *dat);
};


#endif  // #ifndef __PROTOCOL_H__

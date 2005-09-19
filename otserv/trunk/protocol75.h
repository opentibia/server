//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// The protocoll for the clients version 7.5
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


#ifndef tprot75_h
#define tprot75_h

#include "protocol.h"
#include "creature.h"
#include "item.h"
#include "container.h"
#include <string>

class NetworkMessage;

class Protocol75 : public Protocol
{
public:
	// our constructor get's the socket of the client and the initial
	// message the client sent
	Protocol75(SOCKET s);
	virtual ~Protocol75();
	
	bool ConnectPlayer();
	void ReceiveLoop();  
	void WriteBuffer(NetworkMessage &add);
	virtual void reinitializeProtocol();
	
private:
	// the socket the player is on...
	NetworkMessage OutputBuffer;
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
	void parseMoveNorthEast(NetworkMessage &msg);
	void parseMoveSouthEast(NetworkMessage &msg);
	void parseMoveSouthWest(NetworkMessage &msg);
	void parseMoveNorthWest(NetworkMessage &msg);
	
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
	void parseTextWindow(NetworkMessage &msg);
	
	void parseRequestTrade(NetworkMessage &msg);
	void parseLookInTrade(NetworkMessage &msg);
	void parseAcceptTrade(NetworkMessage &msg);
	void parseCloseTrade();
	
	
	// channel tabs
	void parseGetChannels(NetworkMessage &msg);
	void parseOpenChannel(NetworkMessage &msg);
	void parseOpenPriv(NetworkMessage &msg);
	void parseCloseChannel(NetworkMessage &msg);
	virtual void sendChannelsDialog();
	virtual void sendChannel(unsigned short channelId, std::string channelName);
	virtual void sendOpenPriv(std::string &receiver);
	virtual void sendToChannel(const Creature *creature, SpeakClasses type, const std::string &text, unsigned short channelId);
	
	virtual void sendNetworkMessage(NetworkMessage *msg);
	virtual void sendIcons(int icons);
	
	//container to container
	virtual void sendThingMove(const Creature *creature, const Container *fromContainer, unsigned char from_slotid,
		const Item* fromItem, int oldFromCount, Container *toContainer, unsigned char to_slotid, const Item *toItem, int oldToCount, int count);
	
	//inventory to container
	virtual void sendThingMove(const Creature *creature, slots_t fromSlot, const Item* fromItem,
		int oldFromCount, const Container *toContainer, unsigned char to_slotid, const Item *toItem, int oldToCount, int count);
	
	//inventory to inventory
	virtual void sendThingMove(const Creature *creature, slots_t fromSlot, const Item* fromItem,
		int oldFromCount, slots_t toSlot, const Item* toItem, int oldToCount, int count);
	
	//container to inventory
	virtual void sendThingMove(const Creature *creature, const Container *fromContainer, unsigned char from_slotid,
		const Item* fromItem, int oldFromCount, slots_t toSlot, const Item *toItem, int oldToCount, int count);
	
	//container to ground
	virtual void sendThingMove(const Creature *creature, const Container *fromContainer, unsigned char from_slotid,
		const Item* fromItem, int oldFromCount, const Position &toPos, const Item *toItem, int oldToCount, int count);
	
	//inventory to ground
	virtual void sendThingMove(const Creature *creature, slots_t fromSlot,
		const Item* fromItem, int oldFromCount, const Position &toPos, const Item *toItem, int oldToCount, int count);
	
	//ground to container
	virtual void sendThingMove(const Creature *creature, const Position &fromPos, int stackpos, const Item* fromItem,
		int oldFromCount, const Container *toContainer, unsigned char to_slotid, const Item *toItem, int oldToCount, int count);
	
	//ground to inventory
	virtual void sendThingMove(const Creature *creature, const Position &fromPos, int stackpos, const Item* fromItem,
		int oldFromCount, slots_t toSlot, const Item *toItem, int oldToCount, int count);
	
	//ground to ground
	virtual void sendThingMove(const Creature *creature, const Thing *thing,
		const Position *oldPos, unsigned char oldstackpos, unsigned char oldcount,
		unsigned char count, bool tele = false);
	
	void autoCloseContainers(const Container *container, NetworkMessage &msg);
	
	virtual void sendThingDisappear(const Thing *thing, unsigned char stackPos, bool tele = false);
	virtual void sendThingAppear(const Thing *thing);
	virtual void sendThingTransform(const Thing* thing,int stackpos);
	virtual void sendThingRemove(const Thing *thing);
	virtual void sendDistanceShoot(const Position &from, const Position &to, unsigned char type);
	virtual void sendMagicEffect(const Position &pos, unsigned char type);
	virtual void sendAnimatedText(const Position &pos, unsigned char color, std::string text);
	virtual void sendCreatureHealth(const Creature *creature);
	virtual void sendSkills();
	virtual void sendPing();
	virtual void sendCreatureTurn(const Creature *creature, unsigned char stackpos);
	virtual void sendCreatureSay(const Creature *creature, SpeakClasses type, const std::string &text);
	
	virtual void sendCancel(const char *msg);
	virtual void sendCancelWalk();
	virtual void sendChangeSpeed(const Creature* creature);
	virtual void sendCancelAttacking();
	void sendSetOutfit(const Creature* creature);
	virtual void sendTileUpdated(const Position &Pos);
	virtual void sendInventory(unsigned char sl_id);
	virtual void sendStats();
	virtual void sendTextMessage(MessageClasses mclass, const char* message);
	virtual void sendTextMessage(MessageClasses mclass, const char* message,const Position &pos, unsigned char type);
	
	virtual bool CanSee(int x, int y, int z) const;
	virtual bool CanSee(const Creature*) const;
	virtual void logout();
	
	void flushOutputBuffer();
	void WriteMsg(NetworkMessage &msg);
	
	virtual void sendContainer(unsigned char index, Container *container);
	virtual void sendTradeItemRequest(const Player* player, const Item* item, bool ack);
	virtual void sendCloseTrade();
	
	virtual void sendCloseContainer(unsigned char containerid);
	void sendItemAddContainer(const Container *container, const Item *item);
	void sendItemRemoveContainer(const Container* container,const unsigned char slot);
	void sendItemUpdateContainer(const Container* container,const Item* item,const unsigned char slot);
	void sendTextWindow(Item* item,const unsigned short maxlen, const bool canWrite);
	
	// translate a tile to clientreadable format
	void GetTileDescription(const Tile* tile, NetworkMessage &msg);
	
	// translate a map area to clientreadable format
	void GetMapDescription(unsigned short x, unsigned short y, unsigned char z,
		unsigned short width, unsigned short height,
		NetworkMessage &msg);
	virtual void AddTextMessage(NetworkMessage &msg,MessageClasses mclass, const char* message);
	virtual void AddAnimatedText(NetworkMessage &msg,const Position &pos, unsigned char color, std::string text);
	virtual void AddMagicEffect(NetworkMessage &msg,const Position &pos, unsigned char type);
	virtual void AddDistanceShoot(NetworkMessage &msg,const Position &from, const Position &to, unsigned char type);
	virtual void AddCreature(NetworkMessage &msg,const Creature *creature, bool known, unsigned int remove);
	virtual void AddPlayerStats(NetworkMessage &msg,const Player *player);
	virtual void AddPlayerInventoryItem(NetworkMessage &msg,const Player *player, int item);
	virtual void AddCreatureSpeak(NetworkMessage &msg,const Creature *creature, SpeakClasses type, std::string text, unsigned short channelId);
	virtual void AddCreatureHealth(NetworkMessage &msg,const Creature *creature);
	virtual void AddPlayerSkills(NetworkMessage &msg,const Player *player);
	virtual void AddRemoveThing(NetworkMessage &msg, const Position &pos,unsigned char stackpos);
	virtual void AddAppearThing(NetworkMessage &msg, const Position &pos);
	virtual void AddTileUpdated(NetworkMessage &msg, const Position &pos);
	virtual void AddItemContainer(NetworkMessage &msg,unsigned char cid, const Item *item);
	virtual void AddItemContainer(NetworkMessage &msg,unsigned char cid, const Item *item,unsigned char count);
	virtual void TransformItemContainer(NetworkMessage &msg,unsigned char cid,unsigned char slot, const Item *item);
	virtual void RemoveItemContainer(NetworkMessage &msg,unsigned char cid,unsigned char slot);
	
	
	OTSYS_THREAD_LOCKVAR bufferLock;
	unsigned long windowTextID;
	Item *readItem;
	
	friend OTSYS_THREAD_RETURN ConnectionHandler(void *dat);
};

#endif

//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// The protocoll for the clients version 7.6
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


#ifndef __OTSERV_PROTOCOL76_H__
#define __OTSERV_PROTOCOL76_H__

#include "definitions.h"
#include <string>
#include "networkmessage.h"
#include "enums.h"
#include "creature.h"

enum connectResult_t{
	CONNECT_SUCCESS = 1,
	CONNECT_TOMANYPLAYERS = 2,
	CONNECT_MASTERPOSERROR = 3,
	CONNECT_INTERNALERROR = 4
};

class NetworkMessage;
class Player;
class Game;
class House;
class Container;
class Tile;

class Protocol76
{
public:
	Protocol76(SOCKET s);
	~Protocol76();
	
	connectResult_t ConnectPlayer();
	void ReceiveLoop();  
	void WriteBuffer(NetworkMessage &add);
	void reinitializeProtocol();
	
	void setPlayer(Player* p);
	unsigned long getIP() const;
	void sleepTillMove();
	
private:
	NetworkMessage OutputBuffer;
	std::list<unsigned long> knownPlayers;
	void checkCreatureAsKnown(unsigned long id, bool &known, unsigned long &removedKnown);
	
	bool canSee(int x, int y, int z) const;
	bool canSee(const Creature*) const;
	bool canSee(const Position& pos) const;
	void logout();
	
	void flushOutputBuffer();
	void WriteMsg(NetworkMessage& msg);

	// we have all the parse methods
	void parsePacket(NetworkMessage& msg);
	
	//Parse methods
	void parseLogout(NetworkMessage& msg);	
	void parseCancelMove(NetworkMessage& msg);

	void parseRecievePing(NetworkMessage& msg);
	void parseAutoWalk(NetworkMessage& msg);
	void parseStopAutoWalk(NetworkMessage& msg);	
	void parseMoveNorth(NetworkMessage& msg);
	void parseMoveEast(NetworkMessage& msg);
	void parseMoveSouth(NetworkMessage& msg);
	void parseMoveWest(NetworkMessage& msg);
	void parseMoveNorthEast(NetworkMessage& msg);
	void parseMoveSouthEast(NetworkMessage& msg);
	void parseMoveSouthWest(NetworkMessage& msg);
	void parseMoveNorthWest(NetworkMessage& msg);
	
	void parseTurnNorth(NetworkMessage& msg);
	void parseTurnEast(NetworkMessage& msg);
	void parseTurnSouth(NetworkMessage& msg);
	void parseTurnWest(NetworkMessage& msg);
	
	void parseRequestOutfit(NetworkMessage& msg);
	void parseSetOutfit(NetworkMessage& msg);
	void parseSay(NetworkMessage& msg);
	void parseLookAt(NetworkMessage& msg);
	void parseFightModes(NetworkMessage& msg);
	void parseAttack(NetworkMessage& msg);
	void parseFollow(NetworkMessage& msg);
	
	void parseThrow(NetworkMessage& msg);
	void parseUseItemEx(NetworkMessage& msg);
	void parseBattleWindow(NetworkMessage& msg);
	void parseUseItem(NetworkMessage& msg);
	void parseCloseContainer(NetworkMessage& msg);
	void parseUpArrowContainer(NetworkMessage& msg);
	void parseUpdateContainer(NetworkMessage& msg);
	void parseTextWindow(NetworkMessage& msg);
	void parseHouseWindow(NetworkMessage& msg);
	
	//trade methods
	void parseRequestTrade(NetworkMessage& msg);
	void parseLookInTrade(NetworkMessage& msg);
	void parseAcceptTrade(NetworkMessage& msg);
	void parseCloseTrade();
	
	//VIP methods
	void parseAddVip(NetworkMessage& msg);
	void parseRemVip(NetworkMessage& msg);

	void parseRotateItem(NetworkMessage& msg);
		
	//Channel tabs
	void parseGetChannels(NetworkMessage& msg);
	void parseOpenChannel(NetworkMessage& msg);
	void parseOpenPriv(NetworkMessage& msg);
	void parseCloseChannel(NetworkMessage& msg);

	void parseDebug(NetworkMessage& msg);

	//Send functions
	void sendChannelsDialog();
	void sendChannel(unsigned short channelId, std::string channelName);
	void sendOpenPriv(const std::string& receiver);
	void sendToChannel(const Creature* creature, SpeakClasses type, const std::string& text, unsigned short channelId);
	
	void sendIcons(int icons);

	void sendDistanceShoot(const Position& from, const Position& to, unsigned char type);
	void sendMagicEffect(const Position& pos, unsigned char type);
	void sendAnimatedText(const Position& pos, unsigned char color, std::string text);
	void sendCreatureHealth(const Creature* creature);
	void sendSkills();
	void sendPing();
	void sendCreatureTurn(const Creature* creature, unsigned char stackpos);
	void sendCreatureSay(const Creature* creature, SpeakClasses type, const std::string& text);
	
	void sendCancel(const std::string& message);
	void sendCancelWalk();
	void sendChangeSpeed(const Creature* creature, uint32_t speed);
	void sendCancelTarget();
	void sendCreatureVisible(const Creature* creature, bool visible);
	void sendCreatureOutfit(const Creature* creature, const Outfit_t& outfit);
	void sendCreatureInvisible(const Creature* creature);
	void sendStats();
	void sendTextMessage(MessageClasses mclass, const std::string& message);
	void sendTextMessage(MessageClasses mclass, const std::string& message, const Position& pos,
		unsigned char type);
	
	void sendTradeItemRequest(const Player* player, const Item* item, bool ack);
	void sendCloseTrade();
	
	void sendTextWindow(Item* item,const unsigned short maxlen, const bool canWrite);
	void sendHouseWindow(House* house, unsigned long listid, const std::string& text);
	
	void sendVIPLogIn(unsigned long guid);
	void sendVIPLogOut(unsigned long guid);
	void sendVIP(unsigned long guid, const std::string& name, bool isOnline);
	
	void sendCreatureLight(const Creature* creature);
	void sendWorldLight(const LightInfo& lightInfo);
	
	void sendCreatureSkull(const Creature* creature, Skulls_t skull);
	void sendCreatureShield(const Creature* creature);
	void sendCreatureSquare(const Creature* creature, SquareColor_t color);
	
	//tiles
	void sendAddTileItem(const Position& pos, const Item* item);
	void sendUpdateTileItem(const Position& pos, uint32_t stackpos, const Item* item);
	void sendRemoveTileItem(const Position& pos, uint32_t stackpos);
	void UpdateTile(const Position& pos);

	void sendAddCreature(const Creature* creature, bool isLogin);
	void sendRemoveCreature(const Creature* creature, const Position& pos, uint32_t stackpos, bool isLogout);
	void sendMoveCreature(const Creature* creature, const Position& newPos, const Position& oldPos,
		uint32_t oldStackPos, bool teleport);

	//containers
	void sendAddContainerItem(uint8_t cid, const Item* item);
	void sendUpdateContainerItem(uint8_t cid, uint8_t slot, const Item* item);
	void sendRemoveContainerItem(uint8_t cid, uint8_t slot);

	void sendContainer(uint32_t cid, const Container* container, bool hasParent);
	void sendCloseContainer(uint32_t cid);

	//inventory
	void sendAddInventoryItem(slots_t slot, const Item* item);
	void sendUpdateInventoryItem(slots_t slot, const Item* item);
	void sendRemoveInventoryItem(slots_t slot);

	//Help functions

	// translate a tile to clientreadable format
	void GetTileDescription(const Tile* tile, NetworkMessage &msg);
	
	// translate a floor to clientreadable format
	void GetFloorDescription(NetworkMessage& msg, int x, int y, int z, int width, int height, int offset, int& skip);

	// translate a map area to clientreadable format
	void GetMapDescription(unsigned short x, unsigned short y, unsigned char z,
		unsigned short width, unsigned short height,
		NetworkMessage &msg);

	void AddMapDescription(NetworkMessage& msg, const Position& pos);
	void AddTextMessage(NetworkMessage &msg,MessageClasses mclass, const std::string& message);
	void AddAnimatedText(NetworkMessage &msg,const Position& pos, unsigned char color, const std::string& text);
	void AddMagicEffect(NetworkMessage &msg,const Position& pos, unsigned char type);
	void AddDistanceShoot(NetworkMessage &msg,const Position& from, const Position& to, unsigned char type);
	void AddCreature(NetworkMessage &msg,const Creature* creature, bool known, unsigned int remove);
	void AddPlayerStats(NetworkMessage &msg);
	void AddCreatureSpeak(NetworkMessage &msg,const Creature* creature, SpeakClasses type, std::string text, unsigned short channelId);
	void AddCreatureHealth(NetworkMessage &msg,const Creature* creature);
	void AddCreatureOutfit(NetworkMessage &msg, const Creature* creature, const Outfit_t& outfit);
	void AddCreatureInvisible(NetworkMessage &msg, const Creature* creature);	
	void AddPlayerSkills(NetworkMessage &msg);
	void AddWorldLight(NetworkMessage &msg, const LightInfo& lightInfo);
	void AddCreatureLight(NetworkMessage &msg, const Creature* creature);

	//tiles
	void AddTileItem(NetworkMessage& msg, const Position& pos, const Item* item);
	void AddTileCreature(NetworkMessage& msg, const Position& pos, const Creature* creature);
	void UpdateTileItem(NetworkMessage& msg, const Position& pos, uint32_t stackpos, const Item* item);
	void RemoveTileItem(NetworkMessage& msg, const Position& pos, uint32_t stackpos);
	void UpdateTile(NetworkMessage& msg, const Position& pos);

	void MoveUpCreature(NetworkMessage& msg, const Creature* creature,
		const Position& newPos, const Position& oldPos, uint32_t oldStackPos);
	void MoveDownCreature(NetworkMessage& msg, const Creature* creature,
		const Position& newPos, const Position& oldPos, uint32_t oldStackPos);

	//container
	void AddContainerItem(NetworkMessage& msg, uint8_t cid, const Item* item);
	void UpdateContainerItem(NetworkMessage& msg, uint8_t cid, uint8_t slot, const Item* item);
	void RemoveContainerItem(NetworkMessage& msg, uint8_t cid, uint8_t slot);
	
	//inventory
	void AddInventoryItem(NetworkMessage& msg, slots_t slot, const Item* item);
	void UpdateInventoryItem(NetworkMessage& msg, slots_t slot, const Item* item);
	void RemoveInventoryItem(NetworkMessage& msg, slots_t slot);

	OTSYS_THREAD_LOCKVAR bufferLock;
	unsigned long windowTextID;
	Item* readItem;
	unsigned long maxTextLength;
	
	House* house;
	unsigned long listId;
		
	bool pendingLogout;
	Game   *game;
	Player* player;
	SOCKET s;
	
	friend class Player;
	friend OTSYS_THREAD_RETURN ConnectionHandler(void *dat);
};

#endif

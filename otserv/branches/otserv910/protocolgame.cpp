//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Implementation of tibia v8.0 protocol
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
#include "otpch.h"
/* server to client unknown bytes
0xB6 - 2 bytes usage(???)
*/
#include "protocolgame.h"
#include "networkmessage.h"
#include "outputmessage.h"
#include "items.h"
#include "tile.h"
#include "player.h"
#include "chat.h"
#include "configmanager.h"
#include "actions.h"
#include "game.h"
#include "ioplayer.h"
#include "house.h"
#include "waitlist.h"
#include "ban.h"
#include "ioaccount.h"
#include "connection.h"
#include "creatureevent.h"
#include "quests.h"
#include <boost/function.hpp>
#include <string>
#include <iostream>
#include <sstream>
#include <ctime>
#include <list>
#include <fstream>

extern Game g_game;
extern ConfigManager g_config;
extern Actions actions;
extern BanManager g_bans;
extern CreatureEvents* g_creatureEvents;
Chat g_chat;

#ifdef __ENABLE_SERVER_DIAGNOSTIC__
uint32_t ProtocolGame::protocolGameCount = 0;
#endif

// Helping templates to add dispatcher tasks

template<class FunctionType>
void ProtocolGame::addGameTaskInternal(bool droppable, uint32_t delay, const FunctionType& func)
{
	if(droppable)
		g_dispatcher.addTask(createTask(delay, func));
	else
		g_dispatcher.addTask(createTask(func));
}

ProtocolGame::ProtocolGame(Connection_ptr connection) :
	Protocol(connection)
{
	player = NULL;
	m_debugAssertSent = false;
	m_acceptPackets = false;
	eventConnect = 0;
	enableChecksum();

#ifdef __ENABLE_SERVER_DIAGNOSTIC__
	protocolGameCount++;
#endif
}

ProtocolGame::~ProtocolGame()
{
	player = NULL;

#ifdef __ENABLE_SERVER_DIAGNOSTIC__
	protocolGameCount--;
#endif
}

void ProtocolGame::setPlayer(Player* p)
{
	player = p;
}

void ProtocolGame::releaseProtocol()
{
	//dispatcher thread
	if(player){
		if(player->client == this){
			player->client = NULL;
		}
	}

	Protocol::releaseProtocol();
}

void ProtocolGame::deleteProtocolTask()
{
	//dispatcher thread
	if(player){
		#ifdef __DEBUG_NET_DETAIL__
		std::cout << "Deleting ProtocolGame - Protocol:" << this << ", Player: " << player << std::endl;
		#endif

		g_game.FreeThing(player);
		player = NULL;
	}

	Protocol::deleteProtocolTask();
}

bool ProtocolGame::login(const std::string& name, bool isSetGM)
{
	//dispatcher thread
	Player* _player = g_game.getPlayerByName(name);
	if(!_player || g_config.getNumber(ConfigManager::ALLOW_CLONES)){
		player = new Player(name, this);
		player->useThing2();
		player->setID();

		if(!IOPlayer::instance()->loadPlayer(player, name, true)){
#ifdef __DEBUG__
			std::cout << "ProtocolGame::login - preloading loadPlayer failed - " << name << std::endl;
#endif
			disconnectClient(0x14, "Your character could not be loaded.");
			return false;
		}

		if(g_bans.isPlayerBanished(name) && !player->hasFlag(PlayerFlag_CannotBeBanned)){
			disconnectClient(0x14, "Your character is locked!");
			return false;
		}

		if(g_bans.isAccountBanished(player->getAccountId()) && !player->hasFlag(PlayerFlag_CannotBeBanned)){
			disconnectClient(0x14, "Your account is banished!");
			return false;
		}

		if(isSetGM && !player->hasFlag(PlayerFlag_CanAlwaysLogin) &&
			!g_config.getNumber(ConfigManager::ALLOW_GAMEMASTER_MULTICLIENT))
		{
			disconnectClient(0x14, "You may only login with a Gamemaster account.");
			return false;
		}

		if(g_game.getGameState() == GAME_STATE_CLOSING && !player->hasFlag(PlayerFlag_CanAlwaysLogin)){
			disconnectClient(0x14, "The game is just going down.\nPlease try again later.");
			return false;
		}

		if(g_game.getGameState() == GAME_STATE_CLOSED && !player->hasFlag(PlayerFlag_CanAlwaysLogin)){
			disconnectClient(0x14, "Server is closed.");
			return false;
		}

		if(g_config.getNumber(ConfigManager::CHECK_ACCOUNTS) && !player->hasFlag(PlayerFlag_CanAlwaysLogin)
			&& g_game.getPlayerByAccount(player->getAccountId())){
			disconnectClient(0x14, "You may only login with one character per account.");
			return false;
		}

		if(!WaitingList::getInstance()->clientLogin(player)){
			int32_t currentSlot = WaitingList::getInstance()->getClientSlot(player);
			int32_t retryTime = WaitingList::getTime(currentSlot);
			std::stringstream ss;

			ss << "Too many players online.\n" << "You are at place "
				<< currentSlot << " on the waiting list.";

			OutputMessage_ptr output = OutputMessagePool::getInstance()->getOutputMessage(this, false);
			if(output){
				TRACK_MESSAGE(output);
				output->AddByte(0x16);
				output->AddString(ss.str());
				output->AddByte(retryTime);
				OutputMessagePool::getInstance()->send(output);
			}
			getConnection()->closeConnection();
			return false;
		}

		if(!IOPlayer::instance()->loadPlayer(player, name)){
#ifdef __DEBUG__
			std::cout << "ProtocolGame::login - loadPlayer failed - " << name << std::endl;
#endif
			disconnectClient(0x14, "Your character could not be loaded.");
			return false;
		}

		if(!g_game.placeCreature(player, player->getLoginPosition())){
			if(!g_game.placeCreature(player, player->getTemplePosition(), false, true)){
				disconnectClient(0x14, "Temple position is wrong. Contact the administrator.");
				return false;
			}
		}

		player->lastip = player->getIP();
		player->lastLoginSaved = std::max(time(NULL), player->lastLoginSaved + 1);
		player->lastLoginMs = OTSYS_TIME();
		IOPlayer::instance()->updateLoginInfo(player);
		m_acceptPackets = true;

		return true;
	}
	else{
		if(eventConnect != 0){
			//A task has already been scheduled just bail out (should not be overriden)
			disconnectClient(0x14, "You are already logged in.");
			return false;
		}

		if(_player->client){
			g_chat.removeUserFromAllChannels(_player);
			_player->disconnect();
			_player->isConnecting = true;
			addRef();
			eventConnect = g_scheduler.addEvent(
				createSchedulerTask(1000, boost::bind(&ProtocolGame::connect, this, _player->getID())));
			return true;
		}

		addRef();
		return connect(_player->getID());
	}

	return false;
}

bool ProtocolGame::connect(uint32_t playerId)
{
	unRef();
	eventConnect = 0;
	Player* _player = g_game.getPlayerByID(playerId);
	if(!_player || _player->isRemoved() || _player->client){
		disconnectClient(0x14, "You are already logged in.");
		return false;
	}

	player = _player;
	player->useThing2();
	player->isConnecting = false;
	player->client = this;
	player->client->sendAddCreature(player, player->getPosition(),
		player->getTile()->__getIndexOfThing(player));
	player->lastip = player->getIP();
	IOPlayer::instance()->updateLoginInfo(player);
	m_acceptPackets = true;

	return true;
}

bool ProtocolGame::logout(bool forced)
{
	//dispatcher thread
	if(!player)
		return false;

	if(!player->isRemoved()){
		if(!forced){
			if(player->getTile()->hasFlag(TILESTATE_NOLOGOUT)){
				player->sendCancelMessage(RET_YOUCANNOTLOGOUTHERE);
				return false;
			}

			if(player->hasCondition(CONDITION_INFIGHT)){
				player->sendCancelMessage(RET_YOUMAYNOTLOGOUTDURINGAFIGHT);
				return false;
			}

			//scripting event - onLogOut
			if(!g_creatureEvents->playerLogOut(player)){
				//Let the script handle the error message
				return false;
			}
		}
	}

	if(Connection_ptr connection = getConnection()){
		connection->closeConnection();
	}

	return g_game.removeCreature(player);
}

bool ProtocolGame::parseFirstPacket(NetworkMessage& msg)
{
	if(g_game.getGameState() == GAME_STATE_SHUTDOWN){
		getConnection()->closeConnection();
		return false;
	}


	/*uint16_t clientos =*/ msg.GetU16();
	uint16_t version  = msg.GetU16();

	if(!RSA_decrypt(msg)){
		getConnection()->closeConnection();
		return false;
	}

	uint32_t key[4];
	key[0] = msg.GetU32();
	key[1] = msg.GetU32();
	key[2] = msg.GetU32();
	key[3] = msg.GetU32();
	enableXTEAEncryption();
	setXTEAKey(key);

	bool isSetGM = (msg.GetByte() == 1);
	std::string accname = msg.GetString();
	const std::string name = msg.GetString();
	const std::string password = msg.GetString();

	msg.SkipBytes(6); //841 specific

	if(version < CLIENT_VERSION_MIN || version > CLIENT_VERSION_MAX){
		disconnectClient(0x0A, STRING_CLIENT_VERSION);
		return false;
	}

	if(g_game.getGameState() == GAME_STATE_STARTUP){
		disconnectClient(0x14, "Gameworld is starting up. Please wait.");
		return false;
	}

	if(g_bans.isIpDisabled(getIP())){
		disconnectClient(0x14, "Too many connections attempts from this IP. Try again later.");
		return false;
	}

	if(g_bans.isIpBanished(getIP())){
		disconnectClient(0x14, "Your IP is banished!");
		return false;
	}

	std::string acc_pass;
	if(!(IOAccount::instance()->getPassword(accname, name, acc_pass) && passwordTest(password, acc_pass))){
		g_bans.addLoginAttempt(getIP(), false);
		getConnection()->closeConnection();
		return false;
	}

	g_bans.addLoginAttempt(getIP(), true);

	g_dispatcher.addTask(
		createTask(boost::bind(&ProtocolGame::login, this, name, isSetGM)));

	return true;
}

void ProtocolGame::onRecvFirstMessage(NetworkMessage& msg)
{
	parseFirstPacket(msg);
}

void ProtocolGame::onConnect()
{
	OutputMessage_ptr output = OutputMessagePool::getInstance()->getOutputMessage(this, false);

	// Packet length
	//output->AddU16(0x06); //length (6)

	// Packet type
	output->AddByte(0x1F);

	// Seems arbitrary, probably isn't
	output->AddByte(random_range(0, 0xFF));
	output->AddByte(random_range(0, 0xFF));

	output->AddU16(0x00);

	// Arbitrary too (?)
	output->AddByte(random_range(0, 0xFF));

	// Enable checksum
	enableChecksum();

	OutputMessagePool::getInstance()->send(output);
}

void ProtocolGame::disconnectClient(uint8_t error, const char* message)
{
	OutputMessage_ptr output = OutputMessagePool::getInstance()->getOutputMessage(this, false);
	if(output){
		TRACK_MESSAGE(output);
		output->AddByte(error);
		output->AddString(message);
		OutputMessagePool::getInstance()->send(output);
	}
	disconnect();
}

void ProtocolGame::disconnect()
{
	if(getConnection()){
		getConnection()->closeConnection();
	}
}

void ProtocolGame::parsePacket(NetworkMessage &msg)
{
	if(!player || !m_acceptPackets || g_game.getGameState() == GAME_STATE_SHUTDOWN || msg.getMessageLength() <= 0)
		return;

	uint8_t recvbyte = msg.GetByte();
	//a dead player can not performs actions
	if((player->isRemoved() || player->getHealth() <= 0) && recvbyte != 0x14){
		return;
	}

	switch(recvbyte){
	case 0x14: // logout
		parseLogout(msg);
		break;

	case 0x1E: // keep alive / ping response
		parseReceivePing(msg);
		break;

	case 0x64: // move with steps
		parseAutoWalk(msg);
		break;

	case 0x65: // move north
		parseMove(msg, NORTH);
		break;

	case 0x66: // move east
		parseMove(msg, EAST);
		break;

	case 0x67: // move south
		parseMove(msg, SOUTH);
		break;

	case 0x68: // move west
		parseMove(msg, WEST);
		break;

	case 0x69: // stop-autowalk
		parseStopAutoWalk(msg);
		break;

	case 0x6A:
		parseMove(msg, NORTHEAST);
		break;

	case 0x6B:
		parseMove(msg, SOUTHEAST);
		break;

	case 0x6C:
		parseMove(msg, SOUTHWEST);
		break;

	case 0x6D:
		parseMove(msg, NORTHWEST);
		break;

	case 0x6F: // turn north
		parseTurn(msg, NORTH);
		break;

	case 0x70: // turn east
		parseTurn(msg, EAST);
		break;

	case 0x71: // turn south
		parseTurn(msg, SOUTH);
		break;

	case 0x72: // turn west
		parseTurn(msg, WEST);
		break;

	case 0x78: // throw item
		parseThrow(msg);
		break;

	case 0x79: // description in shop window
		parseLookInShop(msg);
		break;

	case 0x7A: // player bought from shop
		parsePlayerPurchase(msg);
		break;

	case 0x7B: // player sold to shop
		parsePlayerSale(msg);
		break;

	case 0x7C: // player closed shop window
		parseCloseShop(msg);
		break;

	case 0x7D: // Request trade
		parseRequestTrade(msg);
		break;

	case 0x7E: // Look at an item in trade
		parseLookInTrade(msg);
		break;

	case 0x7F: // Accept trade
		parseAcceptTrade(msg);
		break;

	case 0x80: // Close/cancel trade
		parseCloseTrade();
		break;

	case 0x82: // use item
		parseUseItem(msg);
		break;

	case 0x83: // use item
		parseUseItemEx(msg);
		break;

	case 0x84: // battle window
		parseBattleWindow(msg);
		break;

	case 0x85:	//rotate item
		parseRotateItem(msg);
		break;

	case 0x87: // close container
		parseCloseContainer(msg);
		break;

	case 0x88: //"up-arrow" - container
		parseUpArrowContainer(msg);
		break;

	case 0x89:
		parseTextWindow(msg);
		break;

	case 0x8A:
		parseHouseWindow(msg);
		break;

	case 0x8C: // throw item
		parseLookAt(msg);
		break;

	case 0x96:  // say something
		parseSay(msg);
		break;

	case 0x97: // request Channels
		parseGetChannels(msg);
		break;

	case 0x98: // open Channel
		parseOpenChannel(msg);
		break;

	case 0x99: // close Channel
		parseCloseChannel(msg);
		break;

	case 0x9A: // open priv
		parseOpenPriv(msg);
		break;

	case 0x9B: //process report
		parseProcessRuleViolation(msg);
		break;

	case 0x9C: //gm closes report
		parseCloseRuleViolation(msg);
		break;

	case 0x9D: //player cancels report
		parseCancelRuleViolation(msg);
		break;

	case 0x9E: // close NPC
		parseCloseNpc(msg);
		break;

	case 0xA0: // set attack and follow mode
		parseFightModes(msg);
		break;

	case 0xA1: // attack
		parseAttack(msg);
		break;

	case 0xA2: //follow
		parseFollow(msg);
		break;

	case 0xA3:
		parseInviteToParty(msg);
		break;

	case 0xA4:
		parseJoinParty(msg);
		break;

	case 0xA5:
		parseRevokePartyInvitation(msg);
		break;

	case 0xA6:
		parsePassPartyLeadership(msg);
		break;

	case 0xA7:
		parseLeaveParty(msg);
		break;

	case 0xA8:
		parseEnableSharedPartyExperience(msg);
		break;

	case 0xAA:
		parseCreatePrivateChannel(msg);
		break;

	case 0xAB:
		parseChannelInvite(msg);
		break;

	case 0xAC:
		parseChannelExclude(msg);
		break;

	case 0xBE: // cancel move
		parseCancelMove(msg);
		break;

	case 0xC9: //client request to resend the tile
		parseUpdateTile(msg);
		break;

	case 0xCA: //client request to resend the container (happens when you store more than container maxsize)
		parseUpdateContainer(msg);
		break;

	case 0xD2: // request outfit
		parseRequestOutfit(msg);
		break;

	case 0xD3: // set outfit
		parseSetOutfit(msg);
		break;

	case 0xD4: // mount/unmount
		parseMountCreature(msg);
		break;

	case 0xDC:
		parseAddVip(msg);
		break;

	case 0xDD:
		parseRemoveVip(msg);
		break;

	case 0xE6:
		parseBugReport(msg);
		break;

	case 0xE7:
		parseViolationWindow(msg);
		break;

	case 0xE8:
		parseDebugAssert(msg);
		break;

	case 0xF0:
		parseQuestLog(msg);
		break;

	case 0xF1:
		parseQuestLine(msg);
		break;

	default:
#ifdef __DEBUG__
		printf("unknown packet header: %x \n", recvbyte);
		parseDebug(msg);
#endif
		break;
	}
}

void ProtocolGame::GetTileDescription(const Tile* tile, NetworkMessage_ptr msg)
{
	if(tile){
		int count = 0;
		if(tile->ground){
			msg->AddItem(tile->ground);
			count++;
		}

		const TileItemVector* items = tile->getItemList();
		const CreatureVector* creatures = tile->getCreatures();

		ItemVector::const_iterator it;
		if(items){
			for(it = items->getBeginTopItem(); ((it != items->getEndTopItem()) && (count < 10)); ++it){
				msg->AddItem(*it);
				count++;
			}
		}

		if(creatures){
			CreatureVector::const_reverse_iterator cit;
			for(cit = creatures->rbegin(); ((cit != creatures->rend()) && (count < 10)); ++cit){
				if(player->canSeeCreature(*cit)){
					bool known;
					uint32_t removedKnown;
					checkCreatureAsKnown((*cit)->getID(), known, removedKnown);
					AddCreature(msg, *cit, known, removedKnown);
					count++;
				}
			}
		}

		if(items){
			for(it = items->getBeginDownItem(); ((it != items->getEndDownItem()) && (count < 10)); ++it){
				msg->AddItem(*it);
				count++;
			}
		}
	}
}

void ProtocolGame::GetMapDescription(int32_t x, int32_t y, int32_t z,
	int32_t width, int32_t height, NetworkMessage_ptr msg)
{
	int32_t skip = -1;
	int32_t startz, endz, zstep = 0;

	if(z > 7){
		startz = z - 2;
		endz = std::min((int32_t)MAP_MAX_LAYERS - 1, (int32_t)z + 2);
		zstep = 1;
	}
	else{
		startz = 7;
		endz = 0;

		zstep = -1;
	}

	for(int32_t nz = startz; nz != endz + zstep; nz += zstep){
		GetFloorDescription(msg, x, y, nz, width, height, z - nz, skip);
	}

	if(skip >= 0){
		msg->AddByte(skip);
		msg->AddByte(0xFF);
	}

#ifdef __DEBUG__
	//printf("tiles in total: %d \n", cc);
#endif
}

void ProtocolGame::GetFloorDescription(NetworkMessage_ptr msg, int32_t x, int32_t y, int32_t z,
	int32_t width, int32_t height, int32_t offset, int32_t& skip)
{
	Tile* tile;

	for(int32_t nx = 0; nx < width; ++nx){
		for(int32_t ny = 0; ny < height; ++ny){
			tile = g_game.getTile(x + nx + offset, y + ny + offset, z);

			if(tile){
				if(skip >= 0){
					msg->AddByte(skip);
					msg->AddByte(0xFF);
				}
				skip = 0;


				GetTileDescription(tile, msg);
			}
			else {
				skip++;
				if(skip == 0xFF){
					msg->AddByte(0xFF);
					msg->AddByte(0xFF);
					skip = -1;
				}
			}
		}
	}
}

void ProtocolGame::checkCreatureAsKnown(uint32_t id, bool &known, uint32_t &removedKnown)
{
	// loop through the known player and check if the given player is in
	std::list<uint32_t>::iterator i;
	for(i = knownCreatureList.begin(); i != knownCreatureList.end(); ++i){
		if((*i) == id){
			// know... make the creature even more known...
			knownCreatureList.erase(i);
			knownCreatureList.push_back(id);

			known = true;
			return;
		}
	}

	// ok, he is unknown...
	known = false;

	// ... but not in future
	knownCreatureList.push_back(id);

	// to many known creatures?
	if(knownCreatureList.size() > 250){
		// lets try to remove one from the end of the list
		for (int n = 0; n < 250; ++n){
			removedKnown = knownCreatureList.front();

			Creature* c = g_game.getCreatureByID(removedKnown);
			if ((!c) || (!canSee(c)))
				break;

			// this creature we can't remove, still in sight, so back to the end
			knownCreatureList.pop_front();
			knownCreatureList.push_back(removedKnown);
		}

		// hopefully we found someone to remove :S, we got only 150 tries
		// if not... lets kick some players with debug errors :)
		knownCreatureList.pop_front();
	}
	else{
		// we can cache without problems :)
		removedKnown = 0;
	}
}

bool ProtocolGame::canSee(const Creature* c) const
{
	if(c->isRemoved())
		return false;

	if(!player->canSeeCreature(c))
		return false;

	return canSee(c->getPosition());
}

bool ProtocolGame::canSee(const Position& pos) const
{
	return canSee(pos.x, pos.y, pos.z);
}

bool ProtocolGame::canSee(int x, int y, int z) const
{
#ifdef __DEBUG__
	if(z < 0 || z >= MAP_MAX_LAYERS) {
		std::cout << "WARNING! ProtocolGame::canSee() Z-value is out of range!" << std::endl;
	}
#endif

	const Position& myPos = player->getPosition();

	if(myPos.z <= 7){
		//we are on ground level or above (7 -> 0)
		//view is from 7 -> 0
		if(z > 7){
			return false;
		}
	}
	else if(myPos.z >= 8){
		//we are underground (8 -> 15)
		//view is +/- 2 from the floor we stand on
		if(std::abs(myPos.z - z) > 2){
			return false;
		}
	}

	//negative offset means that the action taken place is on a lower floor than ourself
	int offsetz = myPos.z - z;

	if ((x >= myPos.x - 8 + offsetz) && (x <= myPos.x + 9 + offsetz) &&
		(y >= myPos.y - 6 + offsetz) && (y <= myPos.y + 7 + offsetz))
		return true;

	return false;
}

//********************** Parse methods *******************************
void ProtocolGame::parseLogout(NetworkMessage& msg)
{
	g_dispatcher.addTask(createTask(boost::bind(&ProtocolGame::logout, this, false)));
}

void ProtocolGame::parseCreatePrivateChannel(NetworkMessage& msg)
{
	addGameTask(&Game::playerCreatePrivateChannel, player->getID());
}

void ProtocolGame::parseChannelInvite(NetworkMessage& msg)
{
	const std::string name = msg.GetString();

	addGameTask(&Game::playerChannelInvite, player->getID(), name);
}

void ProtocolGame::parseChannelExclude(NetworkMessage& msg)
{
	const std::string name = msg.GetString();

	addGameTask(&Game::playerChannelExclude, player->getID(), name);
}

void ProtocolGame::parseGetChannels(NetworkMessage& msg)
{
	addGameTask(&Game::playerRequestChannels, player->getID());
}

void ProtocolGame::parseOpenChannel(NetworkMessage& msg)
{
	uint16_t channelId = msg.GetU16();

	addGameTask(&Game::playerOpenChannel, player->getID(), channelId);
}

void ProtocolGame::parseCloseChannel(NetworkMessage &msg)
{
	uint16_t channelId = msg.GetU16();

	addGameTask(&Game::playerCloseChannel, player->getID(), channelId);
}

void ProtocolGame::parseOpenPriv(NetworkMessage& msg)
{
	const std::string receiver = msg.GetString();

	addGameTask(&Game::playerOpenPrivateChannel, player->getID(), receiver);
}

void ProtocolGame::parseProcessRuleViolation(NetworkMessage& msg)
{
	const std::string reporter = msg.GetString();

	addGameTask(&Game::playerProcessRuleViolation, player->getID(), reporter);
}

void ProtocolGame::parseCloseRuleViolation(NetworkMessage& msg)
{
	const std::string reporter = msg.GetString();

	addGameTask(&Game::playerCloseRuleViolation, player->getID(), reporter);
}

void ProtocolGame::parseCancelRuleViolation(NetworkMessage& msg)
{
	addGameTask(&Game::playerCancelRuleViolation, player->getID());
}

void ProtocolGame::parseCloseNpc(NetworkMessage& msg)
{
	addGameTask(&Game::playerCloseNpcChannel, player->getID());
}

void ProtocolGame::parseCancelMove(NetworkMessage& msg)
{
	addGameTask(&Game::playerCancelAttackAndFollow, player->getID());
}

void ProtocolGame::parseDebug(NetworkMessage& msg)
{
	int dataLength = msg.getMessageLength() - 1;
	if(dataLength != 0){
		printf("data: ");
		int data = msg.GetByte();
		while(dataLength > 0){
			printf("%d ", data);
			if(--dataLength > 0)
				data = msg.GetByte();
		}
		printf("\n");
	}
}

void ProtocolGame::parseReceivePing(NetworkMessage& msg)
{
	g_dispatcher.addTask(
		createTask(boost::bind(&Game::playerReceivePing, &g_game, player->getID())));
}

void ProtocolGame::parseAutoWalk(NetworkMessage& msg)
{
	// first we get all directions...
	std::list<Direction> path;
	size_t numdirs = msg.GetByte();
	for (size_t i = 0; i < numdirs; ++i) {
		uint8_t rawdir = msg.GetByte();
		Direction dir = SOUTH;

		switch(rawdir) {
		case 1: dir = EAST; break;
		case 2: dir = NORTHEAST; break;
		case 3: dir = NORTH; break;
		case 4: dir = NORTHWEST; break;
		case 5: dir = WEST; break;
		case 6: dir = SOUTHWEST; break;
		case 7: dir = SOUTH; break;
		case 8: dir = SOUTHEAST; break;

		default:
			continue;
		};

		/*
		#ifdef __DEBUG__
		std::cout << "Walk by mouse: Direction: " << dir << std::endl;
		#endif
		*/

		path.push_back(dir);
	}

	addGameTask(&Game::playerAutoWalk, player->getID(), path);
}

void ProtocolGame::parseStopAutoWalk(NetworkMessage& msg)
{
	addGameTask(&Game::playerStopAutoWalk, player->getID());
}

void ProtocolGame::parseMove(NetworkMessage& msg, Direction dir)
{
	addGameTask(&Game::playerMove, player->getID(), dir);
}

void ProtocolGame::parseTurn(NetworkMessage& msg, Direction dir)
{
	addGameTaskTimed(DISPATCHER_TASK_EXPIRATION, &Game::playerTurn, player->getID(), dir);
}

void ProtocolGame::parseRequestOutfit(NetworkMessage& msg)
{
	addGameTask(&Game::playerRequestOutfit, player->getID());
}

void ProtocolGame::parseSetOutfit(NetworkMessage& msg)
{
	uint16_t looktype = msg.GetU16();
	uint8_t lookhead = msg.GetByte();
	uint8_t lookbody = msg.GetByte();
	uint8_t looklegs = msg.GetByte();
	uint8_t lookfeet = msg.GetByte();
	uint8_t lookaddons = msg.GetByte();
	uint16_t lookmount = msg.GetU16();

	Outfit_t newOutfit;
	newOutfit.lookType = looktype;
	newOutfit.lookHead = lookhead;
	newOutfit.lookBody = lookbody;
	newOutfit.lookLegs = looklegs;
	newOutfit.lookFeet = lookfeet;
	newOutfit.lookAddons = lookaddons;
	newOutfit.lookMount = lookmount;

	addGameTask(&Game::playerChangeOutfit, player->getID(), newOutfit);
}

void ProtocolGame::parseMountCreature(NetworkMessage& msg)
{
	bool mount = msg.GetByte() == 1;
	addGameTask(&Game::playerMountCreature, player->getID(), mount);
}

void ProtocolGame::parseUseItem(NetworkMessage& msg)
{
	Position pos = msg.GetPosition();
	uint16_t spriteId = msg.GetSpriteId();
	uint8_t stackpos = msg.GetByte();
	uint8_t index = msg.GetByte();
	bool isHotkey = (pos.x == 0xFFFF && pos.y == 0 && pos.z == 0);

	addGameTaskTimed(DISPATCHER_TASK_EXPIRATION, &Game::playerUseItem, player->getID(), pos, stackpos, index, spriteId, isHotkey);
}

void ProtocolGame::parseUseItemEx(NetworkMessage& msg)
{
	Position fromPos = msg.GetPosition();
	uint16_t fromSpriteId = msg.GetSpriteId();
	uint8_t fromStackPos = msg.GetByte();
	Position toPos = msg.GetPosition();
	uint16_t toSpriteId = msg.GetU16();
	uint8_t toStackPos = msg.GetByte();
	bool isHotkey = (fromPos.x == 0xFFFF && fromPos.y == 0 && fromPos.z == 0);

	addGameTaskTimed(DISPATCHER_TASK_EXPIRATION, &Game::playerUseItemEx, player->getID(), fromPos, fromStackPos, fromSpriteId, toPos, toStackPos, toSpriteId, isHotkey);
}

void ProtocolGame::parseBattleWindow(NetworkMessage &msg)
{
	Position fromPos = msg.GetPosition();
	uint16_t spriteId = msg.GetSpriteId();
	uint8_t fromStackPos = msg.GetByte();
	uint32_t creatureId = msg.GetU32();
	bool isHotkey = (fromPos.x == 0xFFFF && fromPos.y == 0 && fromPos.z == 0);

	addGameTaskTimed(DISPATCHER_TASK_EXPIRATION, &Game::playerUseBattleWindow, player->getID(), fromPos, fromStackPos, creatureId, spriteId, isHotkey);
}

void ProtocolGame::parseCloseContainer(NetworkMessage& msg)
{
	uint8_t cid = msg.GetByte();

	addGameTask(&Game::playerCloseContainer, player->getID(), cid);
}

void ProtocolGame::parseUpArrowContainer(NetworkMessage& msg)
{
	uint8_t cid = msg.GetByte();

	addGameTask(&Game::playerMoveUpContainer, player->getID(), cid);
}

void ProtocolGame::parseUpdateTile(NetworkMessage& msg)
{
	Position pos = msg.GetPosition();

	//addGameTask(&Game::playerUpdateTile, player->getID(), pos);
}

void ProtocolGame::parseUpdateContainer(NetworkMessage& msg)
{
	uint8_t cid = msg.GetByte();

	addGameTask(&Game::playerUpdateContainer, player->getID(), cid);
}

void ProtocolGame::parseThrow(NetworkMessage& msg)
{
	Position fromPos = msg.GetPosition();
	uint16_t spriteId = msg.GetSpriteId();
	uint8_t fromStackpos = msg.GetByte();
	Position toPos = msg.GetPosition();
	uint8_t count = msg.GetByte();

	/*
	std::cout << "parseThrow: " << "from_x: " << (int)fromPos.x << ", from_y: " << (int)fromPos.y
	<<  ", from_z: " << (int)fromPos.z << ", item: " << (int)itemId << ", fromStackpos: "
	<< (int)fromStackpos << " to_x: " << (int)toPos.x << ", to_y: " << (int)toPos.y
	<<  ", to_z: " << (int)toPos.z
	<< ", count: " << (int)count << std::endl;
	*/

	if(toPos != fromPos){
		addGameTaskTimed(DISPATCHER_TASK_EXPIRATION,
			&Game::playerMoveThing, player->getID(), fromPos, spriteId,
			fromStackpos, toPos, count);
	}
}

void ProtocolGame::parseLookAt(NetworkMessage& msg)
{
	Position pos = msg.GetPosition();
	uint16_t spriteId = msg.GetSpriteId();
	uint8_t stackpos = msg.GetByte();

/*
#ifdef __DEBUG__
	ss << "You look at x: " << x <<", y: " << y << ", z: " << z << " and see Item # " << itemId << ".";
#endif
*/

	addGameTaskTimed(DISPATCHER_TASK_EXPIRATION, &Game::playerLookAt, player->getID(), pos, spriteId, stackpos);
}

void ProtocolGame::parseSay(NetworkMessage& msg)
{
	SpeakClasses type = (SpeakClasses)msg.GetByte();

	std::string receiver;
	uint16_t channelId = 0;
	switch(type){
	case SPEAK_PRIVATE:
	case SPEAK_PRIVATE_RED:
	case SPEAK_RVR_ANSWER:
		receiver = msg.GetString();
		break;
	case SPEAK_CHANNEL_Y:
	case SPEAK_CHANNEL_R1:
	case SPEAK_CHANNEL_R2:
		channelId = msg.GetU16();
		break;
	default:
		break;
	}

	const std::string text = msg.GetString();
	if(text.length() > 255)
		return;
	addGameTaskTimed(DISPATCHER_TASK_EXPIRATION, &Game::playerSay, player->getID(), channelId, type, receiver, text);
}

void ProtocolGame::parseFightModes(NetworkMessage& msg)
{
	uint8_t rawFightMode = msg.GetByte(); //1 - offensive, 2 - balanced, 3 - defensive
	uint8_t rawChaseMode = msg.GetByte(); // 0 - stand while fightning, 1 - chase opponent
	uint8_t rawSafeMode = msg.GetByte();

	bool safeMode = (rawSafeMode == 1);
	chaseMode_t chaseMode = CHASEMODE_STANDSTILL;

	if(rawChaseMode == 0){
		chaseMode = CHASEMODE_STANDSTILL;
	}
	else if(rawChaseMode == 1){
		chaseMode = CHASEMODE_FOLLOW;
	}

	fightMode_t fightMode = FIGHTMODE_ATTACK;

	if(rawFightMode == 1){
		fightMode = FIGHTMODE_ATTACK;
	}
	else if(rawFightMode == 2){
		fightMode = FIGHTMODE_BALANCED;
	}
	else if(rawFightMode == 3){
		fightMode = FIGHTMODE_DEFENSE;
	}

	addGameTaskTimed(DISPATCHER_TASK_EXPIRATION, &Game::playerSetFightModes, player->getID(), fightMode, chaseMode, safeMode);
}

void ProtocolGame::parseAttack(NetworkMessage& msg)
{
	uint32_t creatureId = msg.GetU32();
	msg.GetU32();
	msg.GetU32();

	addGameTask(&Game::playerSetAttackedCreature, player->getID(), creatureId);
}

void ProtocolGame::parseFollow(NetworkMessage& msg)
{
	uint32_t creatureId = msg.GetU32();

	addGameTask(&Game::playerFollowCreature, player->getID(), creatureId);
}

void ProtocolGame::parseInviteToParty(NetworkMessage& msg)
{
	uint32_t creatureId = msg.GetU32();

	addGameTask(&Game::playerInviteToParty, player->getID(), creatureId);
}

void ProtocolGame::parseJoinParty(NetworkMessage& msg)
{
	uint32_t creatureId = msg.GetU32();

	addGameTask(&Game::playerJoinParty, player->getID(), creatureId);
}

void ProtocolGame::parseRevokePartyInvitation(NetworkMessage& msg)
{
	uint32_t creatureId = msg.GetU32();

	addGameTask(&Game::playerRevokePartyInvitation, player->getID(), creatureId);
}

void ProtocolGame::parsePassPartyLeadership(NetworkMessage& msg)
{
	uint32_t creatureId = msg.GetU32();

	addGameTask(&Game::playerPassPartyLeadership, player->getID(), creatureId);
}

void ProtocolGame::parseLeaveParty(NetworkMessage& msg)
{
	addGameTask(&Game::playerLeaveParty, player->getID());
}

void ProtocolGame::parseEnableSharedPartyExperience(NetworkMessage& msg)
{
	uint8_t sharedExpActive = msg.GetByte();
	uint8_t unknown = msg.GetByte();
	addGameTask(&Game::playerEnableSharedPartyExperience, player->getID(), sharedExpActive, unknown);
}

void ProtocolGame::parseQuestLog(NetworkMessage& msg)
{
	addGameTaskTimed(DISPATCHER_TASK_EXPIRATION, &Game::playerShowQuestLog, player->getID());
}

void ProtocolGame::parseQuestLine(NetworkMessage& msg)
{
	uint16_t questId = msg.GetU16();

	addGameTask(&Game::playerShowQuestLine, player->getID(), questId);
}

void ProtocolGame::parseTextWindow(NetworkMessage& msg)
{
	uint32_t windowTextId = msg.GetU32();
	const std::string newText = msg.GetString();

	addGameTask(&Game::playerWriteItem, player->getID(), windowTextId, newText);
}

void ProtocolGame::parseHouseWindow(NetworkMessage &msg)
{
	uint8_t doorId = msg.GetByte();
	uint32_t id = msg.GetU32();
	const std::string text = msg.GetString();

	addGameTask(&Game::playerUpdateHouseWindow, player->getID(), doorId, id, text);
}

void ProtocolGame::parseLookInShop(NetworkMessage &msg)
{
	uint16_t id = msg.GetU16();
	uint8_t count = msg.GetByte();

	addGameTaskTimed(DISPATCHER_TASK_EXPIRATION, &Game::playerLookInShop, player->getID(), id, count);
}

void ProtocolGame::parsePlayerPurchase(NetworkMessage &msg)
{
	uint16_t id = msg.GetU16();
	uint8_t count = msg.GetByte();
	uint8_t amount = msg.GetByte();
	bool ignoreCapacity = (msg.GetByte() == 0x01);
	bool buyWithBackpack = (msg.GetByte() == 0x01);
	addGameTaskTimed(DISPATCHER_TASK_EXPIRATION, &Game::playerPurchaseItem, player->getID(), id, count, amount, ignoreCapacity, buyWithBackpack);
}

void ProtocolGame::parsePlayerSale(NetworkMessage &msg)
{
	uint16_t id = msg.GetU16();
	uint8_t count = msg.GetByte();
	uint8_t amount = msg.GetByte();
	bool ignoreEquipped = (msg.GetByte() == 0x01);
	addGameTaskTimed(DISPATCHER_TASK_EXPIRATION, &Game::playerSellItem, player->getID(), id, count, amount, ignoreEquipped);
}

void ProtocolGame::parseCloseShop(NetworkMessage &msg)
{
	addGameTask(&Game::playerCloseShop, player->getID());
}

void ProtocolGame::parseRequestTrade(NetworkMessage& msg)
{
	Position pos = msg.GetPosition();
	uint16_t spriteId = msg.GetSpriteId();
	uint8_t stackpos = msg.GetByte();
	uint32_t playerId = msg.GetU32();

	addGameTask(&Game::playerRequestTrade, player->getID(), pos, stackpos, playerId, spriteId);
}

void ProtocolGame::parseAcceptTrade(NetworkMessage& msg)
{
	addGameTask(&Game::playerAcceptTrade, player->getID());
}

void ProtocolGame::parseLookInTrade(NetworkMessage& msg)
{
	bool counterOffer = (msg.GetByte() == 0x01);
	uint8_t index = msg.GetByte();

	addGameTaskTimed(DISPATCHER_TASK_EXPIRATION, &Game::playerLookInTrade, player->getID(), counterOffer, index);
}

void ProtocolGame::parseCloseTrade()
{
	addGameTask(&Game::playerCloseTrade, player->getID());
}

void ProtocolGame::parseAddVip(NetworkMessage& msg)
{
	const std::string name = msg.GetString();
	if(name.size() > 32)
		return;

	addGameTask(&Game::playerRequestAddVip, player->getID(), name);
}

void ProtocolGame::parseRemoveVip(NetworkMessage& msg)
{
	uint32_t guid = msg.GetU32();

	addGameTask(&Game::playerRequestRemoveVip, player->getID(), guid);
}

void ProtocolGame::parseRotateItem(NetworkMessage& msg)
{
	Position pos = msg.GetPosition();
	uint16_t spriteId = msg.GetSpriteId();
	uint8_t stackpos = msg.GetByte();

	addGameTaskTimed(DISPATCHER_TASK_EXPIRATION, &Game::playerRotateItem, player->getID(), pos, stackpos, spriteId);
}

void ProtocolGame::parseViolationWindow(NetworkMessage& msg)
{
	std::string target = msg.GetString();
	uint8_t reason = msg.GetByte();
	violationAction_t action = (violationAction_t)msg.GetByte();
	std::string comment = msg.GetString();
	std::string statement = (action == ACTION_STATEMENT ? "" : msg.GetString());
	uint16_t channelId = msg.GetU16();
	bool ipBanishment = msg.GetByte() != 0;
	addGameTask(&Game::playerViolationWindow, player->getID(), target, reason, action, comment, statement, channelId, ipBanishment);
}

void ProtocolGame::parseBugReport(NetworkMessage& msg)
{
	std::string comment = msg.GetString();
	addGameTask(&Game::playerReportBug, player->getID(), comment);
}

void ProtocolGame::parseDebugAssert(NetworkMessage& msg)
{
	if(!g_config.getNumber(ConfigManager::SAVE_CLIENT_DEBUG_ASSERTIONS)){
		return;
	}

	//only accept 1 report each time
	if(m_debugAssertSent){
		return;
	}
	m_debugAssertSent = true;

	std::string assertLine = msg.GetString();
	std::string report_date = msg.GetString();
	std::string description = msg.GetString();
	std::string comment = msg.GetString();

	//write it in the assertions file
	std::ofstream of("client_assertions.txt", std::ios_base::app);
	if(of){
		char today[32];
		formatDate(time(NULL), today);

		of <<
			"-----" << today << " - " << player->getName() << " (" << convertIPToString(player->getIP()) <<
			assertLine << std::endl <<
			report_date << std::endl <<
			description << std::endl <<
			comment << std::endl;
	}
}

//********************** Send methods  *******************************
void ProtocolGame::sendOpenPrivateChannel(const std::string& receiver)
{
	NetworkMessage_ptr msg = getOutputBuffer();
	if(msg){
		TRACK_MESSAGE(msg);
		msg->AddByte(0xAD);
		msg->AddString(receiver);
	}
}

void ProtocolGame::sendCreatureOutfit(const Creature* creature, const Outfit_t& outfit)
{
	if(canSee(creature)){
		NetworkMessage_ptr msg = getOutputBuffer();
		if(msg){
			TRACK_MESSAGE(msg);
			msg->AddByte(0x8E);
			msg->AddU32(creature->getID());
			AddCreatureOutfit(msg, creature, outfit);
		}
	}
}

void ProtocolGame::sendCreatureLight(const Creature* creature)
{
	if(canSee(creature)){
		NetworkMessage_ptr msg = getOutputBuffer();
		if(msg){
			TRACK_MESSAGE(msg);
			AddCreatureLight(msg, creature);
		}
	}
}

void ProtocolGame::sendWorldLight(const LightInfo& lightInfo)
{
	NetworkMessage_ptr msg = getOutputBuffer();
	if(msg){
		TRACK_MESSAGE(msg);
		AddWorldLight(msg, lightInfo);
	}
}

void ProtocolGame::sendCreatureSkull(const Creature* creature)
{
	if(canSee(creature)){
		NetworkMessage_ptr msg = getOutputBuffer();
		if(msg){
			TRACK_MESSAGE(msg);
			msg->AddByte(0x90);
			msg->AddU32(creature->getID());
			#ifdef __SKULLSYSTEM__
			msg->AddByte(player->getSkullClient(creature->getPlayer()));
			#else
			msg->AddByte(SKULL_NONE);
			#endif
		}
	}
}
#ifdef __GUILDWARSLUARELOAD__
void ProtocolGame::sendCreatureEmblem(const Creature* creature)
{
	if(!canSee(creature))
		return;

	// we are cheating the client in here!
	uint32_t stackpos = creature->getTile()->getClientIndexOfThing(player, creature);
	if(stackpos >= 10)
		return;

	NetworkMessage_ptr msg = getOutputBuffer();
	if(msg)
	{
		TRACK_MESSAGE(msg);
		std::list<uint32_t>::iterator it = std::find(knownCreatureList.begin(), knownCreatureList.end(), creature->getID());
		if(it != knownCreatureList.end())
		{
			RemoveTileItem(msg, creature->getPosition(), stackpos);
			msg->AddByte(0x6A);

			msg->AddPosition(creature->getPosition());
			msg->AddByte(stackpos);
			AddCreature(msg, creature, false, creature->getID());
		}
		else
			AddTileCreature(msg, creature->getPosition(), stackpos, creature);
	}
}
#endif
void ProtocolGame::sendCreatureShield(const Creature* creature)
{
	if(canSee(creature)){
		NetworkMessage_ptr msg = getOutputBuffer();
		if(msg){
			TRACK_MESSAGE(msg);
			msg->AddByte(0x91);
			msg->AddU32(creature->getID());
			msg->AddByte(player->getPartyShield(creature->getPlayer()));
		}
	}
}

void ProtocolGame::sendCreatureSquare(const Creature* creature, uint8_t color)
{
	if(canSee(creature)){
		NetworkMessage_ptr msg = getOutputBuffer();
		if(msg){
			TRACK_MESSAGE(msg);
			msg->AddByte(0x86);
			msg->AddU32(creature->getID());
			msg->AddByte(color);
		}
	}
}

void ProtocolGame::sendStats()
{
	NetworkMessage_ptr msg = getOutputBuffer();
	if(msg){
		TRACK_MESSAGE(msg);
		AddPlayerStats(msg);
	}
}

void ProtocolGame::sendTextMessage(MessageClasses mclass, const std::string& message)
{
	NetworkMessage_ptr msg = getOutputBuffer();
	if(msg){
		TRACK_MESSAGE(msg);
		AddTextMessage(msg, mclass, message);
	}
}

void ProtocolGame::sendClosePrivate(uint16_t channelId)
{
	NetworkMessage_ptr msg = getOutputBuffer();
	if(msg){
		TRACK_MESSAGE(msg);
		msg->AddByte(0xB3);
		msg->AddU16(channelId);
	}
}

void ProtocolGame::sendCreatePrivateChannel(uint16_t channelId, const std::string& channelName)
{
	NetworkMessage_ptr msg = getOutputBuffer();
	if(msg){
		TRACK_MESSAGE(msg);
		msg->AddByte(0xB2);
		msg->AddU16(channelId);
		msg->AddString(channelName);
	}
}

void ProtocolGame::sendChannelsDialog()
{
	NetworkMessage_ptr msg = getOutputBuffer();
	if(msg){
		TRACK_MESSAGE(msg);
		ChannelList list;
		list = g_chat.getChannelList(player);

		msg->AddByte(0xAB);
		msg->AddByte(list.size());

		while(list.size()){
			ChatChannel *channel;
			channel = list.front();
			list.pop_front();

			msg->AddU16(channel->getId());
			msg->AddString(channel->getName());
		}
	}
}

void ProtocolGame::sendChannel(uint16_t channelId, const std::string& channelName)
{
	NetworkMessage_ptr msg = getOutputBuffer();
	if(msg){
		TRACK_MESSAGE(msg);
		msg->AddByte(0xAC);
		msg->AddU16(channelId);
		msg->AddString(channelName);
	}
}

void ProtocolGame::sendRuleViolationsChannel(uint16_t channelId)
{
	NetworkMessage_ptr msg = getOutputBuffer();
	if(msg){
		TRACK_MESSAGE(msg);
		msg->AddByte(0xAE);
		msg->AddU16(channelId);
		RuleViolationsMap::const_iterator it = g_game.getRuleViolations().begin();
		for( ; it != g_game.getRuleViolations().end(); ++it){
			RuleViolation& rvr = *it->second;
			if(rvr.isOpen && rvr.reporter){
				AddCreatureSpeak(msg, rvr.reporter, SPEAK_RVR_CHANNEL, rvr.text, channelId, rvr.time);
			}
		}
	}
}

void ProtocolGame::sendRemoveReport(const std::string& name)
{
	NetworkMessage_ptr msg = getOutputBuffer();
	if(msg){
		TRACK_MESSAGE(msg);
		msg->AddByte(0xAF);
		msg->AddString(name);
	}
}

void ProtocolGame::sendRuleViolationCancel(const std::string& name)
{
	NetworkMessage_ptr msg = getOutputBuffer();
	if(msg){
		TRACK_MESSAGE(msg);
		msg->AddByte(0xB0);
		msg->AddString(name);
	}
}

void ProtocolGame::sendLockRuleViolation()
{
	NetworkMessage_ptr msg = getOutputBuffer();
	if(msg){
		TRACK_MESSAGE(msg);
		msg->AddByte(0xB1);
	}
}

void ProtocolGame::sendIcons(uint16_t icons)
{
	NetworkMessage_ptr msg = getOutputBuffer();
	if(msg){
		TRACK_MESSAGE(msg);
		msg->AddByte(0xA2);
		msg->AddU16(icons);
	}
}

void ProtocolGame::sendContainer(uint32_t cid, const Container* container, bool hasParent)
{
	NetworkMessage_ptr msg = getOutputBuffer();
	if(msg){
		TRACK_MESSAGE(msg);
		msg->AddByte(0x6E);
		msg->AddByte(cid);
		msg->AddItemId(container);
		msg->AddString(container->getName());
		msg->AddByte(container->capacity());
		msg->AddByte(hasParent ? 0x01 : 0x00);
		if(container->size() > 255){
			msg->AddByte(255);
		}
		else{
			msg->AddByte(container->size());
		}

		ItemList::const_iterator cit;
		uint32_t i = 0;
		for(cit = container->getItems(); cit != container->getEnd() && i < 255; ++cit, ++i){
			msg->AddItem(*cit);
		}
	}
}

void ProtocolGame::sendShop(const std::list<ShopInfo>& shop)
{
	NetworkMessage_ptr msg = getOutputBuffer();
	if(msg){
		TRACK_MESSAGE(msg);
		msg->AddByte(0x7A);
		msg->AddByte(std::min((size_t)255, shop.size()));

		std::list<ShopInfo>::const_iterator it;
		uint32_t i = 0;
		for(it = shop.begin(); it != shop.end() && i < 255; ++it, ++i){
			AddShopItem(msg, (*it));
		}
	}
}

void ProtocolGame::sendCloseShop()
{
	NetworkMessage_ptr msg = getOutputBuffer();
	if(msg){
		TRACK_MESSAGE(msg);
		msg->AddByte(0x7C);
	}
}

void ProtocolGame::sendSaleItemList(const std::list<ShopInfo>& shop)
{
	NetworkMessage_ptr msg = getOutputBuffer();
	if(msg){
		msg->AddByte(0x7B);
		msg->AddU32(g_game.getMoney(player));

		std::map<uint32_t, uint32_t> saleMap;

		if(shop.size() <= 5){
			// For very small shops it's not worth it to create the complete map
			for(std::list<ShopInfo>::const_iterator it = shop.begin(); it != shop.end(); ++it){
				const ShopInfo& sInfo = *it;
				if(sInfo.sellPrice > 0){
					int8_t subtype = -1;
					const ItemType& it = Item::items[sInfo.itemId];
					if(it.hasSubType() && !it.stackable){
						subtype = (sInfo.subType == 0 ? -1 :sInfo.subType);
					}

					uint32_t count = player->__getItemTypeCount(sInfo.itemId, subtype);
					if(count > 0){
						saleMap[sInfo.itemId] = count;
					}
				}
			}
		}
		else{
			// Large shop, it's better to get a cached map of all item counts and use it
			// We need a temporary map since the finished map should only contain items
			// available in the shop
			std::map<uint32_t, uint32_t> tempSaleMap;
			player->__getAllItemTypeCount(tempSaleMap);

			// We must still check manually for the special items that require subtype matches
			// (That is, fluids such as potions etc., actually these items are very few since
			// health potions now use their own ID)
			for(std::list<ShopInfo>::const_iterator it = shop.begin(); it != shop.end(); ++it){
				const ShopInfo& sInfo = *it;
				if(sInfo.sellPrice > 0){
					int8_t subtype = -1;
					const ItemType& it = Item::items[sInfo.itemId];
					if(it.hasSubType() && !it.stackable){
						subtype = (sInfo.subType == 0 ? -1 :sInfo.subType);
					}

					if(subtype != -1){
						// This shop item requires extra checks
						uint32_t count = player->__getItemTypeCount(sInfo.itemId, subtype);

						if(count > 0)
							saleMap[sInfo.itemId] = count;
						else
							// No count if you include subtypes in the calculations
							saleMap[sInfo.itemId] = 0;
					}
					else{
						saleMap[sInfo.itemId] = tempSaleMap[sInfo.itemId];
					}
				}
			}
		}
		msg->AddByte(std::min((size_t)255, saleMap.size()));

		uint32_t i = 0;
		for(std::map<uint32_t, uint32_t>::const_iterator it = saleMap.begin(); it != saleMap.end() && i < 255; ++it, ++i){
			msg->AddItemId(it->first);
			msg->AddByte(std::min((uint32_t)255, it->second));
		}
	}
}

void ProtocolGame::sendTradeItemRequest(const Player* player, const Item* item, bool ack)
{
	NetworkMessage_ptr msg = getOutputBuffer();
	if(msg){
		TRACK_MESSAGE(msg);
		if(ack){
			msg->AddByte(0x7D);
		}
		else{
			msg->AddByte(0x7E);
		}

		msg->AddString(player->getName());

		if(const Container* tradeContainer = item->getContainer()){
			std::list<const Container*> listContainer;
			ItemList::const_iterator it;
			Container* tmpContainer = NULL;

			listContainer.push_back(tradeContainer);

			std::list<const Item*> listItem;
			listItem.push_back(tradeContainer);

			while(!listContainer.empty()) {
				const Container* container = listContainer.front();
				listContainer.pop_front();

				for(it = container->getItems(); it != container->getEnd(); ++it){
					if((tmpContainer = (*it)->getContainer())){
						listContainer.push_back(tmpContainer);
					}

					listItem.push_back(*it);
				}
			}

			msg->AddByte(listItem.size());
			while(!listItem.empty()) {
				const Item* item = listItem.front();
				listItem.pop_front();
				msg->AddItem(item);
			}
		}
		else {
			msg->AddByte(1);
			msg->AddItem(item);
		}
	}
}

void ProtocolGame::sendCloseTrade()
{
	NetworkMessage_ptr msg = getOutputBuffer();
	if(msg){
		TRACK_MESSAGE(msg);
		msg->AddByte(0x7F);
	}
}

void ProtocolGame::sendCloseContainer(uint32_t cid)
{
	NetworkMessage_ptr msg = getOutputBuffer();
	if(msg){
		TRACK_MESSAGE(msg);
		msg->AddByte(0x6F);
		msg->AddByte(cid);
	}
}

void ProtocolGame::sendCreatureTurn(const Creature* creature, uint32_t stackpos)
{
	if(stackpos < 10){
		if(canSee(creature)){
			NetworkMessage_ptr msg = getOutputBuffer();
			if(msg){
				TRACK_MESSAGE(msg);
				msg->AddByte(0x6B);
				msg->AddPosition(creature->getPosition());
				msg->AddByte(stackpos);
				msg->AddU16(0x63); /*99*/
				msg->AddU32(creature->getID());
				msg->AddByte(creature->getDirection());
			}
		}
	}
}

void ProtocolGame::sendCreatureSay(const Creature* creature, SpeakClasses type, const std::string& text)
{
	NetworkMessage_ptr msg = getOutputBuffer();
	if(msg){
		TRACK_MESSAGE(msg);
		AddCreatureSpeak(msg, creature, type, text, 0);
	}
}

void ProtocolGame::sendToChannel(const Creature * creature, SpeakClasses type, const std::string& text, uint16_t channelId, uint32_t time /*= 0*/)
{
	NetworkMessage_ptr msg = getOutputBuffer();
	if(msg){
		TRACK_MESSAGE(msg);
		AddCreatureSpeak(msg, creature, type, text, channelId, time);
	}
}

void ProtocolGame::sendCancel(const std::string& message)
{
	NetworkMessage_ptr msg = getOutputBuffer();
	if(msg){
		TRACK_MESSAGE(msg);
		AddTextMessage(msg, MSG_STATUS_SMALL, message);
	}
}

void ProtocolGame::sendCancelTarget()
{
	NetworkMessage_ptr msg = getOutputBuffer();
	if(msg){
		TRACK_MESSAGE(msg);
		msg->AddByte(0xA3);
		msg->AddU32(0);
	}
}

void ProtocolGame::sendChangeSpeed(const Creature* creature, uint32_t speed)
{
	if(canSee(creature)){
		NetworkMessage_ptr msg = getOutputBuffer();
		if(msg){
			TRACK_MESSAGE(msg);
			msg->AddByte(0x8F);
			msg->AddU32(creature->getID());
			msg->AddU16(speed);
		}
	}
}

void ProtocolGame::sendCancelWalk()
{
	NetworkMessage_ptr msg = getOutputBuffer();
	if(msg){
		TRACK_MESSAGE(msg);
		msg->AddByte(0xB5);
		msg->AddByte(player->getDirection());
	}
}

void ProtocolGame::sendSkills()
{
	NetworkMessage_ptr msg = getOutputBuffer();
	if(msg){
		TRACK_MESSAGE(msg);
		AddPlayerSkills(msg);
	}
}

void ProtocolGame::sendPing()
{
	NetworkMessage_ptr msg = getOutputBuffer();
	if(msg){
		TRACK_MESSAGE(msg);
		msg->AddByte(0x1E);
	}
}

void ProtocolGame::sendDistanceShoot(const Position& from, const Position& to, uint8_t type)
{
	if(canSee(from) || canSee(to)){
		NetworkMessage_ptr msg = getOutputBuffer();
		if(msg){
			TRACK_MESSAGE(msg);
			AddDistanceShoot(msg, from, to, type);
		}
	}
}

void ProtocolGame::sendMagicEffect(const Position& pos, uint8_t type)
{
	if(canSee(pos)){
		NetworkMessage_ptr msg = getOutputBuffer();
		if(msg){
			TRACK_MESSAGE(msg);
			AddMagicEffect(msg, pos, type);
		}
	}
}

void ProtocolGame::sendAnimatedText(const Position& pos, uint8_t color, std::string text)
{
	if(canSee(pos)){
		NetworkMessage_ptr msg = getOutputBuffer();
		if(msg){
			TRACK_MESSAGE(msg);
			AddAnimatedText(msg, pos, color, text);
		}
	}
}

void ProtocolGame::sendCreatureHealth(const Creature* creature)
{
	if(canSee(creature)){
		NetworkMessage_ptr msg = getOutputBuffer();
		if(msg){
			TRACK_MESSAGE(msg);
			AddCreatureHealth(msg, creature);
		}
	}
}

void ProtocolGame::sendCreatureWalkthrough(const Creature* creature, bool walkthrough)
{
	if(canSee(creature)){
		NetworkMessage_ptr msg = getOutputBuffer();
		if(msg){
			TRACK_MESSAGE(msg);
			msg->AddByte(0x92);
			msg->AddU32(creature->getID());
			msg->AddByte(!walkthrough); // isBlocking = !walkthrough
		}
	}
}

void ProtocolGame::sendQuestLog()
{
	NetworkMessage_ptr msg = getOutputBuffer();
	if(msg){
		TRACK_MESSAGE(msg);

		msg->AddByte(0xF0);
		msg->AddU16(Quests::getInstance()->getQuestsCount(player));
		for(QuestsList::const_iterator it = Quests::getInstance()->getFirstQuest();
			it != Quests::getInstance()->getEndQuest(); ++it){
			if((*it)->isStarted(player)){
				msg->AddU16((*it)->getID());
				msg->AddString((*it)->getName());
				msg->AddByte((*it)->isCompleted(player));
			}
		}
	}
}

void ProtocolGame::sendQuestLine(const Quest* quest)
{
	NetworkMessage_ptr msg = getOutputBuffer();
	if(msg){
		TRACK_MESSAGE(msg);

		msg->AddByte(0xF1);
		msg->AddU16(quest->getID());
		msg->AddByte(quest->getMissionsCount(player));
		for(MissionsList::const_iterator it = quest->getFirstMission(); it != quest->getEndMission(); ++it){
			if((*it)->isStarted(player)){
				msg->AddString((*it)->getName(player));
				msg->AddString((*it)->getDescription(player));
			}
		}
	}
}

//tile
void ProtocolGame::sendAddTileItem(const Tile* tile, const Position& pos, uint32_t stackpos, const Item* item)
{
	if(canSee(pos)){
		NetworkMessage_ptr msg = getOutputBuffer();
		if(msg){
			TRACK_MESSAGE(msg);
			AddTileItem(msg, pos, stackpos, item);
		}
	}
}

void ProtocolGame::sendUpdateTileItem(const Tile* tile, const Position& pos, uint32_t stackpos, const Item* item)
{
	if(canSee(pos)){
		NetworkMessage_ptr msg = getOutputBuffer();
		if(msg){
			TRACK_MESSAGE(msg);
			UpdateTileItem(msg, pos, stackpos, item);
		}
	}
}

void ProtocolGame::sendRemoveTileItem(const Tile* tile, const Position& pos, uint32_t stackpos)
{
	if(canSee(pos)){
		NetworkMessage_ptr msg = getOutputBuffer();
		if(msg){
			TRACK_MESSAGE(msg);
			RemoveTileItem(msg, pos, stackpos);
		}
	}
}

void ProtocolGame::sendUpdateTile(const Tile* tile, const Position& pos)
{
	if(canSee(pos)){
		NetworkMessage_ptr msg = getOutputBuffer();
		if(msg){
			TRACK_MESSAGE(msg);
			msg->AddByte(0x69);
			msg->AddPosition(pos);

			if(tile){
				GetTileDescription(tile, msg);
				msg->AddByte(0);
				msg->AddByte(0xFF);
			}
			else{
				msg->AddByte(0x01);
				msg->AddByte(0xFF);
			}
		}
	}
}

void ProtocolGame::sendAddCreature(const Creature* creature, const Position& pos, uint32_t stackpos)
{
	if(canSee(creature)){
		NetworkMessage_ptr msg = getOutputBuffer();
		if(msg){
			TRACK_MESSAGE(msg);
			if(creature == player){
				msg->AddByte(0x0A);
				msg->AddU32(player->getID());
				msg->AddU16(0x0032); // Related to client-side drawing speed
				msg->AddByte(player->hasFlag(PlayerFlag_CanReportBugs));

				/*uint16_t violation = player->getViolationLevel();
				if(violationReasons[violation] > 0){
					msg->AddByte(0x0B);
					for(int32_t i = 0; i <= violationReasons[maxViolationLevel]; i++){
						if(i <= violationReasons[1])
							msg->AddByte(violationNames[violation]);
						else if(i <= violationReasons[violation])
							msg->AddByte(violationStatements[violation]);
						else
							msg->AddByte(Action_None);
					}
				}*/

				AddMapDescription(msg, pos);

				AddInventoryItem(msg, SLOT_HEAD, player->getInventoryItem(SLOT_HEAD));
				AddInventoryItem(msg, SLOT_NECKLACE, player->getInventoryItem(SLOT_NECKLACE));
				AddInventoryItem(msg, SLOT_BACKPACK, player->getInventoryItem(SLOT_BACKPACK));
				AddInventoryItem(msg, SLOT_ARMOR, player->getInventoryItem(SLOT_ARMOR));
				AddInventoryItem(msg, SLOT_RIGHT, player->getInventoryItem(SLOT_RIGHT));
				AddInventoryItem(msg, SLOT_LEFT, player->getInventoryItem(SLOT_LEFT));
				AddInventoryItem(msg, SLOT_LEGS, player->getInventoryItem(SLOT_LEGS));
				AddInventoryItem(msg, SLOT_FEET, player->getInventoryItem(SLOT_FEET));
				AddInventoryItem(msg, SLOT_RING, player->getInventoryItem(SLOT_RING));
				AddInventoryItem(msg, SLOT_AMMO, player->getInventoryItem(SLOT_AMMO));

				AddPlayerStats(msg);
				AddPlayerSkills(msg);

				//gameworld light-settings
				LightInfo lightInfo;
				g_game.getWorldLightInfo(lightInfo);
				AddWorldLight(msg, lightInfo);

				//player light level
				AddCreatureLight(msg, creature);

				for(VIPListSet::iterator it = player->VIPList.begin(); it != player->VIPList.end(); ++it){
					std::string vip_name;
					if(IOPlayer::instance()->getNameByGuid((*it), vip_name)){
						Player *p = g_game.getPlayerByName(vip_name);
						bool online = (p && (!p->isGmInvisible() || player->canSeeGmInvisible(p)));
						sendVIP((*it), vip_name, online);
					}
				}
				sendIcons(player->getIcons());
			}
			else{
				AddTileCreature(msg, pos, stackpos, creature);
			}
		}
	}
}

void ProtocolGame::sendRemoveCreature(const Creature* creature, const Position& pos, uint32_t stackpos, bool isLogout)
{
	if(canSee(pos)){
		NetworkMessage_ptr msg = getOutputBuffer();
		if(msg){
			TRACK_MESSAGE(msg);
			RemoveTileItem(msg, pos, stackpos);

			if(isLogout){
				AddMagicEffect(msg, pos, NM_ME_PUFF);
			}
		}
	}
}

void ProtocolGame::sendMoveCreature(const Creature* creature, const Tile* newTile, const Position& newPos,
	uint32_t newStackPos, const Tile* oldTile, const Position& oldPos, uint32_t oldStackPos, bool teleport)
{
	if(creature == player){
		NetworkMessage_ptr msg = getOutputBuffer();
		if(msg){
			TRACK_MESSAGE(msg);
			if(teleport || oldStackPos >= 10){
				RemoveTileItem(msg, oldPos, oldStackPos);
				AddMapDescription(msg, newPos);
			}
			else{
				if(oldPos.z == 7 && newPos.z >= 8){
					RemoveTileItem(msg, oldPos, oldStackPos);
				}
				else{
					msg->AddByte(0x6D);
					msg->AddPosition(oldPos);
					msg->AddByte(oldStackPos);
					msg->AddPosition(newPos);
				}

				//floor change down
				if(newPos.z > oldPos.z){
					MoveDownCreature(msg, creature, newPos, oldPos, oldStackPos);
				}
				//floor change up
				else if(newPos.z < oldPos.z){
					MoveUpCreature(msg, creature, newPos, oldPos, oldStackPos);
				}

				if(oldPos.y > newPos.y){ // north, for old x
					msg->AddByte(0x65);
					GetMapDescription(oldPos.x - 8, newPos.y - 6, newPos.z, 18, 1, msg);
				}
				else if(oldPos.y < newPos.y){ // south, for old x
					msg->AddByte(0x67);
					GetMapDescription(oldPos.x - 8, newPos.y + 7, newPos.z, 18, 1, msg);
				}

				if(oldPos.x < newPos.x){ // east, [with new y]
					msg->AddByte(0x66);
					GetMapDescription(newPos.x + 9, newPos.y - 6, newPos.z, 1, 14, msg);
				}
				else if(oldPos.x > newPos.x){ // west, [with new y]
					msg->AddByte(0x68);
					GetMapDescription(newPos.x - 8, newPos.y - 6, newPos.z, 1, 14, msg);
				}
			}
		}
	}
	else if(canSee(oldPos) && canSee(newPos)){
		if(player->canSeeCreature(creature)){
			NetworkMessage_ptr msg = getOutputBuffer();
			if(msg){
				TRACK_MESSAGE(msg);
				if(teleport || (oldPos.z == 7 && newPos.z >= 8) || oldStackPos >= 10){
					RemoveTileItem(msg, oldPos, oldStackPos);
					AddTileCreature(msg, newPos, newStackPos, creature);
				}
				else{
					msg->AddByte(0x6D);
					msg->AddPosition(oldPos);
					msg->AddByte(oldStackPos);
					msg->AddPosition(newPos);
				}
			}
		}
	}
	else if(canSee(oldPos)){
		if(player->canSeeCreature(creature)){
			NetworkMessage_ptr msg = getOutputBuffer();
			if(msg){
				TRACK_MESSAGE(msg);
				RemoveTileItem(msg, oldPos, oldStackPos);
			}
		}
	}
	else if(canSee(newPos)){
		if(player->canSeeCreature(creature)){
			NetworkMessage_ptr msg = getOutputBuffer();
			if(msg){
				TRACK_MESSAGE(msg);
				AddTileCreature(msg, newPos, newStackPos, creature);
			}
		}
	}
}

//inventory
void ProtocolGame::sendAddInventoryItem(slots_t slot, const Item* item)
{
	NetworkMessage_ptr msg = getOutputBuffer();
	if(msg){
		TRACK_MESSAGE(msg);
		AddInventoryItem(msg, slot, item);
	}
}

void ProtocolGame::sendUpdateInventoryItem(slots_t slot, const Item* item)
{
	NetworkMessage_ptr msg = getOutputBuffer();
	if(msg){
		TRACK_MESSAGE(msg);
		UpdateInventoryItem(msg, slot, item);
	}
}

void ProtocolGame::sendRemoveInventoryItem(slots_t slot)
{
	NetworkMessage_ptr msg = getOutputBuffer();
	if(msg){
		TRACK_MESSAGE(msg);
		RemoveInventoryItem(msg, slot);
	}
}

//containers
void ProtocolGame::sendAddContainerItem(uint8_t cid, const Item* item)
{
	NetworkMessage_ptr msg = getOutputBuffer();
	if(msg){
		TRACK_MESSAGE(msg);
		AddContainerItem(msg, cid, item);
	}
}

void ProtocolGame::sendUpdateContainerItem(uint8_t cid, uint8_t slot, const Item* item)
{
	NetworkMessage_ptr msg = getOutputBuffer();
	if(msg){
		TRACK_MESSAGE(msg);
		UpdateContainerItem(msg, cid, slot, item);
	}
}

void ProtocolGame::sendRemoveContainerItem(uint8_t cid, uint8_t slot)
{
	NetworkMessage_ptr msg = getOutputBuffer();
	if(msg){
		TRACK_MESSAGE(msg);
		RemoveContainerItem(msg, cid, slot);
	}
}

void ProtocolGame::sendTextWindow(uint32_t windowTextId, Item* item, uint16_t maxlen, bool canWrite)
{
	NetworkMessage_ptr msg = getOutputBuffer();
	if(msg){
		TRACK_MESSAGE(msg);
		msg->AddByte(0x96);
		msg->AddU32(windowTextId);
		msg->AddItemId(item);
		if(canWrite){
			msg->AddU16(maxlen);
			msg->AddString(item->getText());
		}
		else{
			msg->AddU16(item->getText().size());
			msg->AddString(item->getText());
		}

		const std::string& writer = item->getWriter();
		if(writer.size()){
			msg->AddString(writer);
		}
		else{
			msg->AddString("");
		}

		time_t writtenDate = item->getWrittenDate();
		if(writtenDate > 0){
			char date[16];
			formatDateShort(writtenDate, date);
			msg->AddString(date);
		}
		else{
			msg->AddString("");
		}
	}
}

void ProtocolGame::sendTextWindow(uint32_t windowTextId, uint32_t itemId, const std::string& text)
{
	NetworkMessage_ptr msg = getOutputBuffer();
	if(msg){
		TRACK_MESSAGE(msg);
		msg->AddByte(0x96);
		msg->AddU32(windowTextId);
		msg->AddItemId(itemId);

		msg->AddU16(text.size());
		msg->AddString(text);

		msg->AddString("");
		msg->AddString("");
	}
}

void ProtocolGame::sendHouseWindow(uint32_t windowTextId, House* _house,
	uint32_t listId, const std::string& text)
{
	NetworkMessage_ptr msg = getOutputBuffer();
	if(msg){
		TRACK_MESSAGE(msg);
		msg->AddByte(0x97);
		msg->AddByte(0);
		msg->AddU32(windowTextId);
		msg->AddString(text);
	}
}

void ProtocolGame::sendOutfitWindow()
{
	#define MAX_NUMBER_OF_OUTFITS 25
	#define MAX_NUMBER_OF_MOUNTS 25
	//client 8.0 outfits limit is 25

	NetworkMessage_ptr msg = getOutputBuffer();
	if(msg){
		TRACK_MESSAGE(msg);
		msg->AddByte(0xC8);
		AddCreatureOutfit(msg, player, player->getDefaultOutfit());

		std::list<Outfit> outfitList;
		for(OutfitMap::iterator it = player->outfits.begin(); it != player->outfits.end(); ++it){
			if(player->canWearOutfit(it->first, it->second.addons)){
				outfitList.push_back(it->second);
			}
		}

		std::list<Mount> mountList;
		for(MountMap::iterator it = player->mounts.begin(); it != player->mounts.end(); ++it){
			if(player->canRideMount(it->first)){
				mountList.push_back(it->second);
			}
		}

		if(!outfitList.empty()){
			if(outfitList.size() > MAX_NUMBER_OF_OUTFITS){
				msg->AddByte(MAX_NUMBER_OF_OUTFITS);
			}
			else{
				msg->AddByte(outfitList.size());
			}

			uint32_t counter = 0;
			std::list<Outfit>::const_iterator it;
			for(it = outfitList.begin(); it != outfitList.end() && (counter < MAX_NUMBER_OF_OUTFITS); ++it, ++counter){
				msg->AddU16(it->lookType);
				msg->AddString(it->name);
				if(g_config.getNumber(ConfigManager::ADDONS_ONLY_FOR_PREMIUM) && !player->isPremium()){
					msg->AddByte(0);
				}
				else{
					msg->AddByte(it->addons);
				}
			}
		}
		else{
			msg->AddByte(1);
			msg->AddU16(player->getDefaultOutfit().lookType);
			msg->AddString("");
			msg->AddByte(player->getDefaultOutfit().lookAddons);
		}

		if(!mountList.empty() && g_config.getBoolean(ConfigManager::ENABLE_MOUNTS)){
			if(mountList.size()+1 > MAX_NUMBER_OF_MOUNTS){
				msg->AddByte(MAX_NUMBER_OF_MOUNTS);
			}
			else{
				msg->AddByte(mountList.size()+1);
			}

			uint32_t counter = 0;
			std::list<Mount>::const_iterator it;
			for(it = mountList.begin(); it != mountList.end() && (counter < MAX_NUMBER_OF_MOUNTS-1); ++it, ++counter){
				msg->AddU16(it->lookType);
				msg->AddString(it->name);
			}
			msg->AddU16(0x00);
			msg->AddString("No Mount");
		}
		else{
			msg->AddByte(1);
			msg->AddU16(0x00);
			msg->AddString("No Mount");
		}
	}
}

void ProtocolGame::sendSpellCooldown(uint16_t spellId, uint32_t cooldown, bool isGroup)
{
	if(!spellId || !cooldown)
		return;

	NetworkMessage_ptr msg = getOutputBuffer();
	if(msg){
		TRACK_MESSAGE(msg);
		msg->AddByte(isGroup ? 0xA5 : 0xA4);
		msg->AddU16(spellId);
		msg->AddU16((uint16_t)cooldown);
		msg->AddByte(0x00);
	}
}

void ProtocolGame::sendVIPLogIn(uint32_t guid)
{
	NetworkMessage_ptr msg = getOutputBuffer();
	if(msg){
		TRACK_MESSAGE(msg);
		msg->AddByte(0xD3);
		msg->AddU32(guid);
	}
}

void ProtocolGame::sendVIPLogOut(uint32_t guid)
{
	NetworkMessage_ptr msg = getOutputBuffer();
	if(msg){
		TRACK_MESSAGE(msg);
		msg->AddByte(0xD4);
		msg->AddU32(guid);
	}
}

void ProtocolGame::sendVIP(uint32_t guid, const std::string& name, bool isOnline)
{
	NetworkMessage_ptr msg = getOutputBuffer();
	if(msg){
		TRACK_MESSAGE(msg);
		msg->AddByte(0xD2);
		msg->AddU32(guid);
		msg->AddString(name);
		msg->AddByte(isOnline ? 1 : 0);
	}
}

void ProtocolGame::sendTutorial(uint8_t tutorialId)
{
	NetworkMessage_ptr msg = getOutputBuffer();
	if(msg){
		TRACK_MESSAGE(msg);
		msg->AddByte(0xDC);
		msg->AddByte(tutorialId);
	}
}

void ProtocolGame::sendAddMarker(const Position& pos, uint8_t markType, const std::string& desc)
{
	NetworkMessage_ptr msg = getOutputBuffer();
	if(msg){
		TRACK_MESSAGE(msg);
		msg->AddByte(0xDD);
		msg->AddPosition(pos);
		msg->AddByte(markType);
		msg->AddString(desc);
	}
}

void ProtocolGame::sendReLoginWindow()
{
	NetworkMessage_ptr msg = getOutputBuffer();
	if(msg){
		TRACK_MESSAGE(msg);
		msg->AddByte(0x28);
		msg->AddByte(0x64);
	}
}

////////////// Add common messages
void ProtocolGame::AddMapDescription(NetworkMessage_ptr msg, const Position& pos)
{
	msg->AddByte(0x64);
	msg->AddPosition(player->getPosition());
	GetMapDescription(pos.x - 8, pos.y - 6, pos.z, 18, 14, msg);
}

void ProtocolGame::AddTextMessage(NetworkMessage_ptr msg, MessageClasses mclass, const std::string& message)
{
	msg->AddByte(0xB4);
	msg->AddByte(mclass);
	msg->AddString(message);
}

void ProtocolGame::AddAnimatedText(NetworkMessage_ptr msg, const Position& pos,
	uint8_t color, const std::string& text)
{
	msg->AddByte(0x84);
	msg->AddPosition(pos);
	msg->AddByte(color);
	msg->AddString(text);
}

void ProtocolGame::AddMagicEffect(NetworkMessage_ptr msg,const Position& pos, uint8_t type)
{
	msg->AddByte(0x83);
	msg->AddPosition(pos);
	msg->AddByte(type + 1);
}

void ProtocolGame::AddDistanceShoot(NetworkMessage_ptr msg, const Position& from, const Position& to,
	uint8_t type)
{
	msg->AddByte(0x85);
	msg->AddPosition(from);
	msg->AddPosition(to);
	msg->AddByte(type + 1);
}

void ProtocolGame::AddCreature(NetworkMessage_ptr msg,const Creature* creature, bool known, uint32_t remove)
{
	if(known){
		msg->AddU16(0x62);
		msg->AddU32(creature->getID());
	}
	else{
		msg->AddU16(0x61);
		msg->AddU32(remove);
		msg->AddU32(creature->getID());
		msg->AddString(creature->getName());
	}

	int32_t healthToSend;
	if(!creature->hasHiddenHealth() || (creature == player))
		healthToSend = (int32_t)std::ceil(((float)creature->getHealth()) * 100 / std::max(creature->getMaxHealth(), (int32_t)1));
	else
		healthToSend = 0;
	msg->AddByte(healthToSend);

	msg->AddByte((uint8_t)creature->getDirection());
	if(creature->isInvisible() ||
		(creature->getPlayer() && creature->getPlayer()->isGmInvisible()))
	{
		static Outfit_t outfit;
		AddCreatureOutfit(msg, creature, outfit);
	}
	else{
		AddCreatureOutfit(msg, creature, creature->getCurrentOutfit());
	}

	LightInfo lightInfo;
	creature->getCreatureLight(lightInfo);
	msg->AddByte(lightInfo.level);
	msg->AddByte(lightInfo.color);

	msg->AddU16(creature->getStepSpeed());
#ifdef __SKULLSYSTEM__
	msg->AddByte(player->getSkullClient(creature->getPlayer()));
#else
	msg->AddByte(SKULL_NONE);
#endif
	msg->AddByte(player->getPartyShield(creature->getPlayer()));
	if(!known){
		msg->AddByte(player->getGuildEmblem(creature->getPlayer())); // guild war emblem
	}

	msg->AddByte(!player->canWalkthrough(creature));
}

void ProtocolGame::AddPlayerStats(NetworkMessage_ptr msg)
{
	msg->AddByte(0xA0);
	msg->AddU16(player->getHealth());
	msg->AddU16(player->getPlayerInfo(PLAYERINFO_MAXHEALTH));
	msg->AddU32((uint32_t)(player->getFreeCapacity() * 100));
	msg->AddU64(player->getExperience());
	msg->AddU16(player->getPlayerInfo(PLAYERINFO_LEVEL));
	msg->AddByte(player->getPlayerInfo(PLAYERINFO_LEVELPERCENT));
	msg->AddU16(player->getMana());
	msg->AddU16(player->getPlayerInfo(PLAYERINFO_MAXMANA));
	msg->AddByte(player->getMagicLevel());
	msg->AddByte(player->getPlayerInfo(PLAYERINFO_MAGICLEVELPERCENT));
	msg->AddByte(player->getPlayerInfo(PLAYERINFO_SOUL));
	msg->AddU16(player->getStaminaMinutes()); //stamina(minutes)
}

void ProtocolGame::AddPlayerSkills(NetworkMessage_ptr msg)
{
	msg->AddByte(0xA1);
	msg->AddByte(player->getSkill(SKILL_FIST,   SKILL_LEVEL));
	msg->AddByte(player->getSkill(SKILL_FIST,   SKILL_PERCENT));
	msg->AddByte(player->getSkill(SKILL_CLUB,   SKILL_LEVEL));
	msg->AddByte(player->getSkill(SKILL_CLUB,   SKILL_PERCENT));
	msg->AddByte(player->getSkill(SKILL_SWORD,  SKILL_LEVEL));
	msg->AddByte(player->getSkill(SKILL_SWORD,  SKILL_PERCENT));
	msg->AddByte(player->getSkill(SKILL_AXE,    SKILL_LEVEL));
	msg->AddByte(player->getSkill(SKILL_AXE,    SKILL_PERCENT));
	msg->AddByte(player->getSkill(SKILL_DIST,   SKILL_LEVEL));
	msg->AddByte(player->getSkill(SKILL_DIST,   SKILL_PERCENT));
	msg->AddByte(player->getSkill(SKILL_SHIELD, SKILL_LEVEL));
	msg->AddByte(player->getSkill(SKILL_SHIELD, SKILL_PERCENT));
	msg->AddByte(player->getSkill(SKILL_FISH,   SKILL_LEVEL));
	msg->AddByte(player->getSkill(SKILL_FISH,   SKILL_PERCENT));
}

void ProtocolGame::AddCreatureSpeak(NetworkMessage_ptr msg, const Creature* creature,
	SpeakClasses type, std::string text, uint16_t channelId, uint32_t time /*= 0*/)
{
	msg->AddByte(0xAA);

	if(creature){
		if(const Player* speaker = creature->getPlayer()){
			msg->AddU32(++Player::channelStatementGuid);
			Player::channelStatementMap[Player::channelStatementGuid] = text;

			if(type == SPEAK_RVR_ANSWER){
				msg->AddString("Gamemaster");
				msg->AddU16(0x0000);
			}
			else{
				if(type == SPEAK_CHANNEL_R2){
					msg->AddString("");
				}
				else{
					msg->AddString(speaker->getName());
				}

				msg->AddU16(speaker->getPlayerInfo(PLAYERINFO_LEVEL));
			}
		}
		else{
			msg->AddU32(0x00000000);
			msg->AddString(creature->getName());
			msg->AddU16(0x0000);
		}
	}
	else{
		msg->AddU32(0x00000000);
		msg->AddString("");
		msg->AddU16(0x0000);
	}

	msg->AddByte(type);
	switch(type){
		case SPEAK_SAY:
		case SPEAK_WHISPER:
		case SPEAK_YELL:
		case SPEAK_MONSTER_SAY:
		case SPEAK_MONSTER_YELL:
		case SPEAK_PRIVATE_NP:
			assert(creature);
			msg->AddPosition(creature->getPosition());
			break;
		case SPEAK_CHANNEL_Y:
		case SPEAK_CHANNEL_W:
		case SPEAK_CHANNEL_R1:
		case SPEAK_CHANNEL_R2:
		case SPEAK_CHANNEL_O:
			msg->AddU16(channelId);
			break;
		case SPEAK_RVR_CHANNEL: {
			uint32_t t = (OTSYS_TIME() / 1000) & 0xFFFFFFFF;
			msg->AddU32(t - time);
		} break;
		default:
			break;
	}

	msg->AddString(text);
}

void ProtocolGame::AddCreatureHealth(NetworkMessage_ptr msg,const Creature* creature)
{
	msg->AddByte(0x8C);
	msg->AddU32(creature->getID());
	msg->AddByte((int32_t)std::ceil(((float)creature->getHealth()) * 100 / std::max(creature->getMaxHealth(), (int32_t)1)));
}

void ProtocolGame::AddCreatureOutfit(NetworkMessage_ptr msg, const Creature* creature, const Outfit_t& outfit)
{
	msg->AddU16(outfit.lookType);
	if(outfit.lookType != 0){
		msg->AddByte(outfit.lookHead);
		msg->AddByte(outfit.lookBody);
		msg->AddByte(outfit.lookLegs);
		msg->AddByte(outfit.lookFeet);
		msg->AddByte(outfit.lookAddons);
	}
	else if(outfit.lookTypeEx != 0){
		msg->AddItemId(outfit.lookTypeEx);
	}
	else{
		msg->AddU16(outfit.lookTypeEx);
	}
	msg->AddU16(creature->isRidingMount() ? outfit.lookMount : 0x00);
}

void ProtocolGame::AddWorldLight(NetworkMessage_ptr msg, const LightInfo& lightInfo)
{
	msg->AddByte(0x82);
	msg->AddByte(lightInfo.level);
	msg->AddByte(lightInfo.color);
}

void ProtocolGame::AddCreatureLight(NetworkMessage_ptr msg, const Creature* creature)
{
	LightInfo lightInfo;
	creature->getCreatureLight(lightInfo);
	msg->AddByte(0x8D);
	msg->AddU32(creature->getID());
	msg->AddByte(lightInfo.level);
	msg->AddByte(lightInfo.color);
}

//tile
void ProtocolGame::AddTileItem(NetworkMessage_ptr msg, const Position& pos, uint32_t stackpos, const Item* item)
{
	if(stackpos < 10){
		msg->AddByte(0x6A);
		msg->AddPosition(pos);
		msg->AddByte(stackpos);
		msg->AddItem(item);
	}
}

void ProtocolGame::AddTileCreature(NetworkMessage_ptr msg, const Position& pos, uint32_t stackpos,
	const Creature* creature)
{
	if(stackpos < 10){
		msg->AddByte(0x6A);
		msg->AddPosition(pos);
		msg->AddByte(stackpos);

		bool known;
		uint32_t removedKnown;
		checkCreatureAsKnown(creature->getID(), known, removedKnown);
		AddCreature(msg, creature, known, removedKnown);
	}
}

void ProtocolGame::UpdateTileItem(NetworkMessage_ptr msg, const Position& pos, uint32_t stackpos, const Item* item)
{
	if(stackpos < 10){
		msg->AddByte(0x6B);
		msg->AddPosition(pos);
		msg->AddByte(stackpos);
		msg->AddItem(item);
	}
}

void ProtocolGame::RemoveTileItem(NetworkMessage_ptr msg, const Position& pos, uint32_t stackpos)
{
	if(stackpos < 10){
		msg->AddByte(0x6C);
		msg->AddPosition(pos);
		msg->AddByte(stackpos);
	}
}

void ProtocolGame::MoveUpCreature(NetworkMessage_ptr msg, const Creature* creature,
	const Position& newPos, const Position& oldPos, uint32_t oldStackPos)
{
	if(creature == player){
		//floor change up
		msg->AddByte(0xBE);

		//going to surface
		if(newPos.z == 7){
			int32_t skip = -1;
			GetFloorDescription(msg, oldPos.x - 8, oldPos.y - 6, 5, 18, 14, 3, skip); //(floor 7 and 6 already set)
			GetFloorDescription(msg, oldPos.x - 8, oldPos.y - 6, 4, 18, 14, 4, skip);
			GetFloorDescription(msg, oldPos.x - 8, oldPos.y - 6, 3, 18, 14, 5, skip);
			GetFloorDescription(msg, oldPos.x - 8, oldPos.y - 6, 2, 18, 14, 6, skip);
			GetFloorDescription(msg, oldPos.x - 8, oldPos.y - 6, 1, 18, 14, 7, skip);
			GetFloorDescription(msg, oldPos.x - 8, oldPos.y - 6, 0, 18, 14, 8, skip);

			if(skip >= 0){
				msg->AddByte(skip);
				msg->AddByte(0xFF);
			}
		}
		//underground, going one floor up (still underground)
		else if(newPos.z > 7){
			int32_t skip = -1;
			GetFloorDescription(msg, oldPos.x - 8, oldPos.y - 6, oldPos.z - 3, 18, 14, 3, skip);

			if(skip >= 0){
				msg->AddByte(skip);
				msg->AddByte(0xFF);
			}
		}

		//moving up a floor up makes us out of sync
		//west
		msg->AddByte(0x68);
		GetMapDescription(oldPos.x - 8, oldPos.y + 1 - 6, newPos.z, 1, 14, msg);

		//north
		msg->AddByte(0x65);
		GetMapDescription(oldPos.x - 8, oldPos.y - 6, newPos.z, 18, 1, msg);
	}
}

void ProtocolGame::MoveDownCreature(NetworkMessage_ptr msg, const Creature* creature,
	const Position& newPos, const Position& oldPos, uint32_t oldStackPos)
{
	if(creature == player){
		//floor change down
		msg->AddByte(0xBF);

		//going from surface to underground
		if(newPos.z == 8){
			int32_t skip = -1;
			GetFloorDescription(msg, oldPos.x - 8, oldPos.y - 6, newPos.z, 18, 14, -1, skip);
			GetFloorDescription(msg, oldPos.x - 8, oldPos.y - 6, newPos.z + 1, 18, 14, -2, skip);
			GetFloorDescription(msg, oldPos.x - 8, oldPos.y - 6, newPos.z + 2, 18, 14, -3, skip);

			if(skip >= 0){
				msg->AddByte(skip);
				msg->AddByte(0xFF);
			}
		}
		//going further down
		else if(newPos.z > oldPos.z && newPos.z > 8 && newPos.z < 14){
			int32_t skip = -1;
			GetFloorDescription(msg, oldPos.x - 8, oldPos.y - 6, newPos.z + 2, 18, 14, -3, skip);

			if(skip >= 0){
				msg->AddByte(skip);
				msg->AddByte(0xFF);
			}
		}

		//moving down a floor makes us out of sync
		//east
		msg->AddByte(0x66);
		GetMapDescription(oldPos.x + 9, oldPos.y - 1 - 6, newPos.z, 1, 14, msg);

		//south
		msg->AddByte(0x67);
		GetMapDescription(oldPos.x - 8, oldPos.y + 7, newPos.z, 18, 1, msg);
	}
}

//inventory
void ProtocolGame::AddInventoryItem(NetworkMessage_ptr msg, slots_t slot, const Item* item)
{
	if(item == NULL){
		msg->AddByte(0x79);
		msg->AddByte(slot);
	}
	else{
		msg->AddByte(0x78);
		msg->AddByte(slot);
		msg->AddItem(item);
	}
}

void ProtocolGame::UpdateInventoryItem(NetworkMessage_ptr msg, slots_t slot, const Item* item)
{
	if(item == NULL){
		msg->AddByte(0x79);
		msg->AddByte(slot);
	}
	else{
		msg->AddByte(0x78);
		msg->AddByte(slot);
		msg->AddItem(item);
	}
}

void ProtocolGame::RemoveInventoryItem(NetworkMessage_ptr msg, slots_t slot)
{
	msg->AddByte(0x79);
	msg->AddByte(slot);
}

//containers
void ProtocolGame::AddContainerItem(NetworkMessage_ptr msg, uint8_t cid, const Item* item)
{
	msg->AddByte(0x70);
	msg->AddByte(cid);
	msg->AddItem(item);
}

void ProtocolGame::UpdateContainerItem(NetworkMessage_ptr msg, uint8_t cid, uint8_t slot, const Item* item)
{
	msg->AddByte(0x71);
	msg->AddByte(cid);
	msg->AddByte(slot);
	msg->AddItem(item);
}

void ProtocolGame::RemoveContainerItem(NetworkMessage_ptr msg, uint8_t cid, uint8_t slot)
{
	msg->AddByte(0x72);
	msg->AddByte(cid);
	msg->AddByte(slot);
}

// shop
void ProtocolGame::AddShopItem(NetworkMessage_ptr msg, const ShopInfo item)
{
	const ItemType& it = Item::items[item.itemId];
	msg->AddU16(it.clientId);

	if(it.stackable || it.charges != 0){
		msg->AddByte(item.subType);
	}
	else if(it.isSplash() || it.isFluidContainer()){
		uint32_t fluidIndex = item.subType % 8;
		msg->AddByte(fluidMap[fluidIndex]);
	}
	else{
		msg->AddByte(1);
	}

	msg->AddString(it.name);
	msg->AddU32((uint32_t)(it.weight*100));
	msg->AddU32(item.buyPrice);
	msg->AddU32(item.sellPrice);
}

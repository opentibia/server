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
#include "otpch.h"

#include <iomanip>
#include "protocollogin.h"
#include "outputmessage.h"
#include "ioaccount.h"
#include "ban.h"
#include "game.h"
#include "configmanager.h"
#include "connection.h"

extern ConfigManager g_config;
extern BanManager g_bans;
extern Game g_game;

#ifdef __ENABLE_SERVER_DIAGNOSTIC__
uint32_t ProtocolLogin::protocolLoginCount = 0;
#endif

#ifdef __DEBUG_NET_DETAIL__
void ProtocolLogin::deleteProtocolTask()
{
	std::cout << "Deleting ProtocolLogin" << std::endl;
	Protocol::deleteProtocolTask();
}
#endif

void ProtocolLogin::disconnectClient(uint8_t error, const char* message)
{
	OutputMessage_ptr output = OutputMessagePool::getInstance()->getOutputMessage(this, false);
	if(output){
		TRACK_MESSAGE(output);
		output->AddByte(error);
		output->AddString(message);
		OutputMessagePool::getInstance()->send(output);
	}
	getConnection()->closeConnection();
}

bool ProtocolLogin::parseFirstPacket(NetworkMessage& msg)
{
	if(g_game.getGameState() == GAME_STATE_SHUTDOWN){
		getConnection()->closeConnection();
		return false;
	}

	uint32_t clientip = getConnection()->getIP();

	/*uint16_t clientos =*/ msg.GetU16();
	uint16_t version  = msg.GetU16();
	msg.SkipBytes(12);

	if(version <= 760){
		disconnectClient(0x0A, "This server requires client version " CLIENT_VERSION_STRING ".");
	}

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

	std::string accname = msg.GetString();
	std::string password = msg.GetString();

	if(!accname.length()){
		//Tibia sends this message if the account name length is < 5
		//We will send it only if account name is BLANK
		disconnectClient(0x0A, "Invalid Account Name.");
		return false;
	}

	if(version < CLIENT_VERSION_MIN || version > CLIENT_VERSION_MAX){
		disconnectClient(0x0A, "This server requires client version " CLIENT_VERSION_STRING ".");
		return false;
	}

	if(g_game.getGameState() == GAME_STATE_STARTUP){
		disconnectClient(0x0A, "Gameworld is starting up. Please wait.");
		return false;
	}

	if(g_bans.isIpDisabled(clientip)){
		disconnectClient(0x0A, "Too many connections attempts from this IP. Try again later.");
		return false;
	}

	if(g_bans.isIpBanished(clientip)){
		disconnectClient(0x0A, "Your IP is banished!");
		return false;
	}
	/*
	uint32_t serverip = serverIPs[0].first;
	for(uint32_t i = 0; i < serverIPs.size(); i++){
		if((serverIPs[i].first & serverIPs[i].second) == (clientip & serverIPs[i].second)){
			serverip = serverIPs[i].first;
			break;
		}
	}
	*/

	Account account = IOAccount::instance()->loadAccount(accname);
	if(!(asLowerCaseString(account.name) == asLowerCaseString(accname) &&
			passwordTest(password, account.password))){

		g_bans.addLoginAttempt(clientip, false);
		disconnectClient(0x0A, "Account name or password is not correct.");
		return false;
	}

	g_bans.addLoginAttempt(clientip, true);


	OutputMessage_ptr output = OutputMessagePool::getInstance()->getOutputMessage(this, false);
	if(output){
		TRACK_MESSAGE(output);
		//Add MOTD
		std::stringstream motd;
		output->AddByte(0x14);
		motd << g_config.getNumber(ConfigManager::MOTD_NUM) << "\n";
		motd << g_config.getString(ConfigManager::MOTD);
		output->AddString(motd.str());
		//Add char list
		output->AddByte(0x64);
		output->AddByte((uint8_t)account.charList.size());
		std::list<AccountCharacter>::iterator it;
		for(it = account.charList.begin(); it != account.charList.end(); it++){
			const AccountCharacter& character = *it;
			output->AddString(character.name);
			output->AddString(character.world_name);
			output->AddU32(character.ip);
			output->AddU16(character.port);
		}
		
		output->AddU16(IOAccount::getPremiumDaysLeft(account.premiumEnd));

		OutputMessagePool::getInstance()->send(output);
	}
	getConnection()->closeConnection();

	return true;
}

void ProtocolLogin::onRecvFirstMessage(NetworkMessage& msg)
{
	parseFirstPacket(msg);
}

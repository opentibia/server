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

#include "protocollogin.h"
#include "outputmessage.h"

#include "rsa.h"
#include "configmanager.h"
#include "tools.h"
#include "ioaccount.h"
#include "ban.h"
#include <iomanip>

#define CLIENT_VERSION_MIN 792
#define CLIENT_VERSION_MAX 792

extern RSA* g_otservRSA;
extern ConfigManager g_config;
extern IPList serverIPs;
extern Ban g_bans;

void ProtocolLogin::deleteProtocolTask()
{
	std::cout << "Deleting ProtocolLogin" << std::endl;
	delete this;
}

void ProtocolLogin::parsePacket(NetworkMessage& msg)
{
	/*uint16_t clientos =*/ msg.GetU16();
	uint16_t version  = msg.GetU16();
	msg.SkipBytes(12);
	
	OutputMessage* output = OutputMessagePool::getInstance()->getOutputMessage(this);
	if(version <= 760){
		output->AddByte(0x0A);
		output->AddString("Only clients with protocol 7.92 allowed!");
	}
	else if(RSA_decrypt(g_otservRSA, msg)){
		m_encryptionEnabled = true;
		m_key[0] = msg.GetU32();
		m_key[1] = msg.GetU32();
		m_key[2] = msg.GetU32();
		m_key[3] = msg.GetU32();

		/*std::cout.flags(std::ios::hex);
		std::cout << std::setw(2) << std::setfill('0') << m_key[0] << " " <<
		std::setw(2) << std::setfill('0') << m_key[1] << " " <<
		std::setw(2) << std::setfill('0') << m_key[2] << " " <<
		std::setw(2) << std::setfill('0') << m_key[3] << std::endl;
		std::cout.flags(std::ios::dec);*/
		
		uint32_t accnumber = msg.GetU32();
		std::string password = msg.GetString();
		
		if(version >= CLIENT_VERSION_MIN && version <= CLIENT_VERSION_MAX){

			uint32_t serverip = serverIPs[0].first;
			uint32_t clientip = m_connection->getIP();
			char buffer[32];
			formatIP(clientip, buffer);
			std::cout << "Connection from " << buffer << std::endl;

			for(uint32_t i = 0; i < serverIPs.size(); i++){
				if((serverIPs[i].first & serverIPs[i].second) == (clientip & serverIPs[i].second)){
					serverip = serverIPs[i].first;
					break;
				}
			}

			if(g_bans.isIpDisabled(clientip)){
				output->AddByte(0x0A);
				output->AddString("To many connections attempts from this IP. Try again later.");
			}
			else if(g_bans.isIpBanished(clientip)){
				output->AddByte(0x0A);
				output->AddString("Your IP is banished!");
			}
			else{
				Account account = IOAccount::instance()->loadAccount(accnumber);

				bool isSuccess = accnumber != 0 && account.accnumber == accnumber &&
					passwordTest(password, account.password);
				
				g_bans.addLoginAttempt(clientip, isSuccess);

				if(isSuccess){
					// seems to be a successful load
					output->AddByte(0x14);
					std::stringstream motd;
					motd << g_config.getNumber(ConfigManager::MOTD_NUM);
					motd << "\n";
					motd << g_config.getString(ConfigManager::MOTD);
					output->AddString(motd.str());

					output->AddByte(0x64);
					output->AddByte((uint8_t)account.charList.size());

					std::list<std::string>::iterator it;
					for(it = account.charList.begin(); it != account.charList.end(); it++){
						output->AddString((*it));
						output->AddString(g_config.getString(ConfigManager::WORLD_NAME));
						output->AddU32(serverip);
						output->AddU16(g_config.getNumber(ConfigManager::PORT));
					}

					output->AddU16(account.premDays);
				}
				else{
					output->AddByte(0x0A);
					output->AddString("Please enter a valid account number and password.");
				}
			}
		}
		else{
			output->AddByte(0x0A);
			output->AddString("Only clients with protocol 7.92 allowed!");
		}
		OutputMessagePool::getInstance()->send(output);
	}
	m_connection->closeConnection();
}

//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// otserv main. The only place where things get instantiated.
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


#include "definitions.h"

#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <ctime>

#include "otsystem.h"
#include "networkmessage.h"
#include "protocol74.h"

#include <stdlib.h>
#include <time.h>
#include "game.h"

#include "ioaccount.h"
#include "ioplayer.h"

#include "status.h"
#include "spells.h"

#include "actions.h"
#include "commands.h"

#include "luascript.h"
#include "account.h"

#include "tools.h"
#include "md5.h"

#ifdef WIN32
	#define ERROR_EINTR WSAEINTR
#else
	#include <fcntl.h>
	#include <arpa/inet.h>
	#include <signal.h>
	#include <errno.h>
	
	#define SOCKET_ERROR -1
	#define ERROR_EINTR EINTR
	
	extern int errno; 
#endif

#ifdef __DEBUG_CRITICALSECTION__
OTSYS_THREAD_LOCK_CLASS::LogList OTSYS_THREAD_LOCK_CLASS::loglist;
#endif

std::vector< std::pair<unsigned long, unsigned long> > serverIPs;
std::vector< std::pair<unsigned long, unsigned long> > bannedIPs;

LuaScript g_config;

Items Item::items;
Game g_game;
Spells spells(&g_game);
Actions actions(&g_game);
Commands commands(&g_game);

#if defined __EXCEPTION_TRACER__
#include "exception.h"
#endif
#include "networkmessage.h"

enum passwordType_t{
	PASSWORD_TYPE_PLAIN,
	PASSWORD_TYPE_MD5,
};

passwordType_t passwordType;

bool passwordTest(std::string &plain, std::string &hash)
{
	if(passwordType == PASSWORD_TYPE_MD5){
		MD5_CTX m_md5;
		std::stringstream hexStream;
		std::string plainHash;
	
		
	
		MD5Init(&m_md5, 0);
		MD5Update(&m_md5, (const unsigned char*)plain.c_str(), plain.length());
		MD5Final(&m_md5);
	
		hexStream.flags(std::ios::hex);
		for(int i=0;i<16;i++){
			hexStream << std::setw(2) << std::setfill('0') << (unsigned long)m_md5.digest[i];
		}
	
		plainHash = hexStream.str();
		std::transform(plainHash.begin(), plainHash.end(), plainHash.begin(), upchar);
		std::transform(hash.begin(), hash.end(), hash.begin(), upchar);
		if(plainHash == hash){
			return true;
		}
		else{
			return false;
		}
	}
	else{
		if(plain == hash){
			return true;
		}
		else{
			return false;
		}
		
	}
	
}
				

bool isclientBanished(SOCKET s)
{
	sockaddr_in sain;
  socklen_t salen = sizeof(sockaddr_in);

	if (getpeername(s, (sockaddr*)&sain, &salen) == 0)
	{
		unsigned long clientip = *(unsigned long*)&sain.sin_addr;

		for (size_t i = 0; i < bannedIPs.size(); ++i) {
      if ((bannedIPs[i].first & bannedIPs[i].second) == (clientip & bannedIPs[i].second))
				return true;
		}
	}

	return false;
}

OTSYS_THREAD_RETURN ConnectionHandler(void *dat)
{
#if defined __EXCEPTION_TRACER__
	ExceptionHandler playerExceptionHandler;	
	playerExceptionHandler.InstallHandler();
#endif
	
	srand((unsigned)time(NULL));
	
	SOCKET s = *(SOCKET*)dat;
    
	NetworkMessage msg;
	if (msg.ReadFromSocket(s))
	{
		unsigned short protId = msg.GetU16();
		
		// login server connection
		if (protId == 0x0201)
		{
			msg.SkipBytes(15);
			unsigned int accnumber = msg.GetU32();
			std::string  password  = msg.GetString();
			
			int serverip = serverIPs[0].first;
			
			sockaddr_in sain;
			socklen_t salen = sizeof(sockaddr_in);
			if (getpeername(s, (sockaddr*)&sain, &salen) == 0)
			{
				unsigned long clientip = *(unsigned long*)&sain.sin_addr;
				for (unsigned int i = 0; i < serverIPs.size(); i++)
					if ((serverIPs[i].first & serverIPs[i].second) == (clientip & serverIPs[i].second))
					{
						serverip = serverIPs[i].first;
						break;
					}
			}
			
			msg.Reset();
			
			if(isclientBanished(s)) {
				msg.AddByte(0x0A);
				msg.AddString("Your IP is banished!");
			}
			else {
				char accstring[16];
				sprintf(accstring, "%i", accnumber);
				
				Account account = IOAccount::instance()->loadAccount(accnumber);
				//if (account.accnumber == accnumber && account.password == password) // seems to be a successful load
				if (account.accnumber == accnumber && passwordTest(password,account.password)) // seems to be a successful load
				{
					msg.AddByte(0x14);
					std::stringstream motd;
					motd << g_config.getGlobalString("motdnum");
					motd << "\n";
					motd << g_config.getGlobalString("motd");
					msg.AddString(motd.str());
					
					msg.AddByte(0x64);
					msg.AddByte((uint8_t)account.charList.size());
					
					std::list<std::string>::iterator it;
					for (it = account.charList.begin(); it != account.charList.end(); it++)
					{
						msg.AddString((*it));
						msg.AddString("OpenTibia");
						msg.AddU32(serverip);
						msg.AddU16(atoi(g_config.getGlobalString("port").c_str()));
					}
					
					msg.AddU16(account.premDays);
				}
				else
				{
					msg.AddByte(0x0A);
					msg.AddString("Please enter a valid account number and password.");
				}
			}
			
			msg.WriteToSocket(s);
		}
		// gameworld connection tibia 7.4
		else if (protId == 0x020A)
		{
			unsigned char  clientos = msg.GetByte();
			unsigned short version  = msg.GetU16();
			unsigned char  unknown = msg.GetByte();
			msg.GetU32();
			std::string name     = msg.GetString();
			std::string password = msg.GetString();
			if(version != 740 && version != 741){
				msg.Reset();
				msg.AddByte(0x14);
				msg.AddString("Only clients with protocol 7.4 allowed!");
				msg.WriteToSocket(s);
			}
			else if(isclientBanished(s)){
				msg.Reset();
				msg.AddByte(0x14);
				msg.AddString("Your IP is banished!");
				msg.WriteToSocket(s);
			}
			else{
				Protocol74 *protocol;
				Player *player;
				bool playerexist;
				Creature *creature;
				
				OTSYS_THREAD_LOCK(g_game.gameLock, "ConnectionHandler()")
				creature = g_game.getCreatureByName(name);
				if(creature && dynamic_cast<Player*>(creature)){
					player = dynamic_cast<Player*>(creature);
					if(player->client->s == 0 && passwordTest(password,player->password) && player->isRemoved == false && !g_config.getGlobalNumber("allowclones", 0)){
						//std::cout << "relogin " << player << std::endl;
						player->lastlogin = std::time(NULL);
						player->client->reinitializeProtocol();
						player->client->s = s;
						player->client->sendThingAppear(player);
						player->lastip = player->getIP();
						s = 0;
					}
					player = NULL;
				}
				OTSYS_THREAD_UNLOCK(g_game.gameLock, "ConnectionHandler()")
				if(s){
					playerexist = (creature != NULL);
					protocol = new Protocol74(s);
					player = new Player(name, protocol);
					player->useThing();
					player->setID();
					IOPlayer::instance()->loadPlayer(player, name);
					//std::cout << "login " << player << std::endl;
					if(passwordTest(password,player->password)){
						if(playerexist && !g_config.getGlobalNumber("allowclones", 0)){
							std::cout << "reject player..." << std::endl;
							msg.Reset();
							msg.AddByte(0x14);
							msg.AddString("You are already logged in.");
							msg.WriteToSocket(s);		
						}
						else if(g_game.getGameState() == GAME_STATE_SHUTDOWN){
							//nothing to do
						}
						else if(g_game.getGameState() == GAME_STATE_CLOSED && player->access == 0){
							msg.Reset();
							msg.AddByte(0x14);
							msg.AddString("Server temporarly closed.");
							msg.WriteToSocket(s);
						}
						else if(!protocol->ConnectPlayer()){
							std::cout << "reject player..." << std::endl;
							msg.Reset();
							msg.AddByte(0x16);
							msg.AddString("Too many Players online.");
							msg.AddByte(45);
							msg.WriteToSocket(s);
						} 
						else{
							Status* stat = Status::instance();
							stat->addPlayer();
							player->lastlogin = std::time(NULL);
							player->lastip = player->getIP();
							s = 0;            // protocol/player will close socket
							protocol->ReceiveLoop();
							stat->removePlayer();
						}
					}
					//free memory
					if(player)
						g_game.FreeThing(player);
					//player->releaseThing();
				}
			}
		} 
		// Since Cip made 02xx as Tibia protocol,
		// Lets make FFxx as "our great info protocol" ;P
		else if (protId == 0xFFFF) {
			if (msg.GetRaw() == "info"){
				Status* status = Status::instance();
				
				uint64_t running = (OTSYS_TIME() - status->start)/1000;
				std::cout << ":: Uptime: " << running << std::endl;
				
				std::string str = status->getStatusString();
				send(s, str.c_str(), (int)str.size(), 0); 
			}
		}
		// Another ServerInfo protocol
		// Starting from 01, so the above could be 00 ;)
		else if (protId == 0xFF01) { 
			// This one doesn't need to read nothing, so we could save time and bandwidth
			// Can be called thgough a program that understand the NetMsg protocol
			Status* status = Status::instance();
			status->getInfo(msg);
			msg.WriteToSocket(s);
		}
  }
  if(s)
	  closesocket(s);
#if defined __EXCEPTION_TRACER__
  playerExceptionHandler.RemoveHandler();
#endif
  
  
#if defined WIN32 || defined WINDOWS
#else
  return 0;
#endif
}



void ErrorMessage(const char* message) {
  std::cout << std::endl << std::endl << "Error: " << message;

  std::string s;
  std::cin >> s;
}



int main(int argc, char *argv[])
{
#if defined __EXCEPTION_TRACER__
	ExceptionHandler mainExceptionHandler;	
	mainExceptionHandler.InstallHandler();
#endif
	/*ULONG  HeapFragValue = 2;

    if(HeapSetInformation(GetProcessHeap(),HeapCompatibilityInformation,&HeapFragValue,sizeof(HeapFragValue))){
		std::cout << "Heap Success" << std::endl;
	}
    else{
		std::cout << "Heap Error" << std::endl;
	}*/


	//std::cout << ":: OTServ Development-Version 0.4.0 - CVS Preview" << std::endl;
	std::cout << ":: OTServ Version 0.4.0" << std::endl;
	std::cout << ":: ====================" << std::endl;
	std::cout << "::" << std::endl;
	
	// ignore sigpipe...
#if defined __WINDOWS__ || defined WIN32	
		//nothing yet
#else
	struct sigaction sigh;
	sigh.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &sigh, NULL);
#endif
	
	// random numbers generator
	std::cout << ":: Initializing the random numbers... ";
	srand ( time(NULL) );
	std::cout << "[done]" << std::endl;
	
	// read global config
	std::cout << ":: Loading lua script config.lua... ";
	if (!g_config.OpenFile("config.lua"))
	{
		ErrorMessage("Unable to load config.lua!");
		return -1;
	}
	std::cout << "[done]" << std::endl;
	
	//load spells data
	std::cout << ":: Loading spells spells.xml... ";
	if(!spells.loadFromXml(g_config.getGlobalString("datadir")))
	{
		ErrorMessage("Unable to load spells.xml!");
		return -1;
	}
	std::cout << "[done]" << std::endl;
	
	//load actions data
	std::cout << ":: Loading actions actions.xml... ";
	if(!actions.loadFromXml(g_config.getGlobalString("datadir")))
	{
		ErrorMessage("Unable to load actions.xml!");
		return -1;
	}
	std::cout << "[done]" << std::endl;
	
	//load commands data
	std::cout << ":: Loading commands commands.xml... ";
	if(!commands.loadXml(g_config.getGlobalString("datadir")))
	{
		ErrorMessage("Unable to load commands.xml!");
		return -1;
	}
	std::cout << "[done]" << std::endl;
	
	// load item data
	std::cout << ":: Reading tibia.dat ...            ";
	if (Item::items.loadFromDat("tibia.dat"))
	{
		ErrorMessage("Could not load tibia.dat!");
		return -1;
	}
	std::cout << "[done]" << std::endl;
	
	std::cout << ":: Reading " << g_config.getGlobalString("datadir") << "items/items.xml ... ";
	if (Item::items.loadXMLInfos(g_config.getGlobalString("datadir") + "items/items.xml"))
	{
		ErrorMessage("Could not load /items/items.xml ...!");
		return -1;
	}
	std::cout << "[done]" << std::endl;
	
	std::string worldtype = g_config.getGlobalString("worldtype");
	std::transform(worldtype.begin(), worldtype.end(), worldtype.begin(), upchar);
	if(worldtype == "PVP")
		g_game.setWorldType(WORLD_TYPE_PVP);
	else if(worldtype == "NO-PVP")
		g_game.setWorldType(WORLD_TYPE_NO_PVP);
	else if(worldtype == "PVP-ENFORCED")
		g_game.setWorldType(WORLD_TYPE_PVP_ENFORCED);
	else{
		ErrorMessage("Unknown world type!");
		return -1;
	}
	std::cout << ":: World Type: " << worldtype << std::endl;

	if(g_config.getGlobalString("md5passwords") == "yes"){
		passwordType = PASSWORD_TYPE_MD5;
		std::cout << ":: Use MD5 passwords" << std::endl;
	}
	else{
		passwordType = PASSWORD_TYPE_PLAIN;
	}

#ifdef _SQLMAP_
	std::cout << ":: Reading spawn info from database ... \n";
	if(g_game.loadMap(g_config.getGlobalString("sqlmap"))) {
		SpawnManager::initialize(&g_game);
		SpawnManager::instance()->loadSpawnsSQL(g_config.getGlobalString("sqlmap"));
		SpawnManager::instance()->startup();
	}
#elif _BINMAP_
	g_game.loadMap(g_config.getGlobalString("mapfile"));
    // doesnt need to load spawns, it will be loaded from the bin (OTM)
#else
	// load map file
	if(g_game.loadMap(g_config.getGlobalString("mapfile"))) {
		SpawnManager::initialize(&g_game);
		SpawnManager::instance()->loadSpawnsXML(g_game.getSpawnFile());
		SpawnManager::instance()->startup();
	}
#endif		
	
  	// Call to WSA Startup on Windows Systems...
#ifdef WIN32
	WORD wVersionRequested; 
	WSADATA wsaData; 
	wVersionRequested = MAKEWORD( 1, 1 );
	
	if (WSAStartup(wVersionRequested, &wsaData) != 0)
	{
		ErrorMessage("Winsock startup failed!!");
		return -1;
	} 
	
	if ((LOBYTE(wsaData.wVersion) != 1) || (HIBYTE(wsaData.wVersion) != 1)) 
	{ 
		WSACleanup( ); 
		ErrorMessage("No Winsock 1.1 found!");
		return -1;
	} 
#endif


	std::pair<unsigned long, unsigned long> IpNetMask;
	IpNetMask.first  = inet_addr("127.0.0.1");
	IpNetMask.second = 0xFFFFFFFF;
	serverIPs.push_back(IpNetMask);
	
	char szHostName[128];
	if (gethostname(szHostName, 128) == 0)
	{
		std::cout << "::" << std::endl << ":: Running on host " << szHostName << std::endl;
	
		hostent *he = gethostbyname(szHostName);
		
		if (he)
		{
			std::cout << ":: Local IP address(es):  ";
			unsigned char** addr = (unsigned char**)he->h_addr_list;
	
			while (addr[0] != NULL)
			{
				std::cout << (unsigned int)(addr[0][0]) << "."
				<< (unsigned int)(addr[0][1]) << "."
				<< (unsigned int)(addr[0][2]) << "."
				<< (unsigned int)(addr[0][3]) << "  ";
		
			IpNetMask.first  = *(unsigned long*)(*addr);
			IpNetMask.second = 0x0000FFFF;
			serverIPs.push_back(IpNetMask);
		
			addr++;
			}

		std::cout << std::endl;
    		}
  	}
  
	std::cout << ":: Global IP address:     ";
	std::string ip;
	
	if(argc > 1)
		ip = argv[1];
	else
		ip = g_config.getGlobalString("ip", "127.0.0.1");
	
	std::cout << ip << std::endl << "::" << std::endl;
	
	IpNetMask.first  = inet_addr(ip.c_str());
	IpNetMask.second = 0;
	serverIPs.push_back(IpNetMask);
	
	std::cout << ":: Starting Server... ";
	
	Status* status = Status::instance();
	status->playersmax = g_config.getGlobalNumber("maxplayers");
	
	// start the server listen...
	int listen_errors;
	int accept_errors;
	listen_errors = 0;
	while(g_game.getGameState() != GAME_STATE_SHUTDOWN && listen_errors < 100){
		sockaddr_in local_adress;
		memset(&local_adress, 0, sizeof(sockaddr_in)); // zero the struct 
	
		local_adress.sin_family      = AF_INET;
		local_adress.sin_port        = htons(atoi(g_config.getGlobalString("port").c_str()));
		local_adress.sin_addr.s_addr = htonl(INADDR_ANY);
	
		// first we create a new socket
		SOCKET listen_socket = socket(AF_INET, SOCK_STREAM, 0);
	
		if(listen_socket <= 0){
#ifdef WIN32
	    		WSACleanup(); 
#endif
			ErrorMessage("Unable to create server socket (1)!");
			return -1;
  		} // if (listen_socket <= 0)

#ifndef WIN32
		int yes = 1;
		// lose the pesky "Address already in use" error message
		if(setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof (int)) == -1){
			ErrorMessage("Unable to set socket options!");
			return -1;
		}
#endif

		// bind socket on port
		if(bind(listen_socket, (struct sockaddr*)&local_adress, sizeof(struct sockaddr_in)) < 0){
#ifdef WIN32
	    		WSACleanup();    
#endif
			ErrorMessage("Unable to create server socket (2)!");
			return -1;
  		} // if (bind(...))
  
		// now we start listen on the new socket
		if(listen(listen_socket, 10) == SOCKET_ERROR){
#ifdef WIN32
    			WSACleanup();
#endif
    			ErrorMessage("Listen on server socket not possible!");
			return -1;
		} // if (listen(*listen_socket, 10) == -1)


		std::cout << "[done]" << std::endl << ":: OpenTibia Server Running..." << std::endl;
		accept_errors = 0;
		while(g_game.getGameState() != GAME_STATE_SHUTDOWN && accept_errors < 100){
			fd_set listen_set;
			timeval tv;
			FD_ZERO(&listen_set);
			FD_SET(listen_socket, &listen_set);
			tv.tv_sec = 1;
			tv.tv_usec = 0;
			
			int reads = select(1, &listen_set, NULL, NULL, &tv);
			int errnum;
#ifdef WIN32
			errnum = WSAGetLastError();
#else
			errnum = errno;
#endif
			if(reads == SOCKET_ERROR){
				break;
			}
			else if(reads == 0 && errnum == ERROR_EINTR){
				accept_errors++;
				continue;
			}
		
			SOCKET s = accept(listen_socket, NULL, NULL); // accept a new connection
			if(s > 0){
					OTSYS_CREATE_THREAD(ConnectionHandler, (void*)&s);
			}
			else{
					accept_errors++;
			}
		}
		closesocket(listen_socket);
		listen_errors++;
  	}
#ifdef WIN32
  	WSACleanup();
#endif

#if defined __EXCEPTION_TRACER__	
  	mainExceptionHandler.RemoveHandler();
#endif
	return 0;
}

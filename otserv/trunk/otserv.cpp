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

#include "otsystem.h"

#include "protocol70.h"

#include <stdlib.h>
#include <time.h>
#include "map.h"

#include "luascript.h"
#include "account.h"

#ifndef WIN32
#include <fcntl.h>
#include <arpa/inet.h>
#endif


std::vector< std::pair<unsigned long, unsigned long> > serverIPs;

LuaScript g_config;

Items Item::items;
Map gmap;


#include "networkmessage.h"



OTSYS_THREAD_RETURN ConnectionHandler(void *dat)
{
  srand((unsigned)time(NULL));

  SOCKET s = *(SOCKET*)dat;
  
  NetworkMessage msg;
  msg.ReadFromSocket(s);

  unsigned short protId = msg.GetU16();

  // login server connection
  if (protId == 0x0201)
  {
    msg.SkipBytes(15);

    unsigned int accnumber = msg.GetU32();
	 std::string password        = msg.GetString();


    int serverip = serverIPs[0].first;

    sockaddr_in sain;
    socklen_t salen = sizeof(sockaddr_in);
    if (getpeername(s, (sockaddr*)&sain, &salen) == 0)
    {
      unsigned long clientip = *(unsigned long*)&sain.sin_addr;
      for (int i = 0; i < serverIPs.size(); i++)
        if ((serverIPs[i].first & serverIPs[i].second) == (clientip & serverIPs[i].second))
        {
          serverip = serverIPs[i].first;
          break;
        }
    }

    msg.Reset();

    Account account;
    char accstring[16];
	  sprintf(accstring, "%i", accnumber);

    if (account.openAccount(accstring, password))
    {
      msg.AddByte(0x14);
      msg.AddString("1\nWelcome to OpenTibia.");

      msg.AddByte(0x64);
      msg.AddByte(account.charList.size());

		std::list<std::string>::iterator it;
      for (it = account.charList.begin(); it != account.charList.end(); it++)
      {
        msg.AddString((*it));
        msg.AddString("OpenTibia");
        msg.AddU32(serverip);
        msg.AddU16(7171);
      }

      msg.AddU16(account.premDays);
    }
    else
    {
      msg.AddByte(0x0A);
      msg.AddString("Please enter a valid account number and password.");
    }

    msg.WriteToSocket(s);

    closesocket(s);
  }
  // gameworld connection tibia 7.1
  else if (protId == 0x020A)
  {
    unsigned char  clientos = msg.GetByte();
    unsigned short version  = msg.GetU16();
    unsigned char  unknown  = msg.GetByte();

	 std::string name     = msg.GetString();
	 std::string password = msg.GetString();


    Protocol70 *protocol = new Protocol70(s);

    Player *player;
    player = new Player(name.c_str(), protocol);
    player->usePlayer();

    protocol->setPlayer(player);

    Account account;
    if (account.openPlayer(name, password, *player))
    {
      protocol->ConnectPlayer();
      protocol->ReceiveLoop();
    }

    player->releasePlayer();

    closesocket(s);
  }
}




void ErrorMessage(const char* message)
{
  std::cout << std::endl << std::endl << "Error: " << message;

  std::string s;
  std::cin >> s;
}



int main(int argc, char *argv[])
{
  std::cout << ":: OTServ Development-Version 0.3.0" << std::endl;
  std::cout << ":: ====================" << std::endl;
  std::cout << "::" << std::endl;

  // read global config
  std::cout << ":: Loading lua script config.lua... ";
  if (!g_config.OpenFile("config.lua"))
  {
    ErrorMessage("Unable to load config.lua!");
    return -1;
  }
  std::cout << "[done]" << std::endl;


  // load item data
  std::cout << ":: reading tibia.dat ...            ";
	if (Item::items.loadFromDat("tibia.dat"))
  {
    ErrorMessage("Could not load tibia.dat!");
    return -1;
	}
	std::cout << "[done]" << std::endl;


  // load map file
  gmap.LoadMap(g_config.getGlobalString("mapfile"));


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
        IpNetMask.second = 0xFFFFFF00;
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


  // start the server listen...
  sockaddr_in local_adress;
  memset(&local_adress, 0, sizeof(sockaddr_in)); // zero the struct 

  local_adress.sin_family      = AF_INET;
  local_adress.sin_port        = htons(7171);
  local_adress.sin_addr.s_addr = htonl(INADDR_ANY);
 
  // first we create a new socket
  SOCKET listen_socket = socket(AF_INET, SOCK_STREAM, 0);
  
  if (listen_socket <= 0)
  {
#ifdef WIN32
    WSACleanup();   
#endif
    ErrorMessage("Unable to create server socket (1)!");
    return -1;
  } // if (listen_socket <= 0)

#ifndef WIN32
    int yes;
    // lose the pesky "Address already in use" error message
    if (setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof (int)) == -1)  {
        throw texception("network.cpp: setsockopt failed!", true);
    }
#endif
  // bind socket on port
  if (bind(listen_socket, (struct sockaddr*)&local_adress, sizeof(struct sockaddr_in)) < 0)
  {
#ifdef WIN32
    WSACleanup();    
#endif
    ErrorMessage("Unable to create server socket (2)!");
    return -1;
  } // if (bind(...))
  
  // now we start listen on the new socket
  if (listen(listen_socket, 10) == -1)
  {
#ifdef WIN32
    WSACleanup();
#endif
    ErrorMessage("Listen on server socket not possible!");
    return -1;
  } // if (listen(*listen_socket, 10) == -1)



  std::cout << "[done]" << std::endl << ":: OpenTibia Server Running..." << std::endl;



  while (true)
  {
    SOCKET s = accept(listen_socket, NULL, NULL); // accept a new connection

    if (s > 0)
    {
      OTSYS_CREATE_THREAD(ConnectionHandler, (void*)&s);
    }
  }

	return 0;
}

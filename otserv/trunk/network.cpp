//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Network provides some various network tools.
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
// $Id$
//////////////////////////////////////////////////////////////////////
// $Log$
// Revision 1.6  2002/04/08 15:57:03  shivoc
// made some changes to be more ansi compliant
//
// Revision 1.5  2002/04/07 09:52:33  acrimon
// minor changes
//
//////////////////////////////////////////////////////////////////////

#include "network.h"

// include system dependent headers...
#ifdef __WINDOWS__
#  include <winsock.h>
#else
#  include <fcntl.h> // fcntl
#  include <sys/socket.h> // listen bind socket
#  include <sys/types.h> // bind socket inet_addr select
#  include <netinet/in.h> // sockaddr_in inet_addr
#  include <arpa/inet.h> // inet_addr
#  include <unistd.h> // fcntl select
#  include <sys/time.h> // select timeval

#  include <cstdlib> // memory management...
#  include <cstring> // memset
#endif


//////////////////////////////////////////////////
// Sets up the Server and starts listen for connections
Socket TNetwork::make_socket(int _socket_type, u_short _port) throw(texception) {
    int yes=1;
    
#ifdef __WINDOWS__
    // Call to WSA Startup...
    
    WORD wVersionRequested; 
    WSADATA wsaData; 
    int err; 
    wVersionRequested = MAKEWORD(1, 1); 
    
    err = WSAStartup(wVersionRequested, &wsaData); 
    if (err != 0) {
	throw texception("network.cpp: No Winsock.dll found!", true);
    } 
    
    if (LOBYTE(wsaData.wVersion) != 1 || HIBYTE(wsaData.wVersion) != 1) { 
	WSACleanup(); 
	throw texception("network.cpp: No Winsock 1.1 found!", true);
    } 
    // WSA Startup complete...
#endif
    
    sockaddr_in local_adress;
    local_adress.sin_family = AF_INET;
    local_adress.sin_port = htons(_port);
    // just find out the networkaddress... (uses 127.0.0.1 AND the real IP)
    local_adress.sin_addr.s_addr = htonl(INADDR_ANY);
    memset(&(local_adress.sin_zero), '\0', 8); // zero the rest of the struct 

    // first we create a new socket
    Socket listen_socket=socket(AF_INET, _socket_type, 0);
    if (listen_socket < 0) {
	// the socket could not be created!
#ifdef __WINDOWS__
	WSACleanup();
#endif	    
	throw texception("network.cpp: The socket could not be created!", true);
    } // if (listen_socket == -1)
#ifdef __LINUX__
    if (fcntl(listen_socket, F_SETFL, O_NONBLOCK) < 0) {  // set O_NONBLOCK flag
        throw texception("network.cpp: could not set O_NONBLOCK!", true);
    }
    // lose the pesky "Address already in use" error message
    if (setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof (int)) == -1)  {
	throw texception("network.cpp: setsockopt failed!", true);
    } 
#endif

    if (bind(listen_socket, (struct sockaddr *) &local_adress, sizeof (struct sockaddr)) < 0) {
	// bind error
#ifdef __WINDOWS__      
	WSACleanup();
#endif      
	throw texception("network.cpp: bind error!", true);
    } // if (bind(...))

    // now we listen on the new socket
    if (listen(listen_socket, 10) == -1) {
	// error while listening on socket
#ifdef __WINDOWS__      
	WSACleanup();
#endif
	throw texception("network.cpp: listen on socket not possible!", true);
    } // if (listen(listen_socket,10) == -1)

    return listen_socket;
} // void TNetwork::StartServer(Socket& listen_socket) throw(texception)



/****************************************************************
Method: void ShutdownServer(const Socket&)
---------------------------------

Shutdown the Server...
only shuts down the connection

 ****************************************************************/

void TNetwork::ShutdownServer(const Socket& listen_socket) throw() {
#ifdef __LINUX__
	close(listen_socket);
#endif // __LINUX__

#ifdef __WINDOWS__
	closesocket(listen_socket);
	WSACleanup();
#endif

} // void TNetwork::ShutdownServer(const Socket& listen_socket) throw()


/****************************************************************
Method: void SendData(const Socket&, const string&)
------------------------------------------

sends Data to the player

 ****************************************************************/

void TNetwork::SendData(const Socket& playersocket, const std::string& data) throw() {
	size_t total=0;  // bytes we sended total...
	int bytesleft=data.length();  // bytes we need to send...
	int sent;  // bytes send sent...

	while (total < data.length())
	{

		sent=send(playersocket, data.c_str()+total, bytesleft, 0);

		if (sent ==-1 || sent == 0) break;
		total+=sent;
		bytesleft-=sent;
	}  // while (total < length)

} // void TNetwork::SendData(const Socket& playersocket, const string& data) throw() 



/****************************************************************
Method: string ReceiveData(const Socket&, const bool&)
------------------------------------------

receives Data from the player

 ****************************************************************/

std::string TNetwork::ReceiveData(const Socket& playersocket) throw(texception) { 
	// maximum length to read
	const size_t max_read = 4096;

	// number of received bytes...
	int numrecv;

	// our receive buffer
	char data[max_read];

	// initialise the buffer to read

	std::string readbuf="";

	// read the whole incoming data into the buffer...
	while ((numrecv=recv(playersocket,data,max_read,0))==256) {
		readbuf += std::string(data,numrecv);
	} // while (numrecv=recv(playersocket,256,0)) 
	if (numrecv > 0) readbuf += std::string(data,numrecv);

	// error while reading...
	if (numrecv == -1) throw texception(true);

	return readbuf;

} // string TNetwork::ReceiveData(const Socket& playersocket, const bool& check=true) throw(texception)  


////////////////////////////////////////
// shuts down the client socket...
////////////////////////////////////////
void TNetwork::CloseSocket(const Socket& playersocket) throw() {

#ifdef __LINUX__
	close(playersocket);
#endif

#ifdef __WINDOWS__
	closesocket(playersocket);
#endif

} // void TNetwork::CloseSocket(const Socket& playersocket) throw()

/****************************************************************
Method: Socket AcceptPlayer(const Socket&)
------------------------------------------

trys to accept a connecting player if possible...

 ****************************************************************/

Socket TNetwork::AcceptPlayer(const Socket& listen) throw(texception) {

	struct timeval notime;
	struct sockaddr from;

#ifdef __LINUX__
	socklen_t fromlen;
#endif

#ifdef __WINDOWS__  
	int fromlen;
#endif

	fd_set player;

	fromlen=sizeof(sockaddr);

	notime.tv_usec=0;
	notime.tv_sec=0;

	FD_ZERO(&player);
	FD_SET(listen,&player);

	Socket psocket;

	if (select(listen+1,&player,NULL,NULL,(struct timeval*)&notime) > 0)
	{
		psocket=accept(listen, &from, &fromlen);
	}
	else throw texception("no player", false);

	return psocket;

} // Socket TNetwork::AcceptPlayer(const Socket& listen) throw(texception) 

/* OpenTibia - an opensource roleplaying game
 *
 * The ServerSocket creates and manages a socket that listens
 * for incoming connections.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>

#include "serversocket.h"
#include "eventscheduler.h"

extern EventScheduler es;

//////////////////////////////////////////////////
// Take a service name, and a service type, and return a port number.  If the
// service name is not found, it tries it as a decimal number.  The number
// returned is byte ordered for the network.
int ServerSocket::atoport(char *service, char *proto) {
  int port;
  long int lport;
  struct servent *serv;
  char *errpos;

  // First try to read it from /etc/services
  serv = getservbyname(service, proto);
  if (serv != NULL)
    port = serv->s_port;
  else { // Not in services, maybe a number?
    lport = strtol(service, &errpos, 0);
    if ((errpos[0] != 0) || (lport < 1) || (lport > 65535))
      return -1; // Invalid port address
    port = htons(lport);
  }
  return port;
}

//////////////////////////////////////////////////
// Converts ascii text to in_addr struct.  NULL is returned if the address
// can not be found.
struct in_addr *ServerSocket::atoaddr(char *address) {
  struct hostent *host;
  static struct in_addr saddr;

  // First try it as aaa.bbb.ccc.ddd.
  saddr.s_addr = inet_addr(address);
  if (saddr.s_addr != -1) {
    return &saddr;
  }
  host = gethostbyname(address);
  if (host != NULL) {
    return (struct in_addr *) *host->h_addr_list;
  }
  return NULL;
}

#if 0
void write_to_host(int filedes, unsigned char MESSAGE[MAXMSG]) {
  int nbytes;
  
  if (filedes!=1) nbytes=write(filedes,MESSAGE,MAXMSG); /* schreiben */
  fprintf(stderr,"%s an %i\n",MESSAGE,filedes);
  if (nbytes<=0) perror("write");        /* fehler */
}
#endif

//////////////////////////////////////////////////
// creates the socket that can accept connections.
int ServerSocket::make_socket(int socket_type, u_short port) {
  // creating socket
  struct protoent *protox = (struct protoent *) getprotobyname("tcp");
  int sock = socket(AF_INET, socket_type, protox->p_proto);
  if (sock < 0) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  // Internet address information
  struct sockaddr_in address;
  memset((char *) &address, 0, sizeof (address));
  address.sin_family = AF_INET;
  address.sin_port   = htons(port);
  address.sin_addr.s_addr = htonl(INADDR_ANY);

  // set O_NONBLOCK flag!
#if 0
  int reuse_addr = 1;
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof (reuse_addr));
#endif

  if (bind(sock, (struct sockaddr *) &address, sizeof (address)) < 0) {
    perror("bind");
    close(sock);
    exit(EXIT_FAILURE);
  }
  if (listen(sock, 10) < 0) { // maximum 10 pending connections
    perror("listen");
    close(sock);
    exit(EXIT_FAILURE);
  }
  return sock;
}

//////////////////////////////////////////////////
// This class listens on a port to create connections.
ServerSocket::ServerSocket() : newconnection(*this) {
  serversocket = make_socket(SOCK_STREAM, 7171);
  es.newsocket(serversocket, &newconnection);
}

//////////////////////////////////////////////////
// free the socket for use by other programs.
ServerSocket::~ServerSocket() {
  es.deletesocket(serversocket);
  close(serversocket);
}

//////////////////////////////////////////////////
// a new connection is pending. Accept it.
void ServerSocket::newconnection::operator()(const signed int &sock) {
  struct sockaddr_in clientname;
  socklen_t size = sizeof (clientname);
  int cs = accept(sock, (struct sockaddr *) &clientname, &size);
  if (cs < 0) {
    perror("accept");
    return;
  }
  fprintf(stderr, "ip= %s, p= %hd.\n", inet_ntoa(clientname.sin_addr), ntohs(clientname.sin_port));
  es.newsocket(cs, &ss.clientread);
  // read_from_client(cs, active_fd_set, 1);
}

//////////////////////////////////////////////////
// Incoming data from a connection socket. Read it.
// should that be here?
void ServerSocket::clientread::operator()(const signed int &sock) {
  static const int MAXMSG = 4096;
  char buffer[MAXMSG];

  int nbytes = read(sock, buffer, MAXMSG);
  if (nbytes < 0) { // error
    perror("read");
    exit(-1);
  } else if (nbytes == 0) { // eof
    printf("eof!\n");
    es.deletesocket(sock);
    close(sock);
  } else {  // lesen erfolgreich
    buffer[nbytes] = 0;
    printf("%s\n", buffer);
  }
}

#if 0
/* This function listens on a port, and returns connections.  It calls
   cb(connected_socket) for each successful connection and returns after
   the first connection is made.
   It installs a thread that listens for subsequent connections.

   The parameters are as follows:
     socket_type: SOCK_STREAM or SOCK_DGRAM (TCP or UDP sockets)
     port: The port to listen on.  Remember that ports < 1024 are
       reserved for the root user.
     cb: Pointer to a callback function. Called whenever a connection
       is accepted, with the accepted socket number as parameter.
       Pass NULL if not interested. Useful for installing connection
       handlers (using threads) and cleanup handlers. */
int ServerSocket::get_connection(int socket_type, u_short port, void (*cb)(int)) {
  int listening_socket = make_socket(socket_type, htons(port));

  if (socket_type == SOCK_STREAM) {
    listen(listening_socket, 5); // Queue up to five connections before
                                 // having them automatically rejected.

    int connected_socket;
    for (;;) {
      //pthread_testcancel();
      connected_socket = accept(listening_socket, NULL, NULL);
      if (connected_socket != -1) {
	// success.
	break;
      } else if (errno != EINTR) {
	// a real error occured.
	perror("accept");
	close(listening_socket);
	exit(EXIT_FAILURE);
      } // else (errno == EINTR) blocking was interrupted for some reason.
    }

    if (cb) cb(connected_socket);

    return connected_socket;
  } else
    return listening_socket;
}

/* This is a generic function to make a connection to a given server/port.
   port is the port number in network byte order (see "man htons"),
   type is either SOCK_STREAM or SOCK_DGRAM, and
   netaddress is the host name to connect to.
   The function returns the socket, ready for action.*/
int ServerSocket::make_connection(int type, u_short port, char *netaddress) {
  struct in_addr *addr;
  int sock, connected;
  struct sockaddr_in address;

  char *pname;
  if (type == SOCK_STREAM) 
    pname = "tcp";
  else if (type == SOCK_DGRAM)
    pname = "udp";
  else {
    fprintf(stderr, "make_connection:  Invalid socket type.\n");
    return -1;
  }
  addr = atoaddr(netaddress);
  if (addr == NULL) {
    fprintf(stderr,"make_connection:  Invalid network address.\n");
    return -1;
  }
 
  memset((char *) &address, 0, sizeof(address));
  address.sin_family = AF_INET;
  address.sin_port = port;
  address.sin_addr.s_addr = addr->s_addr;

  struct protoent *protox = (struct protoent *) getprotobyname(pname);
  sock = socket(AF_INET, type, protox->p_proto);

  printf("Connecting to %s on port %d.\n", inet_ntoa(*addr), htons(port));

  if (type == SOCK_STREAM) {
    connected = connect(sock, (struct sockaddr *) &address, sizeof (address));
    while (connected < 0) {
      perror("connect");
      sleep(10);
      connected = connect(sock, (struct sockaddr *) &address, sizeof (address));
    }
    return sock;
  }
  /* Otherwise, must be for udp, so bind to address. */
  if (bind(sock, (struct sockaddr *) &address, sizeof(address)) < 0) {
    perror("bind");
    return -1;
  }
  return sock;
}
#endif

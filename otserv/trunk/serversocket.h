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

#include "definitions.h"

class ServerSocket {
  Socket serversocket;
public: // this is public only for the functors; it should be private
  int connections;
  int maxconnections;
private:
  struct newconnection : public unary_functor<Socket,void> {
    ServerSocket &ss;
    newconnection(ServerSocket &_ss) : ss(_ss) { }
    void operator() (const Socket &z);
  } newconnection;
#if 0
public:
  struct clientread : public unary_functor<Socket,void> {
    ServerSocket &ss;
    clientread(ServerSocket &_ss) : ss(_ss) { }
    void operator() (const Socket &z);
  } clientread;
#endif
private:
  Socket make_socket(int socket_type, u_short port);
  int atoport(char *service, char *proto);
  struct in_addr *atoaddr(char *address);
public:
  //ServerSocket();
  ServerSocket(Socket _sock = 7171, int _maxconnections = 100);
  ~ServerSocket();
};

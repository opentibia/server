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
private:
  struct newconnection : public unary_functor<int,void> {
    ServerSocket &ss;
    newconnection(ServerSocket &_ss) : ss(_ss) { }
    void operator() (const int &z);
  } newconnection;
public:
  struct clientread : public unary_functor<int,void> {
    void operator() (const int &z);
  } clientread;
private:
  int serversocket;
  int make_socket(int socket_type, u_short port);
  int atoport(char *service, char *proto);
  struct in_addr *atoaddr(char *address);
public:
  ServerSocket();
  ~ServerSocket();
};

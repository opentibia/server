//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// The ServerSocket creates and manages a socket that listens
// for incoming connections.
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
// Revision 1.8  2002/04/07 09:52:33  acrimon
// minor changes
//
// Revision 1.7  2002/04/06 09:04:30  shivoc
// moved make_socket and added win support
//
// Revision 1.6  2002/04/06 08:09:12  shivoc
// moved serversocket to TNetwork Namespace and minor changes to regain win compatibility (unfinished)
//
// Revision 1.5  2002/04/05 18:56:11  acrimon
// Adding a file class.
//
//////////////////////////////////////////////////////////////////////

#include "definitions.h"

namespace TNetwork {
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
    private:
//        Socket make_socket(int socket_type, u_short port);
	int atoport(char *service, char *proto);
	struct in_addr *atoaddr(char *address);
    public:
	ServerSocket(int _port= 7171, int _maxconnections = 100);
	~ServerSocket();
    };

} // namespace TNetwork

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
// $Id$
//////////////////////////////////////////////////////////////////////
// $Log$
// Revision 1.10  2003/09/17 16:35:08  tliffrag
// added !d command and fixed lag on windows
//
// Revision 1.9  2003/09/08 13:28:41  tliffrag
// Item summoning and maploading/saving now working.
//
// Revision 1.8  2003/05/19 16:48:37  tliffrag
// Loggingin, talking, walking around, logging out working
//
// Revision 1.7  2002/05/29 16:07:38  shivoc
// implemented non-creature display for login
//
// Revision 1.6  2002/05/28 13:55:56  shivoc
// some minor changes
//
// Revision 1.5  2002/04/08 13:53:59  acrimon
// Added some very basic map support
//
//////////////////////////////////////////////////////////////////////
#include <string>
#include "eventscheduler.h"
#include "serversocket.h"
#include <stdlib.h>
#include <time.h>
#include "tmap.h"

int ipFromDotted(char*);
int g_serverip;
EventScheduler es;
Map::Map map;

int main(int argc, char *argv[]) {
	char* ip;
	if(argc>1)
		ip=argv[1];
	else
		ip="127.0.0.1";
	g_serverip=ipFromDotted(ip);;
	srand(time(NULL));
    TNetwork::ServerSocket ss;
    es.loop();
}

int ipFromDotted(char* _ip){
  std::string ip=_ip;
  std::string t;
  int num=0;

  for(int i=0; i<4;i++){
    t="";
    while((ip[0]!='.')^(ip.length()==0)){
      t+=ip[0];
      ip.erase(0,1);
    }
    ip.erase(0,1);
    num+=atoi(t.c_str()) << i*8;
  }
  printf("\n%i\n", num);
  return num;
}

int hexint(const char *src)
{
 unsigned int y; /* unsigned for correct wrapping. */
 int h;

 /* high part. */
 if ((y = src[0] - '0') <= '9'-'0') h = y;
 else if ((y = src[0] - 'a') <= 'f'-'a') h = y+10;
 else if ((y = src[0] - 'A') <= 'F'-'A') h = y+10;
 else return -1;
 h <<= 4;

 /* low part. */
 if ((y = src[1] - '0') <= '9'-'0') return h | y;
 if ((y = src[1] - 'a') <= 'f'-'a') return h | (y+10);
 if ((y = src[1] - 'A') <= 'F'-'A') return h | (y+10);
 return -1;
}


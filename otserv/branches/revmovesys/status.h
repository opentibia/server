//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Status-Singleton for OTServ
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

#ifndef __OTSERV_STATUS_H
#define __OTSERV_STATUS_H

#include <string>
#include "otsystem.h"
#include "definitions.h"
#include "networkmessage.h"


class Status{
  public:
  // procs       
	void addPlayer();
	void removePlayer();
	static Status* instance();
	std::string getStatusString();
	void getInfo(NetworkMessage &nm);
	bool hasSlot();
	
	// vars
	int playersonline, playersmax, playerspeak;
	std::string ownername, owneremail;
	std::string motd;
	std::string mapname, mapauthor;
	int mapsizex, mapsizey;
	std::string servername, location, url;
	std::string version;
	uint64_t start;


  private:
	Status();
	static Status* _Status;

	// the stats of our server
};

#endif

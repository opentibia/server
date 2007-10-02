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
#include "definitions.h"
#include "networkmessage.h"

#include "protocol.h"

class ProtocolStatus : public Protocol 
{
public:
	ProtocolStatus(Connection* connection) : Protocol(connection) {}
	virtual ~ProtocolStatus() {}

	virtual void onRecvFirstMessage(NetworkMessage& msg);
	
protected:
	#ifdef __DEBUG_NET_DETAIL__
	virtual void deleteProtocolTask();		
	#endif
};

class Status{
public:
	// procs
	static Status* instance()
	{
		static Status status;
		return &status;
	}
	
	void addPlayer();
	void removePlayer();
	bool hasSlot();
	
	std::string getStatusString();
	void getInfo(uint32_t requestedInfo, OutputMessage* output);

	int getPlayersOnline(){return playersonline;}
	int getMaxPlayersOnline(){return playersmax;}
	
	//static OTSYS_THREAD_RETURN SendInfoThread(void *p);

	uint64_t start;
	int playersmax;

private:
	int playersonline, playerspeak;
	std::string ownername, owneremail;
	std::string motd;
	std::string mapname, mapauthor;
	int mapsizex, mapsizey;
	std::string servername, location, url;
	std::string version;

	Status();
};

#endif

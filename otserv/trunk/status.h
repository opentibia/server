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
	static std::map<uint32_t, uint64_t> ipConnectMap;

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
	bool hasSlot() const;
	
	std::string getStatusString() const;
	void getInfo(uint32_t requestedInfo, OutputMessage* output) const;

	int getPlayersOnline() const {return m_playersonline;}
	int getMaxPlayersOnline() const {return m_playersmax;}

	void setMaxPlayersOnline(int max){m_playersmax = max;}
	
	uint64_t getUptime() const;

protected:
	Status();

private:
	uint64_t m_start;
	int m_playersmax, m_playersonline, m_playerspeak;
	std::string m_mapname, m_mapauthor;

};

#endif

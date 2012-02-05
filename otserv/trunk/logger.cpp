//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Logger class - captures everything that happens on the server
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
#include "otpch.h"

#include "logger.h"
#include "tools.h"
#include <ctime>

Logger::Logger()
{
	m_file.open("otlog.txt", std::ios::app);
	if(m_file.good()){
		m_registering = true;
	}
}

Logger::~Logger()
{
	if(m_registering){
		m_file.close();
	}
}

void Logger::logMessage(const char* channel, eLogType type, int level, std::string message, const char* func)
{
	//TODO: decide if should be saved or not depending on channel type and level
	// if should be save decide where and how

	//check if the file is open, if not, avoid writting to file
	if(!m_registering){
		return;
	}

	//write timestamp of the event
	char buffer[32];
	time_t now = std::time(NULL);
	formatDate(now, buffer);
	m_file << buffer << std::endl;

	//write channel generating the message
	if(channel){
		m_file << " [" << channel << "] ";
	}

	//write message type
	std::string type_str;
	switch(type){
	case LOGTYPE_EVENT:
		type_str = "event";
		break;
	case LOGTYPE_WARNING:
		type_str = "warning";
		break;
	case LOGTYPE_ERROR:
		type_str = "ERROR";
		break;
	default:
		type_str = "???";
		break;
	}
	m_file << " " << type_str << ":";

	//write the message
	m_file << " " << message << std::endl;

	m_file.flush();
}

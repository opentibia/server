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

#include "logger.h"
#include <iostream>

Logger* Logger::instance = NULL;

Logger::Logger(){}

Logger* Logger::getInstance(){
	if(!instance)
		instance = new Logger();
	return instance;
}

void Logger::logMessage(std::string channel, eLogType type, int level,
			std::string message, std::string func,
			int line, std::string file)
{
	std::string sType;
	switch(type){
		case ERROR:
			sType = "error";
			break;
		case EVENT:
			sType = "event";
			break;
		case WARNING:
			sType = "warning";
	}
	std::cout << "Channel: " << channel << std::endl;
	std::cout << "Type: " << sType << std::endl;
	std::cout << "Level: " << level << std::endl;
	std::cout << "Messafe: " << message << std::endl;
	std::cout << "Func: " << func << std::endl; 
	std::cout << "Line: " << line << std::endl; 
	std::cout << "File: " << file << std::endl; 
}

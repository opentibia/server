//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Base class for the Account Loader/Saver
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

int Logger::masterLogLevel = 3;
int Logger::masterFormat = 0;

ScreenLogger::ScreenLogger(std::string id){
    //
}

void ScreenLogger::log(std::string msg){
    std::cout << msg << std::endl << std::flush;
}


FileLogger::FileLogger(std::string id){
    //open the file
}

void FileLogger::log(std::string msg){
    //file << msg << std::endl << std::flush;
}


LoggingChannel::LoggingChannel(){
    //
    format = 0;
    logLevel = 3;
}

void LoggingChannel::setLogLevel(int l){
    logLevel = l;
}

int LoggingChannel::getLogLevel(){
    return logLevel;
}

   
void LoggingChannel::setFormat(int f){
    format = f;
}

int LoggingChannel::getFormat(){
    return format;
}

void LoggingChannel::log(std::string msg, std::string function="", std::string file="", int line=0){

}

void LoggingChannel::setTarget(std::string t){
    //break up target and find out which logger to use
    size_t p = t.find(":");
    std::string method = t.substr(0, p);
    std::string id = t.substr(p+1);
    if(method == "screen"){
	lt = new ScreenLogger(id);
    }
    else if(method == "file"){
	lt = new FileLogger(id);
    }
    else{
	std::cerr << "unknown method: '" << method << "'" << std::endl;  
    }
}

LoggingChannel* Logger::getChannel(std::string channel){
    if(channels.find(channel) != channels.end()){
	LoggingChannel* c = new LoggingChannel();
	channels[channel] = c;
	c->setLogLevel(masterLogLevel);
	c->setFormat(masterFormat);
	c->setTarget("screen");
    }
    return channels[channel];
} 

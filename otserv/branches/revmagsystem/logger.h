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


#ifndef __LOGGER_H
#define __LOGGER_H

#include <string>
#include <map>

class LogTarget{
 public:
    LogTarget();
    virtual void log(std::string)=0;
};

class ScreenLogger : public LogTarget{
 public:
    ScreenLogger(std::string);
    void log(std::string);
};

class FileLogger : public LogTarget{
 public:
    FileLogger(std::string);
    void log(std::string);
};

class LoggingChannel {
 public:
    LoggingChannel();

    void setLogLevel(int l);
    int getLogLevel();
    
    void setFormat(int f);
    int getFormat();
 
    void log(std::string msg);
    void log(std::string msg, std::string function);
    void log(std::string msg, std::string function, std::string file, int line);
    void setTarget(std::string);

 private:
    /******************************
     0 = channel, time with seconds, message
     1 = channel, time with seconds, function
     2 = channel, time with milliseconds, function, linenumbers, file
    */
    int format;
    /*****************************
     0 = Errors
     1 = Warnings
     2 = Notices
     3 = Debug
     4 = All debug
    */
    int logLevel;
    LogTarget* lt;
};

class Logger {
 public:
    static LoggingChannel* getChannel(std::string channel);
 private:
    static std::map<std::string, LoggingChannel*> channels;
    static int masterLogLevel;
    static int masterFormat;
};


#endif

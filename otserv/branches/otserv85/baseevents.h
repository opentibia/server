//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// 
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


#ifndef __BASEEVENTS_H__
#define __BASEEVENTS_H__

#include "luascript.h"
#include <libxml/parser.h>

class Event;

class BaseEvents
{
public:
	BaseEvents();
	virtual ~BaseEvents();
	
	bool loadFromXml(const std::string& datadir);
	bool reload();
	bool isLoaded(){return m_loaded;}
	
protected:
	virtual LuaScriptInterface& getScriptInterface() = 0;
	virtual std::string getScriptBaseName() = 0;
	virtual Event* getEvent(const std::string& nodeName) = 0;
	virtual bool registerEvent(Event* event, xmlNodePtr p) = 0;
	virtual void clear() = 0;
	
	bool m_loaded;
	std::string m_datadir;
	
};

class Event{
public:
	Event(LuaScriptInterface* _interface);
	virtual ~Event();
	
	virtual bool configureEvent(xmlNodePtr p) = 0;
	
	bool checkScript(const std::string& datadir, const std::string& scriptsName, const std::string& scriptFile);
	bool loadScript(const std::string& scriptFile);
	virtual bool loadFunction(const std::string& functionName);
		
protected:
	virtual std::string getScriptEventName() = 0;
	
	int32_t m_scriptId;
	LuaScriptInterface* m_scriptInterface;
	
	bool m_scripted;
};


class CallBack{
public:
	CallBack();
	virtual ~CallBack();
	
	bool loadCallBack(LuaScriptInterface* _interface, std::string name);
	
protected:
	int32_t m_scriptId;
	LuaScriptInterface* m_scriptInterface;
	
	bool m_loaded;
	
	std::string m_callbackName;
};

#endif

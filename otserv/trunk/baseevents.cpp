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
#include "otpch.h"

#include <libxml/xmlmemory.h>
#include <libxml/parser.h> 
#include "tools.h"

#include "baseevents.h"

BaseEvents::BaseEvents()
{
	m_loaded = false;
}

BaseEvents::~BaseEvents()
{
	//
}
	
bool BaseEvents::loadFromXml(const std::string& datadir)
{
	if(m_loaded){
		std::cout << "Error: [BaseEvents::loadFromXml] loaded == true" << std::endl;
		return false;
	}
	m_datadir = datadir;
	Event* event = NULL;

	
	std::string scriptsName = getScriptBaseName();
	
	if(getScriptInterface().loadFile(std::string(m_datadir + scriptsName + "/lib/" + scriptsName + ".lua")) == -1){
		std::cout << "Warning: [BaseEvents::loadFromXml] Can not load " << scriptsName << " lib/" << scriptsName << ".lua" << std::endl;
	}
	
	std::string filename = m_datadir + scriptsName + "/" + scriptsName + ".xml";
	xmlDocPtr doc = xmlParseFile(filename.c_str());
	
	if(doc){
		m_loaded = true;
		xmlNodePtr root, p;
		root = xmlDocGetRootElement(doc);
		
		if(xmlStrcmp(root->name,(const xmlChar*)scriptsName.c_str()) != 0){
			xmlFreeDoc(doc);
			return false;
		}

		p = root->children;
		while(p){
			if(p->name){
				std::string nodeName = (const char*)p->name;
				if((event = getEvent(nodeName))){
					if(event->configureEvent(p)){
						bool success = true;
						std::string scriptfile;
						if(readXMLString(p, "script", scriptfile)){
							if(!event->checkScript(m_datadir, scriptsName, "/scripts/" + scriptfile)){
								success = false;
							}
							else
							if(!event->loadScript(m_datadir + scriptsName + "/scripts/" + scriptfile)){
								success = false;
							}
						}
						else if(readXMLString(p, "function", scriptfile)){
							if(!event->loadFunction(scriptfile)){
								success = false;
							}
						}
						else{
							success = false;
						}
						
						if(success){
							if(!registerEvent(event, p)){
								success = false;
								delete event;
							}
						}
						else{
							delete event;
						}
					}
					else{
						std::cout << "Warning: [BaseEvents::loadFromXml] Can not configure event" << std::endl;
						delete event;
					}
					event = NULL;
				}
			}
			p = p->next;
		}
		
		xmlFreeDoc(doc);
	}
	else{
		std::cout << "Warning: [BaseEvents::loadFromXml] Can not open " << scriptsName << ".xml" << std::endl;
	}
	
	return m_loaded;
}

bool BaseEvents::reload()
{
	m_loaded = false;
	clear();
	return loadFromXml(m_datadir);
}


Event::Event(LuaScriptInterface* _interface)
{
	m_scriptInterface = _interface;
	m_scriptId = 0;
	m_scripted = false;
}

Event::~Event()
{
	//
}
	
bool Event::checkScript(const std::string& datadir, const std::string& scriptsName, const std::string& scriptFile)
{
	LuaScriptInterface testInterface("Test Interface");
	testInterface.initState();
	
	if(testInterface.loadFile(std::string(datadir + scriptsName + "/lib/" + scriptsName + ".lua")) == -1){
		std::cout << "Warning: [Event::checkScript] Can not load " << scriptsName << " lib/" << scriptsName << ".lua" << std::endl;
	}

	if(m_scriptId != 0){
		std::cout << "Failure: [Event::checkScript] scriptid = " << m_scriptId << std::endl;
		return false;
	}
	
	if(testInterface.loadFile(datadir + scriptsName + scriptFile) == -1){
		std::cout << "Warning: [Event::checkScript] Can not load script. " << scriptFile << std::endl;
		std::cout << m_scriptInterface->getLastLuaError() << std::endl;
		return false;
	}

	int32_t id = testInterface.getEvent(getScriptEventName());
	if(id == -1){
		std::cout << "Warning: [Event::checkScript] Event " << getScriptEventName() << " not found. " << scriptFile << std::endl;
		return false;
		
	}

	return true;
}

bool Event::loadScript(const std::string& scriptFile)
{
	if(!m_scriptInterface || m_scriptId != 0){
		std::cout << "Failure: [Event::loadScript] m_scriptInterface == NULL. scriptid = " << m_scriptId << std::endl;
		return false;
	}
	
	if(m_scriptInterface->loadFile(scriptFile) == -1){
		std::cout << "Warning: [Event::loadScript] Can not load script. " << scriptFile << std::endl;
		std::cout << m_scriptInterface->getLastLuaError() << std::endl;
		return false;
	}
	int32_t id = m_scriptInterface->getEvent(getScriptEventName());
	if(id == -1){
		std::cout << "Warning: [Event::loadScript] Event " << getScriptEventName() << " not found. " << scriptFile << std::endl;
		return false;
		
	}
	m_scripted = true;
	m_scriptId = id;
	return true;
}

bool Event::loadFunction(const std::string& functionName)
{
	return false;
}

CallBack::CallBack()
{
	m_scriptId = 0;
	m_scriptInterface = NULL;
	m_loaded = false;
}

CallBack::~CallBack()
{
	//
}
	
bool CallBack::loadCallBack(LuaScriptInterface* _interface, std::string name)
{
	if(!_interface){
		std::cout << "Failure: [CallBack::loadCallBack] m_scriptInterface == NULL" << std::endl;
		return false;
	}
	
	m_scriptInterface = _interface;
	
	int32_t id = m_scriptInterface->getEvent(name);
	if(id == -1){
		std::cout << "Warning: [CallBack::loadCallBack] Event " << name << " not found." << std::endl;
		return false;
	}
	
	m_callbackName = name;
	m_scriptId = id;
	m_loaded = true;
	return true;
}

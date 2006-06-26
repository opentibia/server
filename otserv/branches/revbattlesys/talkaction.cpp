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

#include "creature.h"
#include "player.h"
#include "tools.h"
#include <sstream>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h> 

#include "talkaction.h"

extern Game g_game;

TalkActions::TalkActions() :
m_scriptInterface("TalkAction Interface")
{
	m_loaded = false;
	m_scriptInterface.initState();
}

TalkActions::~TalkActions()
{
	clear();
}

void TalkActions::clear()
{
	TalkActionList::iterator it = wordsMap.begin();
	while(it != wordsMap.end()){
		delete it->second;
		wordsMap.erase(it);
		it = wordsMap.begin();
	}
	
	m_scriptInterface.reInitState();
}

bool TalkActions::reload(){
	m_loaded = false;
	//unload
	clear();
	//load
	return loadFromXml(m_datadir);
}

bool TalkActions::loadFromXml(const std::string& _datadir)
{
	if(m_loaded){
		std::cout << "Error: [TalkActions::loadFromXml] loaded == true" << std::endl;
		return false;
	}
	m_datadir = _datadir;
	TalkAction* talkAction = NULL;
	//load TalkActions lib in script interface
	if(m_scriptInterface.loadFile(std::string(m_datadir + "talkactions/lib/talkactions.lua")) == -1){
		std::cout << "Warning: [TalkActions::loadFromXml] Can not load talkactions lib/talkactions.lua" << std::endl;
	}
	
	std::string filename = m_datadir + "talkactions/talkactions.xml";
	xmlDocPtr doc = xmlParseFile(filename.c_str());
	
	if(doc){
		m_loaded = true;
		xmlNodePtr root, p;
		root = xmlDocGetRootElement(doc);
		
		if(xmlStrcmp(root->name,(const xmlChar*)"talkactions") != 0){
			xmlFreeDoc(doc);
			return false;
		}

		p = root->children;
		while(p){
			if(xmlStrcmp(p->name, (const xmlChar*)"talkaction") == 0){
				talkAction = new TalkAction(&m_scriptInterface);
				if(talkAction->configureTalkAction(p)){
					bool success = true;
					std::string scriptfile;
					if(readXMLString(p, "script", scriptfile)){
						if(!talkAction->loadScriptSay(m_datadir + std::string("talkactions/scripts/") + scriptfile)){
							success = false;
						}
					}
					else{
						//TODO? hardcoded actions ....
						success = false;
					}
						
					if(success){
						wordsMap.push_back(std::make_pair(talkAction->getWords(), talkAction));
					}
					else{
						delete talkAction;
					}
				}
				else{
					std::cout << "Warning: [TalkActions::loadFromXml] Can not configure talkaction" << std::endl;
					delete talkAction;
				}
			}
			p = p->next;
		}
		
		xmlFreeDoc(doc);
	}
	else{
		std::cout << "Warning: [TalkActions::loadFromXml] Can not open talkactions.xml" << std::endl;
	}
	return m_loaded;
}

talkActionResult_t TalkActions::creatureSay(Creature *creature, SpeakClasses type, const std::string& words)
{
	if(type != SPEAK_SAY){
		return TALKACTION_CONTINUE;
	}
	
	std::string str_words;
	std::string str_param;
	unsigned int loc = (uint32_t)words.find( '"', 0 );
	if(loc != std::string::npos && loc >= 0){
		str_words = std::string(words, 0, loc);
		str_param = std::string(words, (loc+1), words.size()-loc-1);
	}
	else {
		str_words = words;
		str_param = std::string(""); 
	}
	
	TalkActionList::iterator it;
	for(it = wordsMap.begin(); it != wordsMap.end(); ++it){
		if(it->first == str_words){
			TalkAction* talkAction = it->second;
			long ret =  talkAction->executeSay(creature, str_words, str_param);
			if(ret == 1){
				return TALKACTION_CONTINUE;
			}
			else{
				return TALKACTION_NO_CONTINUE;
			}
		}
	}
	return TALKACTION_CONTINUE;
}


TalkAction::TalkAction(LuaScriptInterface* _interface)
{
	m_scriptInterface = _interface;
	m_scriptId = 0;
}

TalkAction::~TalkAction()
{
	//
}

bool TalkAction::configureTalkAction(xmlNodePtr p)
{
	std::string str;
	if(readXMLString(p, "words", str)){
		m_words = str;
	}
	else{
		std::cout << "Error: [TalkAction::configureTalkAction] No words for TalkAction or Spell." << std::endl;
		return false;
	}
	return true;
}

bool TalkAction::loadScriptSay(const std::string& script)
{
	if(!m_scriptInterface || m_scriptId != 0){
		std::cout << "Failure: [TalkAction::loadScript] m_scriptInterface == NULL. scriptid = " << m_scriptId << std::endl;
		return false;
	}
	
	if(m_scriptInterface->loadFile(script) == -1){
		std::cout << "Warning: [TalkAction::loadScript] Can not load script. " << script << std::endl;
		return false;
	}
	long id = m_scriptInterface->getEvent("onSay");
	if(id == -1){
		std::cout << "Warning: [TlakAction::loadScript] Event onSay not found. " << script << std::endl;
		return false;
		
	}
	m_scriptId = id;
	return true;
}


long TalkAction::executeSay(Creature* creature, const std::string& words, const std::string& param)
{
	//onSay(cid, words, param)
	ScriptEnviroment* env = m_scriptInterface->getScriptEnv();
	
	//debug only
	std::stringstream desc;
	desc << creature->getName() << " - " << words << " " << param;
	env->setEventDesc(desc.str());
	//
	
	env->setScriptId(m_scriptId, m_scriptInterface);
	env->setRealPos(creature->getPosition());
	
	long cid = env->addThing((Thing*)creature);
	
	lua_State* L = m_scriptInterface->getLuaState();
	int size0 = lua_gettop(L);
	
	m_scriptInterface->pushFunction(m_scriptId);
	lua_pushnumber(L, cid);
	lua_pushstring(L, words.c_str());
	lua_pushstring(L, param.c_str());
	
	long ret;
	if(m_scriptInterface->callFunction(3, ret) == false){
		ret = 0;
	}

	if(size0 != lua_gettop(L)){
		LuaScriptInterface::reportError(NULL, "Stack size changed!");
	}
	
	return ret;
}

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

LuaScriptInterface& TalkActions::getScriptInterface()
{
	return m_scriptInterface;	
}

std::string TalkActions::getScriptBaseName()
{
	return "talkactions";	
}

Event* TalkActions::getEvent(const std::string& nodeName)
{
	if(nodeName == "talkaction"){
		return new TalkAction(&m_scriptInterface);
	}
	else{
		return NULL;
	}
}

bool TalkActions::registerEvent(Event* event, xmlNodePtr p)
{
	TalkAction* talkAction = dynamic_cast<TalkAction*>(event);
	if(!talkAction)
		return false;
	
	wordsMap.push_back(std::make_pair(talkAction->getWords(), talkAction));
	return true;
}

TalkActionResult_t TalkActions::playerSaySpell(Player* player, SpeakClasses type, const std::string& words)
{
	if(type != SPEAK_SAY){
		return TALKACTION_CONTINUE;
	}
	
	std::string str_words;
	std::string str_param;
	size_t loc = words.find( '"', 0 );
	if(loc != std::string::npos && loc >= 0){
		str_words = std::string(words, 0, loc);
		str_param = std::string(words, (loc+1), words.size()-loc-1);
	}
	else {
		str_words = words;
		str_param = std::string(""); 
	}
	
	trim_left(str_words, " ");
	trim_right(str_words, " ");

	TalkActionList::iterator it;
	for(it = wordsMap.begin(); it != wordsMap.end(); ++it){
		if(it->first == str_words){
			TalkAction* talkAction = it->second;
			uint32_t ret =  talkAction->executeSay(player, str_words, str_param);
			if(ret == 1){
				return TALKACTION_CONTINUE;
			}
			else{
				return TALKACTION_BREAK;
			}
		}
	}
	return TALKACTION_CONTINUE;
}


TalkAction::TalkAction(LuaScriptInterface* _interface) :
Event(_interface)
{
	//
}

TalkAction::~TalkAction()
{
	//
}

bool TalkAction::configureEvent(xmlNodePtr p)
{
	std::string str;
	if(readXMLString(p, "words", str)){
		m_words = str;
	}
	else{
		std::cout << "Error: [TalkAction::configureEvent] No words for TalkAction or Spell." << std::endl;
		return false;
	}
	return true;
}

std::string TalkAction::getScriptEventName()
{
	return "onSay";
}

uint32_t TalkAction::executeSay(Creature* creature, const std::string& words, const std::string& param)
{
	//onSay(cid, words, param)
	if(m_scriptInterface->reserveScriptEnv()){
		ScriptEnviroment* env = m_scriptInterface->getScriptEnv();
	
		#ifdef __DEBUG_LUASCRIPTS__
		std::stringstream desc;
		desc << creature->getName() << " - " << words << " " << param;
		env->setEventDesc(desc.str());
		#endif
	
		env->setScriptId(m_scriptId, m_scriptInterface);
		env->setRealPos(creature->getPosition());
	
		uint32_t cid = env->addThing(creature);
	
		lua_State* L = m_scriptInterface->getLuaState();
	
		m_scriptInterface->pushFunction(m_scriptId);
		lua_pushnumber(L, cid);
		lua_pushstring(L, words.c_str());
		lua_pushstring(L, param.c_str());
	
		int32_t result = m_scriptInterface->callFunction(3);
		m_scriptInterface->releaseScriptEnv();
		
		return (result == LUA_TRUE);
	}
	else{
		std::cout << "[Error] Call stack overflow. TalkAction::executeSay" << std::endl;
		return 0;
	}
}

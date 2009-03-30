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

TalkActionResult_t TalkActions::onPlayerSpeak(Player* player, SpeakClasses type, const std::string& words)
{
	if(type != SPEAK_SAY){
		return TALKACTION_CONTINUE;
	}

	std::string str_words_quote;
	std::string str_param_quote;
	std::string str_words_first_word;
	std::string str_param_first_word;
	
	// With quotation filtering
	size_t loc = words.find( '"', 0 );
	if(loc != std::string::npos && loc >= 0){
		str_words_quote = std::string(words, 0, loc);
		str_param_quote = std::string(words, (loc+1), words.size()-loc-1);
	}
	else {
		str_words_quote = words;
		str_param_quote = std::string(""); 
	}
	
	trim_left(str_words_quote, " ");
	trim_right(str_param_quote, " ");
	
	// With whitespace filtering
	loc = words.find( ' ', 0 );
	if(loc != std::string::npos && loc >= 0){
		str_words_first_word = std::string(words, 0, loc);
		str_param_first_word = std::string(words, (loc+1), words.size()-loc-1);
	}
	else {
		str_words_first_word = words;
		str_param_first_word = std::string(""); 
	}

	TalkActionList::iterator it;
	for(it = wordsMap.begin(); it != wordsMap.end(); ++it){
		std::string cmdstring;
		std::string paramstring;
		if(it->second->getFilterType() == TALKACTION_MATCH_QUOTATION) {
			cmdstring = str_words_quote;
			paramstring = str_param_quote;
		} else if(it->second->getFilterType() == TALKACTION_MATCH_FIRST_WORD) {
			cmdstring = str_words_first_word;
			paramstring = str_param_first_word;
		} else {
			continue;
		}
		if(cmdstring == it->first || !it->second->isCaseSensitive() && strcasecmp(it->first.c_str(), cmdstring.c_str()) == 0){
			TalkAction* talkAction = it->second;
			uint32_t ret =  talkAction->executeSay(player, cmdstring, paramstring);
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
Event(_interface),
filterType(TALKACTION_MATCH_QUOTATION),
caseSensitive(false)
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
	int intValue;
	if(readXMLString(p, "words", str)){
		commandString = str;
	}
	else{
		std::cout << "Error: [TalkAction::configureEvent] No words for TalkAction or Spell." << std::endl;
		return false;
	}

	if(readXMLString(p, "filter", str)){
		if(str == "quotation") {
			filterType = TALKACTION_MATCH_QUOTATION;
		} else if(str == "first word") {
			filterType = TALKACTION_MATCH_FIRST_WORD;
		}
	}
	
	if(readXMLInteger(p, "case-sensitive", intValue) || readXMLInteger(p, "sensitive", intValue)){
		caseSensitive = (intValue != 0);
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
		
		return (result != LUA_FALSE);
	}
	else{
		std::cout << "[Error] Call stack overflow. TalkAction::executeSay" << std::endl;
		return 0;
	}
}

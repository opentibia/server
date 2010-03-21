//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Lua script interface
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

#include "params.h"

void Parameters_t::set(const std::string key, const std::string val)
{
	IntParameters_t::iterator it = intParameters.find(key);
	if (it != intParameters.end())
		intParameters.erase(it);
	stringParameters[key] = val;
}

void Parameters_t::set(const std::string key, const int32_t val)
{
	StringParameters_t::iterator it = stringParameters.find(key);
	if (it != stringParameters.end())
		stringParameters.erase(it);
	intParameters[key] = val;
}

bool Parameters_t::get(const std::string key, std::string &ret) const
{
	StringParameters_t::const_iterator it = stringParameters.find(key);
	if (it != stringParameters.end()) {
		ret = it->second;
		return true;
	}
	else
		return false;
}

bool Parameters_t::get(const std::string key, int32_t &ret) const
{
	IntParameters_t::const_iterator it = intParameters.find(key);
	if (it != intParameters.end()) {
		ret = it->second;
		return true;
	}
	else
		return false;
}

bool Parameters_t::isEmpty() const
{
	return ((intParameters.empty()) && (stringParameters.empty()));
}

void Parameters_t::pushValueToLua(lua_State *L, const std::string key) const
{
	std::string retS;
	int32_t retI;
	if (this->get(key, retI))
		lua_pushnumber(L, retI);
	else {
		if (this->get(key,retS))
			lua_pushstring(L, retS.c_str());
		else
			lua_pushnil(L);
	}
}

bool Parameters_t::readXMLParameters(const xmlNodePtr node)
{
	xmlNodePtr attributeNode = node->children;
	while(attributeNode) {
		if(xmlStrcmp(attributeNode->name, (const xmlChar*)"parameter") == 0) {
			std::string key;
			if(readXMLString(attributeNode, "key", key)){
				int32_t valuePar;
				if(readXMLInteger(attributeNode, "intValue", valuePar))
					this->set(key, valuePar);
			}
			else {
				std::string strValue;
				if(readXMLString(attributeNode, "value", strValue))
					this->set(key, strValue);
			}
		}
	attributeNode = attributeNode->next;
	}
	return true;
}

bool ParametersVector_t::get(const std::string owner, const std::string key, std::string &val) const
{
	std::map<std::string, Parameters_t*>::const_iterator it = parametersVector.find(owner);
	if (it == parametersVector.end())
		return false;
	return it->second->get(key, val);
}

bool ParametersVector_t::get(const std::string owner, const std::string key, int32_t &val) const
{
	std::map<std::string, Parameters_t*>::const_iterator it = parametersVector.find(owner);
	if (it == parametersVector.end())
		return false;
	return it->second->get(key, val);
}


void ParametersVector_t::set(const std::string owner, const std::string key, const std::string val)
{
	Parameters_t *parameter;
	std::map<std::string, Parameters_t*>::iterator it = parametersVector.find(owner);
	if (it != parametersVector.end())
		parameter = it->second;
	else {
		parameter = new Parameters_t;
		parametersVector[owner] = parameter;
	}
	parameter->set(key, val);
}

void ParametersVector_t::set(const std::string owner, const std::string key, const int32_t val)
{
	Parameters_t *parameter;
	std::map<std::string, Parameters_t*>::iterator it = parametersVector.find(owner);
	if (it != parametersVector.end())
		parameter = it->second;
	else {
		parameter = new Parameters_t;
		parametersVector[owner] = parameter;
	}
	parameter->set(key, val);
}

void ParametersVector_t::reset()
{
	for (std::map<std::string, Parameters_t*>::iterator it = parametersVector.begin(); it != parametersVector.end(); it++) {
		it->second->clear();
		delete it->second;
	}
	parametersVector.clear();
}

bool ParametersVector_t::pushLuaExtraParametersTable(const std::string owner, LuaScriptInterface *env) const
{
	lua_State* L = env->getLuaState();
	std::map<std::string, Parameters_t*>::const_iterator it = parametersVector.find(owner);
	if (it == parametersVector.end()) {
		lua_pushnil(L);
		return true;
	}
	lua_newtable(L);
	IntParameters_t *sI = &(it->second->intParameters);
	IntParameters_t::iterator itI = sI->begin();
	while (itI != sI->end()) {
		std::string s = itI->first;
		const char *key = s.c_str();
		env->setField(L, key, itI->second);
		itI++;
	}
	StringParameters_t *ss = &(it->second->stringParameters);
	StringParameters_t::iterator itS = ss->begin();
	while (itS != ss->end()) {
		std::string s = itS->first;
		const char *key = s.c_str();
		env->setField(L, key, itS->second);
		itS++;
	}
	return true;
}

bool ParametersVector_t::readXMLParameters(const xmlNodePtr node, const std::string ownerName)
{
	Parameters_t *parameter;
	std::map<std::string, Parameters_t*>::iterator it = parametersVector.find(ownerName);
	if (it != parametersVector.end()) {
		parameter = it->second;
	}
	else {
		parameter = new Parameters_t;
		parametersVector[ownerName] = parameter;
	}
	parameter->readXMLParameters(node);
	return true;
}

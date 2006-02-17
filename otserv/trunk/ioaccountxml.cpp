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

#include "ioaccountxml.h"
#include <algorithm>
#include <functional>
#include <sstream>
#include <string.h>
#include "luascript.h"

extern xmlMutexPtr xmlmutex;
extern LuaScript g_config; 

IOAccountXML::IOAccountXML()
{
	if(xmlmutex == NULL){
		xmlmutex = xmlNewMutex();
	}
}

Account IOAccountXML::loadAccount(unsigned long accno)
{
	Account acc;

	std::stringstream accsstr;
	std::string datadir = g_config.getGlobalString("datadir");
	accsstr << datadir + "accounts/" << accno << ".xml";;
	std::string filename = accsstr.str();
	std::transform(filename.begin(), filename.end(), filename.begin(), tolower);
	xmlMutexLock(xmlmutex);
	xmlDocPtr doc = xmlParseFile(filename.c_str());

	if(doc){
		xmlNodePtr root, p, tmp;
		root = xmlDocGetRootElement(doc);

		if(xmlStrcmp(root->name,(const xmlChar*) "account")){
			xmlFreeDoc(doc);			
			xmlMutexUnlock(xmlmutex);
			return acc;
		}

		p = root->children;

		// perhaps verify name
		char* nodeValue = NULL;
		nodeValue = (char*)xmlGetProp(root, (const xmlChar *)"pass");
		acc.password  = nodeValue;
		xmlFreeOTSERV(nodeValue);

		nodeValue = (char*)xmlGetProp(root, (xmlChar*)"type");
		acc.accType  = atoi(nodeValue);
		xmlFreeOTSERV(nodeValue);

		nodeValue = (char*)xmlGetProp(root, (xmlChar*)"premDays");
		acc.premDays  = atoi(nodeValue);
		xmlFreeOTSERV(nodeValue);

		// now load in characters.
		while(p){
			if(xmlStrcmp(p->name, (const xmlChar*) "characters") == 0){
				tmp = p->children;
				while(tmp){
					nodeValue = (char*)xmlGetProp(tmp, (xmlChar*)"name");

					if(nodeValue){
						if(strcmp((const char*)tmp->name, "character") == 0) {
							acc.charList.push_back(std::string(nodeValue));
						}

						xmlFreeOTSERV(nodeValue);
					}

					tmp = tmp->next;
				}
			}
			p = p->next;
		}
		
		xmlFreeDoc(doc);

		// Organize the char list.
		acc.charList.sort();
		acc.accnumber = accno;
	}	

	xmlMutexUnlock(xmlmutex);
	return acc;
}


bool IOAccountXML::getPassword(unsigned long accno, const std::string &name, std::string &password)
{
	std::string acc_password;
	
	std::stringstream accsstr;
	std::string datadir = g_config.getGlobalString("datadir");
	accsstr << datadir + "accounts/" << accno << ".xml";;
	std::string filename = accsstr.str();
	std::transform(filename.begin(), filename.end(), filename.begin(), tolower);
	
	xmlMutexLock(xmlmutex);
	xmlDocPtr doc = xmlParseFile(filename.c_str());
	if(doc){
		xmlNodePtr root, p, tmp;
		root = xmlDocGetRootElement(doc);

		if(xmlStrcmp(root->name,(const xmlChar*) "account")){
			xmlFreeDoc(doc);			
			xmlMutexUnlock(xmlmutex);
			return false;
		}

		p = root->children;

		char* nodeValue = NULL;
		nodeValue = (char*)xmlGetProp(root, (const xmlChar *)"pass");
		acc_password  = nodeValue;
		xmlFreeOTSERV(nodeValue);

		// now load in characters.
		while(p){
			if(xmlStrcmp(p->name, (const xmlChar*) "characters") == 0){
				tmp = p->children;
				while(tmp){
					nodeValue = (char*)xmlGetProp(tmp, (xmlChar*)"name");

					if(nodeValue){
						if(strcmp((const char*)tmp->name, "character") == 0){
							if(nodeValue == name){
								password = acc_password;
								xmlFreeOTSERV(nodeValue);
								xmlFreeDoc(doc);
								xmlMutexUnlock(xmlmutex);
								return true;
							}
						}
						xmlFreeOTSERV(nodeValue);
					}

					tmp = tmp->next;
				}
			}
			p = p->next;
		}
		
		xmlFreeDoc(doc);
	}

	xmlMutexUnlock(xmlmutex);
	return false;
}

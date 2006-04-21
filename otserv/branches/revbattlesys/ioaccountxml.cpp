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
#include "tools.h"

#include <algorithm>
#include <functional>
#include <sstream>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/threads.h>

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
	accsstr << datadir + "accounts/" << accno << ".xml";
	std::string filename = accsstr.str();
	xmlMutexLock(xmlmutex);

	xmlDocPtr doc = xmlParseFile(filename.c_str());
	if(doc){
		xmlNodePtr root, p;
		root = xmlDocGetRootElement(doc);

		if(xmlStrcmp(root->name,(const xmlChar*)"account") != 0){
			xmlFreeDoc(doc);			
			xmlMutexUnlock(xmlmutex);
			return acc;
		}

		p = root->children;

		std::string strValue;
		int intValue;

		if(readXMLString(root, "pass", strValue)){
			acc.password = strValue;
		}

		if(readXMLInteger(root, "type", intValue)){
			acc.accType = intValue;
		}

		if(readXMLInteger(root, "premDays", intValue)){
			acc.premDays = intValue;
		}

		// now load in characters.
		while(p){
			if(xmlStrcmp(p->name, (const xmlChar*)"characters") == 0){
				xmlNodePtr tmpNode = p->children;
				while(tmpNode){
					if(readXMLString(tmpNode, "name", strValue)){
						if(xmlStrcmp(tmpNode->name, (const xmlChar*)"character") == 0){
							acc.charList.push_back(strValue);
						}
					}

					tmpNode = tmpNode->next;
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

bool IOAccountXML::getPassword(unsigned long accno, const std::string& name, std::string& password)
{
	std::string acc_password;
	
	std::stringstream accsstr;
	std::string datadir = g_config.getGlobalString("datadir");
	accsstr << datadir + "accounts/" << accno << ".xml";
	std::string filename = accsstr.str();
	
	xmlMutexLock(xmlmutex);
	xmlDocPtr doc = xmlParseFile(filename.c_str());

	if(doc){
		xmlNodePtr root, p;
		root = xmlDocGetRootElement(doc);

		if(xmlStrcmp(root->name,(const xmlChar*)"account") != 0){
			xmlFreeDoc(doc);			
			xmlMutexUnlock(xmlmutex);
			return false;
		}

		p = root->children;
		
		std::string strValue;
		if(readXMLString(root, "pass", strValue)){
			acc_password = strValue;
		}

		// now load in characters.
		while(p){
			if(xmlStrcmp(p->name, (const xmlChar*)"characters") == 0){
				xmlNodePtr tmpNode = p->children;
				while(tmpNode){
					if(readXMLString(tmpNode, "name", strValue)){
						if(xmlStrcmp(tmpNode->name, (const xmlChar*)"character") == 0){
							if(strValue == name){
								password = acc_password;
								xmlFreeDoc(doc);
								xmlMutexUnlock(xmlmutex);
								return true;
							}
						}
					}
					tmpNode = tmpNode->next;
				}
			}
			p = p->next;
		}
		
		xmlFreeDoc(doc);
	}

	xmlMutexUnlock(xmlmutex);
	return false;
}

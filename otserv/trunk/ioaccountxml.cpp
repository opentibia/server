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

Account IOAccountXML::loadAccount(unsigned long accno){
	Account acc;

	std::stringstream accsstr;
	accsstr << "data/accounts/" << accno << ".xml";;
	std::string filename = accsstr.str();
	std::transform(filename.begin(), filename.end(), filename.begin(), tolower);

	xmlDocPtr doc = xmlParseFile(filename.c_str());

	if (doc)
	{
		xmlNodePtr root, p, tmp;
		root = xmlDocGetRootElement(doc);

		if (xmlStrcmp(root->name,(const xmlChar*) "account"))
		{
			xmlFreeDoc(doc);
			return acc;
		}

		p = root->children;

		// perhaps verify name
		const char* pwd = (const char*)xmlGetProp(root, (const xmlChar *)"pass");

		acc.password  = pwd;

		acc.accType   = atoi((const char*)xmlGetProp(root, (xmlChar*)"type"));
		acc.premDays  = atoi((const char*)xmlGetProp(root, (xmlChar*)"premDays"));


		// now load in characters.
		while (p)
		{
			const char* str = (char*)p->name;

			if (strcmp(str, "characters") == 0)
			{
				tmp = p->children;
				while(tmp)
				{
					const char* temp_a = (const char*)xmlGetProp(tmp, (xmlChar*)"name");

					if ((strcmp((const char*)tmp->name, "character") == 0) && (temp_a != NULL))
						acc.charList.push_back(temp_a);

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
	return acc;
}

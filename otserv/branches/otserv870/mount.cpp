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

#include "mount.h"
#include "tools.h"
#include "player.h"
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <iostream>

Mounts::Mounts()
{
	//
}

Mounts::~Mounts()
{
	//
}

bool Mounts::loadFromXml(const std::string& datadir)
{
	m_datadir = datadir;
	std::string filename = datadir + "mounts.xml";

	xmlDocPtr doc = xmlParseFile(filename.c_str());
	if(doc){
		xmlNodePtr root, p;
		root = xmlDocGetRootElement(doc);

		if(xmlStrcmp(root->name,(const xmlChar*)"mounts") != 0){
			xmlFreeDoc(doc);
			std::cout << "Warning: mounts.xml not found, using defaults." << std::endl;
			return true;
		}

		p = root->children;

		while(p){
			if(xmlStrcmp(p->name, (const xmlChar*)"mount") == 0){
				int32_t intValue;
				std::string strValue;
				if(readXMLInteger(p, "id", intValue)){
					Mount mount;
					mount.mountId = intValue;
					mount.isDefault = true;
					if(readXMLInteger(p, "looktype", intValue) && readXMLString(p, "name", strValue)){
						mount.lookType = intValue;
						mount.name = strValue;
						if(readXMLInteger(p, "premium", intValue)){
							mount.isPremium = (intValue == 1);
						}

						if(readXMLInteger(p, "default", intValue)){
							mount.isDefault = (intValue == 1);
						}

						if(readXMLInteger(p, "speed", intValue)){
							mount.speed = intValue;
						}

						mounts[mount.mountId] = mount;
					}
					else{
						std::cout << "Missing looktype or name for a mount." << std::endl;
						p = p->next;
						continue;
					}
				}
				else{
					std::cout << "Missing mount id." << std::endl;
				}
			}
			p = p->next;
		}
		xmlFreeDoc(doc);
	}
	return true;
}

bool Mounts::reload()
{
	mounts.clear();
	bool result = loadFromXml(m_datadir);
	if(result){
		for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin();
		it != Player::listPlayer.list.end();
		++it)
		{
			it->second->setSex(it->second->getSex());
		}
	}
	return result;
}

bool Mounts::getMount(uint32_t value, Mount& mount, bool isId)
{
	if(isId){
		MountMap::iterator it = mounts.find(value);
		if(it != mounts.end()){
			mount = it->second;
			return true
		}
	}
	else{
		for(MountMap::iterator it = mounts.begin(); it != mounts.end(); it++){
			if(it->second.lookType == value){
				mount = it->second;
				return true;
			}
		}
	}

	return false;
}


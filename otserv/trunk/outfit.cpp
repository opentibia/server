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

#include "definitions.h"
#include "outfit.h"
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include "creature.h"
#include "player.h"

Outfits* Outfits::_instance = NULL;

OutfitList::OutfitList()
{
	//
}

OutfitList::~OutfitList()
{
	OutfitListType::iterator it;
	for(it = m_list.begin(); it != m_list.end(); it++){
		delete *it;
	}
	m_list.clear();
}

void OutfitList::addOutfit(const Outfit& outfit)
{
	Outfit* new_outfit = new Outfit;
	new_outfit->looktype = outfit.looktype;
	new_outfit->addons = outfit.addons;
	m_list.push_back(new_outfit);
}

bool OutfitList::isInList(uint32_t looktype, uint32_t addons) const
{
	OutfitListType::const_iterator it;
	for(it = m_list.begin(); it != m_list.end(); ++it){
		if((*it)->looktype == looktype){
			if(((*it)->addons & addons) == addons){
				return true;
			}
			else{
				return false;
			}
		}
	}
	return false;
}

const OutfitListType& OutfitList::getOutfits() const
{
	return m_list;
}

Outfits::Outfits()
{
	Outfit outfit;
	//build default outfit lists
	outfit.addons = 0;
	for(int i = PLAYER_FEMALE_1; i <= PLAYER_FEMALE_7; i++){
		outfit.looktype = i;
		m_female_list.addOutfit(outfit);
	}
		
	for(int i = PLAYER_MALE_1; i <= PLAYER_MALE_7; i++){
		outfit.looktype = i;
		m_male_list.addOutfit(outfit);
	}
	
	m_list.resize(10, NULL);
}

Outfits::~Outfits()
{
	OutfitsListVector::iterator it;
	for(it = m_list.begin(); it != m_list.end(); it++){
		delete *it;
	}
	m_list.clear();
}
	
Outfits* Outfits::getInstance()
{
	if(!_instance){
		_instance = new Outfits();
	}
	return _instance;
}

const OutfitListType& Outfits::getOutfits(uint32_t type)
{
	return getOutfitList(type).getOutfits();
}

const OutfitList& Outfits::getOutfitList(uint32_t type)
{
	if(type < m_list.size()){
		return *m_list[type];
	}
	else{
		if(type == PLAYERSEX_FEMALE){
			return m_female_list;
		}
		else{
			return m_male_list;
		}
	}
}

bool Outfits::loadFromXml(const std::string& datadir)
{
	std::string filename = datadir + "outfits.xml";

	xmlDocPtr doc = xmlParseFile(filename.c_str());
	if(doc){
		xmlNodePtr root, p;
		root = xmlDocGetRootElement(doc);
		
		if(xmlStrcmp(root->name,(const xmlChar*)"outfits") != 0){
			xmlFreeDoc(doc);
			std::cout << "Warning: outfits.xml not found, using defaults." << std::endl;
			return true;
		}
		
		p = root->children;
		
		while(p){
			std::string str;
			int intVal;
			if(xmlStrcmp(p->name, (const xmlChar*)"outfit") == 0){
				if(readXMLInteger(p, "type", intVal)){
					if(intVal > 9 || intVal < 0){
						std::cout << "Warning: No valid outfit type " << intVal << std::endl;
					}
					else{
						OutfitList* list;
						
						if(m_list[intVal] != NULL){
							list = m_list[intVal];
						}
						else{
							list = new OutfitList;
							m_list[intVal] = list;
						}
						
						Outfit outfit;
						if(readXMLInteger(p, "looktype", intVal)){
							outfit.looktype = intVal;
						}				
						if(readXMLString(p, "addons", str)){
							uint32_t addons_field = 0;
							for(int i = 0; (i < 2) && i < str.size(); ++i){ //allow 2 addons
								if(str[i] == '1'){
									addons_field = addons_field | ( 1 << i);
								}
							}
							outfit.addons = addons_field;
						}
						list->addOutfit(outfit);
					}
				}
				else{
					std::cout << "Missing outfit type." << std::endl;
				}
			}
			p = p->next;
		}
		xmlFreeDoc(doc);
	}
	return true;
}

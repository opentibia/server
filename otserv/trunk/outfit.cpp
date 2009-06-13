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
#include "tools.h"
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

Outfits::Outfits()
{
	//
}

Outfits::~Outfits()
{
	//
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
			int32_t intValue;
			std::string strVal;

			if(xmlStrcmp(p->name, (const xmlChar*)"outfit") == 0){
				if(readXMLInteger(p, "id", intValue)){
					Outfit outfit;
					outfit.outfitId = intValue;

					if(readXMLInteger(p, "premium", intValue)){
						outfit.premium = (intValue == 1);
					}
			
					xmlNodePtr pchild = p->children;
					while(pchild){
						if(xmlStrcmp(pchild->name, (const xmlChar*)"list") == 0){
							if(readXMLInteger(pchild, "looktype", intValue)){
								outfit.lookType = intValue;
							}
							else{
								std::cout << "Missing looktype for an outfit." << std::endl;
								pchild = pchild->next;
								continue;
							}

							if(readXMLInteger(pchild, "addons", intValue)){
								outfit.addons = intValue;
							}

							if(readXMLString(pchild, "name", strVal)){
								outfit.name = strVal;
							}

							if(readXMLInteger(pchild, "type", intValue)){
								if(intValue >= PLAYERSEX_FEMALE || intValue <= PLAYERSEX_OLDMALE){
									playersex_t playersex = (playersex_t)intValue;

									allOutfits.push_back(outfit);

									switch(playersex){
										case PLAYERSEX_FEMALE:
											femaleMap[outfit.outfitId] = outfit;
											break;
										case PLAYERSEX_MALE:
											maleMap[outfit.outfitId] = outfit;
											break;

										default:
											break;
									}
								}
								else{
									std::cout << "Invalid playersex for an outfit." << std::endl;
									pchild = pchild->next;
									continue;
								}
							}
							else{
								std::cout << "Missing playersex for an outfit." << std::endl;
								pchild = pchild->next;
								continue;
							}
						}

						pchild = pchild->next;
					}
				}
				else{
					std::cout << "Missing outfit id." << std::endl;
				}
			}
			p = p->next;
		}
		xmlFreeDoc(doc);
	}
	return true;
}

uint32_t Outfits::getOutfitId(uint32_t lookType)
{
	for(OutfitList::iterator it = allOutfits.begin(); it != allOutfits.end(); ++it){
		if(it->lookType == lookType){
			return it->outfitId;
		}
	}

	return 0;
}

bool Outfits::getOutfit(uint32_t lookType, Outfit& outfit)
{
	for(OutfitList::iterator it = allOutfits.begin(); it != allOutfits.end(); ++it){
		if(it->lookType == lookType){
			outfit = *it;
			return true;
		}
	}

	return false;
}

bool Outfits::getOutfit(uint32_t outfitId, playersex_t sex, Outfit& outfit)
{
	OutfitMap map = getOutfits(sex);
	OutfitMap::iterator it = map.find(outfitId);
	if(it != map.end()){
		outfit = it->second;
		return true;
	}

	return false;
}

const OutfitMap& Outfits::getOutfits(playersex_t playersex)
{
	switch(playersex){
		case PLAYERSEX_FEMALE:
			return femaleMap;

		case PLAYERSEX_MALE:
			return maleMap;

		default:
			static OutfitMap emptyMap;
			return emptyMap;
	}
}

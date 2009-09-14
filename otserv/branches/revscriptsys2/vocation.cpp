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

#include "vocation.h"
#include "tools.h"

Vocations::Vocations()
{
	//
}

Vocations::~Vocations()
{
	for(VocationsMap::iterator it = vocationsMap.begin(); it != vocationsMap.end(); ++it){
		delete it->second;
	}
	vocationsMap.clear();
}

bool Vocations::loadFromXml(const std::string& datadir)
{
	std::string filename = datadir + "vocations.xml";

	xmlDocPtr doc = xmlParseFile(filename.c_str());
	if(doc){
		xmlNodePtr root, p;
		root = xmlDocGetRootElement(doc);

		if(xmlStrcmp(root->name,(const xmlChar*)"vocations") != 0){
			xmlFreeDoc(doc);
			return false;
		}

		p = root->children;

		while(p){
			std::string str;
			int intVal;
			float floatVal;
			if(xmlStrcmp(p->name, (const xmlChar*)"vocation") == 0){
				Vocation* voc = NULL;
				xmlNodePtr skillNode;
				if(readXMLInteger(p, "id", intVal)){
					voc = new Vocation(intVal);
					if(readXMLString(p, "name", str)){
						voc->name = str;
					}
					if(readXMLString(p, "description", str)){
						voc->description = str;
					}
					if(readXMLInteger(p, "gaincap", intVal)){
						voc->gainCap = intVal;
					}
					if(readXMLInteger(p, "gainhp", intVal)){
						voc->gainHP = intVal;
					}
					if(readXMLInteger(p, "gainmana", intVal)){
						voc->gainMana = intVal;
					}
					if(readXMLInteger(p, "gainhpticks", intVal)){
						voc->gainHealthTicks = intVal;
					}
					if(readXMLInteger(p, "gainhpamount", intVal)){
						voc->gainHealthAmount = intVal;
					}
					if(readXMLInteger(p, "gainmanaticks", intVal)){
						voc->gainManaTicks = intVal;
					}
					if(readXMLInteger(p, "gainmanaamount", intVal)){
						voc->gainManaAmount = intVal;
					}
					if(readXMLInteger(p, "maxsoul", intVal)){
						voc->maxSoul = intVal;
					}
					if(readXMLInteger(p, "gainsoulticks", intVal)){
						voc->gainSoulTicks = intVal;
					}
					if(readXMLFloat(p, "manamultiplier", floatVal)){
						voc->manaMultiplier = floatVal;
					}
					skillNode = p->children;
					while(skillNode){
						if(xmlStrcmp(skillNode->name, (const xmlChar*)"skill") == 0){
							SkillType skill_id;
							try {
								if(readXMLInteger(skillNode, "id", intVal)){
									skill_id = SkillType::fromInteger(intVal);
								} else if(readXMLString(skillNode, "name", str)){
									skill_id = SkillType::fromString(str);
								}
								if(readXMLInteger(skillNode, "base", intVal)){
									voc->skillBases[skill_id.value()] = intVal;
								}
								if(readXMLFloat(skillNode, "multiplier", floatVal)){
									voc->skillMultipliers[skill_id.value()] = floatVal;
								}
							} catch(enum_conversion_error&){
								std::cout << "Missing skill id ." << std::endl;
							}
						}
						else if(xmlStrcmp(skillNode->name, (const xmlChar*)"damage") == 0){
							if(readXMLFloat(skillNode, "magicDamage", floatVal)){
								voc->magicBaseDamage = floatVal;
							}
							if(readXMLFloat(skillNode, "wandDamage", floatVal)){
								voc->wandBaseDamage = floatVal;
							}
							if(readXMLFloat(skillNode, "healingDamage", floatVal)){
								voc->healingBaseDamage = floatVal;
							}
						}
						else if(xmlStrcmp(skillNode->name, (const xmlChar*)"meleeDamage") == 0){
							if(readXMLFloat(skillNode, "sword", floatVal)){
								voc->swordBaseDamage = floatVal;
							}
							if(readXMLFloat(skillNode, "axe", floatVal)){
								voc->axeBaseDamage = floatVal;
							}
							if(readXMLFloat(skillNode, "club", floatVal)){
								voc->clubBaseDamage = floatVal;
							}
							if(readXMLFloat(skillNode, "dist", floatVal)){
								voc->distBaseDamage = floatVal;
							}
							if(readXMLFloat(skillNode, "fist", floatVal)){
								voc->fistBaseDamage = floatVal;
							}
						}
						else if(xmlStrcmp(skillNode->name, (const xmlChar*)"defense") == 0){
							if(readXMLFloat(skillNode, "baseDefense", floatVal)){
								voc->baseDefense = floatVal;
							}
							if(readXMLFloat(skillNode, "armorDefense", floatVal)){
								voc->armorDefense = floatVal;
							}
						}
						skillNode = skillNode->next;
					}

					//std::cout << "Voc id: " << voc_id << std::endl;
					//voc->debugVocation();
					vocationsMap[voc->getID()] = voc;

				}
				else{
					std::cout << "Missing vocation id." << std::endl;
				}
			}
			p = p->next;
		}
		xmlFreeDoc(doc);
	}
	return true;
}

Vocation* Vocations::getVocation(uint32_t vocId)
{
	VocationsMap::iterator it = vocationsMap.find(vocId);
	if(it != vocationsMap.end()){
		return it->second;
	}
	else{
		vocationsMap[vocId] = new Vocation(vocId);
		std::cout << "Warning: [Vocations::getVocation] Vocation " << vocId << " not found." << std::endl;
		return vocationsMap[vocId];
	}
}

int32_t Vocations::getVocationId(const std::string& name)
{
	for(VocationsMap::iterator it = vocationsMap.begin(); it != vocationsMap.end(); ++it){
		if(strcasecmp(it->second->name.c_str(), name.c_str()) == 0){
			return it->first;
		}
	}

	return -1;
}

Vocation::Vocation(uint32_t _id)
{
	id = _id;
	name = "none";
	gainHealthTicks = 6;
	gainHealthAmount = 1;
	gainManaTicks = 6;
	gainManaAmount = 1;
	gainCap = 5;
	gainMana = 5;
	gainHP = 5;
	maxSoul = 100;
	gainSoulTicks = 120;
	manaMultiplier = 4.0;
	skillMultipliers[0] = 1.5f;
	skillMultipliers[1] = 2.0f;
	skillMultipliers[2] = 2.0f;
	skillMultipliers[3] = 2.0f;
	skillMultipliers[4] = 2.0f;
	skillMultipliers[5] = 1.5f;
	skillMultipliers[6] = 1.1f;

	skillBases[0] = 50;
	skillBases[1] = 50;
	skillBases[2] = 50;
	skillBases[3] = 50;
	skillBases[4] = 30;
	skillBases[5] = 100;
	skillBases[6] = 20;

	swordBaseDamage = 1.;
	axeBaseDamage = 1.;
	clubBaseDamage = 1.;
	distBaseDamage = 1.;
	fistBaseDamage = 1.;

	magicBaseDamage = 1.;
	wandBaseDamage = 1.;
	healingBaseDamage = 1.;

	baseDefense = 1.;
	armorDefense = 1.;
}

Vocation::~Vocation()
{
	cacheMana.clear();
	for(SkillType::iterator i = SkillType::begin(); i != SkillType::end(); ++i)
		cacheSkill[i->value()].clear();
}

uint32_t Vocation::getReqSkillTries(SkillType skill, int32_t level)
{
	assert(skill.exists());

	cacheMap& skillMap = cacheSkill[skill.value()];
	cacheMap::iterator it = skillMap.find(level);
	if(it != cacheSkill[skill.value()].end())
		return it->second;
	
	uint32_t tries = (uint32_t)(skillBases[skill.value()] * std::pow((float)skillMultipliers[skill.value()], (float)(level - 11)));
	skillMap[level] = tries;
	return tries;
}

uint32_t Vocation::getReqMana(int32_t magLevel)
{
	cacheMap::iterator it = cacheMana.find(magLevel);
	if(it != cacheMana.end()){
		return it->second;
	}

	uint32_t reqMana = (uint32_t)(1600*std::pow(manaMultiplier, magLevel-1));
	cacheMana[magLevel] = reqMana;

	return reqMana;
}

void Vocation::debugVocation()
{
	std::cout << "name: " << name << std::endl;
	std::cout << "gain cap: " << gainCap << " hp: " << gainHP << " mana: " << gainMana << std::endl;
	std::cout << "gain time: Health(" << gainHealthTicks << " ticks, +" << gainHealthAmount << "). Mana(" <<
		gainManaTicks << " ticks, +" << gainManaAmount << ")" << std::endl;
	std::cout << "mana multiplier: " << manaMultiplier << std::endl;
	for(SkillType::iterator i = SkillType::begin(); i != SkillType::end(); ++i){
		std::cout << "Skill id: " << i->toString() << " multiplier: " << skillMultipliers[i->value()] << std::endl;
	}
}

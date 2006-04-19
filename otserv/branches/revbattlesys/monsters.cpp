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

#include "monsters.h"
#include "monster.h"
#include "container.h"
#include "tools.h"
#include "luascript.h"

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

MonsterType::MonsterType()
{
	reset();
}

void MonsterType::reset()
{
	armor = 0;
	experience = 0;
	defense = 0;
	hasDistanceAttack = false;
	canPushItems = false;
	staticLook = 1;
	staticAttack = 1;
	changeTargetChance = 1;
	maxSummons = 0;
	targetDistance = 1;
	runAwayHealth = 0;
	pushable = true;
	base_speed = 200;
	level = 1;
	magLevel = 1;
	
	health = 100;
	health_max = 100;
	lookhead = 10;
	lookbody = 10;
	looklegs = 10;
	lookfeet = 10;
	looktype = 10;
	lookcorpse = 1000;
	lookmaster = 10;
	immunities = 0;
	lightLevel = 0;
	lightColor = 0;
	
	/*
	for(std::map<PhysicalAttackClass*, TimeProbabilityClass>::iterator it = physicalAttacks.begin(); it != physicalAttacks.end(); ++it) {
		delete it->first;
	}
	physicalAttacks.clear();
	instantSpells.clear();
	runeSpells.clear();

	yellingSentences.clear();
	*/

	summonSpells.clear();
	lootItems.clear();
}

MonsterType::~MonsterType()
{
	/*
	for(std::map<PhysicalAttackClass*, TimeProbabilityClass>::iterator it = physicalAttacks.begin(); it != physicalAttacks.end(); ++it) {
		delete it->first;
	}

	physicalAttacks.clear();
	*/
}

unsigned long Monsters::getRandom()
{
	return (unsigned long)((rand()<< 16 | rand()) % CHANCE_MAX);
}

void MonsterType::createLoot(Container* corpse)
{
	LootItems::const_iterator it;
	for(it = lootItems.begin(); it != lootItems.end(); it++){
		Item* tmpItem = createLootItem(*it);
		if(tmpItem){
			//check containers
			if(Container* container = dynamic_cast<Container*>(tmpItem)){
				createLootContainer(container, *it);
				if(container->size() == 0){
					delete container;
				}
				else{
					corpse->__internalAddThing(tmpItem);
				}
			}
			else{
				corpse->__internalAddThing(tmpItem);
			}
		}
	}
}

Item* MonsterType::createLootItem(const LootBlock& lootBlock)
{
	Item* tmpItem = NULL;
	if(Item::items[lootBlock.id].stackable == true){
		unsigned long randvalue = Monsters::getRandom();
		unsigned long n = 1;
		if(randvalue < lootBlock.chance1){
			if(randvalue < lootBlock.chancemax){
				n = lootBlock.countmax;
			}
			else{
				//if chancemax < randvalue < chance1
				n = (unsigned char)(randvalue % lootBlock.countmax + 1);
			}		
			tmpItem = Item::CreateItem(lootBlock.id,n);
		}
	}
	else{
		if(Monsters::getRandom() < lootBlock.chance1){
			tmpItem = Item::CreateItem(lootBlock.id);
		}
	}
	return tmpItem;
}

void MonsterType::createLootContainer(Container* parent, const LootBlock& lootblock)
{
	LootItems::const_iterator it;
	for(it = lootblock.childLoot.begin(); it != lootblock.childLoot.end(); it++){
		Item* tmpItem = createLootItem(*it);
		if(tmpItem){
			if(Container* container = dynamic_cast<Container*>(tmpItem)){
				createLootContainer(container, *it);
				if(container->size() == 0){
					delete container;
				}
				else{
					parent->__internalAddThing(dynamic_cast<Thing*>(container));
				}
			}
			else{
				parent->__internalAddThing(tmpItem);
			}
		}
	}
}

Monsters::Monsters()
{
	loaded = false;
}

bool Monsters::loadFromXml(const std::string &_datadir,bool reloading /*= false*/)
{	
	loaded = false;	
	datadir = _datadir;
	
	std::string filename = datadir + "monster/monsters.xml";

	xmlDocPtr doc = xmlParseFile(filename.c_str());
	if(doc){
		loaded = true;
		xmlNodePtr root, p;
		unsigned long id = 0;
		root = xmlDocGetRootElement(doc);

		if(xmlStrcmp(root->name,(const xmlChar*)"monsters") != 0){
			xmlFreeDoc(doc);
			loaded = false;
			return false;
		}

		p = root->children;
		while(p){
			if(xmlStrcmp(p->name, (const xmlChar*)"monster") == 0){
				std::string file;
				std::string name;

				if(readXMLString(p, "file", file) && readXMLString(p, "name", name)){
					file = datadir + "monster/" + file;
					
					std::string lowername = name;
					toLowerCaseString(lowername);
						
					MonsterType* mType = loadMonster(file, name, reloading);
					if(mType){
						id++;
						monsterNames[lowername] = id;
						monsters[id] = mType;
					}
				}
			}
			p = p->next;
		}
		
		xmlFreeDoc(doc);
	}

	return loaded;
}

bool Monsters::reload()
{
	return loadFromXml(datadir, true);
}

MonsterType* Monsters::loadMonster(const std::string& file,const std::string& monster_name, bool reloading /*= false*/)
{
	bool monsterLoad;
	MonsterType* mType;
	bool new_mType = true;
	
	if(reloading){
		unsigned long id = getIdByName(monster_name);
		if(id != 0){
			mType = getMonsterType(id);
			if(mType != NULL){
				new_mType = false;
				mType->reset();
			}
		}
	}
	if(new_mType){
		mType = new MonsterType;
	}
	
	monsterLoad = true;
	xmlDocPtr doc = xmlParseFile(file.c_str());

	if(doc){
		xmlNodePtr root, p;
		root = xmlDocGetRootElement(doc);

		if(xmlStrcmp(root->name,(const xmlChar*)"monster") != 0){
			std::cerr << "Malformed XML: " << file << std::endl;
		}

		int intValue;
		std::string strValue;

		p = root->children;

		if(readXMLString(root, "name", strValue)){
			mType->name = strValue;
		}
		else
			monsterLoad = false;

		if(readXMLString(root, "nameDescription", strValue)){
			mType->nameDescription = strValue;
		}
		else{
			mType->nameDescription = "a " + mType->name;
			toLowerCaseString(mType->nameDescription);
		}

		if(readXMLInteger(root, "experience", intValue)){
			mType->experience = intValue;
		}

		if(readXMLInteger(root, "pushable", intValue)){
			mType->pushable = (intValue != 0);
		}

		if(readXMLInteger(root, "level", intValue)){
			mType->level = intValue;
		}

		if(readXMLInteger(root, "speed", intValue)){
			mType->base_speed = intValue;
		}

		if(readXMLInteger(root, "magLevel", intValue)){
			mType->magLevel = intValue;
		}

		if(readXMLInteger(root, "defense", intValue)){
			mType->defense = intValue;
		}

		if(readXMLInteger(root, "armor", intValue)){
			mType->armor = intValue;
		}

		if(readXMLInteger(root, "canpushitems", intValue)){
			mType->canPushItems = (intValue != 0);
		}

		if(readXMLInteger(root, "staticattack", intValue)){
			if(intValue == 0)
				mType->staticAttack = 1;
			else if(intValue >= RAND_MAX)
				mType->staticAttack = RAND_MAX;
			else
				mType->staticAttack = intValue;
		}

		if(readXMLInteger(root, "changetarget", intValue)){
			//0	never, 10000 always
			mType->changeTargetChance = intValue;
		}

		if(readXMLInteger(root, "lightlevel", intValue)){
			mType->lightLevel = intValue;
		}

		if(readXMLInteger(root, "lightcolor", intValue)){
			mType->lightColor = intValue;
		}

		while(p){
			if(xmlStrcmp(p->name, (const xmlChar*)"health") == 0){

				if(readXMLInteger(p, "now", intValue)){
					mType->health = intValue;
				}
				else
					monsterLoad = false;

				if(readXMLInteger(p, "max", intValue)){
					mType->health_max = intValue;
				}
				else
					monsterLoad = false;
			}
			else if(xmlStrcmp(p->name, (const xmlChar*)"combat") == 0){

				if(readXMLInteger(p, "targetdistance", intValue)){
					mType->targetDistance = std::max(1, intValue);
				}

				if(readXMLInteger(p, "runonhealth", intValue)){
					mType->runAwayHealth = intValue;
				}
			}
			else if(xmlStrcmp(p->name, (const xmlChar*)"look") == 0){

				if(readXMLInteger(p, "type", intValue)){
					mType->looktype = intValue;
					mType->lookmaster = mType->looktype;
				}

				if(readXMLInteger(p, "head", intValue)){
					mType->lookhead = intValue;
				}

				if(readXMLInteger(p, "body", intValue)){
					mType->lookbody = intValue;
				}

				if(readXMLInteger(p, "legs", intValue)){
					mType->looklegs = intValue;
				}

				if(readXMLInteger(p, "feet", intValue)){
					mType->lookfeet = intValue;
				}

				if(readXMLInteger(p, "corpse", intValue)){
					mType->lookcorpse = intValue;
				}
			}
			/*else if(xmlStrcmp(p->name, (const xmlChar*)"attacks") == 0){
				xmlNodePtr tmpNode = p->children;
				while(tmpNode){
					if(xmlStrcmp(tmpNode->name, (const xmlChar*)"attack") == 0){
						int cycleTicks = -1;
						int probability = -1;
						int exhaustionTicks = -1;

						if(readXMLInteger(tmpNode, "exhaustion", intValue)){
							exhaustionTicks = intValue;
						}

						if(readXMLInteger(tmpNode, "cycleticks", intValue)){
							cycleTicks = intValue;
						}

						if(readXMLInteger(tmpNode, "probability", intValue)){
							probability = intValue;
						}

						TimeProbabilityClass timeprobsystem(cycleTicks, probability, exhaustionTicks);

						std::string	attacktype = "";
						if(readXMLString(tmpNode, "type", strValue)){
							attacktype = strValue;
						}

						if(strcmp(attacktype.c_str(), "melee") == 0){
							PhysicalAttackClass* physicalattack = new PhysicalAttackClass();

							physicalattack->fighttype = FIGHT_MELEE;

							if(readXMLInteger(tmpNode, "mindamage", intValue)){
								physicalattack->minWeapondamage = intValue;
							}

							if(readXMLInteger(tmpNode, "maxdamage", intValue)){
								physicalattack->maxWeapondamage = intValue;
							}

							mType->physicalAttacks[physicalattack] = TimeProbabilityClass(cycleTicks, probability, exhaustionTicks);
						}
						else if(strcmp(attacktype.c_str(), "distance") == 0){
							mType->hasDistanceAttack = true;
							PhysicalAttackClass* physicalattack = new PhysicalAttackClass();

							physicalattack->fighttype = FIGHT_DIST;
							std::string subattacktype = "";

							if(readXMLString(tmpNode, "name", strValue)){
								subattacktype = strValue;
							}

							if(strcmp(subattacktype.c_str(), "bolt") == 0)
								physicalattack->disttype = DIST_BOLT;
							else if(strcmp(subattacktype.c_str(), "arrow") == 0)
								physicalattack->disttype = DIST_ARROW;
							else if(strcmp(subattacktype.c_str(), "throwingstar") == 0)
								physicalattack->disttype = DIST_THROWINGSTAR;
							else if(strcmp(subattacktype.c_str(), "throwingknife") == 0)
								physicalattack->disttype = DIST_THROWINGKNIFE;
							else if(strcmp(subattacktype.c_str(), "smallstone") == 0)
								physicalattack->disttype = DIST_SMALLSTONE;
							else if(strcmp(subattacktype.c_str(), "largerock") == 0)
								physicalattack->disttype = DIST_LARGEROCK;
							else if(strcmp(subattacktype.c_str(), "snowball") == 0)
								physicalattack->disttype = DIST_SNOWBALL;
							else if(strcmp(subattacktype.c_str(), "powerbolt") == 0)
								physicalattack->disttype = DIST_POWERBOLT;
							else if(strcmp(subattacktype.c_str(), "poisonfield") == 0)
								physicalattack->disttype = DIST_POISONFIELD;

							if(readXMLInteger(tmpNode, "mindamage", intValue)){
								physicalattack->minWeapondamage = intValue;
							}

							if(readXMLInteger(tmpNode, "maxdamage", intValue)){
								physicalattack->maxWeapondamage = intValue;
							}

							mType->physicalAttacks[physicalattack] = TimeProbabilityClass(cycleTicks, probability, exhaustionTicks);
						}
						else if(strcmp(attacktype.c_str(), "instant") == 0) {
							mType->hasDistanceAttack = true;
							std::string spellname = "";

							if(readXMLString(tmpNode, "name", strValue)){
								spellname = strValue;
							}

							if(spells.getAllSpells()->find(spellname) != spells.getAllSpells()->end()){
								mType->instantSpells[spellname].push_back(timeprobsystem);
							}
						}
						else if(strcmp(attacktype.c_str(), "rune") == 0) {
							mType->hasDistanceAttack = true;
							std::string spellname = "";

							if(readXMLString(tmpNode, "name", strValue)){
								spellname = strValue;
							}

							std::transform(spellname.begin(), spellname.end(), spellname.begin(), tolower);

							std::map<unsigned short, Spell*>::const_iterator rsIt;
							for(rsIt = spells.getAllRuneSpells()->begin(); rsIt != spells.getAllRuneSpells()->end(); ++rsIt) {
								if(strcmp(rsIt->second->getName().c_str(), spellname.c_str()) == 0) {
									mType->runeSpells[rsIt->first].push_back(timeprobsystem);
									break;
								}
							}
						}
					}

					tmpNode = tmpNode->next;
				}
			}
			else if(xmlStrcmp(p->name, (const xmlChar*)"defenses") == 0){
				xmlNodePtr tmpNode = p->children;
				while(tmpNode){
					if(xmlStrcmp(tmpNode->name, (const xmlChar*)"defense") == 0){
						std::string immunity = "";

						if(readXMLString(tmpNode, "immunity", strValue)){
							immunity = strValue;
						}

						if(strcmp(immunity.c_str(), "energy") == 0)
							mType->immunities |= ATTACK_ENERGY;
						else if(strcmp(immunity.c_str(), "burst") == 0)
							mType->immunities |= ATTACK_BURST;
						else if(strcmp(immunity.c_str(), "fire") == 0)
							mType->immunities |= ATTACK_FIRE;
						else if(strcmp(immunity.c_str(), "physical") == 0)
							mType->immunities |= ATTACK_PHYSICAL;
						else if(strcmp(immunity.c_str(), "poison") == 0)
							mType->immunities |= ATTACK_POISON;
						else if(strcmp(immunity.c_str(), "paralyze") == 0)
							mType->immunities |= ATTACK_PARALYZE;
						else if(strcmp(immunity.c_str(), "drunk") == 0)
							mType->immunities |= ATTACK_DRUNKNESS;
					}

					tmpNode = tmpNode->next;
				}
			}
			else if(xmlStrcmp(p->name, (const xmlChar*)"voices") == 0){
				xmlNodePtr tmpNode = p->children;
				while(tmpNode){
					if(xmlStrcmp(tmpNode->name, (const xmlChar*)"voice") == 0) {
						int cycleTicks, probability, exhaustionTicks;

						if(readXMLInteger(tmpNode, "exhaustion", intValue)){
							exhaustionTicks = intValue;
						}
						else
							exhaustionTicks = 0;

						if(readXMLInteger(tmpNode, "cycleticks", intValue)){
							cycleTicks = intValue;
						}
						else
							cycleTicks = 30000;

						if(readXMLInteger(tmpNode, "probability", intValue)){
							probability = intValue;
						}
						else
							probability = 30;

						std::string sentence = "";
						if(readXMLString(tmpNode, "sentence", strValue)){
							sentence = strValue;
						}

						if(sentence.length() > 0) {
							mType->yellingSentences.push_back(make_pair(sentence, TimeProbabilityClass(cycleTicks, probability, exhaustionTicks)));
						}
					}

					tmpNode = tmpNode->next;
				}
			}*/
			else if(xmlStrcmp(p->name, (const xmlChar*)"loot") == 0){
				xmlNodePtr tmpNode = p->children;
				while(tmpNode){
					LootBlock lootBlock;
					if(loadLootItem(tmpNode, lootBlock)){
						mType->lootItems.push_back(lootBlock);
					}

					tmpNode = tmpNode->next;
				}
			}
			else if(xmlStrcmp(p->name, (const xmlChar*)"summons") == 0){

				if(readXMLInteger(p, "maxSummons", intValue)){
					mType->maxSummons = std::min(intValue, 100);
				}

				xmlNodePtr tmpNode = p->children;
				while(tmpNode){

					if(xmlStrcmp(tmpNode->name, (const xmlChar*)"summon") == 0){
						summonBlock sb;
						sb.name = "";
						sb.summonChance = CHANCE_MAX;

						if(readXMLString(tmpNode, "name", strValue)){
							sb.name = strValue;
						}
						else
							continue;

						if(readXMLInteger(tmpNode, "chance", intValue)){
							sb.summonChance = std::max(intValue, 100);
							if(sb.summonChance > CHANCE_MAX)
								sb.summonChance = CHANCE_MAX;
						}

						mType->summonSpells.push_back(sb);
					}

					tmpNode = tmpNode->next;
				}
			}
			p = p->next;
		}

		xmlFreeDoc(doc);
	}
	else{
		monsterLoad = false;
	}

	if(monsterLoad){
		return mType;
	}
	else{
		delete mType;
		return NULL;
	}
}

bool Monsters::loadLootItem(xmlNodePtr node, LootBlock& lootBlock)
{
	int intValue;
	if(readXMLInteger(node, "id", intValue)){
		lootBlock.id = intValue;
	}
	
	if(lootBlock.id == 0){
		return false;
	}
	
	if(Item::items[lootBlock.id].stackable == true){
		if(readXMLInteger(node, "countmax", intValue)){
			lootBlock.countmax = intValue;

			if(lootBlock.countmax > 100){
				lootBlock.countmax = 100;
			}
		}
		else{
			std::cout << "missing countmax for loot id = "<< lootBlock.id << std::endl;
			lootBlock.countmax = 1;
		}
			
		if(readXMLInteger(node, "chancemax", intValue)){
			lootBlock.chancemax = intValue;

			if(lootBlock.chancemax > CHANCE_MAX){
				lootBlock.chancemax = 0;
			}
		}
		else{
			std::cout << "missing chancemax for loot id = "<< lootBlock.id << std::endl;
			lootBlock.chancemax = 0;
		}

		if(readXMLInteger(node, "chance1", intValue)){
			lootBlock.chance1 = intValue;

			if(lootBlock.chance1 > CHANCE_MAX){
				lootBlock.chance1 = CHANCE_MAX;
			}
			
			if(lootBlock.chance1 <= lootBlock.chancemax){
				std::cout << "Wrong chance for loot id = "<< lootBlock.id << std::endl;
				return false;
			}
		}
		else{
			std::cout << "missing chance1 for loot id = "<< lootBlock.id << std::endl;
			lootBlock.chance1 = CHANCE_MAX;
		}
	}
	else{
		if(readXMLInteger(node, "chance", intValue)){
			lootBlock.chance1 = intValue;

			if(lootBlock.chance1 > CHANCE_MAX){
				lootBlock.chance1 = CHANCE_MAX;
			}
		}
		else{
			std::cout << "missing chance for loot id = "<< lootBlock.id << std::endl;
			lootBlock.chance1 = CHANCE_MAX;
		}
	}

	if(Item::items[lootBlock.id].isContainer()){
		loadLootContainer(node, lootBlock);
	}
	
	return true;
}

bool Monsters::loadLootContainer(xmlNodePtr node, LootBlock& lBlock)
{
	if(node == NULL){
		return false;
	}
	
	xmlNodePtr tmpNode = node->children;
	xmlNodePtr p;

	if(tmpNode == NULL){
		return false;
	}

	while(tmpNode){
		if(xmlStrcmp(tmpNode->name, (const xmlChar*)"inside") == 0){
			p = tmpNode->children;
			while(p){
				LootBlock lootBlock;
				if(loadLootItem(p, lootBlock)){
					lBlock.childLoot.push_back(lootBlock);
				}
				p = p->next;
			}
			return true;
		}//inside

		tmpNode = tmpNode->next;
	}

	return false;	
}

MonsterType* Monsters::getMonsterType(unsigned long mid)
{
	MonsterMap::iterator it = monsters.find(mid);
	if(it != monsters.end()){
		return it->second;
	}
	else{
		return NULL;
	}
}

unsigned long Monsters::getIdByName(const std::string& name)
{
	std::string lower_name = name;
	std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), tolower);
	MonsterNameMap::iterator it = monsterNames.find(lower_name);
	if(it != monsterNames.end()){
		return it->second;
	}
	else{
		return 0;
	}
}

Monsters::~Monsters()
{
	for(MonsterMap::iterator it = monsters.begin(); it != monsters.end(); it++)
		delete it->second;
}

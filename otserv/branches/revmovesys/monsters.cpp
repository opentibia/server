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
#include "spells.h"
#include "luascript.h"

extern Spells spells;

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
	maglevel = 1;
	
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
	
	for(std::map<PhysicalAttackClass*, TimeProbabilityClass>::iterator it = physicalAttacks.begin(); it != physicalAttacks.end(); ++it) {
		delete it->first;
	}
	physicalAttacks.clear();
	instantSpells.clear();
	runeSpells.clear();
	yellingSentences.clear();
	summonSpells.clear();
	lootItems.clear();
}

MonsterType::~MonsterType()
{
	for(std::map<PhysicalAttackClass*, TimeProbabilityClass>::iterator it = physicalAttacks.begin(); it != physicalAttacks.end(); ++it) {
		delete it->first;
	}
	physicalAttacks.clear();
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
					corpse->addItem(container);
				}
			}
			else{
				corpse->addItem(tmpItem);
			}
		}
	}
}

Item* MonsterType::createLootItem(const LootBlock& lootBlock)
{
	Item* tmpItem = NULL;
	if(Item::items[lootBlock.id].stackable == true){
		unsigned long randvalue = Monster::getRandom();
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
		if(Monster::getRandom() < lootBlock.chance1){
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
					parent->addItem(container);
				}
			}
			else{
				parent->addItem(tmpItem);
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
	this->loaded = false;
	
	datadir = _datadir;
	
	std::string filename = datadir + "monster/monsters.xml";
	std::transform(filename.begin(), filename.end(), filename.begin(), tolower);
	xmlDocPtr doc = xmlParseFile(filename.c_str());
	
	if(doc){
		this->loaded = true;
		xmlNodePtr root, p;
		unsigned long id = 0;
		root = xmlDocGetRootElement(doc);

		if(xmlStrcmp(root->name,(const xmlChar*) "monsters")){
			xmlFreeDoc(doc);
			this->loaded = false;
			return false;
		}
		p = root->children;
        
		while (p){
			const char* str = (char*)p->name;
			if(strcmp(str, "monster") == 0){
				char* monsterfile = (char*)xmlGetProp(p, (const xmlChar *)"file");
				char* name = (char*)xmlGetProp(p, (const xmlChar *)"name");
								
				if(monsterfile && name){
					std::string file = datadir + "monster/" + monsterfile;
					std::transform(file.begin(), file.end(), file.begin(), tolower);
					std::string monster_name = name;
						std::transform(monster_name.begin(), monster_name.end(), monster_name.begin(), tolower);
						
					MonsterType* mType = loadMonster(file,name,reloading);
					if(mType){
						id++;
						monsterNames[monster_name] = id;
						monsters[id] = mType;
					}
				}
			}
			p = p->next;
		}
		
		xmlFreeDoc(doc);
	}
	return this->loaded;
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
		xmlNodePtr root, p, tmp;
		char* nodeValue = NULL;
		root = xmlDocGetRootElement(doc);

		if(xmlStrcmp(root->name,(const xmlChar*) "monster")){
			std::cerr << "Malformed XML: " << file << std::endl;
		}

		p = root->children;

		nodeValue = (char*)xmlGetProp(root, (const xmlChar *)"name");
		if(nodeValue){
			mType->name = nodeValue;
			xmlFreeOTSERV(nodeValue);
		}
		else
			monsterLoad = false;

		nodeValue = (char*)xmlGetProp(root, (const xmlChar *)"experience");
		if(nodeValue){
			mType->experience = atoi(nodeValue);
			xmlFreeOTSERV(nodeValue);
		}

		nodeValue = (char*)xmlGetProp(root, (const xmlChar *)"pushable");
		if(nodeValue){
			mType->pushable = (bool)atoi(nodeValue);
			xmlFreeOTSERV(nodeValue);
		}

		nodeValue = (char*)xmlGetProp(root, (const xmlChar *)"level");
		if(nodeValue){
			mType->level = atoi(nodeValue);
			xmlFreeOTSERV(nodeValue);
		}

		nodeValue = (char*)xmlGetProp(root, (const xmlChar *)"speed");
		if(nodeValue){
			mType->base_speed = atoi(nodeValue);
			xmlFreeOTSERV(nodeValue);
		}

		nodeValue = (char*)xmlGetProp(root, (const xmlChar *)"maglevel");
		if(nodeValue){
			mType->maglevel = atoi(nodeValue);
			xmlFreeOTSERV(nodeValue);
		}

		nodeValue = (char*)xmlGetProp(root, (const xmlChar *)"defense");
		if(nodeValue){
			mType->defense = atoi(nodeValue);
			xmlFreeOTSERV(nodeValue);
		}

		nodeValue = (char*)xmlGetProp(root, (const xmlChar *)"armor");
		if(nodeValue){
			mType->armor = atoi(nodeValue);
			xmlFreeOTSERV(nodeValue);
		}

		nodeValue = (char*)xmlGetProp(root, (const xmlChar *)"canpushitems");
		if(nodeValue){
			mType->canPushItems = (bool)atoi(nodeValue);
			xmlFreeOTSERV(nodeValue);
		}

		nodeValue = (char*)xmlGetProp(root, (const xmlChar *)"staticattack");
		if(nodeValue){
			mType->staticAttack = atoi(nodeValue);
			xmlFreeOTSERV(nodeValue);

			if(mType->staticAttack == 0)
				mType->staticAttack = 1;
			else if(mType->staticAttack >= RAND_MAX)
				mType->staticAttack = RAND_MAX;
		}

		nodeValue = (char*)xmlGetProp(root, (const xmlChar *)"changetarget"); //0	never, 10000 always
		if(nodeValue){
			mType->changeTargetChance = atoi(nodeValue);
			xmlFreeOTSERV(nodeValue);
		}

		while(p){
			const char* str = (char*)p->name;

			if(strcmp(str, "health") == 0) {
				nodeValue = (char*)xmlGetProp(p, (const xmlChar *)"now");
				if(nodeValue){
					mType->health = atoi(nodeValue);
					xmlFreeOTSERV(nodeValue);
				}
				else
					monsterLoad = false;

				nodeValue = (char*)xmlGetProp(p, (const xmlChar *)"max");
				if(nodeValue){
					mType->health_max = atoi(nodeValue);
					xmlFreeOTSERV(nodeValue);
				}
				else
					monsterLoad = false;
			}
			if(strcmp(str, "combat") == 0){
				nodeValue = (char*)xmlGetProp(p, (const xmlChar *)"targetdistance");
				if(nodeValue){
					mType->targetDistance = std::max(1, atoi(nodeValue));
					xmlFreeOTSERV(nodeValue);
				}

				nodeValue = (char*)xmlGetProp(p, (const xmlChar *)"runonhealth");
				if(nodeValue){
					mType->runAwayHealth = atoi(nodeValue);
					xmlFreeOTSERV(nodeValue);
				}
			}
			else if (strcmp(str, "look") == 0) {
				nodeValue = (char*)xmlGetProp(p, (const xmlChar *)"type");
				if(nodeValue) {
					mType->looktype = atoi(nodeValue);
					mType->lookmaster = mType->looktype;
					xmlFreeOTSERV(nodeValue);
				}

				nodeValue = (char*)xmlGetProp(p, (const xmlChar *)"head");
				if(nodeValue) {
					mType->lookhead = atoi(nodeValue);
					xmlFreeOTSERV(nodeValue);
				}

				nodeValue = (char*)xmlGetProp(p, (const xmlChar *)"body");
				if(nodeValue) {
					mType->lookbody = atoi(nodeValue);
					xmlFreeOTSERV(nodeValue);
				}

				nodeValue = (char*)xmlGetProp(p, (const xmlChar *)"legs");
				if(nodeValue) {
					mType->looklegs = atoi(nodeValue);
					xmlFreeOTSERV(nodeValue);
				}

				nodeValue = (char*)xmlGetProp(p, (const xmlChar *)"feet");
				if(nodeValue) {
					mType->lookfeet = atoi(nodeValue);
					xmlFreeOTSERV(nodeValue);
				}

				nodeValue = (char*)xmlGetProp(p, (const xmlChar *)"corpse");
				if(nodeValue) {
					mType->lookcorpse = atoi(nodeValue);
					xmlFreeOTSERV(nodeValue);
				}
			}
			else if (strcmp(str, "attacks") == 0){
				tmp=p->children;
				while(tmp){
					if(strcmp((const char*)tmp->name, "attack") == 0){
						int cycleTicks = -1;
						int probability = -1;
						int exhaustionTicks = -1;

						nodeValue = (char*)xmlGetProp(tmp, (const xmlChar *)"exhaustion");
						if(nodeValue) {
							exhaustionTicks = atoi(nodeValue);
							xmlFreeOTSERV(nodeValue);
						}

						nodeValue = (char*)xmlGetProp(tmp, (const xmlChar *)"cycleticks");
						if(nodeValue){
							cycleTicks = atoi(nodeValue);
							xmlFreeOTSERV(nodeValue);
						}

						nodeValue = (char*)xmlGetProp(tmp, (const xmlChar *)"probability");
						if(nodeValue){
							probability = atoi(nodeValue);
							xmlFreeOTSERV(nodeValue);
						}

						TimeProbabilityClass timeprobsystem(cycleTicks, probability, exhaustionTicks);

						nodeValue = (char*)xmlGetProp(tmp, (const xmlChar *)"type");
						std::string	attacktype = "";
						if(nodeValue){
							attacktype = nodeValue;
							xmlFreeOTSERV(nodeValue);
						}

						if(strcmp(attacktype.c_str(), "melee") == 0){
							PhysicalAttackClass* physicalattack = new PhysicalAttackClass();

							physicalattack->fighttype = FIGHT_MELEE;

							nodeValue = (char*)xmlGetProp(tmp, (const xmlChar *)"mindamage");
							if(nodeValue) {
								physicalattack->minWeapondamage = atoi(nodeValue);
								xmlFreeOTSERV(nodeValue);
							}

							nodeValue = (char*)xmlGetProp(tmp, (const xmlChar *)"maxdamage");
							if(nodeValue) {
								physicalattack->maxWeapondamage = atoi(nodeValue);
								xmlFreeOTSERV(nodeValue);
							}

							mType->physicalAttacks[physicalattack] = TimeProbabilityClass(cycleTicks, probability, exhaustionTicks);
						}
						else if(strcmp(attacktype.c_str(), "distance") == 0){
							mType->hasDistanceAttack = true;
							PhysicalAttackClass* physicalattack = new PhysicalAttackClass();

							physicalattack->fighttype = FIGHT_DIST;
							std::string subattacktype = "";

							nodeValue = (char*)xmlGetProp(tmp, (const xmlChar *)"name");
							if(nodeValue) {
								subattacktype = nodeValue;
								xmlFreeOTSERV(nodeValue);
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

							nodeValue = (char*)xmlGetProp(tmp, (const xmlChar *)"mindamage");
							if(nodeValue) {
								physicalattack->minWeapondamage = atoi(nodeValue);
								xmlFreeOTSERV(nodeValue);
							}

							nodeValue = (char*)xmlGetProp(tmp, (const xmlChar *)"maxdamage");
							if(nodeValue) {
								physicalattack->maxWeapondamage = atoi(nodeValue);
								xmlFreeOTSERV(nodeValue);
							}

							mType->physicalAttacks[physicalattack] = TimeProbabilityClass(cycleTicks, probability, exhaustionTicks);
						}
						else if(strcmp(attacktype.c_str(), "instant") == 0) {
							mType->hasDistanceAttack = true;
							std::string spellname = "";

							nodeValue = (char*)xmlGetProp(tmp, (const xmlChar *)"name");
							if(nodeValue) {
								spellname = nodeValue;
								xmlFreeOTSERV(nodeValue);
							}

							if(spells.getAllSpells()->find(spellname) != spells.getAllSpells()->end()){
								mType->instantSpells[spellname].push_back(timeprobsystem);
							}
						}
						else if(strcmp(attacktype.c_str(), "rune") == 0) {
							mType->hasDistanceAttack = true;
							std::string spellname = "";

							nodeValue = (char*)xmlGetProp(tmp, (const xmlChar *)"name");
							if(nodeValue) {
								spellname = nodeValue;
								xmlFreeOTSERV(nodeValue);
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

					tmp = tmp->next;
				}
			}
			else if (strcmp(str, "defenses") == 0){
				tmp = p->children;
				while(tmp){
					if (strcmp((const char*)tmp->name, "defense") == 0) {
						std::string immunity = "";

						nodeValue = (char*)xmlGetProp(tmp, (const xmlChar *)"immunity");
						if(nodeValue) {
							immunity = nodeValue;
							xmlFreeOTSERV(nodeValue);
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

					tmp = tmp->next;
				}
			}
			else if (strcmp(str, "voices") == 0){
				tmp = p->children;
				while(tmp){
					if (strcmp((const char*)tmp->name, "voice") == 0) {
						int cycleTicks, probability, exhaustionTicks;

						nodeValue = (char*)xmlGetProp(tmp, (const xmlChar *)"exhaustion");
						if(nodeValue) {
							exhaustionTicks = atoi(nodeValue);
							xmlFreeOTSERV(nodeValue);
						}
						else
							exhaustionTicks = 0;

						nodeValue = (char*)xmlGetProp(tmp, (const xmlChar *)"cycleticks");
						if(nodeValue) {
							cycleTicks = atoi(nodeValue);
							xmlFreeOTSERV(nodeValue);
						}
						else
							cycleTicks = 30000;

						nodeValue = (char*)xmlGetProp(tmp, (const xmlChar *)"probability");
						if(nodeValue) {
							probability = atoi(nodeValue);
							xmlFreeOTSERV(nodeValue);
						}
						else
							probability = 30;

						std::string sentence = "";
						nodeValue = (char*)xmlGetProp(tmp, (const xmlChar *)"sentence");
						if(nodeValue) {
							sentence = nodeValue;
							xmlFreeOTSERV(nodeValue);
						}

						if(sentence.length() > 0) {
							mType->yellingSentences.push_back(make_pair(sentence, TimeProbabilityClass(cycleTicks, probability, exhaustionTicks)));
						}
					}
					tmp = tmp->next;
				}
			}
			else if (strcmp(str, "loot") == 0){
				tmp = p->children;
				while(tmp){
					LootBlock lootBlock;
					if(loadLootItem(tmp, lootBlock)){
						mType->lootItems.push_back(lootBlock);
					}
					tmp = tmp->next;
				}
			}
			else if (strcmp(str, "summons") == 0){
				nodeValue = (char*)xmlGetProp(p, (const xmlChar *)"maxSummons");
				if(nodeValue){
					mType->maxSummons = std::min(atoi(nodeValue), 100);
					xmlFreeOTSERV(nodeValue);
				}

				tmp = p->children;
				while(tmp){
					if (strcmp((const char*)tmp->name, "summon") == 0) {

						summonBlock sb;
						sb.name = "";
						sb.summonChance = CHANCE_MAX;

						nodeValue = (char*)xmlGetProp(tmp, (const xmlChar *)"name");
						if(nodeValue) {
							sb.name = nodeValue;
							xmlFreeOTSERV(nodeValue);
						}
						else
							continue;

						nodeValue = (char*)xmlGetProp(tmp, (const xmlChar *)"chance");
						if(nodeValue) {
							sb.summonChance = std::max(atoi(nodeValue), 100);
							if(sb.summonChance > CHANCE_MAX)
								sb.summonChance = CHANCE_MAX;

							xmlFreeOTSERV(nodeValue);
						}

						mType->summonSpells.push_back(sb);
					}

					tmp = tmp->next;
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
	char* nodeValue = (char*)xmlGetProp(node, (const xmlChar *)"id");
	if(nodeValue){
		lootBlock.id = atoi(nodeValue);
		xmlFreeOTSERV(nodeValue);
	}
	
	if(lootBlock.id == 0){
		return false;
	}
	
	if(Item::items[lootBlock.id].stackable == true){
		char* nodeValue = (char*)xmlGetProp(node, (const xmlChar *) "countmax");
		if(nodeValue){
			lootBlock.countmax = atoi(nodeValue);
			xmlFreeOTSERV(nodeValue);
				
			if(lootBlock.countmax > 100){
				lootBlock.countmax = 100;
			}
		}
		else{
			std::cout << "missing countmax for loot id = "<< lootBlock.id << std::endl;
			lootBlock.countmax = 1;
		}
			
		nodeValue = (char*)xmlGetProp(node, (xmlChar*)"chancemax");
		if(nodeValue){
			lootBlock.chancemax = atoi(nodeValue);
			xmlFreeOTSERV(nodeValue);
				
			if(lootBlock.chancemax > CHANCE_MAX){
				lootBlock.chancemax = 0;
			}
		}
		else{
			std::cout << "missing chancemax for loot id = "<< lootBlock.id << std::endl;
			lootBlock.chancemax = 0;
		}

		nodeValue = (char*)xmlGetProp(node, (xmlChar*)"chance1");
		if(nodeValue){
			lootBlock.chance1 = atoi(nodeValue);
			xmlFreeOTSERV(nodeValue);
							
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
		char* nodeValue = (char*)xmlGetProp(node, (xmlChar*)"chance");
		if(nodeValue){
			lootBlock.chance1 = atoi(nodeValue);
			xmlFreeOTSERV(nodeValue);
			
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
	xmlNodePtr tmp,p;
	char* nodeValue = NULL;
	unsigned short s_id; 
	
	if(node == NULL){
		return false;
	}
	tmp = node->children;
	if(tmp == NULL){
		return false;
	}
	while(tmp){
		if(strcmp((const char*)tmp->name, "inside") == 0){
			p = tmp->children;
			while(p){
				LootBlock lootBlock;
				if(loadLootItem(p, lootBlock)){
					lBlock.childLoot.push_back(lootBlock);
				}
				p = p->next;
			}
			return true;
		}//inside
		tmp = tmp->next;
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

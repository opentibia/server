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

#include <vector>
#include <string>
#include <sstream>

#include "monster.h"
#include "spells.h"
#include "luascript.h"

extern Spells spells;
extern LuaScript g_config;

AutoList<Monster>Monster::listMonster;

Monster::Monster(const std::string& name, Game* game) : 
Creature(name)
{
	//std::cout << "Monster constructor " << this  <<std::endl;
	useCount = 0;
	oldThinkTicks = 0;
	loaded = false;
	state = STATE_IDLE;
	updateMovePos = false;
	this->name = name;
	this->game = game;
	this->speed = 220;
	this->level = 8;
	this->maglevel = 0;
	this->experience = 0;
	curPhysicalAttack = NULL;
	hasDistanceAttack = false;
	hasLostMaster = false;
	canPushItems = false;
	staticAttack = 20;
	changeTargetChance = 0;
	maxSummons = 0;

	targetDistance = 1;
	runAwayHealth = 0;
	pushable = true;
	defense = 1;
	armor = 1;

	std::string datadir = g_config.getGlobalString("datadir");
	std::string filename = datadir + "monster/" + std::string(name) + ".xml";
	std::transform(filename.begin(), filename.end(), filename.begin(), tolower);
	xmlDocPtr doc = xmlParseFile(filename.c_str());
	if (doc){
		loaded = true;
		xmlNodePtr root, p, tmp;
		char* nodeValue = NULL;
		root = xmlDocGetRootElement(doc);

		if (xmlStrcmp(root->name,(const xmlChar*) "monster")){
			//TODO:	use	exceptions here
			std::cerr << "Malformed	XML" << std::endl;
		}

		p = root->children;

		nodeValue = (char*)xmlGetProp(root, (const xmlChar *)"name");
		if(nodeValue) {
			this->name = nodeValue;
			xmlFreeOTSERV(nodeValue);
		}
		else
			loaded = false;

		nodeValue = (char*)xmlGetProp(root, (const xmlChar *)"experience");
		if(nodeValue) {
			experience = atoi(nodeValue);
			xmlFreeOTSERV(nodeValue);
		}

		nodeValue = (char*)xmlGetProp(root, (const xmlChar *)"pushable");
		if(nodeValue) {
			pushable = (bool)atoi(nodeValue);
			xmlFreeOTSERV(nodeValue);
		}

		nodeValue = (char*)xmlGetProp(root, (const xmlChar *)"level");
		if(nodeValue) {
			level = atoi(nodeValue);
			xmlFreeOTSERV(nodeValue);
			//setNormalSpeed();
		}

		nodeValue = (char*)xmlGetProp(root, (const xmlChar *)"speed");
		if(nodeValue) {
			this->speed = atoi(nodeValue);
			xmlFreeOTSERV(nodeValue);
		}

		nodeValue = (char*)xmlGetProp(root, (const xmlChar *)"maglevel");
		if(nodeValue) {
			maglevel = atoi(nodeValue);
			xmlFreeOTSERV(nodeValue);
		}

		nodeValue = (char*)xmlGetProp(root, (const xmlChar *)"defense");
		if(nodeValue) {
			defense = atoi(nodeValue);
			xmlFreeOTSERV(nodeValue);
		}

		nodeValue = (char*)xmlGetProp(root, (const xmlChar *)"armor");
		if(nodeValue) {
			armor = atoi(nodeValue);
			xmlFreeOTSERV(nodeValue);
		}

		nodeValue = (char*)xmlGetProp(root, (const xmlChar *)"canpushitems");
		if(nodeValue) {
			canPushItems = (bool)atoi(nodeValue);
			xmlFreeOTSERV(nodeValue);
		}

		nodeValue = (char*)xmlGetProp(root, (const xmlChar *)"staticattack");
		if(nodeValue) {
			staticAttack = atoi(nodeValue);
			xmlFreeOTSERV(nodeValue);

			if(staticAttack == 0)
				staticAttack = 1;
			else if(staticAttack >= RAND_MAX)
				staticAttack = RAND_MAX;
		}

		nodeValue = (char*)xmlGetProp(root, (const xmlChar *)"changetarget"); //0	never, 10000 always
		if (nodeValue) {
			changeTargetChance = atoi(nodeValue);
			xmlFreeOTSERV(nodeValue);
		}

		while	(p)
		{
			const char* str = (char*)p->name;

			if (strcmp(str, "health") == 0) {
				nodeValue = (char*)xmlGetProp(p, (const xmlChar *)"now");
				if(nodeValue) {
					health = atoi(nodeValue);
					xmlFreeOTSERV(nodeValue);
				}
				else
					loaded = false;

				nodeValue = (char*)xmlGetProp(p, (const xmlChar *)"max");
				if(nodeValue) {
					healthmax = atoi(nodeValue);
					xmlFreeOTSERV(nodeValue);
				}
				else
					loaded = false;
			}
			if (strcmp(str, "combat") == 0) {
				nodeValue = (char*)xmlGetProp(p, (const xmlChar *)"targetdistance");
				if(nodeValue) {
					targetDistance = std::max(1, atoi(nodeValue));
					xmlFreeOTSERV(nodeValue);
				}

				nodeValue = (char*)xmlGetProp(p, (const xmlChar *)"runonhealth");
				if(nodeValue) {
					runAwayHealth = atoi(nodeValue);
					xmlFreeOTSERV(nodeValue);
				}
			}
			else if (strcmp(str, "look") == 0) {
				nodeValue = (char*)xmlGetProp(p, (const xmlChar *)"type");
				if(nodeValue) {
					looktype = atoi(nodeValue);
					lookmaster = this->looktype;
					xmlFreeOTSERV(nodeValue);
				}

				nodeValue = (char*)xmlGetProp(p, (const xmlChar *)"head");
				if(nodeValue) {
					lookhead = atoi(nodeValue);
					xmlFreeOTSERV(nodeValue);
				}

				nodeValue = (char*)xmlGetProp(p, (const xmlChar *)"body");
				if(nodeValue) {
					lookbody = atoi(nodeValue);
					xmlFreeOTSERV(nodeValue);
				}

				nodeValue = (char*)xmlGetProp(p, (const xmlChar *)"legs");
				if(nodeValue) {
					looklegs = atoi(nodeValue);
					xmlFreeOTSERV(nodeValue);
				}

				nodeValue = (char*)xmlGetProp(p, (const xmlChar *)"feet");
				if(nodeValue) {
					lookfeet = atoi(nodeValue);
					xmlFreeOTSERV(nodeValue);
				}

				nodeValue = (char*)xmlGetProp(p, (const xmlChar *)"corpse");
				if(nodeValue) {
					lookcorpse = atoi(nodeValue);
					xmlFreeOTSERV(nodeValue);
				}
			}
			else if (strcmp(str, "attacks") == 0)
		 {
				tmp=p->children;
				while(tmp)
			 {
					if (strcmp((const char*)tmp->name, "attack") == 0)
				 {
						int cycleTicks = -1;
						int probability = -1;
						int exhaustionTicks = -1;

						nodeValue = (char*)xmlGetProp(tmp, (const xmlChar *)"exhaustion");
						if(nodeValue) {
							exhaustionTicks = atoi(nodeValue);
							xmlFreeOTSERV(nodeValue);
						}

						nodeValue = (char*)xmlGetProp(tmp, (const xmlChar *)"cycleticks");
						if(nodeValue) {
							cycleTicks = atoi(nodeValue);
							xmlFreeOTSERV(nodeValue);
						}

						nodeValue = (char*)xmlGetProp(tmp, (const xmlChar *)"probability");
						if(nodeValue) {
							probability = atoi(nodeValue);
							xmlFreeOTSERV(nodeValue);
						}

						TimeProbabilityClass timeprobsystem(cycleTicks, probability, exhaustionTicks);

						nodeValue = (char*)xmlGetProp(tmp, (const xmlChar *)"type");
						std::string	attacktype = "";
						if(nodeValue) {
							attacktype = nodeValue;
							xmlFreeOTSERV(nodeValue);
						}

						if(strcmp(attacktype.c_str(), "melee") == 0)
					 {
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

							physicalAttacks[physicalattack] = TimeProbabilityClass(cycleTicks, probability, exhaustionTicks);
						}
						else if(strcmp(attacktype.c_str(), "distance") == 0)
					 {
							hasDistanceAttack = true;
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

							physicalAttacks[physicalattack] = TimeProbabilityClass(cycleTicks, probability, exhaustionTicks);
						}
						else if(strcmp(attacktype.c_str(), "instant") == 0) {
							hasDistanceAttack = true;
							std::string spellname = "";

							nodeValue = (char*)xmlGetProp(tmp, (const xmlChar *)"name");
							if(nodeValue) {
								spellname = nodeValue;
								xmlFreeOTSERV(nodeValue);
							}

							if(spells.getAllSpells()->find(spellname) != spells.getAllSpells()->end())
						 {
								instantSpells[spellname].push_back(timeprobsystem);
							}
						}
						else if(strcmp(attacktype.c_str(), "rune") == 0) {
							hasDistanceAttack = true;
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
									runeSpells[rsIt->first].push_back(timeprobsystem);
									break;
								}
							}
						}
					}

					tmp = tmp->next;
				}
			}
			else if (strcmp(str, "defenses") == 0)
		 {
				tmp=p->children;
				while(tmp)
			 {
					if (strcmp((const char*)tmp->name, "defense") == 0) {
						std::string immunity = "";

						nodeValue = (char*)xmlGetProp(tmp, (const xmlChar *)"immunity");
						if(nodeValue) {
							immunity = nodeValue;
							xmlFreeOTSERV(nodeValue);
						}

						if(strcmp(immunity.c_str(), "energy") == 0)
							immunities |= ATTACK_ENERGY;
						else if(strcmp(immunity.c_str(), "burst") == 0)
							immunities |= ATTACK_BURST;
						else if(strcmp(immunity.c_str(), "fire") == 0)
							immunities |= ATTACK_FIRE;
						else if(strcmp(immunity.c_str(), "physical") == 0)
							immunities |= ATTACK_PHYSICAL;
						else if(strcmp(immunity.c_str(), "poison") == 0)
							immunities |= ATTACK_POISON;
						else if(strcmp(immunity.c_str(), "paralyze") == 0)
							immunities |= ATTACK_PARALYZE;
						else if(strcmp(immunity.c_str(), "drunk") == 0)
							immunities |= ATTACK_DRUNKNESS;
					}

					tmp = tmp->next;
				}
			}
			else if (strcmp(str, "voices") == 0)
		 {
				tmp=p->children;
				while(tmp)
			 {
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
							yellingSentences.push_back(make_pair(sentence, TimeProbabilityClass(cycleTicks, probability, exhaustionTicks)));
							//sentenceVec.push_back(sentence);
						}
					}

					tmp = tmp->next;
				}
			}
			else if (strcmp(str, "loot") == 0)
		 {
				lootItems.clear();
				LoadLootNode(p->children);
			}
			else if (strcmp(str, "summons") == 0)
		 {
				nodeValue = (char*)xmlGetProp(p, (const xmlChar *)"maxSummons");
				if(nodeValue) {
					maxSummons = std::min(atoi(nodeValue), 100);
					xmlFreeOTSERV(nodeValue);
				}

				tmp=p->children;
				while(tmp)
			 {
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

						summonSpells.push_back(sb);
					}

					tmp = tmp->next;
				}
			}

			p = p->next;
		}

		xmlFreeDoc(doc);
	}

}

bool Monster::LoadLootNode(xmlNodePtr	tmp){
	unsigned short s_id;
	char* nodeValue = NULL;
	Item *tmpItem;
	while(tmp){
		s_id = 0;
		nodeValue = (char*)xmlGetProp(tmp, (const xmlChar *) "id");
		if(nodeValue){
			s_id = atoi(nodeValue);
			xmlFreeOTSERV(nodeValue);
		}

		if(s_id != 0){
			if(Item::items[s_id].stackable == false){
				tmpItem = LoadLootItem(tmp,s_id);
			}
			else{
				tmpItem = LoadLootItemStackable(tmp,s_id);
			}
			if(tmpItem){
				lootItems.push_back(tmpItem);
				Container* loot_container = dynamic_cast<Container*>(tmpItem);
				if(loot_container) {
					LoadLootContainer(tmp, loot_container);
					//removes empty container
					if(loot_container->size() == 0){
						lootItems.pop_back();
						delete tmpItem;
					}
				}
			}//tmpitem
		}//s_id != 0
		tmp=tmp->next;
	}
	return true;
}

Item* Monster::LoadLootItemStackable(xmlNodePtr	tmp,unsigned short id){	
	Item *tmpItem;
	unsigned long chance1,chancemax;
	unsigned char countmax,n;

	char* nodeValue = (char*)xmlGetProp(tmp, (const xmlChar *) "countmax");
	if(nodeValue) {
		countmax = atoi(nodeValue);
		xmlFreeOTSERV(nodeValue);

		if(countmax > 100){
			countmax = 100;
		}
	}
	else{
		std::cout << "missing countmax for loot id = "<< id << std::endl;
		countmax = 1;
	}

	nodeValue = (char*)xmlGetProp(tmp, (xmlChar*)"chancemax");
	if (nodeValue){
		chancemax = atoi(nodeValue);
		xmlFreeOTSERV(nodeValue);

		if(chancemax > CHANCE_MAX)
			chancemax = 0;
	}
	else{
		std::cout << "missing	chancemax for loot id = "<< id << std::endl;
		chancemax = 0;
	}

	nodeValue = (char*)xmlGetProp(tmp, (xmlChar*)"chance1");
	if (nodeValue){
		chance1 = atoi(nodeValue);
		xmlFreeOTSERV(nodeValue);

		if(chance1 > CHANCE_MAX)
			chance1 = CHANCE_MAX;
	}
	else{
		std::cout << "missing chance1 for loot id = "<< id << std::endl;
		chance1 = CHANCE_MAX;
	}

	if(chance1 <= chancemax){
		std::cout << "Wrong chance for loot id = "<< id << std::endl;
		return NULL;
	}

	unsigned long randvalue = GetRandom();

	if(randvalue < chance1){
		if(randvalue < chancemax){
			n = countmax;
		}
		else{
			//if chancemax < randvalue < chance1
			//get number of items using a linear relation between
			// n and chances
			//n = (unsigned char)(countmax*(randvalue-chancemax)/(chance1-chancemax));
			n = (unsigned	char)(randvalue % countmax + 1);
		}		
		tmpItem = Item::CreateItem(id,n);
	}
	else{
		tmpItem = NULL;
	}
	return tmpItem;
}

Item* Monster::LoadLootItem(xmlNodePtr tmp,unsigned short id){
	Item *tmpItem;
	unsigned long chance;
	char* nodeValue = (char*)xmlGetProp(tmp, (xmlChar*)"chance");
	if (nodeValue){
		chance = atoi(nodeValue);
		xmlFreeOTSERV(nodeValue);

		if(chance > CHANCE_MAX)
			chance = CHANCE_MAX;
	}
	else{
		std::cout << "missing chance for loot id = "<< id << std::endl;
		chance = CHANCE_MAX;
	}

	if(GetRandom() < chance){
		tmpItem = Item::CreateItem(id);
	}
	else{
		tmpItem = NULL;
	}
	return tmpItem;
}

unsigned long Monster::GetRandom(){
	return (unsigned long)((rand()<< 16 |	rand()) % CHANCE_MAX);
}

bool Monster::LoadLootContainer(xmlNodePtr nodeitem,Container* ccontainer){
	xmlNodePtr tmp,p;
	char* nodeValue = NULL;
	unsigned short s_id; 
	Item *new_item; 
	if(nodeitem==NULL){
		return false;
	}
	tmp=nodeitem->children;
	if(tmp==NULL){
		return false;
	}
	while(tmp){
		if (strcmp((const char*)tmp->name, "inside") == 0){
			p=tmp->children;
			while(p){
				nodeValue = (char*)xmlGetProp(p, (const xmlChar *) "id");
				s_id = 0;

				if(nodeValue) {
					s_id = atoi(nodeValue);
					xmlFreeOTSERV(nodeValue);
				}

				if(s_id != 0){
					if(Item::items[s_id].stackable == false){
						new_item = LoadLootItem(p,s_id);
					}
					else{
						new_item = LoadLootItemStackable(p,s_id);
					}
					if(new_item){
						ccontainer->addItem(new_item);
						Container* in_container = dynamic_cast<Container*>(new_item);
						if(in_container){
							LoadLootContainer(p,in_container);
							//removes empty container
							if(in_container->size() == 0){
								ccontainer->removeItem(new_item);
								delete new_item;
							}
						}
					}//new_item
				}//s_id!=0
				p=p->next;
			}
			return true;
		}	// inside
		tmp = tmp->next;
	}
	return false;
}

Monster::~Monster()
{
	for(std::map<PhysicalAttackClass*, TimeProbabilityClass>::iterator it = physicalAttacks.begin(); it != physicalAttacks.end(); ++it) {
		delete it->first;
	}
	//std::cout << "Monster	destructor " <<	this <<std::endl;
	physicalAttacks.clear();
}

bool Monster::isCreatureAttackable(const Creature* creature)
{
	if(!creature || creature->access != 0)
		return false;

	if(!dynamic_cast<const Player*>(creature) && !(const_cast<Creature*>(creature))->getMaster())
		return false;

	Creature* topMaster = (const_cast<Creature*>(creature))->getMaster();
	while(topMaster && topMaster->getMaster()) {
		topMaster = const_cast<Creature*>(topMaster->getMaster());
	}

	if(topMaster && (!dynamic_cast<const Player*>(topMaster) || topMaster->access > 0))
		return false;
	
	return true;
}

bool Monster::validateDistanceAttack(const Creature *target)
{
	return isInRange(pos) && game->map->canThrowItemTo(this->pos, target->pos, false, true);
}

bool Monster::validateDistanceAttack(const Position &pos)
{
	return isInRange(pos) && game->map->canThrowItemTo(this->pos, pos, false, true);
}

bool Monster::isCreatureReachable(const Creature* creature)
{
	Tile* tile = game->getTile(creature->pos.x, creature->pos.y, creature->pos.z);
	if(!tile || tile->isPz()) {
		return false;
	}

	if(hasDistanceAttack && validateDistanceAttack(creature)) {
		return true;
	}
	else {
		if(getCurrentDistanceToTarget(creature->pos) <= 1) {
			return true;
		}

		Position closePos;
		return getCloseCombatPosition(creature->pos, closePos);

		/*
		//to slow
		if(getCloseCombatPosition(creature->pos, closePos)) {
			return !game->map->getPathTo(this, this->pos, closePos, true, canPushItems).empty();
		}
		*/

		return false;
	}
}

Creature* Monster::findTarget(long range, bool &canReach, const Creature *ignoreCreature /*= NULL*/)
{
	SpectatorVec tmplist;
	if(range == 0)
		game->getSpectators(Range(this->pos,false), tmplist);
	else
		game->getSpectators(Range(this->pos, range, range, range, range, false), tmplist);

	std::vector<Creature*> targetlist;
	std::vector<Creature*> unreachablelist;

	for(SpectatorVec::const_iterator cit = tmplist.begin(); cit != tmplist.end(); ++cit) {
		if(ignoreCreature == *cit || (*cit)->access > 0)
			continue;

		if(!isCreatureAttackable(*cit))
			continue;

		if(!isCreatureReachable(*cit)) {
			unreachablelist.push_back(*cit);
			continue;
		}

		targetlist.push_back(*cit);
	}

	if(targetlist.empty()) {
		if(!unreachablelist.empty()) {
			size_t index = random_range(0, unreachablelist.size() - 1);
			canReach = false;
			return unreachablelist[index];
		}

		canReach = false;
		return NULL;
	}

	size_t index = random_range(0, targetlist.size() - 1);
	canReach = true;
	return targetlist[index];
}

void Monster::getSleepTicks(long long &delay, int& stepDuration)
{
	delay = 0;
	stepDuration = 0;

	Tile *tile =game->getTile(this->pos.x, this->pos.y, this->pos.z);
	if(tile && tile->ground) {
		int groundid = tile->ground->getID();

		uint8_t stepspeed = Item::items[groundid].speed;
		if(stepspeed != 0) {
			stepDuration = this->getStepDuration(stepspeed, (getSpeed() != 0 ? getSpeed() : 220));

			if(lastmove != 0) {
				delay = (((long	long)(this->lastmove)) + ((long	long)(stepDuration))) - ((long long)(OTSYS_TIME()));
			}
		}
	}
}

int Monster::onThink(int& newThinkTicks)
{
	bool yelled = false;
	for(YellingSentences::iterator ysIt = yellingSentences.begin(); ysIt != yellingSentences.end(); ++ysIt) {
		if(ysIt->second.onTick(oldThinkTicks) && !yelled) {
			yelled = true;
			game->creatureMonsterYell(this, ysIt->first);
		}
	}

	long long delay;
	int stepDuration;
	getSleepTicks(delay, stepDuration);

	if(state == STATE_TARGETNOTREACHABLE || attackedCreature != 0) {
		if(delay > 0) {
			newThinkTicks = (int)delay;
			int ret = oldThinkTicks;
			oldThinkTicks = newThinkTicks;
			return ret;
		}

		this->lastmove = OTSYS_TIME();
	}

	reThink(false);

	//check/update/calc route
	if(state != STATE_IDLE && state != STATE_TARGETNOTREACHABLE && !(isSummon() && hasLostMaster))
	{
		if(updateMovePos || !game->map->isPathValid(this, route, canPushItems))
		{
			updateMovePos = false;

			if(calcMovePosition()) {
				route = game->map->getPathTo(this, this->pos, moveToPos, true, canPushItems);

				if(route.empty() && !(state == STATE_IDLESUMMON) && (!hasDistanceAttack || !validateDistanceAttack(targetPos))) {
					state = STATE_TARGETNOTREACHABLE;
				}

				if(!route.empty() && route.front() == this->pos) {
					route.pop_front();
				}
			}
		}
	}

	//process movement
	if(state != STATE_IDLE && !(state == STATE_IDLESUMMON && hasLostMaster)) {
		if(state == STATE_TARGETNOTREACHABLE) {
			Position newMovePos;
			if(getRandomPosition(targetPos, newMovePos)) {
				int dx = newMovePos.x - this->pos.x;
				int dy = newMovePos.y - this->pos.y;

				doMoveTo(dx, dy);
			}
			else {
				updateLookDirection();
			}
		}
		else if(!route.empty()){
			Position nextStep = route.front();
			route.pop_front();

			int dx = nextStep.x - this->pos.x;
			int dy = nextStep.y - this->pos.y;

			doMoveTo(dx, dy);
		}
		else {
			updateLookDirection();
		}

		newThinkTicks = stepDuration;
		int ret = oldThinkTicks;
		oldThinkTicks = newThinkTicks;
		return ret;
	}

	eventCheck = 0;
	stopThink();
	newThinkTicks = 0;
	return oldThinkTicks;
}

int Monster::getTargetDistance()
{
	return targetDistance;
}

int Monster::getCurrentDistanceToTarget(const Position& target)
{
	return std::max(std::abs(pos.x - target.x), std::abs(pos.y - target.y));
}

void Monster::updateLookDirection()
{
	if(isSummon() && state == STATE_IDLESUMMON) {
		return;
	}

	int deltax = targetPos.x - this->pos.x;
	int deltay = targetPos.y - this->pos.y;

	if(!(deltax == 0 && deltay == 0)) {
		Direction newdir = this->getDirection();

		//SE
		if(deltax < 0 && deltay < 0) {
			if(std::abs(deltax) > std::abs(deltay)) {
				newdir = WEST;
			}
			else if(std::abs(deltay) > std::abs(deltax)) {
				newdir = NORTH;
			}
			else if(getDirection() != NORTH && getDirection() != WEST) {
				newdir = (rand() % 2 == 0 ? NORTH : WEST);
			}
		}
		//SW
		else if(deltax > 0 && deltay < 0) {
			if(std::abs(deltax) > std::abs(deltay)) {
				newdir = EAST;
			}
			else if(std::abs(deltay) > std::abs(deltax)) {
				newdir = NORTH;
			}
			else if(getDirection() != NORTH && getDirection() != EAST) {
				newdir = (rand() % 2 == 0 ? NORTH : EAST);
			}
		}
		//NW
		else if(deltax > 0 && deltay > 0) {
			if(std::abs(deltax) > std::abs(deltay)) {
				newdir = EAST;
			}
			else if(std::abs(deltay) > std::abs(deltax)) {
				newdir = SOUTH;
			}
			else if(getDirection() != SOUTH && getDirection() != EAST) {
				newdir = (rand() % 2 == 0 ? SOUTH : EAST);
			}
		}
		//NE
		else if(deltax < 0 && deltay > 0) {
			if(std::abs(deltax) > std::abs(deltay)) {
				newdir = WEST;
			}
			else if(std::abs(deltay) > std::abs(deltax)) {
				newdir = SOUTH;
			}
			else if(getDirection() != SOUTH && getDirection() != EAST) {
				newdir = (rand() % 2 == 0 ? SOUTH : WEST);
			}
		}
		//N
		else if(deltax == 0 && deltay > 0)
			newdir = SOUTH;
		//S
		else if(deltax == 0 && deltay < 0)
			newdir = NORTH;
		//W
		else if(deltax > 0 && deltay == 0)
			newdir = EAST;
		//E
		else if(deltax < 0 && deltay == 0)
			newdir = WEST;

		if(newdir != this->getDirection()) {
			game->creatureTurn(this, newdir);
		}
	}
}

void Monster::setUpdateMovePos() {
	updateMovePos = true;
	route.clear();
}

bool Monster::calcMovePosition()
{
	bool foundDistPath = false;
	Position newMovePos = moveToPos;
	if(state == STATE_IDLESUMMON && isSummon()) {
		getCloseCombatPosition(getMaster()->pos, newMovePos);
	}
	else {
		if(state == STATE_FLEEING) {
			getDistancePosition(targetPos, 10, true, newMovePos);
			foundDistPath = true;
		}
		else {
			bool fullPathSearch = !validateDistanceAttack(targetPos);
			if(getTargetDistance() > 1 /*&& hasDistanceAttack*/) {
				foundDistPath = getDistancePosition(targetPos, getTargetDistance(), fullPathSearch, newMovePos);
			}
		}

		//Close combat
		if(!foundDistPath) {
			int currentDist = getCurrentDistanceToTarget(targetPos);
			if(currentDist > getTargetDistance() || currentDist == 0) {
				getCloseCombatPosition(targetPos, newMovePos);
			}
		}
	}

	if(newMovePos == this->pos) {
		route.clear();
		return false;
	}
	else if(newMovePos != moveToPos) {
		moveToPos = newMovePos;
		route.clear();
		return true;
	}
	else {
		//same move position as before
		return true;
	}
}

bool Monster::getDistancePosition(const Position &target, const int& maxTryDist, bool fullPathSearch, Position &dest)
{
	int minTryDist = 0;
	int currentDistance = getCurrentDistanceToTarget(target);
	if(currentDistance <= maxTryDist && validateDistanceAttack(target)) {

		if(!fullPathSearch) {
			minTryDist = currentDistance;
		}

		if(currentDistance == maxTryDist) {
			dest = this->pos;
			return true;
		}
	}

	std::vector<Position> positionList;
	Position tmpPos;
	int tryDist = 0;
	int prevDist = 0;

	int tryWalkDist = 0;
	int minWalkDist = 0;

	int xmindelta = ((fullPathSearch || (pos.x - target.x) <= 0) ? maxTryDist : 0);
	int xmaxdelta = ((fullPathSearch || (pos.x - target.x) >= 0) ? maxTryDist : 0);
	int ymindelta = ((fullPathSearch || (pos.y - target.y) <= 0) ? maxTryDist : 0);
	int ymaxdelta = ((fullPathSearch || (pos.y - target.y) >= 0) ? maxTryDist : 0);

	for (int y = target.y - ymindelta; y <= target.y + ymaxdelta; ++y) {
		for (int x = target.x - xmindelta; x <= target.x + xmaxdelta; ++x) {

			if((target.x == x && target.y == y))
				continue;

			//tryDist = std::sqrt( std::pow(std::abs(target.x - x), 2) + std::pow(std::abs(target.y - y), 2));
			tryDist = std::abs(target.x - x)*std::abs(target.x - x) + std::abs(target.y - y)*std::abs(target.y - y);

			tmpPos.x = x;
			tmpPos.y = y;
			tmpPos.z = pos.z;

			if(tryDist <= (maxTryDist * maxTryDist) && (tryDist >= prevDist) && (tryDist > (minTryDist * minTryDist))) {

				if(!game->map->canThrowItemTo(tmpPos, target, false, true))
					continue;

				if(tmpPos != this->pos) {
					if(!canMoveTo(x, y, pos.z))
						continue;
				}

				tryWalkDist = std::abs(this->pos.x - x)*std::abs(this->pos.x - x) + 
									std::abs(this->pos.y - y)*std::abs(this->pos.y - y);

				if(tryWalkDist > 0 && (tryWalkDist < minWalkDist || minWalkDist == 0)) {
					positionList.clear();
					minWalkDist = tryWalkDist;
				}
				
				else if(tryDist > prevDist) {
					positionList.clear();
				}

				prevDist = tryDist;
				positionList.push_back(tmpPos);
				//#ifdef __DEBUG__
				//				std::cout << "CalcMovePosition()" << ", x: " << tmpPos.x << ", y: " << tmpPos.y  << std::endl;
				//#endif
			}
		}
	}

	if(positionList.empty()){
		dest = this->pos;
		//dest = target;
		return currentDistance <= maxTryDist && game->map->canThrowItemTo(this->pos, target, false, true);
	}

	size_t index = random_range(0, positionList.size() - 1);
	dest = positionList[index];
	return true;
}

bool Monster::getCloseCombatPosition(const Position &target, Position &dest)
{
	//Close combat
	std::vector<Position> positionList;
	Position tmpPos;
	int prevdist = 0;
	for(int y = target.y - 1; y <= target.y + 1; ++y) {
		for(int x = target.x - 1; x <= target.x + 1; ++x) {
			if((target.x == x && target.y == y))
				continue;

			int dist = std::max(std::abs(pos.x - x), std::abs(pos.y - y));

			tmpPos.x = x;
			tmpPos.y = y;
			tmpPos.z = pos.z;

			if(dist <= prevdist || (prevdist == 0)) {

				if(tmpPos != this->pos) {
					if(!canMoveTo(x,y,pos.z))
						continue;
				}

				if(dist < prevdist)
					positionList.clear();

				prevdist = dist;
				positionList.push_back(tmpPos);
			}
		}
	}

	if(positionList.empty()){
		dest = this->pos;
		//dest = target;
		return false;
	}

	size_t index = random_range(0, positionList.size() - 1);
	dest = positionList[index];
	return true;
}

bool Monster::getRandomPosition(const Position &target, Position &dest)
{
	if(state == STATE_TARGETNOTREACHABLE) {
		std::vector<Position>	positionList;
		Position tmppos;

		tmppos.x = this->pos.x + 1;
		tmppos.y = this->pos.y;
		tmppos.z = this->pos.z;

		if(((std::abs(target.x - tmppos.x) <= 9) && (std::abs(target.y - tmppos.y) <= 7)) &&
			canMoveTo(tmppos.x, tmppos.y, tmppos.z))
			positionList.push_back(tmppos);

		tmppos.x = this->pos.x - 1;
		tmppos.y = this->pos.y;
		tmppos.z = this->pos.z;

		if(((std::abs(target.x - tmppos.x) <= 9) && (std::abs(target.y - tmppos.y) <= 7)) &&
			canMoveTo(tmppos.x, tmppos.y, tmppos.z))
			positionList.push_back(tmppos);

		tmppos.x = this->pos.x;
		tmppos.y = this->pos.y + 1;
		tmppos.z = this->pos.z;

		if(((std::abs(target.x - tmppos.x) <= 9) && (std::abs(target.y - tmppos.y) <= 7)) &&
			canMoveTo(tmppos.x, tmppos.y, tmppos.z))
			positionList.push_back(tmppos);

		tmppos.x = this->pos.x;
		tmppos.y = this->pos.y - 1;
		tmppos.z = this->pos.z;

		if(((std::abs(target.x - tmppos.x) <= 9) && (std::abs(target.y - tmppos.y) <= 7)) &&
			canMoveTo(tmppos.x, tmppos.y, tmppos.z))
			positionList.push_back(tmppos);

		if(!positionList.empty()) {
			size_t index = random_range(0, positionList.size() - 1);
			dest = positionList[index];
			return true;
		}
		else
			return false;
	}
	else if(getCurrentDistanceToTarget(target) <= 1) {
		//only close combat
		std::vector<Position>	positionList;
		Position randMovePos;
		for(int y = pos.y - 1; y <= pos.y + 1; ++y){
			for(int x = pos.x - 1; x <= pos.x + 1; ++x){
				if(std::abs(y - targetPos.y) <= 1 && std::abs(x - targetPos.x) <= 1 &&
					((std::abs(y - pos.y) == 1)^(std::abs(x - pos.x) == 1)) &&
					canMoveTo(x,y,pos.z)){
						randMovePos.x = x;
						randMovePos.y = y;
						randMovePos.z = pos.z;
						positionList.push_back(randMovePos);
					}
			}
		}

		if(positionList.empty())
			return false;

		size_t index = random_range(0, positionList.size() - 1);
		dest = positionList[index];
		return true;
	}

	return false;
}

bool Monster::isInRange(const Position &p)
{
	return ((std::abs(p.x - this->pos.x) <= 9) && (std::abs(p.y - this->pos.y) <= 7) &&
		(p.z == this->pos.z));
}

void Monster::onThingMove(const Creature *creature, const Thing* thing,
	const Position* oldPos, unsigned char oldstackpos, unsigned char oldcount, unsigned char count)
{
	const Creature* moveCreature = dynamic_cast<const Creature *>(thing);
	if(moveCreature && (moveCreature != this || creature != moveCreature)) {
		onCreatureMove(moveCreature, oldPos);
	}
	else
		reThink();
}

void Monster::onCreatureAppear(const Creature* creature)
{
	if(creature == this) {
		return;
	}

	if(isInRange(creature->pos)) {
		bool canReach = isCreatureReachable(creature);
		onCreatureEnter(creature, canReach);
	}
}

void Monster::onCreatureDisappear(const Creature* creature, unsigned char stackPos, bool tele)
{
	if(creature == this) {
		stopThink();
		return;
	}

	onCreatureLeave(creature);
}

void Monster::onThingDisappear(const Thing* thing, unsigned char stackPos)
{
	const Creature *creature = dynamic_cast<const Creature*>(thing);

	if(creature == this) {
		stopThink();
		return;
	}

	if(creature) {
		onCreatureLeave(creature);
	}
	else {
		reThink();
	}
}

void Monster::onThingTransform(const Thing* thing,int stackpos)
{
	const Item* item = dynamic_cast<const Item*>(thing);
	if(item && item->isSplash())
		return;

	reThink();
}

void Monster::onThingAppear(const Thing* thing){
	const Creature *creature = dynamic_cast<const Creature*>(thing);
	if(creature && isInRange(creature->pos)){
		bool canReach = isCreatureReachable(creature);
		onCreatureEnter(creature, canReach);
	}	
	else {
		reThink();
		setUpdateMovePos();
	}
}

void Monster::onTeleport(const Creature	*creature, const Position* oldPos, unsigned char oldstackpos)
{
	if(creature == this) {
		Creature* attackedCreature = game->getCreatureByID(this->attackedCreature);
		if(!attackedCreature || !isCreatureReachable(attackedCreature)) {
			state = STATE_TARGETNOTREACHABLE;
		}

		reThink();
	}
	else {
		onCreatureMove(creature, oldPos);
	}
}


void Monster::onCreatureMove(const Creature* creature, const Position* oldPos)
{
	if(creature == this) {
		//We been pushed
		reThink();

	}
	else if(isInRange(creature->pos) && isInRange(*oldPos)) {
		//Creature just moving around in-range
		if(attackedCreature == creature->getID()) {
			if(state == STATE_ATTACKING && !isCreatureReachable(creature)) {

				if(isSummon()) {
					state = STATE_TARGETNOTREACHABLE;
					route.clear();
				}
				else {
					state = STATE_TARGETNOTREACHABLE;
					route.clear();

					reThink();
				}
			}
			else {
				targetPos = creature->pos;
				setUpdateMovePos();
			}
		}
		else if(isSummon()) {
			if(state == STATE_IDLESUMMON && getMaster() == creature) {
				startThink();
			}
			else
				reThink();
		}
		else if((state == STATE_IDLE || state == STATE_TARGETNOTREACHABLE)) {
			if(isCreatureAttackable(creature)) {
				bool canReach = isCreatureReachable(creature);
				if(canReach || (state == STATE_IDLE)) {
					selectTarget(creature, canReach);
				}
			}
		}
	}
	else if(isInRange(creature->pos) && !isInRange(*oldPos)) {
		bool canReach = isCreatureReachable(creature);
		onCreatureEnter(creature, canReach);
	}
	else if(!isInRange(creature->pos) && isInRange(*oldPos)) {
		onCreatureLeave(creature);
	}
}

void Monster::onCreatureEnter(const Creature* creature, bool canReach /* = true*/)
{
	if(isSummon()) {
		//master found
		if(creature == getMaster()) {
			hasLostMaster = false;
			startThink();
		}
		//target enters
		else if(attackedCreature == creature->getID()) {
			startThink();
		}
		else
			reThink();
	}
	else if(state == STATE_IDLE || (state == STATE_TARGETNOTREACHABLE && canReach)) {
		if(isCreatureAttackable(creature)) {
			selectTarget(creature, canReach);
		}
	}
}

void Monster::onCreatureLeave(const Creature *creature)
{
	if(creature == this) {
		stopThink();
	}
	else if(isSummon()) {
		//master out of reach
		if(creature == getMaster()) {
			hasLostMaster = true;
			stopThink();
		}
		//target lost
		else if(attackedCreature == creature->getID()) {
			stopThink();
		}
		else
			reThink();
	}
	else {
		if(creature && attackedCreature == creature->getID()) {
			//try find a new target
			bool canReach;
			Creature *target = findTarget(0, canReach, creature);
			if(target) {
				selectTarget(target, canReach);
			}
			else {
				stopThink();
			}
		}
		else
			reThink();
	}
}

void Monster::selectTarget(const Creature* creature, bool canReach /* = true*/)
{
	Creature::setAttackedCreature(creature);
	targetPos = creature->pos;

	//start fleeing?
	if(!isSummon() && runAwayHealth > 0 && this->health <= runAwayHealth) {
		state = STATE_FLEEING;
	}
	else if(canReach) {
		state = STATE_ATTACKING;

		updateLookDirection();
		Creature *attackedCreature = const_cast<Creature*>(creature);

		if(attackedCreature) {
			if(validateDistanceAttack(attackedCreature)){
				doAttacks(attackedCreature, MODE_AGGRESSIVE);
			}
		}
	}
	else {
		state = STATE_TARGETNOTREACHABLE;
	}

	startThink();
}

//something changed, check if we should change state
void Monster::reThink(bool checkOnlyState /* = true*/)
{
	if(isSummon()) {
		//try find a path to the target
		if(state == STATE_TARGETNOTREACHABLE) {
			if(checkOnlyState) {
				setUpdateMovePos();
			}
			else if(calcMovePosition()) {
				state = STATE_ATTACKING;
				return;
			}
		}

		if(state == STATE_ATTACKING) {
			Creature *attackedCreature = game->getCreatureByID(this->attackedCreature);
			if (!attackedCreature || !isCreatureReachable(attackedCreature)) {
				state = STATE_TARGETNOTREACHABLE;
				route.clear();
			}
		}

		if(!updateMovePos && route.empty()) {
			//to far away from master?
			if(state == STATE_IDLESUMMON && getCurrentDistanceToTarget(getMaster()->pos) > 1) {
				setUpdateMovePos();
			}
			//
			//to far away from target?
			else if(state != STATE_IDLESUMMON && getCurrentDistanceToTarget(targetPos) > getTargetDistance()) {
				setUpdateMovePos();
			}
			//
		}
	}
	else {
		//change target
		if(state != STATE_IDLE) {
			if(changeTargetChance > rand()*10000/(RAND_MAX+1)){
				bool canReach;
				Creature *newtarget = findTarget(3, canReach);
				if(newtarget && (canReach || state == STATE_TARGETNOTREACHABLE) && newtarget->getID() != attackedCreature){
					selectTarget(newtarget, canReach);
				}
			}
		}

		if(state == STATE_FLEEING) {
			if(this->health > runAwayHealth || !isInRange(targetPos)) {
				bool canReach;
				Creature *newtarget = findTarget(0, canReach);
				if(newtarget) {
					selectTarget(newtarget, canReach);
				}
				else {
					setAttackedCreature(NULL);
				}
			}
		}

		if(state == STATE_ATTACKING) {
			if(runAwayHealth > 0 && this->health <= runAwayHealth) {
				state = STATE_FLEEING;
				setUpdateMovePos();
			}
		}

		if(state == STATE_ATTACKING) {
			Creature *attackedCreature = game->getCreatureByID(this->attackedCreature);
			if (!attackedCreature || !isCreatureReachable(attackedCreature)) {
				state = STATE_TARGETNOTREACHABLE;
				route.clear();
			}
		}

		if(state == STATE_ATTACKING) {
			//summons
			if(summons.size() < maxSummons) {
				SummonSpells::const_iterator it;
				for(it = summonSpells.begin(); it != summonSpells.end(); ++it) {
					if(rand() % (*it).summonChance == 0) {
						Monster *summon = new Monster((*it).name, game);
						Position summonPos = this->pos;

						if(!summon->isLoaded() || !game->placeCreature(summonPos, summon)){
							delete summon;
						}
						else {
							addSummon(summon);
						}
					}
				}
			}

			//static walking
			if(getTargetDistance() <= 1 && getCurrentDistanceToTarget(targetPos) == 1) {
				if(rand() % staticAttack == 0) {
					Position newMovePos;
					if(getRandomPosition(targetPos, newMovePos)) {
						moveToPos = newMovePos;
					}
				}
			}
			//to far away from target?
			else if(route.empty() && getCurrentDistanceToTarget(targetPos) > getTargetDistance()) {
				setUpdateMovePos();
			}
		}

		if(state == STATE_TARGETNOTREACHABLE) {
			//try find a target
			bool canReach;
			Creature* target = findTarget(0, canReach);
			if(target) {
				if(canReach || target->getID() != attackedCreature) {
					selectTarget(target, canReach);
				}
			}
			else {
				setAttackedCreature(NULL);
			}
		}
	}
}

void Monster::startThink()
{
	//Update move	position
	setUpdateMovePos();

	if(!eventCheck){
		eventCheck = game->addEvent(makeTask(500, std::bind2nd(std::mem_fun(&Game::checkCreature), getID())));
	}

	if(attackedCreature != 0 && !(isSummon() && hasLostMaster)) {
		if(!eventCheckAttacking){
			eventCheckAttacking = game->addEvent(makeTask(500, std::bind2nd(std::mem_fun(&Game::checkCreatureAttacking), getID())));
		}
	}
}

void Monster::stopThink()
{
	if(isSummon()) {
		state = STATE_IDLESUMMON;
		setUpdateMovePos();
	}
	else
		state = STATE_IDLE;

	std::list<Creature*>::iterator cit;
	for(cit = summons.begin(); cit != summons.end(); ++cit) {
		(*cit)->setAttackedCreature(NULL);
	}

	//route.clear();
	//updateRoute = true;
	setUpdateMovePos();
	attackedCreature = 0;

	targetPos.x = 0;
	targetPos.y = 0;
	targetPos.z = 0;

	if(state == STATE_IDLE) {
		game->stopEvent(eventCheck);
		eventCheck = 0;
	}

	game->stopEvent(eventCheckAttacking);
	eventCheckAttacking = 0;
}

void Monster::setMaster(Creature* creature)
{
	Creature::setMaster(creature);

	if(creature) {
		Creature* attackedCreature = dynamic_cast<Creature*>(game->getCreatureByID(creature->attackedCreature));
		if(attackedCreature) {
			bool canReach = isCreatureReachable(attackedCreature);
			selectTarget(attackedCreature, canReach);
		}
		else {
			state = STATE_IDLESUMMON;
			startThink();
		}
	}
	else
		state = STATE_IDLE;
}

void Monster::setAttackedCreature(const Creature* creature)
{
	if(creature == NULL || creature == this) {
		//Creature::setAttackedCreature(NULL);
		stopThink();
	}
	else {
		//Creature::setAttackedCreature(creature);

		if(creature) {
			bool canReach = isCreatureReachable(creature);
			selectTarget(creature, canReach);
		}
		else {
			Creature::setAttackedCreature(NULL);
		}
	}
}

std::string	Monster::getDescription(bool self) const
{
	std::stringstream ss;
	std::string str;

	str = getName();
	std::transform(str.begin(), str.end(), str.begin(), tolower);

	ss << "a " << str << ".";
	str = ss.str();

	return str;
}

int Monster::getWeaponDamage() const
{
	if(curPhysicalAttack != NULL)
		return random_range(curPhysicalAttack->minWeapondamage, curPhysicalAttack->maxWeapondamage);
	else
		return 0;
}

void Monster::dropLoot(Container *corpse)
{
	if(isSummon())
		return;

	for(std::list<Item*>::iterator cit = lootItems.begin(); cit != lootItems.end(); ++cit) {
		corpse->addItem(*cit);
	}
}

bool Monster::doAttacks(Creature* attackedCreature, monstermode_t mode /*= MODE_NORMAL*/)
{
	int modeProb = 0;
	switch(mode) {
		case MODE_NORMAL: modeProb = 0; break;
		case MODE_AGGRESSIVE: modeProb = 50; break;
	}

	bool ret = false;

	bool trymelee = getCurrentDistanceToTarget(targetPos) <= 1;
	bool hasmelee = false;
	if(trymelee){
		for(PhysicalAttacks::iterator paIt = physicalAttacks.begin(); paIt != physicalAttacks.end(); ++paIt) {
			if(paIt->first->fighttype == FIGHT_MELEE) {
				hasmelee = true;
				break;
			}
		}
	}

	for(PhysicalAttacks::iterator paIt = physicalAttacks.begin(); paIt != physicalAttacks.end(); ++paIt) {
		TimeProbabilityClass& timeprobsystem = paIt->second;
		if(timeprobsystem.onTick(500) || (random_range(1, 100) <= modeProb)) {
			if(!hasmelee || (hasmelee && paIt->first->fighttype == FIGHT_MELEE)) {
				curPhysicalAttack = paIt->first;
				game->creatureMakeDamage(this, attackedCreature, getFightType());
			}
		}
	}

	if(exhaustedTicks <= 0) {
		for(RuneAttackSpells::iterator raIt = runeSpells.begin(); raIt != runeSpells.end(); ++raIt) {
			for(TimeProbabilityClassVec::iterator asIt = raIt->second.begin(); asIt != raIt->second.end(); ++asIt) {
				TimeProbabilityClass& timeprobsystem = *asIt;
				if(timeprobsystem.onTick(500) || (random_range(1, 100) <= modeProb)) {

					std::map<unsigned short, Spell*>::iterator rit = spells.getAllRuneSpells()->find(raIt->first);
					if(rit != spells.getAllRuneSpells()->end()) {
						bool success = rit->second->getSpellScript()->castSpell(this, attackedCreature->pos, "");

						if(success) {
							ret = true;
							exhaustedTicks = timeprobsystem.getExhaustion();
							if(exhaustedTicks > 0) {
								return true;
							}
						}
					}
				}
			}
		}

		for(InstantAttackSpells::iterator iaIt = instantSpells.begin(); iaIt != instantSpells.end(); ++iaIt) {
			for(TimeProbabilityClassVec::iterator asIt = iaIt->second.begin(); asIt != iaIt->second.end(); ++asIt) {
				TimeProbabilityClass& timeprobsystem = *asIt;
				if(timeprobsystem.onTick(500) || (random_range(1, 100) <= modeProb)) {

					std::map<std::string, Spell*>::iterator rit = spells.getAllSpells()->find(iaIt->first);
					if(rit != spells.getAllSpells()->end()) {
						bool success = rit->second->getSpellScript()->castSpell(this, this->pos, "");

						if(success) {
							ret = true;
							exhaustedTicks = timeprobsystem.getExhaustion();
							if(exhaustedTicks > 0) {
								return true;
							}
						}
					}
				}
			}
		}
	}

	return ret;
}

void Monster::onAttack()
{
	if (attackedCreature != 0 && !(isSummon() && hasLostMaster)) {
		this->eventCheckAttacking = game->addEvent(makeTask(500, std::bind2nd(std::mem_fun(&Game::checkCreatureAttacking), getID())));

		exhaustedTicks -= 500;

		if(exhaustedTicks < 0)
			exhaustedTicks = 0;

		Creature* attackedCreature = game->getCreatureByID(this->attackedCreature);

		if(attackedCreature && attackedCreature->isAttackable()) {
			if(validateDistanceAttack(attackedCreature)){
				doAttacks(attackedCreature);
			}
		}
		else
			setAttackedCreature(NULL);
	}
}

void Monster::doMoveTo(int dx, int dy)
{
	if(canPushItems){
		//move/destroy blocking moveable items
		Tile *tile =game->getTile(this->pos.x + dx ,this->pos.y + dy ,this->pos.z);
		if(tile){
			int countItems = 0;
			countItems = 0;
			while(Item* blockItem = tile->getMoveableBlockingItem()){
				if(countItems < 2){
					if(!monsterMoveItem(blockItem, 3)){
						//destroy it
						if(game->removeThing(NULL, blockItem->pos, blockItem)){
							game->FreeThing(blockItem);
						}
					}
				}
				else{
					//destroy items directly
					if(game->removeThing(NULL, blockItem->pos, blockItem)){
						game->FreeThing(blockItem);
					}
				}
				countItems++;
			}
		}
	}

	Position moveTo = this->pos;
	moveTo.x += dx;
	moveTo.y += dy;

	this->game->thingMove(this, this, moveTo.x, moveTo.y, moveTo.z, 1);

	if(moveTo != this->pos) {
		setUpdateMovePos();
	}
	else if(state == STATE_ATTACKING && this->pos == moveToPos) {
		updateLookDirection();
	}
}

bool Monster::monsterMoveItem(Item* item, int radius)
{
	Position centerPos = item->pos;
	Position tryPos;

	int itemCount;
	if(item->isStackable()){
		itemCount = item->getItemCountOrSubtype();
	}
	else{
		itemCount = 1;
	}
	//try random position in radius
	tryPos.z = item->pos.z;
	for(int i = 0; i < 4*radius; i++){
		tryPos.x = item->pos.x + rand() % radius;
		tryPos.y = item->pos.y + rand() % radius;
		if(game->map->canThrowItemTo(item->pos,tryPos)){
			//game->thingMove(this, item ,tryPos.x, tryPos.y, tryPos.z, itemCount);

			Tile *fromTile = game->getTile(item->pos.x, item->pos.y, item->pos.z);
			int oldstackpos = fromTile->getThingStackPos(item);

			game->thingMoveInternal(this, item->pos.x, item->pos.y, item->pos.z,
				oldstackpos, item->getID(), tryPos.x, tryPos.y, tryPos.z, count);

			if(item->pos == tryPos){
				return true;
			}
		}
	}
	return false;
}
bool Monster::canMoveTo(unsigned short x, unsigned short y, unsigned char z)
{
	Tile *t;
	if((!(t = game->map->getTile(x, y, pos.z))) ||
		t->isBlocking() ||
		t->isPz() || 
		!t->creatures.empty() ||
		t->floorChange() ||
		t->getTeleportItem()){
			return false;
		}
	else{
		return true;
	}
}

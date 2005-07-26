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

AutoList<Monster> Monster::listMonster;

Monster::Monster(const std::string& name, Game* game) : 
 Creature(name)
{
	//std::cout << "Monster constructor " << this  <<std::endl;
	useCount = 0;
	oldThinkTicks = 0;
	loaded = false;
	isfleeing = false;
	this->name = name;
	this->game = game;
	level = 8;
	maglevel = 0;
	experience = 0;
	curPhysicalAttack = NULL;
	hasDistanceAttack = false;
	canPushItems = false;
	staticAttack = 20;
	changeTargetChance = 0;

	targetDistance = 1;
	runawayHealth = 0;
	pushable = true;
	defense = 0;
	armor = 0;
	
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
		//TODO: use exceptions here
		std::cerr << "Malformed XML" << std::endl;
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
			setNormalSpeed();
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
				staticAttack = RAND_MAX + 1;
		}

		nodeValue = (char*)xmlGetProp(root, (const xmlChar *)"changetarget"); //0 never, 10000 always
		if (nodeValue) {
			changeTargetChance = atoi(nodeValue);
			xmlFreeOTSERV(nodeValue);
		}
		
		while (p)
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
					runawayHealth = atoi(nodeValue);
					xmlFreeOTSERV(nodeValue);
				}
			}
			else if (strcmp(str, "look") == 0) {
				nodeValue = (char*)xmlGetProp(p, (const xmlChar *)"type");
				looktype = atoi(nodeValue);
				lookmaster = this->looktype;

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
						std::string attacktype = "";
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
			else if (strcmp(str, "loot") == 0) {
				lootItems.clear();
				LoadLootNode(p->children);
			}			
			p = p->next;
		}

		xmlFreeDoc(doc);
	}

}

bool Monster::LoadLootNode(xmlNodePtr tmp){
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
					LoadLootContainer(tmp ,loot_container);
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

Item* Monster::LoadLootItemStackable(xmlNodePtr tmp,unsigned short id){	
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
		std::cout << "missing chancemax for loot id = "<< id << std::endl;
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
			//if  chancemax < randvalue < chance1
			//get number of items using a linear relation between
			// n and chances
			//n = (unsigned char)(countmax*(randvalue-chancemax)/(chance1-chancemax));
			n = (unsigned char)(randvalue % countmax + 1);
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
	return (unsigned long)((rand()<< 16 | 
		rand()) % CHANCE_MAX);
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
		} // inside
		tmp = tmp->next;
	}
	return false;
}

Monster::~Monster()
{
	for(std::map<PhysicalAttackClass*, TimeProbabilityClass>::iterator it = physicalAttacks.begin(); it != physicalAttacks.end(); ++it) {
		delete it->first;
	}
	//std::cout << "Monster destructor " << this  <<std::endl;
	physicalAttacks.clear();
}

bool Monster::validateDistanceAttack(const Creature *target)
{
	return game->map->canThrowItemTo(this->pos, target->pos, false, true);
}

bool Monster::validateDistanceAttack(const Position &pos)
{
	return game->map->canThrowItemTo(this->pos, pos, false, true);
}

Creature* Monster::findTarget(long range, const Creature *ignoreCreature /*= NULL*/)
{
	if(attackedCreature == 0) {
		std::vector<Creature*> tmplist;
		if(range == 0)
			game->getSpectators(Range(this->pos,false), tmplist);
		else
			game->getSpectators(Range(this->pos, range, range, range, range, false), tmplist);
		
		std::vector<Creature*> playerlist;
		for(std::vector<Creature*>::const_iterator cit = tmplist.begin(); cit != tmplist.end(); ++cit) {
			const Player* player = dynamic_cast<const Player*>(*cit);

			if(player && player->access == 0 && player != ignoreCreature) {
				if(hasDistanceAttack && validateDistanceAttack(player)){
					playerlist.push_back(*cit);
				}
				else{
					Position closePos;
					getCloseCombatPosition(player->pos, closePos);
					std::list<Position> tmp_route = game->map->getPathTo(this, this->pos, closePos, true, canPushItems);
					if(tmp_route.size() != 0){
						playerlist.push_back(*cit);
					}
				}
			}
		}

		if(playerlist.empty())
			return NULL;

		size_t index = random_range(0, playerlist.size() - 1);
		return playerlist[index];
	}

	return NULL;
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

	if(attackedCreature != 0) {
		long long delay = 0;
		int stepDuration = 0;

		Tile *tile =game->getTile(this->pos.x, this->pos.y, this->pos.z);
		if(tile && tile->ground) {
			int groundid = tile->ground->getID();

			uint8_t stepspeed = Item::items[groundid].speed;
			if(stepspeed != 0) {
				stepDuration = this->getStepDuration(stepspeed);

/*
#if __DEBUG__
				std::cout << "ground id: "<< (int)groundid << ", ground speed: " << (int)stepspeed << ", stepDuration: "<< (int)stepDuration << std::endl;
#endif
*/

				if(this->lastmove != 0) {
					delay = (((long long)(this->lastmove)) + ((long long)(stepDuration))) - ((long long)(OTSYS_TIME()));
				}
			}
		}

		if(delay > 0) {
/*
#if __DEBUG__
			std::cout << "Delaying "<< this->getName() << " --- " << delay << std::endl;		
#endif
*/
			newThinkTicks = (int)delay;
			int ret = oldThinkTicks;
			oldThinkTicks = newThinkTicks;
			return ret;
			//return (int)delay;
		}

		this->lastmove = OTSYS_TIME();

		bool isRouteValid = true;
		if(!game->map->isPathValid(this, route, canPushItems)) {
			calcMovePosition();
			isRouteValid = false;
		}

		if(runawayHealth > 0) {
			if(this->health <= runawayHealth) {
				if(!isfleeing) {
					isfleeing = true;
					calcMovePosition();
				}
			}
			else if(isfleeing) {
				isfleeing = false;
				calcMovePosition();
			}
		}

		if(getCurrentDistanceToTarget() == 1 && rand() % staticAttack == 0){
			randMovePosition();
			isRouteValid = false;
		}
		
		if(changeTargetChance > rand()*10000/(RAND_MAX+1)){
			attackedCreature = 0;
			Creature *newtarget = findTarget(3);
			if(newtarget){
				attackedCreature = 0;
				isRouteValid = false;
				OnCreatureEnter(newtarget);
			}
		}
		
		doMoveTo(moveToPos, isRouteValid);

		newThinkTicks = stepDuration;
		int ret = oldThinkTicks;
		oldThinkTicks = newThinkTicks;
		return ret;
	}

	newThinkTicks = 0;
	return oldThinkTicks;
}

int Monster::getTargetDistance()
{
	if(isfleeing) {
		return 8;
	}

	return targetDistance;
}

int Monster::getCurrentDistanceToTarget()
{
	return std::max(std::abs(pos.x - targetPos.x), std::abs(pos.y - targetPos.y));
}

void Monster::calcMovePosition()
{
	int currentdist = getCurrentDistanceToTarget();
	if((currentdist == getTargetDistance() && hasDistanceAttack && validateDistanceAttack(targetPos))
		|| (isfleeing && currentdist >= getTargetDistance()))
		return;

	int distance = getTargetDistance();
	if(distance > 1 && hasDistanceAttack) {

		int prevdist = 0;
		int minwalkdist = 0;
		int d = distance;

		for (int y = targetPos.y - d; y <= targetPos.y + d; ++y)
		{
			for (int x = targetPos.x - d; x <= targetPos.x + d; ++x)
			{
				if((targetPos.x == x && targetPos.y == y))
					continue;

				int dist = std::max(std::abs(targetPos.x - x), std::abs(targetPos.y - y));
				Position tmppos(x, y, pos.z);
				int walkdist = std::max(std::abs(pos.x - x), std::abs(pos.y - y));

				if(dist <= distance &&
					((dist > prevdist || (dist == prevdist && walkdist > 0 && walkdist < minwalkdist)))) {

					if(!game->map->canThrowItemTo(tmppos, targetPos, false, true))
						continue;
					
					if(!(this->pos.x == x && this->pos.y == y)){
						if(!canMoveTo(x,y,pos.z))
							continue;
					}
/*				
#ifdef __DEBUG__
					std::cout << "CalcMovePosition()" << ", x: " << x << ", y: " << y  << std::endl;
#endif
*/

					minwalkdist = walkdist;
					prevdist = dist;
					moveToPos.x = x;
					moveToPos.y = y;
					moveToPos.z = pos.z;
				}
			}
		}
	}
	//Close combat
	else if(currentdist > distance || currentdist == 0 || (distance > 1 && !hasDistanceAttack)) {
		getCloseCombatPosition(targetPos, moveToPos);
	}
}

void Monster::getCloseCombatPosition(const Position &target, Position &dest)
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
				
			if(dist <= prevdist || (prevdist == 0)) {
				if(!canMoveTo(x,y,pos.z))
					continue;
				
				if(dist < prevdist)
					positionList.clear();
				
				prevdist = dist;
				tmpPos.x = x;
				tmpPos.y = y;
				tmpPos.z = pos.z;
				
				positionList.push_back(tmpPos);
			}
		}
	}

	if(positionList.empty()){
		dest = target;
		return;
	}
	
	size_t index = random_range(0, positionList.size() - 1);
	dest = positionList[index];
}

void Monster::randMovePosition()
{
	//only close combat	
	std::vector<Position> positionList;
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
		return;

	size_t index = random_range(0, positionList.size() - 1);
	moveToPos = positionList[index];
}

bool Monster::isInRange(const Position &p)
{
	return ((std::abs(p.x - this->pos.x) <= 9) && (std::abs(p.y - this->pos.y) <= 7) &&
		(p.z == this->pos.z));
}

void Monster::onThingMove(const Creature *creature, const Thing *thing,
	const Position *oldPos, unsigned char oldstackpos, unsigned char oldcount, unsigned char count)
{
	const Creature* moveCreature = dynamic_cast<const Creature *>(thing);
	
	if(moveCreature) {
		if(isInRange(moveCreature->pos) && isInRange(*oldPos)) {
			//Creature just moving around in-range
			if(attackedCreature == creature->getID()) {
				targetPos = creature->pos;
			}
			else if(attackedCreature == 0){
				OnCreatureEnter(creature);
			}
		}
		else if(isInRange(moveCreature->pos) && !isInRange(*oldPos)) {
			OnCreatureEnter(creature);
		}
		else if(!isInRange(moveCreature->pos) && isInRange(*oldPos)) {
			OnCreatureLeave(creature);
		}

		if(attackedCreature != 0 && moveCreature != this) {
			//Update move position
			calcMovePosition();
		}
	}
	/*else if(attackedCreature == 0){
		const Item* moveItem = dynamic_cast<const Item *>(thing);
		if(moveItem && moveItem->isBlocking()){
			OnCreatureLeave(NULL);
		}
	}*/
}

void Monster::onCreatureAppear(const Creature *creature)
{
	if(isInRange(creature->pos)) {
		OnCreatureEnter(creature);
	}
}

void Monster::onCreatureDisappear(const Creature *creature, unsigned char stackPos, bool tele)
{
	OnCreatureLeave(creature);
}

void Monster::onThingDisappear(const Thing* thing, unsigned char stackPos){
	const Creature *creature = dynamic_cast<const Creature*>(thing);
	if(creature)
		OnCreatureLeave(creature);
}

void Monster::onThingTransform(const Thing* thing,int stackpos){
	if(attackedCreature == 0)
		OnCreatureLeave(NULL);
}

void Monster::onThingAppear(const Thing* thing){
	const Creature *creature = dynamic_cast<const Creature*>(thing);
	if(creature && isInRange(creature->pos)){
		OnCreatureEnter(creature);
	}
}

void Monster::onTeleport(const Creature *creature, const Position *oldPos, unsigned char oldstackpos)
{
	if(isInRange(creature->pos) && isInRange(*oldPos)) {
		//Creature just moving around in-range
		if(attackedCreature == creature->getID()) {
			targetPos = creature->pos;
		}
		else if(attackedCreature == 0){
			OnCreatureEnter(creature);
		}
	}
	else if(!isInRange(*oldPos) && isInRange(creature->pos)) {
		OnCreatureEnter(creature);
	}
	else if(isInRange(*oldPos) && !isInRange(creature->pos)) {
		OnCreatureLeave(creature);
	}

	if(attackedCreature != 0) {
		//Update move position
		calcMovePosition();
	}
}

void Monster::OnCreatureEnter(const Creature *creature)
{
	if(attackedCreature == 0) {
		const Player *player = dynamic_cast<const Player*>(creature);
		if(player && player->access == 0) {
			attackedCreature = player->getID();
			targetPos = player->pos;

			//Update move position
			calcMovePosition();
			if(!this->eventCheck){
				this->eventCheck = game->addEvent(makeTask(500, std::bind2nd(std::mem_fun(&Game::checkCreature), getID())));
			}
			if(!this->eventCheckAttacking){
				this->eventCheckAttacking = game->addEvent(makeTask(500, std::bind2nd(std::mem_fun(&Game::checkCreatureAttacking), getID())));
			}
			//game->addEvent(makeTask(500, std::bind2nd(std::mem_fun(&Game::checkCreature), getID())));
			//game->addEvent(makeTask(500, std::bind2nd(std::mem_fun(&Game::checkCreatureAttacking), getID())));
		}
	}
}

void Monster::OnCreatureLeave(const Creature *creature)
{
	if(creature && attackedCreature == creature->getID()) {
		attackedCreature = 0;

		targetPos.x = 0;
		targetPos.y = 0;
		targetPos.z = 0;
	}
	Creature* targetCreature = NULL;
	if(attackedCreature == 0 && (targetCreature = findTarget(0, creature))){
		OnCreatureEnter(targetCreature);
	}
}

void Monster::setAttackedCreature(unsigned long id)
{
	attackedCreature = id;
}

std::string Monster::getDescription(bool self) const
{
  std::stringstream s;
	std::string str;
	
	str = getName();
	std::transform(str.begin(), str.end(), str.begin(), tolower);

	s << "You see a " << str << ".";
	str = s.str();

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
	for(std::vector<Item*>::iterator cit = lootItems.begin(); cit != lootItems.end(); ++cit) {		
		corpse->addItem(*cit);
	}
	//corpse->addItem(Item::CreateItem(2392));
}

bool Monster::doAttacks(Player* attackedPlayer)
{
	bool ret = false;

	bool trymelee = getCurrentDistanceToTarget() <= 1;
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
		if(timeprobsystem.onTick(500)) {
			if(!hasmelee || (hasmelee && paIt->first->fighttype == FIGHT_MELEE)) {
				curPhysicalAttack = paIt->first;
				game->creatureMakeDamage(this, attackedPlayer, getFightType());
			}
		}
	}

	if(exhaustedTicks <= 0) {
		for(RuneAttackSpells::iterator raIt = runeSpells.begin(); raIt != runeSpells.end(); ++raIt) {
			for(TimeProbabilityClassVec::iterator asIt = raIt->second.begin(); asIt != raIt->second.end(); ++asIt) {
				TimeProbabilityClass& timeprobsystem = *asIt;
				if(timeprobsystem.onTick(500)) {

					std::map<unsigned short, Spell*>::iterator rit = spells.getAllRuneSpells()->find(raIt->first);
					if( rit != spells.getAllRuneSpells()->end() ) {
						bool success = rit->second->getSpellScript()->castSpell(this, attackedPlayer->pos, "");

						if(success) {
							ret = true;
							exhaustedTicks = timeprobsystem.getExhaustion();
							if(exhaustedTicks > 0)
								return true;
						}
					}
				}
			}
		}

		for(InstantAttackSpells::iterator iaIt = instantSpells.begin(); iaIt != instantSpells.end(); ++iaIt) {
			for(TimeProbabilityClassVec::iterator asIt = iaIt->second.begin(); asIt != iaIt->second.end(); ++asIt) {
				TimeProbabilityClass& timeprobsystem = *asIt;
				if(timeprobsystem.onTick(500)) {

					std::map<std::string, Spell*>::iterator rit = spells.getAllSpells()->find(iaIt->first);
					if( rit != spells.getAllSpells()->end() ) {
						bool success = rit->second->getSpellScript()->castSpell(this, this->pos, "");

						if(success) {
							ret = true;
							exhaustedTicks = timeprobsystem.getExhaustion();
							if(exhaustedTicks > 0)
								return true;
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
	if (attackedCreature != 0) {
		this->eventCheckAttacking = game->addEvent(makeTask(500, std::bind2nd(std::mem_fun(&Game::checkCreatureAttacking), getID())));

		exhaustedTicks -= 500;

		if(exhaustedTicks < 0)
			exhaustedTicks = 0;

		Player *attackedPlayer = dynamic_cast<Player*>(game->getCreatureByID(this->attackedCreature));
		if (attackedPlayer) {
			Tile* fromtile = game->map->getTile(this->pos.x, this->pos.y, this->pos.z);
			if (!attackedPlayer->isAttackable() == 0 && fromtile && fromtile->isPz() && this->access == 0) {
				return;
			}
			else {
				if (attackedPlayer != NULL && attackedPlayer->health > 0) {
					if(validateDistanceAttack(attackedPlayer)){
						doAttacks(attackedPlayer);
					}
					else{
						//select other target
						OnCreatureLeave(attackedPlayer);
					}
				}
			}
		}
		else{
			this->attackedCreature = 0;
		}
	}
}

void Monster::doMoveTo(const Position& destpos, bool isRouteValid)
{
	//Update look direction
	if(destpos == this->pos) {
		int dx = targetPos.x - this->pos.x;
		int dy = targetPos.y - this->pos.y;
		
		Direction newdir = this->getDirection();

		//SE
		if( dx < 0 && dy < 0)
			newdir = WEST;
		//SW
		else if(dx > 0 && dy < 0)
			newdir = EAST;
		//NW
		else if(dx > 0 && dy > 0)
			newdir = EAST;
		//NE
		else if(dx < 0 && dy > 0)
			newdir = WEST;
		//S
		else if(dx == 0 && dy < 0)
			newdir = NORTH;
		//W
		else if(dx > 0 && dy == 0)
			newdir = EAST;
		//N
		else if(dx == 0 && dy > 0)
			newdir = SOUTH;
		//E
		else if(dx < 0 && dy == 0)
			newdir = WEST;
	
		if(newdir != this->getDirection()) {
			game->creatureTurn(this, newdir);
		}

		return;
	}

	//if(route.size() == 0 || route.back() != destpos || route.front() != this->pos){
	if(route.size() == 0 || route.back() != destpos || !isRouteValid /*!game->map->isPathValid(this, route)*/){
		route = game->map->getPathTo(this, this->pos, destpos, true, canPushItems);
	}

	if(route.size() == 0){
		//still no route
		if(attackedCreature != 0){
			Player *attackedPlayer = dynamic_cast<Player*>(game->getCreatureByID(attackedCreature));
			if(attackedPlayer){
				if(hasDistanceAttack && validateDistanceAttack(attackedPlayer)){
					//stop here and attack
					moveToPos = this->pos;
					//TODO: invalidate route in other way
					route = game->map->getPathTo(this, this->pos, moveToPos, true, canPushItems);
				}
				else{
					//select other target
					OnCreatureLeave(attackedPlayer);
				}
			}
		}
		return;
	}
	else
		route.pop_front();

	Position nextStep = route.front();

	//route.pop_front();
	
	int dx = nextStep.x - this->pos.x;
	int dy = nextStep.y - this->pos.y;
	
	if(canPushItems){
		//move/destroy blocking moveable items
		Tile *tile =game->getTile(nextStep.x,nextStep.y ,this->pos.z);
		int countItems;
		if(tile){
			countItems = 0;
			while(Item* blockItem = tile->getMoveableBlockingItem()){
				if(countItems < 2){
					if(!monsterMoveItem(blockItem, 3)){
						//destroy it
						game->removeThing(NULL, blockItem->pos, blockItem);
					}
				}
				else{
					//destroy items directly
					game->removeThing(NULL, blockItem->pos, blockItem);
				}
				countItems++;
			}
		}
	}
	
	this->game->thingMove(this, this,this->pos.x + dx, this->pos.y + dy, this->pos.z, 1);
}

bool Monster::monsterMoveItem(Item* item, int radius)
{
	Position centerPos = item->pos;
	Position tryPos;
	//int try_x, try_y;
	int itemCount;
	if(item->isStackable()){
		itemCount = item->getItemCountOrSubtype();
	}
	else{
		itemCount = 1;
	}
	//try random position in raiuds
	tryPos.z = item->pos.z;
	for(int i = 0; i < 4*radius; i++){
		tryPos.x = item->pos.x + rand() % radius;
		tryPos.y = item->pos.y + rand() % radius;
		if(game->map->canThrowItemTo(item->pos,tryPos)){
			game->thingMove(this, item ,tryPos.x, tryPos.y, tryPos.z, itemCount);
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
			t->creatures.size() ||
			t->floorChange() ||
			t->getTeleportItem()){
		return false;
	}
	else{
		return true;
	}
}

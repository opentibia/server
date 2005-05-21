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

extern Spells spells;

template<class Monster> typename AutoList<Monster>::list_type AutoList<Monster>::list;
template<class Monster> typename AutoID<Monster>::list_type AutoID<Monster>::list;
template<class Monster> unsigned long AutoID<Monster>::count = Monster::min_id;

Monster::Monster(const char *name, Game* game) : 
  AutoID<Monster>()
 ,AutoList<Monster>(id)
 ,Creature(name, id)
{
	//std::cout << "Monster constructor " << (unsigned long)this  <<std::endl;
	useCount = 0;
	oldThinkTicks = 0;
	loaded = false;
	isfleeing = false;
	this->game = game;
	curPhysicalAttack = NULL;

	targetDistance = 1;
	runawayHealth = 0;
	pushable = true;

	std::string filename = "data/monster/" + std::string(name) + ".xml";
	std::transform(filename.begin(), filename.end(), filename.begin(), tolower);
	xmlDocPtr doc = xmlParseFile(filename.c_str());
	if (doc){
		loaded = true;
		xmlNodePtr root, p, tmp;
		root = xmlDocGetRootElement(doc);

		if (xmlStrcmp(root->name,(const xmlChar*) "monster")){
		//TODO: use exceptions here
		std::cerr << "Malformed XML" << std::endl;
		}

		p = root->children;

		if ((const char*)xmlGetProp(root, (const xmlChar *)"name")) {
			monstername = (const char*)xmlGetProp(root, (const xmlChar *)"name");
		}

		if ((const char*)xmlGetProp(root, (const xmlChar *)"experience")) {
			this->experience = atoi((const char*)xmlGetProp(root, (const xmlChar *)"experience"));
		}

		if ((const char*)xmlGetProp(root, (const xmlChar *)"pushable")) {
			this->pushable = (bool)atoi((const char*)xmlGetProp(root, (const xmlChar *)"pushable"));
		}

		if ((const char*)xmlGetProp(root, (const xmlChar *)"level")) {
			level = atoi((const char*)xmlGetProp(root, (const xmlChar *)"level"));
			setNormalSpeed();
			maglevel = atoi((const char*)xmlGetProp(root, (const xmlChar *)"maglevel"));
		}

		while (p)
		{
			const char* str = (char*)p->name;
			
			if (strcmp(str, "health") == 0) {
				this->health = atoi((const char*)xmlGetProp(p, (const xmlChar *)"now"));
				this->healthmax = atoi((const char*)xmlGetProp(p, (const xmlChar *)"max"));
			}
			if (strcmp(str, "combat") == 0) {
				this->targetDistance = std::max(1, atoi((const char*)xmlGetProp(p, (const xmlChar *)"targetdistance")));
				this->runawayHealth = atoi((const char*)xmlGetProp(p, (const xmlChar *)"runonhealth"));
			}
			else if (strcmp(str, "look") == 0) {
				this->looktype = atoi((const char*)xmlGetProp(p, (const xmlChar *)"type"));
				this->lookmaster = this->looktype;
				if ((const char*)xmlGetProp(p, (const xmlChar *)"head"))
					this->lookhead = atoi((const char*)xmlGetProp(p, (const xmlChar *)"head"));

				if ((const char*)xmlGetProp(p, (const xmlChar *)"body"))
					this->lookbody = atoi((const char*)xmlGetProp(p, (const xmlChar *)"body"));

				if ((const char*)xmlGetProp(p, (const xmlChar *)"legs"))
					this->looklegs = atoi((const char*)xmlGetProp(p, (const xmlChar *)"legs"));

				if ((const char*)xmlGetProp(p, (const xmlChar *)"feet"))
					this->lookfeet = atoi((const char*)xmlGetProp(p, (const xmlChar *)"feet"));

				if ((const char*)xmlGetProp(p, (const xmlChar *)"corpse"))
					this->lookcorpse = atoi((const char*)xmlGetProp(p, (const xmlChar *)"corpse"));
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

						if((const char*)xmlGetProp(tmp, (const xmlChar *)"exhaustion"))
							exhaustionTicks = atoi((const char*)xmlGetProp(tmp, (const xmlChar *)"exhaustion"));

						if((const char*)xmlGetProp(tmp, (const xmlChar *)"cycleticks"))
							cycleTicks = atoi((const char*)xmlGetProp(tmp, (const xmlChar *)"cycleticks"));

						if((const char*)xmlGetProp(tmp, (const xmlChar *)"probability"))
							probability = atoi((const char*)xmlGetProp(tmp, (const xmlChar *)"probability"));

						TimeProbabilityClass timeprobsystem(cycleTicks, probability, exhaustionTicks);

						std::string attacktype = (const char*)xmlGetProp(tmp, (const xmlChar *)"type");

						if(strcmp(attacktype.c_str(), "melee") == 0)
						{
							PhysicalAttackClass* physicalattack = new PhysicalAttackClass();
							
							physicalattack->fighttype = FIGHT_MELEE;
							if(xmlGetProp(tmp, (const xmlChar *)"mindamage")) 
								physicalattack->minWeapondamage = atoi((const char*)xmlGetProp(tmp, (const xmlChar *)"mindamage"));
							
							if(xmlGetProp(tmp, (const xmlChar *)"maxdamage"))
								physicalattack->maxWeapondamage = atoi((const char*)xmlGetProp(tmp, (const xmlChar *)"maxdamage"));

							physicalAttacks[physicalattack] = TimeProbabilityClass(cycleTicks, probability, exhaustionTicks);
						}
						else if(strcmp(attacktype.c_str(), "distance") == 0)
						{
							PhysicalAttackClass* physicalattack = new PhysicalAttackClass();

							physicalattack->fighttype = FIGHT_DIST;
							std::string subattacktype = (const char*)xmlGetProp(tmp, (const xmlChar *)"name");

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

							if(xmlGetProp(tmp, (const xmlChar *)"mindamage"))
								physicalattack->minWeapondamage = atoi((const char*)xmlGetProp(tmp, (const xmlChar *)"mindamage"));
							if(xmlGetProp(tmp, (const xmlChar *)"maxdamage"))
								physicalattack->maxWeapondamage = atoi((const char*)xmlGetProp(tmp, (const xmlChar *)"maxdamage"));
					
							physicalAttacks[physicalattack] = TimeProbabilityClass(cycleTicks, probability, exhaustionTicks);
						}
						else if(strcmp(attacktype.c_str(), "instant") == 0) {
							std::string spellname = (const char*)xmlGetProp(tmp, (const xmlChar *)"name");

							if(spells.getAllSpells()->find(spellname) != spells.getAllSpells()->end())
							{
								instantSpells[spellname].push_back(timeprobsystem);
							}
						}
						else if(strcmp(attacktype.c_str(), "rune") == 0) {
							std::string spellname = (const char*)xmlGetProp(tmp, (const xmlChar *)"name");
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
						std::string immunity = (const char*)xmlGetProp(tmp, (const xmlChar *)"immunity");

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

						if((const char*)xmlGetProp(tmp, (const xmlChar *)"exhaustion"))
							exhaustionTicks = atoi((const char*)xmlGetProp(tmp, (const xmlChar *)"exhaustion"));
						else
							exhaustionTicks = 0;

						if((const char*)xmlGetProp(tmp, (const xmlChar *)"cycleticks"))
							cycleTicks = atoi((const char*)xmlGetProp(tmp, (const xmlChar *)"cycleticks"));
						else
							cycleTicks = 30000;

						if((const char*)xmlGetProp(tmp, (const xmlChar *)"probability"))
							probability = atoi((const char*)xmlGetProp(tmp, (const xmlChar *)"probability"));
						else
							probability = 30;
						
						std::string sentence = (const char*)xmlGetProp(tmp, (const xmlChar *)"sentence");

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
	char *sxml;
	Item *tmpItem;
	while(tmp){
		s_id = 0;
		sxml = (char*)xmlGetProp(tmp, (const xmlChar *) "id");
		if(sxml){
			s_id = atoi(sxml);
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
	
	char *s_count = (char*)xmlGetProp(tmp, (const xmlChar *) "countmax");
	if(s_count) {
		countmax = atoi(s_count);
		if(countmax > 100){			
			countmax = 100;
		}
	}
	else{
		std::cout << "missing countmax for loot id = "<< id << std::endl;
		countmax = 1;
	}
	char* s_chance = (char*)xmlGetProp(tmp, (xmlChar*)"chancemax");
    if (s_chance){
		chancemax = atoi(s_chance);
		if(chancemax > CHANCE_MAX)
			chancemax = 0;
	}
	else{
		std::cout << "missing chancemax for loot id = "<< id << std::endl;
		chancemax = 0;
	}
	s_chance = (char*)xmlGetProp(tmp, (xmlChar*)"chance1");
    if (s_chance){
		chance1 = atoi(s_chance);
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
	char* s_chance = (char*)xmlGetProp(tmp, (xmlChar*)"chance");
    if (s_chance){
		chance = atoi(s_chance);
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
				s_id = atoi((const char*)xmlGetProp(p, (const xmlChar *) "id"));				
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
	//std::cout << "Monster destructor " << (unsigned long)this  <<std::endl;
	physicalAttacks.clear();
}

Creature* Monster::findTarget()
{
	if(attackedCreature == 0) {
		std::vector<Creature*> tmplist;
		game->getSpectators(Range(this->pos, false), tmplist);
		
		std::vector<Creature*> playerlist;
		for(std::vector<Creature*>::const_iterator cit = tmplist.begin(); cit != tmplist.end(); ++cit) {
			const Player* player = dynamic_cast<const Player*>(*cit);

			if(player && player->access == 0) {
				playerlist.push_back(*cit);
			}
		}

		if(playerlist.empty())
			return NULL;

		size_t index = random_range(0, playerlist.size() - 1);
		return tmplist[index];
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
		if(!game->map->isPathValid(this, route)) {
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
	if((currentdist == getTargetDistance() && game->map->canThrowItemTo(this->pos, targetPos, false, true))
		|| (isfleeing && currentdist >= getTargetDistance()))
		return;

	int distance = getTargetDistance();
	if(distance > 1) {

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
					
					if(!(this->pos.x == x && this->pos.y == y)) {
						Tile *t = NULL;
						if((!(t = game->map->getTile(x, y, pos.z))) ||
								t->isBlocking() ||
								t->isPz() || 
								t->creatures.size() ||
								t->floorChange() ||
								t->getTeleportItem())
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
	else if(currentdist > distance || currentdist == 0) {
		//Close combat
		int prevdist = 0;
		for(int y = targetPos.y - 1; y <= targetPos.y + 1; ++y) {
			for(int x = targetPos.x - 1; x <= targetPos.x + 1; ++x) {
				if((targetPos.x == x && targetPos.y == y))
					continue;

				int dist = std::max(std::abs(pos.x - x), std::abs(pos.y - y));

				if(dist < prevdist || (prevdist == 0)) {
					Tile *t;
					if((!(t = game->map->getTile(x, y, pos.z))) || t->isBlocking() || t->isPz() || t->creatures.size())
						continue;
					
					prevdist = dist;
					moveToPos.x = x;
					moveToPos.y = y;
					moveToPos.z = pos.z;
				}
			}
		}

		if(prevdist == 0) {
			moveToPos = targetPos;
		}
	}
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

			game->addEvent(makeTask(500, std::bind2nd(std::mem_fun(&Game::checkCreature), getID())));
			game->addEvent(makeTask(500, std::bind2nd(std::mem_fun(&Game::checkCreatureAttacking), getID())));
		}
	}
}

void Monster::OnCreatureLeave(const Creature *creature)
{
	if(attackedCreature == creature->getID()) {
		attackedCreature = 0;

		targetPos.x = 0;
		targetPos.y = 0;
		targetPos.z = 0;

		Creature* creature = NULL;
		if((creature = findTarget())) {
			OnCreatureEnter(creature);
		}
	}
}

void Monster::setAttackedCreature(unsigned long id)
{
	attackedCreature = id;
}

std::string Monster::getDescription() const
{
  std::stringstream s;
	std::string str;

	s << "You see a " << getName().c_str() << ".";
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
	for(PhysicalAttacks::iterator paIt = physicalAttacks.begin(); paIt != physicalAttacks.end(); ++paIt) {
		if(trymelee && paIt->first->fighttype == FIGHT_MELEE) {
			hasmelee = true;
			break;
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
		game->addEvent(makeTask(500, std::bind2nd(std::mem_fun(&Game::checkCreatureAttacking), getID())));

		exhaustedTicks -= 500;

		if(exhaustedTicks < 0)
			exhaustedTicks = 0;

		Player *attackedPlayer = dynamic_cast<Player*>(game->getCreatureByID(this->attackedCreature));
		if (attackedPlayer) {
			Tile* fromtile = game->map->getTile(this->pos.x, this->pos.y, this->pos.z);
			if (!attackedPlayer->isAttackable() == 0 && fromtile->isPz() && this->access == 0) {
				return;
			}
			else {
				if (attackedPlayer != NULL && attackedPlayer->health > 0) {
					doAttacks(attackedPlayer);
				}
			}
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
		route = this->game->getPathTo(this, this->pos, destpos);
	}

	if(route.size() == 0){
		//still no route
		return;
	}
	else
		route.pop_front();

	Position nextStep = route.front();

	//route.pop_front();
	
	int dx = nextStep.x - this->pos.x;
	int dy = nextStep.y - this->pos.y;

	this->game->thingMove(this, this,this->pos.x + dx, this->pos.y + dy, this->pos.z, 1);
}

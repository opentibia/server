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

#include <string>
#include <sstream>

#include "monster.h"
#include "spells.h"

extern Spells spells;

Monster::Monster(const char *name, Game* game) : 
 Creature(name)
{
	loaded = false;
	this->game = game;
	this->minWeapondamage = 0;
	this->maxWeapondamage = 0;

	//fight_t = FIGHT_NONE;
	disttype = DIST_NONE;

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
		/*
		if ((const char*)xmlGetProp(root, (const xmlChar *)"access")) {
			access = atoi((const char*)xmlGetProp(root, (const xmlChar *)"access"));
		}
		*/
		if ((const char*)xmlGetProp(root, (const xmlChar *)"level")) {
			level = atoi((const char*)xmlGetProp(root, (const xmlChar *)"level"));
			setNormalSpeed();
			//std::cout << level << std::endl;
			maglevel = atoi((const char*)xmlGetProp(root, (const xmlChar *)"maglevel"));
			std::cout << maglevel << std::endl;
		}

		while (p)
		{
			const char* str = (char*)p->name;

			if (strcmp(str, "experience") == 0) {
				this->experience = atoi((const char*)xmlGetProp(p, (const xmlChar *)"experience"));
			}
			
			if (strcmp(str, "health") == 0) {
				this->health = atoi((const char*)xmlGetProp(p, (const xmlChar *)"now"));
				this->healthmax = atoi((const char*)xmlGetProp(p, (const xmlChar *)"max"));
			}

			if (strcmp(str, "look") == 0) {
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
						std::string attacktype = (const char*)xmlGetProp(tmp, (const xmlChar *)"type");

						if(strcmp(attacktype.c_str(), "melee") == 0)
						{
							this->fighttype = FIGHT_MELEE;
							if(xmlGetProp(tmp, (const xmlChar *)"mindamage")) 
								this->minWeapondamage = atoi((const char*)xmlGetProp(tmp, (const xmlChar *)"mindamage"));
							
							if(xmlGetProp(tmp, (const xmlChar *)"maxdamage"))
								this->maxWeapondamage = atoi((const char*)xmlGetProp(tmp, (const xmlChar *)"maxdamage"));
						}
						else if(strcmp(attacktype.c_str(), "distance") == 0)
						{
							this->fighttype = FIGHT_DIST;
							std::string subattacktype = (const char*)xmlGetProp(tmp, (const xmlChar *)"name");

							if(strcmp(subattacktype.c_str(), "bolt") == 0)
								this->disttype = DIST_BOLT;
							else if(strcmp(subattacktype.c_str(), "arrow") == 0)
								this->disttype = DIST_ARROW;
							else if(strcmp(subattacktype.c_str(), "throwingstar") == 0)
								this->disttype = DIST_THROWINGSTAR;
							else if(strcmp(subattacktype.c_str(), "throwingknife") == 0)
								this->disttype = DIST_THROWINGKNIFE;
							else if(strcmp(subattacktype.c_str(), "smallstone") == 0)
								this->disttype = DIST_SMALLSTONE;
							else if(strcmp(subattacktype.c_str(), "largerock") == 0)
								this->disttype = DIST_LARGEROCK;
							else if(strcmp(subattacktype.c_str(), "snowball") == 0)
								this->disttype = DIST_SNOWBALL;
							else if(strcmp(subattacktype.c_str(), "powerbolt") == 0)
								this->disttype = DIST_POWERBOLT;

							if(xmlGetProp(tmp, (const xmlChar *)"mindamage"))
								this->minWeapondamage = atoi((const char*)xmlGetProp(tmp, (const xmlChar *)"mindamage"));
							if(xmlGetProp(tmp, (const xmlChar *)"maxdamage"))
								this->maxWeapondamage = atoi((const char*)xmlGetProp(tmp, (const xmlChar *)"maxdamage"));
						}
						else if(strcmp(attacktype.c_str(), "instant") == 0) {
							std::string spellname = (const char*)xmlGetProp(tmp, (const xmlChar *)"name");
							
							if(spells.getAllSpells()->find(spellname) != spells.getAllSpells()->end())
							{
								instantSpells.push_back(spellname);
							}
						}
						else if(strcmp(attacktype.c_str(), "rune") == 0) {
							/*
							unsigned short id = atoi((const char*)xmlGetProp(tmp, (const xmlChar *)"id"));
							if(spells.getAllRuneSpells()->find(id) != spells.getAllRuneSpells()->end())
							{
								runeSpells.push_back(id);
							}
							*/
							std::string spellname = (const char*)xmlGetProp(tmp, (const xmlChar *)"name");
							std::transform(spellname.begin(), spellname.end(), spellname.begin(), tolower);
							
							std::map<unsigned short, Spell*>::const_iterator rsIt;
							for(rsIt = spells.getAllRuneSpells()->begin(); rsIt != spells.getAllRuneSpells()->end(); ++rsIt) {
								if(strcmp(rsIt->second->getName().c_str(), spellname.c_str()) == 0) {
									runeSpells.push_back(rsIt->first);
									break;
								}
							}
						}
					}

					tmp=tmp->next;
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

					tmp=tmp->next;
				}
			}

			/*
			if (strcmp(str, "loot") == 0) {
				//TODO implement loot
			}
			*/
				
			p = p->next;
		}

		xmlFreeDoc(doc);
	}

}

Monster::~Monster()
{
	//
}

void Monster::onThink()
{
	if(attackedCreature != 0) {
		int ground = game->getTile(this->pos.x, this->pos.y, this->pos.z)->ground.getID();

		long long delay = ((long long)this->lastmove +
			(long long)this->getStepDuration(Item::items[ground].speed)) -
			((long long)OTSYS_TIME());

		if(delay > 0){
#if __DEBUG__     
			std::cout << "Delaying "<< this->getName() << " --- " << delay << std::endl;		
#endif
			
			return;
		}

		this->lastmove = OTSYS_TIME();

		doMoveTo(targetPos);
	}
}

bool Monster::isInRange(const Position &pos)
{
	return ((std::abs(pos.x - this->pos.x) <= 8) && (std::abs(pos.y - this->pos.y) <= 6) &&
		(pos.z == this->pos.z));
}

void Monster::onThingMove(const Creature *creature, const Thing *thing, const Position *oldPos, unsigned char oldstackpos)
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
		/*
		else if(attackedCreature == creature->getID()) {
			targetPos = creature->pos;
		}
		*/
	}
}

void Monster::onCreatureAppear(const Creature *creature)
{
	OnCreatureEnter(creature);
}

void Monster::onCreatureDisappear(const Creature *creature, unsigned char stackPos)
{
	OnCreatureLeave(creature);
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
	/*
	else if(attackedCreature == creature->getID()) {
		targetPos = creature->pos;
	}
	*/
}

void Monster::OnCreatureEnter(const Creature *creature)
{
	if(attackedCreature == 0) {
		const Player *player = dynamic_cast<const Player*>(creature);
		if(player && player->access == 0) {
			attackedCreature = player->getID();
			targetPos = player->pos;

			game->addEvent(makeTask(500, std::bind2nd(std::mem_fun(&Game::checkPlayerAttacking), getID())));
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
	return random_range(0, maxWeapondamage);
}

void Monster::onAttack()
{
	if (attackedCreature != 0) {
		game->addEvent(makeTask(500, std::bind2nd(std::mem_fun(&Game::checkPlayerAttacking), getID())));
		
		if(random_range(0, 100) >= 25)
			return;

		Player *attackedPlayer = dynamic_cast<Player*>(game->getCreatureByID(this->attackedCreature));
		if (attackedPlayer) {
			Tile* fromtile = game->map->getTile(this->pos.x, this->pos.y, this->pos.z);
			if (!attackedPlayer->isAttackable() == 0 && fromtile->isPz() && this->access == 0) {
				return;
			}
			else {
				if (attackedPlayer != NULL && attackedPlayer->health > 0) {
					
					if(runeSpells.size() > 0 &&  random_range(0, 100) >= 50) {
						//Runes
						unsigned short id = random_range(0, (int)runeSpells.size() - 1);

						std::map<unsigned short, Spell*>::iterator rit = spells.getAllRuneSpells()->find(runeSpells[id]);
						if( rit != spells.getAllRuneSpells()->end() ) {
							bool success = rit->second->getSpellScript()->castSpell(this, attackedPlayer->pos, "");
							//ret = success;
						}
					}
					else if(instantSpells.size() > 0 && random_range(0, 100) >= 50) {
						//Instant
						unsigned short id = random_range(0, (int)instantSpells.size() - 1);
						std::map<std::string, Spell*>::iterator sit = spells.getAllSpells()->find(instantSpells[id]);
						if( sit != spells.getAllSpells()->end() ) {
							bool success = sit->second->getSpellScript()->castSpell(this, this->pos, "");
						}
					}
					else if(random_range(0, 100) >= 50)
					{
						game->creatureMakeDamage(this, attackedPlayer, this->getFightType());
					}
				}
			}
		}
	}
}

void Monster::doMoveTo(const Position &target)
{
	if(route.size() == 0 || route.back() != target || route.front() != this->pos){
		route = this->game->getPathTo(this->pos, target);
	}

	if(route.size()==0){
		//still no route, means there is none
		return;
	}
	else
		route.pop_front();

	Position nextStep = route.front();

	route.pop_front();
	
	int dx = nextStep.x - this->pos.x;
	int dy = nextStep.y - this->pos.y;
	this->game->thingMove(this, this,this->pos.x + dx, this->pos.y + dy, this->pos.z);
}

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


#include "attack.h"
#include "creature.h"
#include "player.h"


Attack::Attack():
attack_type(ATTACK_TYPE_NONE),
ownerId(0),
owner(NULL),
manaAttack(false)
{
}


bool Attack::checkManaShield(Creature *attackedCreature, long damage){
	//TODO: check conditions for mana shield
	// player->getCondition(MANA_SHIELD) ?
	if(attackedCreature->manaShieldTicks > 1000 && damage < attackedCreature->getMana()){
		manaAttack = true;
		return true;
	}
	else{
		manaAttack = false;
		return false;
	}
}

void Attack::getDrawInfoManaShield(MagicEffectClasses &me, eATextColor &text_color,eBloodColor &blood_color, subfight_t &shoot){
	me = NM_ME_LOOSE_ENERGY;
	text_color = ATEXT_BLUE;
	shoot = DIST_NONE;
	blood_color = BLOOD_NONE;
}


void Attack::getDrawInfoRace(race_t race ,MagicEffectClasses &me, eATextColor &text_color,eBloodColor &blood_color){
	switch(race){
	case RACE_VENOM:
		text_color = ATEXT_GREEN;
		blood_color = BLOOD_GREEN;
		me = NM_ME_POISEN;
		break;
	case RACE_BLOOD:
		text_color = ATEXT_RED;
		blood_color = BLOOD_RED;
		me = NM_ME_BLOOD;
		break;
	case RACE_UNDEAD:
		text_color = ATEXT_WHITE;
		blood_color = BLOOD_NONE;
		me = NM_ME_HIT_AREA;
		break;
	default:
		text_color = ATEXT_RED;
		blood_color = BLOOD_RED;
		me = NM_ME_BLOOD;
		break;
	}
}


/*   MELEE  ATTACK   */

AttackMelee::AttackMelee():
base_damage(0),
skill(SKILL_NONE),
internal_me(NM_ME_NONE),
attacked_race(RACE_BLOOD),
internal_sq(SQ_COLOR_NONE),
addSkillTry(false)
{	
}

void AttackMelee::initialize(Creature *attacker, Item* weapon){
	if(attacker)
		owner = attacker;
		
	Player *attackerPlayer = dynamic_cast<Player*>(attacker);
	if(!attackerPlayer){
		std::cout << "Error initialize AttackMelee. No Player" << std::endl;
		return;
	}
	
	int damagemax;
	int skill_level;
	if(weapon){
		//assume that weapon is already a weapon
		switch (weapon->getWeaponType()){
		case SWORD:
			skill_level = attackerPlayer->skills[SKILL_SWORD][SKILL_LEVEL];
			skill = SKILL_SWORD;
			break;
		case CLUB:
			skill_level = attackerPlayer->skills[SKILL_CLUB][SKILL_LEVEL];
			skill = SKILL_CLUB;
			break;
		case AXE:
			skill_level = attackerPlayer->skills[SKILL_AXE][SKILL_LEVEL];
			skill = SKILL_AXE;
			break;
		default:
			skill_level = 0;
			skill = 0;
			std::cout << "Error initialize AttackMelee. No skill" << std::endl;
			break;
		}
		damagemax = (skill/20)*Item::items[weapon->getID()].attack + Item::items[weapon->getID()].attack;
	}
	else{
		damagemax = attackerPlayer->skills[SKILL_FIST][SKILL_LEVEL] + 5;
		skill = SKILL_FIST;
	}
	//add conditions?
	//special weapons?
	base_damage = 1+(int)(damagemax*rand()/(1.*RAND_MAX));
}

void AttackMelee::initialize(Creature *attacker, long damage){
	if(attacker)
		owner = attacker;
	
	base_damage = damage;
}


long AttackMelee::getSkillId(){
	if(addSkillTry)
		return skill;
	else
		return SKILL_NONE;
}

bool AttackMelee::addShieldTry(){
	//add shield skill try for each melee attack
	//TODO: add 2 tries max in each turn
	return true;
}

long AttackMelee::getDamage(Creature *attackedCreature, eDamgeType &type){	
	int armor = attackedCreature->getArmor();
	int defense = attackedCreature->getDefense();
	int damage = base_damage;
	if(!owner){
		std::cout << "No owner" << std::endl;
		return 0;
	}
	if((std::abs(owner->pos.x-attackedCreature->pos.x) > 1) ||
		(std::abs(owner->pos.y-attackedCreature->pos.y) > 1) ||
		(owner->pos.z != attackedCreature->pos.z)) {
		internal_me = NM_ME_NONE;
		return 0;
	}
	
	addSkillTry = true;
	attacked_race = attackedCreature->getCreatureType();
	internal_sq = SQ_COLOR_BLACK;
	
	if(owner && owner->access >= GM_MIN_LEVEL){
		damage = 1337;
	}
	else{
		//TODO: change formulas/////////////////////
		//TODO:add conditions if any///////////////
		int probability = rand()%100;
		if(probability < defense){
			damage = 0;
			internal_me = NM_ME_PUFF;
		}
		else{
			damage -= (int)(base_damage*armor/100);
		}
		//////////////////////////////////////////
	}

	
	if(damage > 0 && internal_me == NM_ME_NONE){
		internal_me = NM_ME_DRAW_BLOOD;
	}
	if(checkManaShield(attackedCreature, damage))
		type = DAMAGE_MANA;
	else
		type = DAMAGE_HEALTH;
	
	
	return damage;
}


void AttackMelee::getDrawInfo(MagicEffectClasses &me, eATextColor &text_color,eBloodColor &blood_color, subfight_t &shoot, eSquareColor &sqcolor){
	if(manaAttack){
		getDrawInfoManaShield(me,text_color,blood_color,shoot);
		return;
	}
	MagicEffectClasses local_me;
	getDrawInfoRace(attacked_race, local_me, text_color, blood_color);
	
	if(internal_me == NM_ME_DRAW_BLOOD){
		me = local_me;
	}
	else{
		me = internal_me;
	}
	sqcolor = internal_sq;
	shoot = DIST_NONE;
}


/*  DISTANCE ATTACK   */

AttackDistancePhysical::AttackDistancePhysical():
base_damage(0),
internal_me(NM_ME_NONE),
attacked_race(RACE_BLOOD),
internal_sq(SQ_COLOR_NONE),
internal_block_type(3),
hitchance(0),
internal_shoot(DIST_NONE),
addSkillTry(false),
internal_dist_item(NULL)
{	
}

void AttackDistancePhysical::initialize(Creature *attacker, Item* dist_item){
	if(attacker)
		owner = attacker;
		
	Player *attackerPlayer = dynamic_cast<Player*>(attacker);
	if(!attackerPlayer){
		std::cout << "Error initialize AttackDistancePhysical. No Player" << std::endl;
		return;
	}
	
	int skill = attackerPlayer->skills[SKILL_DIST][SKILL_LEVEL];
	int item_attack = Item::items[dist_item->getID()].attack;
	if(item_attack > 0)
		addSkillTry = true;
	
	int damagemax = (skill/20)*item_attack + item_attack;
	
	switch(dist_item->getWeaponType()){
	case AMO:
		hitchance = 90;
		break;
	default:
		hitchance = 50;
		break;
	}
	
	internal_shoot = dist_item->getSubfightType();
	//add conditions?
	//special ammo?
	
	internal_dist_item = dist_item;
	base_damage = 1+(int)(damagemax*rand()/(1.*RAND_MAX));
}

void AttackDistancePhysical::initialize(Creature *attacker, long damage, subfight_t type, long block_type /*= 3*/){
	if(attacker)
		owner = attacker;
	internal_block_type = block_type;
	internal_shoot = type;
	base_damage = damage;
}


long AttackDistancePhysical::getSkillId(){
	if(addSkillTry)
		return SKILL_DIST;
	else
		return SKILL_NONE;
}

bool AttackDistancePhysical::addShieldTry(){
	//add shield skill try for each melee attack
	//TODO: add 2 tries max in each turn
	return true;
}

long AttackDistancePhysical::getDamage(Creature *attackedCreature, eDamgeType &type){	
	int armor = attackedCreature->getArmor();
	int defense = attackedCreature->getDefense();
	int damage = base_damage;
	if(!owner){
		std::cout << "No owner" << std::endl;
		return 0;
	}
	
	if((std::abs(owner->pos.x-attackedCreature->pos.x) > 7) ||
		(std::abs(owner->pos.y-attackedCreature->pos.y) > 6) ||
		(owner->pos.z != attackedCreature->pos.z)) {
		internal_shoot = DIST_NONE;
		internal_me = NM_ME_NONE;
		return 0;
	}
	
	attacked_race = attackedCreature->getCreatureType();
	
	internal_sq = SQ_COLOR_BLACK;
	if(owner && owner->access >= GM_MIN_LEVEL)
		damage = 1337;
	else{
		if(dynamic_cast<Player*>(owner) && internal_dist_item){
			dynamic_cast<Player*>(owner)->removeDistItem(internal_dist_item);
		}
		//TODO: change formulas/////////////////////
		//TODO:add conditions if any///////////////
		if(rand()%100 < hitchance){ //hit
			damage = (int)((damage*rand()/(RAND_MAX*1.0)));
		}
		else{ //miss
			damage = 0;
			internal_me = NM_ME_PUFF;
		}
		if(damage > 0 && internal_block_type == 3){
			int probability = rand()%100;
			if(probability < defense){
				damage = 0;
				internal_me = NM_ME_PUFF;
			}
			else{
				damage -= (int)(damage*armor/100);
			}
		}
		else if(damage == 0){
			internal_me = NM_ME_PUFF;
		}
		//////////////////////////////////////////
	}
	
	if(damage > 0 && internal_me == NM_ME_NONE){
		internal_me = NM_ME_DRAW_BLOOD;
	}
	
	if(checkManaShield(attackedCreature, damage))
		type = DAMAGE_MANA;
	else
		type = DAMAGE_HEALTH;
	
	
	return damage;
}


void AttackDistancePhysical::getDrawInfo(MagicEffectClasses &me, eATextColor &text_color,eBloodColor &blood_color, subfight_t &shoot, eSquareColor &sqcolor){
	if(manaAttack){
		getDrawInfoManaShield(me,text_color,blood_color,shoot);
		return;
	}
	MagicEffectClasses local_me;
	getDrawInfoRace(attacked_race, local_me, text_color, blood_color);
	
	if(internal_me == NM_ME_DRAW_BLOOD){
		me = local_me;
	}
	else{
		me = internal_me;
	}
	sqcolor = internal_sq;
	shoot = internal_shoot;
}


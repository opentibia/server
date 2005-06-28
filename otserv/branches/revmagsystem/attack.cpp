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
//attack_type(ATTACK_TYPE_NONE),
ownerId(0),
owner(NULL),
manaAttack(false),
base_damage(0),
internal_hit_me(NM_ME_NONE)
{
}


bool Attack::checkManaShield(Creature *attackedCreature, long damage){
	if(attackedCreature->hasCondition(CONDITION_MAGICSHIELD) && damage < attackedCreature->getMana()){
		manaAttack = true;
		return true;
	}
	else{
		manaAttack = false;
		return false;
	}
}

void Attack::getDrawInfoManaShield(MagicEffectClasses &me, eATextColor &text_color,eBloodColor &blood_color){
	me = NM_ME_LOOSE_ENERGY;
	text_color = ATEXT_BLUE;
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
/*
Can be used for: all melee weapons
*/
AttackMelee::AttackMelee():
skill(SKILL_NONE),
attacked_race(RACE_BLOOD),
internal_sq(SQ_COLOR_NONE),
addSkillTry(true)
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
		switch(weapon->getWeaponType()){
		case WEAPON | SWORD:
			skill_level = attackerPlayer->skills[SKILL_SWORD][SKILL_LEVEL];
			skill = SKILL_SWORD;
			break;
		case WEAPON | CLUB:
			skill_level = attackerPlayer->skills[SKILL_CLUB][SKILL_LEVEL];
			skill = SKILL_CLUB;
			break;
		case WEAPON | AXE:
			skill_level = attackerPlayer->skills[SKILL_AXE][SKILL_LEVEL];
			skill = SKILL_AXE;
			break;
		default:
			skill_level = 0;
			skill = 0;
			std::cout << "Error initialize AttackMelee. No skill" << std::endl;
			break;
		}
		damagemax = skill*Item::items[weapon->getID()].attack/20 + Item::items[weapon->getID()].attack;
		internal_hit_me = Item::items[weapon->getID()].hitEffect;
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

long AttackMelee::addShieldTry(){
	//add shield skill try for each melee attack
	//TODO: add 2*2 tries max in each turn
	return 1;
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
		addSkillTry = false;
		internal_hit_me = NM_ME_NONE;
		return 0;
	}
	
	attacked_race = attackedCreature->getCreatureType();
	internal_sq = SQ_COLOR_BLACK;
	
	if(owner && owner->access >= GM_MIN_LEVEL){
		damage = 1337;
	}
	else{
		//TODO: check immunities
		//TODO: change formulas/////////////////////
		int probability = rand()%100;
		if(probability < defense){
			damage = 0;
			internal_hit_me = NM_ME_PUFF;
		}
		
		if(damage > 0){
			probability = rand()%100;
			int reduce_damage = (probability*armor*damage)/3000;
			if(reduce_damage >= damage){
				damage = 0;
				internal_hit_me = NM_ME_BLOCKHIT;
			}
			else{
				damage -= reduce_damage;
			}
		}
		//////////////////////////////////////////
		/*if(damage > 0){
			//TODO:check amulets ///////////////	
			//TODO:add conditions if any///////////////	
		}*/
	}
	
	if(checkManaShield(attackedCreature, damage))
		type = DAMAGE_MANA;
	else
		type = DAMAGE_HEALTH;
	
	
	return damage;
}


void AttackMelee::getDrawInfo(MagicEffectClasses &me, eATextColor &text_color,eBloodColor &blood_color, MagicEffectClasses &blood_effect, subfight_t &shoot, eSquareColor &sqcolor){
	
	me = internal_hit_me;
	sqcolor = internal_sq;
	shoot = DIST_NONE;
	
	if(manaAttack){
		getDrawInfoManaShield(blood_effect,text_color,blood_color);
	}
	else{
		getDrawInfoRace(attacked_race, blood_effect, text_color, blood_color);
	}
}


/*  DISTANCE ATTACK   */
/*
Can be used for: Items: arrows, bolts, magic distance attacks using weapons(staff),
	all spells/runes that attacks only one tile
*/
AttackDistance::AttackDistance():
attacked_race(RACE_BLOOD),
internal_sq(SQ_COLOR_NONE),
internal_block_type(3),
hitchance(0),
internal_shoot(DIST_NONE),
addSkillTry(true),
internal_dist_item(NULL)
{	
}

void AttackDistance::initialize(Creature *attacker, Item* dist_item){
	if(attacker)
		owner = attacker;
		
	Player *attackerPlayer = dynamic_cast<Player*>(attacker);
	if(!attackerPlayer || !dist_item){
		std::cout << "Error initialize AttackDistance. No Player or ammo" << std::endl;
		return;
	}
	
	int item_attack = Item::items[dist_item->getID()].attack;
	if(item_attack > 0)
		addSkillTry = true;
	
	int skill = 0;
	switch(dist_item->getWeaponType()){
	case AMO | AMO_DIST:
		skill = attackerPlayer->skills[SKILL_DIST][SKILL_LEVEL];
		hitchance = 90;
		break;
	case WEAPON | MAGIC:
	case AMO | AMO_MAGIC:
		skill = attackerPlayer->maglevel + attackerPlayer->level/2;
		internal_block_type = ATTACK_DIST_BLOCK_ARMOR;
		hitchance = 100;
		addSkillTry = false;
		break;
	case WEAPON | DIST:
		skill = attackerPlayer->skills[SKILL_DIST][SKILL_LEVEL];		
		hitchance = 50;
		break;
	}
	
	int damagemax = skill*item_attack/20 + item_attack;
	
	internal_shoot = dist_item->getSubfightType();
	//add conditions?
	//special ammo?
	internal_hit_me = Item::items[dist_item->getID()].hitEffect;
	internal_dist_item = dist_item;
	base_damage = 1+(int)(damagemax*rand()/(1.*RAND_MAX));
}

void AttackDistance::initialize(Creature *attacker, long damage, subfight_t type, MagicEffectClasses hitEffect /*= NM_ME_NONE*/, long block_type /*= 3*/, eSquareColor sqcolor /*= SQ_COLOR_BLACK*/){
	if(attacker)
		owner = attacker;
	internal_block_type = block_type;
	internal_hit_me = hitEffect;
	internal_shoot = type;
	internal_sq = sqcolor;
	base_damage = damage; //random?
}


long AttackDistance::getSkillId(){
	if(addSkillTry)
		return SKILL_DIST;
	else
		return SKILL_NONE;
}

long AttackDistance::addShieldTry(){
	//add shield skill try for each melee attack
	//TODO: add 2*2 tries max in each turn
	return 1;
}

long AttackDistance::getDamage(Creature *attackedCreature, eDamgeType &type){	
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
		internal_hit_me = NM_ME_NONE;
		addSkillTry = false;
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
		//TODO: check immunities
		//TODO: change formulas/////////////////////
		if(rand()%100 > hitchance){ //miss
			damage = 0;
			internal_hit_me = NM_ME_PUFF;
		}
		if(damage > 0 && (internal_block_type & ATTACK_DIST_BLOCK_SHIELD)){
			int probability = rand()%100;
			if(probability < defense){
				damage = 0;
				internal_hit_me = NM_ME_PUFF;
			}
		}
		if(damage > 0 && (internal_block_type & ATTACK_DIST_BLOCK_ARMOR)){
			int probability = rand()%100;
			int reduce_damage = (probability*armor*damage)/3000;
			if(reduce_damage > damage){
				damage = 0;
				internal_hit_me = NM_ME_BLOCKHIT;
			}
			else{
				damage -= reduce_damage;
			}
		}
		//////////////////////////////////////////
		/*if(damage > 0){
			//TODO:check amulets ///////////////	
			//TODO:add conditions if any///////////////	
		}*/
	}
	
	if(checkManaShield(attackedCreature, damage))
		type = DAMAGE_MANA;
	else
		type = DAMAGE_HEALTH;
	
	return damage;
}


void AttackDistance::getDrawInfo(MagicEffectClasses &me, eATextColor &text_color,eBloodColor &blood_color, MagicEffectClasses &blood_effect, subfight_t &shoot, eSquareColor &sqcolor){
	
	me = internal_hit_me;
	sqcolor = internal_sq;
	shoot = internal_shoot;
	
	if(manaAttack){
		getDrawInfoManaShield(blood_effect,text_color,blood_color);
	}
	else{
		getDrawInfoRace(attacked_race, blood_effect, text_color, blood_color);
	}
}


/* MAGIC ATTACK */
AttackMagic::AttackMagic():
attacked_race(RACE_BLOOD),
internal_block_type(3),
magicType(ATTACK_MAGIC_NONE),
internal_text_color(ATEXT_RED),
drawBlood(true)
{	
}

void AttackMagic::initialize(Creature *attacker, long damage,eAttackMagicTypes _magicType, long block_type /*= 3*/){
	if(attacker)
		owner = attacker;
	
	internalInitialize(damage,_magicType, block_type);
}

void AttackMagic::initialize(unsigned long _ownerid, long damage,eAttackMagicTypes _magicType, long block_type /*= 3*/){
	ownerId = _ownerid;
	internalInitialize(damage,_magicType, block_type);
}

void AttackMagic::internalInitialize(long damage,eAttackMagicTypes _magicType, long block_type){
	
	internal_block_type = block_type;
	magicType = _magicType;
	switch(magicType){
	case ATTACK_MAGIC_NONE:
		internal_hit_me = NM_ME_NONE;
		drawBlood = false;
		break;
	case ATTACK_MAGIC_ENERGY:
		internal_hit_me = NM_ME_ENERGY_DAMAGE;
		//internal_text_color = ATEXT_BLUE;  //??
		drawBlood = false;
		break;
	case ATTACK_MAGIC_POISON:
		internal_hit_me = NM_ME_POISEN_RINGS;
		internal_text_color = ATEXT_GREEN;
		drawBlood = false;
		break;
	case ATTACK_MAGIC_FIRE:
		internal_hit_me = NM_ME_HITBY_FIRE;
		drawBlood = true;
		break;
	case ATTACK_MAGIC_EXPLOSION:
		internal_hit_me = NM_ME_EXPLOSION_DAMAGE;
		drawBlood = true;
		break;
	case ATTACK_MAGIC_SD:
		internal_hit_me = NM_ME_MORT_AREA;
		drawBlood = false;
		break;
	default:
		break;
	}
	base_damage = damage;
}

long AttackMagic::getSkillId(){
	return SKILL_NONE;
}

long AttackMagic::addShieldTry(){
	return 0;
}

long AttackMagic::getDamage(Creature *attackedCreature, eDamgeType &type){	
	int armor = attackedCreature->getArmor();
	int defense = attackedCreature->getDefense();
	int damage = base_damage;
	
	attacked_race = attackedCreature->getCreatureType();
	
	if(owner && owner->access >= GM_MIN_LEVEL){
		damage = 1337;
	}
	else{
		//TODO: check immunities
		//TODO: change formulas/////////////////////
		if(damage > 0 && (internal_block_type & ATTACK_DIST_BLOCK_SHIELD)){
			int probability = rand()%100;
			if(probability < defense){
				damage = 0;
				internal_hit_me = NM_ME_PUFF;
			}
		}
		if(damage > 0 && (internal_block_type & ATTACK_DIST_BLOCK_ARMOR)){
			int probability = rand()%100;
			int reduce_damage = (probability*armor*damage)/3000;
			if(reduce_damage > damage){
				damage = 0;
				internal_hit_me = NM_ME_BLOCKHIT;
			}
			else{
				damage -= reduce_damage;
			}
		//////////////////////////////////////////
		}
		/*if(damage > 0){
			//TODO:check amulets ///////////////	
			//TODO:add conditions if any///////////////	
		}*/
	}
	
	if(checkManaShield(attackedCreature, damage))
		type = DAMAGE_MANA;
	else
		type = DAMAGE_HEALTH;
	
	
	return damage;
}


void AttackMagic::getDrawInfo(MagicEffectClasses &me, eATextColor &text_color,eBloodColor &blood_color, MagicEffectClasses &blood_effect, subfight_t &shoot, eSquareColor &sqcolor){
	
	me = internal_hit_me;
	sqcolor = SQ_COLOR_NONE;
	shoot = DIST_NONE;
	
	if(manaAttack){
		getDrawInfoManaShield(blood_effect,text_color,blood_color);
	}
	else{
		eATextColor local_text;
		eBloodColor local_blood;
		MagicEffectClasses local_blood_effect;
		getDrawInfoRace(attacked_race, local_blood_effect, local_text, local_blood);
		if(drawBlood){
			blood_color = local_blood;
			blood_effect = local_blood_effect;
		}
		else{
			blood_color = BLOOD_NONE;
			blood_effect = NM_ME_NONE;
		}
		text_color = internal_text_color;
	}
}




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


#include "definitions.h"

#include <string>
#include <sstream>
#include <iostream>
#include <algorithm>

using namespace std;

#include <stdlib.h>

#include "protocol.h"
#include "player.h"
#include "luascript.h"
#include "networkmessage.h"

extern LuaScript g_config;

template<class Player> typename AutoList<Player>::list_type AutoList<Player>::list;
template<class Player> typename AutoID<Player>::list_type AutoID<Player>::list;
template<class Player> unsigned long AutoID<Player>::count = Player::min_id;

Player::Player(const char *name, Protocol *p) :
  AutoID<Player>()
 ,AutoList<Player>(id)
 ,Creature(name, id)
{	
  client     = p;
  client->setPlayer(this);
	/*
	exhaustedTicks  = 0;
	pzLocked = false;
	inFightTicks = 0;
	*/
	looktype   = PLAYER_MALE_1;
	voc        = 0;
  cap        = 300;
  mana       = 0;
  manamax    = 0;
  manaspent  = 0;
  this->name= name;
  food       = 0;

  level      = 1;
  experience = 180;

  maglevel   = 20;

  access     = 0;
  cancelMove = false;
  SendBuffer = false;
  npings = 0;
  internal_ping = 0;
  fightMode = followMode = 0;
  for(int i = 0; i < 7; i++)
  {
    skills[i][SKILL_LEVEL] = 1;
    skills[i][SKILL_TRIES] = 0;
	skills[i][SKILL_PERCENT] = 0;
	for(int j=0;j<2;j++){
		SkillAdvanceCache[i][j].level = 0;
		SkillAdvanceCache[i][j].voc = 0;
		SkillAdvanceCache[i][j].tries = 0;
	}
  }

	lastSentStats.health = 0;
	lastSentStats.healthmax = 0;
	lastSentStats.cap = 0;
	lastSentStats.experience = 0;
	lastSentStats.level = 0;
	lastSentStats.mana = 0;
	lastSentStats.manamax = 0;
	lastSentStats.manaspent = 0;
	lastSentStats.maglevel = 0;
	level_percent = 0;
	maglevel_percent = 0;

  //set item pointers to NULL
	for(int i = 0; i < 11; i++)
		items[i] = NULL;

  useCount = 0;
  
  
  CapGain[0]  = 10;     //for level advances
  CapGain[1]  = 10;     //e.g. Sorcerers will get 10 Cap with each level up
  CapGain[2]  = 10;     
  CapGain[3]  = 20;
  CapGain[4]  = 25;
  
  ManaGain[0] = 5;      //for level advances
  ManaGain[1] = 30;
  ManaGain[2] = 30;
  ManaGain[3] = 15;
  ManaGain[4] = 5;
  
  
  HPGain[0]   = 5;      //for level advances
  HPGain[1]   = 5;
  HPGain[2]   = 5;
  HPGain[3]   = 10;
  HPGain[4]   = 15;  
} 


Player::~Player()
{
	for (int i = 0; i < 11; i++) {
		if (items[i])
      delete items[i];
	}

  delete client;
}

std::string Player::getDescription(bool self) const
{
	std::stringstream s;
	std::string str;
	
	if(self){
		s << "You see yourself."; 
		if(voc > 0)
			s << " You are " << g_config.getGlobalStringField("vocations", voc) << ".";
	}
	else {	
		s << "You see " << name << " (Level " << level <<").";
	
		if(voc > 0){
			if(sex != 0)
				s << " He";
			else
				s << " She";

				s << " is "<< g_config.getGlobalStringField("vocations", voc) << ".";
		}
	}
	
	str = s.str();
	return str;
}

Item* Player::getItem(int pos) const
{
	if(pos>0 && pos <11)
		return items[pos];
	return NULL;
}

int Player::getWeaponDamage() const
{
	double damagemax = 0;
	//TODO:what happens with 2 weapons?
  for (int slot = SLOT_RIGHT; slot <= SLOT_LEFT; slot++)
	  if (items[slot]){
			if ((items[slot]->isWeapon())){
				// check which kind of skill we use...
				// and calculate the damage dealt
				Item *distitem;
				switch (items[slot]->getWeaponType()){
					case SWORD:
						damagemax = 3*skills[SKILL_SWORD][SKILL_LEVEL] + 2*Item::items[items[slot]->getID()].attack;
						break;
					case CLUB:
						damagemax = 3*skills[SKILL_CLUB][SKILL_LEVEL] + 2*Item::items[items[slot]->getID()].attack;
						break;
					case AXE:
						damagemax = 3*skills[SKILL_AXE][SKILL_LEVEL] + 2*Item::items[items[slot]->getID()].attack;
						break;
					case DIST:
						distitem = 	GetDistWeapon();
						if(distitem){
							damagemax = 3*skills[SKILL_DIST][SKILL_LEVEL]+ 2*Item::items[distitem->getID()].attack;
							//hit or miss
							int hitchance;
							if(distitem->getWeaponType() == AMO){//projectile
								hitchance = 90;
							}
							else{//thrown weapons
								hitchance = 50;
							}
							if(rand()%100 < hitchance){ //hit
								return 1+(int)(damagemax*rand()/(RAND_MAX+1.0));
							}
							else{	//miss
								return 0;
							}
						}						
						break;
					case MAGIC:
						damagemax = (level*2+maglevel*3) * 1.25;
						break;
					case AMO:
					case NONE:
					case SHIELD:
						// nothing to do
						break;
			  }
		  }
		  if(damagemax != 0)
		  	break;
    }

	// no weapon found -> fist fighting
	if (damagemax == 0)
		damagemax = 2*skills[SKILL_FIST][SKILL_LEVEL] + 5;

	// return it
	return 1+(int)(damagemax*rand()/(RAND_MAX+1.0));
}

int Player::getArmor() const
{
	int armor=0;
	
	if(items[SLOT_HEAD])
		armor += items[SLOT_HEAD]->getArmor();
	if(items[SLOT_NECKLACE])
		armor += items[SLOT_NECKLACE]->getArmor();
	if(items[SLOT_ARMOR])
		armor += items[SLOT_ARMOR]->getArmor();
	if(items[SLOT_LEGS])
		armor += items[SLOT_LEGS]->getArmor();
	if(items[SLOT_FEET])
		armor += items[SLOT_FEET]->getArmor();
	if(items[SLOT_RING])
		armor += items[SLOT_RING]->getArmor();
	
	return armor;
}

int Player::getDefense() const
{
	int defense=0;
	
	if(items[SLOT_LEFT]){		
		if(items[SLOT_LEFT]->getWeaponType() == SHIELD)
			defense += skills[SKILL_SHIELD][SKILL_LEVEL] + items[SLOT_LEFT]->getDefense();
		else
			defense += items[SLOT_LEFT]->getDefense();
	}
	if(items[SLOT_RIGHT]){
		if(items[SLOT_RIGHT]->getWeaponType() == SHIELD)
			defense += skills[SKILL_SHIELD][SKILL_LEVEL] + items[SLOT_RIGHT]->getDefense();
		else
			defense += items[SLOT_RIGHT]->getDefense();		
	}
	
	return defense;
}

unsigned long Player::getMoney()
{
	unsigned long money=0;
	
	for(int i=SLOT_HEAD; i <= SLOT_AMMO; i++)
	{
		Container* new_container = dynamic_cast<Container*>(items[i]);
		if(new_container)
		{
			money += getMoneyContainer(new_container);
		}
		else if(items[i])
		{
			switch(items[i]->getID())
			{
			case ITEM_COINS_GOLD:
				//gold coins
				money += items[i]->getItemCountOrSubtype();
				break;
			case ITEM_COINS_PLATINUM:
				//platinum coins
				money += items[i]->getItemCountOrSubtype() * 100;
				break;
			}
		}
	}
	
	return money;
}

unsigned long Player::getMoneyContainer(Container *container)
{
	unsigned long money=0;
	
	for(int i=0; i<container->size();i++)
	{
		Item *item = container->getItem(i);
		
		Container* new_container = dynamic_cast<Container*>(item);
		if(new_container)
		{
			money += getMoneyContainer(new_container);
		}
		else if(item)
		{
			switch(item->getID())
			{
			case ITEM_COINS_GOLD:
				//gold coins
				money += item->getItemCountOrSubtype();
				break;
			case ITEM_COINS_PLATINUM:
				//platinum coins
				money += item->getItemCountOrSubtype() * 100;
				break;
			}
		}
	}
	
	return money;
}

bool Player::substractMoney(unsigned long money)
{
	int goldcoins;
	int platcoins;
	NetworkMessage msg;
	
	for(int i=SLOT_HEAD; i <= SLOT_AMMO; i++)
	{
		Container* new_container = dynamic_cast<Container*>(items[i]);
		if(new_container && money)
		{
			substractMoneyContainer(new_container, &money);
		}
		else if(items[i] && money)
		{
			switch(items[i]->getID())
			{
			case ITEM_COINS_GOLD:
				//gold coins
				goldcoins = items[i]->getItemCountOrSubtype();
				if(money >= goldcoins)
				{
					money -= goldcoins;
					delete items[i];
					items[i] = NULL;
					
				}
				else
				{
					items[i]->setItemCountOrSubtype(goldcoins - money);
					money = 0;
				}
				client->sendInventory(i);
				break;
			case ITEM_COINS_PLATINUM:
				//platinum coins
				goldcoins = items[i]->getItemCountOrSubtype() * 100;
				if(money >= goldcoins)
				{
					money -= goldcoins;
					delete items[i];
					items[i] = NULL;
					
				}
				else
				{
					platcoins = (int)((goldcoins - money)/100);
					goldcoins = (goldcoins - money)%100;
					money = 0;
					if(platcoins)
					{
						items[i]->setItemCountOrSubtype(platcoins);
						if(goldcoins)
						{
							Item *new_item = Item::CreateItem(ITEM_COINS_GOLD, goldcoins);
							Container *default_container = dynamic_cast<Container*>(getItem(SLOT_BACKPACK));
							
							if(default_container && default_container->addItem(new_item)) // There is space in container
							{
								for(containerLayout::const_iterator cit = getContainers(); cit != getEndContainer(); ++cit) {
									if(cit->second == default_container) {
										//add item
										msg.AddByte(0x70);
										msg.AddByte(cit->first);
										msg.AddU16(new_item->getID());
										msg.AddByte(new_item->getItemCountOrSubtype());
									}
								}
							}
							else // There is no space in container
							{
								//TODO: place the item in ground...
								delete new_item;
							}
						}
					}
					else
					{
						delete items[i];
						items[i] = NULL;
						
						if(goldcoins)
						{
							Item *new_item = Item::CreateItem(ITEM_COINS_GOLD, goldcoins);
							items[i] = new_item;
						}
					}
				}
				client->sendInventory(i);
				break;
			}
		}
	}
	
	if(money == 0)
		return true;
	else
		return false;
}

bool Player::substractMoneyContainer(Container *container, unsigned long *money)
{
	int goldcoins;
	int platcoins;
	NetworkMessage msg;
	
	for(int i=0; i<container->size();i++)
	{
		Item *item = container->getItem(i);
		
		Container* new_container = dynamic_cast<Container*>(item);
		if(new_container && *money)
		{
			substractMoneyContainer(new_container, money);
		}
		else if(item && *money)
		{
			switch(item->getID())
			{
			case ITEM_COINS_GOLD:
				//gold coins
				msg.Reset();
				goldcoins = item->getItemCountOrSubtype();
				
				for(containerLayout::const_iterator cit = getContainers(); cit != getEndContainer(); ++cit) {
					if(cit->second == container) {
						//remove item
						msg.AddByte(0x72);
						msg.AddByte(cit->first);
						msg.AddByte(i);
					}
				}
				
				container->removeItem(item);
				
				if(*money >= goldcoins)
				{
					i--; // If we remove an item from the container then we need substract 1 to the container's main item counter
					*money -= goldcoins;
					delete item;
				}
				else
				{
					item->setItemCountOrSubtype(goldcoins - *money);
					*money = 0;
					container->addItem(item);
					for(containerLayout::const_iterator cit = getContainers(); cit != getEndContainer(); ++cit) {
						if(cit->second == container) {
							//add item
							msg.AddByte(0x70);
							msg.AddByte(cit->first);
							msg.AddU16(item->getID());
							msg.AddByte(item->getItemCountOrSubtype());
						}
					}
				}
				
				sendNetworkMessage(&msg);
				break;
			case ITEM_COINS_PLATINUM:
				//platinum coins
				msg.Reset();
				goldcoins = item->getItemCountOrSubtype() * 100;
				
				NetworkMessage msg2;
				for(containerLayout::const_iterator cit = getContainers(); cit != getEndContainer(); ++cit) {
					if(cit->second == container) {
						//remove item
						msg.AddByte(0x72);
						msg.AddByte(cit->first);
						msg.AddByte(i);
					}
				}
				container->removeItem(item);
				
				if(*money >= goldcoins)
				{
					i--; // If we remove an item from the container then we need substract 1 to the container's main item counter
					*money -= goldcoins;
					delete item;
				}
				else
				{
					platcoins = (int)((goldcoins - *money)/100);
					goldcoins = (int)(goldcoins - *money)%100;
					*money = 0;
					
					if(platcoins)
					{
						item->setItemCountOrSubtype(platcoins);
						
						container->addItem(item);
						for(containerLayout::const_iterator cit = getContainers(); cit != getEndContainer(); ++cit) {
							if(cit->second == container) {
								//add item
								msg.AddByte(0x70);
								msg.AddByte(cit->first);
								msg.AddU16(item->getID());
								msg.AddByte(item->getItemCountOrSubtype());
							}
						}
						
						if(goldcoins)
						{
							Item *new_item = Item::CreateItem(ITEM_COINS_GOLD, goldcoins);
							Container *default_container = dynamic_cast<Container*>(getItem(SLOT_BACKPACK));
							
							if(default_container && default_container->addItem(new_item)) // There is space in container
							{
								for(containerLayout::const_iterator cit = getContainers(); cit != getEndContainer(); ++cit) {
									if(cit->second == default_container) {
										//add item
										msg.AddByte(0x70);
										msg.AddByte(cit->first);
										msg.AddU16(new_item->getID());
										msg.AddByte(new_item->getItemCountOrSubtype());
									}
								}
							}
							else // There is no space in container
							{
								//TODO: place the item in ground...
								delete new_item;
							}
						}
					}
					else
					{
						if(goldcoins)
						{
							delete item;
							Item *new_item = Item::CreateItem(ITEM_COINS_GOLD, goldcoins);
							item = new_item;
							container->addItem(item);
							
							for(containerLayout::const_iterator cit = getContainers(); cit != getEndContainer(); ++cit) {
								if(cit->second == container) {
									//add item
									msg.AddByte(0x70);
									msg.AddByte(cit->first);
									msg.AddU16(item->getID());
									msg.AddByte(item->getItemCountOrSubtype());
								}
							}
						}
						else
						{
							delete item;
							item = NULL;
						}
					}
				}
				
				sendNetworkMessage(&msg);
				break;
			}
		}
	}
	
	if(*money == 0)
		return true;
	else
		return false;
}


void Player::speak(const std::string &text)
{
}

void Player::sendIcons()
{
	int icons = 0;
	if(inFightTicks >= 6000 || inFightTicks ==4000 || inFightTicks == 2000){
		icons |= ICON_SWORDS;
	}
	if(manaShieldTicks >= 1000){
		icons |= ICON_MANASHIELD;
	}
	if(speed > getNormalSpeed()){
		icons |= ICON_HASTE;
	}
	if(conditions.hasCondition(ATTACK_FIRE) /*burningTicks >= 1000*/){
		icons |= ICON_BURN | ICON_SWORDS;
	}
	if(conditions.hasCondition(ATTACK_ENERGY) /*energizedTicks >= 1000*/){
		icons |= ICON_ENERGY | ICON_SWORDS;
	}
	if(conditions.hasCondition(ATTACK_POISON)/*poisonedTicks >= 1000*/){
		icons |= ICON_POISON | ICON_SWORDS;
	}
	if(conditions.hasCondition(ATTACK_PARALYZE) /*speed < getNormalSpeed()*/ /*paralyzeTicks >= 1000*/) {
		icons |= ICON_PARALYZE | ICON_SWORDS;
	}

	client->sendIcons(icons);             
}

int Player::sendInventory(unsigned char sl_id){
	client->sendInventory(sl_id);
	return true;
}

int Player::addItemInventory(Item* item, int pos, bool internal /*= false*/) {
#ifdef __DEBUG__
	std::cout << "Should add item at " << pos <<std::endl;
#endif
	if(pos > 0 && pos < 11)
  {
		if (items[pos]) {
      delete items[pos];
		}

		items[pos] = item;
		if(items[pos]) {
			items[pos]->pos.x = 0xFFFF;
		}

		if(!internal) {
			client->sendInventory(pos);	
		}
  }
	else
		return false;

	return true;
}

bool Player::addItem(Item *item){
	//find an empty inventory slot
	if(!items[SLOT_RIGHT]){
		addItemInventory(item,SLOT_RIGHT);
		return true;
	}
	else if(!items[SLOT_LEFT]){
		addItemInventory(item,SLOT_LEFT);
		return true;
	}
	else if(!items[SLOT_AMMO]){
		addItemInventory(item,SLOT_AMMO);
		return true;
	}
	//find a in container
	for(int i=0; i< 11;i++){
		Container *container = dynamic_cast<Container*>(items[i]);
		if(container){
			return internalAddItemContainer(container,item);
		}
	}	
	return false;
}

bool Player::internalAddItemContainer(Container *container,Item* item){
	//check if it is full
	if(container->size() < container->capacity()){
		//add the item
		container->addItem(item);
		//update container
		client->sendItemAddContainer(container,item);
		return true;
	}
	else{ //look for more containers inside
		for(ContainerList::const_iterator cit = container->getItems(); 
			cit != container->getEnd(); ++cit){
			Container * temp_container = dynamic_cast<Container*>(*cit);
			if(temp_container){
				return internalAddItemContainer(temp_container,item);				
			}
		}
	}
	return false;
	
}

int Player::removeItemInventory(int pos, bool internal /*= false*/)
{
	if(pos > 0 && pos < 11) {

		items[pos] = NULL;

		if(!internal) {
			client->sendInventory(pos);
		}
	}
	else
		return false;

	return true;
}

unsigned int Player::getReqSkilltries (int skill, int level, int voc) {
	//first find on cache
	for(int i=0;i<2;i++){
		if(SkillAdvanceCache[skill][i].level == level && SkillAdvanceCache[skill][i].voc == voc){
#ifdef __DEBUG__
	std::cout << "Skill cache hit: " << this->name << " " << skill << " " << level << " " << voc <<std::endl;
#endif
			return SkillAdvanceCache[skill][i].tries;
		}
	}
    unsigned short int SkillBases[7] = { 50, 50, 50, 50, 30, 100, 20 };       // follows the order of enum skills_t
    float SkillMultipliers[7][5] = {
                                   {1.5f, 1.5f, 1.5f, 1.2f, 1.1f},     // Fist
                                   {2.0f, 2.0f, 1.8f, 1.2f, 1.1f},     // Club
                                   {2.0f, 2.0f, 1.8f, 1.2f, 1.1f},     // Sword
                                   {2.0f, 2.0f, 1.8f, 1.2f, 1.1f},     // Axe
                                   {2.0f, 2.0f, 1.8f, 1.4f, 1.1f},     // Distance
                                   {1.5f, 1.5f, 1.5f, 1.1f, 1.1f},     // Shielding
                                   {1.1f, 1.1f, 1.1f, 1.1f, 1.1f}      // Fishing
	                           };
#ifdef __DEBUG__
	std::cout << "Skill cache miss: " << this->name << " "<< skill << " " << level << " " << voc <<std::endl;
#endif
	//update cache
	//remove minor level
	int j;
	if(SkillAdvanceCache[skill][0].level > SkillAdvanceCache[skill][1].level){
		j = 1;
	}
	else{
		j = 0;
	}	
	SkillAdvanceCache[skill][j].level = level;
	SkillAdvanceCache[skill][j].voc = voc;
	SkillAdvanceCache[skill][j].tries = (unsigned int) ( SkillBases[skill] * pow((float) SkillMultipliers[skill][voc], (float) ( level - 11) ) );	
    return SkillAdvanceCache[skill][j].tries;
}

void Player::addSkillTry(int skilltry)
{
	int skill;	
	std::string skillname;
	//TODO:what happens with 2 weapons?
	for (int slot = SLOT_RIGHT; slot <= SLOT_LEFT; slot++) {
		if (items[slot]) {
			if (items[slot]->isWeapon()) {
				switch (items[slot]->getWeaponType()) {
					case SWORD: skill = 2; skillname = "sword fighting"; break;
					case CLUB: skill = 1; skillname = "club fighting"; break;
					case AXE: skill = 3; skillname = "axe fighting"; break;
					case DIST: skill = 4; skillname = "distance fighting"; break;
                    case SHIELD: continue; break;
                    case MAGIC: return;	break;//TODO: should add skill try?
					default: skill = 0; skillname = "fist fighting"; break;
			 	}//switch
			 	addSkillTryInternal(skilltry,skill,skillname);
			}			
		}
	}
}

void Player::addSkillShieldTry(int skilltry){
	//look for a shield
	std::string skillname = "shielding";
	for (int slot = SLOT_RIGHT; slot <= SLOT_LEFT; slot++) {
		if (items[slot]) {
			if (items[slot]->isWeapon()) {
				if (items[slot]->getWeaponType() == SHIELD) {
					addSkillTryInternal(skilltry,5,skillname);
					break;
			 	}			 	
			}			
		}
	}	
}

void Player::addSkillTryInternal(int skilltry,int skill,std::string &skillname){
	
	skills[skill][SKILL_TRIES] += skilltry;			
	//for skill level advances
	//int reqTries = (int) ( SkillBases[skill] * pow((float) VocMultipliers[skill][voc], (float) ( skills[skill][SKILL_LEVEL] - 10) ) );			 
#if __DEBUG__
	//for debug
	cout << Creature::getName() << ", has the vocation: " << voc << " and is training his " << skillname << "(" << skill << "). Tries: " << skills[skill][SKILL_TRIES] << "(" << getReqSkilltries (skill, (skills[skill][SKILL_LEVEL] + 1), voc) << ")" << std::endl;
	cout << "Current skill: " << skills[skill][SKILL_LEVEL] << std::endl;
#endif			 
	//Need skill up?
	if (skills[skill][SKILL_TRIES] >= getReqSkilltries (skill, (skills[skill][SKILL_LEVEL] + 1), voc)) {
	 	skills[skill][SKILL_LEVEL]++;
	 	skills[skill][SKILL_TRIES] = 0;
		skills[skill][SKILL_PERCENT] = 0;				 
		std::stringstream advMsg;
		advMsg << "You advanced in " << skillname << ".";
		client->sendTextMessage(MSG_ADVANCE, advMsg.str().c_str());
		client->sendSkills();
	}
	else{
	 //update percent
	 int new_percent = (unsigned int)(100*(skills[skill][SKILL_TRIES])/(1.*getReqSkilltries (skill, (skills[skill][SKILL_LEVEL]+1), voc)));
				 
	 	if(skills[skill][SKILL_PERCENT] != new_percent){
			skills[skill][SKILL_PERCENT] = new_percent;
			client->sendSkills();
	 	}
	}
}


unsigned int Player::getReqMana(int maglevel, int voc) {
  //ATTANTION: MAKE SURE THAT CHARS HAVE REASONABLE MAGIC LEVELS. ESPECIALY KNIGHTS!!!!!!!!!!!
  float ManaMultiplier[5] = { 1.0f, 1.1f, 1.1f, 1.4f, 3 };
  unsigned int reqMana = (unsigned int) ( 400 * pow(ManaMultiplier[voc], maglevel-1) );       //will calculate required mana for a magic level
  if (reqMana % 20 < 10) //CIP must have been bored when they invented this odd rounding
    reqMana = reqMana - (reqMana % 20);
  else
    reqMana = reqMana - (reqMana % 20) + 20;

  return reqMana;
}

Container* Player::getContainer(unsigned char containerid)
{
  for(containerLayout::iterator cl = vcontainers.begin(); cl != vcontainers.end(); ++cl)
  {
	  if(cl->first == containerid)
			return cl->second;
	}

	return NULL;
}

unsigned char Player::getContainerID(const Container* container) const
{
  for(containerLayout::const_iterator cl = vcontainers.begin(); cl != vcontainers.end(); ++cl)
  {
	  if(cl->second == container)
			return cl->first;
	}

	return 0xFF;
}

void Player::addContainer(unsigned char containerid, Container *container)
{
#ifdef __DEBUG__
	cout << Creature::getName() << ", addContainer: " << (int)containerid << std::endl;
#endif
	if(containerid > 0xF)
		return;

	for(containerLayout::iterator cl = vcontainers.begin(); cl != vcontainers.end(); ++cl) {
		if(cl->first == containerid) {
			cl->second = container;
			return;
		}
	}
	
	//id doesnt exist, create it
	containerItem vItem;
	vItem.first = containerid;
	vItem.second = container;

	vcontainers.push_back(vItem);
}

void Player::closeContainer(unsigned char containerid)
{
  for(containerLayout::iterator cl = vcontainers.begin(); cl != vcontainers.end(); ++cl)
  {
	  if(cl->first == containerid)
	  {
		  vcontainers.erase(cl);
			break;
		}
	}

#ifdef __DEBUG__
	cout << Creature::getName() << ", closeContainer: " << (int)containerid << std::endl;
#endif
}

int Player::getLookCorpse(){
	if(sex != 0)
		return ITEM_MALE_CORPSE;
	else
		return ITEM_FEMALE_CORPSE;
}

void Player::dropLoot(Container *corpse)
{
	for (int slot = 0; slot < 11; slot++)
	{
		Item *item = items[slot];		
		if (item && ((dynamic_cast<Container*>(item)) || random_range(1, 100) <= 10)) {
			corpse->addItem(item);
			items[slot] = NULL;
		}
	}
	/*
	//drop backpack if any
	if(items[SLOT_BACKPACK]){
		corpse->addItem(items[SLOT_BACKPACK]);
		items[SLOT_BACKPACK] = NULL;
	}
	*/
	
}

fight_t Player::getFightType()
{
  for (int slot = SLOT_RIGHT; slot <= SLOT_LEFT; slot++)
  {
    if (items[slot])
    {
		if ((items[slot]->isWeapon())) {
			Item *DistItem;
			switch (items[slot]->getWeaponType()){
			case DIST:
				DistItem = GetDistWeapon();
				if(DistItem){
					return FIGHT_DIST;
				}
				else{
					return FIGHT_MELEE;
				}
				break;
			case MAGIC:
				return FIGHT_MAGICDIST;
			default:
				break;
			}
		}
    }
  }
  return FIGHT_MELEE;
}

void Player::RemoveDistItem(){
	Item *DistItem = GetDistWeapon();
	unsigned char sl_id;
	if(DistItem){
		if(DistItem->isStackable() == false)
			return;
		//remove one dist item
		unsigned char n = DistItem->getItemCountOrSubtype();
		if(DistItem == items[SLOT_RIGHT]){
			sl_id = SLOT_RIGHT;
		}
		else if(DistItem == items[SLOT_LEFT]){
			sl_id = SLOT_LEFT;
		}
		else if(DistItem == items[SLOT_AMMO]){
			sl_id = SLOT_AMMO;
		}
		if(n > 1){
			DistItem->setItemCountOrSubtype(n-1);
		}
		else{
			//remove the item			
			items[sl_id] = NULL;
			delete DistItem;
		}		
		//update inventory
		client->sendInventory(sl_id);
	}
	return;
}

subfight_t Player::getSubFightType()
{
	fight_t type = getFightType();
	if(type == FIGHT_DIST) {
		Item *DistItem = GetDistWeapon();
		if(DistItem){
			return DistItem->getSubfightType();
		}
	}
    if(type == FIGHT_MAGICDIST) {
		Item *DistItem = GetDistWeapon();
		if(DistItem){
			return DistItem->getSubfightType();
		}	
	}
	return DIST_NONE;
}

Item * Player::GetDistWeapon() const{
	for (int slot = SLOT_RIGHT; slot <= SLOT_LEFT; slot++){
	    if (items[slot]){
			if ((items[slot]->isWeapon())) {
				switch (items[slot]->getWeaponType()){
				case DIST:
					//find ammunition
					if(items[slot]->getAmuType() == AMU_NONE){
						return items[slot];
					}
					if(items[SLOT_AMMO]){
						//compare ammo types
						if(items[SLOT_AMMO]->getWeaponType() == AMO && 
							items[slot]->getAmuType() == items[SLOT_AMMO]->getAmuType()){
								return items[SLOT_AMMO];
						}
						else{
							return NULL;
						}
						
					}
					else{
						return NULL;
					}
				case MAGIC:
					return items[slot];
				default:
					break;
				}//switch
			}//isweapon
    	}//item[slot]
  	}//for
  	return NULL;
}


bool Player::CanSee(int x, int y, int z) const
{
  return client->CanSee(x, y, z);
}

Container* Player::getDepot(unsigned long depotId){	
	DepotMap::iterator it = depots.find(depotId);
	if (it != depots.end()){	
      return it->second;
	}    
	return NULL;
}

bool Player::addDepot(Container* depot,unsigned long depotId){	
	Container *bdep = getDepot(depotId);
	if(bdep)
		return false;
		
	depot->pos.x = 0xFFFF;
	
	depots[depotId] = depot;
	return true;
}

void Player::sendNetworkMessage(NetworkMessage *msg)
{
  client->sendNetworkMessage(msg);
}

void Player::sendCancel(const char *msg)
{
  client->sendCancel(msg);
}
void Player::sendChangeSpeed(Creature* creature){
     client->sendChangeSpeed(creature);
     }

void Player::sendToChannel(Creature *creature,SpeakClasses type, const std::string &text, unsigned short channelId){
     client->sendToChannel(creature, type, text, channelId);
     }

void Player::sendCancelAttacking()
{
  attackedCreature = 0;   
  client->sendCancelAttacking();
}

void Player::sendCancelWalk(const char *msg)
{
  client->sendCancelWalk(msg);
}
void Player::sendStats(){
	//update level and maglevel percents
	if(lastSentStats.experience != this->experience || 
			lastSentStats.level != this->level)
	    level_percent  = (unsigned char)(100*(experience-getExpForLv(level))/(1.*getExpForLv(level+1)-getExpForLv(level)));		
	if(lastSentStats.manaspent != this->manaspent || 
			lastSentStats.maglevel != this->maglevel)
	    maglevel_percent  = (unsigned char)(100*(manaspent/(1.*getReqMana(maglevel+1,voc))));
	//save current stats 
	lastSentStats.health = this->health;
	lastSentStats.healthmax = this->healthmax;
	lastSentStats.cap = this->cap;
	lastSentStats.experience = this->experience;
	lastSentStats.level = this->level;
	lastSentStats.mana = this->mana;
	lastSentStats.manamax = this->manamax;
	lastSentStats.manaspent = this->manaspent;
	lastSentStats.maglevel = this->maglevel;
	
	client->sendStats();
}
void Player::sendTextMessage(MessageClasses mclass, const char* message){
	client->sendTextMessage(mclass,message);
}

void Player::flushMsg(){
	client->flushOutputBuffer();
}
void Player::sendTextMessage(MessageClasses mclass, const char* message,const Position &pos, unsigned char type){
	client->sendTextMessage(mclass,message,pos,type);
}
void Player::sendPing(){
	internal_ping++;
	if(internal_ping >= 5){ //1 ping each 5 seconds
		internal_ping = 0;
		npings++;
		client->sendPing();
	}
	if(npings >= 6){
		//std::cout << "logout" << std::endl;
		if(inFightTicks >=1000 && health >0) {
			//logout?
			//client->logout();
		}
		else{
			//client->logout();			
		}
	}
}

void Player::receivePing(){
	if(npings > 0)
		npings--;
}

void Player::sendDistanceShoot(const Position &from, const Position &to, unsigned char type){
	client->sendDistanceShoot(from, to,type);
}

void Player::sendMagicEffect(const Position &pos, unsigned char type){
	client->sendMagicEffect(pos,type);
}
void Player::sendAnimatedText(const Position &pos, unsigned char color, std::string text){
	client->sendAnimatedText(pos,color,text);
}

void Player::sendCreatureHealth(const Creature *creature){
	client->sendCreatureHealth(creature);
}


bool Player::NeedUpdateStats(){
	if(lastSentStats.health != this->health ||
		 lastSentStats.healthmax != this->healthmax ||
		 lastSentStats.cap != this->cap ||
		 lastSentStats.experience != this->experience ||
		 lastSentStats.level != this->level ||
		 lastSentStats.mana != this->mana ||
		 lastSentStats.manamax != this->manamax ||
		 lastSentStats.manaspent != this->manaspent ||
		 lastSentStats.maglevel != this->maglevel){
		return true;
	}
	else{
		return false;
	}
}

void Player::onThingMove(const Creature *creature, const Thing *thing, const Position *oldPos,
	unsigned char oldstackpos, unsigned char oldcount, unsigned char count)
{
  client->sendThingMove(creature, thing, oldPos, oldstackpos, oldcount, count);
}

 //container to container
void Player::onThingMove(const Creature *creature, const Container *fromContainer, unsigned char from_slotid,
	const Item* fromItem, int oldFromCount, Container *toContainer, unsigned char to_slotid,
	const Item *toItem, int oldToCount, int count)
{
	client->sendThingMove(creature, fromContainer, from_slotid, fromItem, oldFromCount, toContainer, to_slotid, toItem, oldToCount, count);
}

//inventory to container
void Player::onThingMove(const Creature *creature, slots_t fromSlot, const Item* fromItem,
	int oldFromCount, const Container *toContainer, unsigned char to_slotid, const Item *toItem, int oldToCount, int count)
{
	client->sendThingMove(creature, fromSlot, fromItem, oldFromCount, toContainer, to_slotid, toItem, oldToCount, count);
}

//inventory to inventory
void Player::onThingMove(const Creature *creature, slots_t fromSlot, const Item* fromItem,
	int oldFromCount, slots_t toSlot, const Item* toItem, int oldToCount, int count)
{
	client->sendThingMove(creature, fromSlot, fromItem, oldFromCount, toSlot, toItem, oldToCount, count);
}

//container to inventory
void Player::onThingMove(const Creature *creature, const Container *fromContainer, unsigned char from_slotid,
	const Item* fromItem, int oldFromCount, slots_t toSlot, const Item *toItem, int oldToCount, int count)
{
	client->sendThingMove(creature, fromContainer, from_slotid, fromItem, oldFromCount, toSlot, toItem, oldToCount, count);
}

//container to ground (100%)
void Player::onThingMove(const Creature *creature, const Container *fromContainer, unsigned char from_slotid,
	const Item* fromItem, int oldFromCount, const Position &toPos, const Item *toItem, int oldToCount, int count)
{
	client->sendThingMove(creature, fromContainer, from_slotid, fromItem, oldFromCount, toPos, toItem, oldToCount, count);
}

//inventory to ground
void Player::onThingMove(const Creature *creature, slots_t fromSlot,
	const Item* fromItem, int oldFromCount, const Position &toPos, const Item *toItem, int oldToCount, int count)
{
	client->sendThingMove(creature, fromSlot, fromItem, oldFromCount, toPos, toItem, oldToCount, count);
}

//ground to container
/*
void Player::onThingMove(const Creature *creature, const Position *oldPos, const Item* item, unsigned char stackpos,
	const Container *toContainer, const Item* dropitem, unsigned char to_slotid, bool concat)
*/
void Player::onThingMove(const Creature *creature, const Position &fromPos, int stackpos, const Item* fromItem,
	int oldFromCount, const Container *toContainer, unsigned char to_slotid, const Item *toItem, int oldToCount, int count)
{
	client->sendThingMove(creature, fromPos, stackpos, fromItem, oldFromCount, toContainer, to_slotid, toItem, oldToCount, count);
	//client->sendThingMove(creature, oldPos, item, stackpos, toContainer, dropitem, to_slotid, concat);
}

//ground to inventory
void Player::onThingMove(const Creature *creature, const Position &fromPos, int stackpos, const Item* fromItem,
	int oldFromCount, slots_t toSlot, const Item *toItem, int oldToCount, int count)
{
	client->sendThingMove(creature, fromPos, stackpos, fromItem, oldFromCount, toSlot, toItem, oldToCount, count);
}

void Player::setAttackedCreature(unsigned long id){
     attackedCreature = id;
}

void Player::onCreatureAppear(const Creature *creature)
{
	const Thing *thing = dynamic_cast< const Thing*>(creature);
	if(thing)
  		client->sendThingAppear(thing);
}

void Player::onCreatureDisappear(const Creature *creature, unsigned char stackPos, bool tele /*= false*/)
{
	const Thing *thing = dynamic_cast<const Thing*>(creature);
	if(thing)
  		client->sendThingDisappear(thing, stackPos, tele);
}

void Player::onThingAppear(const Thing* thing){
	client->sendThingAppear(thing);
}

void Player::onThingTransform(const Thing* thing,int stackpos){
	client->sendThingTransform(thing,stackpos);
}

void Player::onThingDisappear(const Thing* thing, unsigned char stackPos){
	client->sendThingDisappear(thing, stackPos, false);
}
//auto-close containers
void Player::onThingRemove(const Thing* thing){
	client->sendThingRemove(thing);
}

void Player::onItemAddContainer(const Container* container,const Item* item){
	client->sendItemAddContainer(container,item);
}

void Player::onItemRemoveContainer(const Container* container,const unsigned char slot){
	client->sendItemRemoveContainer(container,slot);
}

void Player::onItemUpdateContainer(const Container* container,const Item* item,const unsigned char slot){
	client->sendItemUpdateContainer(container,item,slot);
}

void Player::onCreatureTurn(const Creature *creature, unsigned char stackPos)
{
  client->sendCreatureTurn(creature, stackPos);
}

void Player::onCreatureSay(const Creature *creature, SpeakClasses type, const std::string &text)
{
  client->sendCreatureSay(creature, type, text);
}

void Player::onCreatureChangeOutfit(const Creature* creature) {
		  client->sendSetOutfit(creature);
}

int Player::onThink(int& newThinkTicks)
{
	newThinkTicks = 1000;
	return 1000;
}

void Player::onTileUpdated(const Position &pos)
{
  client->sendTileUpdated(pos);
}

void Player::onTeleport(const Creature *creature, const Position *oldPos, unsigned char oldstackpos) { 
  client->sendThingMove(creature, creature,oldPos, oldstackpos, true, 1, 1); 
}

unsigned long Player::getIP() const
{
	return client->getIP();
}

void Player::die() {
	
    client->sendTextMessage(MSG_ADVANCE, "You are dead.");
    client->sendTextMessage(MSG_EVENT, "Own3d!");
		
	//Magic Level downgrade
	unsigned long sumMana = 0;
	long lostMana = 0;
	for (int i = 1; i <= maglevel; i++) {              //sum up all the mana
		sumMana += getReqMana(i, voc);
	}
                
	sumMana += manaspent;
                
	lostMana = (long)(sumMana * 0.1);   //player loses 10% of all spent mana when he dies
    
    while(lostMana > manaspent){
		lostMana -= manaspent;
		manaspent = getReqMana(maglevel, voc);
		maglevel--;
	}
	manaspent -= lostMana;
	//End Magic Level downgrade
                
	//Skill loss
	long lostSkilltries;
	unsigned long sumSkilltries;
	for (int i = 0; i <= 6; i++) {  //for each skill
		lostSkilltries = 0;         //reset to 0
		sumSkilltries = 0;
                    
		for (unsigned c = 11; c <= skills[i][SKILL_LEVEL]; c++) { //sum up all required tries for all skill levels
			sumSkilltries += getReqSkilltries(i, c, voc);
		}
                    
		sumSkilltries += skills[i][SKILL_TRIES];
                    
		lostSkilltries = (long) (sumSkilltries * 0.1);           //player loses 10% of his skill tries

		while(lostSkilltries > skills[i][SKILL_TRIES]){
			lostSkilltries -= skills[i][SKILL_TRIES];
			skills[i][SKILL_TRIES] = getReqSkilltries(i, skills[i][SKILL_LEVEL], voc);
			if(skills[i][SKILL_LEVEL] > 10){
				skills[i][SKILL_LEVEL]--;
			}
			else{
				skills[i][SKILL_LEVEL] = 10;
				skills[i][SKILL_TRIES] = 0;
				lostSkilltries = 0;
				break;
			}
		}
		skills[i][SKILL_TRIES] -= lostSkilltries;
	}               
	//End Skill loss
        
	//Level Downgrade
	long newLevel = level;
	while((unsigned long)(experience - getLostExperience()) < getExpForLv(newLevel)) //0.1f is also used in die().. maybe we make a little function for exp-loss?
	{
		if(newLevel > 1)
			newLevel--;
		else
			break;
	}
	
	if(newLevel != level)
	{
		std::stringstream lvMsg;
		lvMsg << "You were downgraded from level " << level << " to level " << newLevel << ".";
		client->sendTextMessage(MSG_ADVANCE, lvMsg.str().c_str());	
		
	}
}

void Player::preSave()
{  
	if (health <= 0)
	{
		health = healthmax;
		pos.x = masterPos.x;
		pos.y = masterPos.y;
		pos.z = masterPos.z;
		
		experience -= getLostExperience(); //(int)(experience*0.1f);        //0.1f is also used in die().. maybe we make a little function for exp-loss?
				
		while(experience < getExpForLv(level))
		{
			if(level > 1)                               
				level--;
			else
				break;
			
			/* This checks (but not the downgrade sentences) aren't really necesary cause if the
			player has a "normal" hp,mana,etc when he gets level 1 he will not lose more
			hp,mana,etc... but here they are :P */
			if ((healthmax -= HPGain[voc]) < 0) //This could be avoided with a proper use of unsigend int
				healthmax = 0;
			
			health = healthmax;
			
			if ((manamax -= ManaGain[voc]) < 0) //This could be avoided with a proper use of unsigend int
				manamax = 0;
			
			mana = manamax;
			
			if ((cap -= CapGain[voc]) < 0) //This could be avoided with a proper use of unsigend int
				cap = 0;         
		}
	}
}

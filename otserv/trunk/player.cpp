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

extern LuaScript g_config;

Player::Player(const char *name, Protocol *p) : Creature(name)
{
  client     = p;
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
  fightMode = followMode = 0;
  for(int i = 0; i < 7; i++)
  {
    skills[i][SKILL_LEVEL] = 1;
    skills[i][SKILL_TRIES] = 0;
  }

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
	for (int i = 0; i < 11; i++)
		if (items[i])
      delete items[i];

  delete client;
}

std::string Player::getDescription(bool self){
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

Item* Player::getItem(int pos)
{
	if(pos>0 && pos <11)
		return items[pos];
	return NULL;
}

int Player::getWeaponDamage() const
{
	double damagemax = 0;
  for (int slot = SLOT_RIGHT; slot <= SLOT_LEFT; slot++)
	  if (items[slot])
    {
			if ((items[slot]->isWeapon()))
      {
				// check which kind of skill we use...
				// and calculate the damage dealt
				switch (items[slot]->getWeaponType())
        {
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
						damagemax = 4*skills[SKILL_DIST][SKILL_LEVEL];
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
    }

	// no weapon found -> fist fighting
	if (damagemax == 0)
		damagemax = 2*skills[SKILL_FIST][SKILL_LEVEL] + 5;

	// return it
	return 1+(int)(damagemax*rand()/(RAND_MAX+1.0));
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
	if(burningTicks >= 1000){
		icons |= ICON_BURN;
	}
	if(energizedTicks >= 1000){
		icons |= ICON_ENERGY;
	}
	if(poisonedTicks >= 1000){
		icons |= ICON_POISON;
	}
	if(speed < getNormalSpeed() /*paralyzeTicks >= 1000*/) {
		icons |= ICON_PARALYZE;
	}

	client->sendIcons(icons);             
}

int Player::sendInventory(){
	client->sendInventory();
	return true;
}


int Player::addItem(Item* item, int pos){
#ifdef __DEBUG__
	std::cout << "Should add item at " << pos <<std::endl;
#endif

	if(pos>0 && pos <11)
  {
    if (items[pos])
      delete items[pos];
		items[pos]=item;
  }
	client->sendInventory();
	return true;
}

unsigned int Player::getReqSkilltries (int skill, int level, int voc) {
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
                                   
    return (int) ( SkillBases[skill] * pow((float) SkillMultipliers[skill][voc], (float) ( level - 11) ) );
}

void Player::addSkillTry(int skilltry)
{
	int skill;
	std::string skillname;
	for (int slot = SLOT_RIGHT; slot <= SLOT_LEFT; slot++) {
		if (items[slot]) {
			if (items[slot]->isWeapon()) {
				switch (items[slot]->getWeaponType()) {
					case SWORD: skill = 2; skillname = "sword fighting"; break;
					case CLUB: skill = 1; skillname = "club fighting"; break;
					case AXE: skill = 3; skillname = "axe fighting"; break;
					case DIST: skill = 4; skillname = "distance fighting"; break;
          case SHIELD: skill = 5; skillname = "shielding"; break;
					default: skill = 0; skillname = "fist fighting"; break;
			 }
			 
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
				 
				 NetworkMessage msg;
				 std::stringstream advMsg;
				 advMsg << "You advanced in " << skillname << ".";
				 msg.AddTextMessage(MSG_ADVANCE, advMsg.str().c_str());
				 msg.AddPlayerSkills(this);
				 sendNetworkMessage(&msg);
			 }
			}
		}
	}
}




unsigned int Player::getReqMana(int maglevel, int voc) {
  //ATTANTION: MAKE SURE THAT CHARS HAVE REASONABLE MAGIC LEVELS. ESPECIALY KNIGHTS!!!!!!!!!!!

  float ManaMultiplier[5] = { 1.0f, 1.1f, 1.1f, 1.4f, 3 };
  
  return (unsigned int) ( 400 * pow(ManaMultiplier[voc], maglevel-1) );       //will calculate required mana for a magic level
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

unsigned char Player::getContainerID(Container* container)
{
  for(containerLayout::iterator cl = vcontainers.begin(); cl != vcontainers.end(); ++cl)
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

fight_t Player::getFightType()
{
  for (int slot = SLOT_RIGHT; slot <= SLOT_LEFT; slot++)
  {
    if (items[slot])
    {
			if ((items[slot]->isWeapon())) {
				switch (items[slot]->getWeaponType())
        {
					case DIST:
						return FIGHT_DIST;
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


bool Player::CanSee(int x, int y) const
{
  return client->CanSee(x, y);
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

void Player::sendToChannel(Creature *creature, unsigned char type, const std::string &text, unsigned short channelId){
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

void Player::onThingMove(const Creature *creature, const Thing *thing, const Position *oldPos, unsigned char oldstackpos)
{
  client->sendThingMove(creature, thing, oldPos, oldstackpos);
}

void Player::setAttackedCreature(unsigned long id){
     attackedCreature = id;
        }

void Player::onCreatureAppear(const Creature *creature)
{
  client->sendCreatureAppear(creature);
}


void Player::onCreatureDisappear(const Creature *creature, unsigned char stackPos)
{
  client->sendCreatureDisappear(creature, stackPos);
}


void Player::onCreatureTurn(const Creature *creature, unsigned char stackPos)
{
  client->sendCreatureTurn(creature, stackPos);
}


void Player::onCreatureSay(const Creature *creature, unsigned char type, const std::string &text)
{
  client->sendCreatureSay(creature, type, text);
}

void Player::onCreatureChangeOutfit(const Creature* creature) {
		  client->sendSetOutfit(creature);
}

void Player::onThink(){}

void Player::onTileUpdated(const Position *Pos)
{
  client->sendTileUpdated(Pos);
}

void Player::onContainerUpdated(Item *item, unsigned char from_id, unsigned char to_id,
																unsigned char from_slot, unsigned char to_slot, bool remove)
{
	client->sendContainerUpdated(item, from_id, to_id, from_slot, to_slot, remove);
}

unsigned long Player::getIP() const
{
	return client->getIP();
}

void Player::die() {
        NetworkMessage msg;
        
		msg.AddTextMessage(MSG_ADVANCE, "You are dead.");
		msg.AddTextMessage(MSG_EVENT, "Own3d!");
		
		//Magic Level downgrade
		unsigned int sumMana = 0;
		unsigned int lostMana = 0;
		for (int i = 1; i <= maglevel; i++) {              //sum up all the mana
			sumMana += getReqMana(i, voc);
		}
                
		sumMana += manaspent;
                
		lostMana = (int) (sumMana * 0.1);   //player loses 10% of all spent mana when he dies
                
		if ((unsigned) manaspent >= lostMana) { //player does not lose a magic level
		manaspent -= lostMana;
		} 
		else {                             //player DOES lose a magic level
			lostMana -= manaspent;
			manaspent = (int) ( getReqMana(maglevel, voc) - lostMana );
			maglevel--;
		}
		//End Magic Level downgrade
                
		//Skill loss
		unsigned int lostSkilltries;
		unsigned int sumSkilltries;
		for (int i = 0; i <= 6; i++) {  //for each skill
			lostSkilltries = 0;         //reset to 0
			sumSkilltries = 0;
                    
			for (unsigned c = 11; c <= skills[i][SKILL_LEVEL]; c++) {    //sum up all required tries for all skill levels
				sumSkilltries += getReqSkilltries(i, c, voc);
			}
                    
			sumSkilltries += skills[i][SKILL_TRIES];
                    
			lostSkilltries = (int) (sumSkilltries * 0.1);           //player loses 10% of his skill tries

			if ((unsigned)skills[i][SKILL_TRIES] >= lostSkilltries) { //player does not lose a skill level
				skills[i][SKILL_TRIES] -= lostSkilltries;
			}
			else {                                                //player DOES lose a skill level
				if (skills[i][SKILL_LEVEL] > 10 ) {          //skills should not be < 10
					lostSkilltries -= skills[i][SKILL_TRIES];
					skills[i][SKILL_TRIES] = (int) ( getReqSkilltries(i, skills[i][SKILL_LEVEL], voc) - lostSkilltries );
					skills[i][SKILL_LEVEL]--;
				}
				else {
					skills[i][SKILL_LEVEL] = 10;
					skills[i][SKILL_TRIES] = 0;
				}
			}
		}               
		//End Skill loss
        
		//Level Downgrade
		if ((unsigned long)(experience - (unsigned)(experience*0.1f)) < getExpForLv(level))         //0.1f is also used in die().. maybe we make a little function for exp-loss?
		{
            if(level>1){          
			std::stringstream lvMsg;
			lvMsg << "You were downgraded from level " << level << " to level " << level-1 << ".";
			msg.AddTextMessage(MSG_ADVANCE, lvMsg.str().c_str());
            }
            if(experience <0)
            experience =0;
		}
		
		sendNetworkMessage(&msg);
}

void Player::savePlayer(std::string &name)
{  
    std::string filename = "data/players/"+name+".xml";
    std::stringstream sb;
    
    xmlDocPtr doc;
	xmlNodePtr nn, sn, pn, root;
	doc = xmlNewDoc((const xmlChar*)"1.0");
	doc->children = xmlNewDocNode(doc, NULL, (const xmlChar*)"player", NULL);
	root = doc->children;
	
	if (health <= 0)
	{
		health = healthmax;
		pos.x = masterPos.x;
		pos.y = masterPos.y;
		pos.z = masterPos.z;

		//int expLoss = (int)(experience*0.1f);
		experience -= (int)(experience*0.1f);        //0.1f is also used in die().. maybe we make a little function for exp-loss?

		//Player died?

	   int reqExp =  getExpForLv(level);
	   if (experience < (unsigned )reqExp)
		    {
            if(level > 1)                               
            level -= 1;
            healthmax -= HPGain[voc];
            health -= HPGain[voc];
            
            if ((manamax - ManaGain[voc]) <= 0)        //This could be avoided with a proper use of unsigend int
                 manamax = 0;
            else manamax = manamax - ManaGain[voc];
            if ((mana - ManaGain[voc]) <= 0)
                 mana = 0;
            else mana = mana - ManaGain[voc];
            
            cap -= CapGain[voc];            
            }
		 }
       
	sb << name;  	           xmlSetProp(root, (const xmlChar*) "name", (const xmlChar*)sb.str().c_str());     sb.str("");
	sb << accountNumber;       xmlSetProp(root, (const xmlChar*) "account", (const xmlChar*)sb.str().c_str());	sb.str("");
	sb << sex;                 xmlSetProp(root, (const xmlChar*) "sex", (const xmlChar*)sb.str().c_str());     	sb.str("");	
	sb << getDirection();
    if (sb.str() == "North"){sb.str(""); sb << "0";}
	if (sb.str() == "East") {sb.str(""); sb << "1";}
	if (sb.str() == "South"){sb.str(""); sb << "2";}
	if (sb.str() == "West") {sb.str(""); sb << "3";}
	xmlSetProp(root, (const xmlChar*) "lookdir", (const xmlChar*)sb.str().c_str());                             sb.str("");
	sb << experience;         xmlSetProp(root, (const xmlChar*) "exp", (const xmlChar*)sb.str().c_str());       sb.str("");	
	sb << voc;                xmlSetProp(root, (const xmlChar*) "voc", (const xmlChar*)sb.str().c_str());       sb.str("");
	sb << level;              xmlSetProp(root, (const xmlChar*) "level", (const xmlChar*)sb.str().c_str());     sb.str("");	
	sb << access;             xmlSetProp(root, (const xmlChar*) "access", (const xmlChar*)sb.str().c_str());	sb.str("");	
	sb << cap;    	          xmlSetProp(root, (const xmlChar*) "cap", (const xmlChar*)sb.str().c_str());       sb.str("");
	sb << maglevel;	          xmlSetProp(root, (const xmlChar*) "maglevel", (const xmlChar*)sb.str().c_str());  sb.str("");

	pn = xmlNewNode(NULL,(const xmlChar*)"spawn");
	sb << pos.x;    xmlSetProp(pn, (const xmlChar*) "x", (const xmlChar*)sb.str().c_str());        sb.str("");
	sb << pos.y;  	xmlSetProp(pn, (const xmlChar*) "y", (const xmlChar*)sb.str().c_str());        sb.str("");
	sb << pos.z; 	xmlSetProp(pn, (const xmlChar*) "z", (const xmlChar*)sb.str().c_str());	       sb.str("");
	xmlAddChild(root, pn);
	
	pn = xmlNewNode(NULL,(const xmlChar*)"temple");
	sb << masterPos.x;  xmlSetProp(pn, (const xmlChar*) "x", (const xmlChar*)sb.str().c_str());        sb.str("");
	sb << masterPos.y;  xmlSetProp(pn, (const xmlChar*) "y", (const xmlChar*)sb.str().c_str());        sb.str("");
	sb << masterPos.z; 	xmlSetProp(pn, (const xmlChar*) "z", (const xmlChar*)sb.str().c_str());	       sb.str("");
	xmlAddChild(root, pn);
	
	pn = xmlNewNode(NULL,(const xmlChar*)"health");
	sb << health;     xmlSetProp(pn, (const xmlChar*) "now", (const xmlChar*)sb.str().c_str());        sb.str("");
	sb << healthmax;  xmlSetProp(pn, (const xmlChar*) "max", (const xmlChar*)sb.str().c_str());        sb.str("");
	                     xmlSetProp(pn, (const xmlChar*) "food", (const xmlChar*)"0");	   
	xmlAddChild(root, pn);
	
	pn = xmlNewNode(NULL,(const xmlChar*)"mana");
	sb << mana;      xmlSetProp(pn, (const xmlChar*) "now", (const xmlChar*)sb.str().c_str());        sb.str("");
	sb << manamax;   xmlSetProp(pn, (const xmlChar*) "max", (const xmlChar*)sb.str().c_str());        sb.str("");
    sb << manaspent; xmlSetProp(pn, (const xmlChar*) "spent", (const xmlChar*)sb.str().c_str());      sb.str("");
	xmlAddChild(root, pn);
    	               
	pn = xmlNewNode(NULL,(const xmlChar*)"look");
    sb << lookmaster;       xmlSetProp(pn, (const xmlChar*) "type", (const xmlChar*)sb.str().c_str());        sb.str("");
	sb << lookhead;         xmlSetProp(pn, (const xmlChar*) "head", (const xmlChar*)sb.str().c_str());        sb.str("");
	sb << lookbody;         xmlSetProp(pn, (const xmlChar*) "body", (const xmlChar*)sb.str().c_str());        sb.str("");
	sb << looklegs;         xmlSetProp(pn, (const xmlChar*) "legs", (const xmlChar*)sb.str().c_str());        sb.str("");
	sb << lookfeet;         xmlSetProp(pn, (const xmlChar*) "feet", (const xmlChar*)sb.str().c_str());        sb.str("");
	xmlAddChild(root, pn);
    	               
    	      
	sn = xmlNewNode(NULL,(const xmlChar*)"skills");
	for (int i = 0; i <= 6; i++)
	  {
	  pn = xmlNewNode(NULL,(const xmlChar*)"skill");
	  sb << i;                          xmlSetProp(pn, (const xmlChar*) "skillid", (const xmlChar*)sb.str().c_str());      sb.str("");
	  sb << skills[i][SKILL_LEVEL];     xmlSetProp(pn, (const xmlChar*) "level", (const xmlChar*)sb.str().c_str());        sb.str("");
	  sb << skills[i][SKILL_TRIES];     xmlSetProp(pn, (const xmlChar*) "tries", (const xmlChar*)sb.str().c_str());        sb.str("");
	  xmlAddChild(sn, pn);
      }
   xmlAddChild(root, sn);
	
	sn = xmlNewNode(NULL,(const xmlChar*)"inventory");
	for (int i = 1; i <= 10; i++)
	  {
   	  if (items[i])
          {
    	  pn = xmlNewNode(NULL,(const xmlChar*)"slot");
    	  sb << i;                             
          xmlSetProp(pn, (const xmlChar*) "slotid", (const xmlChar*)sb.str().c_str());            
          sb.str("");
          
      	  nn = xmlNewNode(NULL,(const xmlChar*)"item");
          sb << items[i]->getID();
          xmlSetProp(nn, (const xmlChar*) "id", (const xmlChar*)sb.str().c_str());            
          sb.str("");
          
	      xmlAddChild(pn, nn);
	      xmlAddChild(sn, pn);
          }
      }
   xmlAddChild(root, sn);
	
	//Save the character
    if (xmlSaveFile(filename.c_str(), doc))
       {
       #ifdef __DEBUG__
       std::cout << "\tSaved character succefully!\n";
       #endif
       xmlFreeDoc(doc);
       }
    else
       {
       std::cout << "\tCouldn't save character =(\n";
       xmlFreeDoc(doc);
       }
}

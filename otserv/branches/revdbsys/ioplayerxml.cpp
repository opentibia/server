//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Player Loader/Saver implemented with XML
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
#include "otpch.h"

#include <sstream>

#include "ioplayer.h"
#include "ioplayerxml.h"
#include "ioaccount.h"
#include "item.h"
#include "player.h"
#include "tools.h"
#include "configmanager.h"

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/threads.h>

xmlMutexPtr xmlmutex;

extern ConfigManager g_config;

IOPlayerXML::IOPlayerXML()
{
	if(xmlmutex == NULL){
		xmlmutex = xmlNewMutex();
	}
}

bool IOPlayerXML::loadPlayer(Player* player, std::string name)
{
	std::string datadir = g_config.getString(ConfigManager::DATA_DIRECTORY);
	std::string filename = datadir + "players/" + name + ".xml";
	toLowerCaseString(filename); //all players are saved as lowercase

	xmlMutexLock(xmlmutex);
	xmlDocPtr doc = xmlParseFile(filename.c_str());

	if(doc){
		bool isLoaded = true;
		xmlNodePtr root, p;

		root = xmlDocGetRootElement(doc);

		if(xmlStrcmp(root->name,(const xmlChar*)"player") != 0){
			std::cout << "Error while loading " << name << std::endl;
		}

		int intValue;
		std::string strValue;

		uint32_t account = 0;
		if(readXMLInteger(root, "account", intValue)){
			account = intValue;
		}
		else{
		  xmlFreeDoc(doc);
		  xmlMutexUnlock(xmlmutex);
		  return false;
		}

		//need to unlock and relock in order to load xml account (both share the same mutex)
		xmlMutexUnlock(xmlmutex);
		Account a = IOAccount::instance()->loadAccount(account);
		xmlMutexLock(xmlmutex);

		player->password = a.password;
		if(a.accnumber == 0 || a.accnumber != account){
		  xmlFreeDoc(doc);
		  xmlMutexUnlock(xmlmutex);
		  return false;
		}

		uint32_t _guid = 0;
		std::string _name = player->getName();
		if(getGuidByName(_guid, _name)){
			player->setGUID(_guid);
		}

		player->accountNumber = account;

		if(readXMLInteger(root, "sex", intValue)){
			player->setSex((playersex_t)intValue);
		}
		else
			isLoaded = false;

		if(readXMLInteger(root, "lookdir", intValue)){
			player->setDirection((Direction)intValue);
		}
		else
			isLoaded = false;

		if(readXMLInteger(root, "exp", intValue)){
			player->experience = intValue;
		}
		else
			isLoaded = false;

		if(readXMLInteger(root, "level", intValue)){
			player->level = intValue;
		}
		else
			isLoaded = false;

		if(readXMLInteger(root, "maglevel", intValue)){
			player->magLevel = intValue;
		}
		else
			isLoaded = false;

		if(readXMLInteger(root, "soul", intValue)){
			player->soul = intValue;
		}

		if(readXMLInteger(root, "voc", intValue)){
			player->setVocation(intValue);
		}
		else
			isLoaded = false;

		if(readXMLInteger(root, "access", intValue)){
			player->accessLevel = intValue;
		}
		else
			isLoaded = false;

		uint64_t int64Value;
		if(readXMLInteger64(root, "groupflags", int64Value)){
			player->groupFlags = int64Value;
		}

		if(readXMLInteger(root, "cap", intValue)){
			player->capacity = intValue;
		}
		else
			isLoaded = false;

		if(readXMLInteger(root, "maxdepotitems", intValue)){
			player->maxDepotLimit = intValue;
		}
		else
			isLoaded = false;

		player->updateBaseSpeed();

		if(readXMLInteger(root, "lastlogin", intValue)){
			player->lastLoginSaved = intValue;
		}
		else
			player->lastLoginSaved = 0;

		if(readXMLInteger(root, "loss_experience", intValue)){
			player->lossPercent[LOSS_EXPERIENCE] = intValue;
		}

		if(readXMLInteger(root, "loss_mana", intValue)){
            player->lossPercent[LOSS_MANASPENT] = intValue;
		}

		if(readXMLInteger(root, "loss_skills", intValue)){
            player->lossPercent[LOSS_SKILLTRIES] = intValue;
		}

		p = root->children;

		while(p){
			if(xmlStrcmp(p->name, (const xmlChar*)"mana") == 0){

				if(readXMLInteger(p, "now", intValue)){
					player->mana = intValue;
				}
				else
					isLoaded = false;

				if(readXMLInteger(p, "max", intValue)){
					player->manaMax = intValue;
				}
				else
					isLoaded = false;

				if(readXMLInteger(p, "spent", intValue)){
					player->manaSpent = intValue;
				}
				else
					isLoaded = false;

			}
			else if(xmlStrcmp(p->name, (const xmlChar*)"health") == 0){

				if(readXMLInteger(p, "now", intValue)){
					player->health = intValue;

					if(player->health <= 0)
						player->health = 100;
				}
				else
					isLoaded = false;

				if(readXMLInteger(p, "max", intValue)){
					player->healthMax = intValue;
				}
				else
					isLoaded = false;

				/*
				if(readXMLInteger(p, "food", intValue)){
					player->food = intValue;
				}
				else
					isLoaded = false;
				*/
			}
			else if(xmlStrcmp(p->name, (const xmlChar*)"look") == 0){

				if(readXMLInteger(p, "type", intValue)){
					player->defaultOutfit.lookType = intValue;
					player->currentOutfit.lookType = intValue;
				}
				else
					isLoaded = false;

				//player->lookMaster = player->lookType;

				if(readXMLInteger(p, "head", intValue)){
					player->defaultOutfit.lookHead = intValue;
					player->currentOutfit.lookHead = intValue;
				}
				else
					isLoaded = false;

				if(readXMLInteger(p, "body", intValue)){
					player->defaultOutfit.lookBody = intValue;
					player->currentOutfit.lookBody = intValue;
				}
				else
					isLoaded = false;

				if(readXMLInteger(p, "legs", intValue)){
					player->defaultOutfit.lookLegs = intValue;
					player->currentOutfit.lookLegs = intValue;
				}
				else
					isLoaded = false;

				if(readXMLInteger(p, "feet", intValue)){
					player->defaultOutfit.lookFeet = intValue;
					player->currentOutfit.lookFeet = intValue;
				}
				else
					isLoaded = false;
				
				if(readXMLInteger(p, "addons", intValue)){
					player->defaultOutfit.lookAddons = intValue;
					player->currentOutfit.lookAddons = intValue;
				}
				else
					isLoaded = false;
			}
			else if(xmlStrcmp(p->name, (const xmlChar*)"spawn") == 0){

				if(readXMLInteger(p, "x", intValue)){
					player->loginPosition.x = intValue;
				}
				else
					isLoaded = false;

				if(readXMLInteger(p, "y", intValue)){
					player->loginPosition.y = intValue;
				}
				else
					isLoaded = false;

				if(readXMLInteger(p, "z", intValue)){
					player->loginPosition.z = intValue;
				}
				else
					isLoaded = false;
			}
			else if(xmlStrcmp(p->name, (const xmlChar*)"temple") == 0){

				if(readXMLInteger(p, "x", intValue)){
					player->masterPos.x = intValue;
				}
				else
					isLoaded = false;

				if(readXMLInteger(p, "y", intValue)){
					player->masterPos.y = intValue;
				}
				else
					isLoaded = false;

				if(readXMLInteger(p, "z", intValue)){
					player->masterPos.z = intValue;
				}
				else
					isLoaded = false;
			}
			else if(xmlStrcmp(p->name, (const xmlChar*)"guild") == 0){

				if(readXMLString(p, "name", strValue)){
					player->guildName = strValue;
				}
				else
					isLoaded = false;

				if(readXMLString(p, "rank", strValue)){
					player->guildRank = strValue;
				}
				else
					isLoaded = false;

				if(readXMLString(p, "nick", strValue)){
					player->guildNick = strValue;
				}
				else
					isLoaded = false;

				if(readXMLInteger(p, "id", intValue)){
					player->guildId = intValue;
				}
				else
					isLoaded = false;
			}
			else if(xmlStrcmp(p->name, (const xmlChar*)"skills") == 0){
				xmlNodePtr tmpNode = p->children;
				while(tmpNode){
					int s_id = 0;
					int s_lvl = 0;
					int s_tries = 0;
					if(xmlStrcmp(tmpNode->name, (const xmlChar*)"skill") == 0){

						if(readXMLInteger(tmpNode, "skillid", intValue)){
							s_id = intValue;
						}
						else
							isLoaded = false;

						if(readXMLInteger(tmpNode, "level", intValue)){
							s_lvl = intValue;
						}
						else
							isLoaded = false;

						if(readXMLInteger(tmpNode, "tries", intValue)){
							s_tries = intValue;
						}
						else
							isLoaded = false;

						player->skills[s_id][SKILL_LEVEL]=s_lvl;
						player->skills[s_id][SKILL_TRIES]=s_tries;
					}

					tmpNode = tmpNode->next;
				}
			}
			else if(xmlStrcmp(p->name, (const xmlChar*)"inventory") == 0){
				xmlNodePtr slotNode = p->children;
				while(slotNode){
					if(xmlStrcmp(slotNode->name, (const xmlChar*)"slot") == 0){
						int32_t itemId = 0;
						int32_t slotId = 0;

						if(readXMLInteger(slotNode, "slotid", intValue)){
							slotId = intValue;
						}
						else{
							isLoaded = false;
						}

						xmlNodePtr itemNode = slotNode->children;
						while(itemNode){
							if(xmlStrcmp(itemNode->name, (const xmlChar*)"item") == 0){
								if(readXMLInteger(itemNode, "id", intValue)){
									itemId = intValue;

									Item* item = Item::CreateItem(itemId);
									if(item){
										item->unserialize(itemNode);
										player->__internalAddThing(slotId, item);
									}
								}
								else{
									isLoaded = false;
								}
							}

							itemNode = itemNode->next;
						}
						/*
						if(readXMLInteger(slotNode->children, "id", intValue)){
							itemId = intValue;
						}
						else
							isLoaded = false;

						Item* item = Item::CreateItem(itemId);
						if(item){
							item->unserialize(slotNode->children);
							player->__internalAddThing(slotId, item);
						}
						*/
					}

					slotNode = slotNode->next;
				}
			}
			else if(xmlStrcmp(p->name, (const xmlChar*)"depots") == 0){
				xmlNodePtr tmpNode = p->children;
				while(tmpNode){
					if(xmlStrcmp(tmpNode->name, (const xmlChar*)"depot") == 0){
						int32_t depotId = 0;
						int32_t itemId = 0;

						if(readXMLInteger(tmpNode, "depotid", intValue)){
							depotId = intValue;
						}
						else{
							isLoaded = false;
						}

						xmlNodePtr itemNode = tmpNode->children;
						while(itemNode){
							if(xmlStrcmp(itemNode->name, (const xmlChar*)"item") == 0){
								if(readXMLInteger(itemNode, "id", intValue)){
									itemId = intValue;

									if(itemId != 0){
										Depot* myDepot = new Depot(itemId);
										myDepot->useThing2();
										myDepot->unserialize(itemNode);
										myDepot->setDepotId(depotId);

										player->addDepot(myDepot, depotId);
									}

									break;
								}
								else{
									isLoaded = false;
								}
							}

							itemNode = itemNode->next;
						}

						/*
						if(readXMLInteger(tmpNode->children, "id", intValue)){
							itemId = intValue;
						}
						else
							isLoaded = false;

						if(itemId != 0){
							Depot* myDepot = new Depot(itemId);
							myDepot->useThing2();
							myDepot->unserialize(tmpNode->children);

							player->addDepot(myDepot, depotId);
						}
						*/
					}

					tmpNode = tmpNode->next;
				}
			}
			else if(xmlStrcmp(p->name, (const xmlChar*)"storage") == 0){
				xmlNodePtr tmpNode = p->children;
				while(tmpNode){
					if(xmlStrcmp(tmpNode->name, (const xmlChar*)"data") == 0){
						uint32_t key = 0;
						int32_t value = 0;

						if(readXMLInteger(tmpNode, "key", intValue)){
							key = intValue;
						}

						if(readXMLInteger(tmpNode, "value", intValue)){
							value = intValue;
						}

						player->addStorageValue(key, value);
					}

					tmpNode = tmpNode->next;
				}
			}
			else if(xmlStrcmp(p->name, (const xmlChar*)"viplist") == 0){
				xmlNodePtr tmpNode = p->children;
				while(tmpNode){
					if(xmlStrcmp(tmpNode->name, (const xmlChar*)"vip") == 0){
						if(readXMLInteger(tmpNode, "playerguid", intValue)){
							std::string dummy_str;
							player->addVIP(intValue, dummy_str, false, true);
						}
					}

					tmpNode = tmpNode->next;
				}
			}
			else if(xmlStrcmp(p->name, (const xmlChar*)"knownspells") == 0){
				xmlNodePtr tmpNode = p->children;
				while(tmpNode){
					if(xmlStrcmp(tmpNode->name, (const xmlChar*)"spell") == 0){
						if(readXMLString(tmpNode, "name", strValue)){
							player->learnedInstantSpellList.push_back(strValue);
						}
					}

					tmpNode = tmpNode->next;
				}
			}
			else if(xmlStrcmp(p->name, (const xmlChar*)"conditions") == 0){
				int typeValue;
				int idValue;
				int ticksValue;

				xmlNodePtr tmpNode = p->children;
				while(tmpNode){
					if(xmlStrcmp(tmpNode->name, (const xmlChar*)"condition") == 0){
						if(readXMLInteger(tmpNode, "id", idValue) &&
							readXMLInteger(tmpNode, "type", typeValue) &&
							readXMLInteger(tmpNode, "ticks", ticksValue))
						{
							Condition* condition = Condition::createCondition((ConditionId_t)idValue, (ConditionType_t)typeValue, ticksValue, 0);
							if(condition){
								if(condition->unserialize(tmpNode)){
									player->storedConditionList.push_back(condition);
								}
								else{
									delete condition;
								}
							}
						}
					}

					tmpNode = tmpNode->next;
				}
			}
			p = p->next;
		}

		player->updateInventoryWeigth();
		player->updateItemsLight(true);
		player->setSkillsPercents();

		std::cout << "loaded " << filename << std::endl;
		xmlFreeDoc(doc);
		xmlMutexUnlock(xmlmutex);
		return isLoaded;
	}

	xmlMutexUnlock(xmlmutex);
	return false;
}

bool IOPlayerXML::savePlayer(Player* player)
{
	std::string datadir = g_config.getString(ConfigManager::DATA_DIRECTORY);
	std::string filename = datadir + "players/" + player->getName() + ".xml";
	toLowerCaseString(filename); //store all player files in lowercase

	std::stringstream sb;

	xmlMutexLock(xmlmutex);
	xmlNodePtr nn, sn, pn, root;

	xmlDocPtr doc = xmlNewDoc((const xmlChar*)"1.0");
	doc->children = xmlNewDocNode(doc, NULL, (const xmlChar*)"player", NULL);
	root = doc->children;

	player->preSave();

	sb << player->getName();
	xmlSetProp(root, (const xmlChar*)"name", (const xmlChar*)sb.str().c_str());

	sb.str("");
	sb << player->getGUID();
	xmlSetProp(root, (const xmlChar*)"id", (const xmlChar*)sb.str().c_str());

	sb.str("");
	sb << player->accountNumber;
	xmlSetProp(root, (const xmlChar*)"account", (const xmlChar*)sb.str().c_str());

	sb.str("");
	sb << player->sex;
	xmlSetProp(root, (const xmlChar*)"sex", (const xmlChar*)sb.str().c_str());

	sb.str("");
	sb << (int)player->getDirection();
	xmlSetProp(root, (const xmlChar*)"lookdir", (const xmlChar*)sb.str().c_str());

	/*
	sb << player->getDirection();

	if(sb.str() == "North"){
		sb.str("");
		sb << "0";
	}
	if(sb.str() == "East") {sb.str(""); sb << "1";}
	if(sb.str() == "South"){sb.str(""); sb << "2";}
	if(sb.str() == "West") {sb.str(""); sb << "3";}
	xmlSetProp(root, (const xmlChar*)"lookdir", (const xmlChar*)sb.str().c_str());  sb.str("");
	*/

	sb.str("");
	sb << player->experience;
	xmlSetProp(root, (const xmlChar*)"exp", (const xmlChar*)sb.str().c_str());

	sb.str("");
	sb << (int)player->getVocationId();
	xmlSetProp(root, (const xmlChar*)"voc", (const xmlChar*)sb.str().c_str());

	sb.str("");
	sb << player->level;
	xmlSetProp(root, (const xmlChar*)"level", (const xmlChar*)sb.str().c_str());

	sb.str("");
	sb << player->accessLevel;
	xmlSetProp(root, (const xmlChar*)"access", (const xmlChar*)sb.str().c_str());

	sb.str("");
	sb << player->groupFlags;
	xmlSetProp(root, (const xmlChar*)"groupflags", (const xmlChar*)sb.str().c_str());

	sb.str("");
	sb << player->getCapacity();
	xmlSetProp(root, (const xmlChar*)"cap", (const xmlChar*)sb.str().c_str());

	sb.str("");
	sb << player->magLevel;
	xmlSetProp(root, (const xmlChar*)"maglevel", (const xmlChar*)sb.str().c_str());

	sb.str("");
	sb << player->soul;
	xmlSetProp(root, (const xmlChar*)"soul", (const xmlChar*)sb.str().c_str());

	sb.str("");
	sb << player->maxDepotLimit;
	xmlSetProp(root, (const xmlChar*)"maxdepotitems", (const xmlChar*)sb.str().c_str());

	sb.str("");
	sb << player->lastlogin;
	xmlSetProp(root, (const xmlChar*)"lastlogin", (const xmlChar*)sb.str().c_str());

	sb.str("");
	sb << player->lossPercent[LOSS_EXPERIENCE];
	xmlSetProp(root, (const xmlChar*)"loss_experience", (const xmlChar*)sb.str().c_str());

	sb.str("");
	sb << player->lossPercent[LOSS_MANASPENT];
	xmlSetProp(root, (const xmlChar*)"loss_mana", (const xmlChar*)sb.str().c_str());

	sb.str("");
	sb << player->lossPercent[LOSS_SKILLTRIES];
	xmlSetProp(root, (const xmlChar*)"loss_skills", (const xmlChar*)sb.str().c_str());

	pn = xmlNewNode(NULL,(const xmlChar*)"spawn");

	sb.str("");
	sb << player->getLoginPosition().x;
	xmlSetProp(pn, (const xmlChar*)"x", (const xmlChar*)sb.str().c_str());

	sb.str("");
	sb << player->getLoginPosition().y;
	xmlSetProp(pn,  (const xmlChar*)"y", (const xmlChar*)sb.str().c_str());

	sb.str("");
	sb << player->getLoginPosition().z;
	xmlSetProp(pn,  (const xmlChar*)"z", (const xmlChar*)sb.str().c_str());

	xmlAddChild(root, pn);

	pn = xmlNewNode(NULL,(const xmlChar*)"temple");

	sb.str("");
	sb << player->masterPos.x;
	xmlSetProp(pn, (const xmlChar*)"x", (const xmlChar*)sb.str().c_str());

	sb.str("");
	sb << player->masterPos.y;
	xmlSetProp(pn, (const xmlChar*)"y", (const xmlChar*)sb.str().c_str());

	sb.str("");
	sb << player->masterPos.z;
	xmlSetProp(pn, (const xmlChar*)"z", (const xmlChar*)sb.str().c_str());
	xmlAddChild(root, pn);

	pn = xmlNewNode(NULL,(const xmlChar*)"health");

	sb.str("");
	sb << player->health;
	xmlSetProp(pn, (const xmlChar*)"now", (const xmlChar*)sb.str().c_str());
	
	sb.str("");
	sb << player->healthMax;
	xmlSetProp(pn, (const xmlChar*)"max", (const xmlChar*)sb.str().c_str());

	xmlAddChild(root, pn);

	pn = xmlNewNode(NULL,(const xmlChar*)"mana");

	sb.str("");
	sb << player->mana;
	xmlSetProp(pn, (const xmlChar*)"now", (const xmlChar*)sb.str().c_str());

	sb.str("");
	sb << player->manaMax;
	xmlSetProp(pn, (const xmlChar*)"max", (const xmlChar*)sb.str().c_str());

	sb.str("");
	sb << player->manaSpent;
	xmlSetProp(pn, (const xmlChar*)"spent", (const xmlChar*)sb.str().c_str());
	xmlAddChild(root, pn);

	//upconversion of uchar(uint8_t) to get value not character into the stream
	pn = xmlNewNode(NULL,(const xmlChar*)"look");

	sb.str("");
	sb << (int16_t)player->defaultOutfit.lookType;
	xmlSetProp(pn, (const xmlChar*)"type", (const xmlChar*)sb.str().c_str());
	
	sb.str("");
	sb << (int16_t)player->defaultOutfit.lookHead;
	xmlSetProp(pn, (const xmlChar*)"head", (const xmlChar*)sb.str().c_str());
	
	sb.str("");
	sb << (int16_t)player->defaultOutfit.lookBody;
	xmlSetProp(pn, (const xmlChar*)"body", (const xmlChar*)sb.str().c_str());
	
	sb.str("");
	sb << (int16_t)player->defaultOutfit.lookLegs;
	xmlSetProp(pn, (const xmlChar*)"legs", (const xmlChar*)sb.str().c_str());
	
	sb.str("");
	sb << (int16_t)player->defaultOutfit.lookFeet;
	xmlSetProp(pn, (const xmlChar*)"feet", (const xmlChar*)sb.str().c_str());
	
	sb.str("");
	sb << (int16_t)player->defaultOutfit.lookAddons;
	xmlSetProp(pn, (const xmlChar*)"addons", (const xmlChar*)sb.str().c_str());

	xmlAddChild(root, pn);

	pn = xmlNewNode(NULL,(const xmlChar*)"guild");

	sb.str("");
	sb << player->guildName;
	xmlSetProp(pn, (const xmlChar*)"name", (const xmlChar*)sb.str().c_str());

	sb.str("");
	sb << player->guildRank;
	xmlSetProp(pn, (const xmlChar*)"rank", (const xmlChar*)sb.str().c_str());
	
	sb.str("");
	sb << player->guildNick;
	xmlSetProp(pn, (const xmlChar*)"nick", (const xmlChar*)sb.str().c_str());
	
	sb.str("");
	sb << player->guildId;
	xmlSetProp(pn, (const xmlChar*)"id",   (const xmlChar*)sb.str().c_str());

	xmlAddChild(root, pn);

	sn = xmlNewNode(NULL,(const xmlChar*)"skills");
	for(int i = 0; i <= 6; i++){
	  pn = xmlNewNode(NULL,(const xmlChar*)"skill");

		sb.str("");
		sb << i;
		xmlSetProp(pn, (const xmlChar*)"skillid", (const xmlChar*)sb.str().c_str());
		
		sb.str("");
		sb << player->skills[i][SKILL_LEVEL];
		xmlSetProp(pn, (const xmlChar*)"level",   (const xmlChar*)sb.str().c_str());
		
		sb.str("");
		sb << player->skills[i][SKILL_TRIES];
		xmlSetProp(pn, (const xmlChar*)"tries",   (const xmlChar*)sb.str().c_str());

		xmlAddChild(sn, pn);
	}

	xmlAddChild(root, sn);

	sn = xmlNewNode(NULL,(const xmlChar*)"inventory");
	for(int i = 1; i <= 10; i++){
		if(player->inventory[i]){
			pn = xmlNewNode(NULL,(const xmlChar*)"slot");

			sb.str("");
			sb << i;
			xmlSetProp(pn, (const xmlChar*)"slotid", (const xmlChar*)sb.str().c_str());

			nn = player->inventory[i]->serialize();

			xmlAddChild(pn, nn);
			xmlAddChild(sn, pn);
		}
	}

	xmlAddChild(root, sn);

	sn = xmlNewNode(NULL,(const xmlChar*)"depots");

	for(DepotMap::reverse_iterator it = player->depots.rbegin(); it !=player->depots.rend()  ;++it){
		pn = xmlNewNode(NULL,(const xmlChar*)"depot");

		sb.str("");
		sb << it->first;
		xmlSetProp(pn, (const xmlChar*)"depotid", (const xmlChar*)sb.str().c_str());

		nn = (it->second)->serialize();
		xmlAddChild(pn, nn);
		xmlAddChild(sn, pn);
	}

	xmlAddChild(root, sn);

	sn = xmlNewNode(NULL,(const xmlChar*)"storage");
	player->genReservedStorageRange();
	for(StorageMap::const_iterator cit = player->getStorageIteratorBegin(); cit != player->getStorageIteratorEnd();cit++){
		pn = xmlNewNode(NULL,(const xmlChar*)"data");
		sb.str("");
		sb << cit->first;
		xmlSetProp(pn, (const xmlChar*)"key", (const xmlChar*)sb.str().c_str());

		sb.str("");
		sb << cit->second;
		xmlSetProp(pn, (const xmlChar*)"value", (const xmlChar*)sb.str().c_str());

		xmlAddChild(sn, pn);
	}
	xmlAddChild(root, sn);
	
	sn = xmlNewNode(NULL,(const xmlChar*)"viplist");
	for(VIPListSet::iterator it = player->VIPList.begin(); it != player->VIPList.end(); it++){
		pn = xmlNewNode(NULL,(const xmlChar*)"vip");

		sb.str("");
		sb << *it;
		xmlSetProp(pn, (const xmlChar*)"playerguid", (const xmlChar*)sb.str().c_str());

		xmlAddChild(sn, pn);
	}
	xmlAddChild(root, sn);

	sn = xmlNewNode(NULL,(const xmlChar*)"knownspells");
	for(LearnedInstantSpellList::const_iterator it = player->learnedInstantSpellList.begin();
			it != player->learnedInstantSpellList.end(); it++){
		pn = xmlNewNode(NULL,(const xmlChar*)"spell");

		sb.str("");
		sb << *it;
		xmlSetProp(pn, (const xmlChar*)"name", (const xmlChar*)sb.str().c_str());

		xmlAddChild(sn, pn);
	}
	xmlAddChild(root, sn);

	sn = xmlNewNode(NULL,(const xmlChar*)"conditions");

	for(ConditionList::const_iterator it = player->conditions.begin(); it != player->conditions.end(); ++it){
		if((*it)->isPersistent()){
			nn = (*it)->serialize();
			xmlAddChild(sn, nn);
		}
	}

	xmlAddChild(root, sn);

	//Save the character
	//if(xmlSaveFile(filename.c_str(), doc)){
	bool result = xmlSaveFormatFileEnc(filename.c_str(), doc, "UTF-8", 1);
#ifdef __DEBUG__
	if(result){
		std::cout << "\tSaved character succefully!\n";
	}
	else{
		std::cout << "\tCouldn't save character =(\n";
	}
#endif

	xmlFreeDoc(doc);
	xmlMutexUnlock(xmlmutex);
	return result;
}

bool IOPlayerXML::getGuidByName(uint32_t& guid, std::string& name)
{
	bool a;
	return getGuidByNameEx(guid, a, name);
}

bool IOPlayerXML::getGuidByNameEx(uint32_t& guid, bool& specialVip, std::string& name)
{
	//load players.xml to get guid
	std::string datadir = g_config.getString(ConfigManager::DATA_DIRECTORY);
	std::string playersfile = datadir + "players/" + "players.xml";

	xmlDocPtr doc = xmlParseFile(playersfile.c_str());

	bool isSuccess = false;

	if(doc){
		int intValue;
		std::string strValue;

		xmlNodePtr root = xmlDocGetRootElement(doc);

		if(xmlStrcmp(root->name,(const xmlChar*)"players") != 0){
			xmlFreeDoc(doc);
			return false;
		}

		xmlNodePtr playerNode = root->children;
		while(playerNode){
			if(xmlStrcmp(playerNode->name,(const xmlChar*)"player") == 0){
				if(readXMLString(playerNode, "name", strValue)){

					if(strcasecmp(strValue.c_str(), name.c_str()) == 0){

						if(readXMLInteger(playerNode, "guid", intValue)){
							guid = intValue;
							isSuccess = true;

							break;
						}
					}
				}
			}

			playerNode = playerNode->next;
		}
	}

	xmlFreeDoc(doc);

	if(!isSuccess){
		return false;
	}

	//load player file to get "real" name, access level etc.
	std::string playerfile = datadir + "players/" + name + ".xml";
	toLowerCaseString(playerfile);

	doc = xmlParseFile(playerfile.c_str());
	specialVip = false;
	if(doc){
		int intValue;
		std::string strValue;

		xmlNodePtr root = xmlDocGetRootElement(doc);

		if(xmlStrcmp(root->name,(const xmlChar*)"player") != 0){
			xmlFreeDoc(doc);
			return false;
		}

		if(readXMLString(root, "name", strValue)){
			name = strValue;
		}
		else
			isSuccess = false;

		if(readXMLInteger(root, "access", intValue)){
			if(intValue > 0){
				specialVip = true;
			}
		}
		else
			isSuccess = false;
	}

	xmlFreeDoc(doc);

	return isSuccess;
}

bool IOPlayerXML::getNameByGuid(uint32_t guid, std::string& name)
{
	std::string datadir = g_config.getString(ConfigManager::DATA_DIRECTORY);
	std::string filename = datadir + "players/" + "players.xml";

	xmlDocPtr doc = xmlParseFile(filename.c_str());

	bool isSuccess = false;

	if(doc){
		isSuccess = true;

		int intValue;
		std::string strValue;

		xmlNodePtr root = xmlDocGetRootElement(doc);

		if(xmlStrcmp(root->name,(const xmlChar*)"players") != 0){
			xmlFreeDoc(doc);
			return false;
		}

		xmlNodePtr playerNode = root->children;
		while(playerNode){
			if(xmlStrcmp(playerNode->name,(const xmlChar*)"player") == 0){
				if(readXMLInteger(playerNode, "guid", intValue)){

					if((uint32_t)intValue == guid){

						if(readXMLString(playerNode, "name", strValue)){

							name = strValue;
							isSuccess = true;
							break;
						}
					}
				}
			}

			playerNode = playerNode->next;
		}
	}

	xmlFreeDoc(doc);

	return isSuccess;
}

bool IOPlayerXML::getGuildIdByName(uint32_t& guildId, const std::string& guildName)
{
	return false;
}

bool IOPlayerXML::playerExists(std::string name)
{
	std::string datadir = g_config.getString(ConfigManager::DATA_DIRECTORY);
	std::string filename = datadir + "players/" + name + ".xml";
	std::transform(filename.begin(), filename.end(), filename.begin(), tolower);

	return fileExists(filename.c_str());
}

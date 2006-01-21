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

#include <sstream>

#include <boost/filesystem/operations.hpp>

#include "ioplayer.h"
#include "ioplayerxml.h"
#include "ioaccount.h"
#include "item.h"
#include "luascript.h"

xmlMutexPtr xmlmutex;

extern LuaScript g_config;

IOPlayerXML::IOPlayerXML()
{
	if(xmlmutex == NULL){
		xmlmutex = xmlNewMutex();
	}
}

bool IOPlayerXML::loadPlayer(Player* player, std::string name)
{
	std::string datadir = g_config.getGlobalString("datadir");
	std::string filename = datadir + "players/" + name + ".xml";
	std::transform (filename.begin(),filename.end(), filename.begin(), tolower);

	xmlDocPtr doc;	
	xmlMutexLock(xmlmutex);
	doc = xmlParseFile(filename.c_str());

	if(doc){
		bool isLoaded = true;
		xmlNodePtr root, tmp, p, slot;
		char* nodeValue = NULL;
		root=xmlDocGetRootElement(doc);

		if(xmlStrcmp(root->name,(const xmlChar*) "player")){
			std::cout << "Strange. Player-Savefile was no savefile for " << name << std::endl;
		}

		p = root->children;

		nodeValue = (char*)xmlGetProp(root, (const xmlChar *) "account");
		int account = atoi(nodeValue);
		
		//need to unlock and relock in order to load xml account (both share the same mutex)
		xmlMutexUnlock(xmlmutex);
		Account a = IOAccount::instance()->loadAccount(account);
		xmlMutexLock(xmlmutex);
		
		player->password = a.password;
		if(a.accnumber == 0 || a.accnumber != (unsigned long)account){
		  xmlFreeDoc(doc);		  
		  xmlMutexUnlock(xmlmutex);		  
		  return false;
		}

		nodeValue = (char*)xmlGetProp(root, (const xmlChar *) "id");
		if(nodeValue){
			player->setGUID(atoi(nodeValue));
			xmlFreeOTSERV(nodeValue);
		}

		nodeValue = (char*)xmlGetProp(root, (const xmlChar *) "account");
		if(nodeValue){
			player->accountNumber = atoi(nodeValue);
			xmlFreeOTSERV(nodeValue);
		}
		else
			isLoaded = false;

		nodeValue = (char*)xmlGetProp(root, (const xmlChar *) "sex");
		if(nodeValue){
			player->sex=(playersex_t)atoi(nodeValue);
			xmlFreeOTSERV(nodeValue);
		}
		else
			isLoaded = false;

		nodeValue = (char*)xmlGetProp(root, (const xmlChar *) "lookdir");
		if(nodeValue){
			player->setDirection((Direction)atoi(nodeValue));
			xmlFreeOTSERV(nodeValue);
		}
		else
			isLoaded = false;

		nodeValue = (char*)xmlGetProp(root, (const xmlChar *) "exp");
		if(nodeValue){
			player->experience=atoi(nodeValue);
			xmlFreeOTSERV(nodeValue);
		}
		else
			isLoaded = false;
		        
		nodeValue = (char*)xmlGetProp(root, (const xmlChar *) "level");
		if(nodeValue){
			player->level=atoi(nodeValue);
			xmlFreeOTSERV(nodeValue);
		}
		else
			isLoaded = false;

		nodeValue = (char*)xmlGetProp(root, (const xmlChar *) "maglevel");
		if(nodeValue){
			player->maglevel=atoi(nodeValue);
			xmlFreeOTSERV(nodeValue);
		}
		else
			isLoaded = false;

		nodeValue = (char*)xmlGetProp(root, (const xmlChar *) "voc");
		if(nodeValue){
			player->vocation = (playervoc_t)atoi(nodeValue);
			xmlFreeOTSERV(nodeValue);
		}
		else
			isLoaded = false;

		nodeValue = (char*)xmlGetProp(root, (const xmlChar *) "access");
		if(nodeValue){
			player->access=atoi(nodeValue);
			xmlFreeOTSERV(nodeValue);
		}
		else
			isLoaded = false;

		nodeValue = (char*)xmlGetProp(root, (const xmlChar *) "cap");
		if(nodeValue){
			player->capacity = atoi(nodeValue);
			xmlFreeOTSERV(nodeValue);
		}
		else
			isLoaded = false;

		nodeValue = (char*)xmlGetProp(root, (const xmlChar *) "maxdepotitems");
		if(nodeValue){
			player->maxDepotLimit = atoi(nodeValue);
			xmlFreeOTSERV(nodeValue);
		}
		else
			isLoaded = false;

		player->setNormalSpeed();
		
		nodeValue = (char*)xmlGetProp(root, (const xmlChar *) "lastlogin");
		if(nodeValue){
			player->lastLoginSaved = atoi(nodeValue);
			xmlFreeOTSERV(nodeValue);
		}
		else{
			player->lastLoginSaved = 0;
		}
		
		//level percent
		player->level_percent  = (unsigned char)(100*(player->experience-player->getExpForLv(player->level))/(1.*player->getExpForLv(player->level+1)-player->getExpForLv(player->level)));
		while(p){
			std::string str=(char*)p->name;
			if(str=="mana"){
				nodeValue = (char*)xmlGetProp(p, (const xmlChar *) "now");
				if(nodeValue){
					player->mana=atoi(nodeValue);
					xmlFreeOTSERV(nodeValue);
				}
				else
					isLoaded = false;

				nodeValue = (char*)xmlGetProp(p, (const xmlChar *) "max");
				if(nodeValue){
					player->manamax=atoi(nodeValue);
					xmlFreeOTSERV(nodeValue);
				}
				else
					isLoaded = false;

				nodeValue = (char*)xmlGetProp(p, (const xmlChar *) "spent");
				if(nodeValue){
					player->manaspent=atoi(nodeValue);
					xmlFreeOTSERV(nodeValue);
				}
				else
					isLoaded = false;

				player->maglevel_percent  = (unsigned char)(100*(player->manaspent/(1.*player->getReqMana(player->maglevel+1, player->vocation))));
			}
			else if(str=="health"){
				nodeValue = (char*)xmlGetProp(p, (const xmlChar *) "now");
				if(nodeValue){
					player->health=atoi(nodeValue);
					xmlFreeOTSERV(nodeValue);
				}
				else
					isLoaded = false;
				
				nodeValue = (char*)xmlGetProp(p, (const xmlChar *) "max");
				if(nodeValue){
					player->healthmax = atoi(nodeValue);
					xmlFreeOTSERV(nodeValue);
				}
				else
					isLoaded = false;

				nodeValue = (char*)xmlGetProp(p, (const xmlChar *) "food");
				if(nodeValue) {
					player->food=atoi(nodeValue);
					xmlFreeOTSERV(nodeValue);
				}
				else
					isLoaded = false;
			}
			else if(str=="look"){
				nodeValue = (char*)xmlGetProp(p, (const xmlChar *) "type");
				if(nodeValue){
					player->looktype=atoi(nodeValue);
					xmlFreeOTSERV(nodeValue);
				}
				else
					isLoaded = false;

				player->lookmaster = player->looktype;
				
				nodeValue = (char*)xmlGetProp(p, (const xmlChar *) "head");
				if(nodeValue){
					player->lookhead=atoi(nodeValue);
					xmlFreeOTSERV(nodeValue);
				}
				else
					isLoaded = false;

				nodeValue = (char*)xmlGetProp(p, (const xmlChar *) "body");
				if(nodeValue){
					player->lookbody=atoi(nodeValue);
					xmlFreeOTSERV(nodeValue);
				}
				else
					isLoaded = false;

				nodeValue = (char*)xmlGetProp(p, (const xmlChar *) "legs");
				if(nodeValue){
					player->looklegs=atoi(nodeValue);
					xmlFreeOTSERV(nodeValue);
				}
				else
					isLoaded = false;

				nodeValue = (char*)xmlGetProp(p, (const xmlChar *) "feet");
				if(nodeValue){
					player->lookfeet=atoi(nodeValue);
					xmlFreeOTSERV(nodeValue);
				}
				else
					isLoaded = false;
			}
			else if(str=="spawn"){
				Position spawnPos;
				spawnPos.x = 0;
				spawnPos.y = 0;
				spawnPos.z = 0;

				nodeValue = (char*)xmlGetProp(p, (const xmlChar *) "x");
				if(nodeValue) {
					spawnPos.x = atoi(nodeValue);
					xmlFreeOTSERV(nodeValue);
				}
				else
					isLoaded = false;

				nodeValue = (char*)xmlGetProp(p, (const xmlChar *) "y");
				if(nodeValue) {
					spawnPos.y = atoi(nodeValue);
					xmlFreeOTSERV(nodeValue);
				}
				else
					isLoaded = false;

				nodeValue = (char*)xmlGetProp(p, (const xmlChar *) "z");
				if(nodeValue) {
					spawnPos.z = atoi(nodeValue);
					xmlFreeOTSERV(nodeValue);
				}
				else
					isLoaded = false;

				player->loginPosition = spawnPos;
			}
			else if(str=="temple"){
				nodeValue = (char*)xmlGetProp(p, (const xmlChar *) "x");
				if(nodeValue) {
					player->masterPos.x=atoi(nodeValue);
					xmlFreeOTSERV(nodeValue);
				}
				else
					isLoaded = false;
				
				nodeValue = (char*)xmlGetProp(p, (const xmlChar *) "y");
				if(nodeValue){
					player->masterPos.y=atoi(nodeValue);
					xmlFreeOTSERV(nodeValue);
				}
				else
					isLoaded = false;

				nodeValue = (char*)xmlGetProp(p, (const xmlChar *) "z");
				if(nodeValue){
					player->masterPos.z=atoi(nodeValue);
					xmlFreeOTSERV(nodeValue);
				}
				else
					isLoaded = false;
			}
			else if(str=="guild"){
				nodeValue = (char*)xmlGetProp(p, (const xmlChar *) "name");
				if(nodeValue) {
					player->guildName=nodeValue;
					xmlFreeOTSERV(nodeValue);
				}
				else
					isLoaded = false;
				
				nodeValue = (char*)xmlGetProp(p, (const xmlChar *) "rank");
				if(nodeValue) {
					player->guildRank=nodeValue;
					xmlFreeOTSERV(nodeValue);
				}
				else
					isLoaded = false;
				
				nodeValue = (char*)xmlGetProp(p, (const xmlChar *) "nick");
				if(nodeValue){
					player->guildNick=nodeValue;
					xmlFreeOTSERV(nodeValue);
				}
				else
					isLoaded = false;
				
				nodeValue = (char*)xmlGetProp(p, (const xmlChar *) "id");
				if(nodeValue){
					player->guildId=atoi(nodeValue);
					xmlFreeOTSERV(nodeValue);
				}
				else
					isLoaded = false;
			}
			else if(str=="skills"){
				tmp=p->children;
				while(tmp){
					int s_id = 0;
					int s_lvl = 0;
					int s_tries = 0;
					if (strcmp((const char*)tmp->name, "skill") == 0){
						nodeValue = (char*)xmlGetProp(tmp, (const xmlChar *) "skillid");
						if(nodeValue){
							s_id=atoi(nodeValue);
							xmlFreeOTSERV(nodeValue);
						}
						else
							isLoaded = false;

						nodeValue = (char*)xmlGetProp(tmp, (const xmlChar *) "level");
						if(nodeValue){
							s_lvl=atoi(nodeValue);
							xmlFreeOTSERV(nodeValue);
						}
						else
							isLoaded = false;

						nodeValue = (char*)xmlGetProp(tmp, (const xmlChar *) "tries");
						if(nodeValue){
							s_tries=atoi(nodeValue);
							xmlFreeOTSERV(nodeValue);
						}
						else
							isLoaded = false;

						player->skills[s_id][SKILL_LEVEL]=s_lvl;
						player->skills[s_id][SKILL_TRIES]=s_tries;
						player->skills[s_id][SKILL_PERCENT] = (unsigned int)(100*(player->skills[s_id][SKILL_TRIES])/(1.*player->getReqSkillTries (s_id, (player->skills[s_id][SKILL_LEVEL]+1), player->vocation)));
					}
					tmp=tmp->next;
				}
			}
			else if(str=="inventory"){
				slot=p->children;
				while(slot){
					if(strcmp((const char*)slot->name, "slot") == 0){
						int sl_id = 0;
						unsigned int id = 0;
						nodeValue = (char*)xmlGetProp(slot, (const xmlChar *)"slotid");
						if(nodeValue){
							sl_id = atoi(nodeValue);
							xmlFreeOTSERV(nodeValue);
						}
						else
							isLoaded = false;

						nodeValue = (char*)xmlGetProp(slot->children, (const xmlChar *) "id");
						if(nodeValue){
							id = atoi(nodeValue);
							xmlFreeOTSERV(nodeValue);
						}
						else
							isLoaded = false;

						Item* myitem = Item::CreateItem(id);
						myitem->unserialize(slot->children);
						
						player->__internalAddThing(sl_id, myitem);
						Container* container = dynamic_cast<Container*>(myitem);

						if(container){							
							LoadContainer(slot->children, container);
						}
					}
					slot=slot->next;
				}
			}
			else if(str=="depots"){
				slot=p->children;
				while (slot){
					if(strcmp((const char*)slot->name, "depot") == 0){
						int depotId = 0;
						unsigned int id = 0;
						
						nodeValue = (char*)xmlGetProp(slot, (const xmlChar *)"depotid");
						if(nodeValue){
							depotId = atoi(nodeValue);
							xmlFreeOTSERV(nodeValue);
						}
						else
							isLoaded = false;

						nodeValue = (char*)xmlGetProp(slot->children, (const xmlChar *) "id");
						if(nodeValue) {
							id = atoi(nodeValue);
							xmlFreeOTSERV(nodeValue);
						}
						else
							isLoaded = false;

						Depot* myDepot = new Depot(id);
						myDepot->useThing2();
						myDepot->unserialize(slot->children);

						player->addDepot(myDepot, depotId);
						LoadContainer(slot->children, myDepot);
					}

					slot=slot->next;
				}
			}
			else if(str == "storage"){
				slot = p->children;
				while(slot){
					if (strcmp((const char*)slot->name, "data") == 0){
						unsigned long key = 0;
						long value = 0;

						nodeValue = (char*)xmlGetProp(slot, (const xmlChar *)"key"); 
						if(nodeValue){
							key = atoi(nodeValue);
							xmlFreeOTSERV(nodeValue);
						}
						else
							isLoaded = false;

						nodeValue = (char*)xmlGetProp(slot, (const xmlChar *) "value");
						if(nodeValue){
							value = atoi(nodeValue);
							xmlFreeOTSERV(nodeValue);
						}
						else
							isLoaded = false;

						player->addStorageValue(key,value);
					}
					slot = slot->next;
				}
			}
			p=p->next;
		}

		player->updateInventoryWeigth();
		player->updateItemsLight(true);

		std::cout << "loaded " << filename << std::endl;
		xmlFreeDoc(doc);
		xmlMutexUnlock(xmlmutex);		
		return isLoaded;
	}

	xmlMutexUnlock(xmlmutex);	
	return false;
}

bool IOPlayerXML::LoadContainer(xmlNodePtr nodeitem,Container* ccontainer)
{
	xmlNodePtr tmp,p;
	if(nodeitem==NULL){
		return false;
	}
	tmp=nodeitem->children;
	if(tmp==NULL){
		return false;
	}
                  
	if(strcmp((const char*)tmp->name, "inside") == 0){
		char* nodeValue = NULL;
		//load items
		p=tmp->children;
		while(p){
			unsigned int id = 0;

			nodeValue = (char*)xmlGetProp(p, (const xmlChar *) "id");
			if(nodeValue) {
				id = atoi(nodeValue);
				xmlFreeOTSERV(nodeValue);
			}

			Item* myitem = Item::CreateItem(id);
			myitem->unserialize(p);			
			ccontainer->__internalAddThing(myitem);
			
			Container* in_container = dynamic_cast<Container*>(myitem);
			if(in_container){
				LoadContainer(p,in_container);
			}
			p=p->next;
		}

		return true;
	}

	return false;
}

bool IOPlayerXML::SaveContainer(xmlNodePtr nodeitem,Container* ccontainer)
{
	xmlNodePtr pn,nn;
	std::stringstream sb;
	if(ccontainer->size() != 0){
		pn = xmlNewNode(NULL,(const xmlChar*)"inside");
		for(int i=ccontainer->size()-1;i>=0;i--){
			Item * citem = ccontainer->getItem(i);
			nn = citem->serialize();
			Container* in_container = dynamic_cast<Container*>(citem);
			if(in_container){
				SaveContainer(nn,in_container);
			}
			xmlAddChild(pn, nn);
		}
		xmlAddChild(nodeitem, pn);
	}

	return true;
}

bool IOPlayerXML::savePlayer(Player* player)
{
	std::string datadir = g_config.getGlobalString("datadir");
	std::string filename = datadir + "players/" + player->getName() + ".xml";
	std::transform (filename.begin(),filename.end(), filename.begin(), tolower);
	std::stringstream sb;
    
	xmlDocPtr doc;
	xmlMutexLock(xmlmutex);    
	xmlNodePtr nn, sn, pn, root;
	doc = xmlNewDoc((const xmlChar*)"1.0");
	doc->children = xmlNewDocNode(doc, NULL, (const xmlChar*)"player", NULL);
	root = doc->children;

	player->preSave();

	sb << player->getName();  	       xmlSetProp(root, (const xmlChar*) "name", (const xmlChar*)sb.str().c_str());     sb.str("");
	sb << player->getGUID();           xmlSetProp(root, (const xmlChar*) "id", (const xmlChar*)sb.str().c_str());	sb.str("");
	sb << player->accountNumber;       xmlSetProp(root, (const xmlChar*) "account", (const xmlChar*)sb.str().c_str());	sb.str("");
	sb << player->sex;                 xmlSetProp(root, (const xmlChar*) "sex", (const xmlChar*)sb.str().c_str());     	sb.str("");	
	sb << player->getDirection();

	if (sb.str() == "North"){sb.str(""); sb << "0";}
	if (sb.str() == "East") {sb.str(""); sb << "1";}
	if (sb.str() == "South"){sb.str(""); sb << "2";}
	if (sb.str() == "West") {sb.str(""); sb << "3";}
	xmlSetProp(root, (const xmlChar*) "lookdir", (const xmlChar*)sb.str().c_str());            sb.str("");

	sb << player->experience;         xmlSetProp(root, (const xmlChar*) "exp", (const xmlChar*)sb.str().c_str());       sb.str("");	
	sb << (int)player->vocation;      xmlSetProp(root, (const xmlChar*) "voc", (const xmlChar*)sb.str().c_str());       sb.str("");
	sb << player->level;              xmlSetProp(root, (const xmlChar*) "level", (const xmlChar*)sb.str().c_str());     sb.str("");	
	sb << player->access;             xmlSetProp(root, (const xmlChar*) "access", (const xmlChar*)sb.str().c_str());	sb.str("");	
	sb << player->getCapacity();      xmlSetProp(root, (const xmlChar*) "cap", (const xmlChar*)sb.str().c_str());       sb.str("");
	sb << player->maglevel;	          xmlSetProp(root, (const xmlChar*) "maglevel", (const xmlChar*)sb.str().c_str());  sb.str("");
	sb << player->lastlogin;	        xmlSetProp(root, (const xmlChar*) "lastlogin", (const xmlChar*)sb.str().c_str());  sb.str("");

	pn = xmlNewNode(NULL,(const xmlChar*)"spawn");
	sb << player->getLoginPosition().x;    xmlSetProp(pn, (const xmlChar*) "x", (const xmlChar*)sb.str().c_str());        sb.str("");
	sb << player->getLoginPosition().y;  	xmlSetProp(pn, (const xmlChar*) "y", (const xmlChar*)sb.str().c_str());        sb.str("");
	sb << player->getLoginPosition().z; 	xmlSetProp(pn, (const xmlChar*) "z", (const xmlChar*)sb.str().c_str());	       sb.str("");
	xmlAddChild(root, pn);
	
	pn = xmlNewNode(NULL,(const xmlChar*)"temple");
	sb << player->masterPos.x;  xmlSetProp(pn, (const xmlChar*) "x", (const xmlChar*)sb.str().c_str());        sb.str("");
	sb << player->masterPos.y;  xmlSetProp(pn, (const xmlChar*) "y", (const xmlChar*)sb.str().c_str());        sb.str("");
	sb << player->masterPos.z; 	xmlSetProp(pn, (const xmlChar*) "z", (const xmlChar*)sb.str().c_str());	       sb.str("");
	xmlAddChild(root, pn);
	
	pn = xmlNewNode(NULL,(const xmlChar*)"health");
	sb << player->health;     xmlSetProp(pn, (const xmlChar*) "now", (const xmlChar*)sb.str().c_str());        sb.str("");
	sb << player->healthmax;  xmlSetProp(pn, (const xmlChar*) "max", (const xmlChar*)sb.str().c_str());        sb.str("");
	sb << player->food;  	  xmlSetProp(pn, (const xmlChar*) "food", (const xmlChar*)sb.str().c_str());       sb.str("");
	                     
	xmlAddChild(root, pn);
	
	pn = xmlNewNode(NULL,(const xmlChar*)"mana");
	sb << player->mana;      xmlSetProp(pn, (const xmlChar*) "now", (const xmlChar*)sb.str().c_str());        sb.str("");
	sb << player->manamax;   xmlSetProp(pn, (const xmlChar*) "max", (const xmlChar*)sb.str().c_str());        sb.str("");
	sb << player->manaspent; xmlSetProp(pn, (const xmlChar*) "spent", (const xmlChar*)sb.str().c_str());      sb.str("");
	xmlAddChild(root, pn);
    	               
	pn = xmlNewNode(NULL,(const xmlChar*)"look");
	sb << player->lookmaster;       xmlSetProp(pn, (const xmlChar*) "type", (const xmlChar*)sb.str().c_str());        sb.str("");
	sb << player->lookhead;         xmlSetProp(pn, (const xmlChar*) "head", (const xmlChar*)sb.str().c_str());        sb.str("");
	sb << player->lookbody;         xmlSetProp(pn, (const xmlChar*) "body", (const xmlChar*)sb.str().c_str());        sb.str("");
	sb << player->looklegs;         xmlSetProp(pn, (const xmlChar*) "legs", (const xmlChar*)sb.str().c_str());        sb.str("");
	sb << player->lookfeet;         xmlSetProp(pn, (const xmlChar*) "feet", (const xmlChar*)sb.str().c_str());        sb.str("");
	xmlAddChild(root, pn);
    
	pn = xmlNewNode(NULL,(const xmlChar*)"guild");
	sb << player->guildName;     xmlSetProp(pn, (const xmlChar*) "name", (const xmlChar*)sb.str().c_str());        sb.str("");
	sb << player->guildRank;  xmlSetProp(pn, (const xmlChar*) "rank", (const xmlChar*)sb.str().c_str());        sb.str("");
	sb << player->guildNick;  	  xmlSetProp(pn, (const xmlChar*) "nick", (const xmlChar*)sb.str().c_str());       sb.str("");
	sb << player->guildId;  	  xmlSetProp(pn, (const xmlChar*) "id", (const xmlChar*)sb.str().c_str());       sb.str("");
	
	xmlAddChild(root, pn);
    	      
	sn = xmlNewNode(NULL,(const xmlChar*)"skills");
	for(int i = 0; i <= 6; i++){
	  pn = xmlNewNode(NULL,(const xmlChar*)"skill");
	  sb << i;                                  xmlSetProp(pn, (const xmlChar*) "skillid", (const xmlChar*)sb.str().c_str());      sb.str("");
	  sb << player->skills[i][SKILL_LEVEL];     xmlSetProp(pn, (const xmlChar*) "level", (const xmlChar*)sb.str().c_str());        sb.str("");
	  sb << player->skills[i][SKILL_TRIES];     xmlSetProp(pn, (const xmlChar*) "tries", (const xmlChar*)sb.str().c_str());        sb.str("");
	  xmlAddChild(sn, pn);
	}
	
	xmlAddChild(root, sn);
	
	sn = xmlNewNode(NULL,(const xmlChar*)"inventory");
	for(int i = 1; i <= 10; i++){
		if(player->items[i]){
			pn = xmlNewNode(NULL,(const xmlChar*)"slot");
			sb << i;
			xmlSetProp(pn, (const xmlChar*) "slotid", (const xmlChar*)sb.str().c_str());
			sb.str("");
          
			nn = player->items[i]->serialize();
			Container* is_container = dynamic_cast<Container*>(player->items[i]);
			if(is_container){
				SaveContainer(nn,is_container);
			}
          
	    xmlAddChild(pn, nn);
	    xmlAddChild(sn, pn);
		}
	}
	
	xmlAddChild(root, sn);
	
	sn = xmlNewNode(NULL,(const xmlChar*)"depots");
	
	for(DepotMap::reverse_iterator it = player->depots.rbegin(); it !=player->depots.rend()  ;++it){
		pn = xmlNewNode(NULL,(const xmlChar*)"depot");
		sb << it->first;
		xmlSetProp(pn, (const xmlChar*) "depotid", (const xmlChar*)sb.str().c_str());
		sb.str("");
          
		nn = (it->second)->serialize();
		Container* is_container = dynamic_cast<Container*>(it->second);
		if(is_container){
			SaveContainer(nn,is_container);
		}
          
		xmlAddChild(pn, nn);
		xmlAddChild(sn, pn);
	}
      
	xmlAddChild(root, sn);
   
	sn = xmlNewNode(NULL,(const xmlChar*)"storage");
	for(StorageMap::const_iterator cit = player->getStorageIteratorBegin(); cit != player->getStorageIteratorEnd();cit++){
		pn = xmlNewNode(NULL,(const xmlChar*)"data");
		sb << cit->first;
		xmlSetProp(pn, (const xmlChar*) "key", (const xmlChar*)sb.str().c_str());
		sb.str("");
          
		sb << cit->second;
		xmlSetProp(pn, (const xmlChar*) "value", (const xmlChar*)sb.str().c_str());
		sb.str("");
        
		xmlAddChild(sn, pn);
	}
	
	xmlAddChild(root, sn);
	
	//Save the character
	if(xmlSaveFile(filename.c_str(), doc)){
		#ifdef __DEBUG__
		std::cout << "\tSaved character succefully!\n";
		#endif

		xmlFreeDoc(doc);
		xmlMutexUnlock(xmlmutex);
		return true;
	}
	else{
		std::cout << "\tCouldn't save character =(\n";
		xmlFreeDoc(doc);       
		xmlMutexUnlock(xmlmutex);       
		return false;
	}
}


bool IOPlayerXML::getGuidByName(unsigned long &guid, unsigned long &alvl, std::string &name)
{
	std::string datadir = g_config.getGlobalString("datadir");
	std::string filename = datadir + "players/" + name + ".xml";
	std::transform (filename.begin(),filename.end(), filename.begin(), tolower);
	xmlDocPtr doc;	
	xmlMutexLock(xmlmutex);
	doc = xmlParseFile(filename.c_str());

	bool isSuccess = false;
	if(doc){
		char* nodeValue = NULL;
		xmlNodePtr root = xmlDocGetRootElement(doc);

		if(xmlStrcmp(root->name,(const xmlChar*) "player")){
			isSuccess = false;
		}
		else{
			nodeValue = (char*)xmlGetProp(root, (const xmlChar *) "name");
			if(nodeValue){
				name = nodeValue;
				xmlFreeOTSERV(nodeValue);
			}

			nodeValue = (char*)xmlGetProp(root, (const xmlChar *) "access");
			if(nodeValue){
				alvl = atoi(nodeValue);
				xmlFreeOTSERV(nodeValue);
			}

			nodeValue = (char*)xmlGetProp(root, (const xmlChar *) "id");
			if(nodeValue){
				guid = atoi(nodeValue);
				xmlFreeOTSERV(nodeValue);

				isSuccess = true;
			}
		}
	}

	xmlMutexUnlock(xmlmutex);	
	return isSuccess;
}

bool IOPlayerXML::playerExists(std::string name)
{
	std::string datadir = g_config.getGlobalString("datadir");
	std::string filename = datadir + "players/" + name + ".xml";
	std::transform(filename.begin(), filename.end(), filename.begin(), tolower);

	return boost::filesystem::exists(filename);
}

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

#include "ioplayer.h"
#include "ioplayerxml.h"
#include "ioaccount.h"
#include "item.h"
#include "luascript.h"

xmlMutexPtr xmlmutex;

extern LuaScript g_config;

IOPlayerXML::IOPlayerXML(){
	if(xmlmutex == NULL){
		xmlmutex = xmlNewMutex();
	}
}

bool IOPlayerXML::loadPlayer(Player* player, std::string name){
	std::string datadir = g_config.getGlobalString("datadir");
	std::string filename = datadir + "players/" + name + ".xml";
	std::transform (filename.begin(),filename.end(), filename.begin(), tolower);

	xmlDocPtr doc;	
	xmlMutexLock(xmlmutex);
	doc = xmlParseFile(filename.c_str());

	if (doc)
	{
		xmlNodePtr root, tmp, p, slot;
		root=xmlDocGetRootElement(doc);

		if (xmlStrcmp(root->name,(const xmlChar*) "player"))
		{
			std::cout << "Strange. Player-Savefile was no savefile for " << name << std::endl;
		}

		p = root->children;

		const char *account = (const char*)xmlGetProp(root, (const xmlChar *) "account");
		
		//need to unlock and relock in order to load xml account
		//xmlMutexUnlock(xmlmutex); 
		Account a = IOAccount::instance()->loadAccount(atoi(account));
		//xmlMutexLock(xmlmutex);
		
		player->password = a.password;
		if (a.accnumber == 0 || a.accnumber != (unsigned long)atoi(account)) {
		  xmlFreeDoc(doc);		  
		  xmlMutexUnlock(xmlmutex);		  
		  return false;
		}

		player->accountNumber = atoi((const char*)xmlGetProp(root, (const xmlChar *) "account"));
		player->sex=(playersex_t)atoi((const char*)xmlGetProp(root, (const xmlChar *) "sex"));
		player->setDirection((Direction)atoi((const char*)xmlGetProp(root, (const xmlChar *) "lookdir")));
		player->experience=atoi((const char*)xmlGetProp(root, (const xmlChar *) "exp"));
		player->level=atoi((const char*)xmlGetProp(root, (const xmlChar *) "level"));

		player->maglevel=atoi((const char*)xmlGetProp(root, (const xmlChar *) "maglevel"));
		player->vocation = (playervoc_t)atoi((const char*)xmlGetProp(root, (const xmlChar *) "voc"));
		player->access=atoi((const char*)xmlGetProp(root, (const xmlChar *) "access"));
		player->capacity = atoi((const char*)xmlGetProp(root, (const xmlChar *) "cap"));
		player->max_depot_items = atoi((const char*)xmlGetProp(root, (const xmlChar *) "maxdepotitems"));
		player->setNormalSpeed();
		
		if(xmlGetProp(root, (const xmlChar *) "lastlogin")){
			player->lastlogin = atoi((const char*)xmlGetProp(root, (const xmlChar *) "lastlogin"));
		}
		else{
			player->lastlogin = 0;
		}
		
		//level percent
		player->level_percent  = (unsigned char)(100*(player->experience-player->getExpForLv(player->level))/(1.*player->getExpForLv(player->level+1)-player->getExpForLv(player->level)));
		while (p)
		{
			std::string str=(char*)p->name;
			if(str=="mana")
			{
				player->mana=atoi((const char*)xmlGetProp(p, (const xmlChar *) "now"));
				player->manamax=atoi((const char*)xmlGetProp(p, (const xmlChar *) "max"));
				player->manaspent=atoi((const char*)xmlGetProp(p, (const xmlChar *) "spent"));
				player->maglevel_percent  = (unsigned char)(100*(player->manaspent/(1.*player->getReqMana(player->maglevel+1, player->vocation))));
			}
			else if(str=="health")
			{
				player->health=atoi((const char*)xmlGetProp(p, (const xmlChar *) "now"));
				player->healthmax=atoi((const char*)xmlGetProp(p, (const xmlChar *) "max"));
				player->food=atoi((const char*)xmlGetProp(p, (const xmlChar *) "food"));
			}
			else if(str=="look")
			{
				player->looktype=atoi((const char*)xmlGetProp(p, (const xmlChar *) "type"));
				player->lookmaster = player->looktype;
				player->lookhead=atoi((const char*)xmlGetProp(p, (const xmlChar *) "head"));
				player->lookbody=atoi((const char*)xmlGetProp(p, (const xmlChar *) "body"));
				player->looklegs=atoi((const char*)xmlGetProp(p, (const xmlChar *) "legs"));
				player->lookfeet=atoi((const char*)xmlGetProp(p, (const xmlChar *) "feet"));
			}
			else if(str=="spawn")
			{
				player->pos.x=atoi((const char*)xmlGetProp(p, (const xmlChar *) "x"));
				player->pos.y=atoi((const char*)xmlGetProp(p, (const xmlChar *) "y"));
				player->pos.z=atoi((const char*)xmlGetProp(p, (const xmlChar *) "z"));
			}
			else if(str=="temple")
			{
				player->masterPos.x=atoi((const char*)xmlGetProp(p, (const xmlChar *) "x"));
				player->masterPos.y=atoi((const char*)xmlGetProp(p, (const xmlChar *) "y"));
				player->masterPos.z=atoi((const char*)xmlGetProp(p, (const xmlChar *) "z"));
			}
			else if(str=="skills")
			{
				tmp=p->children;
				while(tmp)
				{
					int s_id, s_lvl, s_tries;
					if (strcmp((const char*)tmp->name, "skill") == 0)
					{
						s_id=atoi((const char*)xmlGetProp(tmp, (const xmlChar *) "skillid"));
						s_lvl=atoi((const char*)xmlGetProp(tmp, (const xmlChar *) "level"));
						s_tries=atoi((const char*)xmlGetProp(tmp, (const xmlChar *) "tries"));
						player->skills[s_id][SKILL_LEVEL]=s_lvl;
						player->skills[s_id][SKILL_TRIES]=s_tries;
						player->skills[s_id][SKILL_PERCENT] = (unsigned int)(100*(player->skills[s_id][SKILL_TRIES])/(1.*player->getReqSkillTries (s_id, (player->skills[s_id][SKILL_LEVEL]+1), player->vocation)));
					}
					tmp=tmp->next;
				}
			}
			else if(str=="inventory")
			{
				slot=p->children;
				while (slot)
				{
					if (strcmp((const char*)slot->name, "slot") == 0)
					{
						int sl_id = atoi((const char*)xmlGetProp(slot, (const xmlChar *)"slotid"));
						unsigned int id = atoi((const char*)xmlGetProp(slot->children, (const xmlChar *) "id"));
						Item* myitem = Item::CreateItem(id);
						myitem->unserialize(slot->children);
						//we dont want to sendinventory before login
						player->addItemInventory(myitem, sl_id, true);
						Container* default_container = dynamic_cast<Container*>(myitem);
						if(default_container){							
							LoadContainer(slot->children,default_container);
						}
					}
				slot=slot->next;
				}
			}
			else if(str=="depots")
			{
				slot=p->children;
				while (slot)
				{
					if (strcmp((const char*)slot->name, "depot") == 0)
					{
						int dp_id = atoi((const char*)xmlGetProp(slot, (const xmlChar *)"depotid"));
						unsigned int id = atoi((const char*)xmlGetProp(slot->children, (const xmlChar *) "id"));
						Item* myitem = Item::CreateItem(id);
						myitem->unserialize(slot->children);
						Container* default_container = dynamic_cast<Container*>(myitem);
						if(default_container){							
							player->addDepot(default_container , dp_id);
							LoadContainer(slot->children,default_container);
						}
						else{
							delete myitem;
						}							
					}
				slot=slot->next;
				}
			}
			else if(str == "storage"){
				slot = p->children;
				while(slot){
					if (strcmp((const char*)slot->name, "data") == 0)
					{
						unsigned long key = atoi((const char*)xmlGetProp(slot, (const xmlChar *)"key"));
						long value = atoi((const char*)xmlGetProp(slot, (const xmlChar *) "value"));
						player->addStorageValue(key,value);
					}
					slot = slot->next;
				}
			}
			p=p->next;
		}

		player->updateInventoryWeigth();

		std::cout << "loaded " << filename << std::endl;
		xmlFreeDoc(doc);		
		xmlMutexUnlock(xmlmutex);		
		return true;
	}	
	xmlMutexUnlock(xmlmutex);	
	return false;
}

bool IOPlayerXML::LoadContainer(xmlNodePtr nodeitem,Container* ccontainer)
{
	xmlNodePtr tmp,p;
	/*unsigned short s_id;
	unsigned char s_count;
	Item *new_item;*/
	if(nodeitem==NULL){
		return false;
	}
	tmp=nodeitem->children;
	if(tmp==NULL){
		return false;
	}
                  
	if (strcmp((const char*)tmp->name, "inside") == 0){
		//load items
		p=tmp->children;
		while(p){			
			unsigned int id = atoi((const char*)xmlGetProp(p, (const xmlChar *) "id"));
			Item* myitem = Item::CreateItem(id);
			myitem->unserialize(p);			
			ccontainer->addItem(myitem);
			
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


bool IOPlayerXML::savePlayer(Player* player){
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
	sb << player->accountNumber;       xmlSetProp(root, (const xmlChar*) "account", (const xmlChar*)sb.str().c_str());	sb.str("");
	sb << player->sex;                 xmlSetProp(root, (const xmlChar*) "sex", (const xmlChar*)sb.str().c_str());     	sb.str("");	
	sb << player->getDirection();
    if (sb.str() == "North"){sb.str(""); sb << "0";}
	if (sb.str() == "East") {sb.str(""); sb << "1";}
	if (sb.str() == "South"){sb.str(""); sb << "2";}
	if (sb.str() == "West") {sb.str(""); sb << "3";}
	xmlSetProp(root, (const xmlChar*) "lookdir", (const xmlChar*)sb.str().c_str());                             sb.str("");
	sb << player->experience;         xmlSetProp(root, (const xmlChar*) "exp", (const xmlChar*)sb.str().c_str());       sb.str("");	
	sb << (int)player->vocation;      xmlSetProp(root, (const xmlChar*) "voc", (const xmlChar*)sb.str().c_str());       sb.str("");
	sb << player->level;              xmlSetProp(root, (const xmlChar*) "level", (const xmlChar*)sb.str().c_str());     sb.str("");	
	sb << player->access;             xmlSetProp(root, (const xmlChar*) "access", (const xmlChar*)sb.str().c_str());	sb.str("");	
	//sb << player->cap;    	        xmlSetProp(root, (const xmlChar*) "cap", (const xmlChar*)sb.str().c_str());       sb.str("");
	sb << player->getCapacity();      xmlSetProp(root, (const xmlChar*) "cap", (const xmlChar*)sb.str().c_str());       sb.str("");
	sb << player->maglevel;	          xmlSetProp(root, (const xmlChar*) "maglevel", (const xmlChar*)sb.str().c_str());  sb.str("");
	sb << player->lastlogin;	        xmlSetProp(root, (const xmlChar*) "lastlogin", (const xmlChar*)sb.str().c_str());  sb.str("");

	pn = xmlNewNode(NULL,(const xmlChar*)"spawn");
	sb << player->pos.x;    xmlSetProp(pn, (const xmlChar*) "x", (const xmlChar*)sb.str().c_str());        sb.str("");
	sb << player->pos.y;  	xmlSetProp(pn, (const xmlChar*) "y", (const xmlChar*)sb.str().c_str());        sb.str("");
	sb << player->pos.z; 	xmlSetProp(pn, (const xmlChar*) "z", (const xmlChar*)sb.str().c_str());	       sb.str("");
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
    
    	      
	sn = xmlNewNode(NULL,(const xmlChar*)"skills");
	for (int i = 0; i <= 6; i++)
	  {
	  pn = xmlNewNode(NULL,(const xmlChar*)"skill");
	  sb << i;                          xmlSetProp(pn, (const xmlChar*) "skillid", (const xmlChar*)sb.str().c_str());      sb.str("");
	  sb << player->skills[i][SKILL_LEVEL];     xmlSetProp(pn, (const xmlChar*) "level", (const xmlChar*)sb.str().c_str());        sb.str("");
	  sb << player->skills[i][SKILL_TRIES];     xmlSetProp(pn, (const xmlChar*) "tries", (const xmlChar*)sb.str().c_str());        sb.str("");
	  xmlAddChild(sn, pn);
      }
   xmlAddChild(root, sn);
	
	sn = xmlNewNode(NULL,(const xmlChar*)"inventory");
	for (int i = 1; i <= 10; i++)
	  {
   	  if (player->items[i])
          {
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
    if (xmlSaveFile(filename.c_str(), doc))
       {
       #ifdef __DEBUG__
       std::cout << "\tSaved character succefully!\n";
       #endif
       xmlFreeDoc(doc);       
       xmlMutexUnlock(xmlmutex);       
	   return true;
       }
    else
       {
       std::cout << "\tCouldn't save character =(\n";
       xmlFreeDoc(doc);       
       xmlMutexUnlock(xmlmutex);       
	   return false;
       }
}

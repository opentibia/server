//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Player Loader/Saver based on MySQL
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


#ifdef __BUILD_WITH_SQL
#include "ioplayer.h"
#include "ioplayersql.h"
#include "ioaccount.h"
#include "item.h"
#include "luascript.h"

#include <iostream>
#include <iomanip>
#include <mysql++.h>


extern LuaScript g_config;

bool IOPlayerSQL::loadPlayer(Player* player, std::string name){
	std::string host = g_config.getGlobalString("sql_host");
	std::string user = g_config.getGlobalString("sql_user");
	std::string pass = g_config.getGlobalString("sql_pass");
	std::string db   = g_config.getGlobalString("sql_db");
	mysqlpp::Connection con;
	//mysqlpp::Connection con(db.c_str(), host.c_str(), user.c_str(), pass.c_str()); 

	/*
std::string filename="data/players/"+name+".xml";
	std::transform (filename.begin(),filename.end(), filename.begin(), tolower);

	xmlDocPtr doc;
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
		Account a = IOAccount::instance()->loadAccount(atoi(account));
		
		player->password = a.password;
		if (a.accnumber == 0 || a.accnumber != atoi(account)) {
		  xmlFreeDoc(doc);
		  return false;
		}

		player->accountNumber = atoi((const char*)xmlGetProp(root, (const xmlChar *) "account"));
		player->sex=atoi((const char*)xmlGetProp(root, (const xmlChar *) "sex"));
		player->setDirection((Direction)atoi((const char*)xmlGetProp(root, (const xmlChar *) "lookdir")));
		player->experience=atoi((const char*)xmlGetProp(root, (const xmlChar *) "exp"));
		player->level=atoi((const char*)xmlGetProp(root, (const xmlChar *) "level"));
		player->maglevel=atoi((const char*)xmlGetProp(root, (const xmlChar *) "maglevel"));
		player->voc=atoi((const char*)xmlGetProp(root, (const xmlChar *) "voc"));
		player->access=atoi((const char*)xmlGetProp(root, (const xmlChar *) "access"));
		player->setNormalSpeed();
		while (p)
		{
			std::string str=(char*)p->name;
			if(str=="mana")
			{
				player->mana=atoi((const char*)xmlGetProp(p, (const xmlChar *) "now"));
				player->manamax=atoi((const char*)xmlGetProp(p, (const xmlChar *) "max"));
				player->manaspent=atoi((const char*)xmlGetProp(p, (const xmlChar *) "spent"));
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
						Item* myitem = new Item();
						myitem->unserialize(slot->children);
						player->items[sl_id]=myitem;

						//Should be loaded from xml later on...
						if(player->items[sl_id] && player->items[sl_id]->isContainer()) {

							Item *backpack = new Item(1411);
							player->items[sl_id]->addItem(backpack);
							backpack->addItem(new Item(1663, 99));
							backpack->addItem(new Item(1663, 99));
							backpack->addItem(new Item(1663, 99));
							backpack->addItem(new Item(1663, 99));

							backpack = new Item(1411);
							player->items[sl_id]->addItem(backpack);
							
							backpack->addItem(new Item(1623, 99));
							backpack->addItem(new Item(1623, 99));
							backpack->addItem(new Item(1623, 99));
							backpack->addItem(new Item(1623, 99));

							backpack = new Item(1411);
							player->items[sl_id]->addItem(backpack);
							
							backpack->addItem(new Item(1618, 99));
							backpack->addItem(new Item(1618, 99));
							backpack->addItem(new Item(1618, 99));
							backpack->addItem(new Item(1618, 99));

							player->items[sl_id]->addItem(new Item(1655, 5));
							player->items[sl_id]->addItem(new Item(1643, 5));
							player->items[sl_id]->addItem(new Item(1654, 5));
						}
					}
				slot=slot->next;
				}
			}
			p=p->next;
		}
		std::cout << "loaded " << filename << std::endl;
		xmlFreeDoc(doc);

		return true;
	}*/
	return false;
	
}
#endif
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



#include "ioplayer.h"
#include "ioplayerxml.h"
#include "ioaccount.h"
#include "item.h"


bool IOPlayerXML::loadPlayer(Player* player, std::string name){

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
		if (a.accnumber == 0 || a.accnumber != (unsigned long)atoi(account)) {
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
						player->items[sl_id]= Item::CreateItem(myitem->getID(), myitem->getItemCountOrSubtype());
						delete myitem;
						myitem = NULL;

						//Should be loaded from xml later on...
						Container* defaultbackpack = dynamic_cast<Container*>(player->items[sl_id]);
						if(defaultbackpack) {

							Container *backpack = dynamic_cast<Container*>(Item::CreateItem(1988));
							if(!backpack)
								continue;

							defaultbackpack->addItem(backpack);
							for(int i = 0; i < 20; ++i) {
								backpack->addItem(Item::CreateItem(2313, 99));
							}

							backpack = dynamic_cast<Container*>(Item::CreateItem(1988));
							if(!backpack)
								continue;
							defaultbackpack->addItem(backpack);
							
							for(int i = 0; i < 20; ++i) {
								backpack->addItem(Item::CreateItem(2273, 99));
							}

							backpack = dynamic_cast<Container*>(Item::CreateItem(1988));
							if(!backpack)
								continue;
							defaultbackpack->addItem(backpack);
							
							for(int i = 0; i < 20; ++i) {
								backpack->addItem(Item::CreateItem(2268, 99));
							}

							backpack = dynamic_cast<Container*>(Item::CreateItem(1988));
							if(!backpack)
								continue;
							defaultbackpack->addItem(backpack);

							for(int i = 0; i < 20; ++i) {
								backpack->addItem(Item::CreateItem(2304, 3));
							}

							backpack = dynamic_cast<Container*>(Item::CreateItem(1988));
							if(!backpack)
								continue;
							defaultbackpack->addItem(backpack);

							for(int i = 0; i < 20; ++i) {
								backpack->addItem(Item::CreateItem(2293, 3));
							}

							defaultbackpack->addItem(Item::CreateItem(2304, 2));
							defaultbackpack->addItem(Item::CreateItem(2308, 50));
							defaultbackpack->addItem(Item::CreateItem(2262, 5));
							defaultbackpack->addItem(Item::CreateItem(2305, 5));
							defaultbackpack->addItem(Item::CreateItem(2311, 99));
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
	}
	return false;
}

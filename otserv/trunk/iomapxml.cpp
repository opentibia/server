//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// XML implementation of the Map Loader/Saver
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

#include "iomapxml.h"

#include <iostream>

bool IOMapXML::loadMap(Map* map, std::string identifier){
	xmlDocPtr doc;
	xmlNodePtr root, tile, p;
	int width, height;

	xmlLineNumbersDefault(1);
	doc=xmlParseFile(identifier.c_str());
	if (!doc) {
		std::cout << "FATAL: couldnt load map. exiting" << std::endl;
		exit(1);
	}
	root=xmlDocGetRootElement(doc);
	if(xmlStrcmp(root->name,(const xmlChar*) "map")){
		xmlFreeDoc(doc);
		std::cout << "FATAL: couldnt load map. exiting" << std::endl;
		exit(1);
	}

	width=atoi((const char*)xmlGetProp(root, (const xmlChar *) "width"));
	height=atoi((const char*)xmlGetProp(root, (const xmlChar *) "height"));

    int xorig=((MAP_WIDTH)-width)/2;
    int yorig=((MAP_HEIGHT)-height)/2;
	tile=root->children;
	int numpz = 0;
	for(int y=0; y < height; y++){
		for(int x=0; x < width; x++){
			if (!tile) {
			std::cout << "no tile for " << x << " / " << y << std::endl;
		    exit(1);
	    }
		const char* pz = (const char*)xmlGetProp(tile, (const xmlChar *) "pz");
		p=tile->children;
	
		while(p)
		{
			if(xmlStrcmp(p->name,(const xmlChar*) "item")==0){
				Item* tmpitem=new Item();
				tmpitem->unserialize(p);
				
				if (tmpitem->isGroundTile())
				{
					map->setTile(xorig+x, yorig+y, 7, tmpitem->getID());
					delete tmpitem;
					
					if (pz && (strcmp(pz, "1") == 0)) {
						numpz++;
						map->getTile(x, y, 7)->setPz();
					}
				}
				else
				{
					Tile *t = map->getTile(xorig+x, yorig+y, 7);
					if (t)
					{
						Item* myitem = Item::CreateItem(tmpitem->getID());
						delete tmpitem;

						if (myitem->isAlwaysOnTop())
							t->topItems.push_back(myitem);
						else
							t->downItems.push_back(myitem);
						}
					}

				}
				if(xmlStrcmp(p->name,(const xmlChar*) "npc")==0){
/*					std::string name = (const char*)xmlGetProp(p, (const xmlChar *) "name");
				Npc* mynpc = new Npc(name.c_str(), this);
					//first we have to set the position of our creature...
					mynpc->pos.x=x;
					mynpc->pos.y=y;
					if(!this->placeCreature(mynpc)){
						//tinky winky: "oh oh"
					}*/
				}
				p=p->next;
			}
			tile=tile->next;
		}
	}
	
	xmlFreeDoc(doc);
	
	return true;
}

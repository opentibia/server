//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// the map of OpenTibia
//////////////////////////////////////////////////////////////////////
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundumpion; either version 2
// of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundumpion,
// Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//////////////////////////////////////////////////////////////////////
// $Id$
//////////////////////////////////////////////////////////////////////
// $Log$
// Revision 1.22  2003/11/06 17:16:47  tliffrag
// 0.2.7 release
//
// Revision 1.21  2003/11/05 23:28:24  tliffrag
// Addex XML for players, outfits working
//
// Revision 1.20  2003/11/03 22:48:14  tliffrag
// Changing look, walking by mouse working
//
// Revision 1.19  2003/11/03 12:16:01  tliffrag
// started walking by mouse
//
// Revision 1.18  2003/11/01 15:58:52  tliffrag
// Added XML for players and map
//
// Revision 1.17  2003/11/01 15:52:43  tliffrag
// Improved eventscheduler
//
// Revision 1.16  2003/10/21 17:55:07  tliffrag
// Added items on player
//
// Revision 1.15  2003/10/19 21:32:19  tliffrag
// Reworked the Tile class; stackable items now working
//
// Revision 1.14  2003/10/17 22:25:02  tliffrag
// Addes SorryNotPossible; added configfile; basic lua support
//
// Revision 1.13  2003/09/25 21:17:52  timmit
// Adding PlayerList in TMap and getID().  Not workigng!
//
// Revision 1.12  2003/09/23 20:00:51  tliffrag
// added !g command
//
// Revision 1.11  2003/09/23 16:41:19  tliffrag
// Fixed several map bugs
//
// Revision 1.10  2003/09/18 12:35:22  tliffrag
// added item dragNdrop
//
// Revision 1.9  2003/09/17 16:35:08  tliffrag
// added !d command and fixed lag on windows
//
// Revision 1.8  2003/09/08 13:28:41  tliffrag
// Item summoning and maploading/saving now working.
//
// Revision 1.7  2003/08/26 21:09:53  tliffrag
// fixed maphandling
//
// Revision 1.6  2003/08/25 21:28:12  tliffrag
// Fixed all warnings.
//
// Revision 1.5  2003/05/19 16:48:37  tliffrag
// Loggingin, talking, walking around, logging out working
//
// Revision 1.4  2002/05/28 13:55:57  shivoc
// some minor changes
//
// Revision 1.3  2002/04/08 15:57:03  shivoc
// made some changes to be more ansi compliant
//
// Revision 1.2  2002/04/08 14:12:49  shivoc
// fixed a segfault because of out of bounds access to tiles array
//
// Revision 1.1  2002/04/08 13:53:59  acrimon
// Added some very basic map support
//
//////////////////////////////////////////////////////////////////////

#include <iostream>
#include "items.h"
#include "tmap.h"
#include "npc.h"
#include <stdio.h>
#include <string>
#include <sstream>

#include "eventscheduler.h"
extern EventScheduler es;

//TODO move all the tile stuff to tile.cpp
int Tile::getStackPosItem(){
	int pos=1;
	if(size()<=1)
		return 0;
	if(creature && size()>=2)
		pos++;
	Tile::iterator it=end();
	it--;
	if(size()>=3 && (*it)->isAlwaysOnTop())
		pos++;
	std::cout << "stackpos of top item is "<< pos << std::endl;
	return pos;
}

void Tile::push_front(Item* i){
	this->insert(this->begin(), i);
}

Item* Tile::pop_front(){
	Item* a= this->operator[](0);
	erase(this->begin());
	return a;
}

bool Tile::isBlocking(){
	bool blocking=false;
	Tile::iterator i;
	//if any of the items on this tile is blocking,
	//the whole tile blocks
	for(i = this->begin(); i != this->end(); i++){
		if((*i)->isBlocking())
			blocking=true;
	}
	if(creature) //creatures also block a tile
		blocking=true;
	return blocking;
}

int Tile::removeItem(int stack, int type, int count){
	//returns how much items are left
	//TODO
	Item* item=getItemByStack(stack);
	int itemsleft = item->count-count;
	if(itemsleft<=0){
		itemsleft=0;
		removeItemByStack(stack);
	}
	item->count=itemsleft;
	return itemsleft;
}

int Tile::addItem(Item* item){
	if(size()==0){
		push_back(item);
		return true;
	}
	if(item->isGroundTile()){
		if(size()==0)
			push_back(item);
		else
			(*this)[0]=item;
	return true;
	}
	else if(item->isAlwaysOnTop()){
		Tile::iterator it=end();
		it--;
		if((*it)->isAlwaysOnTop())
			(*it)=item;
		else
			push_back(item);
		return true;
	}
/*	else if(!item->isAlwaysOnBottom()){//normal item
		Tile::iterator it=begin();
		while(it != end() && !(*it)->isAlwaysOnTop())
			it++;
		insert(it,item);
	}*/
	else{
		Tile::iterator it=begin();
		while(it != end() && (*it)->isGroundTile() )
			it++;
		insert(it,item);
		return true;
	}
	return true;
}

std::vector<Item*>::iterator Tile::getItToPos(int pos){
	Tile::iterator it;
	int i=0;
	for(it=begin(); i <pos; ++it)i++;
	return it;
}

Item* Tile::getItemByStack(int stack){
	if(stack==0)
		return(*getItToPos(0));
	//if theres a creature on this tile...
	if((*(getItToPos(size()-1)))->isAlwaysOnTop()){
		if(stack==1)
			return *(getItToPos(size()-1));
		else{
			if(this->creature)
				if(stack==2)
					return NULL;
				else
					return *(getItToPos(size()-(stack-1)));
			else
				return *(getItToPos(size()-stack));
		}
	}
	else{
		if(this->creature)
			if(stack==1){
				return NULL;
			}
			else{
				return *(getItToPos(size()-(stack-1)));
			}
		else{
			return *(getItToPos(size()-stack));
		}
	}
	return NULL;
}

int Tile::removeItemByStack(int stack){
	if(stack==0)
		erase(getItToPos(0));
	//if theres a creature on this tile...
	else if((*(getItToPos(size()-1)))->isAlwaysOnTop()){
		if(stack==1)
			erase ((getItToPos(size()-1)));
		else{
			if(this->creature)
				if(stack==2)
					return 0;
				else
					erase((getItToPos(size()-(stack-1))));
			else
				erase( (getItToPos(size()-stack)));
		}
	}
	else{
		if(this->creature)
			if(stack==1){
				return 0;
			}
			else{
				erase((getItToPos(size()-(stack-1))));
			}
		else{
			erase( (getItToPos(size()-stack)));
		}
	}
	return 0;
}

int Tile::getStackPosPlayer(){
	bool top=false;
	Tile::iterator i;
	//if any of the items on this tile is blocking,
	//the whole tile blocks
	for(i = this->begin(); i != this->end(); i++){
		if((*i)->isAlwaysOnTop())
			top=true;
	}
	if(top)
		return 2;
	else
		return 1;
}

std::string Tile::getDescription(){
	std::string ret;
	std::cout << "Items: "<< size() << std::endl;
	for(unsigned int i=0; i < size(); i++)
		std::cout << "ID: "<< (*this)[i]->getID() << std::endl;
	Tile::iterator it;
	it=end();
	it--;
	ret=(*it)->getDescription();
	//ret="You dont know why, but you cant see anything!";
	return ret;
}


int Map::tick(double time){
	es.addMapTick(1000);
	return true;
}
Map::Map() {
	//first we fill the map with
	for(int y=MINY; y< MAXY; y++){
		for(int x=MINX; x < MAXX; x++){
		tiles[x-MINX][y-MINY] = new Tile;
		if(abs(x-MINX)<10 || abs(x-MAXX)<10 ||
		   abs(y-MINY)<10 || abs(y-MAXY)<10)
			tiles[x-MINX][y-MINY]->addItem(new Item(605));
		else
			tiles[x-MINX][y-MINY]->addItem(new Item(102));
		}
	}

	FILE* f;
	f=fopen("data/world/map.xml","r");
	if(f){
	fclose(f);
		loadMapXml("data/world/map.xml");
	}
	else{
		//this code is ugly but works
		//TODO improve this code to support things like
		//a quadtree to speed up everything
		#ifdef __DEBUG__
		std::cout << "Loading map" << std::endl;
		#endif
		FILE* dump=fopen("otserv.map", "rb");
		if(!dump){
			#ifdef __DEBUG__
			std::cout << "Loading old format mapfile" << std::endl;
			#endif
			exit(1);
		}
		position topleft, bottomright, now;


		topleft.x=fgetc(dump)*256;	topleft.x+=fgetc(dump);
		topleft.y=fgetc(dump)*256;	topleft.y+=fgetc(dump);
		topleft.z=fgetc(dump);

		bottomright.x=fgetc(dump)*256;	bottomright.x+=fgetc(dump);
		bottomright.y=fgetc(dump)*256;	bottomright.y+=fgetc(dump);
		bottomright.z=fgetc(dump);

		int xsize= bottomright.x-topleft.x;
		int ysize= bottomright.y-topleft.y;
		int xorig=((MAXX-MINX)-xsize)/2;
		int yorig=((MAXY-MINY)-ysize)/2;
		//TODO really place this map patch where it belongs

		for(int y=0; y < ysize; y++){
			for(int x=0; x < xsize; x++){
				while(true){
					int id=fgetc(dump)*256;id+=fgetc(dump);
					if(id==0x00FF)
						break;
					//now.x=x+MINX;now.y=y+MINY;now.z=topleft.z;
					tiles[xorig][yorig]->addItem(new Item(id));
					//tiles[x][y]->push_back(new Item(id));
				}
			}
		}
		fclose(dump);
	}
	tiles[32864-MINX][32863-MINY]->creature=new NPC("ruediger");
}

Map::Map(char *filename) {
    // load the map from a file
}

Tile *Map::tile(unsigned short _x, unsigned short _y, unsigned char _z) {
    return tiles[_x - MINX][_y - MINY];
}

Map::~Map() {
	saveMapXml();
}

int Map::saveMapXml(){
	//save the map in the new format
	std::stringstream s;
	xmlDocPtr doc;
	xmlNodePtr p, root;
	std::cout << "Saving the map in XML format" << std::endl;
	doc = xmlNewDoc((const xmlChar*)"1.0");
	doc->children = xmlNewDocNode(doc, NULL, (const xmlChar*)"map", NULL);
	root=doc->children;
	s <<MAXY-MINY;
	xmlSetProp(doc->children, (const xmlChar*)"height", (const xmlChar*)s.str().c_str());
	s.str(""); //empty the stringstream
	s <<MAXX-MINX;
    xmlSetProp(doc->children, (const xmlChar*)"width", (const xmlChar*)s.str().c_str());
	for(int y=0; y < MAXY-MINY; y++){
		for(int x=0; x < MAXX-MINX; x++){
			p=xmlNewChild(doc->children, NULL, (const xmlChar*)"tile", NULL);
			Tile* tile=tiles[x][y];
			xmlAddChild(p,(*(tile->begin()))->serialize());
			Tile::iterator it=tile->end(); it--;
			for (; it != tile->begin(); it--)
				xmlAddChild(p,(*it)->serialize());
		}
	}
	xmlKeepBlanksDefault(1);
	xmlSaveFile("map.xml", doc);
	xmlFreeDoc(doc);
	return true;
}

int Map::loadMapXml(std::string map){
	xmlDocPtr doc;
	xmlNodePtr root, tile, item;
	int width, height;


	doc=xmlParseFile(map.c_str());
	root=xmlDocGetRootElement(doc);
	if(xmlStrcmp(root->name,(const xmlChar*) "map")){
		std::cout << "FATAL: couldnt load map. exiting" << std::endl;
		exit(1);
	}
	width=atoi((const char*)xmlGetProp(root, (const xmlChar *) "width"));
	height=atoi((const char*)xmlGetProp(root, (const xmlChar *) "height"));
	int xorig=((MAXX-MINX)-width)/2;
	int yorig=((MAXY-MINY)-height)/2;
	std::cout << xorig << "   " << yorig << std::endl;
	tile=root->children;
	for(int y=0; y < height; y++){
		for(int x=0; x < width; x++){
			item=tile->children;

			while(item != NULL){
				Item* myitem=new Item();
				myitem->unserialize(item);
				tiles[x+xorig][y+yorig]->addItem(myitem);
				item=item->next;
			}
			tile=tile->next;

		}
	}
	std::cout << "Loaded XML-Map" << std::endl;
	xmlFreeDoc(doc);
	return 0;
}

int Map::saveMap(){
	//save the map
	#ifdef __DEBUG__
	std::cout << "Saving the map" << std::endl;
	#endif
	FILE* dump=fopen("otserv.map", "wb+");
	//first dump position of top left corner, then bottom right corner
	//then the raw map-dumpa
	fputc((int)(MINX/256), dump);
	fputc((int)(MINX%256), dump);
	fputc((int)(MINY/256), dump);
	fputc((int)(MINY%256), dump);
	fputc( (int) 7, dump);

	fputc((int)(MAXX/256), dump);
	fputc((int)(MAXX%256), dump);
	fputc( (int)(MAXY/256), dump);
	fputc((int)(MAXY%256), dump);
	fputc((int) 7, dump);

	for(int y=0; y < MAXY-MINY; y++){
		for(int x=0; x < MAXX-MINX; x++){
			Tile* tile=tiles[x][y];
			for (Tile::iterator it=tile->begin(); it != tile->end(); it++) {
				fputc((int)( (*it)->getID()/256), dump);
				fputc((int)( (*it)->getID()%256), dump);
			}
			fputc(0x00, dump);
			fputc(0xFF, dump); // tile end
		}
	}
	fclose(dump);
	return true;
}

Creature* Map::getPlayerByID( unsigned long id ){
  std::map<long, Creature*>::iterator i;
  for( i = playersOnline.begin(); i != playersOnline.end(); i++ )
  {
    if( (i->second)->getID() == id )
    {
		return i->second;
    }
  }
  return NULL; //just in case the player doesnt exist
}

position Map::placeCreature(position pos, Creature* c){
//	pos.x+=192;
//	pos.y+=192;
	//add creature to the online list
	playersOnline[c->getID()]=c;
  if( tiles[pos.x-MINX][pos.y-MINY]->isBlocking()){
  	//crap we need to find another spot
	pos.x++;
	return placeCreature(pos, c);
  }
  tiles[pos.x-MINX][pos.y-MINY]->creature=c;
  //we placed the creature, now tell everbody who saw it about the login
  Action* a = new Action;
  a->type=ACTION_LOGIN;
  a->pos1=pos;
  a->creature=c;
  distributeAction(pos, a);
  return pos;
}

int Map::removeCreature(position pos){
	//removeCreature from the online list
	playersOnline.erase(playersOnline.find(tiles[pos.x-MINX][pos.y-MINY]->creature->getID()));
	std::cout << "removing creature "<< std::endl;
	if(tiles[pos.x-MINX][pos.y-MINY]->creature)
		tiles[pos.x-MINX][pos.y-MINY]->creature=NULL;
	else
		std::cout <<"ERROR NO CREATURE" <<std::endl;
  //now distribute the action
  Action* a= new Action;
  a->type=ACTION_LOGOUT;
  a->pos1=pos;
  distributeAction(pos, a);
  delete a;
  return true;
}

int Map::requestAction(Creature* c, Action* a){
	//TODO us a switch here
	if(a->type == ACTION_TURN){
		//no checking needs to be done, we can always turn
		a->stack=GETTILEBYPOS(a->pos1)->getStackPosPlayer();
		distributeAction(a->pos1, a);
	}
	if(a->type==ACTION_MOVE){
		a->pos2=a->pos1;
		if(a->direction%2){
			int x=a->direction-2;
			a->pos2.x-=x;
		}
		else{
			int y=a->direction-1;
			a->pos2.y+=y;
		}
		//we got the new action, now distribute it
		//FIXME THIS IS A BUG!!!
			#ifdef __DEBUG__
			std::cout << "Going to distribute an action" << std::endl;
			#endif
		//we move the pointer
		if(tiles[a->pos2.x-MINX][a->pos2.y-MINY]->isBlocking())
			return TMAP_ERROR_TILE_OCCUPIED;
		a->stack=tiles[a->pos1.x-MINX][a->pos1.y-MINY]->getStackPosPlayer();
		tiles[a->pos2.x-MINX][a->pos2.y-MINY]->creature=tiles[a->pos1.x-MINX][a->pos1.y-MINY]->creature;
		tiles[a->pos1.x-MINX][a->pos1.y-MINY]->creature=NULL;
		a->creature=c;
		distributeAction(a->pos1, a);
	}
	if(a->type==ACTION_SAY){
		//says should be ok most of the time
		distributeAction(a->pos1, a);
	}
	if(a->type==ACTION_THROW){
	//TODO make stackbles work correctly for equip
	//TODO we should really check, if the player can throw
	/* throwing works like this:
	we check where the item comes from and if the number of items is available
	there then we calculate the number that can be added to the target. if
	they are stackable and it makes them become more then 100, we add another
	pile. if we are on an inventore slot, 100 is the max and the rest has to stay
	where we started the move.
	we
	*/
	//First, we get the source item
	Item* itemMoved;
	if(a->pos1.x==0xFFFF){
		itemMoved=a->creature->getItem(a->pos1.y);
		a->creature->addItem(NULL, a->pos1.y);
	}
	else{
		Tile* tile=tiles[a->pos1.x-MINX][a->pos1.y-MINX];
		itemMoved = tile->getItemByStack(a->stack);
		//now, we remove the item from the source
		Action remove;
		remove.type=ACTION_ITEM_DISAPPEAR;
		remove.pos1=a->pos1;
		remove.stack=a->stack;
		remove.count=a->count;
		remove.id=a->id;
		removeItem(&remove);
	}

	if(!itemMoved){
		//something went wrong, we got no item
		return false;
	}

	//we got the item;
	if(a->pos2.x==0xFFFF){
		//we have to spawn this on a playerslot
		//DAMN!! ;)
		if(a->creature->getItem(a->pos2.y)){
			//TODO make this work, when already occupied by some other item
			//theres something on it...
		}
		else{
			a->creature->addItem(itemMoved, a->pos2.y);
		}
	}
	else{ //target is on a maptile
		//TODO
		//check if there are enough items to move
		//TODO
		//check if we can put the item there
		//TODO
		//move only the pointer: faster and necessary
		//for things that are more the and id and a count
		Action createItem;
		createItem.type=ACTION_ITEM_APPEAR;
		createItem.pos1=a->pos2;
		createItem.id=itemMoved->getID();
		createItem.count=a->count;
		summonItem(&createItem);
	}
	}
	if(a->type==ACTION_LOOK_AT){

		Tile* tile=tiles[a->pos1.x-MINX][a->pos1.y-MINX];
		a->buffer=tile->getDescription();
		a->creature->sendAction(a);
	}
	else if(a->type==ACTION_REQUEST_APPEARANCE){
		a->creature->sendAction(a);
	}
	else if(a->type==ACTION_CHANGE_APPEARANCE){
		distributeAction(a->pos1,a);
	}
	else if(a->type==ACTION_ITEM_USE){
		GETTILEBYPOS(a->pos1)->getItemByStack(a->stack)->use();
		distributeAction(a->pos1,a);
	}
	delete a;
	return true;
}

int Map::distributeAction(position pos, Action* a){
	//finds out how saw a certain action and tells him about it
	//the region in which we can be seen is
	// -9/-7 to 9/7
	std::list<Creature*> victims;
	for(int x=-7; x <=7; x++){
		for(int y=-6; y <=6; y++){
			if(tiles[a->pos1.x+x-MINX][a->pos1.y+y-MINY]->creature){
				victims.push_back(tiles[a->pos1.x+x-MINX][a->pos1.y+y-MINY]->creature);
			}
		}
	}
	for(int x=-7; x <=7; x++){
		for(int y=-6; y <=6; y++){
			if(tiles[a->pos2.x+x-MINX][a->pos2.y+y-MINY]->creature){
				victims.push_back(tiles[a->pos2.x+x-MINX][a->pos2.y+y-MINY]->creature);
			}
		}
	}
	victims.sort();
	victims.unique();
	std::list<Creature*>::iterator i;
	for(i=victims.begin(); i!=victims.end();++i)
		(*i)->sendAction(a);
	return true;
}

int Map::summonItem(Action* a){
	Item* item=new Item(a->id);
	if(item->isStackable() && !a->count)
		return TMAP_ERROR_NO_COUNT;
	if(!a->id)
		return false;
	#ifdef __DEBUG__
	#endif
	item->count=a->count;
	Tile* tile = tiles[a->pos1.x-MINX][a->pos1.y-MINY];
	if((unsigned int)a->id != tile->getItemByStack(tile->getStackPosItem())->getID() ||
		!item->isStackable()){
		std::cout << "appear id: " << tile->getItemByStack(tile->getStackPosItem())->getID()<< std::endl;
		//if an item of this type isnt already there or this type isnt stackable
		tile->addItem(item);
		Action b;
		b.type=ACTION_ITEM_APPEAR;
		b.pos1=a->pos1;
		b.id=a->id;
		if(item->isStackable())
		b.count=a->count;
		distributeAction(a->pos1, &b);
	}
	else {
		std::cout << "merge" << std::endl;
		//we might possibly merge the top item and the stuff we want to add
		Item* onTile = tile->getItemByStack(tile->getStackPosItem());
		onTile->count+=item->count;
		int tmpnum=0;
		if(onTile->count>100){
			tmpnum= onTile->count-100;
			onTile->count=100;
		}
		Action b;
		b.type=ACTION_ITEM_CHANGE;
		b.pos1=a->pos1;
		b.id=a->id;
		b.count=onTile->count;

		distributeAction(a->pos1, &b);

		if(tmpnum>0){
			std::cout << "creating extra item" << std::endl;
			item->count=tmpnum;
			tile->addItem(item);
			Action c;
			c.type=ACTION_ITEM_APPEAR;
			c.pos1=a->pos1;
			c.id=a->id;
			c.count=item->count;
			distributeAction(a->pos1, &c);
			//not all fit into the already existing item, create a new one
		}
	}
	return true;
}

int Map::summonItem(position pos, int id){
	std::cout << "Deprecated summonItem" << std::cout;
	if(!id)
		return false;
	#ifdef __DEBUG__
	std::cout << "Summoning item with id " << id << std::endl;
	#endif
	Item* i=new Item(id);
	tiles[pos.x-MINX][pos.y-MINY]->addItem(i);
	Action* a= new Action;
	a->type=ACTION_ITEM_APPEAR;
	a->pos1=pos;
	a->id=id;
	if(!i->isStackable())
		a->count=0;
	distributeAction(pos, a);
	return true;
}

int Map::changeGround(position pos, int id){
  if(!id)
    return false;
#ifdef __DEBUG__
  std::cout << "Summoning item with id " << id << std::endl;
#endif
	Item* i=new Item(id);
  tiles[pos.x-MINX][pos.y-MINY]->addItem(i);
  Action* a= new Action;
  a->type=ACTION_GROUND_CHANGE;
  a->pos1=pos;
  a->id=id;
	if(!i->isStackable())
		a->count=0;
  distributeAction(pos, a);
  return true;
}

int Map::removeItem(position pos){
	std::cout << "Deprecated removeItem" << std::cout;
	Tile* tile=tiles[pos.x-MINX][pos.y-MINY];
	if(tile->size()<=1)
		return false;
	//FIXME Do this by stack
	tile->pop_back();
	Action* a= new Action;
	a->type=ACTION_ITEM_DISAPPEAR;
	if(tile->creature)
	  a->stack=2;
	else
	  a->stack=1;
	a->pos1=pos;
	distributeAction(pos, a);
	return true;
}

int Map::removeItem(Action* a){
	int newcount;
	Tile* tile=tiles[a->pos1.x-MINX][a->pos1.y-MINY];
	std::cout << "COUNT: " << a->count << std::endl;
	newcount=tile->removeItem(a->stack, a->type, a->count);
	std::cout << "COUNT: " << newcount << std::endl;
	if(newcount==0)
		distributeAction(a->pos1,a);
	else{
		std::cout << "distributing change" << std::endl;
		a->type=ACTION_ITEM_CHANGE;
		a->count=newcount;
		distributeAction(a->pos1,a);
	}
	return true;
}

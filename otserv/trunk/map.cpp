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


#include "definitions.h"

#include <string>

using namespace std;

#include "otsystem.h"


#include <stdio.h>

#include "items.h"
#include "map.h"
#include "tile.h"

#include "player.h"

#include "networkmessage.h"

#include "npc.h"


#define EVENT_CHECKPLAYER          123
#define EVENT_CHECKPLAYERATTACKING 124


Map::Map()
{
  //first we fill the map with
	for(int y = 0; y < MAP_HEIGHT; y++)
  {
		for(int x = 0; x < MAP_WIDTH; x++)
    {
      tiles[x][y] = new Tile();

		  if(abs(x)<10 || abs(x)<10 || abs(y)<10 || abs(y)<10)
			  tiles[x][y]->ground = 605;
		  else
  			tiles[x][y]->ground = 102;
		}
	}

  /* todo: out of the constructor :/ */

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
		Position topleft, bottomright, now;


		topleft.x=fgetc(dump)*256;	topleft.x+=fgetc(dump);
		topleft.y=fgetc(dump)*256;	topleft.y+=fgetc(dump);
		topleft.z=fgetc(dump);

		bottomright.x=fgetc(dump)*256;	bottomright.x+=fgetc(dump);
		bottomright.y=fgetc(dump)*256;	bottomright.y+=fgetc(dump);
		bottomright.z=fgetc(dump);

		int xsize= bottomright.x-topleft.x;
		int ysize= bottomright.y-topleft.y;
		int xorig=((MAP_WIDTH)-xsize)/2;
		int yorig=((MAP_HEIGHT)-ysize)/2;
		//TODO really place this map patch where it belongs

		for(int y=0; y < ysize; y++){
			for(int x=0; x < xsize; x++){
				while(true){
					int id=fgetc(dump)*256;id+=fgetc(dump);
					if(id==0x00FF)
						break;
					//now.x=x+MINX;now.y=y+MINY;now.z=topleft.z;
					tiles[xorig][yorig]->addThing(new Item(id));
					//tiles[x][y]->push_back(new Item(id));
				}
			}
		}
		fclose(dump);
	}
  
  OTSYS_THREAD_LOCKVARINIT(mapLock);
  OTSYS_THREAD_LOCKVARINIT(eventLock);
  
  Npc *npc = new Npc("Ruediger");
  npc->pos.x = 220;
  npc->pos.y = 220;

  placeCreature(npc);

  OTSYS_CREATE_THREAD(eventThread, this);
}


Map::~Map()
{
}


/*****************************************************************************/


OTSYS_THREAD_RETURN Map::eventThread(void *p)
{
  Map* _this = (Map*)p;

  memset(_this->eventLists, 0, sizeof(_this->eventLists));

  __int64 eventTick = OTSYS_TIME() / 10;

  while (true)
  {
    if (eventTick < OTSYS_TIME() / 10)
    {
      list<MapEvent> *eventList = NULL;

      OTSYS_THREAD_LOCK(_this->eventLock)
      {
        eventList = _this->eventLists[eventTick % 12000];
        _this->eventLists[eventTick % 12000] = NULL;
      }
      OTSYS_THREAD_UNLOCK(_this->eventLock)

      if (eventList != NULL)
      {
        list<MapEvent>::iterator it;
        for (it = eventList->begin(); it != eventList->end(); it++)
        {
          if ((*it).tick == eventTick)
          {
            switch ((*it).type)
            {
              case EVENT_CHECKPLAYER:
                _this->checkPlayer((unsigned long)(*it).data);
                break;

              case EVENT_CHECKPLAYERATTACKING:
                _this->checkPlayerAttacking((unsigned long)(*it).data);
                break;
            }
          }
          else
          {
            /* todo reschedule */
          }
        }

        delete eventList;
      }

      eventTick++;
    }
    else
    {
      OTSYS_SLEEP(1);  // nothing to-do :)
    }
  }
}

void Map::addEvent(long ticks, int type, void *data)
{
  MapEvent e;
  e.tick = OTSYS_TIME() / 10 + ticks;
  e.type = type;
  e.data = data;

  OTSYS_THREAD_LOCK(eventLock)

  if (eventLists[e.tick % 12000] == NULL)
    eventLists[e.tick % 12000] = new list<MapEvent>;

  eventLists[e.tick % 12000]->push_back(e);

  OTSYS_THREAD_UNLOCK(eventLock)
}


/*****************************************************************************/


Tile *Map::tile(unsigned short _x, unsigned short _y, unsigned char _z) {
    return tiles[_x][_y];
}


int Map::loadMapXml(const char *filename){
	xmlDocPtr doc;
	xmlNodePtr root, tile, item;
	int width, height;


	doc=xmlParseFile(filename);
	root=xmlDocGetRootElement(doc);
	if(xmlStrcmp(root->name,(const xmlChar*) "map")){
		std::cout << "FATAL: couldnt load map. exiting" << std::endl;
		exit(1);
	}
	width=atoi((const char*)xmlGetProp(root, (const xmlChar *) "width"));
	height=atoi((const char*)xmlGetProp(root, (const xmlChar *) "height"));
	int xorig=((MAP_WIDTH)-width)/2;
	int yorig=((MAP_HEIGHT)-height)/2;
	std::cout << xorig << "   " << yorig << std::endl;
	tile=root->children;
	for(int y=0; y < height; y++){
		for(int x=0; x < width; x++){
			item=tile->children;

			while(item != NULL){
				Item* myitem=new Item();
				myitem->unserialize(item);
				tiles[x+xorig][y+yorig]->addThing(myitem);
				item=item->next;
			}
			tile=tile->next;

		}
	}
	std::cout << "Loaded XML-Map" << std::endl;
	xmlFreeDoc(doc);
	return 0;
}



Creature* Map::getCreatureByID(unsigned long id)
{
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


bool Map::placeCreature(Creature* c)
{
  OTSYS_THREAD_LOCK(mapLock)

	// add player to the online list
  if (c->isPlayer())
  {
	  playersOnline[c->getID()] = c;

    addEvent(100, EVENT_CHECKPLAYER, (void*)c->id);
    addEvent(200, EVENT_CHECKPLAYERATTACKING, (void*)c->id);

    ((Player*)c)->usePlayer();
  }

  while (tiles[c->pos.x][c->pos.y]->isBlocking())
  {
  	//crap we need to find another spot
	  c->pos.x++;
  }

  tiles[c->pos.x][c->pos.y]->addThing(c);

  CreatureVector::iterator cit;
  for (int x =c->pos.x - 9; x <= c->pos.x + 9; x++)
    for (int y = c->pos.y - 7; y <= c->pos.y + 7; y++)
      for (cit = tiles[x][y]->creatures.begin(); cit != tiles[x][y]->creatures.end(); cit++)
      {
        (*cit)->onCreatureAppear(c);
      }

  OTSYS_THREAD_UNLOCK(mapLock)

  return true;
}

bool Map::removeCreature(Creature* c)
{
  OTSYS_THREAD_LOCK(mapLock)

	//removeCreature from the online list
  playersOnline.erase(playersOnline.find(c->getID()));

	std::cout << "removing creature "<< std::endl;

  int stackpos = tiles[c->pos.x][c->pos.y]->getCreatureStackPos(c);
  tiles[c->pos.x][c->pos.y]->removeThing(c);

  CreatureVector::iterator cit;
  for (int x =c->pos.x - 9; x <= c->pos.x + 9; x++)
    for (int y = c->pos.y - 7; y <= c->pos.y + 7; y++)
      for (cit = tiles[x][y]->creatures.begin(); cit != tiles[x][y]->creatures.end(); cit++)
      {
        (*cit)->onCreatureDisappear(c, stackpos);
      }

  if (c->isPlayer())
    ((Player*)c)->releasePlayer();

  OTSYS_THREAD_UNLOCK(mapLock)

  return true;
}

/*int Map::requestAction(Creature* c, Action* a)
{

	if(a->type==ACTION_LOOK_AT){

		a->buffer=tiles[a->pos1.x][a->pos1.y]->getDescription();
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

	return true;
}
*/

/*int Map::summonItem(Action* a)
{
	Item* item=new Item(a->id);
	if(item->isStackable() && !a->count)
		return TMAP_ERROR_NO_COUNT;
	if(!a->id)
		return false;
	#ifdef __DEBUG__
	#endif
	item->count=a->count;
	Tile* tile = tiles[a->pos1.x][a->pos1.y];
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
*/

/*
int Map::summonItem(Position pos, int id)
{
  
	std::cout << "Deprecated summonItem" << std::cout;
	if(!id)
		return false;
	#ifdef __DEBUG__
	std::cout << "Summoning item with id " << id << std::endl;
	#endif
	Item* i=new Item(id);
	tiles[pos.x][pos.y]->addItem(i);
	Action* a= new Action;
	a->type=ACTION_ITEM_APPEAR;
	a->pos1=pos;
	a->id=id;
	if(!i->isStackable())
		a->count=0;
	distributeAction(pos, a);
	return true;
  
}

  */

/*
int Map::changeGround(Position pos, int id){
  if(!id)
    return false;
#ifdef __DEBUG__
  std::cout << "Summoning item with id " << id << std::endl;
#endif
	Item* i=new Item(id);
  tiles[pos.x][pos.y]->addItem(i);
  Action* a= new Action;
  a->type=ACTION_GROUND_CHANGE;
  a->pos1=pos;
  a->id=id;
	if(!i->isStackable())
		a->count=0;
  distributeAction(pos, a);
  return true;
}

int Map::removeItem(Action* a){
	int newcount;
	Tile* tile=tiles[a->pos1.x][a->pos1.y];
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

*/

void Map::thingMove(Player *player, Thing *thing,
                    unsigned short to_x, unsigned short to_y, unsigned char to_z)
{
  OTSYS_THREAD_LOCK(mapLock)

  //if ((abs(player->pos.x - from_x) > 1) || (abs(player->pos - from_y) > 1))
//    return;

  Position oldPos = thing->pos;

  Tile *fromTile = tiles[oldPos.x][oldPos.y];  // TODO: null for outside -> func
  Tile *toTile   = tiles[to_x  ][to_y  ];

  if ((fromTile != NULL) && (toTile != NULL))
  {
    if ((abs(oldPos.x - to_x) > thing->ThrowRange) ||
        (abs(oldPos.y - to_y) > thing->ThrowRange))
    {
      // TODO... "not Posible"
    }
    else if (!thing->CanMovedTo(toTile))
    {
      // TODO... "not Posible"
    }
    else
    {
      int oldstackpos = fromTile->getThingStackPos(thing);

      if (!fromTile->removeThing(thing))
        return;  // TODO... "not Posible" ; hmm not there

      toTile->addThing(thing);

      thing->pos.x = to_x;
      thing->pos.y = to_y;
      thing->pos.z = to_z;
    
      CreatureVector::iterator cit;
      for (int x = min(oldPos.x, (int)to_x) - 9; x <= max(oldPos.x, (int)to_x) + 9; x++)
        for (int y = min(oldPos.y, (int)to_y) - 7; y <= max(oldPos.y, (int)to_y) + 7; y++)
          for (cit = tiles[x][y]->creatures.begin(); cit != tiles[x][y]->creatures.end(); cit++)
          {
            (*cit)->onThingMove(player, thing, &oldPos, oldstackpos);
          }
    }
  }

  OTSYS_THREAD_UNLOCK(mapLock)
}


void Map::thingMove(Player *player,
                    unsigned short from_x, unsigned short from_y, unsigned char from_z,
                    unsigned char stackPos,
                    unsigned short to_x, unsigned short to_y, unsigned char to_z)
{
  OTSYS_THREAD_LOCK(mapLock)


  OTSYS_THREAD_UNLOCK(mapLock)
}


void Map::creatureTurn(Creature *creature, Direction dir)
{
  OTSYS_THREAD_LOCK(mapLock)

  if (creature->direction != dir)
  {
    creature->direction = dir;

    int stackpos = 1; /* todo */

    CreatureVector::iterator cit;
    for (int x = creature->pos.x - 9; x <= creature->pos.x + 9; x++)
      for (int y = creature->pos.y - 7; y <= creature->pos.y + 7; y++)
        for (cit = tiles[x][y]->creatures.begin(); cit != tiles[x][y]->creatures.end(); cit++)
        {
          (*cit)->onCreatureTurn(creature, stackpos);
        }
  }

  OTSYS_THREAD_UNLOCK(mapLock)
}


void Map::playerSay(Player *player, unsigned char type, const string &text)
{
  OTSYS_THREAD_LOCK(mapLock)

  CreatureVector::iterator cit;

  for (int x = player->pos.x - 7; x <= player->pos.x + 7; x++)
    for (int y = player->pos.y - 5; y <= player->pos.y + 5; y++)
      for (cit = tiles[x][y]->creatures.begin(); cit != tiles[x][y]->creatures.end(); cit++)
      {
        (*cit)->onCreatureSay(player, type, text);
      }

  OTSYS_THREAD_UNLOCK(mapLock)
}


void Map::playerYell(Player *player, const string &text)
{
  OTSYS_THREAD_LOCK(mapLock)

  CreatureVector::iterator cit;

  for (int x = player->pos.x - 18; x <= player->pos.x + 18; x++)
    for (int y = player->pos.y - 14; y <= player->pos.y + 14; y++)
      for (cit = tiles[x][y]->creatures.begin(); cit != tiles[x][y]->creatures.end(); cit++)
      {
        (*cit)->onCreatureSay(player, 3, text);
      }

  OTSYS_THREAD_UNLOCK(mapLock)
}


void Map::playerSpeakTo(Player *player, const string &receiver, const string &text)
{
  OTSYS_THREAD_LOCK(mapLock)
  OTSYS_THREAD_UNLOCK(mapLock)
}


void Map::playerBroadcastMessage(Player *player, const string &text)
{
  OTSYS_THREAD_LOCK(mapLock)
  OTSYS_THREAD_UNLOCK(mapLock)
}


void Map::creatureMakeDistDamage(Creature *creature, Creature *attackedCreature)
{
//  if ((Math.Abs(creature.Position.x-attackedCreature.Position.x) <= 8) &&
  //    (Math.Abs(creature.Position.y-attackedCreature.Position.y) <= 6) &&
    //  (creature.Position.z == attackedCreature.Position.z))
  {
    int damage = rand() % 300 - 100; //attackedCreature.CalculateDistDamage(creature);

    if (damage < -50)
      damage = 0;

    if (damage > 0)
      attackedCreature->drainHealth(damage);

    CreatureVector::iterator cit;

    NetworkMessage msg;

    for (int x = min(creature->pos.x, attackedCreature->pos.x) - 9; x <= max(creature->pos.x, attackedCreature->pos.x) + 9; x++)
      for (int y = min(creature->pos.y, attackedCreature->pos.y) - 7; y <= max(creature->pos.y, attackedCreature->pos.y) + 7; y++)
        for (cit = tiles[x][y]->creatures.begin(); cit != tiles[x][y]->creatures.end(); cit++)
        {
          if ((*cit)->isPlayer())
          {
            Player *p = (Player*)(*cit);

            msg.Reset();

            msg.AddDistanceShoot(creature->pos, attackedCreature->pos, 13);

            if ((damage == 0) && (p->CanSee(attackedCreature->pos.x, attackedCreature->pos.y)))
            {
              msg.AddMagicEffect(attackedCreature->pos, NM_ME_PUFF);
            }
            else if ((damage < 0) && (p->CanSee(attackedCreature->pos.x, attackedCreature->pos.y)))
            {
              msg.AddMagicEffect(attackedCreature->pos, NM_ME_BLOCKHIT);
            }
            else
            {
              if (p->CanSee(attackedCreature->pos.x, attackedCreature->pos.y))
              {
                msg.AddAnimatedText(attackedCreature->pos, 0xB4, "1337");
                msg.AddMagicEffect(attackedCreature->pos, NM_ME_DRAW_BLOOD);
                msg.AddCreatureHealth(attackedCreature);
              }

              // own damage, update infos
              //if (p == attackedCreature)
                //CreateDamageUpdate(p, creature, ref netMessage);
            }

            p->sendNetworkMessage(&msg);
          }
        }
  }
}


void Map::checkPlayer(unsigned long id)
{
  OTSYS_THREAD_LOCK(mapLock)

  Creature *creature = getCreatureByID(id);

  if (creature != NULL)
  {
    
    addEvent(100, EVENT_CHECKPLAYER, (void*)id);   
  }

  OTSYS_THREAD_UNLOCK(mapLock)
}


void Map::checkPlayerAttacking(unsigned long id)
{
  OTSYS_THREAD_LOCK(mapLock)

  Player *player = (Player*)getCreatureByID(id);

  if (player != NULL)
  {
    if (player->attackedCreature != 0)
    {
      Creature *attackedCreature = (Player*)getCreatureByID(player->attackedCreature);

      if (attackedCreature != NULL)
        creatureMakeDistDamage(player, attackedCreature);
    }

    addEvent(200, EVENT_CHECKPLAYERATTACKING, (void*)id);
  }

  OTSYS_THREAD_UNLOCK(mapLock)
}



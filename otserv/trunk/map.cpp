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
#include <sstream>

using namespace std;

#include "otsystem.h"

#include <stdio.h>

#include "items.h"
#include "map.h"
#include "tile.h"

#include "player.h"

#include "networkmessage.h"

#include "npc.h"

#include "luascript.h"
#include <ctype.h>

#define EVENT_CHECKPLAYER          123
#define EVENT_CHECKPLAYERATTACKING 124

extern LuaScript g_config;

Map::Map()
{
  //first we fill the map with
  for(int y = 0; y < MAP_HEIGHT; y++)
  {
    for(int x = 0; x < MAP_WIDTH; x++)
    {
      //				setTile(x,y,7,102);
    }
  }


  OTSYS_THREAD_LOCKVARINIT(mapLock);
  OTSYS_THREAD_LOCKVARINIT(eventLock);
  OTSYS_THREAD_SIGNALVARINIT(eventSignal);

  OTSYS_CREATE_THREAD(eventThread, this);
}


Map::~Map()
{
}


bool Map::LoadMap(std::string filename) {

  // get maximum number of players allowed...
  max_players = atoi(g_config.getGlobalString("maxplayers").c_str());
  std::cout << ":: Player Limit: " << max_players << std::endl;

  FILE* f;
  std::cout << ":: Loading Map from " << filename << " ... ";
  f=fopen(filename.c_str(),"r");
  if(f){
    fclose(f);
    loadMapXml(filename.c_str());
    std::cout << "[done]" << std::endl;
    return true;
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
      std::cout << "Loading old format mapfile failed" << std::endl;
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
        while(true)
        {
          int id=fgetc(dump)*256;id+=fgetc(dump);
          if(id==0x00FF)
            break;
          //now.x=x+MINX;now.y=y+MINY;now.z=topleft.z;

          Item *item = new Item(id);
          if (item->isGroundTile())
          {
            setTile(xorig+x, yorig+y, 7, id);
            delete item;
          }
          else
          {
            Tile *t = getTile(xorig+x, yorig+y, 7);
            if (t)
              t->addThing(item);
          }
          //tiles[x][y]->push_back(new Item(id));
        }
      }
    }
    fclose(dump);
  }

  Npc *npc = new Npc("Ruediger");
  npc->pos.x = 220;
  npc->pos.y = 220;

  placeCreature(npc);

  return true;
}



/*****************************************************************************/


OTSYS_THREAD_RETURN Map::eventThread(void *p)
{
  Map* _this = (Map*)p;

  // basically what we do is, look at the first scheduled item,
  // and then sleep until it's due (or if there is none, sleep until we get an event)
  // of course this means we need to get a notification if there are new events added
  while (true)
  {
#ifdef __DEBUG__EVENTSCHEDULER__
    std::cout << "schedulercycle start..." << std::endl;
#endif

    SchedulerTask* task = NULL;

    // check if there are events waiting...
    OTSYS_THREAD_LOCK(_this->eventLock)

      int ret;
    if (_this->eventList.size() == 0) {
      // unlock mutex and wait for signal
      ret = OTSYS_THREAD_WAITSIGNAL(_this->eventSignal, _this->eventLock);
    } else {
      // unlock mutex and wait for signal or timeout
      ret = OTSYS_THREAD_WAITSIGNAL_TIMED(_this->eventSignal, _this->eventLock, _this->eventList.top()->getCycle());
    }
    // the mutex is locked again now...
    if (ret == OTSYS_THREAD_TIMEOUT) {
      // ok we had a timeout, so there has to be an event we have to execute...
#ifdef __DEBUG__EVENTSCHEDULER__
      std::cout << "event found at " << OTSYS_TIME() << " which is to be scheduled at: " << _this->eventList.top()->getCycle() << std::endl;
#endif
      task = _this->eventList.top();
      _this->eventList.pop();
    }

    OTSYS_THREAD_UNLOCK(_this->eventLock);
    if (task) {
      (*task)(_this);
      delete task;
    }
  }

  /*

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
     std::list<MapEvent>::iterator it;
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
  // todo reschedule 
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
  */
}

void Map::addEvent(SchedulerTask* event) {
  bool do_signal = false;
  OTSYS_THREAD_LOCK(eventLock)

    eventList.push(event);
  if (eventList.empty() || *event < *eventList.top())
    do_signal = true;

  OTSYS_THREAD_UNLOCK(eventLock)

    if (do_signal)
      OTSYS_THREAD_SIGNAL_SEND(eventSignal);

}

/*****************************************************************************/



Tile* Map::getTile(unsigned short _x, unsigned short _y, unsigned char _z)
{
  if (_z < MAP_LAYER)
  {
    // _x & 0x3F  is like _x % 64
    TileMap *tm = &tileMaps[_x & 1][_y & 1][_z];

    // search in the stl map for the requested tile
    TileMap::iterator it = tm->find((_x << 16) | _y);

    // ... found
    if (it != tm->end())
      return it->second;
  }

  // or not
  return NULL;
}


void Map::setTile(unsigned short _x, unsigned short _y, unsigned char _z, unsigned short groundId)
{
  Tile *tile = getTile(_x, _y, _z);

  if (tile != NULL)
  {
    tile->ground = groundId;
  }
  else
  {
    tile = new Tile();
    tile->ground = groundId;
    tileMaps[_x & 1][_y & 1][_z][(_x << 16) | _y] = tile;
  }  
}



int Map::loadMapXml(const char *filename){
  xmlDocPtr doc;
  xmlNodePtr root, tile, item;
  int width, height;


  doc=xmlParseFile(filename);
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
  for(int y=0; y < height; y++){
    for(int x=0; x < width; x++){
      item=tile->children;

      while(item != NULL)
      {
        Item* myitem=new Item();
        myitem->unserialize(item);

        if (myitem->isGroundTile())
        {
          setTile(xorig+x, yorig+y, 7, myitem->getID());
          delete myitem;
        }
        else
        {
          Tile *t = getTile(xorig+x, yorig+y, 7);
          if (t)
            t->addThing(myitem);
        }

        item=item->next;
      }
      tile=tile->next;

    }
  }
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
  if (c->access == 0 && playersOnline.size() >= max_players)
    return false;

  OTSYS_THREAD_LOCK(mapLock)

    // add player to the online list
    if (c->isPlayer())
    {
      playersOnline[c->getID()] = c;

      addEvent(makeTask(1000, std::bind2nd(std::mem_fun(&Map::checkPlayer), c->id)));
      addEvent(makeTask(2000, std::bind2nd(std::mem_fun(&Map::checkPlayerAttacking), c->id)));

      ((Player*)c)->usePlayer();
      std::cout << playersOnline.size() << " players online." << std::endl;
    }

  while (getTile(c->pos.x, c->pos.y, c->pos.z)->isBlocking())
  {
    // crap we need to find another spot
    c->pos.x++;
  }

  getTile(c->pos.x, c->pos.y, c->pos.z)->addThing(c);

  CreatureVector::iterator cit;
  for (int x = c->pos.x - 9; x <= c->pos.x + 9; x++)
    for (int y = c->pos.y - 7; y <= c->pos.y + 7; y++)
    {
      Tile *tile = getTile(x, y, 7);
      if (tile)
      {
        for (cit = tile->creatures.begin(); cit != tile->creatures.end(); cit++)
        {
          (*cit)->onCreatureAppear(c);
        }
      }
    }

  OTSYS_THREAD_UNLOCK(mapLock)

    return true;
}

bool Map::removeCreature(Creature* c)
{
  OTSYS_THREAD_LOCK(mapLock)

    //removeCreature from the online list

    std::map<long, Creature*>::iterator pit = playersOnline.find(c->getID());
  if (pit != playersOnline.end()) {
    playersOnline.erase(pit);


#ifdef __DEBUG__
    std::cout << "removing creature "<< std::endl;
#endif

    int stackpos = getTile(c->pos.x, c->pos.y, c->pos.z)->getCreatureStackPos(c);
    getTile(c->pos.x, c->pos.y, c->pos.z)->removeThing(c);

    CreatureVector::iterator cit;
    for (int x = c->pos.x - 9; x <= c->pos.x + 9; x++)
      for (int y = c->pos.y - 7; y <= c->pos.y + 7; y++)
      {
        Tile *tile = getTile(x, y, 7);
        if (tile)
        {
          for (cit = tile->creatures.begin(); cit != tile->creatures.end(); cit++)
          {
            (*cit)->onCreatureDisappear(c, stackpos);
          }
        }
      }

  }

  std::cout << playersOnline.size() << " players online." << std::endl;

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

    Tile *fromTile = getTile(thing->pos.x, thing->pos.y, thing->pos.z);

  if (fromTile)
  {
    int oldstackpos = fromTile->getThingStackPos(thing);

    thingMoveInternal(player, thing->pos.x, thing->pos.y, thing->pos.z, oldstackpos, to_x, to_y, to_z);
  }

  OTSYS_THREAD_UNLOCK(mapLock)
}


void Map::thingMove(Player *player,
    unsigned short from_x, unsigned short from_y, unsigned char from_z,
    unsigned char stackPos,
    unsigned short to_x, unsigned short to_y, unsigned char to_z)
{
  OTSYS_THREAD_LOCK(mapLock)

    thingMoveInternal(player, from_x, from_y, from_z, stackPos, to_x, to_y, to_z);

  OTSYS_THREAD_UNLOCK(mapLock)
}



void Map::thingMoveInternal(Player *player,
    unsigned short from_x, unsigned short from_y, unsigned char from_z,
    unsigned char stackPos,
    unsigned short to_x, unsigned short to_y, unsigned char to_z)
{
  Thing *thing = getTile(from_x, from_y, from_z)->getThingByStackPos(stackPos);

  if (thing)
  {
    if (player->access == 0 && thing->isPlayer() && ((Player*)thing)->access != 0)
      return;

    Position oldPos;
    oldPos.x = from_x;
    oldPos.y = from_y;
    oldPos.z = from_z;

    Tile *fromTile = getTile(from_x, from_y, from_z);
    Tile *toTile   = getTile(to_x, to_y, to_z);

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
      else if (toTile->isBlocking())
      {
      }
      else
      {
        int oldstackpos = fromTile->getThingStackPos(thing);

        if (fromTile->removeThing(thing))
        {
          toTile->addThing(thing);

          thing->pos.x = to_x;
          thing->pos.y = to_y;
          thing->pos.z = to_z;

          if (thing->isPlayer())
          {
            // we need to update the direction the player is facing to...
            // otherwise we are facing some problems in turning into the
            // direction we were facing before the movement
            // check y first cuz after a diagonal move we lock to east or west
            if (to_y < oldPos.y) ((Player*)thing)->direction = NORTH;
            if (to_y > oldPos.y) ((Player*)thing)->direction = SOUTH;
            if (to_x > oldPos.x) ((Player*)thing)->direction = EAST;
            if (to_x < oldPos.x) ((Player*)thing)->direction = WEST;
          }

          CreatureVector::iterator cit;
          for (int x = min(oldPos.x, (int)to_x) - 9; x <= max(oldPos.x, (int)to_x) + 9; x++)
            for (int y = min(oldPos.y, (int)to_y) - 7; y <= max(oldPos.y, (int)to_y) + 7; y++)
            {
              Tile *tile = getTile(x, y, 7);
              if (tile)
              {
                for (cit = tile->creatures.begin(); cit != tile->creatures.end(); cit++)
                {
                  (*cit)->onThingMove(player, thing, &oldPos, oldstackpos);
                }
              }
            }
        }
      }
    }
  }
}



void Map::creatureTurn(Creature *creature, Direction dir)
{
  OTSYS_THREAD_LOCK(mapLock)

    if (creature->direction != dir)
    {
      creature->direction = dir;

      int stackpos = getTile(creature->pos.x, creature->pos.y, creature->pos.z)->getThingStackPos(creature);

      CreatureVector::iterator cit;
      for (int x = creature->pos.x - 9; x <= creature->pos.x + 9; x++)
        for (int y = creature->pos.y - 7; y <= creature->pos.y + 7; y++)
        {
          Tile *tile = getTile(x, y, 7);
          if (tile)
          {
            for (cit = tile->creatures.begin(); cit != tile->creatures.end(); cit++)
            {
              (*cit)->onCreatureTurn(creature, stackpos);
            }
          }
        }
    }

  OTSYS_THREAD_UNLOCK(mapLock)
}


void Map::playerSay(Player *player, unsigned char type, const std::string &text)
{
  OTSYS_THREAD_LOCK(mapLock)

    CreatureVector::iterator cit;

  for (int x = player->pos.x - 9; x <= player->pos.x + 9; x++)
    for (int y = player->pos.y - 7; y <= player->pos.y + 7; y++)
    {
      Tile *tile = getTile(x, y, 7);
      if (tile)
      {
        for (cit = tile->creatures.begin(); cit != tile->creatures.end(); cit++)
        {
          (*cit)->onCreatureSay(player, type, text);
        }
      }
    }

  OTSYS_THREAD_UNLOCK(mapLock)
}


void Map::playerChangeOutfit(Player *player)
{
  OTSYS_THREAD_LOCK(mapLock)

    CreatureVector::iterator cit;
  for (int x = player->pos.x - 9; x <= player->pos.x + 9; x++)
    for (int y = player->pos.y - 7; y <= player->pos.y + 7; y++)
    {
      Tile *tile = getTile(x, y, 7);
      if (tile)
      {
        for (cit = tile->creatures.begin(); cit != tile->creatures.end(); cit++)
        {
          (*cit)->onCreatureChangeOutfit(player);
        }
      }
    }

  OTSYS_THREAD_UNLOCK(mapLock)
}


void Map::playerYell(Player *player, const std::string &text)
{
  OTSYS_THREAD_LOCK(mapLock)

    CreatureVector::iterator cit;

  for (int x = player->pos.x - 18; x <= player->pos.x + 18; x++)
    for (int y = player->pos.y - 14; y <= player->pos.y + 14; y++)
    {
      Tile *tile = getTile(x, y, 7);
      if (tile)
      {
        for (cit = tile->creatures.begin(); cit != tile->creatures.end(); cit++)
        {
          (*cit)->onCreatureSay(player, 3, text);
        }
      }
    }

  OTSYS_THREAD_UNLOCK(mapLock)
}


void Map::playerSpeakTo(Player *player, const std::string &receiver, const std::string &text)
{
  OTSYS_THREAD_LOCK(mapLock)
    OTSYS_THREAD_UNLOCK(mapLock)
}


void Map::playerBroadcastMessage(Player *player, const std::string &text)
{
  OTSYS_THREAD_LOCK(mapLock)
    OTSYS_THREAD_UNLOCK(mapLock)
}

void Map::creatureMakeMeleeDamage(Creature *creature, Creature *attackedCreature)
{
  if ((std::abs(creature->pos.x-attackedCreature->pos.x) <= 1) &&
      (std::abs(creature->pos.y-attackedCreature->pos.y) <= 1) &&
      (creature->pos.z == attackedCreature->pos.z))
  {
    int damage = 1+(int)(80.0*rand()/(RAND_MAX+1.0));
    if (creature->access != 0)
      damage += 1337;

    if (damage < -50 || attackedCreature->access != 0)
      damage = 0;

    if (damage > 0)
      attackedCreature->drainHealth(damage);

    CreatureVector::iterator cit;

    NetworkMessage msg;

    Tile* targettile = getTile(attackedCreature->pos.x, attackedCreature->pos.y, attackedCreature->pos.z);

    for (int x = min(creature->pos.x, attackedCreature->pos.x) - 9; x <= max(creature->pos.x, attackedCreature->pos.x) + 9; x++)
      for (int y = min(creature->pos.y, attackedCreature->pos.y) - 7; y <= max(creature->pos.y, attackedCreature->pos.y) + 7; y++)
      {
        Tile *tile = getTile(x, y, 7);
        if (tile)
        {
          for (cit = tile->creatures.begin(); cit != tile->creatures.end(); cit++)
          {
            if ((*cit)->isPlayer())
            {
              Player *p = (Player*)(*cit);

              msg.Reset();

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
                  std::stringstream dmg;
                  dmg << damage;
                  msg.AddAnimatedText(attackedCreature->pos, 0xB4, dmg.str());
                  msg.AddMagicEffect(attackedCreature->pos, NM_ME_DRAW_BLOOD);
                  if (attackedCreature->health <= 0)
                  {
                    // remove character
                    msg.AddByte(0x6c);
                    msg.AddPosition(attackedCreature->pos);
                    msg.AddByte(targettile->getThingStackPos(attackedCreature));

                    msg.AddByte(0x6a);
                    msg.AddPosition(attackedCreature->pos);
                    msg.AddItem(&Item(2278));
                  }
                  else
                    msg.AddCreatureHealth(attackedCreature);
                }

                // own damage, update infos
                if (p == attackedCreature)
                  CreateDamageUpdate(p, creature, damage, msg);
              }

              p->sendNetworkMessage(&msg);
            }
          }
        }
      }

    if (attackedCreature->health <= 0) {
      targettile->removeThing(attackedCreature);
      playersOnline.erase(playersOnline.find(attackedCreature->getID()));
      targettile->addThing(new Item(2278));
    }
  }
}


void Map::creatureMakeDistDamage(Creature *creature, Creature *attackedCreature)
{
  if ((std::abs(creature->pos.x-attackedCreature->pos.x) <= 8) &&
      (std::abs(creature->pos.y-attackedCreature->pos.y) <= 6) &&
      (creature->pos.z == attackedCreature->pos.z))
  {
    int damage = 1+(int)(100.0*rand()/(RAND_MAX+1.0));
    if (creature->access != 0)
      damage += 1337;

    if (damage < -50 || attackedCreature->access != 0)
      damage = 0;

    if (damage > 0)
      attackedCreature->drainHealth(damage);

    CreatureVector::iterator cit;

    NetworkMessage msg;

    Tile* targettile = getTile(attackedCreature->pos.x, attackedCreature->pos.y, attackedCreature->pos.z);

    for (int x = min(creature->pos.x, attackedCreature->pos.x) - 9; x <= max(creature->pos.x, attackedCreature->pos.x) + 9; x++)
      for (int y = min(creature->pos.y, attackedCreature->pos.y) - 7; y <= max(creature->pos.y, attackedCreature->pos.y) + 7; y++)
      {
        Tile *tile = getTile(x, y, 7);
        if (tile)
        {
          for (cit = tile->creatures.begin(); cit != tile->creatures.end(); cit++)
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
                  std::stringstream dmg;
                  dmg << damage;
                  msg.AddAnimatedText(attackedCreature->pos, 0xB4, dmg.str());
                  msg.AddMagicEffect(attackedCreature->pos, NM_ME_DRAW_BLOOD);
                  if (attackedCreature->health <= 0)
                  {
                    // remove character
                    msg.AddByte(0x6c);
                    msg.AddPosition(attackedCreature->pos);
                    msg.AddByte(targettile->getThingStackPos(attackedCreature));

		    msg.AddByte(0x6a);
		    msg.AddPosition(attackedCreature->pos);
		    msg.AddItem(&Item(2278));
                  }
                  else
                    msg.AddCreatureHealth(attackedCreature);
                }

                // own damage, update infos
                if (p == attackedCreature)
                  CreateDamageUpdate(p, creature, damage, msg);
              }

              p->sendNetworkMessage(&msg);
            }
          }
        }
      }

    if (attackedCreature->health <= 0) {
      targettile->removeThing(attackedCreature);
      playersOnline.erase(playersOnline.find(attackedCreature->getID()));
      targettile->addThing(new Item(2278));
    }
  }
}


void Map::creatureMakeMagicDistDamage(Creature *creature, Creature *attackedCreature)
{
  if ((std::abs(creature->pos.x-attackedCreature->pos.x) <= 8) &&
      (std::abs(creature->pos.y-attackedCreature->pos.y) <= 6) &&
      (creature->pos.z == attackedCreature->pos.z))
  {
    int damage = 1+(int)(300.0*rand()/(RAND_MAX+1.0));
    if (creature->access != 0)
      damage += 1337;

    if (damage < -50 || attackedCreature->access != 0)
      damage = 0;

    if (damage > 0)
      attackedCreature->drainHealth(damage);

    CreatureVector::iterator cit;

    NetworkMessage msg;

    Tile* targettile = getTile(attackedCreature->pos.x, attackedCreature->pos.y, attackedCreature->pos.z);

    for (int x = min(creature->pos.x, attackedCreature->pos.x) - 9; x <= max(creature->pos.x, attackedCreature->pos.x) + 9; x++)
      for (int y = min(creature->pos.y, attackedCreature->pos.y) - 7; y <= max(creature->pos.y, attackedCreature->pos.y) + 7; y++)
      {
        Tile *tile = getTile(x, y, 7);
        if (tile)
        {
          for (cit = tile->creatures.begin(); cit != tile->creatures.end(); cit++)
          {
            if ((*cit)->isPlayer())
            {
              Player *p = (Player*)(*cit);

              msg.Reset();

              msg.AddDistanceShoot(creature->pos, attackedCreature->pos, 4);

              if (p->CanSee(attackedCreature->pos.x, attackedCreature->pos.y))
              {
                std::stringstream dmg;
                dmg << damage;
                msg.AddMagicEffect(attackedCreature->pos, NM_ME_ENERGY_DAMAGE);
                msg.AddAnimatedText(attackedCreature->pos, 0xB4, dmg.str());
                msg.AddMagicEffect(attackedCreature->pos, NM_ME_DRAW_BLOOD);

                if (attackedCreature->health <= 0)
                {
                  // remove character
                  msg.AddByte(0x6c);
                  msg.AddPosition(attackedCreature->pos);
                  msg.AddByte(targettile->getThingStackPos(attackedCreature));

                  msg.AddByte(0x6a);
                  msg.AddPosition(attackedCreature->pos);
                  msg.AddItem(&Item(2278));
                }
                else
                  msg.AddCreatureHealth(attackedCreature);
              }

              // own damage, update infos
              if (p == attackedCreature)
                CreateDamageUpdate(p, creature, damage, msg);

              p->sendNetworkMessage(&msg);
            }
          }
        }
      }

    if (attackedCreature->health <= 0) {
      targettile->removeThing(attackedCreature);
      playersOnline.erase(playersOnline.find(attackedCreature->getID()));
      targettile->addThing(new Item(2278));
    }
  }
}


void Map::checkPlayer(unsigned long id)
{
  OTSYS_THREAD_LOCK(mapLock)

    Creature *creature = getCreatureByID(id);

  if (creature != NULL)
  {

    addEvent(makeTask(1000, std::bind2nd(std::mem_fun(&Map::checkPlayer), id)));
  }

  OTSYS_THREAD_UNLOCK(mapLock)
}


void Map::checkPlayerAttacking(unsigned long id)
{
  OTSYS_THREAD_LOCK(mapLock)

    Player *player = (Player*)getCreatureByID(id);

  if (player != NULL && player->health > 0)
  {
    if (player->attackedCreature != 0)
    {
      Creature *attackedCreature = (Player*)getCreatureByID(player->attackedCreature);

      if (attackedCreature != NULL && attackedCreature->health > 0)
      {
        switch (player->getFightType())
        {
          case FIGHT_MELEE:
            creatureMakeMeleeDamage(player, attackedCreature);
            break;

          case FIGHT_DIST:
            creatureMakeDistDamage(player, attackedCreature);
            break;

          case FIGHT_MAGICDIST:
            creatureMakeMagicDistDamage(player, attackedCreature);
            break;
        }
      }
    }

    addEvent(makeTask(2000, std::bind2nd(std::mem_fun(&Map::checkPlayerAttacking), id)));
  }

  OTSYS_THREAD_UNLOCK(mapLock)
}

void Map::CreateDamageUpdate(Player* player, Creature* attackCreature, int damage, NetworkMessage& msg) {
  msg.AddPlayerStats(player);
  std::stringstream dmgmesg;
  dmgmesg << "You lose " << damage << " hitpoints due to an attack by ";
  dmgmesg << attackCreature->getName();
  msg.AddTextMessage(MSG_EVENT, dmgmesg.str().c_str());
  if (player->health <= 0)
    msg.AddTextMessage(MSG_EVENT, "Own3d!");
}


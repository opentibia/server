//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Binary file implementation of the Map Loader/Saver
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

#include "iomapbin.h"
#include "spawn.h"

#include <iostream>
#include <stdlib.h>

extern Game g_game;

bool IOMapBin::loadMap(Map* map, std::string identifier){
  fh = fopen(identifier.c_str() ,"rb"); 

  char str[4];
  str[0] = (char)fgetc(fh);
  str[1] = (char)fgetc(fh);
  str[2] = (char)fgetc(fh);
  str[3] = '\0';
  
  if (strcmp(str, "JXB\0") == 0) {
    loadJXB3(map);
  } else if (strcmp(str, "OTM\0") == 0) {
    loadOTM(map);
  } else {
    std::cout << "ERROR: Not a JXB3 or OTM map!!!" << std::endl;
    fclose(fh); 
    exit(1);
  }

  return true;
}

void IOMapBin::loadJXB3(Map* map){
  char str[2];
  str[0] = (char)fgetc(fh);
  str[1] = '\0';
  
  if (strcmp(str, "3\0") != 0) {
    std::cout << "ERROR: This isn't a JXB3 map file!!!" << std::endl;
    fclose(fh);
    exit(1);
  }

  std::cout << ":: File type detected: JXB v3 (JXB3)" << std::endl;

  int width = fgetc(fh);
  width += fgetc(fh) << 8;
  int height = fgetc(fh);
  height += fgetc(fh) << 8;
  
  std::cout << ":: Map dimensions: " << width << "x" << height << std::endl;
  
  map->mapwidth = width;
  map->mapheight = height;
  
  int ax, ay, az, id, count, tmp, itemcount;
  unsigned short gid;
  Tile *t;
 
  while (true) {
    ax = fgetc(fh);
    ax += fgetc(fh) << 8;    
    ay = fgetc(fh);
    ay += fgetc(fh) << 8;    
    az = fgetc(fh);
    
    if (ax == 0xffff && ay == 0xffff && az == 0xff) {
      fclose(fh);
      return;
    }
    
    gid = fgetc(fh);
    gid += fgetc(fh) << 8;
    
    map->setTile( ax, ay, az, gid + 100);
    t = map->getTile( ax, ay, az);
    
    tmp = fgetc(fh);
    if (tmp == 1) { t->setPz(); }
    
    itemcount = fgetc(fh);
    for (tmp = 0; tmp < itemcount; tmp++) {
      id = fgetc(fh);
      id += fgetc(fh) << 8;
      count = fgetc(fh);
      
      Item *i = Item::CreateItem(id + 100);
      if (i->isStackable())
        i->setItemCountOrSubtype(count);
      
      i->pos.x = ax;
      i->pos.y = ay;
      i->pos.z = az;
			
      if (i->isAlwaysOnTop()) {
        t->topItems.push_back(i);
      } else { 
        t->downItems.push_back(i);
      }
    }
  }
}

void IOMapBin::loadOTM(Map* map){
  std::cout << ":: File type detected: OpenTibia Map (OTM)" << std::endl;
  int cx, cy, i, j, op, len, width, height, tmp, itemcount, count, total;
  do {
    op = fgetc(fh);
    switch(op) {
      case 0x10: 
           // just bypass the info
           // TODO: modify the class Status and Map to store this info there
           len = fgetc(fh);
           for (i = 0; i < len; len--) { fgetc(fh); }
           len = fgetc(fh);
           for (i = 0; i < len; len--) { fgetc(fh); }
           break; 
      case 0x20:
           width = fgetc(fh);
           width += fgetc(fh) << 8;
           height = fgetc(fh);
           height += fgetc(fh) << 8;

           std::cout << ":: Map dimensions: " << width << "x" << height << std::endl;

           map->mapwidth = width;
           map->mapheight = height;
           break; 
      case 0x30:
           // TODO: use the temple point and radius
           fgetc(fh); fgetc(fh); // X
           fgetc(fh); fgetc(fh); // Y
           fgetc(fh);            // Z
           fgetc(fh);            // Radius
           break; 
      case 0x40:
           int ax, ay, az, id, gid; 
           Tile *t;
           while (true) {
             ax = fgetc(fh);
             ax += fgetc(fh) << 8;    
             ay = fgetc(fh);
             ay += fgetc(fh) << 8;    
             az = fgetc(fh);

             if (ax == 0xffff && ay == 0xffff && az == 0xff) 
               break;
             
             gid = fgetc(fh);
             gid += fgetc(fh) << 8;
             
             map->setTile( ax, ay, az, gid + 100);
             t = map->getTile( ax, ay, az);
    
             tmp = fgetc(fh);
             if (tmp == 1) { t->setPz(); }

             fgetc(fh); // Action ID
             fgetc(fh); // Unique ID

             itemcount = fgetc(fh);
             for (tmp = 0; tmp < itemcount; itemcount--) {
               id = fgetc(fh);
               id += fgetc(fh) << 8;
               count = fgetc(fh);
  
               fgetc(fh); // Action ID
               fgetc(fh); // Unique ID
      
               Item *i = Item::CreateItem(id + 100);
               if (i->isStackable())
                 i->setItemCountOrSubtype(count);
         
               i->pos.x = ax;
               i->pos.y = ay;
               i->pos.z = az;
               
               if (id == 1287) {
                 fgetc(fh); fgetc(fh);
                 fgetc(fh); fgetc(fh);
                 fgetc(fh);
               }
			
               if (i->isAlwaysOnTop()) {
                 t->topItems.push_back(i);
               } else { 
                 t->downItems.push_back(i);
               }
             }
           }
           break; 
      case 0x50:
           SpawnManager::initialize(&g_game);
           total = 0;
           
           int num = fgetc(fh);
           num += fgetc(fh) << 8;
           
           Position pos;
           int radius, len;
           long int secs;
           std::string cname;

           for (i = 0; i < num; num--) {
             len = fgetc(fh);
             cname = "";

             for (j = 0; j < len; len --) {
               cname.push_back(fgetc(fh)); // get the creature name
             }
             std::cout << cname.c_str() << std::endl;

             pos.x = fgetc(fh); 
             pos.x += fgetc(fh) << 8;
             pos.y = fgetc(fh); 
             pos.y += fgetc(fh) << 8;
             pos.z = fgetc(fh); 
             radius = fgetc(fh) + 1;
             
             count = fgetc(fh);
             total += count;
             secs = fgetc(fh);
             secs += fgetc(fh) << 8; 
             
             Spawn *spawn = new Spawn(&g_game, pos, radius);
             SpawnManager::instance()->spawns.push_back(spawn);
             
             for (j = 0; j < count; count--){
               cx = (rand() % (radius * 2)) - radius;
               cy = (rand() % (radius * 2)) - radius;
               
               spawn->addMonster(cname, cx, cy, secs * 1000);
             }
             
             // the checker is not needed on the CVS, since
             // it checks automatically if creatures are near
             fgetc(fh); // 1 = check for players near, 0 = dont check
           }
           
           std::cout << ":: Creatures loaded in spawns: " << total << std::endl;
           SpawnManager::instance()->startup();
           break; 
    }  
  } while (!feof(fh) || op == 0xF0 );
  
  fclose(fh);
  return;
}

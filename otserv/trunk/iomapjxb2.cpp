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

#include "iomapjxb2.h"

#include <iostream>

bool IOMapJXB2::loadMap(Map* map, std::string identifier){
  FILE* f = fopen(identifier.c_str() ,"rb"); 

  char str[5];
  str[0] = (char)fgetc(f);
  str[1] = (char)fgetc(f);
  str[2] = (char)fgetc(f);
  str[3] = (char)fgetc(f);
  str[4] = '\0';
  
  if (strcmp(str, "JXB2\0") != 0) {
    std::cout << "ERROR: Not a JXB2 map!!!" << std::endl;
    fclose(f); 
    exit(1);
  } else { std::cout << ":: Reading tiles from the file..." << std::endl; }

  int width = fgetc(f);
  width += fgetc(f) << 8;
  int height = fgetc(f);
  height += fgetc(f) << 8;
  
  std::cout << ":: Map dimensions: " << width << "x" << height << std::endl;
  
  map->mapwidth = width;
  map->mapheight = height;
  
  int ax, ay, az, id, count, tmp, npclen, letter, itemcount;
  unsigned short gid;
  Tile *t;
 
  while (true) {
    ax = fgetc(f);
    ax += fgetc(f) << 8;    
    ay = fgetc(f);
    ay += fgetc(f) << 8;    
    az = fgetc(f);
    
    if (ax == 0xffff && ay == 0xffff && az == 0xff) {
      fclose(f);
      return false;
    }
    
    gid = fgetc(f);
    gid += fgetc(f) << 8;
    
    map->setTile( ax, ay, az, gid);
    t = map->getTile( ax, ay, az);
    
    tmp = fgetc(f);
    if (tmp == 1) { t->setPz(); }
    
    // UPDATED ON 9.JUN.2005: 
    //   spawns will be moved into a new file,
    //   now they are disabled
    
    // *** NPC CODE STARTS HERE ***
    // TODO: NPC reg, 2 next bytes = size of the npc name
    // if size <> 0, read 1 byte (1 = respawn, 0 = norespawn)
    // and read each letter, 1 byte  for each one
    // just bypass, dont parse now
    npclen = fgetc(f);
    if (npclen > 0) {
      // ttr means time to respawn, in case of a simple boolean respawn id
      // now we have a 2-bytes var to give, in secs, the time to respawn
      /* ttr = */ fgetc(f);
      /* ttr = ttr + */ fgetc(f); /* << 8 */
      for (tmp = 0; tmp < npclen; tmp++) {
        // TODO: convert from simple byte to letter type
        /*letter = */fgetc(f);
      }
      // TODO: convert the final buffer to str and put a new npc at th tile
      // *** NPC CODE ENDS HERE ***
    }  
    
    itemcount = fgetc(f);
    for (tmp = 0; tmp < itemcount; tmp++) {
      id = fgetc(f);
      id += fgetc(f) << 8;
      
      count = fgetc(f);

      /* action script bypass, means the length of the action string */ fgetc(f);
      
      Item *i = Item::CreateItem(id + 100);
      if (i->isStackable())
        i->setItemCountOrSubtype(count);
      
      if (id == 1287) {
        // TODO: implement teleport, depends of the kind of tp
        // of the current OTserv, use this only with the JXB's
        // "official" one, aka, "fool's tp" by some noobs ;P
        // just bypass, dont parse now
        /*ax =*/ fgetc(f);
        /*ax +=*/ fgetc(f)/* << 8*/;
        /*ay = */fgetc(f);
        /*ay +=*/ fgetc(f)/* << 8*/;
        /*az = */fgetc(f);
      }

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
  
  return true;
}

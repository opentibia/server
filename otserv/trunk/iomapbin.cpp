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

  if (strcmp(str, "OTM\0") != 0) {
    std::cout << "ERROR: Not a OpenTibia Map file (wrong header)!!!" << std::endl;
    fclose(fh); 
    exit(1);
  }

  if (fgetc(fh) != 1) {
    std::cout << "ERROR: Wrong byte operator, revision byte expected!!! (old map revision?)" << std::endl;
    fclose(fh); 
    exit(1);
  }

  int revmajor, revminor, revnum;
  revmajor = fgetc(fh);
  revminor = fgetc(fh);
  revnum = fgetc(fh);
  revnum += fgetc(fh) << 8;

  if (revmajor != 1) {
    std::cout << "ERROR: Wrong map revision for this OTM file (might be 1.x.x)!!!" << std::endl;
    fclose(fh); 
    exit(1);
  }

  if (revminor > 1 || revnum > 2) {
    std::cout << "WARNING: This is a newer map revision format. Check for the lastest editors to" << std::endl << "   get the new functions" << std::endl;
  }

  loadOTM(map);
  return true;
}

void IOMapBin::loadOTM(Map* map){
  std::cout << ":: File type detected: OpenTibia Map (OTM)" << std::endl;
  int ax, ay, az, cx, cy, i, j, op, len, width, height, tmp, itemcount, count, total;
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
        ax = fgetc(fh);  // X
        ax += fgetc(fh); // X
        ay = fgetc(fh);  // Y
        ay += fgetc(fh); // Y
        az = fgetc(fh);  // Z
        tmp = fgetc(fh); // Radius
        std::cout << ":: Temple position is X(" << ax << ") Y(" << ay <<
           ") Z(" << az << ") with radius " << tmp << " (NOT IN USE!!!)" << std::endl;
      break; 
      case 0x40:
        total = 0;   
        int id, gid; 
        Tile *t;
        char* str;
        while (true) {
          ax = fgetc(fh);
          ax += fgetc(fh) << 8;    
          ay = fgetc(fh);
          ay += fgetc(fh) << 8;    
          az = fgetc(fh);
          
          if (ax == 0xffff && ay == 0xffff && az == 0xff) 
           break;
           
          total += 1; 
             
          gid = fgetc(fh);
          gid += fgetc(fh) << 8;
          
          map->setTile( ax, ay, az, gid + 100);
          t = map->getTile( ax, ay, az);
    
          tmp = fgetc(fh);
          if (tmp == 1) { t->setPz(); }

          do {
            op = fgetc(fh);  
            switch (op) {
              /* 
              TODO: all IDs (actions, unique and target)
              case 0x10: 
              case 0x20:
              case 0x30:
                fgetc(fh); (2) 
                fgetc(fh); 
                fgetc(fh);
              break;
              */
              case 0xA0:
                itemcount = fgetc(fh);
                for (tmp = 0; tmp < itemcount; itemcount--) {
                  id = fgetc(fh);
                  id += fgetc(fh) << 8;
                  count = 1;
                          
                  do {
                    op = fgetc(fh);
                    switch (op) {
                      case 0x10:
                        fgetc(fh);                
                        count = fgetc(fh);
                      break;                 
                      /*
                      TODO: all IDs (actions, unique and target)
                      case 0x20: 
                      case 0x30:
                      case 0x40:
                        fgetc(fh); (2)
                        fgetc(fh); 
                        fgetc(fh);
                      break;       
                      TODO: teleport destination, mandatory for 1287
                      case 0x70:
                        fgetc(fh); (5)
                        + 5 bytes, XX YY Z   
                      break;
                      TODO: fluid kind, mandatory for all fluids (0x9)
                      case 0x80:
                        fgetc(fh);
                        fgetc(fh); Fluid kind
                      break;                      
                      */     
                      case 0xFF:
                      break;
                      default:
                        len = fgetc(fh);
                        for (i = 0; i < len; i++) { fgetc(fh); }       
                      break;
                    }                             
                  } while (op < 0xFF);
      
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
              break;                    
              case 0xFF:
              break;      
              default: /* for unkown/newer operators */
                len = fgetc(fh);
                for (i = 0; i < len; i++) { fgetc(fh); }  
              break;
            }
          } while (op < 0xFF);
             
        }
        
        std::cout << ":: Total of tiles loaded is " << total << std::endl;
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
          SpawnManager::instance()->addSpawn(spawn);
             
          for (j = 0; j < count; count--){
            cx = (rand() % (radius * 2)) - radius;
            cy = (rand() % (radius * 2)) - radius;
              
            spawn->addMonster(cname, NORTH, cx, cy, secs * 1000);
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

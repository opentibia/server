#include "iomapbin.h"
#include "spawn.h"

#include <iostream>
#include <stdlib.h>

extern Game g_game;

bool IOMapBin::loadMap(Map* map, std::string identifier)
{
	fh = fopen(identifier.c_str() ,"rb"); 
	if (!fh)
	{
		std::cout << "ERROR: Failed to open " <<  identifier.c_str() << "!" << std::endl;
		exit(1);
	}

	char str[4] = "";
	str[0] = (char)fgetc(fh);
	str[1] = (char)fgetc(fh);
	str[2] = (char)fgetc(fh);
	str[3] = '\0';

	if (strcmp(str, "OTM") != 0)
	{
		std::cout << str << std::endl;
		std::cout << "ERROR: Not a OpenTibia Map file (wrong header)!!!" << std::endl;
		fclose(fh); 
		exit(1);
	}

	if (fgetc(fh) != 1)
	{
		std::cout << "ERROR: Wrong byte operator, revision byte expected!!! (old map revision?)" << std::endl;
		fclose(fh); 
		exit(1);
	}

	int revmajor, revminor, revnum;
	revmajor = fgetc(fh);
	revminor = fgetc(fh);
	revnum = fgetc(fh); revnum+=fgetc(fh)<<8;

	if (revmajor != 1)
	{
		std::cout << "ERROR: Wrong map revision for this OTM file (might be 1.x.x)!!!" << std::endl;
		fclose(fh); 
		exit(1);
	}

	if (revminor > 1 || revnum > 2)
		std::cout << "WARNING: This is a newer map revision format. Check for the lastest editors to get the new functions" << std::endl;
	
	std::cout << ":: Loaded map OTM: " << identifier.c_str() << std::endl;
	loadOTM(map);

	return true;
}

void IOMapBin::loadOTM(Map* map)
{
  int op;
  bool end = false;
  while(!feof(fh) || !end)
  {
		op = fgetc(fh);
		switch(op) 
		{
			case 0x10: // Information of the map
			{
				char name[100], author[100];
				int pos;
				int len;

				// Map Name
				len = fgetc(fh);
				for (pos = 0; pos < len; pos++)
					name[pos] = fgetc(fh);
				name[pos] = '\0';
				std::cout << ":: Map Name: " << name << std::endl;
				
				// Map Author
				len = fgetc(fh);
				for (pos = 0; pos < len; pos++)
					author[pos] = fgetc(fh);
				author[pos] = '\0';
				std::cout << ":: Map Author: " << author << std::endl;
				
				
			} break;
			case 0x20: // Map dimensions
			{
				int width, height;
				width = fgetc(fh);
				width += fgetc(fh)<<8;
				height = fgetc(fh);
				height += fgetc(fh)<<8;
				map->mapwidth = width;
				map->mapheight = height;
				std::cout << ":: Map dimensions: " << width << "x" << height << std::endl;
			} break; 
			case 0x30: // Global Temple Position
			{
				PositionEx templePos;

				templePos.x = fgetc(fh);
				templePos.x += fgetc(fh);	// X
				templePos.y = fgetc(fh);
				templePos.y += fgetc(fh);	// Y
				templePos.z = fgetc(fh);	// Z
				int radius = fgetc(fh);						// Radius

				// TODO: use the temple point and radius
				std::cout << ":: Global Temple Position: " << templePos.x << " " << templePos.y << " " << templePos.z << " Radius: " << radius << std::endl;
			} break; 
			case 0x40: // Tiles and items
			{
				Tile *t;
				int x, y, z, id, total = 0;
				while(true)
				{
					// tile pos
					x = fgetc(fh); x += fgetc(fh) << 8;    
					y = fgetc(fh); y += fgetc(fh) << 8;    
					z = fgetc(fh);
					
					// end the loop
					if (x == 0xFFFF && y == 0xFFFF && z == 0xFF) 
						break;
		           			             
					id = fgetc(fh) + 100; 
					id += fgetc(fh) << 8; 
			        total += 1;
			        
					map->setTile(x, y, z, id);
					t = map->getTile(x, y, z);
					
					// check if the tile is pz
					if (fgetc(fh) == 1)
						t->setPz();
					
					int op2;
					int tmpid;
					do 
					{
						op2 = fgetc(fh);  
						switch (op2)
						{
							case 0x10: // Action Id
								fgetc(fh); // len
								tmpid = fgetc(fh);
								tmpid += fgetc(fh) << 8;
								// t->ground->setActionId(tmpid);
								break;
							case 0x20: // Unique Id
								fgetc(fh); // len
								tmpid = fgetc(fh);
								tmpid += fgetc(fh) << 8;
								//t ->ground->setUniqueId(tmpid);
								break; 
							case 0x30: // Target Id
								fgetc(fh); // len
								tmpid = fgetc(fh);
								tmpid += fgetc(fh) << 8;
								// TODO: implement target ids
								break; 
							case 0xA0: // Item
							{
								int itemcount = fgetc(fh);
								for (int count = 0; count < itemcount; count++)
								{
									int itemid = fgetc(fh) + 100;
									itemid += fgetc(fh) << 8;
									
									Item *item = Item::CreateItem(itemid);
									int op3;
									do
									{
										op3 = fgetc(fh);
										switch (op3)
										{
											case 0x10: // Count
												fgetc(fh); //len
									   			item->setItemCountOrSubtype((unsigned char)fgetc(fh));
												break;                 
											case 0x20: // Action Id
												fgetc(fh); //len
												tmpid = fgetc(fh);
												tmpid += fgetc(fh) << 8;
												// item->setActionId(tmpid);
												break; 
											case 0x30: // Unique Id
												fgetc(fh); //len
												tmpid = fgetc(fh);
												tmpid += fgetc(fh) << 8;
												// item->setUniqueId(tmpid);
												break; 
											case 0x40: // Target Id
												fgetc(fh); //len
												tmpid = fgetc(fh);
												tmpid += fgetc(fh) << 8;
												// item->setTargetId(tmpid);
												break; 
											case 0x70: //Teleport
											{
												Teleport *tele = dynamic_cast<Teleport*>(item);
												Position toPos;
												fgetc(fh); //len

												toPos.x = fgetc(fh); toPos.x += fgetc(fh)<<8;
												toPos.y = fgetc(fh); toPos.y += fgetc(fh)<<8;
												toPos.z = fgetc(fh);

												if (tele)
													tele->setDestPos(toPos);
											} break;
											case 0x80: // Fluids
												fgetc(fh);
												if (item->isFluidContainer())
													item->setItemCountOrSubtype((unsigned char)fgetc(fh));
												else
													fgetc(fh);
											break;                      
											case 0xFF: // End
												break;
											default: // Unknow/New operators
											{
												printf("WARNING: Unknown operator loading items: 0x%X!\n",op3);
												int len = fgetc(fh);
												for (int i = 0; i < len; i++)
													fgetc(fh);     
											} break;
										}                             
									} while (op3 < 0xFF);
									
									item->pos.x = x;
									item->pos.y = y;
									item->pos.z = z;

									if (item->isAlwaysOnTop())
										t->topItems.push_back(item);
									else
										t->downItems.push_back(item);
								}
							} break;
							case 0xFF: // End
								break;
							default: // Unknow/New operators
							{
								printf("WARNING: Unknown operator loading tiles: 0x%X!\n",op2);
								int len = fgetc(fh);
								for (int i = 0;i < len; i++)
									fgetc(fh);  
							} break;
						}
					} while (op2 < 0xFF);
				}
				std::cout << ":: Total of tiles loaded is " << total << std::endl;
			} break;
			case 0x50: // Spawns
			{
				SpawnManager::initialize(&g_game);
				Position pos;
				int cx, cy, radius, total=0;
				long int secs;
				std::string cname;
				int num = fgetc(fh); num+=fgetc(fh)<<8;

				for (int i = 0; i < num; i++)
				{
					int len = fgetc(fh);
					int count;
					cname = "";

					for (int j = 0;j < len; j++)
						cname.push_back(fgetc(fh)); // get the creature name

					//std::cout << cname.c_str() << std::endl;

					pos.x = fgetc(fh); pos.x += fgetc(fh) << 8;
					pos.y = fgetc(fh); pos.y += fgetc(fh) << 8;
					pos.z = fgetc(fh); 
					radius = fgetc(fh) + 1;
			             
					count = fgetc(fh); // number of creatures in this respawn
					total += count;
					secs = fgetc(fh); secs += fgetc(fh) << 8;
			             
					Spawn *spawn = new Spawn(&g_game, pos, radius);
					SpawnManager::instance()->addSpawn(spawn);
		             
					for (int j = 0; j < count; j++)
					{
						cx = (rand() % (radius * 2)) - radius;
						cy = (rand() % (radius * 2)) - radius;
						spawn->addMonster(cname, NORTH, cx, cy, secs * 1000);
					}

					fgetc(fh); // 1 = check for players near, 0 = dont check
				}
		           
				std::cout << ":: Loaded spawns: " << total << std::endl;
				SpawnManager::instance()->startup();
			} break;
			case 0xF0:
				end = true;
				break;
		}
  }
  fclose(fh);
  return;
}

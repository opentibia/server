#ifndef __IOMAPSQL_H
#define __IOMAPSQL_H

#include <string>

#include "iomap.h"

class Map;

/** Map-Loader implementation based on SQL format */
class IOMapSQL : public IOMap {
  public:
	IOMapSQL(){};
	~IOMapSQL(){};
	std::string house;
	virtual char* getSourceDescription(){ return "SQL (SQL based Map)"; };
	bool loadMap(Map* map, std::string identifier);
    bool SaveMap(Map* map, std::string identifier, int x, int y, int z);
  //private:
};

#endif

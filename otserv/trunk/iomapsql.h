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

	virtual char* getSourceDescription(){ return "SQL"; };
	virtual bool loadMap(Map* map, const std::string& identifier);
	virtual bool loadSpawns();
	virtual bool loadHouses();
};

#endif

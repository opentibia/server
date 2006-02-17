//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Base class for the Map Loader/Saver
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


#ifndef __IOMAP_H
#define __IOMAP_H

#include <string>

#include "map.h"

enum LoadMapError_t{
	LOADMAPERROR_NONE,
	LOADMAPERROR_CANNOTOPENFILE,
	LOADMAPERROR_GETPROPFAILED,
	LOADMAPERROR_OUTDATEDHEADER,
	LOADMAPERROR_GETROOTHEADERFAILED,
	LOADMAPERROR_FAILEDTOCREATEITEM,
	LOADMAPERROR_FAILEDUNSERIALIZEITEM,
	LOADMAPERROR_FAILEDTOREADCHILD,
	LOADMAPERROR_UNKNOWNNODETYPE
};

class IOMap{
public:
	IOMap(){};
	virtual ~IOMap(){};

	/** Get a textual description of what source is used
	* \returns Name of the source
	*/
	virtual char* getSourceDescription() = 0;

	/** Load the map from an OTBM file
	  * \param map pointer to the Map class
	  * \param identifier is the mapfile/database to open
	  * \returns Returns true if the map was loaded successfully
	*/
	virtual bool loadMap(Map* map, const std::string& identifier) = 0;

	virtual bool loadSpawns() = 0;
	virtual bool loadHouses() = 0;

	LoadMapError_t getLastError() {return lasterrortype;}
	int getErrorCode() {return lasterrorcode;}

	void setLastError(LoadMapError_t errtype, unsigned long _code = 0)
	{
		lasterrorcode = _code;
		lasterrortype = errtype;
	}

private:
	LoadMapError_t lasterrortype;
	unsigned long lasterrorcode;
};

#endif

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

#ifndef __OTSERV_IOMAP_H__
#define __OTSERV_IOMAP_H__

#include "map.h"
#include "spawn.h"
#include "house.h"

class IOMap{
public:
	IOMap() {}
	virtual ~IOMap() {}

	/** Get a textual description of what source is used
	* \return Name of the source
	*/
	virtual const char* getSourceDescription() = 0;

	/** Load the map from a file/database
	  * \param map pointer to the Map class
	  * \param identifier is the mapfile/database to open
	  * \return Returns true if the map was loaded successfully
	*/
	virtual bool loadMap(Map* map, const std::string& identifier) = 0;


	/** Load the spawns
	  * \param map pointer to the Map class
	  * \return Returns true if the spawns were loaded successfully
	*/
	bool loadSpawns(Map* map)
	{
		if(map->spawnfile.empty()){
			return true;
		}
		
		return Spawns::getInstance()->loadFromXml(map->spawnfile);
	}

	/** Load the houses (not house tile-data)
	  * \param map pointer to the Map class
	  * \return Returns true if the houses were loaded successfully
	*/
	bool loadHouses(Map* map)
	{
		if(!map->housefile.empty()){
			return Houses::getInstance().loadHousesXML(map->housefile);
		}
		return true;
	}

	const std::string& getLastErrorString() const
	{
		return errorString;
	}

	void setLastErrorString(const std::string& _errorString)
	{
		errorString = _errorString;
	}

protected:
	std::string errorString;
};

#endif

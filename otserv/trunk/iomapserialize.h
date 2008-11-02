//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Base class for the map serialization
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


#ifndef __OTSERV_IOMAPSERIALIZE_H__
#define __OTSERV_IOMAPSERIALIZE_H__

class Map;

#include "iomapserialize.h"
#include "database.h"
#include "map.h"

#include <string>

class IOMapSerialize{
public:
	static IOMapSerialize* getInstance()
	{
		static IOMapSerialize instance;
		return &instance;
	}

	IOMapSerialize() {}
	~IOMapSerialize() {}

	/** Load the map from a file/database
	  * \param map pointer to the Map class
	  * \return Returns true if the map was loaded successfully
	*/
	bool loadMap(Map* map);

	/** Save the map to a file/database
	  * \param map pointer to the Map class
	  * \return Returns true if the map was saved successfully
	*/
	bool saveMap(Map* map);

	/** Load the house access list to a file/database
	  * \param map pointer to the Map class
	  * \return Returns true if the house access list was opened successfully
	*/
	bool loadHouseInfo(Map* map);

	/** Save the house access list to a file/database
	  * \param map pointer to the Map class
	  * \return Returns true if the house access list was saved successfully
	*/
	bool saveHouseInfo(Map* map);

protected:
	bool saveTile(Database* db, uint32_t tileId, const Tile* tile);
	bool loadTile(Database& db, Tile* tile);
};

#endif

//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// SQL map serialization
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


#ifndef __IOMAPSERIALIZESQL_H__
#define __IOMAPSERIALIZESQL_H__

#include "iomapserialize.h"
#include "database.h"
#include "map.h"

#include <string>

class IOMapSerializeSQL : public IOMapSerialize{
public:
	IOMapSerializeSQL();
	virtual ~IOMapSerializeSQL();

	virtual bool loadMap(Map* map, const std::string& identifier);
	virtual bool saveMap(Map* map, const std::string& identifier);
	virtual bool loadHouseInfo(Map* map, const std::string& identifier);
	virtual bool saveHouseInfo(Map* map, const std::string& identifier);

protected:
	std::string m_host;
	std::string m_user;
	std::string m_pass;
	std::string m_db;

	bool saveTile(Database& db, uint32_t tileId, const Tile* tile);
	bool loadTile(Database& db, Tile* tile);
};

#endif

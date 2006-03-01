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

#include "iomapserializesql.h"
#include "house.h"
#include "luascript.h"
#include "database.h"

extern LuaScript g_config;

IOMapSerializeSQL::IOMapSerializeSQL()
{
	m_host = g_config.getGlobalString("sql_host");
	m_user = g_config.getGlobalString("sql_user");
	m_pass = g_config.getGlobalString("sql_pass");
	//m_db   = g_config.getGlobalString("sql_db");
}

IOMapSerializeSQL::~IOMapSerializeSQL()
{
	//
}

bool IOMapSerializeSQL::loadMap(Map* map, const std::string& identifier)
{
	Database db;
	DBQuery query;
	DBResult result;

	db.connect(identifier.c_str(), m_host.c_str(), m_user.c_str(), m_pass.c_str());
	
	for(HouseMap::iterator it = Houses::getInstance().getHouseBegin(); it != Houses::getInstance().getHouseEnd(); ++it){
		DBQuery query;

		//query << "DELETE FROM tileitems WHERE tilex='"<< x << "'" AND tiley='"<< y << "'" AND tilez='"<< z << "'";
		
		if(!db.executeQuery(query))
			return false;

		//load tile
		House* house = it->second;
		for(HouseTileList::iterator it = house->getHouseTileBegin(); it != house->getHouseTileEnd(); ++it){
			loadTile(db, query, *it);
		}

		//End the transaction
		query.reset();
		query << "COMMIT;";

		if(!db.executeQuery(query))
			return false;
	}
}

bool IOMapSerializeSQL::saveMap(Map* map, const std::string& identifier)
{
	Database db;
	DBQuery query;
	DBResult result;

	db.connect(identifier.c_str(), m_host.c_str(), m_user.c_str(), m_pass.c_str());
	
	for(HouseMap::iterator it = Houses::getInstance().getHouseBegin(); it != Houses::getInstance().getHouseEnd(); ++it){
		DBQuery query;

		//Start the transaction	
		query << "BEGIN;";
		if(!db.executeQuery(query))
			return false;

		//save house items
		House* house = it->second;
		for(HouseTileList::iterator it = house->getHouseTileBegin(); it != house->getHouseTileEnd(); ++it){
			//clear old tile data
			query << "DELETE FROM tileitems WHERE x='"<< (*it)->getPosition().x
				<< "' AND y='"<< (*it)->getPosition().y
				<< "' AND z='"<< (*it)->getPosition().z << "'";
			
			if(!db.executeQuery(query))
				return false;

			saveTile(db, query, *it);
		}

		//End the transaction
		query.reset();
		query << "COMMIT;";

		if(!db.executeQuery(query))
			return false;
	}

	return true;
}

bool IOMapSerializeSQL::saveTile(Database& db, DBQuery& query, const Tile* tile)
{
	query << "INSERT INTO `tileitems` (`x`, `y` , `z` , `sid` , `pid` , `type` , `number` , `actionid` , `text` , `specialdesc` ) VALUES";

	typedef std::list<std::pair<Container*, int> > ContainerStackList;
	typedef ContainerStackList::value_type ContainerStackList_Pair;
	ContainerStackList containerStackList;

	int runningID = 0;
	Item* item = NULL;
	Container* container = NULL;

	int parentid = 0;
	std::stringstream streamitems;
	std::string itemsstring;

	for(int i = 0; i < tile->getThingCount(); ++i){
		item = tile->__getThing(i)->getItem();
		if(!item || item->isNotMoveable())
			continue;

		++runningID;

		streamitems << "(" << tile->getPosition().x << "," << tile->getPosition().y << "," << tile->getPosition().z << ","
			<< runningID << "," << parentid << ","
			<< item->getID() << "," << (int)item->getItemCountOrSubtype() << "," << (int)item->getActionId() << ",'"
			<< Database::escapeString(item->getText()) << "','" << Database::escapeString(item->getSpecialDescription()) << "'),";

		if(item->getContainer()){
			containerStackList.push_back(ContainerStackList_Pair(item->getContainer(), runningID));
		}
	}
		
	while(containerStackList.size() > 0){
		//split into sub-queries
		if(streamitems.str().length() > 8192){
			DBQuery subquery;
			subquery << query.str();

			itemsstring = streamitems.str();
			itemsstring.erase(itemsstring.length() - 1);
			subquery << itemsstring;

			if(!db.executeQuery(subquery))
				return false;

			streamitems.str("");
			itemsstring = "";
		}

		ContainerStackList_Pair csPair = containerStackList.front();
		container = csPair.first;
		parentid = csPair.second;
		containerStackList.pop_front();

		for(ItemList::const_iterator it = container->getItems(); it != container->getEnd(); ++it){
			item = (*it);
			++runningID;
			if(item->getContainer()){
				containerStackList.push_back(ContainerStackList_Pair(item->getContainer(), runningID));
			}

			streamitems << "(" << tile->getPosition().x << "," << tile->getPosition().y << "," << tile->getPosition().z << ","
				<< runningID << "," << parentid << ","
				<< item->getID() << "," << (int)item->getItemCountOrSubtype() << "," << (int)item->getActionId() << ",'"
				<< Database::escapeString(item->getText()) << "','" << Database::escapeString(item->getSpecialDescription()) << "'),";
		}
	}

	if(streamitems.str().length() > 0){
		itemsstring = streamitems.str();
		itemsstring.erase(itemsstring.length() - 1);
		query << itemsstring;
		
		if(!db.executeQuery(query))
			return false;
		
		streamitems.str("");
		itemsstring = "";
	}
}

bool IOMapSerializeSQL::loadTile(Database& db, DBQuery& query, Tile* tile)
{
	typedef OTSERV_HASH_MAP<int,std::pair<Item*,int> > ItemMap;
	ItemMap itemMap;

	DBResult result;
	query << "SELECT * FROM tileitems WHERE x='" << tile->getPosition().x
		<< "' AND y='" << tile->getPosition().y
		<< "' AND z='" << tile->getPosition().z
		<< "' ORDER BY sid ASC";

	if(!db.storeQuery(query, result) || result.getNumRows() != 1)
		return false;

	if(db.storeQuery(query, result) && (result.getNumRows() > 0)){	
		for(int i=0; i < result.getNumRows(); ++i){
			int type = result.getDataInt("type", i);
			int count = result.getDataInt("number", i);
			Item* item = Item::CreateItem(type, count);
			if(result.getDataInt("actionid", i) >= 100)
				item->setActionId(result.getDataInt("actionid", i));

			item->setText(result.getDataString("text", i));
			item->setSpecialDescription(result.getDataString("specialdesc",i));
			std::pair<Item*, int> myPair(item, result.getDataInt("pid", i));
			itemMap[result.getDataInt("sid", i)] = myPair;
			
			tile->__internalAddThing(item);
		}
	}

	ItemMap::iterator it;

	for(int i = (int)itemMap.size(); i > 0; --i){
		it = itemMap.find(i);
		if(it == itemMap.end())
			continue;

		if(int p=(*it).second.second){
			if(Container* container = itemMap[p].first->getContainer()){
				container->__internalAddThing((*it).second.first);
			}
		}
	}
}

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
#include "otpch.h"

#include "definitions.h"
#include "iomapserialize.h"

#include "house.h"
#include "configmanager.h"
#include "luascript.h"
#include "game.h"

extern ConfigManager g_config;
extern Game g_game;

IOMapSerialize* IOMapSerialize::_instance = NULL;

IOMapSerialize* IOMapSerialize::getInstance()
{
	if(!_instance){
		_instance = new IOMapSerialize;
	}

	return _instance;
}

IOMapSerialize::IOMapSerialize()
{
}

IOMapSerialize::~IOMapSerialize()
{
	//
}

bool IOMapSerialize::loadMap(Map* map, const std::string& identifier)
{
	Database* db = Database::instance();
	DBQuery query; //we need this to lock database

	if(!db->connect()){
		return false;
	}

	for(HouseMap::iterator it = Houses::getInstance().getHouseBegin(); it != Houses::getInstance().getHouseEnd(); ++it){
		//load tile
		House* house = it->second;
		for(HouseTileList::iterator it = house->getHouseTileBegin(); it != house->getHouseTileEnd(); ++it){
			loadTile(*db, *it);
		}
	}

	return true;
}

bool IOMapSerialize::saveMap(Map* map, const std::string& identifier)
{
	Database* db = Database::instance();
	DBQuery query;

	if(!db->connect()){
		return false;
	}

	//Start the transaction
	if( !db->beginTransaction() )
    {
        return false;
    }

	//clear old tile data
	query.reset();
	query << "DELETE FROM tile_items;";
	if(!db->executeQuery(query)){
		return false;
	}

	query.reset();
	query << "DELETE FROM tiles;";
	if(!db->executeQuery(query)){
		return false;
	}

	uint32_t tileId = 0;

	for(HouseMap::iterator it = Houses::getInstance().getHouseBegin(); it != Houses::getInstance().getHouseEnd(); ++it){

		//save house items
		House* house = it->second;
		for(HouseTileList::iterator it = house->getHouseTileBegin(); it != house->getHouseTileEnd(); ++it){
			++tileId;
			saveTile(db, tileId, *it);
		}
	}

	//End the transaction
	return db->commit();
}

bool IOMapSerialize::saveTile(Database* db, uint32_t tileId, const Tile* tile)
{
	typedef std::list<std::pair<Container*, int> > ContainerStackList;
	typedef ContainerStackList::value_type ContainerStackList_Pair;
	ContainerStackList containerStackList;

	bool storedTile = false;
	int runningID = 0;
	Item* item = NULL;
	Container* container = NULL;

	int parentid = 0;
	std::stringstream streamitems;
	std::string itemsstring;

	DBSplitInsert query_insert(db);
	query_insert.setQuery("INSERT INTO `tile_items` (`tile_id`, `sid` , `pid` , `itemtype` , `count`, `attributes` ) VALUES ");

	for(uint32_t i = 0; i < tile->getThingCount(); ++i){
		item = tile->__getThing(i)->getItem();

		if(!item)
			continue;

		if(!(!item->isNotMoveable() ||
			item->getDoor() ||
			(item->getContainer() && item->getContainer()->size() != 0)||
			(item->canWriteText())
			/*item->getBed()*/))
			continue;

		if(!storedTile){
            DBQuery tileListQuery;
            const Position& tilePos = tile->getPosition();
            tileListQuery << "INSERT INTO `tiles` (`id`, `x` , `y` , `z` ) VALUES";
            tileListQuery << "(" << tileId << "," << tilePos.x << "," << tilePos.y << "," << tilePos.z << ")";

            if(!db->executeQuery(tileListQuery))
                return false;

            storedTile = true;
        }
		++runningID;

		uint32_t attributesSize;

		PropWriteStream propWriteStream;
		item->serializeAttr(propWriteStream);
		const char* attributes = propWriteStream.getStream(attributesSize);

		streamitems << "(" << tileId << "," << runningID << "," << parentid << "," << item->getID() << ","
			<< (int)item->getItemCountOrSubtype() << ",'" << Database::escapeString(attributes, attributesSize) <<"')";

		if(!query_insert.addRow(streamitems.str()))
			return false;

		streamitems.str("");

		if(item->getContainer()){
			containerStackList.push_back(ContainerStackList_Pair(item->getContainer(), runningID));
		}
	}

	while(containerStackList.size() > 0){

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

			uint32_t attributesSize;

			PropWriteStream propWriteStream;
			item->serializeAttr(propWriteStream);
			const char* attributes  = propWriteStream.getStream(attributesSize);

			streamitems << "(" << tileId << "," << runningID << "," << parentid << "," << item->getID() << ","
				<< (int)item->getItemCountOrSubtype() << ",'" << Database::escapeString(attributes, attributesSize) <<"')";

			if(!query_insert.addRow(streamitems.str()))
				return false;

			streamitems.str("");
		}
	}

	if(!query_insert.executeQuery()){
		return false;
	}

	return true;
}

bool IOMapSerialize::loadTile(Database& db, Tile* tile)
{
	typedef std::map<int,std::pair<Item*,int> > ItemMap;
	ItemMap itemMap;

	const Position& tilePos = tile->getPosition();

	DBQuery query;
	query.reset();
	query << "SELECT id FROM tiles WHERE x=" << tilePos.x
		<< " AND y=" << tilePos.y
		<< " AND z=" << tilePos.z;

	DBResult result;
	if(!db.storeQuery(query, result) || result.getNumRows() != 1)
		return false;

	int tileId = result.getDataInt("id");

	query.reset();
	query << "SELECT * FROM tile_items WHERE tile_id=" << tileId <<" ORDER BY sid DESC";
	if(db.storeQuery(query, result) && (result.getNumRows() > 0)){
		Item* item = NULL;

		for(uint32_t i=0; i < result.getNumRows(); ++i){
			int sid = result.getDataInt("sid", i);
			int pid = result.getDataInt("pid", i);
			int type = result.getDataInt("itemtype", i);
			int count = result.getDataInt("count", i);
			item = NULL;

			unsigned long attrSize = 0;
			const char* attr = result.getDataBlob("attributes", attrSize, i);
			PropStream propStream;
			propStream.init(attr, attrSize);

			const ItemType& iType = Item::items[type];
			if(iType.moveable || /* or object in a container*/ pid != 0){
				//create a new item
				item = Item::CreateItem(type, count);

				if(item){
					if(!item->unserializeAttr(propStream)){
						std::cout << "WARNING: Serialize error in IOMapSerialize::loadTile()" << std::endl;
					}

					if(pid == 0){
						tile->__internalAddThing(item);
						item->__startDecaying();
					}
				}
				else
					continue;
			}
			else{
				bool isDoor = iType.isDoor();

				//find this type in the tile
				for(uint32_t i = 0; i < tile->getThingCount(); ++i){
					Item* findItem = tile->__getThing(i)->getItem();

					if(!findItem)
						continue;

					if(findItem->getID() == type){
						item = findItem;
						if(!item->unserializeAttr(propStream)){
							std::cout << "WARNING: Serialize error in IOMapSerialize::loadTile()" << std::endl;
						}

						break;
					}
					else if(isDoor && findItem->getDoor()){
						item = findItem;
						item->setID(type);
					}
				}
			}

			if(item){
				std::pair<Item*, int> myPair(item, pid);
				itemMap[sid] = myPair;
			}
			else{
				std::cout << "WARNING: IOMapSerialize::loadTile(). NULL item at " << tile->getPosition() << ". type = " << type << ", sid = " << sid << ", pid = " << pid << std::endl;
			}
		}
	}

	ItemMap::reverse_iterator it;
	ItemMap::iterator it2;

	for(it = itemMap.rbegin(); it != itemMap.rend(); ++it){
		Item* item = it->second.first;
		int pid = it->second.second;

		it2 = itemMap.find(pid);
		if(it2 != itemMap.end()){
			if(Container* container = it2->second.first->getContainer()){
				container->__internalAddThing(item);
				g_game.startDecay(item);
			}
		}
	}

	return true;
}

bool IOMapSerialize::loadHouseInfo(Map* map, const std::string& identifier)
{
	Database* db = Database::instance();
	DBQuery query;
	DBResult result;

	if(!db->connect()){
		return false;
	}

	query << "SELECT * FROM houses";
	if(!db->storeQuery(query, result) || result.getNumRows() == 0)
		return false;

	for(uint32_t i = 0; i < result.getNumRows(); ++i){
		int houseid = result.getDataInt("id", i);
		House* house = Houses::getInstance().getHouse(houseid);
		if(house){
			int ownerid = result.getDataInt("owner", i);
			int paid = result.getDataInt("paid", i);
			int payRentWarnings = result.getDataInt("warnings", i);

			house->setHouseOwner(ownerid);
			house->setPaidUntil(paid);
			house->setPayRentWarnings(payRentWarnings);
		}
	}

	for(HouseMap::iterator it = Houses::getInstance().getHouseBegin(); it != Houses::getInstance().getHouseEnd(); ++it){
		query.reset();
		House* house = it->second;
		if(house->getHouseOwner() != 0 && house->getHouseId() != 0){
			query << "SELECT listid,list FROM house_lists WHERE house_id =" << house->getHouseId();

			if(db->storeQuery(query, result) && result.getNumRows() != 0){
				for(uint32_t i = 0; i < result.getNumRows(); ++i){
					int listid = result.getDataInt("listid", i);
					std::string list = result.getDataString("list", i);
					house->setAccessList(listid, list);
				}
			}
		}
	}

	return true;
}

bool IOMapSerialize::saveHouseInfo(Map* map, const std::string& identifier)
{
	Database* db = Database::instance();
	DBQuery query;
	DBResult result;

	if(!db->connect()){
		return false;
	}

    if( !db->beginTransaction() )
        return false;

	query << "DELETE FROM houses;";
	if(!db->executeQuery(query))
		return false;

	query.reset();
	query << "DELETE FROM house_lists;";
	if(!db->executeQuery(query))
		return false;

    std::stringstream housestream;

	DBSplitInsert query_insert(db);
	query_insert.setQuery("INSERT INTO `houses` (`id` , `owner` , `paid`, `warnings`) VALUES ");

	for(HouseMap::iterator it = Houses::getInstance().getHouseBegin(); it != Houses::getInstance().getHouseEnd(); ++it){
		House* house = it->second;

		housestream << "(" << house->getHouseId() << "," << house->getHouseOwner() << "," << house->getPaidUntil() << "," << house->getPayRentWarnings() << ")";

		if(!query_insert.addRow(housestream.str()))
			return false;

		housestream.str("");
	}

	if(!query_insert.executeQuery())
		return false;

	for(HouseMap::iterator it = Houses::getInstance().getHouseBegin(); it != Houses::getInstance().getHouseEnd(); ++it){
		bool save_lists = false;
		query_insert.setQuery("INSERT INTO `house_lists` (`house_id` , `listid` , `list`) VALUES ");
		House* house = it->second;

		std::string listText;
		if(house->getAccessList(GUEST_LIST, listText) && listText != ""){
			housestream << "(" << house->getHouseId() << "," << GUEST_LIST << ",'" << Database::escapeString(listText) << "')";
			save_lists = true;

			if(!query_insert.addRow(housestream.str()))
				return false;

			housestream.str("");
		}
		if(house->getAccessList(SUBOWNER_LIST, listText) && listText != ""){
			housestream << "(" << house->getHouseId() << "," << SUBOWNER_LIST << ",'" << Database::escapeString(listText) << "')";
			save_lists = true;

			if(!query_insert.addRow(housestream.str()))
				return false;

			housestream.str("");
		}

		for(HouseDoorList::iterator it = house->getHouseDoorBegin(); it != house->getHouseDoorEnd(); ++it){
			const Door* door = *it;
			if(door->getAccessList(listText) && listText != ""){
				housestream << "(" << house->getHouseId() << "," << door->getDoorId() << ",'" << Database::escapeString(listText) << "')";
				save_lists = true;

				if(!query_insert.addRow(housestream.str()))
					return false;

				housestream.str("");
			}
		}

		if(save_lists){
			if(!query_insert.executeQuery())
				return false;
		}
	}

	return db->commit();
}

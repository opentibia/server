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
#include "game.h"

extern ConfigManager g_config;
extern Game g_game;

bool IOMapSerialize::loadMap(Map* map)
{
	int64_t start = OTSYS_TIME();
	bool s = false;

	if(g_config.getString(ConfigManager::MAP_STORAGE_TYPE) == "relational")
		s = loadMapRelational(map);
	else if(g_config.getString(ConfigManager::MAP_STORAGE_TYPE) == "binary")
		s = loadMapBinary(map);
	else
		std::cout << "[IOMapSerialize::loadMap] Unknown map storage type" << std::endl;

	std::cout << "Notice: Map load (" << g_config.getString(ConfigManager::MAP_STORAGE_TYPE) << ") took : " <<
		(OTSYS_TIME() - start)/(1000.) << " s" << std::endl;

	return s;
}

bool IOMapSerialize::saveMap(Map* map)
{
	
	int64_t start = OTSYS_TIME();
	bool s = false;

	if(g_config.getString(ConfigManager::MAP_STORAGE_TYPE) == "relational")
		s = saveMapRelational(map);
	else if(g_config.getString(ConfigManager::MAP_STORAGE_TYPE) == "binary")
		s = saveMapBinary(map);
	else
		std::cout << "[IOMapSerialize::saveMap] Unknown map storage type" << std::endl;

	std::cout << "Notice: Map save (" << g_config.getString(ConfigManager::MAP_STORAGE_TYPE) << ") took : " <<
		(OTSYS_TIME() - start)/(1000.) << " s" << std::endl;

	return s;
}

bool IOMapSerialize::loadMapRelational(Map* map)
{
	Database* db = Database::instance();
	DBQuery query;

	for(HouseMap::iterator it = Houses::getInstance().getHouseBegin(); it != Houses::getInstance().getHouseEnd(); ++it){
		//load tile
		House* house = it->second;
		for(HouseTileList::iterator it = house->getHouseTileBegin(); it != house->getHouseTileEnd(); ++it){
			loadTile(*db, *it);
		}
	}

	return true;
}

bool IOMapSerialize::loadTile(Database& db, Tile* tile)
{
	typedef std::map<int,std::pair<Item*,int> > ItemMap;
	ItemMap itemMap;

	const Position& tilePos = tile->getPosition();

	DBQuery query;

	query << "SELECT `id` FROM `tiles` WHERE `x` = " << tilePos.x
	<< " AND `y` = " << tilePos.y
	<< " AND `z` = " << tilePos.z;

	DBResult* result = db.storeQuery(query.str());
	if(!result)
		return false;

	int tileId = result->getDataInt("id");
	db.freeResult(result);

	query.str("");
	query << "SELECT * FROM `tile_items` WHERE `tile_id` = "
	<< tileId
	<< " ORDER BY `sid` DESC";
	if((result = db.storeQuery(query.str()))){
		Item* item = NULL;

		do{
			int sid = result->getDataInt("sid");
			int pid = result->getDataInt("pid");
			int type = result->getDataInt("itemtype");
			int count = result->getDataInt("count");
			item = NULL;

			unsigned long attrSize = 0;
			const char* attr = result->getDataStream("attributes", attrSize);
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
				//find this type in the tile
				for(uint32_t i = 0; i < tile->getThingCount(); ++i){
					Item* findItem = tile->__getThing(i)->getItem();

					if(!findItem)
						continue;

					if(findItem->getID() == type){
						item = findItem;
						break;
					}
					else if(iType.isDoor() && findItem->getDoor()){
						item = findItem;
						break;
					}
					//[ added for beds system
					else if(iType.isBed() && findItem->getBed()) {
						item = findItem;
						break;
					}
					//]
				}
			}

			if(item){
				if(!item->unserializeAttr(propStream)){
					std::cout << "WARNING: Serialize error in IOMapSerialize::loadTile()" << std::endl;
				}

				item = g_game.transformItem(item, type);

				std::pair<Item*, int> myPair(item, pid);
				itemMap[sid] = myPair;
			}
			else{
				std::cout << "WARNING: IOMapSerialize::loadTile(). NULL item at " << tile->getPosition() << ". type = " << type << ", sid = " << sid << ", pid = " << pid << std::endl;
			}
		}while(result->next());

		db.freeResult(result);
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

bool IOMapSerialize::saveMapRelational(Map* map)
{
	Database* db = Database::instance();
	DBQuery query;
	DBTransaction transaction(db);

	//Start the transaction
	if(!transaction.begin())
		return false;

	//clear old tile data
	if(!db->executeQuery("DELETE FROM `tiles`")){
		return false;
	}
	if(!db->executeQuery("DELETE FROM `tile_items`")){
		return false;
	}

	uint32_t tileId = 0;
	for(HouseMap::iterator it = Houses::getInstance().getHouseBegin();
		it != Houses::getInstance().getHouseEnd(); ++it){

		//save house items
		House* house = it->second;
		for(HouseTileList::iterator it = house->getHouseTileBegin(); it != house->getHouseTileEnd(); ++it){
			++tileId;
			if(!saveTile(db, tileId, *it)){
				return false;
			}
		}
	}

	//End the transaction
	return transaction.commit();
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
	DBQuery query;

	DBInsert stmt(db);

	stmt.setQuery("INSERT INTO `tile_items` (`tile_id`, `sid`, `pid`, `itemtype`, `count`, `attributes`) VALUES ");

	for(uint32_t i = 0; i < tile->getThingCount(); ++i){
		item = tile->__getThing(i)->getItem();

		if(!item)
			continue;

		if(!(!item->isNotMoveable() ||
			item->getDoor() ||
			(item->getContainer() && item->getContainer()->size() != 0)||
			(item->canWriteText())
			/* uncommented for beds system :] */ || item->getBed()))
			continue;

		if(!storedTile){
			const Position& tilePos = tile->getPosition();
			query << "INSERT INTO `tiles` (`id`, `x`, `y`, `z`) VALUES (" << tileId
			<< ", " << tilePos.x << ", " << tilePos.y << ", " << tilePos.z
			<< ")";

			if( !db->executeQuery(query.str()))
				return false;
			query.str("");
			storedTile = true;
		}
		++runningID;

		uint32_t attributesSize;

		PropWriteStream propWriteStream;
		item->serializeAttr(propWriteStream);
		const char* attributes = propWriteStream.getStream(attributesSize);

		query << tileId << ", " << runningID << ", " << parentid << ", " << item->getID() << ", " << (int32_t)item->getSubType() << ", " << db->escapeBlob(attributes, attributesSize);

		if(!stmt.addRow(query))
			return false;

		if(item->getContainer())
			containerStackList.push_back(ContainerStackList_Pair(item->getContainer(), runningID));
	}

	while(containerStackList.size() > 0){
		ContainerStackList_Pair csPair = containerStackList.front();
		container = csPair.first;
		parentid = csPair.second;
		containerStackList.pop_front();

		for(ItemList::const_iterator it = container->getItems(); it != container->getEnd(); ++it){
			item = (*it);
			++runningID;
			if(item->getContainer())
				containerStackList.push_back(ContainerStackList_Pair(item->getContainer(), runningID));

			uint32_t attributesSize;

			PropWriteStream propWriteStream;
			item->serializeAttr(propWriteStream);
			const char* attributes = propWriteStream.getStream(attributesSize);

			query << tileId << ", " << runningID << ", " << parentid << ", " << item->getID() << ", " << (int32_t)item->getSubType() << ", " << db->escapeBlob(attributes, attributesSize);

			if(!stmt.addRow(query))
				return false;
		}
	}

	if(!stmt.execute())
		return false;

	return true;
}

bool IOMapSerialize::loadMapBinary(Map* map)
{
	Database* db = Database::instance();
	DBQuery query;
 
	DBResult* result = db->storeQuery("SELECT `data` FROM `map_store`;");
 	if(!result)
 		return false;
 
	do {
		unsigned long attrSize = 0;
		const char* attr = result->getDataStream("data", attrSize);
		PropStream propStream;
		propStream.init(attr, attrSize);
 
		while(propStream.size()) {
			uint32_t item_count = 0;
			uint16_t x = 0, y = 0;
			uint8_t z = 0;

			propStream.GET_USHORT(x);
			propStream.GET_USHORT(y);
			propStream.GET_UCHAR(z);
 
			Tile* tile = map->getTile(x, y, z);
			if(!tile) {
				std::cout << "ERROR: Unserialization of invalid tile in IOMapSerialize::loadTile()" << std::endl;
				break;
			}
 
			propStream.GET_ULONG(item_count);
			while(item_count--){
				loadItem(propStream, tile);
			}
 		}
	} while(result->next());
 
	db->freeResult(result);

 	return true;
}

bool IOMapSerialize::loadItem(PropStream& propStream, Cylinder* parent){
	Item* item = NULL;
	
	uint16_t id = 0;
	propStream.GET_USHORT(id);

	const ItemType& iType = Item::items[id];
	if(iType.moveable){
		//create a new item
		item = Item::CreateItem(id);

		if(item){
			bool ret = item->unserializeAttr(propStream);

			item = g_game.transformItem(item, id);

			if(!ret) {
				// Somewhat ugly hack to inject a custom attribute for container items
				propStream.SKIP_N(-1);
				uint8_t prop = 0;
				propStream.GET_UCHAR(prop);
				if(prop == ATTR_CONTAINER_ITEMS){
					Container* container = item->getContainer();
					uint32_t nitems = 0;
					propStream.GET_ULONG(nitems);
					while(nitems > 0){
						if(!loadItem(propStream, container)){
							std::cout << "WARNING: Unserialization error for containing item in IOMapSerialize::loadItem()" << id << std::endl;
							return false;
						}
						--nitems;
					}
					ret = item->unserializeAttr(propStream);
				}
			}
			if(!ret){
				std::cout << "WARNING: Unserialization error in IOMapSerialize::loadItem()" << id << std::endl;
			}

			if(parent){
				parent->__internalAddThing(item);
				item->__startDecaying();
			}
			else{
				delete item;
			}
		}
	}
	else
	{
		// A static item (bed)
		// find this type in the tile
		Tile* tile = parent->getTile();
		if(!tile){
			std::cout << "WARNING: Unserialization error in IOMapSerialize::loadItem()" << std::endl;
			return false;
		}
		for(uint32_t i = 0; i < tile->getThingCount(); ++i)
		{
			Item* findItem = tile->__getThing(i)->getItem();

			if(!findItem)
				continue;

			if(findItem->getID() == id){
				item = findItem;
				break;
			}
			else if(iType.isDoor() && findItem->getDoor()){
				item = findItem;
				break;
			}
			else if(iType.isBed() && findItem->getBed()) {
				item = findItem;
				break;
			}
		}

		if(item)
		{
			bool ret = item->unserializeAttr(propStream);

			item = g_game.transformItem(item, id);

			// Code duplication is bad, fix?
			if(!ret) {
				// Somewhat ugly hack to inject a custom attribute for container items
				propStream.SKIP_N(-1);
				uint8_t prop = 0;
				propStream.GET_UCHAR(prop);
				if(prop == ATTR_CONTAINER_ITEMS){
					Container* container = item->getContainer();
					uint32_t nitems = 0;
					propStream.GET_ULONG(nitems);
					while(nitems > 0){
						if(!loadItem(propStream, container)){
							std::cout << "WARNING: Unserialization error for containing item in IOMapSerialize::loadItem()" << id << std::endl;
							return false;
						}
						--nitems;
					}
					ret = item->unserializeAttr(propStream);
				}
			}
			if(!ret){
				std::cout << "WARNING: Unserialization error in IOMapSerialize::loadItem()" << id << std::endl;
			}
		}
		else
		{
			// Problems! We need to unserialize the attributes, but we have no item to do it on.
			// Use a dummy item to unserialize.
			Item dummy(0);
			dummy.unserializeAttr(propStream);
		}
	}

	return true;
}

bool IOMapSerialize::saveMapBinary(Map* map)
{
 	Database* db = Database::instance();
 	DBQuery query;
 	DBTransaction transaction(db);
	DBInsert stmt(db);
	stmt.setQuery("INSERT INTO `map_store` (`house_id`, `data`) VALUES ");
 

	//Start the transaction
 	if(!transaction.begin())
 		return false;
 
	if(!db->executeQuery("DELETE FROM `map_store`;"))
 		return false;
		
	//clear old tile data
 	for(HouseMap::iterator it = Houses::getInstance().getHouseBegin();
		it != Houses::getInstance().getHouseEnd();
		++it)
	{
 		//save house items
 		House* house = it->second;
		PropWriteStream stream;
		for(HouseTileList::iterator tile_iter = house->getHouseTileBegin();
			tile_iter != house->getHouseTileEnd();
			++tile_iter)
		{
			if(!saveTile(stream, *tile_iter)){
 				return false;
 			}
 		}

		uint32_t attributesSize;
		const char* attributes = stream.getStream(attributesSize);
		query << it->second->getHouseId() << ", " <<
			db->escapeBlob(attributes, attributesSize);

		if(!stmt.addRow(query))
			return false;
 	}
 
	if(!stmt.execute())
		return false;

 	//End the transaction
 	return transaction.commit();
}

bool IOMapSerialize::saveItem(PropWriteStream& stream, const Item* item)
{
	const Container* container = item->getContainer();
	
	// Write ID & props
	stream.ADD_USHORT(item->getID());
	item->serializeAttr(stream);

	if(container){
		// Hack our way into the attributes
		stream.ADD_UCHAR(ATTR_CONTAINER_ITEMS);
		stream.ADD_ULONG(container->size());
		for(ItemList::const_reverse_iterator i = container->getReversedItems(); i != container->getReversedEnd(); ++i){
			saveItem(stream, *i);
		}
	}

	stream.ADD_UCHAR(0x00); // attr end

	return true;
}

bool IOMapSerialize::saveTile(PropWriteStream& stream, const Tile* tile)
{
	std::vector<Item*> items;
	for(int32_t i = tile->getThingCount(); i > 0; --i)
	{
		Item* item = tile->__getThing(i - 1)->getItem();
		if(!item)
			continue;

		// Note that these are NEGATED, ie. these are the items that will be saved.
		if(!(	item->isMoveable() || // is Moveable
				item->getDoor() ||
				(item->getContainer() && item->getContainer()->size() != 0) || // Static containers that can't be moved
				item->canWriteText() || // Blackboards needs to be saved too
				item->getBed()))
			continue;
		
		items.push_back(item);
	}

	if(items.size() > 0) {
		stream.ADD_USHORT(tile->getPosition().x);
		stream.ADD_USHORT(tile->getPosition().y);
		stream.ADD_UCHAR(tile->getPosition().z);
		stream.ADD_ULONG(items.size());

		for(std::vector<Item*>::iterator iter = items.begin();
			iter != items.end();
			++iter)
		{
			saveItem(stream, *iter);
		}
	}

	return true;
}

bool IOMapSerialize::loadHouseInfo(Map* map)
{
	Database* db = Database::instance();
	DBQuery query;
	DBResult* result;

	if(!(result = db->storeQuery("SELECT * FROM `houses`")))
		return false;

	do{
		int houseid = result->getDataInt("id");
		House* house = Houses::getInstance().getHouse(houseid);
		if(house){
			int ownerid = result->getDataInt("owner");
			int paid = result->getDataInt("paid");
			int payRentWarnings = result->getDataInt("warnings");
			uint32_t lastWarning = result->getDataInt("lastwarning");

			house->setHouseOwner(ownerid);
			house->setPaidUntil(paid);
			house->setPayRentWarnings(payRentWarnings);
			house->setLastWarning(lastWarning);
		}
	}while(result->next());

	db->freeResult(result);

	for(HouseMap::iterator it = Houses::getInstance().getHouseBegin(); it != Houses::getInstance().getHouseEnd(); ++it){
		House* house = it->second;
		if(house->getHouseOwner() != 0 && house->getHouseId() != 0){
			query << "SELECT `listid`, `list` FROM `house_lists` WHERE `house_id` = " << house->getHouseId();
			if((result = db->storeQuery(query.str()))){
				do{
					int listid = result->getDataInt("listid");
					std::string list = result->getDataString("list");
					house->setAccessList(listid, list);
				}while(result->next());

				db->freeResult(result);
			}
			query.str("");
		}
	}

	return true;
}

bool IOMapSerialize::saveHouseInfo(Map* map)
{
	Database* db = Database::instance();
	DBQuery query;
	DBTransaction transaction(db);

	if(!transaction.begin())
		return false;

	if(!db->executeQuery("DELETE FROM `houses`")){
		return false;
	}

	DBInsert stmt(db);

	stmt.setQuery("INSERT INTO `houses` (`id`, `owner`, `paid`, `warnings`, `lastwarning`) VALUES ");

	for(HouseMap::iterator it = Houses::getInstance().getHouseBegin(); it != Houses::getInstance().getHouseEnd(); ++it){
		House* house = it->second;

		query << house->getHouseId() << ", " << house->getHouseOwner() << ", "
		<< house->getPaidUntil() << ", " << house->getPayRentWarnings() << ", " << house->getLastWarning();

		if(!stmt.addRow(query)){
			return false;
		}
	}

	if(!stmt.execute()){
		return false;
	}

	stmt.setQuery("INSERT INTO `house_lists` (`house_id`, `listid`, `list`) VALUES ");

	for(HouseMap::iterator it = Houses::getInstance().getHouseBegin(); it != Houses::getInstance().getHouseEnd(); ++it){
		House* house = it->second;

		std::string listText;
		if(house->getAccessList(GUEST_LIST, listText) && listText != ""){
			query << house->getHouseId() << ", " << GUEST_LIST << ", " << db->escapeString(listText);

			if(!stmt.addRow(query)){
				return false;
			}
		}
		if(house->getAccessList(SUBOWNER_LIST, listText) && listText != ""){
			query << house->getHouseId() << ", " << SUBOWNER_LIST << ", " << db->escapeString(listText);

			if(!stmt.addRow(query)){
				return false;
			}
		}

		for(HouseDoorList::iterator it = house->getHouseDoorBegin(); it != house->getHouseDoorEnd(); ++it){
			const Door* door = *it;
			if(door->getAccessList(listText) && listText != ""){
				query << house->getHouseId() << ", " << door->getDoorId() << ", " << db->escapeString(listText);

				if(!stmt.addRow(query)){
					return false;
				}
			}
		}
	}

	if(!stmt.execute()){
		return false;
	}

	return transaction.commit();
}

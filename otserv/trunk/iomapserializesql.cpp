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
#include "otpch.h"

#include "iomapserializesql.h"
#include "house.h"
#include "configmanager.h"
#include "luascript.h"
#include "game.h"

extern ConfigManager g_config;
extern Game g_game;

IOMapSerializeSQL::IOMapSerializeSQL()
{
	m_host = g_config.getString(ConfigManager::SQL_HOST);
	m_user = g_config.getString(ConfigManager::SQL_USER);
	m_pass = g_config.getString(ConfigManager::SQL_PASS);
	m_db   = g_config.getString(ConfigManager::SQL_DB);
}

IOMapSerializeSQL::~IOMapSerializeSQL()
{
	//
}

bool IOMapSerializeSQL::loadMap(Map* map, const std::string& identifier)
{
	Database* db = Database::instance();
	DBQuery query(); //we need this to lock database
	
	if(!db->connect(m_db.c_str(), m_host.c_str(), m_user.c_str(), m_pass.c_str())){
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

bool IOMapSerializeSQL::saveMap(Map* map, const std::string& identifier)
{
	Database* db = Database::instance();
	DBQuery query;
	
	if(!db->connect(m_db.c_str(), m_host.c_str(), m_user.c_str(), m_pass.c_str())){
		return false;
	}
	
	//Start the transaction
	DBTransaction trans(db);
	if(!trans.start()){
		return false;
	}

	//clear old tile data
	query.reset();
	query << "DELETE FROM tileitems;";
	if(!db->executeQuery(query)){
		return false;
	}

	query.reset();
	query << "DELETE FROM tilelist;";
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
	return trans.success();
}

bool IOMapSerializeSQL::saveTile(Database* db, uint32_t tileId, const Tile* tile)
{
	typedef std::list<std::pair<Container*, int> > ContainerStackList;
	typedef ContainerStackList::value_type ContainerStackList_Pair;
	ContainerStackList containerStackList;

	bool storeTile = false;
	int runningID = 0;
	Item* item = NULL;
	Container* container = NULL;

	int parentid = 0;
	std::stringstream streamitems;
	std::string itemsstring;
	int n = 0;

	DBSplitInsert query_insert(db);
	query_insert.setQuery("INSERT INTO `tileitems` (`tileid`, `sid` , `pid` , `type` , `attributes` ) VALUES ");

	for(uint32_t i = 0; i < tile->getThingCount(); ++i){
		item = tile->__getThing(i)->getItem();

		if(!item)
			continue;

		if(!(!item->isNotMoveable() ||
			item->getDoor() ||
			(item->getContainer() && item->getContainer()->size() != 0)||
			(item->getRWInfo(n) & CAN_BE_WRITTEN)
			/*item->getBed()*/))
			continue;

		storeTile = true;
		++runningID;

		const char* attributes = NULL;
		unsigned long attribSize = 0;

		PropWriteStream propWriteStream;
		item->serializeAttr(propWriteStream);
		attributes = propWriteStream.getStream(attribSize);

		streamitems << "(" << tileId << "," << runningID << "," << parentid << "," << item->getID() << ","
			<< "'" << Database::escapeString(attributes, attribSize) <<"')";
		
		if(!query_insert.addRow(streamitems.str()))
			return false;

		streamitems.str("");
        
		if(item->getContainer()){
			containerStackList.push_back(ContainerStackList_Pair(item->getContainer(), runningID));
		}
	}

	if(storeTile){
		DBQuery tileListQuery;
		const Position& tilePos = tile->getPosition();
		tileListQuery << "INSERT INTO `tilelist` (`tileid`, `x` , `y` , `z` ) VALUES";
		tileListQuery << "(" << tileId << "," << tilePos.x << "," << tilePos.y << "," << tilePos.z << ")";

		if(!db->executeQuery(tileListQuery))
			return false;
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

			const char* attributes = NULL;
			unsigned long attribSize = 0;

			PropWriteStream propWriteStream;
			item->serializeAttr(propWriteStream);
			attributes = propWriteStream.getStream(attribSize);

			streamitems << "(" << tileId << "," << runningID << "," << parentid << "," << item->getID() << ","
				<< "'" << Database::escapeString(attributes, attribSize) <<"')";
				
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

bool IOMapSerializeSQL::loadTile(Database& db, Tile* tile)
{
	typedef OTSERV_HASH_MAP<int,std::pair<Item*,int> > ItemMap;
	ItemMap itemMap;

	const Position& tilePos = tile->getPosition();

	DBQuery query;
	query.reset();
	query << "SELECT tilelist.tileid FROM tilelist WHERE x='" << tilePos.x
		<< "' AND y='" << tilePos.y
		<< "' AND z='" << tilePos.z
		<< "'";

	DBResult result;
	if(!db.storeQuery(query, result) || result.getNumRows() != 1)
		return false;

	int tileId = result.getDataInt("tileid");

	query.reset();
	query << "SELECT * FROM tileitems WHERE tileid='" << tileId <<"' ORDER BY sid DESC";
	if(db.storeQuery(query, result) && (result.getNumRows() > 0)){
		Item* item = NULL;

		for(uint32_t i=0; i < result.getNumRows(); ++i){
			int sid = result.getDataInt("sid", i);
			int pid = result.getDataInt("pid", i);
			int type = result.getDataInt("type", i);
			item = NULL;

			unsigned long attrSize = 0;
			const char* attr = result.getDataBlob("attributes", attrSize, i);
			PropStream propStream;
			propStream.init(attr, attrSize);

			const ItemType& iType = Item::items[type];
			if(iType.moveable || /* or object in a container*/ pid != 0){
				//create a new item
				item = Item::CreateItem(type);

				if(item){
					if(!item->unserializeAttr(propStream)){
						std::cout << "WARNING: Serialize error in IOMapSerializeSQL::loadTile()" << std::endl;
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
							std::cout << "WARNING: Serialize error in IOMapSerializeSQL::loadTile()" << std::endl;
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
				std::cout << "WARNING: IOMapSerializeSQL::loadTile(). NULL item at " << tile->getPosition() << ". type = " << type << ", sid = " << sid << ", pid = " << pid << std::endl;
			}
		}
	}

	ItemMap::iterator it;
	for(int i = (int)itemMap.size(); i > 0; --i){
		it = itemMap.find(i);
		if(it == itemMap.end())
			continue;

		if(int p = (*it).second.second){
			ItemMap::iterator pit = itemMap.find(p); //parent container
			if(pit == itemMap.end())
				continue;

			if(Container* container = (*pit).second.first->getContainer()){
				container->__internalAddThing((*it).second.first);
				g_game.startDecay((*it).second.first);
			}
		}
	}

	return true;
}

bool IOMapSerializeSQL::loadHouseInfo(Map* map, const std::string& identifier)
{
	Database* db = Database::instance();
	DBQuery query;
	DBResult result;
	
	if(!db->connect(m_db.c_str(), m_host.c_str(), m_user.c_str(), m_pass.c_str())){
		return false;
	}

	query << "SELECT * FROM houses";
	if(!db->storeQuery(query, result) || result.getNumRows() == 0)
		return false;

	for(uint32_t i = 0; i < result.getNumRows(); ++i){
		int houseid = result.getDataInt("houseid", i);
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
			query << "SELECT listid,list FROM houseaccess WHERE houseid =" << house->getHouseId();

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

bool IOMapSerializeSQL::saveHouseInfo(Map* map, const std::string& identifier)
{
	Database* db = Database::instance();
	DBQuery query;
	DBResult result;
	
	if(!db->connect(m_db.c_str(), m_host.c_str(), m_user.c_str(), m_pass.c_str())){
		return false;
	}

	DBTransaction trans(db);
	if(!trans.start()){
		return false;
	}
	
	query << "DELETE FROM houses;";
	if(!db->executeQuery(query))
		return false;

	query.reset();
	query << "DELETE FROM houseaccess;";
	if(!db->executeQuery(query))
		return false;

    std::stringstream housestream;

	DBSplitInsert query_insert(db);
	query_insert.setQuery("INSERT INTO `houses` (`houseid` , `owner` , `paid`, `warnings`) VALUES ");

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
		query_insert.setQuery("INSERT INTO `houseaccess` (`houseid` , `listid` , `list`) VALUES ");
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

	return trans.success();
}

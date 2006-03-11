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

extern LuaScript g_config;

IOMapSerializeSQL::IOMapSerializeSQL()
{
	m_host = g_config.getGlobalString("sql_host");
	m_user = g_config.getGlobalString("sql_user");
	m_pass = g_config.getGlobalString("sql_pass");
	m_db   = g_config.getGlobalString("sql_db");
}

IOMapSerializeSQL::~IOMapSerializeSQL()
{
	//
}

bool IOMapSerializeSQL::loadMap(Map* map, const std::string& identifier)
{
	db.connect(identifier.c_str(), m_host.c_str(), m_user.c_str(), m_pass.c_str());
	
	for(HouseMap::iterator it = Houses::getInstance().getHouseBegin(); it != Houses::getInstance().getHouseEnd(); ++it){
		//load tile
		House* house = it->second;
		for(HouseTileList::iterator it = house->getHouseTileBegin(); it != house->getHouseTileEnd(); ++it){
			loadTile(*it);
		}
	}

	return true;
}

bool IOMapSerializeSQL::saveMap(Map* map, const std::string& identifier)
{
	DBQuery query;
	db.connect(m_db.c_str(), m_host.c_str(), m_user.c_str(), m_pass.c_str());

	//clear old tile data
	query.reset();
	query << "DELETE FROM tileitems;";
	if(!db.executeQuery(query))
		return false;

	query.reset();
	query << "DELETE FROM tilelist;";
	if(!db.executeQuery(query))
		return false;

	uint32_t tileId = 0;

	for(HouseMap::iterator it = Houses::getInstance().getHouseBegin(); it != Houses::getInstance().getHouseEnd(); ++it){
		//Start the transaction	
		query << "BEGIN;";
		query.reset();
		if(!db.executeQuery(query))
			return false;

		//save house items
		House* house = it->second;
		for(HouseTileList::iterator it = house->getHouseTileBegin(); it != house->getHouseTileEnd(); ++it){
			saveTile(tileId, *it);
			++tileId;
		}

		//End the transaction
		query.reset();
		query << "COMMIT;";

		if(!db.executeQuery(query))
			return false;
	}

	return true;
}

bool IOMapSerializeSQL::saveTile(uint32_t tileId, const Tile* tile)
{
	DBQuery query;
	query << "INSERT INTO `tileitems` (`tileid`, `sid` , `pid` , `type` , `attributes` ) VALUES";

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

		++runningID;

		streamitems << "(" << tileId << "," << runningID << "," << parentid << "," << item->getID() << ","
		//attributes
		<< 0x00 <<"')'";

		/*
		<< (int)item->getItemCountOrSubtype() << "," << (int)item->getActionId() << ",'"
		<< Database::escapeString(item->getText()) << "','"
		<< Database::escapeString(item->getSpecialDescription()) << "'),";
		*/

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

			streamitems << "(" << tileId << "," << runningID << "," << parentid << "," << item->getID() << ","
			//attributes
			<< 0x00 <<"')'";
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

	return true;
}

bool IOMapSerializeSQL::loadTile(Tile* tile)
{
	typedef OTSERV_HASH_MAP<int,std::pair<Item*,int> > ItemMap;
	ItemMap itemMap;

	const Position& tilePos = tile->getPosition();

	DBQuery query;
	query << "SELECT tilelist.tileid FROM tilelist WHERE x='" << tilePos.x
		<< "' AND y='" << tilePos.y
		<< "' AND z='" << tilePos.z
		<< "'";

	DBResult result;
	if(!db.storeQuery(query, result) || result.getNumRows() != 1)
		return false;

	int tileId = result.getDataInt("tileid");

	query.reset();
	query << "SELECT * FROM tileitems WHERE tileid='" << tileId <<"'";
	if(db.storeQuery(query, result) && (result.getNumRows() > 0)){	
		for(int i=0; i < result.getNumRows(); ++i){
			int sid = result.getDataInt("sid", i);
			int pid = result.getDataInt("pid", i);
			int type = result.getDataInt("type", i);

			if(Item::items[type].moveable){
				//create a new item
			}
			else{
				//find this type in the tile
			}

			/*
			int count = result.getDataInt("number", i);
			Item* item = Item::CreateItem(type, count);
			if(result.getDataInt("actionid", i) >= 100)
				item->setActionId(result.getDataInt("actionid", i));

			item->setText(result.getDataString("text", i));
			item->setSpecialDescription(result.getDataString("specialdesc",i));
			std::pair<Item*, int> myPair(item, result.getDataInt("pid", i));
			itemMap[result.getDataInt("sid", i)] = myPair;
			
			tile->__internalAddThing(item);
			*/
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

	return true;
}

bool IOMapSerializeSQL::loadHouseInfo(Map* map, const std::string& identifier)
{
	
	Database db;
	DBQuery query;
	DBResult result;

	db.connect(m_db.c_str(), m_host.c_str(), m_user.c_str(), m_pass.c_str());
	
	query << "SELECT * FROM houses";

	if(!db.storeQuery(query, result) || result.getNumRows() == 0)
		return false;
	
	for(int i=0; i < result.getNumRows(); ++i){
		int houseid = result.getDataInt("houseid", i);
		House* house = Houses::getInstance().getHouse(houseid);
		if(house){
			int ownerid = result.getDataInt("owner", i);
			int paid = result.getDataInt("paid", i);
			house->setHouseOwner(ownerid);
			house->setPaidUntil(paid);
		}
	}

	for(HouseMap::iterator it = Houses::getInstance().getHouseBegin(); it != Houses::getInstance().getHouseEnd(); ++it){
		query.reset();
		House* house = it->second;
		if(house->getHouseOwner() != 0 && house->getHouseId() != 0){
			query << "SELECT listid,list FROM houseaccess WHERE houseid =" << house->getHouseId();
			if(db.storeQuery(query, result) && result.getNumRows() != 0){
				for(int i=0; i < result.getNumRows(); ++i){
					int listid = result.getDataInt("listid", i);
					std::string list = result.getDataString("list", i);
					house->setAccessList(listid, list);
				}
			}
		}
	}
}

bool IOMapSerializeSQL::saveHouseInfo(Map* map, const std::string& identifier)
{
	
	Database db;
	DBQuery query;
	DBResult result;

	db.connect(m_db.c_str(), m_host.c_str(), m_user.c_str(), m_pass.c_str());
	
	query << "BEGIN;";
	if(!db.executeQuery(query))
		return false;

	query << "DELETE FROM houses;";
	if(!db.executeQuery(query))
		return false;

	query.reset();
	query << "DELETE FROM houseaccess;";
	if(!db.executeQuery(query))
		return false;

	query << "INSERT INTO `houses` (`houseid` , `owner` , `paid`) VALUES ";
	for(HouseMap::iterator it = Houses::getInstance().getHouseBegin(); it != Houses::getInstance().getHouseEnd(); ++it){
		House* house = it->second;
		query << query.getSeparator() << "(" << house->getHouseId() << "," << house->getHouseOwner() << "," << house->getPaidUntil() << ")";
	}
	if(!db.executeQuery(query))
		return false;
	
	
	for(HouseMap::iterator it = Houses::getInstance().getHouseBegin(); it != Houses::getInstance().getHouseEnd(); ++it){
		query.reset();
		query << "INSERT INTO `houseaccess` (`houseid` , `listid` , `list`) VALUES ";
		House* house = it->second;
		
		std::string listText;
		if(house->getAccessList(GUEST_LIST, listText)){
			query << query.getSeparator() << "(" << house->getHouseId() << "," << GUEST_LIST << ",'" << Database::escapeString(listText) << "')";
		}
		if(house->getAccessList(SUBOWNER_LIST, listText)){
			query << query.getSeparator()<< "(" << house->getHouseId() << "," << SUBOWNER_LIST << ",'" << Database::escapeString(listText) << "')";
		}
		
		for(HouseDoorList::iterator it = house->getHouseDoorBegin(); it != house->getHouseDoorEnd(); ++it){
			const Door* door = *it;
			if(door->getAccessList(listText)){
				query << query.getSeparator() << "(" << house->getHouseId() << "," << door->getDoorId() << ",'" << Database::escapeString(listText) << "')";
			}
		}
		
		if(!db.executeQuery(query))
			return false;
	}
	
	query.reset();
	query << "COMMIT;";	
	if(!db.executeQuery(query))
		return false;	
	
}

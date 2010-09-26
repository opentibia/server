//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
//
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

#include "house.h"
#include "ioplayer.h"
#include "game.h"
#include "town.h"
#include "configmanager.h"
#include "tools.h"
#include "guild.h"
#include "beds.h"
#include <boost/algorithm/string/predicate.hpp>
#include <sstream>
#include <algorithm>

extern ConfigManager g_config;
extern Game g_game;
extern Guilds g_guilds;

House::House(uint32_t _id) :
transfer_container(ITEM_LOCKER1)
{
	isLoaded = false;
	name = "OTServ headquarter (Flat 1, Area 42)";
	owner = 0;
	posEntry.x = 0;
	posEntry.y = 0;
	posEntry.z = 0;
	paidUntil = 0;
	id = _id;
	rentWarnings = 0;
	lastWarning = 0;
	rent = 0;
	townid = 0;
	syncFlags = HOUSE_SYNC_TOWNID | HOUSE_SYNC_NAME | HOUSE_SYNC_RENT | HOUSE_SYNC_GUILDHALL;
	transferItem = NULL;
	guildHall = false;
	pendingDepotTransfer = false;
}

House::~House()
{
	//
}

void House::addTile(HouseTile* tile)
{
	tile->setFlag(TILESTATE_PROTECTIONZONE);
	houseTiles.push_back(tile);
}

void House::setOwner(uint32_t guid)
{
	if(isLoaded && owner == guid)
		return;

	isLoaded = true;

	if(owner){
		clean();

		//clean access lists
		owner = 0;
		setAccessList(SUBOWNER_LIST, "");
		setAccessList(GUEST_LIST, "");

		for(HouseDoorList::iterator it = doorList.begin(); it != doorList.end(); ++it){
			(*it)->setAccessList("");
		}

		//reset paid date
		paidUntil = 0;
		rentWarnings = 0;
	}

	std::string name;
	if(guid != 0 && IOPlayer::instance()->getNameByGuid(guid, name)){
		owner = guid;
		ownerName = name;
	}

	updateDoorDescription();
	setLastWarning(std::time(NULL)); //So the new owner has one day before he start the payment
}

void House::updateDoorDescription()
{
	std::stringstream houseDescription;
	houseDescription << "It belongs to house '" << name << "'." << std::endl;

	if(owner != 0){
		houseDescription << ownerName << " owns this house.";
	}
	else{
		houseDescription << "Nobody owns this house.";
		if(g_config.getNumber(ConfigManager::SHOW_HOUSE_PRICES)){
			uint32_t price = getTileCount() * g_config.getNumber(ConfigManager::HOUSE_TILE_PRICE);
			houseDescription << std::endl << "It costs " << price << " gold coins.";
			std::string strPeriod;
			Houses::getInstance().getRentPeriodString(strPeriod);
			if(strPeriod != "never"){
				houseDescription << " Its rent costs " << getRent() << " gold coins and it's paid " << strPeriod << ".";
			}
		}
	}

	HouseDoorList::iterator it;
	for(it = doorList.begin(); it != doorList.end(); ++it){
		(*it)->setSpecialDescription(houseDescription.str());
	}
}

AccessHouseLevel_t House::getHouseAccessLevel(const Player* player)
{
	if(player == NULL) // By script
		return HOUSE_OWNER;

	if(player->hasFlag(PlayerFlag_CanEditHouses))
		return HOUSE_OWNER;

	if(player->getGUID() == owner)
		return HOUSE_OWNER;

	if(subOwnerList.isInList(player))
		return HOUSE_SUBOWNER;

	if(guestList.isInList(player))
		return HOUSE_GUEST;

	return HOUSE_NO_INVITED;
}

bool House::kickPlayer(Player* player, const std::string& name)
{
	Player* kickingPlayer = g_game.getPlayerByName(name);
	if(kickingPlayer){
		HouseTile* houseTile = kickingPlayer->getTile()->getHouseTile();

		if(houseTile && houseTile->getHouse() == this){
			if((getHouseAccessLevel(player) > getHouseAccessLevel(kickingPlayer) || kickingPlayer == player) && !kickingPlayer->hasFlag(PlayerFlag_CanEditHouses)){
				if(g_game.internalTeleport(kickingPlayer, getEntryPosition()) == RET_NOERROR){
					g_game.addMagicEffect(getEntryPosition(), NM_ME_TELEPORT);
				}
				return true;
			}
		}
	}
	return false;
}

void House::setAccessList(uint32_t listId, const std::string& textlist)
{
	if(listId == GUEST_LIST){
		guestList.parseList(textlist);
	}
	else if(listId == SUBOWNER_LIST){
		subOwnerList.parseList(textlist);
	}
	else{
		Door* door = getDoorByNumber(listId);
		if(door){
			door->setAccessList(textlist);
		}
		else{
			#ifdef __DEBUG_HOUSES__
			std::cout << "Failure: [House::setAccessList] door == NULL, listId = " << listId <<std::endl;
			#endif
		}
		//We dont have kick anyone
		return;
	}

	//kick uninvited players
	typedef std::list<Player*> KickPlayerList;
	KickPlayerList kickList;
	HouseTileList::iterator it;
	for(it = houseTiles.begin(); it != houseTiles.end(); ++it){
		HouseTile* hTile = *it;
		if(CreatureVector* creatures = hTile->getCreatures()){
			CreatureVector::iterator cit;
			for(cit = creatures->begin(); cit != creatures->end(); ++cit){
				Player* player = (*cit)->getPlayer();
				if(player && isInvited(player) == false){
					kickList.push_back(player);
				}
			}
		}
	}

	KickPlayerList::iterator itkick;
	for(itkick = kickList.begin(); itkick != kickList.end(); ++itkick){
		if(g_game.internalTeleport(*itkick, getEntryPosition()) == RET_NOERROR){
			g_game.addMagicEffect(getEntryPosition(), NM_ME_TELEPORT);
		}
	}
}

bool House::transferToDepot()
{
	if(townid == 0) {
		return false;
	}

	Player* player = g_game.getPlayerByGuidEx(owner);
	if(!player){
		return false;
	}

	Depot* depot = player->getDepot(townid, true);

	std::list<Item*> moveItemList;
	Container* tmpContainer = NULL;
	Item* item = NULL;

	for(HouseTileList::iterator it = houseTiles.begin(); it != houseTiles.end(); ++it){
		if(const TileItemVector* items = (*it)->getItemList()){
			for(ItemVector::const_iterator it = items->begin(); it != items->end(); ++it){
				item = (*it);
				if(item->isPickupable()){
					moveItemList.push_back(item);
				}
				else if((tmpContainer = item->getContainer())){
					for(ItemList::const_iterator it = tmpContainer->getItems(); it != tmpContainer->getEnd(); ++it){
						moveItemList.push_back(*it);
					}
				}
			}
		}
	}

	for(std::list<Item*>::iterator it = moveItemList.begin(); it != moveItemList.end(); ++it){
		if(depot) {
			g_game.internalMoveItem((*it)->getParent(), depot, INDEX_WHEREEVER,
				(*it), (*it)->getItemCount(), NULL, FLAG_NOLIMIT);
		} else {
			g_game.internalRemoveItem(*it);
		}
	}

	if(player && player->isOffline()){
		IOPlayer::instance()->savePlayer(player);
		delete player;
	}

	return true;
}

void House::clean()
{
	transferToDepot();

	PlayerVector to_kick;
	for(HouseTileList::iterator it = houseTiles.begin(); it != houseTiles.end(); ++it){
		if(const CreatureVector* creatures = (*it)->getCreatures()){
			for(CreatureVector::const_iterator cit = creatures->begin(); cit != creatures->end(); ++cit){
				if((*cit)->getPlayer()){
					to_kick.push_back((*cit)->getPlayer());
				}
			}
		}
	}
	while(!to_kick.empty()){
		Player* c = to_kick.back();
		to_kick.pop_back();
		kickPlayer(NULL, c->getName());
	}

	// we need to remove players from beds
	for(HouseBedItemList::iterator it = bedsList.begin(); it != bedsList.end(); ++it){
		if((*it)->getSleeper() != 0){
			(*it)->wakeUp();
		}
	}
}

bool House::getAccessList(uint32_t listId, std::string& list) const
{
	if(listId == GUEST_LIST){
		guestList.getList(list);
		return true;
	}
	else if(listId == SUBOWNER_LIST){
		subOwnerList.getList(list);
		return true;
	}
	else{
		Door* door = getDoorByNumber(listId);
		if(door){
			return door->getAccessList(list);
		}
		else{
			#ifdef __DEBUG_HOUSES__
			std::cout << "Failure: [House::getAccessList] door == NULL, listId = " << listId <<std::endl;
			#endif
			return false;
		}
	}
	return false;
}

bool House::isInvited(const Player* player)
{
	if(getHouseAccessLevel(player) != HOUSE_NO_INVITED){
		return true;
	}
	else{
		return false;
	}
}

void House::addDoor(Door* door)
{
	door->useThing2();
	doorList.push_back(door);
	door->setHouse(this);
	updateDoorDescription();
}

void House::removeDoor(Door* door)
{
	HouseDoorList::iterator it = std::find(doorList.begin(), doorList.end(), door);
	if(it != doorList.end()){
		(*it)->releaseThing2();
		doorList.erase(it);
	}
}


//[ added for beds system
void House::addBed(BedItem* bed)
{
	bedsList.push_back(bed);
	bed->setHouse(this);
}
//]

Door* House::getDoorByNumber(uint32_t doorId)
{
	HouseDoorList::iterator it;
	for(it = doorList.begin(); it != doorList.end(); ++it){
		if((*it)->getDoorId() == doorId){
			return *it;
		}
	}
	return NULL;
}

Door* House::getDoorByNumber(uint32_t doorId) const
{
	HouseDoorList::const_iterator it;
	for(it = doorList.begin(); it != doorList.end(); ++it){
		if((*it)->getDoorId() == doorId){
			return *it;
		}
	}
	return NULL;
}

Door* House::getDoorByPosition(const Position& pos)
{
	for(HouseDoorList::iterator it = doorList.begin(); it != doorList.end(); ++it){
		if((*it)->getPosition() == pos){
			return *it;
		}
	}

	return NULL;
}

bool House::canEditAccessList(uint32_t listId, const Player* player)
{
	switch(getHouseAccessLevel(player)){
	case HOUSE_OWNER:
		return true;
		break;
	case HOUSE_SUBOWNER:
		/*subowners can edit guest access list*/
		if(listId == GUEST_LIST){
			return true;
		}
		else /*subowner/door list*/{
			return false;
		}
		break;
	default:
		return false;
	}
}

HouseTransferItem* House::getTransferItem()
{
	if(transferItem != NULL)
		return NULL;

	transfer_container.setParent(NULL);
	transferItem =  HouseTransferItem::createHouseTransferItem(this);
	transfer_container.__addThing(transferItem);
	return transferItem;
}

void House::resetTransferItem()
{
	if(transferItem){
		Item* tmpItem = transferItem;
		transferItem = NULL;
		transfer_container.setParent(NULL);

		transfer_container.__removeThing(tmpItem, tmpItem->getItemCount());
		g_game.FreeThing(tmpItem);
	}
}

HouseTransferItem* HouseTransferItem::createHouseTransferItem(House* house)
{
	HouseTransferItem* transferItem = new HouseTransferItem(house);
	transferItem->useThing2();
	transferItem->setID(ITEM_DOCUMENT_RO);
	transferItem->setSubType(1);
	std::stringstream stream;
	stream << " It is a house transfer document for '" << house->getName() << "'.";
	transferItem->setSpecialDescription(stream.str());
	return transferItem;
}

bool HouseTransferItem::onTradeEvent(TradeEvents_t event, Player* owner)
{
	House* house;
	switch(event){
		case ON_TRADE_TRANSFER:
		{
			house = getHouse();
			if(house){
				house->executeTransfer(this, owner);
			}

			g_game.internalRemoveItem(this, 1);
			break;
		}

		case ON_TRADE_CANCEL:
		{
			house = getHouse();
			if(house){
				house->resetTransferItem();
			}

			break;
		}

		default:
			break;
	}

	return true;
}

bool House::executeTransfer(HouseTransferItem* item, Player* newOwner)
{
	if(transferItem != item){
		return false;
	}

	setOwner(newOwner->getGUID());
	transferItem = NULL;
	return true;
}

AccessList::AccessList()
{
	//
}

AccessList::~AccessList()
{
	//
}

bool AccessList::parseList(const std::string& _list)
{
	playerList.clear();
	guildList.clear();
	expressionList.clear();
	regExList.clear();
	list = _list;

	if(_list == "")
		return true;

	std::stringstream listStream(_list);
	std::string line;
	while(getline(listStream, line)){
		//trim left
		trim_left(line, " ");
		trim_left(line, "\t");

		//trim right
		trim_right(line, " ");
		trim_right(line, "\t");

		std::transform(line.begin(), line.end(), line.begin(), tolower);

		if(line.substr(0,1) == "#")
			continue;

		if(line.length() > 100)
			continue;

		if(line.find("@") != std::string::npos){
			std::string::size_type pos = line.find("@");
			addGuild(line.substr(pos + 1), line.substr(0, pos));
		}
		else if(line.find("!") != std::string::npos || line.find("*") != std::string::npos || line.find("?") != std::string::npos){
			addExpression(line);
		}
		else{
			addPlayer(line);
		}
	}
	return true;
}

bool AccessList::addPlayer(std::string& name)
{
	uint32_t guid;
	std::string dbName = name;
	if(IOPlayer::instance()->getGuidByName(guid, dbName)){
		if(playerList.find(guid) == playerList.end()){
			playerList.insert(guid);
			return true;
		}
	}
	return false;
}

bool AccessList::addGuild(const std::string& guildName, const std::string& rank)
{
	uint32_t guildId;
	if(g_guilds.getGuildIdByName(guildId, guildName)){
		if(guildId != 0){
			for(GuildList::iterator it = guildList.begin(); it != guildList.end(); ++it){
				if(it->first == guildId && boost::algorithm::iequals(rank, it->second)){
					return false;
				}
			}

			guildList.push_back(std::pair<uint32_t, std::string>(guildId, rank));
			return true;
		}
	}
	return false;
}

bool AccessList::addExpression(const std::string& expression)
{
	ExpressionList::iterator it;
	for(it = expressionList.begin(); it != expressionList.end(); ++it){
		if((*it) == expression){
			return false;
		}
	}

	std::string outExp;
	std::string metachars = ".[{}()\\+|^$";

	for(std::string::const_iterator it = expression.begin(); it != expression.end(); ++it){
		if(metachars.find(*it) != std::string::npos){
			outExp += "\\";
		}

		outExp += (*it);
	}

	replaceString(outExp, "*", ".*");
	replaceString(outExp, "?", ".?");

	try{
		if(outExp.length() > 0){
			expressionList.push_back(outExp);

			if(outExp.substr(0,1) == "!"){
				if(outExp.length() > 1){
					//push 'NOT' expressions upfront so they are checked first
					regExList.push_front(std::make_pair(boost::regex(outExp.substr(1)), false));
				}
			}
			else{
				regExList.push_back(std::make_pair(boost::regex(outExp), true));
			}
		}
	}
	catch(...){
		//
	}

	return true;
}

bool AccessList::isInList(const Player* player)
{
	RegExList::iterator it;
	std::string name = player->getName();
	boost::cmatch what;

	try{
		std::transform(name.begin(), name.end(), name.begin(), tolower);
		for(it = regExList.begin(); it != regExList.end(); ++it){
			if(boost::regex_match(name.c_str(), what, it->first)){
				if(it->second){
					return true;
				}
				else{
					return false;
				}
			}
		}
	}
	catch(...){
		//
	}

	PlayerList::iterator playerIt = playerList.find(player->getGUID());
	if(playerIt != playerList.end())
		return true;

	for(GuildList::iterator it = guildList.begin(); it != guildList.end(); ++it){
		if((player->getGuild() && it->first == player->getGuildId()) && boost::algorithm::iequals(player->guildRank, it->second)){
			return true;
		}
	}

	return false;
}

void AccessList::getList(std::string& _list) const
{
	_list = list;
}

Door::Door(uint16_t _type):
Item(_type)
{
	house = NULL;
	accessList = NULL;
}

Door::~Door()
{
	if(accessList)
		delete accessList;
}

Attr_ReadValue Door::readAttr(AttrTypes_t attr, PropStream& propStream)
{
	if(ATTR_HOUSEDOORID == attr){
		unsigned char _doorId = 0;
		if(!propStream.GET_UINT8(_doorId)){
			return ATTR_READ_ERROR;
		}

		setDoorId(_doorId);
		return ATTR_READ_CONTINUE;
	}
	else
		return Item::readAttr(attr, propStream);
}

bool Door::serializeAttr(PropWriteStream& propWriteStream) const
{
	//dont call Item::serializeAttr(propWriteStream);

	/*
	if(house){
		unsigned char _doorId = getDoorId();
		propWriteStream.ADD_UINT8(ATTR_HOUSEDOORID);
		propWriteStream.ADD_UINT8(_doorId);
	}
	*/

	return true;
}

void Door::setHouse(House* _house)
{
	if(house != NULL){
		#ifdef __DEBUG_HOUSES__
		std::cout << "Warning: [Door::setHouse] house != NULL" << std::endl;
		#endif
		return;
	}
	house = _house;

	if(!accessList){
		accessList = new AccessList();
	}
}

bool Door::canUse(const Player* player)
{
	if(!house){
		return true;
	}
	if(house->getHouseAccessLevel(player) >= HOUSE_SUBOWNER)
		return true;

	return accessList->isInList(player);
}

void Door::setAccessList(const std::string& textlist)
{
	if(!accessList){
		accessList = new AccessList();
	}

	accessList->parseList(textlist);
}

bool Door::getAccessList(std::string& list) const
{
	if(!house){
		#ifdef __DEBUG_HOUSES__
		std::cout << "Failure: [Door::getAccessList] house == NULL" << std::endl;
		#endif
		return false;
	}

	accessList->getList(list);
	return true;
}

void Door::copyAttributes(Item* item)
{
	Item::copyAttributes(item);

	if(Door* door = item->getDoor()){
		std::string list;
		if(door->getAccessList(list)){
			setAccessList(list);
		}
	}
}

void Door::onRemoved()
{
	Item::onRemoved();

	if(house){
		house->removeDoor(this);
	}
}

Houses::Houses()
{
	std::string strRentPeriod = g_config.getString(ConfigManager::HOUSE_RENT_PERIOD);

	rentPeriod = RENTPERIOD_MONTHLY;

	if(asLowerCaseString(strRentPeriod) == "yearly"){
		rentPeriod = RENTPERIOD_YEARLY;
	}
	else if(asLowerCaseString(strRentPeriod) == "weekly"){
		rentPeriod = RENTPERIOD_WEEKLY;
	}
	else if(asLowerCaseString(strRentPeriod) == "daily"){
		rentPeriod = RENTPERIOD_DAILY;
	}
	else if(asLowerCaseString(strRentPeriod) == "never"){
		rentPeriod = RENTPERIOD_NEVER;
	}
}

Houses::~Houses()
{
	//
}

House* Houses::getHouseByPlayerId(uint32_t playerId)
{
	for(HouseMap::iterator it = houseMap.begin(); it != houseMap.end(); ++it){
		House* house = it->second;
		if(house->getOwner() == playerId){
			return house;
		}
	}
	return NULL;
}

bool Houses::loadHousesXML(std::string filename)
{
	xmlDocPtr doc = xmlParseFile(filename.c_str());
	if(doc){
		xmlNodePtr root, houseNode;
		root = xmlDocGetRootElement(doc);

		if(xmlStrcmp(root->name,(const xmlChar*)"houses") != 0){
			xmlFreeDoc(doc);
			return false;
		}

		int intValue;
		std::string strValue;

		houseNode = root->children;
		while(houseNode){
			if(xmlStrcmp(houseNode->name,(const xmlChar*)"house") == 0){
				int _houseid = 0;
				Position entryPos(0, 0, 0);

				if(!readXMLInteger(houseNode, "houseid", _houseid)){
					xmlFreeDoc(doc);
					return false;
				}

				House* house = Houses::getInstance().getHouse(_houseid);
				if(!house){
					std::cout << "Error: [Houses::loadHousesXML] Unknown house, id = " << _houseid << std::endl;
					xmlFreeDoc(doc);
					return false;
				}

				if(readXMLInteger(houseNode, "entryx", intValue)){
					entryPos.x = intValue;
				}

				if(readXMLInteger(houseNode, "entryy", intValue)){
					entryPos.y = intValue;
				}

				if(readXMLInteger(houseNode, "entryz", intValue)){
					entryPos.z = intValue;
				}

				if(entryPos.x == 0 || entryPos.y == 0){
					std::cout << "Warning: [Houses::loadHousesXML] House entry not set"
						<< " - Name: " << house->getName()
						<< " - House id: " << _houseid << std::endl;
				}

				house->setEntryPos(entryPos);

				if(readXMLInteger(houseNode, "townid", intValue)){
					house->setTownId(intValue);
				}
				else{
					house->resetSyncFlag(House::HOUSE_SYNC_TOWNID);
				}

				if(readXMLString(houseNode, "name", strValue)){
					house->setName(strValue);
				}
				else{
					house->resetSyncFlag(House::HOUSE_SYNC_NAME);
				}

				if(readXMLInteger(houseNode, "rent", intValue)){
					house->setRent(intValue);
				}
				else{
					house->resetSyncFlag(House::HOUSE_SYNC_RENT);
				}

				if(readXMLInteger(houseNode, "guildhall", intValue)){
					house->setGuildHall(intValue == 1);
				}
				else{
					house->resetSyncFlag(House::HOUSE_SYNC_GUILDHALL);
				}

				house->setOwner(0);
			}

			houseNode = houseNode->next;
		}

		xmlFreeDoc(doc);
		return true;
	}

	return false;
}

bool Houses::payRent(Player* player, House* house, time_t time /*= 0*/)
{
	if(rentPeriod == RENTPERIOD_NEVER){
		return true;
	}

	if(time == 0){
		time = std::time(NULL);
	}

	if(house->getRent() == 0 || house->getPaidUntil() > time){
		return true;
	}

	Town* town = Towns::getInstance().getTown(house->getTownId());
	if(!town){
		#ifdef __DEBUG_HOUSES__
		std::cout << "Warning: [Houses::payHouses] town = NULL, townid = " <<
			house->getTownId() << ", houseid = " << house->getId() << std::endl;
		#endif
		return false;
	}

	bool hasEnoughMoney = false;
	Depot* depot = player->getDepot(town->getTownID(), true);
	if(depot){
		if(g_config.getNumber(ConfigManager::USE_BALANCE_HOUSE_PAYING)){
			if(player->balance >= house->getRent()){
				player->balance -= house->getRent();
				hasEnoughMoney = true;
			}
		}
		else{
			hasEnoughMoney = g_game.removeMoney(depot, house->getRent(), FLAG_NOLIMIT);
		}
	}

	if(hasEnoughMoney){
		time_t paidUntil = time;
		switch(rentPeriod){
		case RENTPERIOD_DAILY:
			paidUntil += 24 * 60 * 60;
			break;
		case RENTPERIOD_WEEKLY:
			paidUntil += 24 * 60 * 60 * 7;
			break;
		case RENTPERIOD_MONTHLY:
			paidUntil += 24 * 60 * 60 * 30;
			break;
		case RENTPERIOD_YEARLY:
			paidUntil += 24 * 60 * 60 * 365;
			break;
		case RENTPERIOD_NEVER:
		default:
			break;
		}

		house->setPaidUntil(paidUntil);
	}

	return hasEnoughMoney;
}

bool Houses::payHouse(House* house, time_t time)
{
	if(rentPeriod == RENTPERIOD_NEVER){
		return true;
	}

	if(house->getRent() == 0 || house->getPaidUntil() > time || house->getOwner() == 0){
		return true;
	}

	uint32_t ownerid = house->getOwner();
	Town* town = Towns::getInstance().getTown(house->getTownId());
	if(!town){
		#ifdef __DEBUG_HOUSES__
		std::cout << "Warning: [Houses::payHouses] town = NULL, townid = " <<
			house->getTownId() << ", houseid = " << house->getId() << std::endl;
		#endif
		return false;
	}

	std::string name;
	if(!IOPlayer::instance()->getNameByGuid(ownerid, name)){
		//player doesnt exist, remove it as house owner?
		//house->setOwner(0);
		return false;
	}

	Player* player = g_game.getPlayerByNameEx(name);
	if(!player){
		return false;
	}

	// savePlayerHere is an ugly hack
	// to avoid saving 2 times a not online player
	// when items are transferred to his depot
	bool savePlayerHere = true;
	bool hasPaidRent = payRent(player, house, time);

	if(!hasPaidRent && time >= house->getLastWarning() + 24 * 60 * 60){
		if(house->getPayRentWarnings() >= 7){
			house->setOwner(0);
			// setOwner will load the player,
			// transfer house items to his depot and then
			// will save it, so here should not be saved
			// again
			savePlayerHere = false;
		}
		else{
			Depot* depot = player->getDepot(town->getTownID(), true);
			if(depot){
				int daysLeft = 7 - house->getPayRentWarnings();

				Item* letter = Item::CreateItem(ITEM_LETTER_STAMPED);
				std::string period = "";

				switch(rentPeriod){
					case RENTPERIOD_DAILY:
						period = "daily";
					break;

					case RENTPERIOD_WEEKLY:
						period = "weekly";
					break;

					case RENTPERIOD_MONTHLY:
						period = "monthly";
					break;

					case RENTPERIOD_YEARLY:
						period = "annual";
					break;
					case RENTPERIOD_NEVER:
						//
					break;
				}

				std::stringstream warningText;
				warningText << "Warning! \n" <<
					"The " << period << " rent of " << house->getRent() << " gold for your house \""
					<< house->getName() << "\" is payable. Have it available within " << daysLeft <<
					" days, or you will lose this house.";

				letter->setText(warningText.str());
				g_game.internalAddItem(depot, letter, INDEX_WHEREEVER, FLAG_NOLIMIT);

				house->setPayRentWarnings(house->getPayRentWarnings() + 1);
				house->setLastWarning(time);
			}
		}
	}

	if(player->isOffline()){
		if(savePlayerHere){
			IOPlayer::instance()->savePlayer(player);
		}
		delete player;
	}

	return hasPaidRent;
}

bool Houses::payHouses()
{
	if(rentPeriod == RENTPERIOD_NEVER){
		return true;
	}

	time_t currentTime = std::time(NULL);
	for(HouseMap::iterator it = houseMap.begin(); it != houseMap.end(); ++it){
		payHouse(it->second, currentTime);
	}

	return true;
}

void Houses::getRentPeriodString(std::string& strPeriod)
{
	switch(rentPeriod){
	case RENTPERIOD_DAILY:
		strPeriod = "daily";
		break;
	case RENTPERIOD_WEEKLY:
		strPeriod = "weekly";
		break;
	case RENTPERIOD_MONTHLY:
		strPeriod = "monthly";
		break;
	case RENTPERIOD_YEARLY:
		strPeriod = "yearly";
		break;
	case RENTPERIOD_NEVER:
	default:
		strPeriod = "never";
		break;
	}
}

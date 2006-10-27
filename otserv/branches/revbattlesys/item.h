//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Item represents an existing item.
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


#ifndef __OTSERV_ITEM_H__
#define __OTSERV_ITEM_H__

#include "thing.h"
#include "items.h"

#include <iostream>
#include <list>
#include <vector>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

class Creature;
class Player;
class Container;
class Depot;
class Teleport;
class TrashHolder;
class Mailbox;
class Door;
class MagicField;

enum ITEMPROPERTY{
	BLOCKSOLID,
	HASHEIGHT,
	BLOCKPROJECTILE,
	BLOCKPATHFIND,
	PROTECTIONZONE,
	ISVERTICAL,
	ISHORIZONTAL,
	BLOCKINGANDNOTMOVEABLE,
};

enum TradeEvents_t{
	ON_TRADE_TRANSFER,
	ON_TRADE_CANCEL,
};

enum ItemDecayState_t{
	DECAYING_FALSE = 0,
	DECAYING_TRUE,
	DECAYING_PENDING

	/*
	ITEM_NO_DECAYING = 0,
	ITEM_DECAYING = 1,
	ITEM_PENDING_START_DECAY = 2,
	*/
};

/*from iomapotbm.h*/
#pragma pack(1)
struct TeleportDest{
	unsigned short _x;
	unsigned short _y;
	unsigned char	_z;
};
#pragma pack()

enum AttrTypes_t{
	//ATTR_DESCRIPTION = 1,
	//ATTR_EXT_FILE = 2,
	ATTR_TILE_FLAGS = 3,
	ATTR_ACTION_ID = 4,
	ATTR_UNIQUE_ID = 5,
	ATTR_TEXT = 6,
	ATTR_DESC = 7,
	ATTR_TELE_DEST = 8,
	ATTR_ITEM = 9,
	ATTR_DEPOT_ID = 10,
	//ATTR_EXT_SPAWN_FILE = 11,
	ATTR_RUNE_CHARGES = 12,
	//ATTR_EXT_HOUSE_FILE = 13,
	ATTR_HOUSEDOORID = 14,
	ATTR_COUNT = 15,
	ATTR_DURATION = 16,
	ATTR_DECAYING_STATE = 17,
};

class ItemAttributes{
public:
	ItemAttributes();
	virtual ~ItemAttributes();
	
	void setSpecialDescription(const std::string& desc) {setStrAttr(ATTR_ITEM_DESC, desc);}
	const std::string& getSpecialDescription() const {return getStrAttr(ATTR_ITEM_DESC);}
	
	void setText(const std::string& text) {setStrAttr(ATTR_ITEM_TEXT, text);}
	const std::string& getText() const {return getStrAttr(ATTR_ITEM_TEXT);}
	
	void setActionId(unsigned short n) {if(n < 100) n = 100; setIntAttr(ATTR_ITEM_ACTIONID, n);}
	unsigned short getActionId() const {return getIntAttr(ATTR_ITEM_ACTIONID);}

	void setUniqueId(unsigned short n) {if(n < 1000) n = 1000; setIntAttr(ATTR_ITEM_UNIQUEID, n);}
	unsigned short getUniqueId() const {return getIntAttr(ATTR_ITEM_UNIQUEID);}

	void setOwner(uint32_t _owner) {setIntAttr(ATTR_ITEM_OWNER, _owner);}
	uint32_t getOwner() const {return getIntAttr(ATTR_ITEM_OWNER);}

	void setDuration(int32_t time) {setIntAttr(ATTR_ITEM_DURATION, time);}
	void decreaseDuration(int32_t time) {increaseIntAttr(ATTR_ITEM_DURATION, -time);}
	bool hasDuration() {return hasAttribute(ATTR_ITEM_DURATION);}
	int32_t getDuration() const {return getIntAttr(ATTR_ITEM_DURATION);}

	void setDecaying(ItemDecayState_t decayState) {setIntAttr(ATTR_ITEM_DECAYING, decayState);}
	uint32_t getDecaying() const {return getIntAttr(ATTR_ITEM_DECAYING);}

protected:
	enum itemAttrTypes{
		ATTR_ITEM_ACTIONID = 1,
		ATTR_ITEM_UNIQUEID = 2,
		ATTR_ITEM_DESC = 4,
		ATTR_ITEM_TEXT = 8,

		ATTR_ITEM_OWNER = 65536,
		ATTR_ITEM_DURATION = 131072,
		ATTR_ITEM_DECAYING = 262144
	};

	bool hasAttribute(itemAttrTypes type) const;

private:
	static std::string emptyString;
	
	struct Attribute{
		itemAttrTypes type;
		void* value;
		Attribute* next;
		Attribute(itemAttrTypes _type){
			type = _type;
			value = NULL;
			next = NULL;
		}
	};
	
	uint32_t m_attributes;
	Attribute* m_firstAttr;
	
	const std::string& getStrAttr(itemAttrTypes type) const;
	void setStrAttr(itemAttrTypes type, const std::string& value);
	
	uint32_t getIntAttr(itemAttrTypes type) const;
	void setIntAttr(itemAttrTypes type, uint32_t value);
	void increaseIntAttr(itemAttrTypes type, uint32_t value);
	
	bool validateIntAttrType(itemAttrTypes type) const;
	bool validateStrAttrType(itemAttrTypes type) const;
	
	void addAttr(Attribute* attr);
	Attribute* getAttr(itemAttrTypes type) const;
	Attribute* getAttr(itemAttrTypes type);
	
	void deleteAttrs(Attribute* attr);
};

class Item : virtual public Thing, public ItemAttributes
{
public:
	//Factory member to create item of right type based on type
	static Item* CreateItem(const unsigned short _type, unsigned short _count = 1);
	static Item* CreateItem(PropStream& propStream);
	static Items items;

	// Constructor for items
	Item(const unsigned short _type, unsigned short _count = 0);
	//Item(const unsigned short _type);
	//Item();
	Item(const Item &i);

	virtual ~Item();

	virtual Item* getItem() {return this;};
	virtual const Item* getItem() const {return this;};
	virtual Container* getContainer() {return NULL;};
	virtual const Container* getContainer() const {return NULL;};
	virtual Teleport* getTeleport() {return NULL;};
	virtual const Teleport* getTeleport() const {return NULL;};
	virtual TrashHolder* getTrashHolder() {return NULL;};
	virtual const TrashHolder* getTrashHolder() const {return NULL;};
	virtual Mailbox* getMailbox() {return NULL;};
	virtual const Mailbox* getMailbox() const {return NULL;};
	virtual Door* getDoor() {return NULL;};
	virtual const Door* getDoor() const {return NULL;};
	virtual MagicField* getMagicField() {return NULL;};
	virtual const MagicField* getMagicField() const {return NULL;};

	//serialization
	virtual bool unserialize(xmlNodePtr p);
	virtual xmlNodePtr serialize();

	virtual bool readAttr(AttrTypes_t attr, PropStream& propStream);
	virtual bool unserializeAttr(PropStream& propStream);
	virtual bool unserializeItemNode(FileLoader& f, NODE node, PropStream& propStream);
	//virtual bool serializeItemNode();

	virtual bool serializeAttr(PropWriteStream& propWriteStream);

	virtual bool isPushable() const {return !isNotMoveable();};
	virtual int getThrowRange() const {return (isPickupable() ? 15 : 2);};

	virtual std::string getDescription(int32_t lookDistance) const;
	std::string getWeightDescription() const;

	unsigned short getID() const {return id;}
	unsigned short getClientID() const {return items[id].clientId;}
	virtual void setID(unsigned short newid) {id = newid;}

	WeaponType_t getWeaponType() const {return items[id].weaponType;}
	Ammo_t	getAmuType() const {return items[id].amuType;}

	virtual double getWeight() const;
	int getAttack() const {return items[id].attack;}
	int getArmor() const {return items[id].armor;}
	int getDefense() const {return items[id].defence;}
	int getSlotPosition() const {return items[id].slot_position;}
	int getRWInfo(int& maxlen) const;
	int getWorth() const;
	void getLight(LightInfo& lightInfo);

	bool hasProperty(enum ITEMPROPERTY prop) const;
	bool isBlocking() const {return items[id].blockSolid;}
	bool isStackable() const {return items[id].stackable;}
	bool isRune() const {return (items[id].group == ITEM_GROUP_RUNE);}
	bool isFluidContainer() const {return (items[id].isFluidContainer());}
	bool isAlwaysOnTop() const {return items[id].alwaysOnTop;}
	bool isGroundTile() const {return items[id].isGroundTile();}
	bool isSplash() const {return items[id].isSplash();}
	bool isMagicField() const {return items[id].isMagicField();}
	bool isNotMoveable() const {return !items[id].moveable;}
	bool isPickupable() const {return items[id].pickupable;}
	bool isWeapon() const {return (items[id].weaponType != WEAPON_NONE && items[id].weaponType != WEAPON_AMMO);}
	bool isUseable() const {return items[id].useable;}
	bool isHangable() const {return items[id].isHangable;}
	bool isRoteable() const {const ItemType& it = items[id]; return it.rotable && it.rotateTo;}
	bool isDoor() const {return items[id].isDoor();}

	bool floorChangeDown() const {return items[id].floorChangeDown;}
	bool floorChangeNorth() const {return items[id].floorChangeNorth;}
	bool floorChangeSouth() const {return items[id].floorChangeSouth;}
	bool floorChangeEast() const {return items[id].floorChangeEast;}
	bool floorChangeWest() const {return items[id].floorChangeWest;}

	const std::string& getName() const {return items[id].name;}

	// get the number of items
	unsigned short getItemCount() const {return count;}
	void setItemCount(uint8_t n) {count = n;}

	unsigned char getItemCountOrSubtype() const;
	void setItemCountOrSubtype(unsigned char n);
	bool hasSubType() const;

	unsigned char getItemCharge() const {return charges;};
	void setItemCharge(unsigned char n) {charges = n;};

	unsigned char getFluidType() const {return fluid;};
	void setFluidType(unsigned char n) {fluid = n;};
	
	void setUniqueId(unsigned short n);

	void setDefaultDuration();
	uint32_t getDefaultDuration() const;
	bool canDecay();

	virtual bool canRemove() const {return true;}
	virtual bool onTradeEvent(TradeEvents_t event, Player* owner){return true;};

	virtual void __startDecaying();

protected:
	unsigned short id;  // the same id as in ItemType
	unsigned char count; // number of stacked items
	unsigned char charges; //number of charges on the item
	unsigned char fluid; //fluid type
};

#endif

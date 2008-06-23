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
	BLOCKSOLID = 0,
	HASHEIGHT = 1,
	BLOCKPROJECTILE = 2,
	BLOCKPATHFIND = 3,
	ISVERTICAL = 4,
	ISHORIZONTAL = 5,
	MOVEABLE = 6,
	BLOCKINGANDNOTMOVEABLE = 7,
	SUPPORTHANGABLE = 8,
};

enum TradeEvents_t{
	ON_TRADE_TRANSFER,
	ON_TRADE_CANCEL,
};

enum ItemDecayState_t{
	DECAYING_FALSE = 0,
	DECAYING_TRUE,
	DECAYING_PENDING
};

/*from iomapotbm.h*/
#pragma pack(1)
struct TeleportDest{
	uint16_t _x;
	uint16_t _y;
	uint8_t	_z;
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
	ATTR_WRITTENDATE = 18,
	ATTR_WRITTENBY = 19,
};

class ItemAttributes{
public:
	ItemAttributes(){
		m_attributes = 0;
		m_firstAttr = NULL;
	}
	virtual ~ItemAttributes(){
		if(m_firstAttr){
			deleteAttrs(m_firstAttr);
		}
	}

	void setSpecialDescription(const std::string& desc) {setStrAttr(ATTR_ITEM_DESC, desc);}
	void resetSpecialDescription() {removeAttribute(ATTR_ITEM_DESC);}
	const std::string& getSpecialDescription() const {return getStrAttr(ATTR_ITEM_DESC);}

	void setText(const std::string& text) {setStrAttr(ATTR_ITEM_TEXT, text);}
	void resetText() {removeAttribute(ATTR_ITEM_TEXT);}
	const std::string& getText() const {return getStrAttr(ATTR_ITEM_TEXT);}

	void setWrittenDate(time_t n) {setIntAttr(ATTR_ITEM_WRITTENDATE, (uint32_t)n);}
	void resetWrittenDate() {removeAttribute(ATTR_ITEM_WRITTENDATE);}
	time_t getWrittenDate() const {return (time_t)getIntAttr(ATTR_ITEM_WRITTENDATE);}

	void setWriter(std::string _writer) {setStrAttr(ATTR_ITEM_WRITTENBY, _writer);}
	void resetWriter() {removeAttribute(ATTR_ITEM_WRITTENBY);}
	const std::string& getWriter() const {return getStrAttr(ATTR_ITEM_WRITTENBY);}

	void setActionId(uint16_t n) {if(n < 100) n = 100; setIntAttr(ATTR_ITEM_ACTIONID, n);}
	uint16_t getActionId() const {return getIntAttr(ATTR_ITEM_ACTIONID);}

	void setUniqueId(uint16_t n) {if(n < 1000) n = 1000; setIntAttr(ATTR_ITEM_UNIQUEID, n);}
	uint16_t getUniqueId() const {return getIntAttr(ATTR_ITEM_UNIQUEID);}

	void setOwner(uint32_t _owner) {setIntAttr(ATTR_ITEM_OWNER, _owner);}
	uint32_t getOwner() const {return getIntAttr(ATTR_ITEM_OWNER);}

	void setDuration(int32_t time) {setIntAttr(ATTR_ITEM_DURATION, time);}
	void decreaseDuration(int32_t time) {increaseIntAttr(ATTR_ITEM_DURATION, -time);}
	int32_t getDuration() const {return getIntAttr(ATTR_ITEM_DURATION);}

	void setDecaying(ItemDecayState_t decayState) {setIntAttr(ATTR_ITEM_DECAYING, decayState);}
	uint32_t getDecaying() const {return getIntAttr(ATTR_ITEM_DECAYING);}

protected:
	enum itemAttrTypes{
		ATTR_ITEM_ACTIONID = 1,
		ATTR_ITEM_UNIQUEID = 2,
		ATTR_ITEM_DESC = 4,
		ATTR_ITEM_TEXT = 8,
		ATTR_ITEM_WRITTENDATE = 16,
		ATTR_ITEM_WRITTENBY = 32,

		ATTR_ITEM_OWNER = 65536,
		ATTR_ITEM_DURATION = 131072,
		ATTR_ITEM_DECAYING = 262144
	};

	bool hasAttribute(itemAttrTypes type) const;
	void removeAttribute(itemAttrTypes type);

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
	void setIntAttr(itemAttrTypes type, int32_t value);
	void increaseIntAttr(itemAttrTypes type, int32_t value);

	bool validateIntAttrType(itemAttrTypes type) const;
	bool validateStrAttrType(itemAttrTypes type) const;

	void addAttr(Attribute* attr);
	Attribute* getAttrConst(itemAttrTypes type) const;
	Attribute* getAttr(itemAttrTypes type);

	void deleteAttrs(Attribute* attr);
};

class Item : virtual public Thing, public ItemAttributes
{
public:
	//Factory member to create item of right type based on type
	static Item* CreateItem(const uint16_t _type, uint16_t _count = 1);
	static Item* CreateItem(PropStream& propStream);
	static Items items;

	// Constructor for items
	Item(const uint16_t _type, uint16_t _count = 0);
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

	static std::string getDescription(const ItemType& it, int32_t lookDistance, const Item* item = NULL);
	static std::string getWeightDescription(const ItemType& it, double weight, uint32_t count = 1);

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

	uint16_t getID() const {return id;}
	uint16_t getClientID() const {return items[id].clientId;}
	void setID(uint16_t newid);

	WeaponType_t getWeaponType() const {return items[id].weaponType;}
	Ammo_t	getAmuType() const {return items[id].amuType;}
	int32_t	getShootRange() const {return items[id].shootRange;}

	virtual double getWeight() const;
	int getAttack() const {return items[id].attack;}
	int getArmor() const {return items[id].armor;}
	int getDefense() const {return items[id].defence;}
	int getExtraDef() const {return items[id].extraDef;}
	int getSlotPosition() const {return items[id].slot_position;}

	bool isReadable() const {return items[id].canReadText;}
	bool canWriteText() const {return items[id].canWriteText;}
	int32_t getMaxWriteLength() const {return items[id].maxTextLen;}

	int getWorth() const;
	void getLight(LightInfo& lightInfo);

	bool hasProperty(enum ITEMPROPERTY prop) const;
	bool isBlocking() const {return items[id].blockSolid;}
	bool isStackable() const {return items[id].stackable;}
	bool isRune() const {return items[id].isRune();}
	bool isFluidContainer() const {return (items[id].isFluidContainer());}
	bool isAlwaysOnTop() const {return items[id].alwaysOnTop;}
	bool isGroundTile() const {return items[id].isGroundTile();}
	bool isSplash() const {return items[id].isSplash();}
	bool isMagicField() const {return items[id].isMagicField();}
	bool isNotMoveable() const {return !items[id].moveable;}
	bool isPickupable() const {return items[id].pickupable;}
	bool isWeapon() const {return (items[id].weaponType != WEAPON_NONE);}
	bool isUseable() const {return items[id].useable;}
	bool isHangable() const {return items[id].isHangable;}
	bool isRoteable() const {const ItemType& it = items[id]; return it.rotable && it.rotateTo;}
	bool isDoor() const {return items[id].isDoor();}
	bool hasCharges() const {return items[id].charges != 0;}

	bool floorChangeDown() const {return items[id].floorChangeDown;}
	bool floorChangeNorth() const {return items[id].floorChangeNorth;}
	bool floorChangeSouth() const {return items[id].floorChangeSouth;}
	bool floorChangeEast() const {return items[id].floorChangeEast;}
	bool floorChangeWest() const {return items[id].floorChangeWest;}

	const std::string& getName() const {return items[id].name;}
	const std::string& getPluralName() const {return items[id].pluralName;}

	// get the number of items
	uint8_t getItemCount() const {return count;}
	void setItemCount(uint8_t n) {count = n;}

	uint8_t getItemCountOrSubtype() const;
	void setItemCountOrSubtype(uint8_t n);
	void setDefaultSubtype();
	bool hasSubType() const;
	uint8_t getSubType() const;

	uint8_t getItemCharge() const {return charges;};
	void setItemCharge(uint8_t n) {charges = n;};

	uint8_t getFluidType() const {return fluid;};
	void setFluidType(uint8_t n) {fluid = n;};

	void setUniqueId(uint16_t n);

	void setDefaultDuration(){
		uint32_t duration = getDefaultDuration();
		if(duration != 0){
			setDuration(duration);
		}
	}
	uint32_t getDefaultDuration() const {return items[id].decayTime * 1000;}
	bool canDecay();

	virtual bool canRemove() const {return true;}
	virtual bool canTransform() const {return true;}
	virtual bool onTradeEvent(TradeEvents_t event, Player* owner){return true;};

	virtual void __startDecaying();

protected:
	// If weight description is needed from outside of item class
	// use the other getWeightDescription
	std::string getWeightDescription(double weight) const;

	uint16_t id;  // the same id as in ItemType
	uint8_t count; // number of stacked items
	uint8_t charges; //number of charges on the item
	uint8_t fluid; //fluid type
};

#endif

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

#include "definitions.h"
#include "thing.h"
#include "items.h"
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <iostream>
#include <list>
#include <vector>

class Creature;
class Player;
class Container;
class Depot;
class Teleport;
class TrashHolder;
class Mailbox;
class Door;
class MagicField;
//[ added for beds system
class BedItem;
//]

enum ITEMPROPERTY
{
	BLOCKSOLID = 0,
	HASHEIGHT,
	BLOCKPROJECTILE,
	BLOCKPATH,
	ISVERTICAL,
	ISHORIZONTAL,
	MOVEABLE,
	IMMOVABLEBLOCKSOLID,
	IMMOVABLEBLOCKPATH,
	IMMOVABLENOFIELDBLOCKPATH,
	NOFIELDBLOCKPATH,
	SUPPORTHANGABLE
};

enum TradeEvents_t
{
	ON_TRADE_TRANSFER,
	ON_TRADE_CANCEL
};

enum ItemDecayState_t
{
	DECAYING_FALSE = 0,
	DECAYING_TRUE,
	DECAYING_PENDING
};

/*from iomapotbm.h*/
#pragma pack(1)
struct TeleportDest
{
	uint16_t _x;
	uint16_t _y;
	uint8_t	_z;
};
#pragma pack()

enum AttrTypes_t
{
	// attr 0 means end of attribute list
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
	ATTR_SLEEPERGUID = 20,
	ATTR_SLEEPSTART = 21,
	ATTR_CHARGES = 22,
	// This is NOT stored in serializeAttr, but rather used by IOMapSerialize
	// look at that code for the ugly hack that makes this work. :)
	ATTR_CONTAINER_ITEMS = 23
};

enum Attr_ReadValue
{
	ATTR_READ_CONTINUE,
	ATTR_READ_ERROR,
	ATTR_READ_END
};

class ItemAttributes
{
public:
	ItemAttributes();
	ItemAttributes(const ItemAttributes& i);
	virtual ~ItemAttributes();

	void setSpecialDescription(const std::string& desc);
	void resetSpecialDescription();
	const std::string& getSpecialDescription() const;

	void setText(const std::string& text);
	void resetText();
	const std::string& getText() const;

	void setWrittenDate(const time_t& n);
	void resetWrittenDate();
	time_t getWrittenDate() const;

	void setWriter(const std::string& _writer);
	void resetWriter();
	const std::string& getWriter() const;

	void setActionId(const uint16_t& n);
	uint16_t getActionId() const;

	void setUniqueId(const uint16_t& n);
	uint16_t getUniqueId() const;

	void setCharges(const uint16_t& n);
	uint16_t getCharges() const;

	void setFluidType(const uint16_t& n);
	uint16_t getFluidType() const;

	void setOwner(const uint32_t& _owner);
	uint32_t getOwner() const;

	void setCorpseOwner(const uint32_t& _corpseOwner);
	uint32_t getCorpseOwner();

	void setDuration(const int32_t& time);
	void decreaseDuration(const int32_t& time);
	int32_t getDuration() const;

	void setDecaying(const ItemDecayState_t& decayState);
	uint32_t getDecaying() const;

protected:
	enum itemAttrTypes
	{
		ATTR_ITEM_ACTIONID = 1 << 0,
		ATTR_ITEM_UNIQUEID = 1 << 1,
		ATTR_ITEM_DESC = 1 << 2,
		ATTR_ITEM_TEXT = 1 << 3,
		ATTR_ITEM_WRITTENDATE = 1 << 4,
		ATTR_ITEM_WRITTENBY = 1 << 5,
		ATTR_ITEM_OWNER = 1 << 6,
		ATTR_ITEM_DURATION = 1 << 7,
		ATTR_ITEM_DECAYING = 1 << 8,
		ATTR_ITEM_CORPSEOWNER = 1 << 9,
		ATTR_ITEM_CHARGES = 1 << 10,
		ATTR_ITEM_FLUIDTYPE = 1 << 11,
		ATTR_ITEM_DOORID = 1 << 12
	};

	bool hasAttribute(const itemAttrTypes& type) const;
	void removeAttribute(const itemAttrTypes& type);

protected:
	class Attribute
	{
	public:
		itemAttrTypes type;
		void* value;
		Attribute* next;
		Attribute(itemAttrTypes _type)
		{
			type = _type;
			value = NULL;
			next = NULL;
		}

		Attribute(const Attribute& i)
		{
			type = i.type;

			if (ItemAttributes::validateIntAttrType(type))
			{
				value = i.value;
			}
			else if (ItemAttributes::validateStrAttrType(type))
			{
				value = (void*)new std::string(*((std::string*)i.value));
			}
			else
			{
				value = NULL;
			}

			next = NULL;

			if (i.next)
			{
				next = new Attribute(*i.next);
			}
		}
	};

	const std::string& getStrAttr(const itemAttrTypes& type) const;
	void setStrAttr(const itemAttrTypes& type, const std::string& value);

	uint32_t getIntAttr(const itemAttrTypes& type) const;
	void setIntAttr(const itemAttrTypes& type, const int32_t& value);
	void increaseIntAttr(const itemAttrTypes& type, const int32_t& value);

	static bool validateIntAttrType(const itemAttrTypes& type);
	static bool validateStrAttrType(const itemAttrTypes& type);

	void addAttr(Attribute* attr);
	Attribute* getAttrConst(const itemAttrTypes& type) const;
	Attribute* getAttr(const itemAttrTypes& type);

	void deleteAttrs(Attribute* attr);

	uint32_t m_attributes;
	Attribute* m_firstAttr;
};

class Item : virtual public Thing, public ItemAttributes
{
public:
	Item(const uint16_t& _type, const uint16_t& _count = 0);
	Item(const Item& i);
	virtual ~Item();

	//Factory member to create item of right type based on type
	static Item* CreateItem(const uint16_t& _type, const uint16_t& _count = 0);
	static Item* CreateItem(PropStream& propStream);
	static bool loadItem(xmlNodePtr node, Container* parent);
	static bool loadContainer(xmlNodePtr node, Container* parent);

	static Items items;

	virtual Item* clone() const;
	virtual void copyAttributes(Item* item);

	virtual Item* getItem();
	virtual const Item* getItem() const;
	virtual Container* getContainer();
	virtual const Container* getContainer() const;
	virtual Teleport* getTeleport();
	virtual const Teleport* getTeleport() const;
	virtual TrashHolder* getTrashHolder();
	virtual const TrashHolder* getTrashHolder() const;
	virtual Mailbox* getMailbox();
	virtual const Mailbox* getMailbox() const;
	virtual Door* getDoor();
	virtual const Door* getDoor() const;
	virtual MagicField* getMagicField();
	virtual const MagicField* getMagicField() const;
	virtual BedItem* getBed();
	virtual const BedItem* getBed() const;

	static std::string getDescription(const ItemType& it, const int32_t& lookDistance,
	                                  const Item* item = NULL, int32_t subType = -1, bool addArticle = true);
	static std::string getLongName(const ItemType& it, const int32_t& lookDistance,
	                               const Item* item = NULL, int32_t subType = -1, bool addArticle = true);
	static std::string getWeightDescription(const ItemType& it, double weight, const uint32_t& count = 1);

	//serialization
	virtual Attr_ReadValue readAttr(const AttrTypes_t& attr, PropStream& propStream);
	virtual bool unserializeAttr(PropStream& propStream);
	virtual bool unserializeItemNode(FileLoader& f, NODE node, PropStream& propStream);
	virtual bool serializeAttr(PropWriteStream& propWriteStream) const;

	virtual bool isPushable() const;
	virtual int getThrowRange() const;

	virtual std::string getDescription(const int32_t& lookDistance) const;
	virtual std::string getXRayDescription() const;
	std::string getWeightDescription() const;

	const uint16_t& getID() const;
	const uint16_t& getClientID() const;
	void setID(const uint16_t& newid);

	// Returns the player that is holding this item in his inventory
	Player* getHoldingPlayer();
	const Player* getHoldingPlayer() const;

	const WeaponType_t& getWeaponType() const;
	const Ammo_t& getAmuType() const;
	const uint32_t& getShootRange() const;

	virtual double getWeight() const;
	const int& getAttack() const;
	const int& getArmor() const;
	const int& getDefense() const;
	const int& getExtraDef() const;
	const uint32_t& getSlotPosition() const;
	const uint16_t& getWieldPosition() const;
	const int& getHitChance() const;

	bool isReadable() const;
	bool canWriteText() const;
	const uint16_t& getMaxWriteLength() const;

	uint32_t getWorth() const;
	void getLight(LightInfo& lightInfo);

	bool hasProperty(const ITEMPROPERTY& prop) const;
	bool isBlocking(const Creature* creature) const;
	bool isStackable() const;
	bool isRune() const;
	bool isFluidContainer() const;
	bool isAlwaysOnTop() const;
	bool isGroundTile() const;
	bool isSplash() const;
	bool isMagicField() const;
	bool isNotMoveable() const;
	bool isMoveable() const;
	bool isPickupable() const;
	bool isWeapon() const;
	bool isUseable() const;
	bool isHangable() const;
	bool isRoteable() const;
	bool isDoor() const;
	bool isBed() const;
	bool isLevelDoor() const;
	bool hasCharges() const;
	bool isSolidForItems() const;

	bool floorChangeDown() const;
	bool floorChangeNorth() const;
	bool floorChangeSouth() const;
	bool floorChangeEast() const;
	bool floorChangeWest() const;

	const std::string& getName() const;
	const std::string& getPluralName() const;
	std::string getLongName() const;

	// get the number of items
	const uint8_t& getItemCount() const;
	void setItemCount(const uint8_t& n);

	static uint32_t countByType(const Item* i, const int32_t& subType);

	void setDefaultSubtype();
	bool hasSubType() const;
	uint16_t getSubType() const;
	void setSubType(const uint16_t& n);

	void setUniqueId(const uint16_t& n);

	void setDefaultDuration();
	uint32_t getDefaultDuration() const;
	bool canDecay();

	virtual bool canRemove() const;
	virtual bool canTransform() const;
	virtual void onRemoved();
	virtual bool onTradeEvent(const TradeEvents_t& event, Player* owner);

	virtual void __startDecaying();

protected:
	// If weight description is needed from outside of item class
	// use the other getWeightDescription
	std::string getWeightDescription(const double& weight) const;

	uint16_t id;  // the same id as in ItemType
	uint8_t count; // number of stacked items

	//Don't add variables here, use the ItemAttribute class.
};

typedef std::list<Item*> ItemList;

#endif

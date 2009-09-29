//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
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

#include "classes.h"
#include "items.h"
#include "item_attributes.h"

enum TradeEvents_t{
	ON_TRADE_TRANSFER,
	ON_TRADE_CANCEL
};

enum ItemDecayState_t{
	DECAYING_FALSE = 0,
	DECAYING_TRUE,
	DECAYING_PENDING
};

enum ItemProp{
	ITEMPROP_BLOCKSOLID			= 1 << 0,
	ITEMPROP_BLOCKPATHFIND		= 1 << 1,
	ITEMPROP_BLOCKPROJECTILE	= 1 << 2,
	ITEMPROP_ALLOWPICKUPABLE	= 1 << 3,
	ITEMPROP_HASHEIGHT			= 1 << 4,
	ITEMPROP_ISVERTICAL			= 1 << 5,
	ITEMPROP_ISHORIZONTAL		= 1 << 6,
	ITEMPROP_ISHANGEABLE		= 1 << 7,
	ITEMPROP_CLIENTCHARGES		= 1 << 8,
	ITEMPROP_LOOKTHROUGH		= 1 << 9,
	ITEMPROP_PICKUPABLE			= 1 << 10,
	ITEMPROP_ROTATEABLE			= 1 << 11,
	ITEMPROP_STACKABLE			= 1 << 12,
	ITEMPROP_USEABLE			= 1 << 13,
	ITEMPROP_MOVEABLE			= 1 << 14,
	ITEMPROP_ALWAYSONTOP		= 1 << 15,
	ITEMPROP_CANREADTEXT		= 1 << 16,
	ITEMPROP_CANWRITETEXT		= 1 << 17,
	ITEMPROP_FLOORCHANGEDOWN	= 1 << 18,
	ITEMPROP_FLOORCHANGENORTH	= 1 << 19,
	ITEMPROP_FLOORCHANGESOUTH	= 1 << 20,
	ITEMPROP_FLOORCHANGEEAST	= 1 << 21,
	ITEMPROP_FLOORCHANGEWEST	= 1 << 22,
	ITEMPROP_ALLOWDISTREAD		= 1 << 23
};

/*from iomapotbm.h*/
#pragma pack(1)
struct TeleportDest{
	uint16_t _x;
	uint16_t _y;
	uint8_t	_z;
};
#pragma pack()

// These are here for backwards-compatibility
enum AttrTypes_t{
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
	ATTR_CONTAINER_ITEMS = 23,

	ATTR_ATTRIBUTE_MAP = 128
};

enum Attr_ReadValue{
	ATTR_READ_CONTINUE,
	ATTR_READ_ERROR,
	ATTR_READ_END
};

class Item : virtual public Thing, public ItemAttributes
{
public:
	//Factory member to create item of right type based on type
	static Item* CreateItem(const uint16_t _type, uint16_t _count = 1);
	static Item* CreateItem(PropStream& propStream);
	static bool loadItem(xmlNodePtr node, Container* parent);
	static bool loadContainer(xmlNodePtr node, Container* parent);

	static Items items;

	// Constructor for items
	Item(const uint16_t _type, uint16_t _count = 0);
	Item(const Item &i);
	virtual Item* clone() const;
	virtual void copyAttributes(Item* item);

	virtual ~Item();

	virtual Item* getItem() {return this;}
	virtual const Item* getItem() const {return this;}
	virtual Container* getContainer() {return NULL;}
	virtual const Container* getContainer() const {return NULL;}
	virtual Teleport* getTeleport() {return NULL;}
	virtual const Teleport* getTeleport() const {return NULL;}
	virtual TrashHolder* getTrashHolder() {return NULL;}
	virtual const TrashHolder* getTrashHolder() const {return NULL;}
	virtual Door* getDoor() {return NULL;}
	virtual const Door* getDoor() const {return NULL;}
	virtual MagicField* getMagicField() {return NULL;}
	virtual const MagicField* getMagicField() const {return NULL;}
	virtual BedItem* getBed(){ return NULL; }
	virtual const BedItem* getBed() const { return NULL; }

	static std::string getDescription(const ItemType& it, int32_t lookDistance,
		const Item* item = NULL, int32_t subType = -1, bool addArticle = true);
	static std::string getLongName(const ItemType& it, int32_t lookDistance,
		const Item* item = NULL, int32_t subType = -1, bool addArticle = true);
	static std::string getWeightDescription(const ItemType& it, double weight, uint32_t count = 1);

	//serialization
	virtual Attr_ReadValue readAttr(AttrTypes_t attr, PropStream& propStream);
	virtual bool unserializeAttr(PropStream& propStream);
	virtual bool unserializeItemNode(FileLoader& f, NODE node, PropStream& propStream);
	virtual bool serializeAttr(PropWriteStream& propWriteStream) const;

	virtual bool isPushable() const {return isMoveable();}
	virtual int getThrowRange() const {return (isPickupable() ? 15 : 2);}

	virtual std::string getDescription(int32_t lookDistance) const;
	std::string getWeightDescription() const;

	uint16_t getID() const {return id;}
	uint16_t getClientID() const {return items[id].clientId;}
	void setID(uint16_t newid);

	// Returns the player that is holding this item in his inventory
	Player* getHoldingPlayer();
	const Player* getHoldingPlayer() const;

	// Item attributes
	void setSpecialDescription(const std::string& desc);
	void clearSpecialDescription();
	std::string getSpecialDescription() const;

	void setText(const std::string& text);
	void clearText();
	std::string getText() const;

	void setWrittenDate(time_t n);
	void clearWrittenDate();
	time_t getWrittenDate() const;

	void setWriter(std::string _writer);
	void clearWriter();
	std::string getWriter() const;

	void setActionId(int32_t n);
	int32_t getActionId() const;

	void setCharges(uint16_t n);
	uint16_t getCharges() const;

	void setFluidType(uint16_t n);
	uint16_t getFluidType() const;

	void setOwner(uint32_t _owner);
	uint32_t getOwner() const;

	void setCorpseOwner(uint32_t _corpseOwner);
	uint32_t getCorpseOwner();

	void setDuration(int32_t time);
	void decreaseDuration(int32_t time);
	int32_t getDuration() const;

	void setDecaying(ItemDecayState_t decayState);
	ItemDecayState_t getDecaying() const;

	virtual double getWeight() const;
	int getAttack() const;
	int getArmor() const;
	int getDefense() const;
	int getExtraDef() const;
	int getHitChance() const;

	uint32_t getWorth() const;
	void getLight(LightInfo& lightInfo);

	const std::string& getName() const;
	const std::string& getPluralName() const;
	const std::string& getArticle() const;
	std::string getLongName() const;

	// get the number of items
	uint16_t getItemCount() const {return count;}
	void setItemCount(uint8_t n) {count = n;}

	static uint32_t countByType(const Item* i, int checkType, bool multiCount);

	void setDefaultSubtype();
	bool hasSubType() const;
	uint16_t getSubType() const;
	void setSubType(uint16_t n);

	void setDefaultDuration(){
		uint32_t duration = getDefaultDuration();
		if(duration != 0){
			setDuration(duration);
		}
	}

	bool canDecay();
	bool isTradeable() const {return true;}

	virtual bool canRemove() const {return true;}
	virtual bool canTransform() const {return true;}
	virtual void onRemoved();
	virtual bool onTradeEvent(TradeEvents_t event, Player* owner){return true;}

	bool hasProperty(uint32_t props) const;

	// "const" properties
	ItemTypes_t getType() const {return items[id].type;}
	bool blockSolid() const {return items[id].blockSolid;}
	bool blockPathFind() const {return items[id].blockPathFind;}
	bool blockProjectile() const {return items[id].blockProjectile;}
	bool isStackable() const {return items[id].stackable;}
	bool isRune() const {return items[id].isRune();}
	bool isFluidContainer() const {return (items[id].isFluidContainer());}
	bool isAlwaysOnTop() const {return items[id].alwaysOnTop;}
	bool isGroundTile() const {return items[id].isGroundTile();}
	bool isSplash() const {return items[id].isSplash();}
	bool isMagicField() const {return items[id].isMagicField();}
	bool isMoveable() const {return items[id].moveable;}
	bool isPickupable() const {return items[id].pickupable;}
	bool isWeapon() const {return (items[id].weaponType != WEAPON_NONE);}
	bool isUseable() const {return items[id].useable;}
	bool isHangable() const {return items[id].isHangable;}
	bool isRotateable() const {const ItemType& it = items[id]; return it.rotateable && it.rotateTo;}
	bool isDoor() const {return items[id].isDoor();}
	bool isBed() const {return items[id].isBed();}
	bool hasCharges() const {return items[id].charges != 0;}
	bool hasHeight() const {return items[id].hasHeight;}
	bool floorChangeDown() const {return items[id].floorChangeDown;}
	bool floorChangeNorth() const {return items[id].floorChangeNorth;}
	bool floorChangeSouth() const {return items[id].floorChangeSouth;}
	bool floorChangeEast() const {return items[id].floorChangeEast;}
	bool floorChangeWest() const {return items[id].floorChangeWest;}
	
	int32_t getTopOrder() const {return items[id].alwaysOnTopOrder;}
	SlotPosition getSlotPosition() const {return items[id].slotPosition;}
	SlotType getWieldPosition() const {return items[id].wieldPosition;}
	bool isReadable() const {return items[id].canReadText;}
	bool canWriteText() const {return items[id].canWriteText;}
	uint16_t getMaxWriteLength() const {return items[id].maxTextLen;}
	uint32_t getDefaultDuration() const {return items[id].decayTime * 1000;}

	WeaponType_t getWeaponType() const {return items[id].weaponType;}
	Weapon* getWeapon() const {return items[id].weaponInstance;}
	Ammo_t	getAmmoType() const {return items[id].ammoType;}
	int32_t	getShootRange() const {return items[id].shootRange;}

protected:
	// If weight description is needed from outside of item class
	// use the other getWeightDescription
	std::string getWeightDescription(double weight) const;

	uint16_t id;  // the same id as in ItemType
	uint8_t count; // number of stacked items

	//Don't add variables here, use the ItemAttribute class.
};

typedef std::list<Item *> ItemList;

inline uint32_t Item::countByType(const Item* i, int checkType, bool multiCount){
	if(checkType == -1 || checkType == i->getSubType()){

		if(multiCount)
			return i->getItemCount();

		if(i->isRune())
			return i->getCharges();

		return i->getItemCount();
	}
	return 0;
}

inline int Item::getAttack() const {
	const int32_t* attack = getIntegerAttribute("attack");
	if(attack)
		return (int)(*attack);
	return items[id].attack;
}

inline int Item::getArmor() const {
	const int32_t* armor = getIntegerAttribute("armor");
	if(armor)
		return (int)(*armor);
	return items[id].armor;
}

inline int Item::getDefense() const {
	const int32_t* defense = getIntegerAttribute("defense");
	if(defense)
		return (int)(*defense);
	return items[id].defense;
}

inline int Item::getExtraDef() const {
	const int32_t* extraDefense = getIntegerAttribute("extraDefense");
	if(extraDefense)
		return (int)(*extraDefense);
	return items[id].extraDefense;
}

inline int Item::getHitChance() const {
	const int32_t* hitChance = getIntegerAttribute("hitChance");
	if(hitChance)
		return (int)(*hitChance);
	return items[id].hitChance;
}

inline const std::string& Item::getName() const {
	const std::string* name = getStringAttribute("name");
	if(name)
		return *name;
	return items[id].name;
}

inline const std::string& Item::getPluralName() const {
	const std::string* pluralname = getStringAttribute("pluralname");
	if(pluralname)
		return *pluralname;
	return items[id].pluralName;
}

inline const std::string& Item::getArticle() const {
	const std::string* article = getStringAttribute("article");
	if(article)
		return *article;
	return items[id].article;
}

inline void Item::setSpecialDescription(const std::string& desc) {
	setAttribute("desc", desc);
}

inline void Item::clearSpecialDescription() {
	eraseAttribute("desc");
}

inline std::string Item::getSpecialDescription() const {
	const std::string* desc = getStringAttribute("desc");
	if(desc)
		return *desc;
	return "";
}

inline void Item::setText(const std::string& text) {
	setAttribute("text", text);
}

inline void Item::clearText() {
	eraseAttribute("text");
}

inline std::string Item::getText() const {
	const std::string* text = getStringAttribute("text");
	if(text)
		return *text;
	return "";
}

inline void Item::setWrittenDate(time_t n) {
	setAttribute("writtendate", (int32_t)n);
}

inline void Item::clearWrittenDate() {
	eraseAttribute("writtendate");
}

inline time_t Item::getWrittenDate() const {
	const int32_t* date = getIntegerAttribute("writtendate");
	if(date)
		return (time_t)*date;
	return 0;
}

inline void Item::setWriter(std::string _writer) {
	setAttribute("writer", _writer);
}

inline void Item::clearWriter() {
	eraseAttribute("writer");
}

inline std::string Item::getWriter() const {
	const std::string* writer = getStringAttribute("writer");
	if(writer)
		return *writer;
	return "";
}

inline int32_t Item::getActionId() const {
	const int32_t* aid = getIntegerAttribute("aid");
	if(aid)
		return *aid;
	return 0;
}

inline void Item::setCharges(uint16_t n) {
	setAttribute("charges", (int32_t)n);
}

inline uint16_t Item::getCharges() const {
	const int32_t* charges = getIntegerAttribute("charges");
	if(charges && *charges >= 0)
		return (uint16_t)*charges;
	return 0;
}

inline void Item::setFluidType(uint16_t n) {
	setAttribute("fluidtype", (int32_t)n);
}

inline uint16_t Item::getFluidType() const {
	const int32_t* fluidtype = getIntegerAttribute("fluidtype");
	if(fluidtype && *fluidtype >= 0)
		return (uint16_t)*fluidtype;
	return 0;
}

inline void Item::setOwner(uint32_t _owner) {
	setAttribute("owner", (int32_t)_owner);
}

inline uint32_t Item::getOwner() const {
	const int32_t* owner = getIntegerAttribute("owner");
	if(owner)
		return (uint32_t)*owner;
	return 0;
}

inline void Item::setCorpseOwner(uint32_t _corpseOwner) {
	setAttribute("corpseowner", (int32_t)_corpseOwner);
}

inline uint32_t Item::getCorpseOwner() {
	const int32_t* owner = getIntegerAttribute("corpseowner");
	if(owner)
		return (uint32_t)*owner;
	return 0;
}

inline void Item::setDuration(int32_t time) {
	setAttribute("duration", time);
}

inline void Item::decreaseDuration(int32_t time) {
	const int32_t* duration = getIntegerAttribute("duration");
	if(duration)
		setAttribute("duration", *duration - time);
}

inline int32_t Item::getDuration() const {
	const int32_t* duration = getIntegerAttribute("duration");
	if(duration)
		return *duration;
	return 0;
}


inline void Item::setDecaying(ItemDecayState_t decayState) {
	setAttribute("decaystate", (int32_t)decayState);
}

inline ItemDecayState_t Item::getDecaying() const {
	const int32_t* state = getIntegerAttribute("decaystate");
	if(state)
		return (ItemDecayState_t)(*state);
	return DECAYING_FALSE;
}

#endif

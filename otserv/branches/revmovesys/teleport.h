#ifndef __TELEPORT_H__
#define __TELEPORT_H__

#include "tile.h"

class Teleport : public Item, public Cylinder
{
public:
	Teleport(uint16_t _type);
	~Teleport();
	
	virtual Teleport* getTeleport() {return this;};
	virtual const Teleport* getTeleport() const {return this;};
	
	void setDestPos(const Position &pos) {destPos = pos;};
	const Position& getDestPos() const {return destPos;};

	//cylinder implementations
	virtual ReturnValue __queryAdd(int32_t index, const Thing* thing, uint32_t count,
		bool childIsOwner = false) const;
	virtual ReturnValue __queryMaxCount(int32_t index, const Thing* thing, uint32_t count,
		uint32_t& maxQueryCount) const;
	virtual ReturnValue __queryRemove(const Thing* thing, uint32_t count) const;
	virtual Cylinder* __queryDestination(int32_t& index, const Thing* thing, Item** destItem);

	virtual void __addThing(Thing* thing);
	virtual void __addThing(int32_t index, Thing* thing);

	virtual void __updateThing(Thing* thing, uint32_t count);
	virtual void __updateThing(uint32_t index, Thing* thing);

	virtual void __removeThing(Thing* thing, uint32_t count);

	virtual int32_t __getIndexOfThing(const Thing* thing) const;
	virtual Thing* __getThing(uint32_t index) const;

	virtual void postAddNotification(Thing* thing, bool hasOwnership = true);
	virtual void postRemoveNotification(Thing* thing, bool hadOwnership = true);

	virtual void __internalAddThing(Thing* thing);
	virtual void __internalAddThing(uint32_t index, Thing* thing);

private:
	virtual int unserialize(xmlNodePtr p);
	virtual xmlNodePtr serialize();
	Position destPos;
};

#endif

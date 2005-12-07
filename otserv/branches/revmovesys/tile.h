//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// a Tile represents a single field on the map.
// it is a list of Items
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


#ifndef __TILE_H__
#define __TILE_H__


#include "item.h"
#include "cylinder.h"
#include "magic.h"

#include "definitions.h"
#include "templates.h"
#include "scheduler.h"

class Creature;

typedef std::vector<Item*> ItemVector;
typedef std::vector<Creature*> CreatureVector;

class Tile : public Cylinder
{
public:
  Creature* getCreature() const{
		if(creatures.size())
			return creatures[0];
		else
			return NULL;
  }

  Tile(int x, int y, int z)
  {
		tilePos.x = x;
		tilePos.y = y;
		tilePos.z = z;

    pz     = false;
    splash = NULL;
		ground = NULL;
  }

	virtual int getThrowRange() const {return 0;};
	virtual bool isPushable() const {return false;};

  Item*          ground;
  Item*          splash;
  ItemVector     topItems;
  CreatureVector creatures;
  ItemVector     downItems;

  bool removeThing(Thing *thing);
  void addThing(Thing *thing);
	MagicEffectItem* getFieldItem();
	Teleport* getTeleportItem() const;

	Thing* getTopMoveableThing();
	Creature* getTopCreature();
	Item* getTopTopItem();
	Item* getTopDownItem();
	Item* getMoveableBlockingItem();
  Thing* getTopThing();
	
	//bool insertThing(Thing *thing, int stackpos);
  //int getCreatureStackPos(Creature *c) const;
  //int getThingStackPos(const Thing *thing) const;
	//Thing* getThingByStackPos(int pos);
	int getThingCount() const;

	ReturnValue isBlocking(int objectstate, bool ignoreCreature = false, bool ignoreMoveableBlocking = false) const;

  bool isPz() const;
  void setPz();
  
  bool floorChange() const;
  bool floorChangeDown() const;
  bool floorChange(Direction direction) const;
  
  virtual std::string getDescription(uint32_t lookDistance) const;

	//
	virtual ReturnValue __queryMaxCount(int32_t index, const Thing* thing, uint32_t count,
		uint32_t& maxQueryCount) const;
	virtual ReturnValue __queryAdd(uint32_t index, const Thing* thing, uint32_t count) const;
	virtual ReturnValue __queryRemove(const Thing* thing, uint32_t count) const;
	virtual Cylinder* __queryDestination(int32_t& index, const Thing* thing, Thing** destThing);

	virtual ReturnValue __addThing(Thing* thing);
	virtual ReturnValue __addThing(uint32_t index, Thing* thing);

	virtual ReturnValue __updateThing(Thing* thing, uint32_t count);
	virtual ReturnValue __updateThing(uint32_t index, Thing* thing);

	virtual ReturnValue __removeThing(Thing* thing, uint32_t count);

	virtual int32_t __getIndexOfThing(const Thing* thing) const;
	virtual Thing* __getThing(uint32_t index);
	virtual Thing* __getThing(uint32_t index) const;

	virtual void postAddNotification(const Thing* thing, bool hasOwnership = true);
	virtual void postRemoveNotification(const Thing* thing, bool hadOwnership = true);

	virtual void __internalAddThing(Thing* thing);
	virtual void __internalAddThing(uint32_t index, Thing* thing);

	const Position& getTilePosition() const {return tilePos;};

protected:
	Position tilePos;
  bool pz;
};


#endif // #ifndef __TILE_H__


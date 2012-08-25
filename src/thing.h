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

#ifndef __OTSERV_THING_H__
#define __OTSERV_THING_H__

#include <string>
#include <stdint.h>

// Forward declaration
class Cylinder;
class Tile;
class Position;
class Creature;
class Item;

class Thing {
protected:
	Thing();

public:
	virtual ~Thing();

	void addRef();
	void unRef();

	virtual std::string getDescription(int32_t lookDistance) const = 0;

	Cylinder* getParent();
	const Cylinder* getParent() const;

	virtual void setParent(Cylinder* cylinder);

	Cylinder* getTopParent(); //returns Tile/Container or a Player
	const Cylinder* getTopParent() const;

	virtual Tile* getParentTile();
	virtual const Tile* getParentTile() const;

	virtual Position getPosition() const;
	virtual int getThrowRange() const = 0;
	virtual bool isPushable() const = 0;

	virtual Item* getItem();
	virtual const Item* getItem() const;
	virtual Tile* getTile();
	virtual const Tile* getTile() const;
	virtual Creature* getCreature();
	virtual const Creature* getCreature() const;

	virtual bool isRemoved() const;

private:
	Cylinder* parent;
	int32_t m_refCount;
};

#endif //__THING_H__

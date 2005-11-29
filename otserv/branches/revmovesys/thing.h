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

#ifndef __THING_H__
#define __THING_H__

#include "definitions.h"
#include "position.h"

enum BlockState {
 BLOCK_SOLID = 1,
 BLOCK_PROJECTILE = 2, 
 BLOCK_PATHFIND =  4, 
 BLOCK_PICKUPABLE = 8,
 BLOCK_PZ = 16
};

enum ReturnValue {
	RET_NOERROR,
	RET_NOMOVEABLE,
	RET_NOTPOSSIBLE,
	RET_NOTENOUGHROOM,
	RET_PROTECTIONZONE,
	RET_CANNOTTHROW,
	RET_THEREISNOWAY,
	RET_NOTILE,
	RET_DESTINATIONOUTOFREACH,
	RET_CREATUREBLOCK,
	RET_NOTMOVEABLE,
	RET_DROPTWOHANDEDITEM,
	RET_INVENTORYALREADYEQUIPPED,
	RET_CANNOTBEDRESSED,
	RET_TOFARAWAY,
	RET_FIRSTGODOWNSTAIRS,
	RET_FIRSTGOUPSTAIRS
};

class Tile;
class Cylinder;

class Thing {
public:
	Thing();
	virtual ~Thing();

	void useThing2();
	void releaseThing2();
	
	virtual std::string getDescription(uint32_t lookDistance) const = 0;

	Cylinder* getParent() {return parent;};
	const Cylinder* getParent() const {return parent;};

	void setParent(Cylinder* cylinder) {parent = cylinder;};

	Cylinder* getTopParent(); //returns Tile/Container or a Player
	const Cylinder* getTopParent() const;

	Tile* getTile();
	const Tile* getTile() const;

	const Position& getPosition() const;
	virtual int getThrowRange() const = 0;
	virtual bool isPushable() const = 0;

	bool isRemoved() const {return parent == NULL;};

private:
	Cylinder* parent;
	uint32_t useCount;
};


#endif //__THING_H__

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

enum ReturnValue{
	RET_NOERROR,
	RET_NOTPOSSIBLE,
	RET_NOTENOUGHROOM,
	RET_PLAYERISPZLOCKED,
	RET_PLAYERISNOTINVITED,
	RET_CANNOTTHROW,
	RET_THEREISNOWAY,
	RET_DESTINATIONOUTOFREACH,
	RET_CREATUREBLOCK,
	RET_NOTMOVEABLE,
	RET_DROPTWOHANDEDITEM,
	RET_BOTHHANDSNEEDTOBEFREE,
	RET_CANONLYUSEONEWEAPON,
	RET_NEEDEXCHANGE,
	RET_CANNOTBEDRESSED,
	RET_PUTTHISOBJECTINYOURHAND,
	RET_PUTTHISOBJECTINBOTHHANDS,
	RET_TOOFARAWAY,
	RET_FIRSTGODOWNSTAIRS,
	RET_FIRSTGOUPSTAIRS,
	RET_CONTAINERNOTENOUGHROOM,
	RET_NOTENOUGHCAPACITY,
	RET_CANNOTPICKUP,
	RET_THISISIMPOSSIBLE,
	RET_DEPOTISFULL,

	RET_CANNOTUSETHISOBJECT,
	RET_NOTREQUIREDLEVELTOUSERUNE,
	RET_YOUAREALREADYTRADING,
	RET_THISPLAYERISALREADYTRADING,
	RET_YOUMAYNOTLOGOUTDURINGAFIGHT,
	RET_DIRECTPLAYERSHOOT,
	RET_CANONLYUSETHISRUNEONCREATURES,
	RET_ACTIONNOTPERMITTEDINPROTECTIONZONE
};

class Tile;
class Cylinder;
class Item;
class Creature;

class Thing {
public:
	Thing();
	virtual ~Thing();

	void useThing2();
	void releaseThing2();
	
	virtual std::string getDescription(int32_t lookDistance) const = 0;

	Cylinder* getParent() {return parent;};
	const Cylinder* getParent() const {return parent;};

	virtual void setParent(Cylinder* cylinder) {parent = cylinder;};

	Cylinder* getTopParent(); //returns Tile/Container or a Player
	const Cylinder* getTopParent() const;

	Tile* getTile();
	const Tile* getTile() const;

	const Position& getPosition() const;
	virtual int getThrowRange() const = 0;
	virtual bool isPushable() const = 0;

	virtual Item* getItem() {return NULL;};
	virtual const Item* getItem() const {return NULL;};
	virtual Creature* getCreature() {return NULL;};
	virtual const Creature* getCreature() const {return NULL;};

	virtual bool isRemoved() const;

private:
	Cylinder* parent;
	int32_t useCount;
};


#endif //__THING_H__

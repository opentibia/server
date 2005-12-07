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

#ifndef __CYLINDER_H__
#define __CYLINDER_H__

#include "definitions.h"
#include "thing.h"

class Creature;

class Cylinder : virtual public Thing{
public:	
	virtual ReturnValue __queryMaxCount(int32_t index, const Thing* thing, uint32_t count,
		uint32_t& maxQueryCount) const = 0;
	virtual ReturnValue __queryAdd(uint32_t index, const Thing* thing, uint32_t count) const = 0;
	virtual ReturnValue __queryRemove(const Thing* thing, uint32_t count) const = 0;
	virtual Cylinder* __queryDestination(int32_t& index, const Thing* thing, Thing** destThing) = 0;

	virtual ReturnValue __addThing(Thing* thing) = 0;
	virtual ReturnValue __addThing(uint32_t index, Thing* thing) = 0;

	virtual ReturnValue __updateThing(Thing* thing, uint32_t count) = 0;
	virtual ReturnValue __updateThing(uint32_t index, Thing* thing) = 0;

	virtual ReturnValue __removeThing(Thing* thing, uint32_t count) = 0;

	virtual void postAddNotification(const Thing* thing, bool hasOwnership = true) = 0;
	virtual void postRemoveNotification(const Thing* thing, bool hadOwnership = true) = 0;

	virtual int32_t __getIndexOfThing(const Thing* thing) const = 0;
	virtual Thing* __getThing(uint32_t index) = 0;
	virtual Thing* __getThing(uint32_t index) const = 0;
	
	virtual void __internalAddThing(Thing* thing) = 0;
	virtual void __internalAddThing(uint32_t index, Thing* thing) = 0;

private:
};

#endif

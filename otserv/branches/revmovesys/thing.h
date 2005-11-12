


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
	RET_NOTENOUGHROOM,
	RET_PROTECTIONZONE,
	RET_CANNOTTHROW,
	RET_THEREISNOWAY,
	RET_NOTILE,
	RET_CREATUREBLOCK
};

class Tile;


class Thing {
public:
	Thing();
	virtual ~Thing();

	virtual bool canMovedTo(const Tile* tile) const;
	virtual void useThing() = 0;
	virtual void releaseThing() = 0;

	int throwRange;
	bool isRemoved;

	Position pos;
};


#endif // #ifndef __THING_H__

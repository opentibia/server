#ifndef __OTSERV_POS_H
#define __OTSERV_POS_H
//////////////////////////////////////////////////
// represents a map position
// for now just a 3d point
#include <stdlib.h>
#include <cmath>
#include <iostream>

enum slots_t {
	SLOT_WHEREEVER=0,
	SLOT_HEAD=1,
	SLOT_NECKLACE=2,
	SLOT_BACKPACK=3,
	SLOT_ARMOR=4,
	SLOT_RIGHT=5,
	SLOT_LEFT=6,
	SLOT_LEGS=7,
	SLOT_FEET=8,
	SLOT_RING=9,
	SLOT_AMMO=10
};

enum Direction { NORTH = 0, EAST = 1, SOUTH = 2, WEST = 3 };

class Position {
public:
  int x,y,z;


	bool operator==(const Position p)  const {
		if(p.x==x && p.y==y && p.z ==z)
			return true;
		else
			return false;
	}

	Position operator-(const Position p1){
		return Position(x-p1.x, y-p1.y,z-p1.z);
	}

	void dist(){
		x=abs(x);
		y=abs(y);
		z=abs(z);
	}
/*
	bool operator==(const position p){
		return (x==p.x && y== p.x && z==p.z);
	}*/

    // for now we just initialise the position to a startpoint
    Position() : x(223), y(223), z(7) { };

    Position(int _x, int _y, int _z)
        : x(_x), y(_y), z(_z) {};

};

std::ostream& operator<<(std::ostream&, const Position&);
std::ostream& operator<<(std::ostream&, const Direction&);

#endif

#ifndef __OTSERV_POS_H
#define __OTSERV_POS_H
//////////////////////////////////////////////////
// represents a map position
// for now just a 3d point
#include <stdlib.h>
#include <cmath>
#include <iostream>


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

	bool operator!=(const Position p)  const {
		if(p.x==x && p.y==y && p.z ==z)
			return false;
		else
			return true;
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
  Position() : x(31), y(31), z(7) { };

    Position(int _x, int _y, int _z)
        : x(_x), y(_y), z(_z) {};

};

std::ostream& operator<<(std::ostream&, const Position&);
std::ostream& operator<<(std::ostream&, const Direction&);

#endif

#ifndef __OTSERV_POS_H
#define __OTSERV_POS_H
//////////////////////////////////////////////////
// represents a map position
// for now just a 3d point
#include <stdlib.h>
#include <math.h>

struct position {
    int x,y,z;

	bool operator==(const position p)  const {
		if(p.x==x && p.y==y && p.z ==z)
			return true;
		else
			return false;
	}

	position operator-(const position p1){
		return position(x-p1.x, y-p1.y,z-p1.z);
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
    position() : x(32864), y(32864), z(7) { };

    position(int _x, int _y, int _z)
        : x(_x), y(_y), z(_z) {};

};

#endif

#include "position.h"
#include <iomanip>

std::ostream& operator<<(std::ostream& os, const Position& pos) {
		  os << "( " << std::setw(5) << std::setfill('0') << pos.x;
		  os << " / " << std::setw(5) << std::setfill('0') << pos.y;
		  os << " / " << std::setw(3) << std::setfill('0') << pos.z;
		  os << " )";
}

std::ostream& operator<<(std::ostream& os, const Direction& dir) {
		  switch (dir) { 
					 case NORTH:
								os << "North";
								break;
					 case EAST:
								os << "East";
								break;
					 case WEST:
								os << "West";
								break;
					 case SOUTH:
								os << "South";
								break;
		  }
		  return os;
}

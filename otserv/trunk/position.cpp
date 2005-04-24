#include "position.h"
#include <iomanip>

std::ostream& operator<<(std::ostream& os, const Position& pos) {
		  os << "( " << std::setw(5) << std::setfill('0') << pos.x;
		  os << " / " << std::setw(5) << std::setfill('0') << pos.y;
		  os << " / " << std::setw(3) << std::setfill('0') << pos.z;
		  os << " )";

      return os;
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

		//diagonal
		case SOUTHWEST:
				os << "South-West";
				break;
		case SOUTHEAST:
				os << "South-East";
				break;
		case NORTHWEST:
				os << "North-West";
				break;
		case NORTHEAST:
				os << "North-East";
				break;
	}

	return os;
}

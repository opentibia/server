#include "position.h"
#include <iomanip>

std::ostream& operator<<(std::ostream& os, const Position& pos) {
		  os << "( " << std::setw(5) << std::setfill('0') << pos.x;
		  os << " / " << std::setw(5) << std::setfill('0') << pos.y;
		  os << " / " << std::setw(3) << std::setfill('0') << pos.z;
		  os << " )";
}

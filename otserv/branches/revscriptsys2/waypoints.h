//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
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

#include <string>
#include <map>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

class Waypoint : public boost::enable_shared_from_this<Waypoint> {
public:
	Waypoint(const std::string& name, const Position& pos) :
		name(name), pos(pos) {}

	std::string name;
	Position pos;
};

typedef boost::shared_ptr<Waypoint> Waypoint_ptr;

class Waypoints {
public:
	// Does not require either constructor nor destructor

	void addWaypoint(Waypoint_ptr wp);
	Waypoint_ptr getWaypointByName(const std::string& name) const;

protected:
	typedef std::map<std::string, Waypoint_ptr> WaypointMap;
	WaypointMap waypoints;
};


inline void Waypoints::addWaypoint(Waypoint_ptr wp)
{
	waypoints.insert(std::make_pair(asUpperCaseString(wp->name), wp));
}

inline Waypoint_ptr Waypoints::getWaypointByName(const std::string& name) const
{
	std::string s = asUpperCaseString(name);
	WaypointMap::const_iterator f = waypoints.find(s);
	if(f == waypoints.end()) {
		return Waypoint_ptr();
	}
	return f->second;
}

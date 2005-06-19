//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// XML implementation of the Map Loader/Saver
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


#ifndef __IOMAPJXB2_H
#define __IOMAPJXB2_H

#include <string>

#include "iomap.h"

class Map;

/** Map-Loader implementation based on JXB2 format */
class IOMapJXB2 : public IOMap {
  public:
	IOMapJXB2(){};
	~IOMapJXB2(){};
	virtual char* getSourceDescription(){ return "JXB2 (JXBMAP based)"; };
	/** Load the map from an JXB2 file
	  * \param map Pointer to the Map
	  * \param identifier Name of the JXB2 to load
	  * \returns Whether map load was successful*/
	bool loadMap(Map* map, std::string identifier);
//private:
// may be implemented when get on JXB3
//	bool LoadContainer(xmlNodePtr nodeitem,Container* ccontainer);
};

#endif

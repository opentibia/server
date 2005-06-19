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


#ifndef __IOMAPXML_H
#define __IOMAPXML_H

#include <string>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include "iomap.h"

class Map;

/** Map-Loader implementation based on XML-Files */
class IOMapXML : public IOMap {
  public:
	IOMapXML(){};
	~IOMapXML(){};
	virtual char* getSourceDescription(){ return "OTX (XML2-based)"; };
	/** Load the map from an XML file
	  * \param map Pointer to the Map
	  * \param identifier Name of the XML-File to load
	  * \returns Whether map load was successful*/
	bool loadMap(Map* map, std::string identifier);
private:
	bool LoadContainer(xmlNodePtr nodeitem,Container* ccontainer);
};

#endif

//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Base class for the Player Loader/Saver
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


#ifndef __IOPLAYERXML_H
#define __IOPLAYERXML_H

#include <string>

#include "ioplayer.h"
#include "player.h"

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>


/** Baseclass for all Player-Loaders */
class IOPlayerXML : protected IOPlayer{
  public:
	/** Get a textual description of what source is used
	  * \returns Name of the source*/
	virtual char* getSourceDescription(){return "XML Player";};
	virtual bool loadPlayer(Player* player, std::string name);
	/** Save a player
	  * \returns Wheter the player was successfully saved
	  * \param player the player to save
	  */
	bool savePlayer(Player* player);
	IOPlayerXML(){};
	virtual ~IOPlayerXML(){};
};

#endif

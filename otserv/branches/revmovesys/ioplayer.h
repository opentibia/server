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


#ifndef __IOPLAYER_H
#define __IOPLAYER_H

#include <string>

#include "player.h"

/** Baseclass for all Player-Loaders */
class IOPlayer {
  public:
	static IOPlayer* instance();
	/** Get a textual description of what source is used
	  * \returns Name of the source*/
	virtual char* getSourceDescription(){return "Player source: NULL";};
	/** Load a player
	  * \returns Wheter the player was successfully loaded
	  * \param player Player structure to load to
	  * \param name Name of the player
	  */
	virtual bool loadPlayer(Player* player, std::string name);
	/** Save a player
	  * \returns Wheter the player was successfully saved
	  * \param player the player to save
	  */
	virtual bool savePlayer(Player* player);
	
	virtual bool getGuidByName(unsigned long &guid, unsigned long &alvl, std::string &name);
	virtual bool getNameByGuid(unsigned long guid, std::string &name);
	
  protected:
	IOPlayer(){};
	virtual ~IOPlayer(){};
	static IOPlayer* _instance;
};

#endif

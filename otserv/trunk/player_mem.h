//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// class which takes care of all data which must get saved in the player
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
// $Id$
//////////////////////////////////////////////////////////////////////
// $Log$
// Revision 1.7  2003/09/24 20:10:13  timmit
// *** empty log message ***
//
// Revision 1.6  2003/09/08 13:28:41  tliffrag
// Item summoning and maploading/saving now working.
//
// Revision 1.5  2003/05/19 16:48:37  tliffrag
// Loggingin, talking, walking around, logging out working
//
// Revision 1.4  2002/05/29 16:07:38  shivoc
// implemented non-creature display for login
//
// Revision 1.3  2002/05/28 13:55:56  shivoc
// some minor changes
//
//
//////////////////////////////////////////////////////////////////////

#ifndef __OTSERV_PLAYER_MEM_H
#define __OTSERV_PLAYER_MEM_H

#include "memory.h"
#include "pos.h"
#include <string>

enum skills_t {
    SKILL_FIST,
    SKILL_CLUB,
    SKILL_SWORD,
    SKILL_AXE,
    SKILL_DIST,
    SKILL_SHIELD,
    SKILL_FISH
};
enum skillsid_t {
    SKILL_LEVEL,
    SKILL_TRIES
};

   class player_mem : public ::Memory {
        public:
            void load();
            void save();
			player_mem();
            std::string name, passwd;
			int color_hair, color_shirt, color_legs, color_shoes;
			int lookdir;
			position pos;
			
			// health, health max.
			int health, health_max;
			// mana, mana max.
			int mana, mana_max;
			// food level
			int food_level;
			// inventory
			// TODO: how do you want to store the inventory?
			// use a list of Item?
			// capacity ( based off what is in inventory ), capacity max.
			int cap, cap_max;
			// level
			int level;
            // experience
            unsigned long experience;
			// magic level
			int mag_level;
            //mana spent ( to know when the character will advance in magic level )
			unsigned long mana_spent;
			// skills, # of tries on each skill ( to know when the character will advance )
			// 2 sub-elements.  [ skill ][ SKILL_LEVEL ] is the current level of that skill
			// [ skill ][ SKILL_TRIES ] is how many tries on that skill ( to determine when the skill advances )
			int skills[ 7 ][ 2 ];
			
            unsigned int pnum;

    }; // class player_mem : public Memory


#endif

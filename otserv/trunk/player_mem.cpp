//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// 
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
// Revision 1.10  2003/09/25 18:24:37  timmit
// Updated the 0x0A packet to send the client the actual player information.  Log on, then log off to create a .chr file.  You can edit the .chr file (within some limits) and the next time you log on the client will show the changes.
//
// Revision 1.8  2003/09/24 20:59:28  tliffrag
// replaced itoa with sprintf
//
// Revision 1.7  2003/09/24 20:47:05  timmit
// Player loading/saving added.  Both load() and save() in player_mem work.  Saves player's character to the appropriate *.chr file when the player logs out but does NOT load() the player's file when they log in.  Once accounts are added then the call to load() will be added.
//
// Revision 1.5  2003/09/08 13:28:41  tliffrag
// Item summoning and maploading/saving now working.
//
// Revision 1.4  2003/05/19 16:48:37  tliffrag
// Loggingin, talking, walking around, logging out working
//
// Revision 1.3  2002/05/28 13:55:56  shivoc
// some minor changes
//
//
//////////////////////////////////////////////////////////////////////
#include <stdlib.h>
#include <iostream>
#include <stdio.h>
#include <string>
#include "player_mem.h"

player_mem::player_mem(){
	color_hair=rand()%256;
	color_shirt=rand()%256;
	color_legs=rand()%256;
	color_shoes=rand()%256;
	lookdir=1;
	
	// default values
	// these will be changed when the character file is loaded, if one exists
	// TODO: make these the default values for a level 1 character
    health = 50;//150;
    health_max = 50;//150;
    mana = 0;
    mana_max = 0;
    food_level = 0;
    cap = 300;
    cap_max = 300;
    level = 15;
    experience = 3000;
    mag_level = 0;
    mana_spent = 0;
    for(int i=0;i<7;i++)
    {
        skills[ i ][ SKILL_LEVEL ] = 10;
        skills[ i ][ SKILL_TRIES ] = 0;
        std::cout<<"skills["<<i<<"] = "<<skills[i][SKILL_TRIES]<<std::endl;
    }	
}
    // player's name must be set before calling load()
    void player_mem::load() {std::cout << "loading player " << name << "...\n";
        std::cout << "player name is " << name << std::endl;
        std::string charstr = name + ".chr";
        
        // FIXME: this doesn't work, how do you change directory?
        // must create the directory if it doesn't exist
        //char * filepath = "\\players";
        //chdir( filepath );

        // FIXME/TODO: Use a pointer to a function since load() and save() are so similar
        //        char mode = 'l';
        //        if( mode == 'l' )
        //          readWrite = (void *)readVal;
        //        else
        //          readWrite = writeVal;

        // open up the character file
        FILE* charfile = fopen( charstr.c_str(), "r+t" );
        if( charfile != NULL )
        {std::cout<<"opened character file for reading..."<<std::endl;
			// FIXME: don't need to pass these names (2nd param), i will fix soon!
			health = readVal( charfile );
			health_max = readVal( charfile );
			mana = readVal( charfile );
			mana_max = readVal( charfile );
			food_level = readVal( charfile );
			cap = readVal( charfile );
			cap_max = readVal( charfile );
			level = readVal( charfile );
			experience = readVal( charfile );
			mag_level = readVal( charfile );
			mana_spent = readVal( charfile );
			
			skills[ SKILL_FIST ][ SKILL_LEVEL ] = readVal( charfile );
			skills[ SKILL_FIST ][ SKILL_TRIES ] = readVal( charfile );
			skills[ SKILL_CLUB ][ SKILL_LEVEL ] = readVal( charfile );
			skills[ SKILL_CLUB ][ SKILL_TRIES ] = readVal( charfile );
			skills[ SKILL_SWORD ][ SKILL_LEVEL ] = readVal( charfile );
			skills[ SKILL_SWORD ][ SKILL_TRIES ] = readVal( charfile );
			skills[ SKILL_AXE ][ SKILL_LEVEL ] = readVal( charfile );
			skills[ SKILL_AXE ][ SKILL_TRIES ] = readVal( charfile );
			skills[ SKILL_DIST ][ SKILL_LEVEL ] = readVal( charfile );
			skills[ SKILL_DIST ][ SKILL_TRIES ] = readVal( charfile );
			skills[ SKILL_SHIELD ][ SKILL_LEVEL ] = readVal( charfile );
			skills[ SKILL_SHIELD ][ SKILL_TRIES ] = readVal( charfile );
			skills[ SKILL_FISH ][ SKILL_LEVEL ] = readVal( charfile );
			skills[ SKILL_FISH ][ SKILL_TRIES ] = readVal( charfile );

/*			
            readVal( charfile, "health", (unsigned long)health );
            readVal( charfile, "health_max", (unsigned long)health_max );
            readVal( charfile, "mana", (unsigned long)mana );
            readVal( charfile, "mana_max", (unsigned long)mana_max );
            readVal( charfile, "food_level", (unsigned long)food_level );
            readVal( charfile, "cap", (unsigned long)cap );
            readVal( charfile, "cap_max", (unsigned long)cap_max );
            readVal( charfile, "level", (unsigned long)level );
            readVal( charfile, "experience", (unsigned long)experience );
            readVal( charfile, "mag_level", (unsigned long)mag_level );
            readVal( charfile, "mana_spent", (unsigned long)mana_spent );
            
            readVal( charfile, "skill_fist", skills[ SKILL_FIST ][ SKILL_LEVEL ] );
            readVal( charfile, "skill_fist_tries", skills[ SKILL_FIST ][ SKILL_TRIES ] );
            
            readVal( charfile, "skill_club", skills[ SKILL_CLUB ][ SKILL_LEVEL ] );
            readVal( charfile, "skill_club_tries", skills[ SKILL_CLUB ][ SKILL_TRIES ] );

            readVal( charfile, "skill_sword", skills[ SKILL_SWORD ][ SKILL_LEVEL ] );
            readVal( charfile, "skill_sword_tries", skills[ SKILL_SWORD ][ SKILL_TRIES ] );

            readVal( charfile, "skill_axe", skills[ SKILL_AXE ][ SKILL_LEVEL ] );
            readVal( charfile, "skill_axe_tries", skills[ SKILL_AXE ][ SKILL_TRIES ] );

            readVal( charfile, "skill_dist", skills[ SKILL_DIST ][ SKILL_LEVEL ] );
            readVal( charfile, "skill_dist_tries", skills[ SKILL_DIST ][ SKILL_TRIES ] );

            readVal( charfile, "skill_shield", skills[ SKILL_SHIELD ][ SKILL_LEVEL ] );
            readVal( charfile, "skill_shield_tries", skills[ SKILL_SHIELD ][ SKILL_TRIES ] );

            readVal( charfile, "skill_fish", skills[ SKILL_FISH ][ SKILL_LEVEL ] );
            readVal( charfile, "skill_fish_tries", skills[ SKILL_FISH ][ SKILL_TRIES ] );
*/            
            
            // close the file
            fclose( charfile );
        }
        else
        {
            std::cout << "ERROR: No character file exists for that character!\n";
        }
    } // player_mem::load()

    void player_mem::save() {
        std::cout << "player name is " << name << std::endl;
        std::string charstr = name + ".chr";

        // open up the character file
        FILE* charfile = fopen( charstr.c_str(), "wt" );
        //if( fopen( charstr.c_str(), "wt" ) != NULL )
        if( charfile != NULL )
        {std::cout<<"opened the character file for writing"<<std::endl;
            writeVal( charfile, "health", health );
            writeVal( charfile, "health_max", health_max );
            writeVal( charfile, "mana", mana );
            writeVal( charfile, "mana_max", mana_max );
            writeVal( charfile, "food_level", food_level );
            writeVal( charfile, "cap", cap );
            writeVal( charfile, "cap_max", cap_max );
            writeVal( charfile, "level", level );
            writeVal( charfile, "experience", experience );
            writeVal( charfile, "mag_level", mag_level );
            writeVal( charfile, "mana_spent", mana_spent );
            
            writeVal( charfile, "skill_fist", skills[ SKILL_FIST ][ SKILL_LEVEL ] );
            writeVal( charfile, "skill_fist_tries", skills[ SKILL_FIST ][ SKILL_TRIES ] );
            
            writeVal( charfile, "skill_club", skills[ SKILL_CLUB ][ SKILL_LEVEL ] );
            writeVal( charfile, "skill_club_tries", skills[ SKILL_CLUB ][ SKILL_TRIES ] );

            writeVal( charfile, "skill_sword", skills[ SKILL_SWORD ][ SKILL_LEVEL ] );
            writeVal( charfile, "skill_sword_tries", skills[ SKILL_SWORD ][ SKILL_TRIES ] );

            writeVal( charfile, "skill_axe", skills[ SKILL_AXE ][ SKILL_LEVEL ] );
            writeVal( charfile, "skill_axe_tries", skills[ SKILL_AXE ][ SKILL_TRIES ] );

            writeVal( charfile, "skill_dist", skills[ SKILL_DIST ][ SKILL_LEVEL ] );
            writeVal( charfile, "skill_dist_tries", skills[ SKILL_DIST ][ SKILL_TRIES ] );

            writeVal( charfile, "skill_shield", skills[ SKILL_SHIELD ][ SKILL_LEVEL ] );
            writeVal( charfile, "skill_shield_tries", skills[ SKILL_SHIELD ][ SKILL_TRIES ] );

            writeVal( charfile, "skill_fish", skills[ SKILL_FISH ][ SKILL_LEVEL ] );
            writeVal( charfile, "skill_fish_tries", skills[ SKILL_FISH ][ SKILL_TRIES ] );
            
            // close the file
            fclose( charfile );
        }
        else
        {
            std::cout << "ERROR: Could not create character save file!\n";
        }
    } // player_mem::save()
    
    unsigned long player_mem::readVal( FILE* charfile )//, std::string name, unsigned long &value )
    {
        char buffer[ 32 ];
        std::string charstr;
        
        charstr = fgets( buffer, 32, charfile );
        int strpos = charstr.find( "=", 0 ) + 1;
        charstr = charstr.substr( strpos, charstr.length() - strpos );
//        unsigned long value = atoi( charstr.c_str() );
        
        return atoi( charstr.c_str() );
    } // player_mem::readVal( FILE* charfile, std::string name, unsigned long &value )
    
    void player_mem::writeVal( FILE* charfile, std::string name, unsigned long value )
    {
        char buffer[ 32 ];
        std::string charstr;
        char tmp[ 32 ];

        charstr = name + "=";
        sprintf(tmp,"%ld",value);
        charstr += (char*)tmp;
        charstr += "\n";
        fputs( charstr.c_str(), charfile );
    } // player_mem::writeVal( FILE* charfile, std::string name, unsigned long value )

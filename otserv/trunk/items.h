//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// The database of items.
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
// Revision 1.9  2003/10/17 22:25:02  tliffrag
// Addes SorryNotPossible; added configfile; basic lua support
//
// Revision 1.8  2003/09/08 13:28:41  tliffrag
// Item summoning and maploading/saving now working.
//
// Revision 1.7  2003/08/26 21:09:53  tliffrag
// fixed maphandling
//
// Revision 1.6  2003/05/19 16:48:37  tliffrag
// Loggingin, talking, walking around, logging out working
//
// Revision 1.5  2002/08/01 14:11:28  shivoc
// added initial support for 6.9x clients
//
// Revision 1.4  2002/05/29 16:07:38  shivoc
// implemented non-creature display for login
//
// Revision 1.3  2002/05/28 13:55:56  shivoc
// some minor changes
//
// Revision 1.2  2002/04/08 15:57:03  shivoc
// made some changes to be more ansi compliant
//
// Revision 1.1  2002/04/08 13:53:59  acrimon
// Added some very basic map support
//
//////////////////////////////////////////////////////////////////////

#ifndef __OTSERV_ITEMS_H
#define __OTSERV_ITEMS_H

#include <map>
using namespace __gnu_cxx;

#include <string>

class ItemType {
public:
	unsigned short id;
    unsigned short maxitems; // maximum size if this is a container
    unsigned short tibiaid; // the ID in the Tibia protocol
    unsigned short weight; // weight of the item, e.g. throwing distance depends on it
    std::string name; // the name of the item

    // other vars:
    // what items can be used on what other items? pointer to function, think on implementation


    // other bools

	bool iscontainer;
    bool stackable;
	bool useable;
	bool alwaysOnBottom;
	bool alwaysOnTop;
    bool groundtile; // is this necessary?
    bool blocking; // people can walk on it
    bool pickupable; // people can pick it up
	//what freak has chosen this const static unsigned short?
    //const static unsigned short WATER = 486;
    const static unsigned short WATER = 0x01DB;
	const static unsigned short STREET = 0x02B6;
	const static unsigned short THING = 0x0759;

    bool isContainer(); // return if this item is a Container

    // maybe it should be enough if itemtypes van be defined with constructors only.
    ItemType();
    // some simple constructor:
    ItemType(unsigned short _id, unsigned short _tibiaid, std::string _name);
    ~ItemType();
};

class Items {
    public:
	typedef std::map<unsigned short, ItemType*> ItemHash;
    ItemHash items;

	std::string readDatEntryHeader(FILE* f);
	void Items::readDatEntry(FILE* f);

	int loadFromDat(std::string);
    Items();
    ~Items();
};

#endif










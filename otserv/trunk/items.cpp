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
// Revision 1.7  2003/11/01 15:58:52  tliffrag
// Added XML for players and map
//
// Revision 1.6  2003/10/19 21:32:19  tliffrag
// Reworked the Tile class; stackable items now working
//
// Revision 1.5  2003/10/17 22:25:02  tliffrag
// Addes SorryNotPossible; added configfile; basic lua support
//
// Revision 1.4  2003/09/08 13:28:41  tliffrag
// Item summoning and maploading/saving now working.
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

#include "items.h"
#include <iostream>

bool ItemType::isContainer() {
    return iscontainer;
}

ItemType::ItemType() {
	iscontainer=false;
    stackable=false;
	useable=false;
	alwaysOnBottom=false;
	alwaysOnTop=false;
    groundtile=false;
    blocking=false; // people can walk on it
    pickupable=false; // people can pick it up
}

ItemType::ItemType(unsigned short _id, unsigned short _tibiaid, std::string _name) {
    id = _id;
    tibiaid = _tibiaid;
    name = _name;
}

ItemType::~ItemType() {

}

Items::Items() {
    // add a few items
	loadFromDat("tibia.dat");
}

int Items::loadFromDat(std::string file){
	int id=100; // tibia.dat start with id 100
	#ifdef __DEBUG__
	std::cout << "Reading item data from tibia.dat" << std::endl;
	#endif
	FILE* f=fopen(file.c_str(), "rb");
	if(!f){
	#ifdef __DEBUG__
	std::cout << "FAILED!" << std::endl;
	#endif
		return -1;
	}
	fseek(f,0,SEEK_END);
	long size=ftell(f);
	#ifdef __DEBUG__
	std::cout << "tibia.dat size is " << size << std::endl;
	#endif
	fseek(f,20,SEEK_SET);
	readDatEntry(f); //skip the first buggy entry TODO: find out what that is
	while(ftell(f)< size){
		id++;
		ItemType* iType= new ItemType;
		iType->id=id;iType->tibiaid=id;

		std::string header=readDatEntryHeader(f);
		readDatEntry(f);
		//TODO include this in the loop:
		if((unsigned char)(*(header.begin()))==0x00)
			iType->groundtile=true;
		for(std::string::iterator i=header.begin(); i != header.end(); i++){
			unsigned char header_byte=(*i);
			switch(header_byte){
				case 0x0F:
				//can be equipped
				iType->pickupable=true;
				break;
				case 0x0B:
				//is blocking
				iType->blocking=true;
				break;
				case 0x04:
				//is stackable
				iType->stackable=true;
				break;
				case 0x05:
				//is useable
				iType->useable=true;
				break;
				case 0x0C:
				//is alwaysOnBottom
				//the concept of always on bottom is wron
				//this only means, that this item has a heigth
				//TLIFF
				//iType->alwaysOnBottom=true;
				break;
				case 0x02:
				//is on top
				i++;
				iType->alwaysOnTop=true;
				break;
				case 0x03:
				//is a container
				iType->iscontainer=true;
				break;
				case 0x10:
				//makes light (5 bytes)
				for(int j=0; j<4;j++)
					i++;
				break;
				case 0x1A:
				//unknown
				for(int j=0; j<2;j++)
					i++;
				break;

			}
		}
		items[id]=iType;

	}
	return true;
}

std::string Items::readDatEntryHeader(FILE* f){
	std::string header;
	int tmpbyte;
	while((tmpbyte=fgetc(f))!=EOF){
		if(tmpbyte==0xFF)
			return header;
		header+=(unsigned char)tmpbyte;
	}
	return "";
}

void Items::readDatEntry(FILE* f){
	//read over the pic-data contained in the dat
	int count=1;
	int tmpbyte;
	for(int i=0; i < 6; i++){
		tmpbyte=fgetc(f);
		if(tmpbyte>2 && i ==2) //some strange case where blend is very high
			tmpbyte=fgetc(f);
		count*=tmpbyte;
	}
	fseek(f,count*2, SEEK_CUR);
}
Items::~Items() {
    items.clear();
}


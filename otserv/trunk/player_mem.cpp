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
// Revision 1.16  2003/11/05 23:28:24  tliffrag
// Addex XML for players, outfits working
//
// Revision 1.15  2003/11/03 12:16:01  tliffrag
// started walking by mouse
//
// Revision 1.14  2003/11/01 15:58:52  tliffrag
// Added XML for players and map
//
// Revision 1.13  2003/10/21 17:55:07  tliffrag
// Added items on player
//
// Revision 1.12  2003/10/17 22:25:02  tliffrag
// Addes SorryNotPossible; added configfile; basic lua support
//
// Revision 1.11  2003/09/25 21:17:52  timmit
// Adding PlayerList in TMap and getID().  Not workigng!
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
#include <algorithm>
#include "player_mem.h"

player_mem::~player_mem(){
	saveXml();
}

player_mem::player_mem(){
	lookhead=rand()%256;
	lookbody=rand()%256;
	looklegs=rand()%256;
	lookfeet=rand()%256;
	looktype=0x88;
	pnum=rand();
	lookdir=1;
	sex=0;
	voc=0;

	//set item pointers to NULL
	for(int i=0; i < 11; i++)
		items[i]=NULL;

	//give the player some default stuff
	Item* item;


	item= new Item(0x582);
	items[SLOT_BACKPACK]=item;

	item= new Item(0x759);
	items[SLOT_ARMOR]=item;

	item= new Item(0x689);
	items[SLOT_RIGHT]=item;


	item= new Item(0x5B2);
	items[SLOT_LEFT]=item;;



	// default values
	// these will be changed when the character file is loaded, if one exists
	// TODO: make these the default values for a level 1 character
    health = 50;//150;
    healthmax = 50;//150;
    mana = 0;
    manamax = 0;
    food = 0;
    cap = 300;
    level = 15;
    experience = 3000;
    maglevel = 0;
    manaspent = 0;
    for(int i=0;i<7;i++)
    {
        skills[ i ][ SKILL_LEVEL ] = 10;
        skills[ i ][ SKILL_TRIES ] = 0;
    }
}
    // player's name must be set before calling load()
    void player_mem::load() {


    } // player_mem::load()

    void player_mem::save() {

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
       
        std::string charstr;
        char tmp[ 32 ];

        charstr = name + "=";
        sprintf(tmp,"%ld",value);
        charstr += (char*)tmp;
        charstr += "\n";
        fputs( charstr.c_str(), charfile );
    } // player_mem::writeVal( FILE* charfile, std::string name, unsigned long value )

int player_mem::unserialize(xmlNodePtr p){
	return true;
}

xmlNodePtr player_mem::serialize(){
	std::stringstream s;
	xmlNodePtr ret,p, tmp, slot;
	ret=xmlNewNode(NULL,(const xmlChar*)"player");

	xmlSetProp(ret, (const xmlChar*)"name", (const xmlChar*)name.c_str());
	xmlSetProp(ret, (const xmlChar*)"pass", (const xmlChar*)passwd.c_str());

	s.str(""); //empty the stringstream
	s << sex;
	xmlSetProp(ret, (const xmlChar*)"sex", (const xmlChar*)s.str().c_str());

	s.str(""); //empty the stringstream
	s << lookdir;
	xmlSetProp(ret, (const xmlChar*)"lookdir", (const xmlChar*)s.str().c_str());

	s.str(""); //empty the stringstream
	s << experience;
	xmlSetProp(ret, (const xmlChar*)"exp", (const xmlChar*)s.str().c_str());

	s.str(""); //empty the stringstream
	s << voc;
	xmlSetProp(ret, (const xmlChar*)"voc", (const xmlChar*)s.str().c_str());

	s.str(""); //empty the stringstream
	s << cap;
	xmlSetProp(ret, (const xmlChar*)"cap", (const xmlChar*)s.str().c_str());

	p=xmlNewChild(ret, NULL, (const xmlChar*)"mana", NULL);

	s.str(""); //empty the stringstream
	s << mana;
	xmlSetProp(p, (const xmlChar*)"now", (const xmlChar*)s.str().c_str());

	s.str(""); //empty the stringstream
	s << manamax;
	xmlSetProp(p, (const xmlChar*)"max", (const xmlChar*)s.str().c_str());

	s.str(""); //empty the stringstream
	s << manaspent;
	xmlSetProp(p, (const xmlChar*)"spent", (const xmlChar*)s.str().c_str());

	p=xmlNewChild(ret, NULL, (const xmlChar*)"health", NULL);

	s.str(""); //empty the stringstream
	s << health;
	xmlSetProp(p, (const xmlChar*)"now", (const xmlChar*)s.str().c_str());

	s.str(""); //empty the stringstream
	s << healthmax;
	xmlSetProp(p, (const xmlChar*)"max", (const xmlChar*)s.str().c_str());

	s.str(""); //empty the stringstream
	s << food;
	xmlSetProp(p, (const xmlChar*)"food", (const xmlChar*)s.str().c_str());

	p=xmlNewChild(ret, NULL, (const xmlChar*)"look", NULL);

	s.str(""); //empty the stringstream
	s << looktype;
	xmlSetProp(p, (const xmlChar*)"type", (const xmlChar*)s.str().c_str());

	s.str(""); //empty the stringstream
	s << lookhead;
	xmlSetProp(p, (const xmlChar*)"head", (const xmlChar*)s.str().c_str());

	s.str(""); //empty the stringstream
	s << lookbody;
	xmlSetProp(p, (const xmlChar*)"body", (const xmlChar*)s.str().c_str());

	s.str(""); //empty the stringstream
	s << looklegs;
	xmlSetProp(p, (const xmlChar*)"legs", (const xmlChar*)s.str().c_str());

	s.str(""); //empty the stringstream
	s << lookfeet;
	xmlSetProp(p, (const xmlChar*)"feet", (const xmlChar*)s.str().c_str());

	p=xmlNewChild(ret, NULL, (const xmlChar*)"skills", NULL);

	for(int i=0; i< 7; i++){
		tmp=xmlNewChild(p, NULL, (const xmlChar*)"skill", NULL);
		s.str(""); //empty the stringstream
		s << i;
		xmlSetProp(tmp, (const xmlChar*)"skillid", (const xmlChar*)s.str().c_str());
		s.str(""); //empty the stringstream
		s << skills[ i ][ SKILL_LEVEL ];
		xmlSetProp(tmp, (const xmlChar*)"level", (const xmlChar*)s.str().c_str());
		s.str(""); //empty the stringstream
		s << skills[ i ][ SKILL_TRIES ];
		xmlSetProp(tmp, (const xmlChar*)"tries", (const xmlChar*)s.str().c_str());
	}

	p=xmlNewChild(ret, NULL, (const xmlChar*)"inventory", NULL);

	for(int i=0; i< 11; i++){
		if(items[i]){
			slot=xmlNewChild(p, NULL, (const xmlChar*)"slot", NULL);
			s.str(""); //empty the stringstream
			s << i;
			xmlSetProp(slot, (const xmlChar*)"slotid", (const xmlChar*)s.str().c_str());
			xmlAddChild(slot,items[i]->serialize());
		}
	}


	//TODO: add known spells once there are known spells...


	return ret;
}

/*
	xmlDocPtr doc;
	xmlNodePtr root, tile, item;
	int width, height;


	doc=xmlParseFile("map.xml");
	root=xmlDocGetRootElement(doc);
	if(xmlStrcmp(root->name,(const xmlChar*) "map")){
		std::cout << "FATAL: couldnt load map. exiting" << std::endl;
		exit(1);
	}
	width=atoi((const char*)xmlGetProp(root, (const xmlChar *) "width"));
	height=atoi((const char*)xmlGetProp(root, (const xmlChar *) "height"));
	tile=root->children;
	for(int y=0; y < height; y++){
		for(int x=0; x < width; x++){
			item=tile->children;
			tiles[x][y] = new Tile;
			while(item != NULL){
				Item* myitem=new Item();
				myitem->unserialize(item);
				tiles[x][y]->addItem(myitem);
				item=item->next;
			}
			tile=tile->next;

		}
	}
	xmlFreeDoc(doc);
	return 0;

*/

int player_mem::loadXml(){
	FILE* f;
	std::string filename="data/players/"+name+".xml";
	transform (filename.begin(),filename.end(), filename.begin(), tolower);
	if(!(f=fopen(filename.c_str(),"r")))
		return 0; // no xml for this guy
	fclose(f);
	xmlDocPtr doc;
	xmlNodePtr root, tmp, p, slot;
	doc=xmlParseFile(filename.c_str());
	root=xmlDocGetRootElement(doc);
	root=xmlDocGetRootElement(doc);
	if(xmlStrcmp(root->name,(const xmlChar*) "player")){
		std::cout << "Strange. Player-Savefile was no savefile for " << name << std::endl;
	}
	p=root->children;
	//perhaps verify name
	passwd=(const char*)xmlGetProp(root, (const xmlChar *) "pass");
	sex=atoi((const char*)xmlGetProp(root, (const xmlChar *) "sex"));
	lookdir=atoi((const char*)xmlGetProp(root, (const xmlChar *) "lookdir"));
	experience=atoi((const char*)xmlGetProp(root, (const xmlChar *) "exp"));
	voc=atoi((const char*)xmlGetProp(root, (const xmlChar *) "voc"));
	while(p){
		std::string str=(char*)p->name;
		if(str=="mana"){
			mana=atoi((const char*)xmlGetProp(p, (const xmlChar *) "now"));
			manamax=atoi((const char*)xmlGetProp(p, (const xmlChar *) "max"));
			manaspent=atoi((const char*)xmlGetProp(p, (const xmlChar *) "spent"));
		}
		else if(str=="health"){
			health=atoi((const char*)xmlGetProp(p, (const xmlChar *) "now"));
			healthmax=atoi((const char*)xmlGetProp(p, (const xmlChar *) "max"));
			food=atoi((const char*)xmlGetProp(p, (const xmlChar *) "food"));
		}
		else if(str=="look"){
			looktype=atoi((const char*)xmlGetProp(p, (const xmlChar *) "type"));
			lookhead=atoi((const char*)xmlGetProp(p, (const xmlChar *) "head"));
			lookbody=atoi((const char*)xmlGetProp(p, (const xmlChar *) "body"));
			looklegs=atoi((const char*)xmlGetProp(p, (const xmlChar *) "legs"));
			lookfeet=atoi((const char*)xmlGetProp(p, (const xmlChar *) "feet"));
		}
		else if(str=="skills"){
			tmp=p->children;
			while(tmp){
				int s_id, s_lvl, s_tries;
				s_id=atoi((const char*)xmlGetProp(tmp, (const xmlChar *) "skillid"));
				s_lvl=atoi((const char*)xmlGetProp(tmp, (const xmlChar *) "level"));
				s_tries=atoi((const char*)xmlGetProp(tmp, (const xmlChar *) "tries"));
				skills[s_id][SKILL_LEVEL]=s_lvl;
				skills[s_id][SKILL_TRIES]=s_tries;
				tmp=tmp->next;
			}
		}
		else if(str=="inventory"){
			slot=p->children;
			while(slot){
				int sl_id=atoi((const char*)xmlGetProp(slot, (const xmlChar *) "slotid"));
				Item* myitem=new Item();
				myitem->unserialize(slot->children);
				items[sl_id]=myitem;
				slot=slot->next;
			}
		}
		p=p->next;
	}
	std::cout << "loaded " << filename << std::endl;
	xmlFreeDoc(doc);
	return true;
}

int player_mem::saveXml(){
	std::string filename="data/players/"+name+".xml";
	transform (filename.begin(),filename.end(), filename.begin(), tolower);
	std::stringstream s;
	xmlDocPtr doc;
	doc = xmlNewDoc((const xmlChar*)"1.0");
	doc->children = serialize();
	xmlKeepBlanksDefault(0);
	xmlSaveFile(filename.c_str(), doc);
	xmlFreeDoc(doc);
	return true;
}

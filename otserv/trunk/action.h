#ifndef __OTSERV_ACTION_H
#define __OTSERV_ACTION_H

#include "pos.h"

#include <string>
enum {
ACTION_NONE=0,
ACTION_LOGOUT,
ACTION_TURN,
ACTION_MOVE,
ACTION_SETOUTFIT,
ACTION_SAY,
ACTION_WALK_IN,
ACTION_LOGIN,
ACTION_ITEM_APPEAR,
ACTION_THROW,
ACTION_ITEM_DISAPPEAR,
};

class Creature;

// Structure which defines actions

struct Action {
  int type;    // which kind of action?
  int direction; // north/east/south/west
  position pos1;  // first argument of the action (like attacked person...)
  position pos2;  // second argument (like place an item is used on...)
  int stack;	//postition on the stack of a tile
  std::string playername; //player who said something
  std::string buffer;          // buffer if the char wants to say something etc.
  int id; // an id
Creature* creature;

  //that will have to work for now
	Action(){
		type=ACTION_NONE;
	}

  //unsigned short count;  // count of the items the action is performed on...
  //unsigned ityp;     // type of the item the action is performed on...
}; // struct action

#endif

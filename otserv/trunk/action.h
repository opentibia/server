//+-----------------------------------------------------------------+
//| Inclusion Gaurds
//+-----------------------------------------------------------------+
#ifndef ACTION_H
#define ACTION_H
//+-----------------------------------------------------------------+
//| End of ( Inclusion Gaurds )
//+-----------------------------------------------------------------+

//+-----------------------------------------------------------------+
//| Declared Classes for this header
//+-----------------------------------------------------------------+
class Creature;
//+-----------------------------------------------------------------+
//| End of ( Declared Classes )
//+-----------------------------------------------------------------+

#include "pos.h"
#include <iostream>
#include <stdint.h>

//+-----------------------------------------------------------------+
//| Enumeration of: Action Types;
//+-----------------------------------------------------------------+
enum ActionTypes {
//  No Action.
ACTION_NONE=0,
//  A player wishes to log out.
ACTION_LOGOUT,
//  A player wants to Turn.
ACTION_TURN,
//  A player wants to move.
ACTION_MOVE,
//  A player wishes to say something.
ACTION_SAY,
//  A player walked in screen.
ACTION_WALK_IN,
//  A player logged in.
ACTION_LOGIN,
//  An item was created. ( Not needed? )
ACTION_CREATE_ITEM,
//  An item appeared.
ACTION_ITEM_APPEAR,
//  A player wishes to throw something.
ACTION_THROW,
//  An item dissapeared. ( Not needed? )
ACTION_ITEM_DISAPPEAR,
//  An item changed.
ACTION_ITEM_CHANGE,
//  The ground changed.
ACTION_GROUND_CHANGE,
//  A player wishes to inspect a tile.
ACTION_LOOK_AT,
//  Look at a player appearance.
ACTION_REQUEST_APPEARANCE,
//  A player wishes to change their appearance.
ACTION_CHANGE_APPEARANCE,
//  A player wishes to use an item on something.
ACTION_ITEM_USE,
//  An animation appeared.
ACTION_ANIMATION,
//  Update health.
ACTION_HPUPDATE,
//  Send animated item ( bolt, etc. )
ACTION_ANI_ITEM,
ACTION_TEXT_ANI,
ACTION_KICK,
// make a hit in a started attack
ACTION_DOATTACK,
};
//+-----------------------------------------------------------------+
//| End of ( Enumeration of: Action Types; )
//+-----------------------------------------------------------------+


//+-----------------------------------------------------------------+
//| Struct - Action
//+-----------------------------------------------------------------+
struct Action {
//  Type of action ( Action Enum? )
  ActionTypes type;
// Direction of action/player ( N/E/S/W ( 1, 2, 3, 4 ) )?
  int direction;
//  First position ( used from / attacked? )
  position pos1;
//  Second Position ( used on? )
  position pos2;
//  Stack number or position on tile.
  int stack;
//  Name of the player action is for.
  std::string playername;
//  Data or Buffer ( If a player wants to say something(?) )
  std::string buffer;
//  ID ( For items? )
  int id;
//  Count ( Also for items? )
  int count;
//  Main Creature action is (for)?
  Creature* creature;
//  For attacks?
  uint32_t attackCreature;

  // time the action was received on; automatically filled in by constructor
  double receiveTime;

//  Constructor ( Will have to do for now. )
	Action();
};
//+-----------------------------------------------------------------+
//| End of ( Struct - Action )
//+-----------------------------------------------------------------+


//+-----------------------------------------------------------------+
//| Inclusion Gaurds
//+-----------------------------------------------------------------+
#endif
//+-----------------------------------------------------------------+
//| End of ( Inclusion Gaurds )
//+-----------------------------------------------------------------+

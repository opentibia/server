//+-----------------------------------------------------------------+
//| Inclusion Gaurds
//+-----------------------------------------------------------------+
#ifndef BATTLESYSTEM_H
#define BATTLESYSTEM_H
//+-----------------------------------------------------------------+
//| End of ( Inclusion Gaurds )
//+-----------------------------------------------------------------+

//+-----------------------------------------------------------------+
//| Declared Classes for this header
//+-----------------------------------------------------------------+
#include "protokoll.h"
#include "network.h"
#include "creature.h"
#include "player.h"
#include "action.h"
#include <unistd.h> // read
#include <stdio.h>
#include <iostream>
#include "tmap.h"

#include <iostream>
#include <stdio.h>
#include <string>
#include <sstream>
#include <vector>
//+-----------------------------------------------------------------+
//| End of ( Declared Classes )
//+-----------------------------------------------------------------+


//  Functions

//  Return if we may start attacking.
bool battle_StartAttack(Creature*, Map&);

//  Send an attack. 
void battle_Attack(int, int, position, Creature*, Map&);

//  Stop an attack.
void battle_End();

//+-----------------------------------------------------------------+
//| Inclusion Gaurds
//+-----------------------------------------------------------------+
#endif
//+-----------------------------------------------------------------+
//| End of ( Inclusion Gaurds )
//+-----------------------------------------------------------------+

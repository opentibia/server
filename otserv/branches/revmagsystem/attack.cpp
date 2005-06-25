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


#include "attack.h"



Attack::Attack():
attack_type(ATTACK_TYPE_NONE),
ownerId(0),
owner(NULL),
manaAttack(false)
{
}


bool Attack::checkManaShield(Creature *attackedCreature, long damage){
	//TODO: check conditions for mana shield
	if(attackedCreature->manaShieldTicks > 1000){
		if(damage < attackedCreature->getMana()){
			manaAttack = true;
			return true;
		}
		else{
			manaAttack = false;
			return false;
		}
	}
	else{
		manaAttack = false;
		return false;
	}
}

void Attack::getDrawInfoManaShield(MagicEffectClasses &me, eATextColor text_color){
	me = NM_ME_LOOSE_ENERGY;
	text_color = ATEXT_BLUE;
}


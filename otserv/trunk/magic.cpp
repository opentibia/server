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

#include "definitions.h"

#include "magic.h"

BaseMagicEffect::BaseMagicEffect()
{
	centerpos.x = 0;
	centerpos.y = 0;
	centerpos.z = 0;
	damageEffect = 0;
	animationcolor = 0;
	animationEffect = 0;
}

MagicEffectClass::MagicEffectClass()
{
	minDamage = 0;
	maxDamage = 0;
	offensive = false;
	physical = false;
}

MagicEffectRuneClass::MagicEffectRuneClass()
{
	//
}

MagicEffectAreaClass::MagicEffectAreaClass()
{
	direction = 0;
	areaEffect = 0xFF;
	memset(area, 0, sizeof(area));
}

MagicEffectInstantSpellClass::MagicEffectInstantSpellClass()
{
	manaCost = 0;
}

MagicEffectGroundClass::MagicEffectGroundClass()
{
	groundID = 0;
}

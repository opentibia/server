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

#ifndef __MAGIC_H__
#define __MAGIC_H__

#include "position.h"

class BaseMagicEffect
{
public:
	BaseMagicEffect();
  virtual ~BaseMagicEffect() {};

	Position centerpos;
	unsigned char damageEffect;
	unsigned char animationcolor;
	unsigned char animationEffect;
};

class MagicEffectClass : public BaseMagicEffect
{
public:
	MagicEffectClass();
	int minDamage;
	int maxDamage;
	bool offensive;
};

/*
class MagicSpellConditionClass : public MagicEffectClass
{
public:
	MagicSpellConditionClass(cond_t cond);
	MagicSpellConditionClass(const MagicSpellClass *rhs);
	MagicSpellConditionClass& operator = (const MagicSpellClass* rhs);

	cond_t condition;
	unsigned long condTimeDefaultInSeconds;
	unsigned long condTimeElapsedInSeconds;
	unsigned long condCount;
};
*/

class MagicEffectRuneClass : public MagicEffectClass
{
public:
	MagicEffectRuneClass();
};

class MagicEffectAreaClass : public MagicEffectClass
{
public:
	MagicEffectAreaClass();
	unsigned char direction;
	unsigned char areaEffect;
	unsigned char area[14][18];
};

class MagicEffectInstantSpellClass : public MagicEffectAreaClass
{
public:
	MagicEffectInstantSpellClass();
	int manaCost;
};

class MagicEffectGroundClass : public MagicEffectAreaClass
{
public:
	MagicEffectGroundClass();
	unsigned short groundID;
};

#endif //__MAGIC_H__
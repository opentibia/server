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

#ifndef __condition_h_
#define __condition_h_

class Creature;
class Player;

enum conditiontype_t {
	CONDITION_NONE,
	CONDITION_POISON,		//Damage
	CONDITION_FIRE,			//Damage
	CONDITION_ENERGY,		//Damage
	CONDITION_SPEED,		//Speed
	CONDITION_OUTFIT,		//Outfit
	CONDITION_LIGHT,		//Light   -- Player only
	CONDITION_MAGICSHIELD,	//Generic -- Player only
	CONDITION_PZLOCK,		//Generic -- Player only
	CONDITION_INFIGHT,		//Generic -- Player only
	CONDITION_DRUNK,		//Generic -- Player only
	CONDITION_EXHAUSTED,	//Generic
	CONDITION_FOOD,			//Generic -- Player only -- NOT IMPLEMENTED YET
};

enum condition_endreason_t {
	REASON_ENDTICKS,
	REASON_ABORT,
};


class Condition{
public:
	Condition();
	virtual ~Condition(){};
	
	virtual bool startCondition(Creature *c) = 0;
	virtual void executeCondition(int interval) = 0;
	virtual void endCondition(int reason) = 0;
	
	virtual void addCondition(Condition *condition) = 0;
	
	virtual unsigned char getIcons() = 0;
	conditiontype_t getType(){ return condition_type;}
	
	int ticks;
	
	static Condition* createCondition(conditiontype_t _type, int ticks, int param);
	
protected:
	
	conditiontype_t condition_type;
	Creature *creature;
	Player *player;
};


class ConditionGeneric: public Condition
{
public:
	ConditionGeneric(conditiontype_t _type, int _ticks);
	virtual ~ConditionGeneric(){};
	
	virtual bool startCondition(Creature *c);
	virtual void executeCondition(int interval);
	virtual void endCondition(int reason);
	
	virtual void addCondition(Condition *condition);
	
	virtual unsigned char getIcons();
};


/*
class ConditionDamage: public Condition
{
public:
	ConditionDamage(conditiontype_t _type, int _ticks, int _damagetype);
	virtual ~ConditionDamage(){};
	
	virtual bool startCondition(Creature *c);
	virtual void executeCondition(int interval);
	virtual void endCondition(int reason);
	
	virtual void addCondition(Condition *condition);
	
	virtual unsigned char getIcons();
	
};
*/

class ConditionSpeed: public Condition
{
public:
	ConditionSpeed(int _ticks, int change_speed);
	virtual ~ConditionSpeed(){};
	
	virtual bool startCondition(Creature *c);
	virtual void executeCondition(int interval);
	virtual void endCondition(int reason);
	
	virtual void addCondition(Condition *condition);
	
	virtual unsigned char getIcons();
};


class ConditionOutfit: public Condition
{
public:
	ConditionOutfit(int _ticks, int new_outfit);
	virtual ~ConditionOutfit(){};
	
	virtual bool startCondition(Creature *c);
	virtual void executeCondition(int interval);
	virtual void endCondition(int reason);
	
	virtual void addCondition(Condition *condition);
	
	virtual unsigned char getIcons();
};


class ConditionLight: public Condition
{
public:
	ConditionLight(int _ticks, int _lightlevel, int _lightcolor);
	virtual ~ConditionLight(){};
	
	virtual bool startCondition(Creature *c);
	virtual void executeCondition(int interval);
	virtual void endCondition(int reason);
	
	virtual void addCondition(Condition *condition);
	
	virtual unsigned char getIcons();
};


#endif

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

#ifndef __OTSERV_CONDITION_H__
#define __OTSERV_CONDITION_H__

#include <list>
#include <string>

class Creature;
class Player;

enum ConditionType_t {
	CONDITION_NONE,
	CONDITION_POISON,		   //Damage
	CONDITION_FIRE,			   //Damage
	CONDITION_ENERGY,		   //Damage
	CONDITION_HASTE,		   //Speed
	CONDITION_PARALYZE,		 //Speed
	CONDITION_OUTFIT,		   //Outfit
	CONDITION_LIGHT,		   //Light   -- Player only
	CONDITION_MANASHIELD,  //Generic -- Player only
	CONDITION_PZLOCK,		   //Generic -- Player only
	CONDITION_INFIGHT,		 //Generic -- Player only
	CONDITION_DRUNK,		   //Generic -- Player only
	CONDITION_EXHAUSTED,	 //Generic
	CONDITION_FOOD,			   //Generic -- Player only -- NOT IMPLEMENTED YET
};

enum EndCondition_t {
	REASON_ENDTICKS,
	REASON_ABORT,
};

class Condition{
public:
	Condition();
	virtual ~Condition(){};

	virtual std::string serialize(){return std::string();}
	virtual bool unserialize(std::string ){return true;}
	
	virtual bool startCondition(Creature* creature) = 0;
	virtual void executeCondition(int32_t interval) = 0;
	virtual void endCondition(EndCondition_t reason) = 0;
	
	virtual void addCondition(Condition* condition) = 0;
	
	virtual uint8_t getIcons() = 0;
	ConditionType_t getType(){ return conditionType;}
	
	int ticks;
	
	static Condition* createCondition(ConditionType_t _type, int32_t ticks, int32_t param);
	
protected:	
	ConditionType_t conditionType;

	//Creature* creature;
	//Player* player;
};

class ConditionGeneric: public Condition
{
public:
	ConditionGeneric(ConditionType_t _type, int32_t _ticks);
	virtual ~ConditionGeneric(){};
	
	virtual bool startCondition(Creature* creature);
	virtual void executeCondition(int32_t interval);
	virtual void endCondition(EndCondition_t reason);
	
	virtual void addCondition(Condition *condition);
	
	virtual uint8_t getIcons();
};

class ConditionDamage: public Condition
{
public:
	ConditionDamage(ConditionType_t _type, int32_t _ticks, int32_t _damagetype);
	virtual ~ConditionDamage(){};
	
	virtual bool startCondition(Creature* creature);
	virtual void executeCondition(int32_t interval);
	virtual void endCondition(EndCondition_t reason);
	
	virtual void addCondition(Condition* condition);
	
	virtual uint8_t getIcons();	
};

class ConditionSpeed: public Condition
{
public:
	ConditionSpeed(ConditionType_t _type, int32_t _ticks, int32_t changeSpeed);
	virtual ~ConditionSpeed(){};
	
	virtual bool startCondition(Creature* creature);
	virtual void executeCondition(int32_t interval);
	virtual void endCondition(EndCondition_t reason);
	
	virtual void addCondition(Condition* condition);
	
	virtual uint8_t getIcons();
	
protected:
	int32_t speedDelta;
};

class ConditionOutfit: public Condition
{
public:
	ConditionOutfit(int32_t _ticks, int32_t new_outfit);
	virtual ~ConditionOutfit(){};
	
	virtual bool startCondition(Creature* creature);
	virtual void executeCondition(int32_t interval);
	virtual void endCondition(EndCondition_t reason);
	
	virtual void addCondition(Condition* condition);
	
	virtual uint8_t getIcons();
};

class ConditionLight: public Condition
{
public:
	ConditionLight(int32_t _ticks, int32_t _lightlevel, int32_t _lightcolor);
	virtual ~ConditionLight(){};
	
	virtual bool startCondition(Creature* creature);
	virtual void executeCondition(int32_t interval);
	virtual void endCondition(EndCondition_t reason);
	
	virtual void addCondition(Condition *condition);
	
	virtual uint8_t getIcons();
};


#endif

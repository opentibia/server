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


#ifndef __OTSERV_SPELLS_H__
#define __OTSERV_SPELLS_H__

#include "game.h"
#include "luascript.h"
#include "player.h"
#include "actions.h"
#include "talkaction.h"
#include "baseevents.h"

class RuneSpell;
class InstantSpell;
class Spell;

typedef std::map<uint32_t, RuneSpell*> RunesMap;
typedef std::map<std::string, InstantSpell*> InstantsMap;

class Spells : public BaseEvents
{
public:
	Spells();
	virtual ~Spells();
	
	Spell* getSpellByName(const std::string& name);
	RuneSpell* getRuneSpell(uint32_t id);
	RuneSpell* getRuneSpellByName(const std::string& name);
	
	InstantSpell* getInstantSpell(const std::string& words);
	InstantSpell* getInstantSpellByName(const std::string& name);

	bool playerSaySpell(Player* player, SpeakClasses type, const std::string& words);

	static int32_t spellExhaustionTime;
	static int32_t spellInFightTime;
	
protected:
	virtual void clear();
	virtual LuaScriptInterface& getScriptInterface();
	virtual std::string getScriptBaseName();
	virtual Event* getEvent(const std::string& nodeName);
	virtual bool registerEvent(Event* event, xmlNodePtr p);	
	
	RunesMap runes;
	InstantsMap instants;

	LuaScriptInterface m_scriptInterface;
};

typedef bool (InstantSpellFunction)(Creature* creature, const std::string& param);
typedef bool (RuneSpellFunction)(Creature* creature, Item* item, const Position& posFrom, const Position& posTo, Creature* target);

class Spell
{
public:
	Spell();
	virtual ~Spell(){};
	
	bool configureSpell(xmlNodePtr xmlspell);
	
	const std::string& getName() const {return name;}

	virtual bool castSpell(Creature* creature) = 0;
	virtual bool castSpell(Creature* creature, Creature* target) = 0;

protected:
	bool playerSpellCheck(const Player* player);
	bool playerInstantSpellCheck(const Player* player, const Position& toPos);
	bool playerRuneSpellCheck(const Player* player, const Position& toPos);
	
	void postCastSpell(Player* player);
	
	bool enabled;
	bool premium;
	int32_t level;
	int32_t magLevel;
	
	int32_t mana;
	int32_t soul;
	bool exhaustion;
	bool needTarget;
	bool blocking;
	bool isAggressive;

private:
	std::string name;
	uint32_t vocationBits;
};

class InstantSpell : public TalkAction, public Spell
{
public:
	InstantSpell(LuaScriptInterface* _interface);
	virtual ~InstantSpell();
	
	virtual bool configureEvent(xmlNodePtr p);
	virtual bool loadFunction(const std::string& functionName);
	
	bool playerCastInstant(Player* player, const std::string& param);

	virtual bool castSpell(Creature* creature);
	virtual bool castSpell(Creature* creature, Creature* target);

	//scripting
	bool executeCastSpell(Creature* creature, const LuaVariant& var);
	
protected:	
	virtual std::string getScriptEventName();
	
	static InstantSpellFunction HouseGuestList;
	static InstantSpellFunction HouseSubOwnerList;
	static InstantSpellFunction HouseDoorList;
	static InstantSpellFunction HouseKick;
	static InstantSpellFunction SearchPlayer;
	
	static House* getHouseFromPos(Creature* creature);
	
	bool internalCastSpell(Creature* creature, const LuaVariant& var);
	Position getCasterPosition(Creature* creature);
	
	bool needDirection;
	bool hasParam;
	InstantSpellFunction* function;
};

class RuneSpell : public Action, public Spell
{
public:
	RuneSpell(LuaScriptInterface* _interface);
	virtual ~RuneSpell();
	
	virtual bool configureEvent(xmlNodePtr p);
	virtual bool loadFunction(const std::string& functionName);
	
	virtual bool canExecuteAction(const Player* player, const Position& toPos);

	virtual bool executeUse(Player* player, Item* item, const PositionEx& posFrom, const PositionEx& posTo);
	
	virtual bool castSpell(Creature* creature);
	virtual bool castSpell(Creature* creature, Creature* target);

	//scripting
	bool executeCastSpell(Creature* creature, const LuaVariant& var);
	
	uint32_t getRuneItemId(){return runeId;};
	
protected:
	virtual std::string getScriptEventName();
	
	bool internalCastSpell(Creature* creature, const LuaVariant& var);

	bool hasCharges;
	uint32_t runeId;
	
	RuneSpellFunction* function;
};

#endif

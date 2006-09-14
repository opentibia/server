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

class Spells : public BaseEvents
{
public:
	Spells();
	virtual ~Spells();
	
	RuneSpell* getRuneSpell(const Item* item);
	RuneSpell* getRuneSpell(const std::string& name);
	
	InstantSpell* getInstantSpell(const std::string& words);
	InstantSpell* getInstantSpellByName(const std::string& name);

	bool playerSaySpell(Player* player, SpeakClasses type, const std::string& words);
	
protected:
	virtual void clear();
	virtual LuaScriptInterface& getScriptInterface();
	virtual std::string getScriptBaseName();
	virtual Event* getEvent(const std::string& nodeName);
	virtual bool registerEvent(Event* event, xmlNodePtr p);
	
	
	typedef std::map<uint32_t, RuneSpell*> RunesMap;
	typedef std::map<std::string, InstantSpell*> InstantsMap ;
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
	
protected:
	bool playerSpellCheck(const Player* player);
	
	bool causeExhaustion(){return exhaustion;};
	
	void postCastSpell(Player* player);
	
	bool enabled;
	bool premium;
	int32_t level;
	int32_t magLevel;
	
	int32_t mana;
	int32_t soul;
	bool exhaustion;
	bool needTarget;

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
	bool castInstant(Creature* creature);
	bool castInstant(Creature* creature, Creature* target);

	//scripting
	bool executeCastInstant(Creature* creature, const LuaVariant& var);
	
protected:	
	virtual std::string getScriptEventName();
	
	static InstantSpellFunction HouseGuestList;
	static InstantSpellFunction HouseSubOwnerList;
	static InstantSpellFunction HouseDoorList;
	static InstantSpellFunction HouseKick;
	static InstantSpellFunction SearchPlayer;
	
	static House* getHouseFromPos(Creature* creature);
	
	bool castInstant(Creature* creature, const LuaVariant& var);
	
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
	bool castRune(Creature* creature, Creature* target);
	bool castRune(Creature* creature, const Position& pos);

	//scripting
	bool executeCastRune(Creature* creature, const LuaVariant& var);
	
	uint32_t getRuneItemId(){return runeId;};
	
protected:
	virtual std::string getScriptEventName();
	
	bool castRune(Creature* creature, const LuaVariant& var);

	bool hasCharges;
	uint32_t runeId;
	
	RuneSpellFunction* function;
};

#endif

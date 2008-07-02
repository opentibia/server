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

#ifndef __OTSERV_NPC_H__
#define __OTSERV_NPC_H__

#include "creature.h"
#include "luascript.h"
#include "templates.h"

//////////////////////////////////////////////////////////////////////
// Defines an NPC...
class Npc;
class NpcResponse;
struct NpcState;

typedef std::list<Npc*> NpcList;
class Npcs{
public:
	Npcs() {};
	~Npcs() {};
	void reload();
};

class NpcScriptInterface : public LuaScriptInterface
{
public:
	NpcScriptInterface();
	virtual ~NpcScriptInterface();

	bool loadNpcLib(std::string file);

	static void pushState(lua_State *L, NpcState* state);
	static void popState(lua_State *L, NpcState* &state);

protected:

	virtual void registerFunctions();

	static int luaActionSay(lua_State *L);
	static int luaActionMove(lua_State *L);
	static int luaActionMoveTo(lua_State *L);
	static int luaActionTurn(lua_State* L);
	static int luaActionFollow(lua_State* L);
	static int luaCreatureGetName(lua_State *L);
	static int luaCreatureGetName2(lua_State *L);
	static int luaCreatureGetPos(lua_State *L);
	static int luaSelfGetPos(lua_State *L);
	static int luagetDistanceTo(lua_State *L);
	static int luaSetNpcFocus(lua_State *L);
	static int luaGetNpcCid(lua_State *L);
	static int luaGetNpcPos(lua_State *L);
	static int luaGetNpcState(lua_State *L);
	static int luaSetNpcState(lua_State *L);
	static int luaGetNpcName(lua_State *L);
	static int luaGetNpcParameter(lua_State *L);

private:
	virtual bool initState();
	virtual bool closeState();

	bool m_libLoaded;
};

class NpcEventsHandler
{
public:
	NpcEventsHandler(Npc* npc);
	virtual ~NpcEventsHandler();

	virtual void onCreatureAppear(const Creature* creature){};
	virtual void onCreatureDisappear(const Creature* creature){};
	virtual void onCreatureMove(const Creature* creature, const Position& oldPos, const Position& newPos){};
	virtual void onCreatureSay(const Creature* creature, SpeakClasses, const std::string& text){};
	virtual void onThink(){};

	bool isLoaded();

protected:
	Npc* m_npc;
	bool m_loaded;
};

class NpcScript : public NpcEventsHandler
{
public:
	NpcScript(std::string file, Npc* npc);
	virtual ~NpcScript();

	virtual void onCreatureAppear(const Creature* creature);
	virtual void onCreatureDisappear(const Creature* creature);
	virtual void onCreatureMove(const Creature* creature, const Position& oldPos, const Position& newPos);
	virtual void onCreatureSay(const Creature* creature, SpeakClasses, const std::string& text);
	virtual void onThink();

private:
	NpcScriptInterface* m_scriptInterface;

	int32_t m_onCreatureAppear;
	int32_t m_onCreatureDisappear;
	int32_t m_onCreatureMove;
	int32_t m_onCreatureSay;
	int32_t m_onThink;
};

enum RespondParam_t{
	RESPOND_DEFAULT = 0,
	RESPOND_MALE = 1,
	RESPOND_FEMALE = 2,
	RESPOND_PZBLOCK = 4,
	RESPOND_LOWMONEY = 8,
	RESPOND_NOAMOUNT = 16,
	RESPOND_LOWAMOUNT = 32,
	RESPOND_PREMIUM = 64,
	RESPOND_DRUID = 128,
	RESPOND_KNIGHT = 256,
	RESPOND_PALADIN = 512,
	RESPOND_SORCERER = 1024,
	RESPOND_LOWLEVEL = 2048
};

enum ResponseType_t{
	RESPONSE_DEFAULT,
	RESPONSE_SCRIPT,
};

enum InteractType_t{
	INTERACT_TEXT,
	INTERACT_EVENT
};

enum ReponseActionParam_t{
	ACTION_NONE,
	ACTION_SETTOPIC,
	ACTION_SETLEVEL,
	ACTION_PRICE,
	ACTION_TAKEMONEY,
	ACTION_GIVEMONEY,
	ACTION_SELLITEM,
	ACTION_BUYITEM,
	ACTION_GIVEITEM,
	ACTION_TAKEITEM,
	ACTION_AMOUNT,
	ACTION_ITEM,
	ACTION_SUBTYPE,
	ACTION_EFFECT,
	ACTION_SPELL,
	ACTION_TEACHSPELL,
	ACTION_STORAGE,
	ACTION_TELEPORT,
	ACTION_SCRIPT,
	ACTION_SCRIPTPARAM,
	ACTION_ADDQUEUE,
	ACTION_IDLE
};

enum StorageComparision_t{
	STORAGE_LESS,
	STORAGE_LESSOREQUAL,
	STORAGE_EQUAL,
	STORAGE_GREATEROREQUAL,
	STORAGE_GREATER
};

enum NpcEvent_t{
	EVENT_NONE,
	EVENT_BUSY,
	EVENT_THINK,
	EVENT_PLAYER_ENTER,
	EVENT_PLAYER_MOVE,
	EVENT_PLAYER_LEAVE
	/*
	EVENT_CREATURE_ENTER,
	EVENT_CREATURE_MOVE,
	EVENT_CREATURE_LEAVE,
	*/
};

struct ResponseAction{
public:
	ResponseAction()
	{
		actionType = ACTION_NONE;
		key = 0;
		intValue = 0;
		strValue = "";
		pos.x = 0;
		pos.y = 0;
		pos.z = 0;
	}

	ReponseActionParam_t actionType;
	int32_t key;
	int32_t intValue;
	std::string strValue;
	Position pos;
};

struct ScriptVars{
	ScriptVars()
	{
		n1 = -1;
		n2 = -1;
		n3 = -1;
		b1 = false;
		b2 = false;
		b3 = false;
		s1 = "";
		s2 = "";
		s3 = "";
	}

	int32_t n1;
	int32_t n2;
	int32_t n3;
	bool b1;
	bool b2;
	bool b3;
	std::string s1;
	std::string s2;
	std::string s3;
};

typedef std::list<ResponseAction> ActionList;
typedef std::map<std::string, int32_t> ResponseScriptMap;
typedef std::list<NpcResponse*> ResponseList;

class NpcResponse
{
public:

	NpcResponse(
		InteractType_t _interactType,
		ResponseType_t _responseType,
		std::list<std::string> _inputList,
		const std::string& _output,
		int32_t _topic,
		int32_t _focusStatus,
		int32_t _storageId,
		int32_t _storageValue,
		StorageComparision_t _storageComp,
		const std::string& _knowSpell,
		uint32_t _params,
		ScriptVars _scriptVars)
	{
		interactType = _interactType;
		responseType = _responseType;
		inputList = _inputList;
		if(!inputList.empty()){
			input = *inputList.begin();
		}
		output = _output;
		params = _params;
		topic = _topic;
		focusStatus = _focusStatus;
		price = -1;
		amount = -1;
		storageId = _storageId;
		storageValue = _storageValue;
		storageComp = _storageComp;
		knowSpell = _knowSpell;
		scriptVars = _scriptVars;
	};

	~NpcResponse()
	{
		for(ResponseList::iterator it = subResponseList.begin(); it != subResponseList.end(); ++it){
			delete *it;
		}

		subResponseList.clear();
	}

	uint32_t getParams() const {return params;}
	const std::string& getInputText() const {return input;}
	int32_t getTopic() const {return topic;}
	int32_t getFocusState() const {return focusStatus;}
	int32_t getStorageId() const {return storageId;}
	int32_t getStorageValue() const {return storageValue;}
	ResponseType_t getResponseType() const {return responseType;}
	InteractType_t getInteractType() const {return interactType;}
	StorageComparision_t getStorageComp() const {return storageComp;}
	const std::string& getKnowSpell() const {return knowSpell;}
	const std::string& getText() const {return output;}
	int32_t getAmount() const {return amount;}

	std::string formatResponseString(Creature* creature) const;
	void addAction(ResponseAction action) {actionList.push_back(action);}
	void setResponseList(ResponseList _list) { subResponseList.insert(subResponseList.end(),_list.begin(),_list.end());}
	const ResponseList& getResponseList() const { return subResponseList;}
	const std::list<std::string>& getInputList() const { return inputList;}
	void setAmount(int32_t _amount) { amount = _amount;}

	ActionList::const_iterator getFirstAction() const {return actionList.begin();}
	ActionList::const_iterator getEndAction() const {return actionList.end();}

	ScriptVars scriptVars;

protected:
	int32_t topic;
	int32_t price;
	int32_t amount;
	int32_t focusStatus;
	std::string input;
	std::list<std::string> inputList;
	std::string output;
	InteractType_t interactType;
	ResponseType_t responseType;
	uint32_t params;
	int32_t storageId;
	int32_t storageValue;
	StorageComparision_t storageComp;
	std::string knowSpell;
	ActionList actionList;
	ResponseList subResponseList;
};

struct NpcState{
	int32_t topic;
	bool isIdle;
	bool isQueued;
	int32_t price;
	int32_t amount;
	int32_t itemId;
	int32_t subType;
	std::string spellName;
	int32_t level;
	uint64_t prevInteraction;
	std::string respondToText;
	uint32_t respondToCreature;
	std::string prevRespondToText;
	const NpcResponse* lastResponse;

	//script variables
	ScriptVars scriptVars;

	//Do not forget to update pushState/popState if you add more variables
};

class Npc : public Creature
{
public:
#ifdef __ENABLE_SERVER_DIAGNOSTIC__
	static uint32_t npcCount;
#endif

	virtual ~Npc();

	virtual Npc* getNpc() {return this;};
	virtual const Npc* getNpc() const {return this;};

	virtual bool isPushable() const { return false;};

	virtual uint32_t idRange(){ return 0x80000000;}
	static AutoList<Npc> listNpc;
	void removeList() {listNpc.removeList(getID());}
	void addList() {listNpc.addList(this);}

	static Npc* createNpc(const std::string& name);

	virtual bool canSee(const Position& pos) const;

	bool load(const std::string& _name);
	void reload();

	virtual const std::string& getName() const {return name;};
	virtual const std::string& getNameDescription() const {return name;};

	void doSay(std::string msg);
	void doMove(Direction dir);
	void doTurn(Direction dir);
	void doMoveTo(Position pos);
	bool isLoaded(){return loaded;}

	void setCreatureFocus(Creature* creature);

	NpcScriptInterface* getScriptInterface();

protected:
	Npc();

	virtual void onAddTileItem(const Tile* tile, const Position& pos, const Item* item);
	virtual void onUpdateTileItem(const Tile* tile, const Position& pos, uint32_t stackpos,
		const Item* oldItem, const ItemType& oldType, const Item* newItem, const ItemType& newType);
	virtual void onRemoveTileItem(const Tile* tile, const Position& pos, uint32_t stackpos,
		const ItemType& iType, const Item* item);
	virtual void onUpdateTile(const Tile* tile, const Position& pos);

	virtual void onCreatureAppear(const Creature* creature, bool isLogin);
	virtual void onCreatureDisappear(const Creature* creature, uint32_t stackpos, bool isLogout);
	virtual void onCreatureMove(const Creature* creature, const Tile* newTile, const Position& newPos,
		const Tile* oldTile, const Position& oldPos, uint32_t oldStackPos, bool teleport);

	virtual void onCreatureTurn(const Creature* creature, uint32_t stackpos);
	virtual void onCreatureSay(const Creature* creature, SpeakClasses type, const std::string& text);
	virtual void onCreatureChangeOutfit(const Creature* creature, const Outfit_t& outfit);
	virtual void onThink(uint32_t interval);
	virtual std::string getDescription(int32_t lookDistance) const;

	bool isImmune(CombatType_t type) const {return true;}
	bool isImmune(ConditionType_t type) const {return true;}
	virtual bool isAttackable() const { return attackable; }
	virtual bool getNextStep(Direction& dir);

	bool canWalkTo(const Position& fromPos, Direction dir);
	bool getRandomStep(Direction& dir);

	void reset();
	bool loadFromXml(const std::string& name);

	const NpcResponse* getResponse(const ResponseList& list, const Player* player,
		NpcState* npcState, const std::string& text, bool exactMatch = false);
	const NpcResponse* getResponse(const Player* player, NpcState* npcState, const std::string& text);
	const NpcResponse* getResponse(const Player* player, NpcState* npcState, NpcEvent_t eventType);
	uint32_t getMatchCount(NpcResponse* response, std::vector<std::string> wordList, bool exactMatch, bool& matchAll);

	void executeResponse(Player* player, NpcState* npcState, const NpcResponse* response);

	std::string formatResponse(Creature* creature, const NpcState* npcState, const NpcResponse* response) const;

	typedef std::map<std::string, std::string> ParametersMap;
	ParametersMap m_parameters;

	uint32_t loadParams(xmlNodePtr node);
	ResponseList loadInteraction(xmlNodePtr node);

	NpcState* getState(const Player* player, bool makeNew = true);

	std::string name;
	std::string m_datadir;
	std::string m_scriptdir;
	uint32_t walkTicks;
	bool floorChange;
	bool attackable;
	bool isIdle;
	bool hasBusyReply;
	int32_t talkRadius;
	int32_t idleTime;
	int32_t focusCreature;

	ResponseScriptMap responseScriptMap;
	ResponseList responseList;

	typedef std::list<NpcState*> StateList;
	StateList stateList;
	NpcEventsHandler* m_npcEventHandler;

	typedef std::list<uint32_t> QueueList;
	QueueList queueList;
	bool loaded;

	static NpcScriptInterface* m_scriptInterface;

	friend class NpcScriptInterface;
};

#endif

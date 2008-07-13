//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
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

#include <string>
#include "shared_ptr.h"

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}

enum ScriptErrorMode {
	SCRIPT_ERROR_PASS,
	SCRIPT_ERROR_WARN,
	SCRIPT_ERROR_THROW,
};

class LuaScriptError : public std::runtime_error {
public:
	LuaScriptError(const std::string& msg) : std::runtime_error(msg) {}
};

class LuaState /*abstract*/ {
public:
	LuaState(ScriptEnviroment& enviroment);
	~LuaState();

	// Stack manipulation
	int getStackTop();
	bool checkStackSize(int low, int high = -1);
	// Check
	bool isBoolean(int index = 1);
	bool isNumber(int index = 1); // No way to decide if it's double or int
	bool isString(int index = 1);
	bool isUserdata(int index = 1);
	bool isLuaFunction(int index = 1);
	bool isCFunction(int index = 1);
	// Pop
	void pop(int n = 1);
	bool popBoolean();
	int32_t popInteger();
	uint32_t popUnsignedInteger();
	double popFloat();
	std::string popString();
	void* getUserdata();
	// Push
	void pushBoolean(bool b);
	void pushInteger(int32_t i);
	void pushUnsignedInteger(uint32_t ui);
	void pushFloat(double d);
	void pushString(const std::string& str);
	void pushUserdata(void* ptr);

	// Events
	void pushEventCallback(EventListener_ptr listener);
	
	// Advanced types
	// Pop
	Thing* popThing(ScriptErrorMode mode = SCRIPTERROR_THROW);
	Creature* popCreature(ScriptErrorMode mode = SCRIPTERROR_THROW);
	Player* popPlayer(ScriptErrorMode mode = SCRIPTERROR_THROW);
	// Push
	void pushThing(Thing* thing);
	
protected:
	void registerClass(const std::string& cname);
	void registerClass(const std::string& cname, const std::string& parent_class);
	
	lua_State* state;
	ScriptEnviroment& enviroment;
};

class LuaThread : public LuaState {
public:
	LuaThread(LuaStateManager& manager, const std::string& name);
	~LuaThread();

	void run();
protected:
	LuaStateManager& manager;
	std::string name;
};

typedef shared_ptr<LuaThread> LuaThread_ptr;
typedef weak_ptr<LuaThread> LuaThread_wptr;

class LuaStateManager : public LuaState {
public:
	LuaStateManager(ScriptEnviroment& enviroment);
	~LuaStateManager();

	LuaThread_ptr newThread();
protected:
	struct LuaThreadSchedule {
		time_t scheduled;
		LuaThread_ptr thread;
		bool operator<(const LuaThreadSchedule& rhs) {
			return scheduled < rhs.scheduled;
		}
	};
	std::priority_queue<LuaThreadSchedule> threads;
};
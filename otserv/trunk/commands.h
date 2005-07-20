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


#ifndef __commands_h_
#define __commands_h_

#include <string>
#include <map>
#include "creature.h"

class Game;
struct Command;
struct s_defcommands;

class Commands{
public:
	Commands():game(NULL),loaded(false){};
	Commands(Game* igame);

	bool loadXml(const std::string &_datadir);	
	bool isLoaded(){return loaded;}
	bool reload();

	bool exeCommand(Creature *creature, const std::string &cmd);
	
	
protected:
	bool loaded;
	Game *game;
	std::string datadir;

	//commands
	bool placeNpc(Creature* c, const std::string &cmd, const std::string &param);
	bool placeMonster(Creature* c, const std::string &cmd, const std::string &param);
	bool broadcastMessage(Creature* c, const std::string &cmd, const std::string &param);
	bool banPlayer(Creature* c, const std::string &cmd, const std::string &param);
	bool teleportMasterPos(Creature* c, const std::string &cmd, const std::string &param);
	bool teleportHere(Creature* c, const std::string &cmd, const std::string &param);
	bool teleportTo(Creature* c, const std::string &cmd, const std::string &param);
	bool createItems(Creature* c, const std::string &cmd, const std::string &param);
	bool substract_contMoney(Creature* c, const std::string &cmd, const std::string &param);
	bool reloadInfo(Creature* c, const std::string &cmd, const std::string &param);
	bool testCommand(Creature* c, const std::string &cmd, const std::string &param);
	bool getInfo(Creature* c, const std::string &cmd, const std::string &param);
	bool closeServer(Creature* c, const std::string &cmd, const std::string &param);
	bool openServer(Creature* c, const std::string &cmd, const std::string &param);
	
	//table of commands
	static s_defcommands defined_commands[];
	
	typedef std::map<std::string,Command*> CommandMap;
	CommandMap commandMap;
	
};

typedef  bool (Commands::*CommandFunc)(Creature*,const std::string&,const std::string&);


struct Command{
	CommandFunc f;
	long accesslevel;
	bool loaded;
};

struct s_defcommands{
	char *name;
	CommandFunc f;
};

#endif //__commands_h_

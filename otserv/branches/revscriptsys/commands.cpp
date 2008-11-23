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
#include "otpch.h"

#include <string>
#include <sstream>
#include <utility>

#include <boost/tokenizer.hpp>
typedef boost::tokenizer<boost::char_separator<char> > tokenizer;

#include "commands.h"
#include "player.h"
#include "npc.h"
#include "monsters.h"
#include "game.h"
#include "house.h"
#include "ioplayer.h"
#include "tools.h"
#include "ban.h"
#include "configmanager.h"
#include "town.h"

#ifdef __ENABLE_SERVER_DIAGNOSTIC__
#include "outputmessage.h"
#include "connection.h"
#include "admin.h"
#include "status.h"
#include "protocollogin.h"
#endif

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

extern ConfigManager g_config;
extern Monsters g_monsters;
extern BanManager g_bans;
extern Game g_game;

extern bool readXMLInteger(xmlNodePtr p, const char *tag, int &value);

#define ipText(a) (unsigned int)a[0] << "." << (unsigned int)a[1] << "." << (unsigned int)a[2] << "." << (unsigned int)a[3]


Commands::Commands()
{
	//table of commands
	s_defcommands defined_commands[] = {
		{"/reload",&Commands::reloadInfo},
#ifdef __ENABLE_SERVER_DIAGNOSTIC__
		{"/serverdiag",&Commands::serverDiag},
#endif
	};

	//setup command map
	for(uint32_t i = 0; i < sizeof(defined_commands) / sizeof(defined_commands[0]); i++){
		Command* cmd = new Command;
		cmd->loaded = false;
		cmd->accesslevel = 1;
		if(defined_commands[i].f == NULL || defined_commands[i].name == NULL)
			continue;
		cmd->f = defined_commands[i].f;
		std::string key = defined_commands[i].name;
		commandMap[key] = cmd;
	}
}

bool Commands::loadXml(const std::string& _datadir)
{
	datadir = _datadir;
	std::string filename = datadir + "commands.xml";

	xmlDocPtr doc = xmlParseFile(filename.c_str());
	if(doc){
		loaded = true;
		xmlNodePtr root, p;
		root = xmlDocGetRootElement(doc);

		if(xmlStrcmp(root->name,(const xmlChar*)"commands") != 0){
			xmlFreeDoc(doc);
			return false;
		}

		std::string strCmd;

		p = root->children;

		while (p){
			if(xmlStrcmp(p->name, (const xmlChar*)"command") == 0){
				if(readXMLString(p, "cmd", strCmd)){
					CommandMap::iterator it = commandMap.find(strCmd);
					int alevel;
					if(it != commandMap.end()){
						if(readXMLInteger(p,"access",alevel)){
							if(!it->second->loaded){
								it->second->accesslevel = alevel;
								it->second->loaded = true;
							}
							else{
								std::cout << "Duplicated command " << strCmd << std::endl;
							}
						}
						else{
							std::cout << "missing access tag for " << strCmd << std::endl;
						}
					}
					else{
						//error
						std::cout << "Unknown command " << strCmd << std::endl;
					}
				}
				else{
					std::cout << "missing cmd." << std::endl;
				}
			}
			p = p->next;
		}
		xmlFreeDoc(doc);
	}

	//
	for(CommandMap::iterator it = commandMap.begin(); it != commandMap.end(); ++it){
		if(it->second->loaded == false){
			std::cout << "Warning: Missing access level for command " << it->first << std::endl;
		}
		//register command tag in game
		g_game.addCommandTag(it->first.substr(0,1));
	}


	return this->loaded;
}

bool Commands::reload()
{
	loaded = false;
	for(CommandMap::iterator it = commandMap.begin(); it != commandMap.end(); ++it){
		it->second->accesslevel = 1;
		it->second->loaded = false;
	}
	g_game.resetCommandTag();

	loadXml(datadir);
	return true;
}

bool Commands::exeCommand(Creature* creature, const std::string& cmd)
{
	std::string str_command;
	std::string str_param;

	std::string::size_type loc = cmd.find( ' ', 0 );
	if(loc != std::string::npos && loc >= 0){
		str_command = std::string(cmd, 0, loc);
		str_param = std::string(cmd, (loc+1), cmd.size()-loc-1);
	}
	else {
		str_command = cmd;
		str_param = std::string("");
	}

	//find command
	CommandMap::iterator it = commandMap.find(str_command);
	if(it == commandMap.end()){
		return false;
	}

	Player* player = creature->getPlayer();
	//check access for this command
	if(player && player->getAccessLevel() < it->second->accesslevel){
		if(player->getAccessLevel() > 0){
			player->sendTextMessage(MSG_STATUS_SMALL, "You can not execute this command.");
		}

		return false;
	}

	//execute command
	CommandFunc cfunc = it->second->f;
	(this->*cfunc)(creature, str_command, str_param);
	if(player)
		player->sendTextMessage(MSG_STATUS_CONSOLE_RED, cmd.c_str());

	return true;
}

bool Commands::reloadInfo(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	
	if(param == "commands"){
		this->reload();
		std::cout << "================================================================================";
		std::cout << ":: Reloaded Commands " << std::endl;
		if(player) player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Reloaded commands.");
	}
	else if(param == "monsters" || param == "monster"){
		g_monsters.reload();
		std::cout << "================================================================================";
		std::cout << ":: Reloaded Monsters " << std::endl;
		if(player) player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Reloaded monsters.");
	}
	else if(param == "config"){
		g_config.reload();
		std::cout << "================================================================================";
		std::cout << ":: Reloaded config " << std::endl;
		if(player) player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Reloaded config.");
	}
	else if(param == "scripts"){
		g_game.loadScripts();
		std::cout << "================================================================================";
		std::cout << ":: Reloaded Scripts " << std::endl;
		if(player) player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Reloaded scripts.");
	}
	/*
	else if(param == "items"){
		Item::items.reload();
	}
	*/
	else{
		if(player) player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Option not found.");
	}

	return true;
}

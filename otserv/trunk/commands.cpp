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
#include "actions.h"
#include "house.h"
#include "ioplayer.h"
#include "tools.h"
#include "ban.h"
#include "configmanager.h"
#include "town.h"

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

extern ConfigManager g_config;
extern Actions actions;
extern Monsters g_monsters;
extern Ban g_bans;

extern bool readXMLInteger(xmlNodePtr p, const char *tag, int &value);

#define ipText(a) (unsigned int)a[0] << "." << (unsigned int)a[1] << "." << (unsigned int)a[2] << "." << (unsigned int)a[3]

//table of commands
s_defcommands Commands::defined_commands[] = { 
	{"/s",&Commands::placeNpc},
	{"/m",&Commands::placeMonster},
	{"/summon",&Commands::placeSummon},
	{"/B",&Commands::broadcastMessage},
	{"/b",&Commands::banPlayer},
	{"/t",&Commands::teleportMasterPos},
	{"/c",&Commands::teleportHere},
	{"/i",&Commands::createItemById},
	{"/n",&Commands::createItemByName},
	{"/q",&Commands::subtractMoney},
	{"/reload",&Commands::reloadInfo},
	{"/z",&Commands::testCommand},
	{"/goto",&Commands::teleportTo},
	{"/info",&Commands::getInfo},
	{"/closeserver",&Commands::closeServer},
	{"/openserver",&Commands::openServer},
	{"/getonline",&Commands::onlineList},
	{"/a",&Commands::teleportNTiles},
	{"/kick",&Commands::kickPlayer},
	{"/owner",&Commands::setHouseOwner},
	{"/sellhouse",&Commands::sellHouse},
	{"/gethouse",&Commands::getHouse},
	{"/bans",&Commands::bansManager},
	{"/exiva",&Commands::exivaPlayer},
	{"/town",&Commands::teleportToTown},
	{"/serverinfo",&Commands::serverInfo},
};


Commands::Commands(Game* igame):
game(igame),
loaded(false)
{
	//setup command map
	for(int i = 0; i < sizeof(defined_commands) / sizeof(defined_commands[0]); i++){
		Command* cmd = new Command;
		cmd->loaded = false;
		cmd->accesslevel = 1;
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
		game->addCommandTag(it->first.substr(0,1));
	}
	
	
	return this->loaded;
}

bool Commands::reload()
{
	this->loaded = false;
	for(CommandMap::iterator it = commandMap.begin(); it != commandMap.end(); ++it){
		it->second->accesslevel = 1;
		it->second->loaded = false;
	}
	game->resetCommandTag();
	this->loadXml(datadir);
	return true;
}

bool Commands::exeCommand(Creature* creature, const std::string& cmd)
{	
	std::string str_command;
	std::string str_param;
	
	unsigned int loc = (uint32_t)cmd.find( ' ', 0 );
	if( loc != std::string::npos && loc >= 0){
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
	if(creature->access < it->second->accesslevel){
		if(creature->access > 0){
			if(player)
				player->sendTextMessage(MSG_STATUS_SMALL, "You can not execute this command.");

			return true;
		}
		else{
			return false;
		}
	}
	//execute command
	CommandFunc cfunc = it->second->f;
	(this->*cfunc)(creature, str_command, str_param);
	if(player)
		player->sendTextMessage(MSG_STATUS_CONSOLE_RED, cmd.c_str());

	return true;
}

bool Commands::placeNpc(Creature* creature, const std::string& cmd, const std::string& param)
{
	Npc* npc = new Npc(param);
	if(!npc->isLoaded()){
		delete npc;
		return true;
	}

	// Place the npc
	if(game->placeCreature(creature->getPosition(), npc)){
		game->AddMagicEffectAt(creature->getPosition(), NM_ME_MAGIC_BLOOD);
		return true;
	}
	else{
		delete npc;
		Player* player = creature->getPlayer();
		if(player){
			player->sendMagicEffect(player->getPosition(), NM_ME_PUFF);
			player->sendCancelMessage(RET_NOTENOUGHROOM);
		}
		return true;
	}

	return false;
}

bool Commands::placeMonster(Creature* creature, const std::string& cmd, const std::string& param)
{
	Monster* monster = Monster::createMonster(param);
	if(!monster){
		return false;
	}

	// Place the monster
	if(game->placeCreature(creature->getPosition(), monster)){
		game->AddMagicEffectAt(creature->getPosition(), NM_ME_MAGIC_BLOOD);
		return true;
	}
	else{
		delete monster;
		Player* player = creature->getPlayer();
		if(player){
			player->sendCancelMessage(RET_NOTENOUGHROOM);
			player->sendMagicEffect(player->getPosition(), NM_ME_PUFF);
		}
	}

	return false;
}

bool Commands::placeSummon(Creature* creature, const std::string& cmd, const std::string& param)
{
	Monster* monster = Monster::createMonster(param);
	if(!monster){
		return false;
	}
	
	// Place the monster
	creature->addSummon(monster);
	if(game->placeCreature(creature->getPosition(), monster)){
		return true;
	}
	else{
		creature->removeSummon(monster);
		//delete monster;

		if(Player* player = creature->getPlayer()) {
			player->sendMagicEffect(player->getPosition(), NM_ME_PUFF);
			player->sendCancelMessage(RET_NOTENOUGHROOM);
		}
	}

	return false;
}

bool Commands::broadcastMessage(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(!player)
		return false;

	game->playerBroadcastMessage(player, param);
	return true;
}

bool Commands::banPlayer(Creature* creature, const std::string& cmd, const std::string& param)
{	
	Player* playerBan = game->getPlayerByName(param);
	if(playerBan) {
		MagicEffectClass me;
		
		me.animationColor = 0xB4;
		me.damageEffect = NM_ME_MAGIC_BLOOD;
		me.maxDamage = (playerBan->health + playerBan->mana)*10;
		me.minDamage = (playerBan->health + playerBan->mana)*10;
		me.offensive = true;

		game->creatureMakeMagic(NULL, playerBan->getPosition(), &me);

		Player* player = creature->getPlayer();
		if(player && player->access <= playerBan->access){
			player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "You cannot ban this player.");
			return true;
		}

		playerBan->sendTextMessage(MSG_STATUS_CONSOLE_RED, "You have been banned.");
		unsigned long ip = playerBan->lastip;
		if(ip > 0) {
			g_bans.addIpBan(ip, 0xFFFFFFFF, 0);
		}

		playerBan->kickPlayer();
		return true;
	}

	return false;
}

bool Commands::teleportMasterPos(Creature* creature, const std::string& cmd, const std::string& param)
{
	Position destPos = creature->getPosition();
	if(game->internalTeleport(creature, creature->masterPos) == RET_NOERROR){
		game->AddMagicEffectAt(destPos, NM_ME_ENERGY_AREA);
		return true;
	}

	return false;
}

bool Commands::teleportHere(Creature* creature, const std::string& cmd, const std::string& param)
{
	Creature* paramCreature = game->getCreatureByName(param);
	if(paramCreature){
		Position destPos = paramCreature->getPosition();
		if(game->internalTeleport(paramCreature, creature->getPosition()) == RET_NOERROR){
			game->AddMagicEffectAt(destPos, NM_ME_ENERGY_AREA);
			return true;
		}
	}

	return false;
}

bool Commands::createItemById(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(!player)
		return false;

	std::string tmp = param;
	
	std::string::size_type pos = tmp.find(' ', 0);
	if(pos == std::string::npos)
		return false;
	
	int type = atoi(tmp.substr(0, pos).c_str());
	tmp.erase(0, pos+1);
	int count = std::min(atoi(tmp.c_str()), 100);
				
	Item* newItem = Item::CreateItem(type, count);
	if(!newItem)
		return false;

	ReturnValue ret = game->internalAddItem(player, newItem);
	
	if(ret != RET_NOERROR){
		ret = game->internalAddItem(player->getTile(), newItem);

		if(ret != RET_NOERROR){
			delete newItem;
			return false;
		}
	}
	
	game->AddMagicEffectAt(player->getPosition(), NM_ME_MAGIC_POISON);
	return true;
}

bool Commands::createItemByName(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(!player)
		return false;

	std::string::size_type pos1 = param.find("\"");
	pos1 = (std::string::npos == pos1 ? 0 : pos1 + 1);

	std::string::size_type pos2 = param.rfind("\"");
	if(pos2 == pos1 || pos2 == std::string::npos){
		pos2 = param.rfind(' ');

		if(pos2 == std::string::npos){
			pos2 = param.size();
		}
	}
	
	std::string itemName = param.substr(pos1, pos2 - pos1);

	int count = 1;
	if(pos2 < param.size()){
		std::string itemCount = param.substr(pos2 + 1, param.size() - (pos2 + 1));
		count = std::min(atoi(itemCount.c_str()), 100);
	}

	int itemId = Item::items.getItemIdByName(itemName);
	if(itemId == -1){
		player->sendTextMessage(MSG_STATUS_CONSOLE_RED, "Item could not be summoned.");
		return false;
	}
				
	Item* newItem = Item::CreateItem(itemId, count);
	if(!newItem)
		return false;

	ReturnValue ret = game->internalAddItem(player, newItem);
	
	if(ret != RET_NOERROR){
		ret = game->internalAddItem(player->getTile(), newItem);

		if(ret != RET_NOERROR){
			delete newItem;
			return false;
		}
	}
	
	game->AddMagicEffectAt(player->getPosition(), NM_ME_MAGIC_POISON);
	return true;
}

bool Commands::subtractMoney(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(!player)
		return false;
				
	int count = atoi(param.c_str());
	uint32_t money = game->getMoney(player);
	if(!count){
		std::stringstream info;
		info << "You have " << money << " gold.";
		player->sendCancel(info.str().c_str());
		return true;
	}
	else if(count > money){
		std::stringstream info;
		info << "You have " << money << " gold and is not sufficient.";
		player->sendCancel(info.str().c_str());
		return true;
	}

	if(!game->removeMoney(player, count)){
		std::stringstream info;
		info << "Can not subtract money!";
		player->sendCancel(info.str().c_str());
	}

	return true;
}

bool Commands::reloadInfo(Creature* creature, const std::string& cmd, const std::string& param)
{	
	if(param == "actions"){
		actions.reload();
	}
	else if(param == "commands"){
		this->reload();
	}
	else if(param == "monsters"){
		g_monsters.reload();
	}
	else if(param == "config"){
		g_config.reload();
	}
	else{
		Player *player = creature->getPlayer();
		if(player)
			player->sendCancel("Option not found.");
	}

	return true;
}

bool Commands::testCommand(Creature* creature, const std::string& cmd, const std::string& param)
{
	int color = atoi(param.c_str());
	Player* player = creature->getPlayer();
	if(player) {
		player->sendMagicEffect(player->getPosition(), color);
		/*
		LightInfo lightInfo;
		lightInfo.level = color / 0x100;
		lightInfo.color = color & 0xFF;
		player->setCreatureLight(lightInfo);
		game->changeLight(player);
		*/
	}

	return true;
}

bool Commands::teleportToTown(Creature* creature, const std::string& cmd, const std::string& param)
{
    std::string tmp = param;
    Player* player = creature->getPlayer();
    
    if(!player)
        return false;
            
    Town* town = Towns::getInstance().getTown(tmp);
    if(town){
        if(game->internalTeleport(creature, town->getTemplePosition()) == RET_NOERROR) {
            game->AddMagicEffectAt(town->getTemplePosition(), NM_ME_ENERGY_AREA);
            return true;
        }
    }
    
    player->sendCancel("Could not find the town.");
    
    return false;    
}

bool Commands::teleportTo(Creature* creature, const std::string& cmd, const std::string& param)
{
	Creature* paramCreature = game->getCreatureByName(param);
	if(paramCreature){
		Position destPos = creature->getPosition();
		if(game->internalTeleport(creature, paramCreature->getPosition()) == RET_NOERROR){
			game->AddMagicEffectAt(destPos, NM_ME_ENERGY_AREA);
			return true;
		}
	}
	
	return false;
}

bool Commands::getInfo(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(!player)
		return true;
	
	Player* paramPlayer = game->getPlayerByName(param);
	if(paramPlayer) {
		std::stringstream info;
		if(paramPlayer->access >= player->access && player != paramPlayer){
			player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "You can not get info about this player.");
			return true;
		}
		unsigned char ip[4];
		*(unsigned long*)&ip = paramPlayer->lastip;
		info << "name:   " << paramPlayer->getName() << std::endl <<
		        "access: " << paramPlayer->access << std::endl <<
		        "level:  " << paramPlayer->getPlayerInfo(PLAYERINFO_LEVEL) << std::endl <<
		        "maglvl: " << paramPlayer->getPlayerInfo(PLAYERINFO_MAGICLEVEL) << std::endl <<
		        "speed:  " <<  paramPlayer->speed <<std::endl <<
		        "position " << paramPlayer->getPosition() << std::endl << 
				"ip: " << ipText(ip);
		player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, info.str().c_str());
	}
	else{
		player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Player not found.");
	}

	return true;
}

bool Commands::closeServer(Creature* creature, const std::string& cmd, const std::string& param)
{
	game->setGameState(GAME_STATE_CLOSED);
	//kick players with access = 0
	AutoList<Player>::listiterator it = Player::listPlayer.list.begin();
	while(it != Player::listPlayer.list.end())
	{
		if((*it).second->access == 0){
			(*it).second->kickPlayer();
			it = Player::listPlayer.list.begin();
		}
		else{
			++it;
		}
	}
	
	Player* player = creature->getPlayer();
	
	if(!g_bans.saveBans(g_config.getString(ConfigManager::BAN_FILE))){
		if(player)
			player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Error while saving bans.");
	}
	
	if(param == "serversave"){
		Houses::getInstance().payHouses();
	}
	
	if(!game->map->saveMap("")){
		if(player)
			player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Error while saving map.");
	}

	return true;
}

bool Commands::openServer(Creature* creature, const std::string& cmd, const std::string& param)
{
	game->setGameState(GAME_STATE_NORMAL);
	return true;
}

bool Commands::onlineList(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(!player)
		return false;

	unsigned long alevelmin = 0;
	unsigned long alevelmax = 10000;
	int i, n;
	
	if(param == "gm")
		alevelmin = 1;
	else if(param == "normal")
		alevelmax = 0;
	
	std::stringstream players;
	players << "name   level   mag" << std::endl;
	
	i = 0;
	n = 0;
	AutoList<Player>::listiterator it = Player::listPlayer.list.begin();
	for(;it != Player::listPlayer.list.end();++it)
	{
		if((*it).second->access >= alevelmin && (*it).second->access <= alevelmax){
			players << (*it).second->getName() << "   " << 
				(*it).second->getPlayerInfo(PLAYERINFO_LEVEL) << "    " <<
				(*it).second->getPlayerInfo(PLAYERINFO_MAGICLEVEL) << std::endl;
			n++;
			i++;
		}
		if(i == 10){
			player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, players.str().c_str());
			players.str("");
			i = 0;
		}
	}
	if(i != 0)
		player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, players.str().c_str());
	
	players.str("");
	players << "Total: " << n << " player(s)" << std::endl;
	player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, players.str().c_str());
	return true;
}

bool Commands::teleportNTiles(Creature* creature, const std::string& cmd, const std::string& param)
{				
	int ntiles = atoi(param.c_str());
	if(ntiles != 0)
	{
		Position newPos = creature->getPosition();
		switch(creature->getDirection()){
		case NORTH:
			newPos.y = newPos.y - ntiles;
			break;
		case SOUTH:
			newPos.y = newPos.y + ntiles;
			break;
		case EAST:
			newPos.x = newPos.x + ntiles;
			break;
		case WEST:
			newPos.x = newPos.x - ntiles;
			break;
		}

		if(game->internalTeleport(creature, newPos) == RET_NOERROR){
			game->AddMagicEffectAt(newPos, NM_ME_ENERGY_AREA);
		}
	}

	return true;
}

bool Commands::kickPlayer(Creature* creature, const std::string &cmd, const std::string &param)
{
	Player* playerKick = game->getPlayerByName(param);
	if(playerKick){
		Player* player = creature->getPlayer();
		if(player && player->access <= playerKick->access){
			player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "You cannot kick this player.");
			return true;
		}

		playerKick->kickPlayer();
		return true;
	}
	return false;
}

bool Commands::exivaPlayer(Creature* creature, const std::string &cmd, const std::string &param)
{
	//a. From 1 to 4 sq's [Person] is standing next to you.
	//b. From 5 to 100 sq's [Person] is to the south, north, east, west.
	//c. From 101 to 274 sq's [Person] is far to the south, north, east, west.
	//d. From 275 to infinite sq's [Person] is very far to the south, north, east, west.
	//e. South-west, s-e, n-w, n-e (corner coordinates): this phrase appears if the player you're looking for has moved five squares in any direction from the south, north, east or west.
	//f. Lower level to the (direction): this phrase applies if the person you're looking for is from 1-25 squares up/down the actual floor you're in.
	//g. Higher level to the (direction): this phrase applies if the person you're looking for is from 1-25 squares up/down the actual floor you're in.

	Player* player = creature->getPlayer();
	if(!player){
		return false;
	}
	enum distance_t{
		DISTANCE_BESIDE,
		DISTANCE_CLOSE_1,
		DISTANCE_CLOSE_2,
		DISTANCE_FAR,
		DISTANCE_VERYFAR,
	};
	
	enum direction_t{
		DIR_N, DIR_S, DIR_E, DIR_W,
		DIR_NE, DIR_NW, DIR_SE, DIR_SW,
	};
	
	enum level_t{
		LEVEL_HIGHER,
		LEVEL_LOWER,
		LEVEL_SAME,
	};

	Player* playerExiva = game->getPlayerByName(param);
	if(playerExiva){
		const Position lookPos = player->getPosition();
		const Position searchPos = playerExiva->getPosition();
		
		long dx = lookPos.x - searchPos.x;
		long dy = lookPos.y - searchPos.y;
		long dz = lookPos.z - searchPos.z;
		
		distance_t distance;
		direction_t direction;
		level_t level;
		//getting floor
		if(dz > 0){
			level = LEVEL_HIGHER;
		}
		else if(dz < 0){
			level = LEVEL_LOWER;
		}
		else{
			level = LEVEL_SAME;
		}
		//getting distance
		if(std::abs(dx) < 4 && std::abs(dy) <4){
			distance = DISTANCE_BESIDE;
		}
		else{
			long distance2 = dx*dx + dy*dy;
			if(distance2 < 625){
				distance = DISTANCE_CLOSE_1;
			}
			else if(distance2 < 10000){
				distance = DISTANCE_CLOSE_2;
			}
			else if(distance2 < 75076){
				distance = DISTANCE_FAR;
			}
			else{
				distance = DISTANCE_VERYFAR;
			}
		}
		//getting direction
		float tan;
		if(dx != 0){
			tan = (float)dy/(float)dx;
		}
		else{
			tan = 10.;
		}
		if(std::abs(tan) < 0.4142){
			if(dx > 0){
				direction = DIR_W;
			}
			else{
				direction = DIR_E;
			}			
		}
		else if(std::abs(tan) < 2.4142){
			if(tan > 0){
				if(dy > 0){
					direction = DIR_NW;
				}
				else{
					direction = DIR_SE;
				}
			}
			else{ //tan < 0
				if(dx > 0){
					direction = DIR_SW;
				}
				else{
					direction = DIR_NE;
				}
			}
		}
		else{
			if(dy > 0){
				direction = DIR_N;
			}
			else{
				direction = DIR_S;
			}
		}
		
		std::stringstream ss;
		ss << playerExiva->getName() << " ";

		if(distance == DISTANCE_BESIDE){
			if(level == LEVEL_SAME)
				ss << "is standing next to you";
			else if(level == LEVEL_HIGHER)
				ss << "is above you";
			else if(level == LEVEL_LOWER)
				ss << "is below you";
			}
		else{
			switch(distance){
				case DISTANCE_CLOSE_1:
					if(level == LEVEL_SAME){
						ss << "is to the";
					}
					else if(level == LEVEL_HIGHER){
						ss << "is on a higher level to the";
					}
					else if(level == LEVEL_LOWER){
						ss << "is on a lower level to the";
					}
					break;

				case DISTANCE_CLOSE_2:
					ss << "is to the";
					break;

				case DISTANCE_FAR:
					ss << "is far to the";
					break;

				case DISTANCE_VERYFAR:
					ss << "is very far to the";
					break;
			}

			ss << " ";
			switch(direction){
				case DIR_N:
					ss << "north";
					break;

				case DIR_S:
					ss << "south";
					break;
				case DIR_E:

					ss << "east";
					break;

				case DIR_W:
					ss << "west";
					break;

				case DIR_NE:
					ss << "north-east";
					break;

				case DIR_NW:
					ss << "north-west";
					break;

				case DIR_SE:
					ss << "south-east";
					break;

				case DIR_SW:
					ss << "south-west";
					break;
			}
		}

		ss << ".";
		player->sendTextMessage(MSG_INFO_DESCR, ss.str().c_str());		
		return true;
	}

	return false;
}

bool Commands::setHouseOwner(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(player){
		if(player->getTile()->hasFlag(TILESTATE_HOUSE)){
			HouseTile* houseTile = dynamic_cast<HouseTile*>(player->getTile());
			if(houseTile){
				
				std::string real_name = param;
				unsigned long guid;
				if(param == "none"){
					houseTile->getHouse()->setHouseOwner(0);
				}
				else if(IOPlayer::instance()->getGuidByName(guid, real_name)){
					houseTile->getHouse()->setHouseOwner(guid);
				}
				else{
					player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Player not found.");
				}
				return true;
			}
		}
	}
	return false;
}

bool Commands::sellHouse(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(player){
		if(!player->getTile()->hasFlag(TILESTATE_HOUSE)){
			player->sendCancel("You are not in a house");	
			return false;
		}
		
		HouseTile* houseTile = dynamic_cast<HouseTile*>(player->getTile());
		if(!houseTile){
			return false;
		}
		
		House* house = houseTile->getHouse();
		if(!(house && house->getHouseOwner() == player->getGUID())){
			player->sendCancel("You are not owner of this house.");
			return false;
		}
		
		Player* tradePartner = game->getPlayerByName(param);
		if(!(tradePartner && tradePartner != player)){
			player->sendCancel("Trade player not found.");
			return false;
		}
		
		if(tradePartner->getPlayerInfo(PLAYERINFO_LEVEL) < 1){
			player->sendCancel("Trade player level is too low.");
			return false;
		}
		
		if(Houses::getInstance().getHouseByPlayerId(tradePartner->getGUID())){
			player->sendCancel("Trade player already owns a house.");
			return false;
		}
		
		if(!Position::areInRange<2,2,0>(tradePartner->getPosition(), player->getPosition())){
			player->sendCancel("Trade player is too far away.");
			return false;
		}
		
		Item* transferItem = house->getTransferItem();
		if(!transferItem){
			player->sendCancel("You can not trade this house.");
			return false;
		}
		
		transferItem->getParent()->setParent(player);
		if(game->internalStartTrade(player, tradePartner, transferItem)){
			return true;
		}
		else{
			house->resetTransferItem();
		}
	}
	return false;
}

bool Commands::getHouse(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(!player)
		return false;
	
	std::string real_name = param;
	unsigned long guid;
	if(IOPlayer::instance()->getGuidByName(guid, real_name)){
		House* house = Houses::getInstance().getHouseByPlayerId(guid);
		std::stringstream str;
		str << real_name;
		if(house){
			str << " owns house: " << house->getName() << ".";
		}
		else{
			str << " does not own any house.";
		}

		player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, str.str().c_str());
	}
	return false;
}

bool Commands::serverInfo(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(!player)
		return false;
	
	std::stringstream text;
	text << "SERVER INFO:";
	text << "\nExp Rate: " << g_config.getNumber(ConfigManager::RATE_EXPERIENCE);
	text << "\nSkill Rate: " << g_config.getNumber(ConfigManager::RATE_SKILL);
	text << "\nMagic Rate: " << g_config.getNumber(ConfigManager::RATE_MAGIC);
	text << "\nLoot Rate: " << g_config.getNumber(ConfigManager::RATE_LOOT);
	
	player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, text.str().c_str());
	
	return true;
}

void showTime(std::stringstream& str, unsigned long time)
{
	if(time == 0xFFFFFFFF){
		str << "permanent";
	}
	else if(time == 0){
		str << "shutdown";
	}
	else{
		time_t tmp = time;
		const tm* tms = localtime(&tmp);
		if(tms){
			str << tms->tm_hour << ":" << tms->tm_min << ":" << tms->tm_sec << "  " << 
				tms->tm_mday << "/" << tms->tm_mon << "/" << tms->tm_year + 1900;
		}
		else{
			str << "UNIX Time : " <<  time;
		}
	}
}

unsigned long parseTime(const std::string& time)
{
	if(time == ""){
		return 0;
	}
	if(time == "permanent"){
		return 0xFFFFFFFF;
	}
	else{
		boost::char_separator<char> sep("+");
		tokenizer timetoken(time, sep);
		tokenizer::iterator timeit = timetoken.begin();
		if(timeit == timetoken.end()){
			return 0;
		}
		unsigned long number = atoi(timeit->c_str());
		unsigned long multiplier = 0;
		++timeit;
		if(timeit == timetoken.end()){
			return 0;
		}
		if(*timeit == "m") //minute
			multiplier = 60;
		if(*timeit == "h") //hour
			multiplier = 60*60;
		if(*timeit == "d") //day
			multiplier = 60*60*24;
		if(*timeit == "w") //week
			multiplier = 60*60*24*7;
		if(*timeit == "o") //month
			multiplier = 60*60*24*30;
		if(*timeit == "y") //year
			multiplier = 60*60*24*365;
			
		uint32_t currentTime = std::time(NULL);
		return currentTime + number*multiplier;
	}
}

std::string parseParams(tokenizer::iterator &it, tokenizer::iterator end)
{
	std::string tmp;
	if(it == end){
		return "";
	}
	else{
		tmp = *it;
		++it;
		if(tmp[0] == '"'){
			tmp.erase(0,1);
			while(it != end && tmp[tmp.length() - 1] != '"'){
				tmp += " " + *it;
				++it;
			}
			tmp.erase(tmp.length() - 1);
		}
		return tmp;
	}
}

bool Commands::bansManager(Creature* creature, const std::string& cmd, const std::string& param)
{
	std::stringstream str;
	Player* player = creature->getPlayer();
	if(!player)
		return false;

	boost::char_separator<char> sep(" ");
	tokenizer cmdtokens(param, sep);
	tokenizer::iterator cmdit = cmdtokens.begin();
	
	if(cmdit != cmdtokens.end() && *cmdit == "add"){
		unsigned char type;
		++cmdit;
		if(cmdit == cmdtokens.end()){
			str << "Parse error.";
			player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, str.str().c_str());
			return true;
		}
		type = cmdit->c_str()[0];
		++cmdit;
		if(cmdit == cmdtokens.end()){
			str << "Parse error.";
			player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, str.str().c_str());
			return true;
		}
		std::string param1;
		std::string param2;
		std::string param3;
		param1 = parseParams(cmdit, cmdtokens.end());
		param2 = parseParams(cmdit, cmdtokens.end());
		param3 = parseParams(cmdit, cmdtokens.end());
		long time = 0;
		switch(type){
		case 'i':
		{
			str << "Add IP-ban is not implemented.";
			break;
		}
		
		case 'p':
		{
			std::string playername = param1;
			unsigned long guid;
			if(!IOPlayer::instance()->getGuidByName(guid, playername)){
				str << "Player not found.";
				break;
			}
			time = parseTime(param2);
			g_bans.addPlayerBan(guid, time);
			str << playername <<  " banished.";
			break;
		}
		
		case 'a':
		{
			long account = atoi(param1.c_str());
			time = parseTime(param2);
			if(account != 0){
				g_bans.addAccountBan(account, time);
				str << "Account " << account << " banished.";
			}
			else{
				str << "Not a valid account.";
			}
			break;
		}
		
		case 'b':
		{
			Player* playerBan = game->getPlayerByName(param1);
			if(!playerBan){
				str << "Player is not online.";
				break;
			}
			time = parseTime(param2);
			long account = playerBan->getAccount();
			if(account){
				g_bans.addAccountBan(account, time);
				str << "Account banished.";
			}
			else{
				str << "Not a valid account.";
			}
			break;
		}

		default:
			str << "Unknown ban type.";
			break;
		}
	}
	else if(cmdit != cmdtokens.end() && *cmdit == "rem"){
		unsigned char type;
		++cmdit;
		if(cmdit == cmdtokens.end()){
			str << "Parse error.";
			player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, str.str().c_str());
			return true;
		}
		type = cmdit->c_str()[0];
		++cmdit;
		if(cmdit == cmdtokens.end()){
			str << "Parse error.";
			player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, str.str().c_str());
			return true;
		}
		unsigned long number = atoi(cmdit->c_str());
		bool ret = false;
		bool typeFound = true;
		switch(type){
		case 'i':
			ret = g_bans.removeIpBan(number);
			break;
		case 'p':
			ret = g_bans.removePlayerBan(number);
			break;
		case 'a':
			ret = g_bans.removeAccountBan(number);
			break;
		default:
			str << "Unknown ban type.";
			typeFound = false;
			break;
		}

		if(typeFound){
			if(!ret){
				str << "Error while removing ban "<<  number <<".";
			}
			else{
				str << "Ban removed.";
			}
		}
	}
	else{
		uint32_t currentTime = std::time(NULL);
		//ip bans
		{
			str << "IP bans: " << std::endl;
			const IpBanList ipBanList = g_bans.getIpBans();
			IpBanList::const_iterator it;
			unsigned char ip[4];
			unsigned char mask[4];
			unsigned long n = 0;
			for(it = ipBanList.begin(); it != ipBanList.end(); ++it){
				n++;
				if(it->time != 0 && it->time < currentTime){
					str << "*";
				}
				str << n << " : ";
				*(unsigned long*)&ip = it->ip;
				*(unsigned long*)&mask = it->mask;
				str << ipText(ip) << " " << ipText(mask) << " ";
				showTime(str, it->time);
				str << std::endl;
				if(str.str().size() > 200){
					player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, str.str().c_str());
					str.str("");
				}
			}
		}
		//player bans
		{
			str << "Player bans: " << std::endl;
			const PlayerBanList playerBanList = g_bans.getPlayerBans();
			PlayerBanList::const_iterator it;
			unsigned long n = 0;
			for(it = playerBanList.begin(); it != playerBanList.end(); ++it){
				n++;
				if(it->time != 0 && it->time < currentTime){
					str << "*";
				}
				str << n << " : ";
				std::string name;
				if(IOPlayer::instance()->getNameByGuid(it->id, name)){
					str << name << " ";
					showTime(str, it->time);
				}
				str << std::endl;
				if(str.str().size() > 200){
					player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, str.str().c_str());
					str.str("");
				}
			}
		}
		//account bans
		{
			str << "Account bans: " << std::endl;
			const AccountBanList accountBanList = g_bans.getAccountBans();
			AccountBanList::const_iterator it;
			unsigned long n = 0;
			for(it = accountBanList.begin(); it != accountBanList.end(); ++it){
				n++;
				if(it->time != 0 && it->time < currentTime){
					str << "*";
				}
				str << n << " : ";
				str << it->id << " ";
				showTime(str, it->time);
				str << std::endl;
				if(str.str().size() > 200){
					player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, str.str().c_str());
					str.str("");
				}
			}
		}
	}
	if(str.str().size() > 0){
		player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, str.str().c_str());
	}
	return true;
}

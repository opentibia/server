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

#include <string>
#include <sstream>
#include <utility>

#include "commands.h"
#include "player.h"
#include "npc.h"
#include "monsters.h"
#include "game.h"
#include "actions.h"
//#include "house.h"

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

extern IPList bannedIPs;
extern Actions actions;
extern Monsters g_monsters;

extern bool readXMLInteger(xmlNodePtr p, const char *tag, int &value);

//table of commands
s_defcommands Commands::defined_commands[] = { 
	{"/s",&Commands::placeNpc},
	{"/m",&Commands::placeMonster},
	{"/summon",&Commands::placeSummon},
	{"/B",&Commands::broadcastMessage},
	{"/b",&Commands::banPlayer},
	{"/t",&Commands::teleportMasterPos},
	{"/c",&Commands::teleportHere},
	{"/i",&Commands::createItems},
	{"/q",&Commands::substract_contMoney},
	{"/reload",&Commands::reloadInfo},
	{"/z",&Commands::testCommand},
	{"/goto",&Commands::teleportTo},
	{"/info",&Commands::getInfo},
	{"/closeserver",&Commands::closeServer},
	{"/openserver",&Commands::openServer},
	{"/getonline",&Commands::onlineList},
	{"/a",&Commands::teleportNTiles},
	{"/kick",&Commands::kickPlayer},
	//{"/invite",&Commands::invitePlayer},
	//{"/uninvite",&Commands::uninvitePlayer},
};


Commands::Commands(Game* igame):
game(igame),
loaded(false)
{
	//setup command map
	for(int i = 0; i < sizeof(defined_commands) / sizeof(defined_commands[0]); i++){
		Command *tmp = new Command;
		tmp->loaded = false;
		tmp->accesslevel = 1;
		tmp->f = defined_commands[i].f;
		std::string key = defined_commands[i].name;
		commandMap[key] = tmp;
	}
}

bool Commands::loadXml(const std::string& _datadir)
{	
	datadir = _datadir;
	
	std::string filename = datadir + "commands.xml";
	std::transform(filename.begin(), filename.end(), filename.begin(), tolower);
	xmlDocPtr doc = xmlParseFile(filename.c_str());

	if (doc){
		this->loaded = true;
		xmlNodePtr root, p;
		root = xmlDocGetRootElement(doc);
		
		if (xmlStrcmp(root->name,(const xmlChar*) "commands")){
			xmlFreeDoc(doc);
			return false;
		}
		p = root->children;
        
		while (p)
		{
			const char* str = (char*)p->name;
			
			if (strcmp(str, "command") == 0){
				char *tmp = (char*)xmlGetProp(p, (const xmlChar *) "cmd");
				if(tmp){
					CommandMap::iterator it = commandMap.find(tmp);
					int alevel;
					if(it != commandMap.end()){
						if(readXMLInteger(p,"access",alevel)){
							if(!it->second->loaded){
								it->second->accesslevel = alevel;
								it->second->loaded = true;
							}
							else{
								std::cout << "Duplicated command " << tmp << std::endl;
							}
						}
						else{
							std::cout << "missing access tag for " << tmp << std::endl;
						}
					}
					else{
						//error
						std::cout << "Unknown command " << tmp << std::endl;
					}
					xmlFreeOTSERV(tmp);
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
			std::cout << "Warning: Missing access level for command" << it->first << std::endl;
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
	Player *player = dynamic_cast<Player*>(creature);
	//check access for this command
	if(creature->access < it->second->accesslevel){
		if(creature->access > 0){
			player->sendTextMessage(MSG_SMALLINFO,"You can not execute this command.");
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
		player->sendTextMessage(MSG_RED_TEXT,cmd.c_str());

	return true;
}

bool Commands::placeNpc(Creature* creature, const std::string& cmd, const std::string& param)
{
	Npc* npc = new Npc(param, game);
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
		Player* player = dynamic_cast<Player*>(creature);
		if(player) {
			player->sendMagicEffect(player->getPosition(), NM_ME_PUFF);
			player->sendCancel("Sorry not enough room.");
		}
		return true;
	}

	return false;
}

bool Commands::placeMonster(Creature* creature, const std::string& cmd, const std::string& param)
{
	Monster* monster = Monster::createMonster(param, game);
	if(!monster){
		return false;
	}

	// Place the monster
	if(game->placeCreature(creature->getPosition(), monster)){
		game->AddMagicEffectAt(creature->getPosition(), NM_ME_MAGIC_BLOOD);
		
		SpectatorVec list;
		SpectatorVec::iterator it;

		game->getSpectators(Range(monster->getPosition(), false), list);
		/*makes monster attack if someone is nearby*/
		for(it = list.begin(); it != list.end(); ++it) {
			monster->onCreatureAppear(*it, false);
		}

		return true;
	}
	else{
		delete monster;
		Player* player = dynamic_cast<Player*>(creature);
		if(player) {
			game->playerSendErrorMessage(player, RET_NOTENOUGHROOM);
			player->sendMagicEffect(player->getPosition(), NM_ME_PUFF);
		}
	}

	return false;
}

bool Commands::placeSummon(Creature* creature, const std::string& cmd, const std::string& param)
{
	Monster* monster = Monster::createMonster(param, game);
	if(!monster){
		return false;
	}
	
	// Place the monster
	if(game->placeCreature(creature->getPosition(), monster)){
		creature->addSummon(monster);
		return true;
	}
	else{
		delete monster;

		if(Player* player = creature->getPlayer()) {
			player->sendMagicEffect(player->getPosition(), NM_ME_PUFF);
			player->sendCancel("Sorry not enough room.");
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
			player->sendTextMessage(MSG_BLUE_TEXT,"You cannot ban this player.");
			return true;
		}

		playerBan->sendTextMessage(MSG_RED_TEXT,"You have been banned.");
		std::pair<unsigned long, unsigned long> IpNetMask;
		IpNetMask.first = playerBan->lastip;
		IpNetMask.second = 0xFFFFFFFF;
		if(IpNetMask.first > 0) {
			bannedIPs.push_back(IpNetMask);
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

bool Commands::createItems(Creature* creature, const std::string& cmd, const std::string& param)
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
	
	game->AddMagicEffectAt(player->getPosition(), NM_ME_MAGIC_POISEN);
	return true;
}

bool Commands::substract_contMoney(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(!player)
		return false;
				
	int count = atoi(param.c_str());
	unsigned long money = player->getMoney();
	if(!count){
		std::stringstream info;
		info << "You have " << money << " of money.";
		player->sendCancel(info.str().c_str());
		return true;
	}
	else if(count > money){
		std::stringstream info;
		info << "You have " << money << " of money and is not suficient.";
		player->sendCancel(info.str().c_str());
		return true;
	}

	if(player->substractMoney(count) != true){
		std::stringstream info;
		info << "Can not substract money!";
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
		/*
		if(player->getTile()->hasFlag(TILESTATE_HOUSE)){
			HouseTile* houseTile = dynamic_cast<HouseTile*>(player->getTile());
			houseTile->getHouse()->setHouseOwner(player->getGUID());
		}
		*/
		//player->sendMagicEffect(player->getPosition(), color);
		LightInfo lightInfo;
		lightInfo.level = color / 0x100;
		lightInfo.color = color & 0xFF;
		player->setCreatureLight(lightInfo);
		game->changeLight(player);
	}

	return true;
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
		unsigned char ip[4];
		if(paramPlayer->access >= player->access && player != paramPlayer){
			player->sendTextMessage(MSG_BLUE_TEXT,"You can not get info about this player.");
			return true;
		}
		*(unsigned long*)&ip = paramPlayer->lastip;
		info << "name:   " << paramPlayer->getName() << std::endl <<
		        "access: " << paramPlayer->access << std::endl <<
		        "level:  " << paramPlayer->getPlayerInfo(PLAYERINFO_LEVEL) << std::endl <<
		        "maglvl: " << paramPlayer->getPlayerInfo(PLAYERINFO_MAGICLEVEL) << std::endl <<
		        "speed:  " <<  paramPlayer->speed <<std::endl <<
		        "position " << paramPlayer->getPosition() << std::endl << 
				"ip: " << (unsigned int)ip[0] << "." << (unsigned int)ip[1] << 
				   "." << (unsigned int)ip[2] << "." << (unsigned int)ip[3];
		player->sendTextMessage(MSG_BLUE_TEXT,info.str().c_str());
	}
	else{
		player->sendTextMessage(MSG_BLUE_TEXT,"Player not found.");
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
			player->sendTextMessage(MSG_BLUE_TEXT,players.str().c_str());
			players.str("");
			i = 0;
		}
	}
	if(i != 0)
		player->sendTextMessage(MSG_BLUE_TEXT,players.str().c_str());
	
	players.str("");
	players << "Total: " << n << " player(s)" << std::endl;
	player->sendTextMessage(MSG_BLUE_TEXT,players.str().c_str());
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

bool Commands::kickPlayer(Creature* c, const std::string &cmd, const std::string &param)
{
	Player* playerKick = game->getPlayerByName(param);
	if(playerKick){
		Player* player = dynamic_cast<Player*>(c);
		if(player && player->access <= playerKick->access){
			player->sendTextMessage(MSG_BLUE_TEXT,"You cannot kick this player.");
			return true;
		}
		playerKick->kickPlayer();
		return true;
	}
	return false;
}

/*
bool Commands::invitePlayer(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(!player)
		return false;

	if(player->getTile()->hasFlag(TILESTATE_HOUSE)){
		HouseTile* houseTile = dynamic_cast<HouseTile*>(player->getTile());
		House* house = houseTile->getHouse();

		if(house->getHouseOwner() == player->getGUID()){
			if(house->addGuest(param) != RET_NOERROR){
				player->sendCancel("Player could not be invited.");
				return false;
			}
		}
	}

	return true;
}

bool Commands::uninvitePlayer(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(!player)
		return false;

	if(player->getTile()->hasFlag(TILESTATE_HOUSE)){
		HouseTile* houseTile = dynamic_cast<HouseTile*>(player->getTile());
		House* house = houseTile->getHouse();

		if(house->getHouseOwner() == player->getGUID()){
			if(house->removeGuest(param) != RET_NOERROR){
				player->sendCancel("Player is not invited.");
				return false;
			}
		}
	}

	return true;
}
*/

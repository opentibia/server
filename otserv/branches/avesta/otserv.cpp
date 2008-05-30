//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// otserv main. The only place where things get instantiated.
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

#include "definitions.h"

#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>

#include "otsystem.h"
#include "server.h"
#include <boost/asio.hpp>

#include <stdlib.h>
#include <time.h>
#include "game.h"


#include "status.h"
#include "monsters.h"
#include "commands.h"
#include "outfit.h"
#include "vocation.h"
#include "scriptmanager.h"
#include "configmanager.h"

#include "tools.h"
#include "ban.h"
#include "rsa.h"
#include "admin.h"
#include "raids.h"

#ifdef __OTSERV_ALLOCATOR__
#include "allocator.h"
#endif

#ifdef __DEBUG_CRITICALSECTION__
OTSYS_THREAD_LOCK_CLASS::LogList OTSYS_THREAD_LOCK_CLASS::loglist;
#endif


#ifdef BOOST_NO_EXCEPTIONS
	#include <exception>
	void boost::throw_exception(std::exception const & e){
		std::cout << "Boost exception: " << e.what() << std::endl;
	}
#endif

IPList serverIPs;

ConfigManager g_config;

Game g_game;
Commands commands(&g_game);
Monsters g_monsters;
BanManager g_bans;
Vocations g_vocations;

RSA* g_otservRSA = NULL;
Server* g_server = NULL;

extern AdminProtocolConfig* g_adminConfig;

#if defined __EXCEPTION_TRACER__
#include "exception.h"
#endif
#include "networkmessage.h"

void ErrorMessage(const char* message) {
	std::cout << std::endl << std::endl << "Error: " << message;

	std::string s;
	std::cin >> s;
}

int main(int argc, char *argv[])
{
#ifdef __OTSERV_ALLOCATOR_STATS__
	OTSYS_CREATE_THREAD(allocatorStatsThread, NULL);
#endif

#if defined __EXCEPTION_TRACER__
	ExceptionHandler mainExceptionHandler;
	mainExceptionHandler.InstallHandler();
#endif

	std::cout << ":: OTServ Development-Version 0.6.0 - Avesta branch" << std::endl;
	std::cout << ":: ==============================================" << std::endl;
	//std::cout << ":: OTServ Version 0.6.0" << std::endl;
	//std::cout << ":: ====================" << std::endl;
	std::cout << "::" << std::endl;

#if defined __DEBUG__MOVESYS__ || defined __DEBUG_HOUSES__ || defined __DEBUG_MAILBOX__ \
	|| defined __DEBUG_LUASCRIPTS__ || defined __DEBUG_RAID__ || defined __DEBUG_NET__ \
	|| defined __DEBUG_SQL__

	std::cout << ":: Debugging:";
	#ifdef __DEBUG__MOVESYS__
	std::cout << " MOVESYS";
	#endif
	#ifdef __DEBUG_MAILBOX__
	std::cout << " MAILBOX";
	#endif
	#ifdef __DEBUG_HOUSES__
	std::cout << " HOUSES";
	#endif
	#ifdef __DEBUG_LUASCRIPTS__
	std::cout << " LUA-SCRIPTS";
	#endif
	#ifdef __DEBUG_RAID__
	std::cout << " RAIDS";
	#endif
	#ifdef __DEBUG_NET__
	std::cout << " NET-ASIO";
	#endif
	#ifdef __DEBUG_SQL__
	std::cout << " SQL";
	#endif
	std::cout << std::endl;
#endif

#if !defined(WIN32) && !defined(__ROOT_PERMISSION__)
	if( getuid() == 0 || geteuid() == 0 ){
		std::cout << std::endl << "OTServ executed as root user, please login with a normal user." << std::endl;
		return 1;
	}
#endif


#if defined __WINDOWS__ || defined WIN32
	//nothing yet
#else
	// ignore sigpipe...
	struct sigaction sigh;
	sigh.sa_handler = SIG_IGN;
	sigh.sa_flags = 0;
	sigemptyset(&sigh.sa_mask);
	sigaction(SIGPIPE, &sigh, NULL);
#endif

//	LOG_MESSAGE("main", EVENT, 1, "Starting server");

	Dispatcher::getDispatcher().addTask(
		createTask(boost::bind(&Game::setGameState, &g_game, GAME_STATE_STARTUP)));

	// random numbers generator
	std::cout << ":: Initializing the random numbers... ";
	srand((unsigned int)OTSYS_TIME());
	std::cout << "[done]" << std::endl;

#if defined __LUA_NAME_ALTER__
	const char* configname = "otserv.lua";
#else
	const char* configname = "config.lua";
#endif

	// read global config
	std::cout << ":: Loading lua script " << configname << "... ";
#if !defined(WIN32) && !defined(__NO_HOMEDIR_CONF__)
	std::string configpath;
	configpath = getenv("HOME");
	configpath += "/.otserv/";
	configpath += configname;
	if (!g_config.loadFile(configpath))
#else
	if (!g_config.loadFile(configname))
#endif
	{
		char errorMessage[26];
		sprintf(errorMessage, "Unable to load %s!", configname);
		ErrorMessage(errorMessage);
		return -1;
	}
	std::cout << "[done]" << std::endl;

	//load RSA key
	std::cout << ":: Loading RSA key...";
	const char* p("14299623962416399520070177382898895550795403345466153217470516082934737582776038882967213386204600674145392845853859217990626450972452084065728686565928113");
	const char* q("7630979195970404721891201847792002125535401292779123937207447574596692788513647179235335529307251350570728407373705564708871762033017096809910315212884101");
	const char* d("46730330223584118622160180015036832148732986808519344675210555262940258739805766860224610646919605860206328024326703361630109888417839241959507572247284807035235569619173792292786907845791904955103601652822519121908367187885509270025388641700821735345222087940578381210879116823013776808975766851829020659073");
	g_otservRSA = new RSA();
	g_otservRSA->setKey(p, q, d);

	std::cout << "[done]" << std::endl;

	//load bans
	/*
	std::cout << ":: Loading bans... ";
	g_bans.init();
	if(!g_bans.loadBans()){
		ErrorMessage("Unable to load bans!");
		return -1;
	}
	std::cout << "[done]" << std::endl;
	*/

	std::stringstream filename;

	//load vocations
	filename.str("");
	filename << g_config.getString(ConfigManager::DATA_DIRECTORY) << "vocations.xml";
	std::cout << ":: Loading " << filename.str() << "... ";
	if(!g_vocations.loadFromXml(g_config.getString(ConfigManager::DATA_DIRECTORY))){
		ErrorMessage("Unable to load vocations!");
		return -1;
	}
	std::cout << "[done]" << std::endl;

	//load commands
	filename.str("");
	filename << g_config.getString(ConfigManager::DATA_DIRECTORY) << "commands.xml";
	std::cout << ":: Loading " << filename.str() << "... ";
	if(!commands.loadXml(g_config.getString(ConfigManager::DATA_DIRECTORY))){
		std::stringstream errormsg;
		errormsg << "Unable to load " << filename.str() << "!";
		ErrorMessage(errormsg.str().c_str());
		return -1;
	}
	std::cout << "[done]" << std::endl;

	// load item data
	filename.str("");
	filename << g_config.getString(ConfigManager::DATA_DIRECTORY) << "items/items.otb";
	std::cout << ":: Loading " << filename.str() << "... ";
	if(Item::items.loadFromOtb(filename.str())){
		std::stringstream errormsg;
		errormsg << "Unable to load " << filename.str() << "!";
		ErrorMessage(errormsg.str().c_str());
		return -1;
	}
	std::cout << "[done]" << std::endl;

	filename.str("");
	filename << g_config.getString(ConfigManager::DATA_DIRECTORY) << "items/items.xml";
	std::cout << ":: Loading " << filename.str() << "... ";
	if(!Item::items.loadFromXml(g_config.getString(ConfigManager::DATA_DIRECTORY))){
		std::stringstream errormsg;
		errormsg << "Unable to load " << filename.str() << "!";
		ErrorMessage(errormsg.str().c_str());
		return -1;
	}
	std::cout << "[done]" << std::endl;

	//load scripts
	if(ScriptingManager::getInstance()->loadScriptSystems() == false){
		return -1;
	}

	// load monster data
	filename.str("");
	filename << g_config.getString(ConfigManager::DATA_DIRECTORY) << "monsters/monsters.xml";
	std::cout << ":: Loading " << filename.str() << "... ";
	if(!g_monsters.loadFromXml(g_config.getString(ConfigManager::DATA_DIRECTORY))){
		std::stringstream errormsg;
		errormsg << "Unable to load " << filename.str() << "!";
		ErrorMessage(errormsg.str().c_str());
		return -1;
	}
	std::cout << "[done]" << std::endl;

	// load outfits data
	filename.str("");
	filename << g_config.getString(ConfigManager::DATA_DIRECTORY) << "outfits.xml";
	std::cout << ":: Loading " << filename.str() << "... ";
	Outfits* outfits = Outfits::getInstance();
	if(!outfits->loadFromXml(g_config.getString(ConfigManager::DATA_DIRECTORY))){
		std::stringstream errormsg;
		errormsg << "Unable to load " << filename.str() << "!";
		ErrorMessage(errormsg.str().c_str());
		return -1;
	}
	std::cout << "[done]" << std::endl;

	//load admin protocol configuration
	filename.str("");
	filename << g_config.getString(ConfigManager::DATA_DIRECTORY) << "admin.xml";
	g_adminConfig = new AdminProtocolConfig();
	std::cout << ":: Loading admin protocol config... ";
	if(!g_adminConfig->loadXMLConfig(g_config.getString(ConfigManager::DATA_DIRECTORY))){
		std::stringstream errormsg;
		errormsg << "Unable to load " << filename.str() << "!";
		ErrorMessage(errormsg.str().c_str());
		return -1;
	}
	std::cout << "[done]" << std::endl;

	std::string worldType = g_config.getString(ConfigManager::WORLD_TYPE);
	std::transform(worldType.begin(), worldType.end(), worldType.begin(), upchar);

	if(worldType == "PVP")
		g_game.setWorldType(WORLD_TYPE_PVP);
	else if(worldType == "NO-PVP")
		g_game.setWorldType(WORLD_TYPE_NO_PVP);
	else if(worldType == "PVP-ENFORCED")
		g_game.setWorldType(WORLD_TYPE_PVP_ENFORCED);
	else{
		ErrorMessage("Unknown world type!");
		return -1;
	}
	std::cout << ":: Worldtype: " << worldType << std::endl;

	#ifdef __SKULLSYSTEM__
	std::cout << ":: Skulls enabled" << std::endl;
	#endif

	std::string passwordType = g_config.getString(ConfigManager::PASSWORD_TYPE_STR);
	std::transform(passwordType.begin(), passwordType.end(), passwordType.begin(), upchar);
	if(passwordType == "" || passwordType == "PLAIN"){
		g_config.setNumber(ConfigManager::PASSWORD_TYPE, PASSWORD_TYPE_PLAIN);
		std::cout << ":: Use plain passwords" << std::endl;
	}
	else if(passwordType == "MD5"){
		g_config.setNumber(ConfigManager::PASSWORD_TYPE, PASSWORD_TYPE_MD5);
		std::cout << ":: Use MD5 passwords" << std::endl;
	}
	else if(passwordType == "SHA1"){
		g_config.setNumber(ConfigManager::PASSWORD_TYPE, PASSWORD_TYPE_SHA1);
		std::cout << ":: Use SHA1 passwords" << std::endl;
	}
	else{
		ErrorMessage("Unknown password type!");
		return -1;
	}

	if(!g_game.loadMap(g_config.getString(ConfigManager::MAP_FILE), g_config.getString(ConfigManager::MAP_KIND))){
		return -1;
	}

	Raids::getInstance()->loadFromXml(g_config.getString(ConfigManager::DATA_DIRECTORY) + "raids/raids.xml");
	Raids::getInstance()->startup();

	std::pair<uint32_t, uint32_t> IpNetMask;
	IpNetMask.first  = inet_addr("127.0.0.1");
	IpNetMask.second = 0xFFFFFFFF;
	serverIPs.push_back(IpNetMask);

	char szHostName[128];
	if(gethostname(szHostName, 128) == 0){
		std::cout << "::" << std::endl << ":: Running on host " << szHostName << std::endl;

		hostent *he = gethostbyname(szHostName);

		if(he){
			std::cout << ":: Local IP address(es):  ";
			unsigned char** addr = (unsigned char**)he->h_addr_list;

			while (addr[0] != NULL){
				std::cout << (unsigned int)(addr[0][0]) << "."
				<< (unsigned int)(addr[0][1]) << "."
				<< (unsigned int)(addr[0][2]) << "."
				<< (unsigned int)(addr[0][3]) << "  ";

				IpNetMask.first  = *(uint32_t*)(*addr);
				IpNetMask.second = 0x0000FFFF;
				serverIPs.push_back(IpNetMask);

				addr++;
			}

			std::cout << std::endl;
		}
	}

	std::cout << ":: Local port:            " << g_config.getNumber(ConfigManager::PORT) << std::endl;

	std::cout << ":: Global IP address:     ";
	std::string ip;

	if(argc > 1)
		ip = argv[1];
	else
		ip = g_config.getString(ConfigManager::IP);

	uint32_t resolvedIp = inet_addr(ip.c_str());
	if(resolvedIp == INADDR_NONE){
		struct hostent* he = gethostbyname(ip.c_str());
		if(he != 0){
			resolvedIp = *(uint32_t*)he->h_addr;
		}
		else{
			std::string error_msg = "Can't resolve: " + ip;
			ErrorMessage(error_msg.c_str());
			return -1;
		}
	}

	char resolvedIpstr[32];
	formatIP(resolvedIp, resolvedIpstr);
	std::cout << resolvedIpstr << std::endl << "::" << std::endl;

	IpNetMask.first  = resolvedIp;
	IpNetMask.second = 0;
	serverIPs.push_back(IpNetMask);
	std::cout << ":: Starting Server... ";

	Status* status = Status::instance();
	status->setMaxPlayersOnline(g_config.getNumber(ConfigManager::MAX_PLAYERS));

	//OTSYS_CREATE_THREAD(Creature::creaturePathThread, NULL);

	Dispatcher::getDispatcher().addTask(
		createTask(boost::bind(&Game::setGameState, &g_game, GAME_STATE_INIT)));

	Dispatcher::getDispatcher().addTask(
		createTask(boost::bind(&Game::setGameState, &g_game, GAME_STATE_NORMAL)));

	Server server(INADDR_ANY, g_config.getNumber(ConfigManager::PORT));
		std::cout << "[done]" << std::endl << ":: OpenTibia Server Running..." << std::endl;
	g_server = &server;
	server.run();

#if defined __EXCEPTION_TRACER__
	mainExceptionHandler.RemoveHandler();
#endif

	return 0;
}

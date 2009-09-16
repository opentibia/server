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

#include "otsystem.h"
#include "tasks.h"
#include "scheduler.h"
#include "server.h"
#include "database.h"
#include "ioplayer.h"
#include "game.h"
#include "vocation.h"
#include "creature_manager.h"
#include "protocolgame.h"
#include "protocolold.h"
#include "protocollogin.h"
#include "status.h"
#include "admin.h"
#include "tools.h"
#include "ban.h"
#include "rsa.h"
#include "configmanager.h"


#if !defined(__WINDOWS__)
	#include <unistd.h> // for access()
#endif

#if !defined(__WINDOWS__)
	#define OTSERV_ACCESS(file,mode) access(file,mode)
#else
	#define OTSERV_ACCESS(file,mode) _access(file,mode)
#endif

#ifdef __OTSERV_ALLOCATOR__
#include "allocator.h"
#endif


#ifdef BOOST_NO_EXCEPTIONS
	#include <exception>
	void boost::throw_exception(std::exception const & e){
		std::cout << "Boost exception: " << e.what() << std::endl;
	}
#endif

Game g_game;
Dispatcher g_dispatcher;
Scheduler g_scheduler;
RSA g_RSA;
ConfigManager g_config;
CreatureManager g_creature_types;
BanManager g_bans;
Vocations g_vocations;
IPList serverIPs;

boost::mutex g_loaderLock;
boost::condition_variable g_loaderSignal;
boost::unique_lock<boost::mutex> g_loaderUniqueLock(g_loaderLock);

extern AdminProtocolConfig* g_adminConfig;

#if defined __EXCEPTION_TRACER__
#include "exception.h"
#endif
#include "networkmessage.h"

#if !defined(__WINDOWS__)
time_t start_time;
#endif


void ErrorMessage(const char* message) {
	std::cout << std::endl << std::endl << "Error: " << message << std::endl;

	std::string s;
	std::cin >> s;
}

void ErrorMessage(std::string m){
	ErrorMessage(m.c_str());
}

struct CommandLineOptions{
	std::string configfile;
	bool truncate_log;
	std::string logfile;
	std::string errfile;
#if !defined(__WINDOWS__)
	std::string runfile;
#endif
};

CommandLineOptions g_command_opts;

bool parseCommandLine(CommandLineOptions& opts, std::vector<std::string> args);
void mainLoader(const CommandLineOptions& command_opts, ServiceManager* servicer);
void badAllocationHandler();

#if !defined(__WINDOWS__)
// Runfile, for running OT as daemon in the background. If the server is shutdown by internal
// means, we need to clear the file to notify the daemon manager of our change in status.
// Note that if the server crashes, this will not happend. :|
void closeRunfile(void)
{
	std::ofstream runfile(g_command_opts.runfile.c_str(), std::ios::trunc | std::ios::out);
	runfile.close(); // Truncate file
}
#endif

int main(int argc, char *argv[])
{
	// Parse any command line (and display help text)
	if(parseCommandLine(g_command_opts, std::vector<std::string>(argv, argv + argc)) == false){
		return 0;
	}
	
#if !defined(__WINDOWS__)
	if(g_command_opts.runfile != ""){
		std::ofstream f(g_command_opts.runfile.c_str(), std::ios::trunc | std::ios::out);
		f << getpid();
		f.close();
		atexit(closeRunfile);
	}
#endif

	// Setup bad allocation handler
	std::set_new_handler(badAllocationHandler);

#if !defined(__WINDOWS__)
	// Create the runfile
	if(g_command_opts.runfile != ""){
		std::ofstream f(g_command_opts.runfile.c_str(), std::ios::trunc | std::ios::out);
		f << getpid();
		f.close();
		atexit(closeRunfile);
	}
#endif

	// Redirect streams if we need to
	// use shared_ptr to guarantee file closing no matter what happens
	boost::shared_ptr<std::ofstream> logfile;
	boost::shared_ptr<std::ofstream> errfile;
	if(g_command_opts.logfile != ""){
		logfile.reset(new std::ofstream(g_command_opts.logfile.c_str(),
			(g_command_opts.truncate_log? std::ios::trunc : std::ios::app) | std::ios::out)
		);
		if(!logfile->is_open()){
			ErrorMessage("Could not open standard log file for writing!");
		}
		std::cout.rdbuf(logfile->rdbuf());
	}
	if(g_command_opts.errfile != ""){
		errfile.reset(new std::ofstream(g_command_opts.errfile.c_str(),
			(g_command_opts.truncate_log? std::ios::trunc : std::ios::app) | std::ios::out)
		);
		if(!errfile->is_open()){
			ErrorMessage("Could not open error log file for writing!");
		}
		std::cerr.rdbuf(errfile->rdbuf());
	}

#if !defined(__WINDOWS__)
	time(&start_time);
#endif
#ifdef __OTSERV_ALLOCATOR_STATS__
	// This keeps track of all allocations, can be used to find memory leak
	boost::thread(boost::bind(&allocatorStatsThread, (void*)NULL));
#endif

	// Provides stack traces when the server crashes, if compiled in.
#if defined __EXCEPTION_TRACER__
	ExceptionHandler mainExceptionHandler;
	mainExceptionHandler.InstallHandler();
#endif

	std::cout << ":: " OTSERV_NAME " Version " OTSERV_VERSION << std::endl;
	std::cout << ":: ============================================================================" << std::endl;
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

	ServiceManager servicer;

	// Start scheduler and dispatcher threads
	g_dispatcher.start();
	g_scheduler.start();

	// Add load task
	g_dispatcher.addTask(createTask(boost::bind(mainLoader, g_command_opts, &servicer)));

	// Wait for loading to finish
	g_loaderSignal.wait(g_loaderUniqueLock);

	if(servicer.is_running()){
		std::cout << "[done]" << std::endl << ":: OpenTibia Server Running..." << std::endl;
		servicer.run();
	}
	else{
		ErrorMessage("No services running. Server is not online.");
	}

#if defined __EXCEPTION_TRACER__
	mainExceptionHandler.RemoveHandler();
#endif
	// Don't run destructors, may hang!
	exit(0);
}

bool parseCommandLine(CommandLineOptions& opts, std::vector<std::string> args)
{
	std::vector<std::string>::iterator argi = args.begin();
	opts.truncate_log = false;

	if(argi != args.end()){
		++argi;
	}


	while(argi != args.end()){
		std::string arg = *argi;
		if(arg == "-p" || arg == "--port"){
			if(++argi == args.end()){
				std::cout << "Missing parameter 1 for '" << arg << "'" << std::endl;
				return false;
			}
			std::string type = *argi;

			if(++argi == args.end()){
				std::cout << "Missing parameter 2 for '" << arg << "'" << std::endl;
				return false;
			}

			if(type == "g" || type == "game")
				g_config.setNumber(ConfigManager::GAME_PORT, atoi(argi->c_str()));
			if(type == "l" || type == "login")
				g_config.setNumber(ConfigManager::LOGIN_PORT, atoi(argi->c_str()));
			if(type == "a" || type == "admin")
				g_config.setNumber(ConfigManager::ADMIN_PORT, atoi(argi->c_str()));
			if(type == "s" || type == "status")
				g_config.setNumber(ConfigManager::STATUS_PORT, atoi(argi->c_str()));
		}
#if !defined(__WINDOWS__)
		else if(arg == "-r" || arg == "--runfile"){
			if(++argi == args.end()){
				std::cout << "Missing parameter for '" << arg << "'" << std::endl;
				return false;
			}
			opts.runfile = *argi;
		}
#endif
		else if(arg == "-i" || arg == "--ip"){
			if(++argi == args.end()){
				std::cout << "Missing parameter for '" << arg << "'" << std::endl;
				return false;
			}
			g_config.setString(ConfigManager::IP, *argi);
		}
		else if(arg == "-c" || arg == "--config"){
			if(++argi == args.end()){
				std::cout << "Missing parameter for '" << arg << "'" << std::endl;
				return false;
			}
			opts.configfile = *argi;
		}
		else if(arg == "--truncate-log"){
			opts.truncate_log = true;
		}
		else if(arg == "-l" || arg == "--log-file"){
			if(++argi == args.end()){
				std::cout << "Missing parameter 1 for '" << arg << "'" << std::endl;
				return false;
			}
			opts.logfile = *argi;
			if(++argi == args.end()){
				std::cout << "Missing parameter 2 for '" << arg << "'" << std::endl;
				return false;
			}
			opts.errfile = *argi;
		}
		else if(arg == "--help"){
			std::cout <<
			"Usage: otserv {-i|-p|-c|-r|-l}\n"
			"\n"
			"\t-i, --ip $1\t\tIP of gameworld server. Should be equal to the \n"
			"\t\t\t\tglobal IP.\n"
			"\t-p, --port $1\t\tPort for server to listen on.\n"
			"\t-c, --config $1\t\tAlternate config file path.\n"
			"\t-l, --log-file $1 $2\tAll standard output will be logged to the $1\n"
			"\t\t\t\tfile, all errors will be logged to $2.\n"
			#if !defined(__WINDOWS__)
			"\t-r, --run-file $1\tSpecifies a runfile. Will contain the pid\n"
			"\t\t\t\tof the server process as long as it is running \n\t\t\t\t(UNIX).\n"
			#endif
			"\t--truncate-log\t\tReset log file each time the server is \n"
			"\t\t\t\tstarted.\n";
			return false;
		}
		else{
			std::cout << "Unrecognized command line argument '" << arg << "'\n"
			"Usage: otserv {-i|-p|-c|-r|-l}" << "\n";
			return false;
		}
		++argi;
	}
	return true;
}

void badAllocationHandler()
{
	// Use functions that only use stack allocation
	puts("Allocation failed, server out of memory.\nDecrese the size of your map or compile in 64-bit mode.");
	char buf[1024];
	fgets(buf, 1024, stdin);
	exit(-1);
}

void mainLoader(const CommandLineOptions& command_opts, ServiceManager* service_manager)
{
	//dispatcher thread
	g_game.setGameState(GAME_STATE_STARTUP);

	// random numbers generator
	std::cout << ":: Initializing the random numbers... " << std::flush;
	srand((unsigned int)OTSYS_TIME());
	std::cout << "[done]" << std::endl;

#if defined LUA_CONFIGFILE
	const char* configname = LUA_CONFIGFILE;
#elif defined __LUA_NAME_ALTER__
	const char* configname = "otserv.lua";
#else
	const char* configname = "config.lua";
#endif
	if(command_opts.configfile != ""){
		configname = command_opts.configfile.c_str();
	}

	// read global config
	std::cout << ":: Loading lua script " << configname << "... " << std::flush;

#ifdef SYSCONFDIR
	std::string sysconfpath;
	sysconfpath = SYSCONFDIR;
	sysconfpath += "/otserv/";
	sysconfpath += configname;
#endif


#if !defined(WIN32) && !defined(__NO_HOMEDIR_CONF__)
	std::string configpath;
	configpath = getenv("HOME");
	configpath += "/.otserv/";
	configpath += configname;

	#ifdef SYSCONFDIR
		if (!g_config.loadFile(configname) && !g_config.loadFile(configpath) && !g_config.loadFile(sysconfpath))
	#else
		if (!g_config.loadFile(configname) && !g_config.loadFile(configpath))
	#endif
#else
	#ifdef SYSCONFDIR
		if (!g_config.loadFile(configname) && !g_config.loadFile(sysconfpath))
	#else
		if (!g_config.loadFile(configname))
	#endif
#endif
	{
		std::ostringstream os;
#if !defined(WIN32) && !defined(__NO_HOMEDIR_CONF__)
		os << "Unable to load " << configname << " or " << configpath;
#else
		os << "Unable to load " << configname;
#endif
		ErrorMessage(os.str());
		exit(-1);
	}
	std::cout << " [done]" << std::endl;

#ifdef WIN32
	std::stringstream mutexName;
	mutexName << "otserv_" << g_config.getNumber(ConfigManager::LOGIN_PORT);
	CreateMutex(NULL, FALSE, mutexName.str().c_str());
	if(GetLastError() == ERROR_ALREADY_EXISTS)
		ErrorMessage("There's an another instance of the OTServ running with the same login port, please shut it down first or change ports for this one.");
#endif

#if defined(PKGDATADIR) && !defined(__WINDOWS__) // I dont care enough to port this to win32, prolly not needed
	// let's fix the datadir, if necessary...
	if (access(g_config.getString(ConfigManager::DATA_DIRECTORY).c_str(), F_OK)) { // check if datadir exists
		// if not then try replacing it with "global" datadir
		std::cout << ":: No datadir '" << g_config.getString(ConfigManager::DATA_DIRECTORY).c_str() << "', using a system-wide one" << std::endl;

		std::string dd = PKGDATADIR;
		dd += "/";
		dd += g_config.getString(ConfigManager::DATA_DIRECTORY);
		g_config.setString(ConfigManager::DATA_DIRECTORY, dd);
	}
#endif
    std::cout << ":: Using data directory " << g_config.getString(ConfigManager::DATA_DIRECTORY).c_str() << "... " << std::flush;
	/* Won't compile! access is not standard
    if (access(g_config.getString(ConfigManager::DATA_DIRECTORY).c_str(), F_OK)) { // check if datadir exists
        ErrorMessage("Data directory does not exist!");
        exit(-1);
    }
	*/
    std::cout << "[done]" << std::endl;

	std::cout << ":: Checking Database Connection... ";
	Database* db = Database::instance();
	if(db == NULL || !db->isConnected())
	{
		ErrorMessage("Database Connection Failed!");
		exit(-1);
	}
	std::cout << "[done]" << std::endl;

	std::cout << ":: Checking Schema version... ";
	DBResult* result;
	if(!(result = db->storeQuery("SELECT `value` FROM `schema_info` WHERE `name` = 'version';"))){
		ErrorMessage("Can't get schema version! Does `schema_info` exist?");
		exit(-1);
	}
	int schema_version = result->getDataInt("value");
	db->freeResult(result);
	if(schema_version != CURRENT_SCHEMA_VERSION){
		ErrorMessage("Your database is outdated. Run the dbupdate utility to update it to the latest schema version.");
		exit(-1);
	}
	std::cout << "Version = " << schema_version << " ";
	std::cout << "[done]" << std::endl;


	//load RSA key
	std::cout << ":: Loading RSA key..." << std::flush;
	const char* p("14299623962416399520070177382898895550795403345466153217470516082934737582776038882967213386204600674145392845853859217990626450972452084065728686565928113");
	const char* q("7630979195970404721891201847792002125535401292779123937207447574596692788513647179235335529307251350570728407373705564708871762033017096809910315212884101");
	const char* d("46730330223584118622160180015036832148732986808519344675210555262940258739805766860224610646919605860206328024326703361630109888417839241959507572247284807035235569619173792292786907845791904955103601652822519121908367187885509270025388641700821735345222087940578381210879116823013776808975766851829020659073");
	g_RSA.setKey(p, q, d);

	std::cout << "[done]" << std::endl;

	std::stringstream filename;

	//load vocations
	filename.str("");
	filename << g_config.getString(ConfigManager::DATA_DIRECTORY) << "vocations.xml";
	std::cout << ":: Loading " << filename.str() << "... " << std::flush;
	if(!g_vocations.loadFromXml(g_config.getString(ConfigManager::DATA_DIRECTORY))){
		ErrorMessage("Unable to load vocations!");
		exit(-1);
	}
	std::cout << "[done]" << std::endl;

	// load item data
	filename.str("");
	filename << g_config.getString(ConfigManager::DATA_DIRECTORY) << "items/items.otb";
	std::cout << ":: Loading " << filename.str() << "... " << std::flush;
	if(Item::items.loadFromOtb(filename.str())){
		std::stringstream errormsg;
		errormsg << "Unable to load " << filename.str() << "!";
		ErrorMessage(errormsg.str().c_str());
		exit(-1);
	}
	std::cout << "[done]" << std::endl;

	filename.str("");
	filename << g_config.getString(ConfigManager::DATA_DIRECTORY) << "items/items.xml";
	std::cout << ":: Loading " << filename.str() << "... " << std::flush;
	if(!Item::items.loadFromXml(g_config.getString(ConfigManager::DATA_DIRECTORY))){
		std::stringstream errormsg;
		errormsg << "Unable to load " << filename.str() << "!";
		ErrorMessage(errormsg.str().c_str());
		exit(-1);
	}
	std::cout << "[done]" << std::endl;

	// load monster data
	filename.str("");
	filename << g_config.getString(ConfigManager::DATA_DIRECTORY) << "monster/monsters.xml";
	std::cout << ":: Loading " << filename.str() << "... " << std::flush;
	if(!g_creature_types.loadFromXml(g_config.getString(ConfigManager::DATA_DIRECTORY))){
		std::stringstream errormsg;
		errormsg << "Unable to load " << filename.str() << "!";
		ErrorMessage(errormsg.str().c_str());
		exit(-1);
	}
	std::cout << "[done]" << std::endl;

	// load outfits data
	filename.str("");
	filename << g_config.getString(ConfigManager::DATA_DIRECTORY) << "outfits.xml";
	std::cout << ":: Loading " << filename.str() << "... " << std::flush;
	Outfits* outfits = Outfits::getInstance();
	if(!outfits->loadFromXml(g_config.getString(ConfigManager::DATA_DIRECTORY))){
		std::stringstream errormsg;
		errormsg << "Unable to load " << filename.str() << "!";
		ErrorMessage(errormsg.str().c_str());
		exit(-1);
	}
	std::cout << "[done]" << std::endl;

	//load admin protocol configuration
	filename.str("");
	filename << g_config.getString(ConfigManager::DATA_DIRECTORY) << "admin.xml";
	g_adminConfig = new AdminProtocolConfig();
	std::cout << ":: Loading admin protocol config... " << std::flush;
	if(!g_adminConfig->loadXMLConfig(g_config.getString(ConfigManager::DATA_DIRECTORY))){
		std::stringstream errormsg;
		errormsg << "Unable to load " << filename.str() << "!";
		ErrorMessage(errormsg.str().c_str());
		exit(-1);
	}
	std::cout << "[done]" << std::endl;

	// Setup scripts
	std::cout << "::" << std::endl;
	std::cout << ":: Loading Scripts ..." << std::endl;
	g_game.loadScripts();
	std::cout << std::endl << "::" << std::endl;

	std::string worldType = g_config.getString(ConfigManager::WORLD_TYPE);

	if(asLowerCaseString(worldType) == "pvp")
		g_game.setWorldType(WORLD_TYPE_PVP);
	else if(asLowerCaseString(worldType) == "no-pvp")
		g_game.setWorldType(WORLD_TYPE_NOPVP);
	else if(asLowerCaseString(worldType) == "pvp-enforced")
		g_game.setWorldType(WORLD_TYPE_PVPE);
	else{
		ErrorMessage("Unknown world type!");
		exit(-1);
	}

	std::cout << ":: Worldtype: " << asUpperCaseString(worldType) << std::endl;
	
	std::cout << ":: Cleaning online players info... " << std::flush;
	if(!IOPlayer::instance()->cleanOnlineInfo()){
		std::stringstream errormsg;
		errormsg << "Unable to execute query for cleaning online status!";
		ErrorMessage(errormsg.str().c_str());
		exit(-1);
	}
	std::cout << "[done]" << std::endl;

	#ifdef __SKULLSYSTEM__
	std::cout << ":: Skulls enabled" << std::endl;
	#endif

	std::string passwordType = g_config.getString(ConfigManager::PASSWORD_TYPE_STR);
	if(passwordType.empty() || asLowerCaseString(passwordType) == "plain"){
		g_config.setNumber(ConfigManager::PASSWORD_TYPE, PASSWORD_TYPE_PLAIN);
		std::cout << ":: Use plain passwords";
	}
	else if(asLowerCaseString(passwordType) == "md5"){
		g_config.setNumber(ConfigManager::PASSWORD_TYPE, PASSWORD_TYPE_MD5);
		std::cout << ":: Use MD5 passwords";
	}
	else if(asLowerCaseString(passwordType) == "sha1"){
		g_config.setNumber(ConfigManager::PASSWORD_TYPE, PASSWORD_TYPE_SHA1);
		std::cout << ":: Use SHA1 passwords";
	}
	else{
		ErrorMessage("Unknown password type!");
		exit(-1);
	}

	if(g_config.getString(ConfigManager::PASSWORD_SALT) != "")
		std::cout << " [salted]";
	std::cout << std::endl;

	if(!g_game.loadMap(g_config.getString(ConfigManager::MAP_FILE),
		g_config.getString(ConfigManager::MAP_KIND))){
		// ok ... so we didn't succeed in laoding the map.
		// perhaps the path to map didn't include path to data directory?
		// let's try to prepend path to datadir before bailing out miserably.
		filename.str("");
		filename << g_config.getString(ConfigManager::DATA_DIRECTORY) << g_config.getString(ConfigManager::MAP_FILE);

		if(!g_game.loadMap(filename.str(), g_config.getString(ConfigManager::MAP_KIND))){
			ErrorMessage("Couldn't load map");
			exit(-1);
		}
	}

	g_game.setGameState(GAME_STATE_INIT);
	g_game.runStartupScripts(true);

	// Tie ports and register services
	
	// Tibia protocols
	service_manager->add<ProtocolGame>(g_config.getNumber(ConfigManager::GAME_PORT));
	service_manager->add<ProtocolLogin>(g_config.getNumber(ConfigManager::LOGIN_PORT));

	// OT protocols
	service_manager->add<ProtocolAdmin>(g_config.getNumber(ConfigManager::ADMIN_PORT));
	service_manager->add<ProtocolStatus>(g_config.getNumber(ConfigManager::STATUS_PORT));

	// Legacy protocols (they need to listen on login port as all old server only used one port
	// which was the login port.)
	service_manager->add<ProtocolOldLogin>(g_config.getNumber(ConfigManager::LOGIN_PORT));
	service_manager->add<ProtocolOldGame>(g_config.getNumber(ConfigManager::LOGIN_PORT));


	// Print ports/ip addresses that we listen too

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

	std::cout << ":: Local ports:           ";
	std::list<uint16_t> ports = service_manager->get_ports();
	while(ports.size()){
		std::cout << ports.front() << "\t";
		ports.pop_front();
	}
	std::cout << std::endl;

	std::cout << ":: Global IP address:     ";
	std::string ip = g_config.getString(ConfigManager::IP);

	uint32_t resolvedIp = inet_addr(ip.c_str());
	if(resolvedIp == INADDR_NONE){
		struct hostent* he = gethostbyname(ip.c_str());
		if(he != 0){
			resolvedIp = *(uint32_t*)he->h_addr;
		}
		else{
			std::string error_msg = "Can't resolve: " + ip;
			ErrorMessage(error_msg.c_str());
			exit(-1);
		}
	}

	std::cout << convertIPToString(resolvedIp) << std::endl << "::" << std::endl;

	IpNetMask.first  = resolvedIp;
	IpNetMask.second = 0;
	serverIPs.push_back(IpNetMask);
	std::cout << ":: Starting Server... ";

	Status* status = Status::instance();
	status->setMaxPlayersOnline(g_config.getNumber(ConfigManager::MAX_PLAYERS));

	g_game.setGameState(GAME_STATE_NORMAL);
	g_game.start(service_manager);
	g_loaderSignal.notify_all();
}

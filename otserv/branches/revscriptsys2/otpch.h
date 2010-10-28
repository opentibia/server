#ifdef __OTSERV_OTCP_H__
#error "Precompiled header should only be included once"
#endif

#define __OTSERV_OTCP_H__

// Definitions should be global.
#include "definitions.h"

#ifdef __WINDOWS__
#include <winerror.h>
#endif

//libxml
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/threads.h>
#include <libxml/xmlschemas.h>
//boost
#include <boost/config.hpp>
#include "boost_common.h"
#include <boost/any.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/tokenizer.hpp>
#include <boost/regex.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string/predicate.hpp>

//std
#include <algorithm>
#include <list>
#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <fstream>
//lua
extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}
//otserv
#include "position.h"
#include "thing.h"
#include "ban.h"
#include "party.h"
#include "cylinder.h"
#include "fileloader.h"
#include "database.h"
#include "databasemysql.h"
#include "databasepgsql.h"
#include "databasesqlite.h"
#include "exception.h"
#include "logger.h"
#include "md5.h"
#include "sha1.h"
#include "rsa.h"
#include "scheduler.h"
#include "tasks.h"
#include "server.h"
#include "status.h"


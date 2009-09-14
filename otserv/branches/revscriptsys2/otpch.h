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

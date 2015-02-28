// Prefix header = compiler automatically includes this header
// Then an error should not be emitted on subsequent includes
#if defined __OTSERV_OTCP_H__ && !defined USE_PREFIX_HEADER
#error "Precompiled header should only be included once"
#endif


#ifndef __OTSERV_OTCP_H__
#define __OTSERV_OTCP_H__

// Definitions should be global.
#include "definitions.h"

#ifdef __WINDOWS__
#include <winerror.h>
#endif

#ifdef __STATIC__
#define LIBXML_STATIC
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
#include <boost/enable_shared_from_this.hpp>

//std
#include <algorithm>
#include <list>
#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <fstream>

//lua
#include "lua.hpp"

// otserv
// These files very rarely changes
#include "position.h"
#include "fileloader.h"
#include "exception.h"
#include "logger.h"
#include "md5.h"
#include "sha1.h"
#include "rsa.h"

// Forward declarations
#include "classes.h"

#endif

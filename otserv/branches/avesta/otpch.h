#ifdef __OTSERV_OTCP_H__
#error "Including more than one time precompiled header."
#endif
#define __OTSERV_OTCP_H__

//#undef __USE_OTPCH__

#ifdef __USE_OTPCH__

#include "definitions.h"

#if defined __WINDOWS__ || defined WIN32
#include <winerror.h>
#endif

//libxml
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/threads.h>
//boost
#include <boost/config.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/tokenizer.hpp>
#include <boost/regex.hpp>
#include <boost/asio.hpp>
//std
#include <list>
#include <vector>
#include <map>
#include <string>
//otserv
#include "thing.h"

#endif

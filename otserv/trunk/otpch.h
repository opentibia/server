#ifdef __OTSERV_OTCP_H__
#error "Including more than one time precompiled header."
#endif
#define __OTSERV_OTCP_H__


#ifdef __USE_OTPCH__

//libxml
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/threads.h>
//boost
#include <boost/config.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/tokenizer.hpp>
#include "boost/regex.hpp"
//std
#include <list>
#include <vector>
#include <map>
#include <string>
//otserv
#include "thing.h"
#include "creature.h"
#include "player.h"
#include "monster.h"
#include "item.h"
#include "game.h"

#endif

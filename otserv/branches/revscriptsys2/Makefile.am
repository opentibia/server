
sbin_PROGRAMS = otserv

AM_CXXFLAGS = $(XML_CPPFLAGS) $(OTSERV_FLAGS) $(LUA_CFLAGS) $(DEBUG_FLAGS)\
	$(MYSQL_FLAGS) $(SQLITE_FLAGS) $(ODBC_FLAGS) $(PGSQL_FLAGS)\
	$(PROFILER_FLAGS) $(OPTIONAL_FLAGS) \
	-DSYSCONFDIR=\"$(sysconfdir)\" -DPKGDATADIR=\"$(pkgdatadir)\" \
	-D_THREAD_SAFE -D_REENTRANT -Wall

otserv_LDADD = $(LUA_LIBS)

if USE_MYSQL
MAYBE_MYSQL = databasemysql.cpp databasemysql.h
endif
if USE_SQLITE
MAYBE_SQLITE = databasesqlite.cpp databasesqlite.h
endif
if USE_ODBC
MAYBE_ODBC = databaseodbc.cpp databaseodbc.h
endif
if USE_PGSQL
MAYBE_PGSQL = databasepgsql.cpp databasepgsql.h
endif

EXTRA_DIST=autogen.sh \
	\
	schema.mysql  schema.pgsql  schema.sqlite config.lua.dist \
	AUTHORS Doxyfile LICENSE Makefile.am README README.DATABASE \
	\
	\
	account.h	 map.h	     scriptmanager.h \
	actions.h	 md5.h	     server.h \
	admin.h		 definitions.h	   monster.h	     sha1.h \
	allocator.h	 depot.h	   monsters.h	     spawn.h \
	ban.h		 enums.h	   movement.h	     spells.h \
	baseevents.h	 exception.h	   networkmessage.h  status.h \
	beds.h		 fileloader.h	   npc.h	     talkaction.h \
	chat.h		 game.h		   otpch.h	     tasks.h \
	combat.h	 house.h	   otsystem.h	     teleport.h \
	commands.h	 housetile.h	   outfit.h	     templates.h \
	condition.h	 ioaccount.h	   outputmessage.h   thing.h \
	config.h	 iomap.h	   party.h	     tile.h \
	configmanager.h  iomapotbm.h	   player.h	     tools.h \
	connection.h	 iomapserialize.h  position.h	     town.h \
	const.h		 iomapxml.h	   protocolgame.h    trashholder.h \
	container.h	 ioplayer.h	   protocol.h	     vocation.h \
	creatureevent.h  item.h		   protocollogin.h   waitlist.h \
	creature.h	 itemloader.h	   protocolold.h     waypoints.h \
	cylinder.h	 items.h	   quests.h	     weapons.h \
	database.h	 logger.h	   raids.h \
	luascript.h	   rsa.h \
	mailbox.h	   scheduler.h
	



otserv_SOURCES = $(MAYBE_MYSQL) $(MAYBE_SQLITE) $(MAYBE_ODBC) $(MAYBE_PGSQL) \
	account.cpp        map.cpp             rsa.cpp \
	actions.cpp        md5.cpp             scheduler.cpp \
	admin.cpp          monster.cpp         server.cpp \
	allocator.cpp      depot.cpp           monsters.cpp \
	ban.cpp	           exception.cpp       sha1.cpp \
	fileloader.cpp     networkmessage.cpp  spawn.cpp \
	beds.cpp           game.cpp            status.cpp \
	chat.cpp           house.cpp           otserv.cpp \
	combat.cpp         housetile.cpp       outfit.cpp \
	commands.cpp       ioaccount.cpp       outputmessage.cpp   tasks.cpp \
	condition.cpp      iomapotbm.cpp       party.cpp           teleport.cpp \
	configmanager.cpp  iomapserialize.cpp  player.cpp          thing.cpp \
	connection.cpp     position.cpp	       tile.cpp \
	container.cpp      ioplayer.cpp        protocol.cpp        tools.cpp \
	creature.cpp       item.cpp	           protocolgame.cpp    trashholder.cpp \
	items.cpp          protocollogin.cpp   vocation.cpp \
	cylinder.cpp       logger.cpp	       protocolold.cpp     waitlist.cpp \
	database.cpp       weapons.cpp         script_environment.cpp \
	mailbox.cpp        raids.cpp           script_environment.h \
	script_event.cpp   script_listener.cpp script_functions.cpp \
	lua_manager.cpp

pkgsysconfdir=$(sysconfdir)/$(PACKAGE)

pkgsysconf_DATA = config.lua

SUBDIRS = data

config.lua:
	cp $(srcdir)/config.lua.dist config.lua


distclean-local:
	-rm -f config.lua.dist config.lua


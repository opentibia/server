
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
	housetile.h	   outfit.h	     templates.h \
	condition.h	 ioaccount.h	   outputmessage.h   thing.h \
	config.h	 iomap.h	   party.h	     tile.h \
	configmanager.h  iomapotbm.h	   player.h	     tools.h \
	connection.h	 iomapserialize.h  position.h	     town.h \
	const.h		 		   protocolgame.h    trashholder.h \
	container.h	 ioplayer.h	   protocol.h	     vocation.h \
	creatureevent.h  item.h		   protocollogin.h   waitlist.h \
	creature.h	 itemloader.h	   protocolold.h     waypoints.h \
	cylinder.h	 items.h	   quests.h	     weapons.h \
	database.h	 logger.h	   raids.h \
	luascript.h	   rsa.h \
	mailbox.h	   scheduler.h \
	guild.h




otserv_SOURCES = $(MAYBE_MYSQL) $(MAYBE_SQLITE) $(MAYBE_ODBC) $(MAYBE_PGSQL) \
	account.cpp	   map.cpp		   rsa.cpp \
	actions.cpp	   md5.cpp		   scheduler.cpp \
	admin.cpp	   monster.cpp	           scriptmanager.cpp \
	allocator.cpp	   depot.cpp	       monsters.cpp	   server.cpp \
	ban.cpp		   exception.cpp       movement.cpp	   sha1.cpp \
	baseevents.cpp	   fileloader.cpp      networkmessage.cpp  spawn.cpp \
	beds.cpp	   game.cpp	       npc.cpp		   spells.cpp \
	chat.cpp	   house.cpp	       otserv.cpp	   status.cpp \
	combat.cpp	   housetile.cpp       outfit.cpp	   talkaction.cpp \
	ioaccount.cpp       outputmessage.cpp   tasks.cpp \
	condition.cpp	   iomapotbm.cpp       party.cpp	   teleport.cpp \
	configmanager.cpp  iomapserialize.cpp  player.cpp	   thing.cpp \
	connection.cpp	   position.cpp	   tile.cpp \
	container.cpp	   ioplayer.cpp        protocol.cpp	   tools.cpp \
	creature.cpp	   item.cpp	       protocolgame.cpp    trashholder.cpp \
	creatureevent.cpp  items.cpp	       protocollogin.cpp   vocation.cpp \
	cylinder.cpp	   logger.cpp	       protocolold.cpp	   waitlist.cpp \
	database.cpp	   luascript.cpp       quests.cpp	   weapons.cpp \
	mailbox.cpp	   raids.cpp \
	guild.cpp

pkgsysconfdir=$(sysconfdir)/$(PACKAGE)

pkgsysconf_DATA = config.lua

config.lua:
	cp $(srcdir)/config.lua.dist config.lua


distclean-local:
	-rm -f config.lua.dist config.lua


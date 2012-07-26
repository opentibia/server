
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

EXTRA_DIST = autogen.sh \
	\
	schema.mysql schema.pgsql schema.sqlite config.lua.dist \
	AUTHORS Doxyfile LICENSE Makefile.am README README.DATABASE \
	\
	\
	actor.h admin.h allocator.h ban.h beds.h \
	boost_common.h chat.h classes.h combat.h \
	condition.h condition_attributes.h configmanager.h \
	connection.h const.h container.h creature.h \
	creature_manager.h creature_type.h cylinder.h \
	database.h definitions.h depot.h enum.h enums.h \
	exception.h fileloader.h game.h house.h housetile.h \
	ioaccount.h iomap.h iomapotbm.h iomapserialize.h \
	ioplayer.h item.h item_attributes.h itemloader.h \
	items.h logger.h lua_manager.h map.h md5.h \
	networkmessage.h otpch.h otsystem.h outfit.h \
	outputmessage.h party.h player.h position.h \
	protocolgame.h protocollogin.h protocolold.h \
	rsa.h scheduler.h script_environment.h script_event.h \
	script_listener.h script_manager.h server.h sha1.h \
	shared_ptr.h spawn.h status.h tasks.h teleport.h \
	templates.h thing.h tile.h tools.h town.h \
	trashholder.h vocation.h waitlist.h waypoints.h




otserv_SOURCES = $(MAYBE_MYSQL) $(MAYBE_SQLITE) $(MAYBE_ODBC) $(MAYBE_PGSQL) \
	actor.cpp admin.cpp allocator.cpp ban.cpp \
	beds.cpp chat.cpp combat.cpp condition.cpp \
	condition_attributes.cpp configmanager.cpp \
	connection.cpp const.cpp container.cpp creature.cpp \
	creature_manager.cpp creature_type.cpp cylinder.cpp \
	database.cpp depot.cpp enums.cpp exception.cpp \
	fileloader.cpp game.cpp house.cpp housetile.cpp \
	ioaccount.cpp iomapotbm.cpp iomapserialize.cpp \
	ioplayer.cpp item.cpp item_attributes.cpp items.cpp \
	logger.cpp lua_manager.cpp map.cpp md5.cpp networkmessage.cpp \
	otpch.cpp otserv.cpp outputmessage.cpp party.cpp \
	player.cpp position.cpp protocol.cpp protocolgame.cpp \
	protocollogin.cpp protocolold.cpp rsa.cpp scheduler.cpp \
	script_environment.cpp script_event.cpp script_functions.cpp \
	script_listener.cpp script_manager.cpp server.cpp sha1.cpp \
	spawn.cpp status.cpp tasks.cpp teleport.cpp thing.cpp \
	tile.cpp tools.cpp trashholder.cpp vocation.cpp waitlist.cpp

pkgsysconfdir=$(sysconfdir)/$(PACKAGE)

pkgsysconf_DATA = config.lua

config.lua:
	cp $(srcdir)/config.lua.dist config.lua

enums:
	lua -l enum condition_attributes.lua enums.lua


distclean-local:
	-rm -f config.lua.dist config.lua \
	enums.cpp enums.h \
	condition_attributes.cpp condition_attributes.h


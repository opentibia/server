
noinst_PROGRAMS = otserv

AM_CXXFLAGS = $(XML_CPPFLAGS) $(OTSERV_FLAGS) $(LUA_CFLAGS) $(DEBUG_FLAGS)\
	$(MYSQL_FLAGS) $(SQLITE_FLAGS) $(ODBC_FLAGS) $(PGSQL_FLAGS)\
	$(PROFILER_FLAGS) $(OPTIONAL_FLAGS) -D_THREAD_SAFE -D_REENTRANT -Wall

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

otserv_SOURCES = $(MAYBE_MYSQL) $(MAYBE_SQLITE) $(MAYBE_ODBC) $(MAYBE_PGSQL) \
	account.cpp container.cpp creature.cpp game.cpp ioaccount.cpp \
	iomapxml.cpp ioplayer.cpp database.cpp database.h \
	item.cpp items.cpp luascript.cpp map.cpp monster.cpp networkmessage.cpp npc.cpp \
	otserv.cpp player.cpp position.cpp protocol80.cpp spells.cpp \
	status.cpp thing.cpp tile.cpp tools.cpp account.h bootstrap.sh config.h const80.h \
	container.h creature.h definitions.h game.h ioaccount.h \
	iomap.h iomapxml.h ioplayer.h item.h items.h \
	luascript.h map.h monster.h networkmessage.h npc.h otsystem.h player.h \
	position.h protocol80.h scheduler.h spells.h stamp-h status.h \
	tasks.h templates.h texcept.h thing.h tile.h tools.h spawn.cpp spawn.h \
	actions.h actions.cpp commands.cpp commands.h depot.cpp depot.h \
	md5.cpp md5.h fileloader.cpp fileloader.h chat.cpp chat.h logger.cpp logger.h \
	iomapotbm.h iomapotbm.cpp monsters.h monsters.cpp teleport.cpp teleport.h \
	trashholder.cpp trashholder.h mailbox.cpp mailbox.h waitlist.cpp waitlist.h \
	house.cpp house.h housetile.cpp housetile.h iomapserialize.cpp iomapserialize.h \
	cylinder.cpp cylinder.h ban.cpp ban.h \
	rsa.cpp rsa.h configmanager.cpp configmanager.h outfit.cpp outfit.h otpch.h \
	admin.cpp admin.h baseevents.cpp baseevents.h \
	raids.cpp raids.h movement.cpp movement.h combat.cpp combat.h \
	condition.cpp condition.h weapons.cpp weapons.h talkaction.cpp talkaction.h \
	vocation.cpp vocation.h scriptmanager.cpp scriptmanager.h server.cpp connection.cpp \
	outputmessage.cpp tasks.cpp scheduler.cpp protocollogin.cpp protocol.cpp creatureevent.cpp \
	creatureevent.h

AUTOMAKE_OPTIONS = foreign

noinst_PROGRAMS = otserv

AM_CXXFLAGS = $(XML_CPPFLAGS) $(OTSERV_FLAGS) $(MYSQL_FLAGS) \
	$(SQLITE_FLAGS) $(PROFILER_FLAGS) -D_THREAD_SAFE -D_REENTRANT -Werror

otserv_LDADD = $(XML_LIBS) $(LUA_LIBS) $(BOOST_LIBS) $(GMP_LIBS) -ldl

if USE_MYSQL
MAYBE_MYSQL = databasemysql.cpp databasemysql.h
endif
if USE_SQLITE
MAYBE_SQLITE = databasesqlite.cpp databasesqlite.h
endif
if USE_SQL
MAYBE_SQL = ioplayersql.cpp ioplayersql.h ioaccountsql.cpp ioaccountsql.h \
	database.cpp database.h iomapserializesql.cpp iomapserializesql.h
endif

otserv_SOURCES = $(MAYBE_SQL) $(MAYBE_MYSQL) $(MAYBE_SQLITE) \
	account.cpp container.cpp creature.cpp game.cpp ioaccount.cpp \
	ioaccountxml.cpp iomapxml.cpp ioplayer.cpp ioplayerxml.cpp \
	item.cpp items.cpp luascript.cpp map.cpp monster.cpp networkmessage.cpp npc.cpp \
	otserv.cpp player.cpp position.cpp protocol79.cpp spells.cpp \
	status.cpp thing.cpp tile.cpp tools.cpp account.h bootstrap.sh config.h const78.h \
	container.h creature.h definitions.h game.h ioaccount.h ioaccountxml.h \
	iomap.h iomapxml.h ioplayer.h ioplayerxml.h item.h items.h \
	luascript.h map.h monster.h networkmessage.h npc.h otsystem.h player.h \
	position.h protocol79.h scheduler.h spells.h stamp-h status.h \
	tasks.h templates.h texcept.h thing.h tile.h tools.h spawn.cpp spawn.h \
	actions.h actions.cpp scheduler.cpp commands.cpp commands.h depot.cpp depot.h \
	md5.cpp md5.h fileloader.cpp fileloader.h chat.cpp chat.h logger.cpp logger.h \
	iomapotbm.h iomapotbm.cpp monsters.h monsters.cpp teleport.cpp teleport.h \
	trashholder.cpp trashholder.h mailbox.cpp mailbox.h waitlist.cpp waitlist.h \
	house.cpp house.h housetile.cpp housetile.h iomapserialize.cpp iomapserialize.h \
	iomapserializexml.cpp iomapserializexml.h cylinder.cpp cylinder.h ban.cpp ban.h \
	rsa.cpp rsa.h configmanager.cpp configmanager.h outfit.cpp outfit.h otpch.h \
	itemloader.cpp itemloader.h admin.cpp admin.h baseevents.cpp baseevents.h \
	raids.cpp raids.h movement.cpp movement.h combat.cpp combat.h \
	condition.cpp condition.h weapons.cpp weapons.h talkaction.cpp talkaction.h \
	vocation.cpp vocation.h scriptmanager.cpp scriptmanager.h



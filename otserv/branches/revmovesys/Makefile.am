AUTOMAKE_OPTIONS = foreign

noinst_PROGRAMS = otserv


AM_CXXFLAGS = $(XML_CPPFLAGS) $(OTSERV_FLAGS) $(LUA50_CFLAGS) $(MYSQL_FLAGS) -D_THREAD_SAFE -D_REENTRANT -Werror
otserv_LDADD = $(XML_LIBS) $(LUA50_LIBS) $(BOOST_LIBS) -ldl
if USE_MYSQL
otserv_SOURCES = account.cpp container.cpp creature.cpp game.cpp ioaccount.cpp \
	iomapxml.cpp ioplayer.cpp \
	item.cpp items.cpp luascript.cpp magic.cpp map.cpp monster.cpp networkmessage.cpp npc.cpp \
	otserv.cpp player.cpp position.cpp protocol.cpp protocol75.cpp spells.cpp \
	status.cpp thing.cpp tile.cpp tools.cpp account.h bootstrap.sh config.h const75.h \
	container.h creature.h definitions.h game.h ioaccount.h depot.cpp depot.h \
	iomap.h iomapxml.h ioplayer.h item.h items.h monsters.cpp monsters.h\
	luascript.h magic.h map.h monster.h networkmessage.h npc.h otsystem.h player.h \
	position.h protocol.h protocol75.h scheduler.h spells.h stamp-h status.h \
	tasks.h templates.h texcept.h thing.h tile.h tools.h spawn.cpp spawn.h \
	actions.h actions.cpp scheduler.cpp ioplayersql.cpp ioplayersql.h ioaccountsql.cpp ioaccountsql.h \
	commands.cpp commands.h database.cpp database.h md5.cpp md5.h fileloader.cpp fileloader.h \
	chat.cpp chat.h logger.h logger.cpp cylinder.cpp cylinder.h teleport.cpp teleport.h
else
otserv_SOURCES = account.cpp container.cpp creature.cpp game.cpp ioaccount.cpp \
	ioaccountxml.cpp iomapxml.cpp ioplayer.cpp ioplayerxml.cpp \
	item.cpp items.cpp luascript.cpp magic.cpp map.cpp monster.cpp networkmessage.cpp npc.cpp \
	otserv.cpp player.cpp position.cpp protocol.cpp protocol75.cpp spells.cpp \
	status.cpp thing.cpp tile.cpp tools.cpp account.h bootstrap.sh config.h const75.h \
	container.h creature.h definitions.h game.h ioaccount.h ioaccountxml.h \
	iomap.h iomapxml.h ioplayer.h ioplayerxml.h item.h items.h \
	luascript.h magic.h map.h monster.h networkmessage.h npc.h otsystem.h player.h \
	position.h protocol.h protocol75.h scheduler.h spells.h stamp-h status.h \
	tasks.h templates.h texcept.h thing.h tile.h tools.h spawn.cpp spawn.h \
	actions.h actions.cpp scheduler.cpp commands.cpp commands.h monsters.cpp monsters.h \
	md5.cpp md5.h fileloader.cpp fileloader.h chat.cpp chat.h logger.cpp logger.h \
	cylinder.cpp cylinder.h teleport.cpp teleport.h depot.cpp depot.h
endif
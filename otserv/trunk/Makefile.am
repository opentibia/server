AUTOMAKE_OPTIONS = foreign

noinst_PROGRAMS = otserv


AM_CXXFLAGS = $(XML_CPPFLAGS) $(OTSERV_FLAGS) $(LUA50_CFLAGS) -D_THREAD_SAFE -D_REENTRANT -Wall -Werror
otserv_LDADD = $(XML_LIBS) $(LUA50_LIBS) $(BOOST_LIBS) -ldl
otserv_SOURCES = account.cpp container.cpp creature.cpp game.cpp ioaccount.cpp \
					  ioaccountxml.cpp iomapxml.cpp ioplayer.cpp ioplayersql.cpp ioplayerxml.cpp \
					  item.cpp items.cpp luascript.cpp magic.cpp map.cpp networkmessage.cpp npc.cpp \
					  otserv.cpp player.cpp position.cpp protocol.cpp protocol70.cpp spells.cpp \
					  status.cpp thing.cpp tile.cpp tools.cpp account.h bootstrap.sh config.h \
					  container.h creature.h definitions.h game.h ioaccount.h ioaccountxml.h \
					  iomap.h iomapxml.h ioplayer.h ioplayersql.h ioplayerxml.h item.h items.h \
					  luascript.h magic.h map.h networkmessage.h npc.h otsystem.h player.h \
					  position.h protocol.h protocol70.h scheduler.h spells.h stamp-h status.h \
					  tasks.h texcept.h thing.h tile.h tools.h

AUTOMAKE_OPTIONS = foreign

noinst_PROGRAMS = otserv


AM_CXXFLAGS = $(XML_CPPFLAGS) $(OTSERV_FLAGS) $(LUA50_CFLAGS) -D__DEBUG__
otserv_LDADD = $(XML_LIBS) $(LUA50_LIBS)
otserv_SOURCES = creature.cpp creature.h definitions.h item.cpp item.h \
					  items.cpp items.h luascript.cpp luascript.h map.cpp \
					  map.h networkmessage.cpp networkmessage.h npc.cpp \
					  npc.h otserv.cpp otsystem.h player.cpp player.h \
					  position.h protocol.cpp protocol.h protocol70.cpp \
					  protocol70.h texcept.h thing.cpp thing.h tile.cpp \
					  tile.h tools.cpp position.cpp scheduler.h account.h \
					  account.cpp

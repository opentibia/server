AUTOMAKE_OPTIONS = foreign

noinst_PROGRAMS = otserv

CXXFLAGS = $(XML_CPPFLAGS) $(OTSERV_FLAGS) $(LUA50_CFLAGS)
LIBS = $(XML_LIBS) $(LUA50_LIBS)

otserv_SOURCES = action.h creature.h definitions.h eventscheduler.cpp \
eventscheduler.h file.cpp file.h item.cpp item.h items.cpp items.h memory.h \
network.cpp network.h otserv.cpp player.cpp player.h player_mem.cpp \
player_mem.h pos.h protokoll.cpp protokoll.h serversocket.cpp serversocket.h \
texcept.h tmap.cpp tmap.h tools.cpp tools.h tprot65.cpp tprot65.h tprot69.cpp \
tprot69.h tprot70.cpp tprot70.h tprot.cpp tprot.h npc.cpp npc.h luascript.h luascript.cpp basescript.cpp basescript.h itemscript.cpp itemscript.h

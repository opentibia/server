srcdir = .

CXX = g++
CXXFLAGS = -g -O2
LDFLAGS = 
DEFINES= -D__LINUX__ 
EXEEXT = 
OBJEXT = o

OBJS = item.$(OBJEXT) network.$(OBJEXT) player.$(OBJEXT) player_mem.$(OBJEXT) \
protokoll.$(OBJEXT) tprot.$(OBJEXT)

all: $(OBJS)

.cpp.$(OBJEXT):
	$(CXX) -c $(DEFINES) $(CXXFLAGS) $<

clean:
	rm -f $(OBJS)

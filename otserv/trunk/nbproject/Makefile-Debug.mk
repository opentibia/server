#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=
AS=as

# Macros
CND_PLATFORM=MinGW-Windows
CND_CONF=Debug
CND_DISTDIR=dist

# Include project Makefile
include Makefile.NetBeans

# Object Directory
OBJECTDIR=build/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/npc.o \
	${OBJECTDIR}/map.o \
	${OBJECTDIR}/sha1.o \
	${OBJECTDIR}/ioplayer.o \
	${OBJECTDIR}/position.o \
	${OBJECTDIR}/protocol.o \
	${OBJECTDIR}/raids.o \
	${OBJECTDIR}/databasepgsql.o \
	${OBJECTDIR}/baseevents.o \
	${OBJECTDIR}/iomapotbm.o \
	${OBJECTDIR}/protocolgame.o \
	${OBJECTDIR}/luascript.o \
	${OBJECTDIR}/spells.o \
	${OBJECTDIR}/account.o \
	${OBJECTDIR}/databasemysql.o \
	${OBJECTDIR}/tasks.o \
	${OBJECTDIR}/rsa.o \
	${OBJECTDIR}/creature.o \
	${OBJECTDIR}/scriptmanager.o \
	${OBJECTDIR}/game.o \
	${OBJECTDIR}/outputmessage.o \
	${OBJECTDIR}/item.o \
	${OBJECTDIR}/party.o \
	${OBJECTDIR}/container.o \
	${OBJECTDIR}/protocolold.o \
	${OBJECTDIR}/protocollogin.o \
	${OBJECTDIR}/iomapserialize.o \
	${OBJECTDIR}/md5.o \
	${OBJECTDIR}/databasesqlite.o \
	${OBJECTDIR}/cylinder.o \
	${OBJECTDIR}/teleport.o \
	${OBJECTDIR}/mailbox.o \
	${OBJECTDIR}/vocation.o \
	${OBJECTDIR}/scheduler.o \
	${OBJECTDIR}/otserv.o \
	${OBJECTDIR}/ioaccount.o \
	${OBJECTDIR}/tile.o \
	${OBJECTDIR}/ban.o \
	${OBJECTDIR}/networkmessage.o \
	${OBJECTDIR}/chat.o \
	${OBJECTDIR}/server.o \
	${OBJECTDIR}/thing.o \
	${OBJECTDIR}/database.o \
	${OBJECTDIR}/housetile.o \
	${OBJECTDIR}/waitlist.o \
	${OBJECTDIR}/movement.o \
	${OBJECTDIR}/spawn.o \
	${OBJECTDIR}/depot.o \
	${OBJECTDIR}/status.o \
	${OBJECTDIR}/creatureevent.o \
	${OBJECTDIR}/connection.o \
	${OBJECTDIR}/condition.o \
	${OBJECTDIR}/player.o \
	${OBJECTDIR}/fileloader.o \
	${OBJECTDIR}/tools.o \
	${OBJECTDIR}/actions.o \
	${OBJECTDIR}/trashholder.o \
	${OBJECTDIR}/quests.o \
	${OBJECTDIR}/combat.o \
	${OBJECTDIR}/allocator.o \
	${OBJECTDIR}/beds.o \
	${OBJECTDIR}/weapons.o \
	${OBJECTDIR}/items.o \
	${OBJECTDIR}/outfit.o \
	${OBJECTDIR}/house.o \
	${OBJECTDIR}/configmanager.o \
	${OBJECTDIR}/logger.o \
	${OBJECTDIR}/talkaction.o \
	${OBJECTDIR}/databaseodbc.o \
	${OBJECTDIR}/exception.o \
	${OBJECTDIR}/monster.o \
	${OBJECTDIR}/admin.o \
	${OBJECTDIR}/monsters.o

# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=-llibxml2 -llibmysql -llibsqlite3 -lgmp -llua -llibboost_system-mgw44-mt-1_41 -llibboost_thread-mgw44-mt-1_41 -llibboost_regex-mgw44-mt-1_41 -lwsock32 -lws2_32 -Wl,-Map=C:\otserv\otserv.map

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	${MAKE}  -f nbproject/Makefile-Debug.mk dist/Debug/MinGW-Windows/trunk.exe

dist/Debug/MinGW-Windows/trunk.exe: ${OBJECTFILES}
	${MKDIR} -p dist/Debug/MinGW-Windows
	g++ -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/trunk ${OBJECTFILES} ${LDLIBSOPTIONS} 

${OBJECTDIR}/npc.o: nbproject/Makefile-${CND_CONF}.mk npc.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/npc.o npc.cpp

${OBJECTDIR}/map.o: nbproject/Makefile-${CND_CONF}.mk map.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/map.o map.cpp

${OBJECTDIR}/sha1.o: nbproject/Makefile-${CND_CONF}.mk sha1.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/sha1.o sha1.cpp

${OBJECTDIR}/ioplayer.o: nbproject/Makefile-${CND_CONF}.mk ioplayer.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/ioplayer.o ioplayer.cpp

${OBJECTDIR}/position.o: nbproject/Makefile-${CND_CONF}.mk position.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/position.o position.cpp

${OBJECTDIR}/protocol.o: nbproject/Makefile-${CND_CONF}.mk protocol.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/protocol.o protocol.cpp

${OBJECTDIR}/raids.o: nbproject/Makefile-${CND_CONF}.mk raids.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/raids.o raids.cpp

${OBJECTDIR}/databasepgsql.o: nbproject/Makefile-${CND_CONF}.mk databasepgsql.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/databasepgsql.o databasepgsql.cpp

${OBJECTDIR}/baseevents.o: nbproject/Makefile-${CND_CONF}.mk baseevents.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/baseevents.o baseevents.cpp

${OBJECTDIR}/iomapotbm.o: nbproject/Makefile-${CND_CONF}.mk iomapotbm.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/iomapotbm.o iomapotbm.cpp

${OBJECTDIR}/protocolgame.o: nbproject/Makefile-${CND_CONF}.mk protocolgame.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/protocolgame.o protocolgame.cpp

${OBJECTDIR}/luascript.o: nbproject/Makefile-${CND_CONF}.mk luascript.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/luascript.o luascript.cpp

${OBJECTDIR}/spells.o: nbproject/Makefile-${CND_CONF}.mk spells.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/spells.o spells.cpp

${OBJECTDIR}/account.o: nbproject/Makefile-${CND_CONF}.mk account.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/account.o account.cpp

${OBJECTDIR}/databasemysql.o: nbproject/Makefile-${CND_CONF}.mk databasemysql.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/databasemysql.o databasemysql.cpp

${OBJECTDIR}/tasks.o: nbproject/Makefile-${CND_CONF}.mk tasks.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/tasks.o tasks.cpp

${OBJECTDIR}/rsa.o: nbproject/Makefile-${CND_CONF}.mk rsa.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/rsa.o rsa.cpp

${OBJECTDIR}/creature.o: nbproject/Makefile-${CND_CONF}.mk creature.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/creature.o creature.cpp

${OBJECTDIR}/scriptmanager.o: nbproject/Makefile-${CND_CONF}.mk scriptmanager.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/scriptmanager.o scriptmanager.cpp

${OBJECTDIR}/game.o: nbproject/Makefile-${CND_CONF}.mk game.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/game.o game.cpp

${OBJECTDIR}/outputmessage.o: nbproject/Makefile-${CND_CONF}.mk outputmessage.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/outputmessage.o outputmessage.cpp

${OBJECTDIR}/item.o: nbproject/Makefile-${CND_CONF}.mk item.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/item.o item.cpp

${OBJECTDIR}/party.o: nbproject/Makefile-${CND_CONF}.mk party.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/party.o party.cpp

${OBJECTDIR}/container.o: nbproject/Makefile-${CND_CONF}.mk container.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/container.o container.cpp

${OBJECTDIR}/protocolold.o: nbproject/Makefile-${CND_CONF}.mk protocolold.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/protocolold.o protocolold.cpp

${OBJECTDIR}/protocollogin.o: nbproject/Makefile-${CND_CONF}.mk protocollogin.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/protocollogin.o protocollogin.cpp

${OBJECTDIR}/iomapserialize.o: nbproject/Makefile-${CND_CONF}.mk iomapserialize.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/iomapserialize.o iomapserialize.cpp

${OBJECTDIR}/md5.o: nbproject/Makefile-${CND_CONF}.mk md5.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/md5.o md5.cpp

${OBJECTDIR}/databasesqlite.o: nbproject/Makefile-${CND_CONF}.mk databasesqlite.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/databasesqlite.o databasesqlite.cpp

${OBJECTDIR}/cylinder.o: nbproject/Makefile-${CND_CONF}.mk cylinder.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/cylinder.o cylinder.cpp

${OBJECTDIR}/teleport.o: nbproject/Makefile-${CND_CONF}.mk teleport.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/teleport.o teleport.cpp

${OBJECTDIR}/mailbox.o: nbproject/Makefile-${CND_CONF}.mk mailbox.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/mailbox.o mailbox.cpp

${OBJECTDIR}/vocation.o: nbproject/Makefile-${CND_CONF}.mk vocation.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/vocation.o vocation.cpp

${OBJECTDIR}/scheduler.o: nbproject/Makefile-${CND_CONF}.mk scheduler.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/scheduler.o scheduler.cpp

${OBJECTDIR}/otserv.o: nbproject/Makefile-${CND_CONF}.mk otserv.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/otserv.o otserv.cpp

${OBJECTDIR}/ioaccount.o: nbproject/Makefile-${CND_CONF}.mk ioaccount.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/ioaccount.o ioaccount.cpp

${OBJECTDIR}/tile.o: nbproject/Makefile-${CND_CONF}.mk tile.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/tile.o tile.cpp

${OBJECTDIR}/ban.o: nbproject/Makefile-${CND_CONF}.mk ban.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/ban.o ban.cpp

${OBJECTDIR}/networkmessage.o: nbproject/Makefile-${CND_CONF}.mk networkmessage.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/networkmessage.o networkmessage.cpp

${OBJECTDIR}/chat.o: nbproject/Makefile-${CND_CONF}.mk chat.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/chat.o chat.cpp

${OBJECTDIR}/server.o: nbproject/Makefile-${CND_CONF}.mk server.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/server.o server.cpp

${OBJECTDIR}/thing.o: nbproject/Makefile-${CND_CONF}.mk thing.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/thing.o thing.cpp

${OBJECTDIR}/database.o: nbproject/Makefile-${CND_CONF}.mk database.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/database.o database.cpp

${OBJECTDIR}/housetile.o: nbproject/Makefile-${CND_CONF}.mk housetile.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/housetile.o housetile.cpp

${OBJECTDIR}/waitlist.o: nbproject/Makefile-${CND_CONF}.mk waitlist.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/waitlist.o waitlist.cpp

${OBJECTDIR}/movement.o: nbproject/Makefile-${CND_CONF}.mk movement.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/movement.o movement.cpp

${OBJECTDIR}/spawn.o: nbproject/Makefile-${CND_CONF}.mk spawn.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/spawn.o spawn.cpp

${OBJECTDIR}/depot.o: nbproject/Makefile-${CND_CONF}.mk depot.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/depot.o depot.cpp

${OBJECTDIR}/status.o: nbproject/Makefile-${CND_CONF}.mk status.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/status.o status.cpp

${OBJECTDIR}/creatureevent.o: nbproject/Makefile-${CND_CONF}.mk creatureevent.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/creatureevent.o creatureevent.cpp

${OBJECTDIR}/connection.o: nbproject/Makefile-${CND_CONF}.mk connection.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/connection.o connection.cpp

${OBJECTDIR}/condition.o: nbproject/Makefile-${CND_CONF}.mk condition.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/condition.o condition.cpp

${OBJECTDIR}/player.o: nbproject/Makefile-${CND_CONF}.mk player.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/player.o player.cpp

${OBJECTDIR}/fileloader.o: nbproject/Makefile-${CND_CONF}.mk fileloader.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/fileloader.o fileloader.cpp

${OBJECTDIR}/tools.o: nbproject/Makefile-${CND_CONF}.mk tools.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/tools.o tools.cpp

${OBJECTDIR}/actions.o: nbproject/Makefile-${CND_CONF}.mk actions.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/actions.o actions.cpp

${OBJECTDIR}/trashholder.o: nbproject/Makefile-${CND_CONF}.mk trashholder.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/trashholder.o trashholder.cpp

${OBJECTDIR}/quests.o: nbproject/Makefile-${CND_CONF}.mk quests.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/quests.o quests.cpp

${OBJECTDIR}/combat.o: nbproject/Makefile-${CND_CONF}.mk combat.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/combat.o combat.cpp

${OBJECTDIR}/allocator.o: nbproject/Makefile-${CND_CONF}.mk allocator.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/allocator.o allocator.cpp

${OBJECTDIR}/beds.o: nbproject/Makefile-${CND_CONF}.mk beds.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/beds.o beds.cpp

${OBJECTDIR}/weapons.o: nbproject/Makefile-${CND_CONF}.mk weapons.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/weapons.o weapons.cpp

${OBJECTDIR}/items.o: nbproject/Makefile-${CND_CONF}.mk items.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/items.o items.cpp

${OBJECTDIR}/outfit.o: nbproject/Makefile-${CND_CONF}.mk outfit.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/outfit.o outfit.cpp

${OBJECTDIR}/house.o: nbproject/Makefile-${CND_CONF}.mk house.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/house.o house.cpp

${OBJECTDIR}/configmanager.o: nbproject/Makefile-${CND_CONF}.mk configmanager.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/configmanager.o configmanager.cpp

${OBJECTDIR}/logger.o: nbproject/Makefile-${CND_CONF}.mk logger.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/logger.o logger.cpp

${OBJECTDIR}/talkaction.o: nbproject/Makefile-${CND_CONF}.mk talkaction.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/talkaction.o talkaction.cpp

${OBJECTDIR}/databaseodbc.o: nbproject/Makefile-${CND_CONF}.mk databaseodbc.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/databaseodbc.o databaseodbc.cpp

${OBJECTDIR}/exception.o: nbproject/Makefile-${CND_CONF}.mk exception.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/exception.o exception.cpp

${OBJECTDIR}/monster.o: nbproject/Makefile-${CND_CONF}.mk monster.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/monster.o monster.cpp

${OBJECTDIR}/admin.o: nbproject/Makefile-${CND_CONF}.mk admin.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/admin.o admin.cpp

${OBJECTDIR}/monsters.o: nbproject/Makefile-${CND_CONF}.mk monsters.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D__USE_SQLITE__ -D__USE_MYSQL__ -D__DEBUG__ -D__EXCEPTION_TRACER__ -D__ENABLE_SERVER_DIAGNOSTIC__ -MMD -MP -MF $@.d -o ${OBJECTDIR}/monsters.o monsters.cpp

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r build/Debug
	${RM} dist/Debug/MinGW-Windows/trunk.exe

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc

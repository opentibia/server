
CC = g++
CFLAGS = -Wall -D__USE_MYSQL__ -D__USE_SQLITE__
LIBS = -lsqlite3 -lmysqlclient -llua -lboost_thread-mt
OBJS = dbupdate.o configmanager.o database.o databasemysql.o \
	databasesqlite.o

all: dbupdate

clean: 
	rm dbupdate *.o

dbupdate : ${OBJS}
	${CC} ${LIBS} ${OBJS} -o dbupdate

dbupdate.o: dbupdate.cpp
	${CC} -c ${CFLAGS} -o $*.o $*.cpp

configmanager.o: ./../../configmanager.cpp
	${CC} -c ${CFLAGS} -o $*.o ./../../$*.cpp

database.o: ./../../database.cpp
	${CC} -c ${CFLAGS} -o $*.o ./../../$*.cpp

databasemysql.o: ./../../databasemysql.cpp
	${CC} -c ${CFLAGS} -o $*.o ./../../$*.cpp

# databasepgsql.o: ./../../databasepgsl.cpp
#	${CC} -c ${CFLAGS} -o $*.o ./../../$*.cpp

databasesqlite.o: ./../../databasesqlite.cpp
	${CC} -c ${CFLAGS} -o $*.o ./../../$*.cpp

#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.50])
AC_INIT([otserv], [0.6.3.SVN])
AM_INIT_AUTOMAKE([1.10 foreign])
AC_CONFIG_SRCDIR([raids.h])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CXX
AC_LANG(C++)

# check if we want debuging
AC_ARG_ENABLE(debug, [  --enable-debug      enable debuging], 
	[DEBUG_FLAGS="-D__DEBUG__MOVESYS__ -D__DEBUG_HOUSES__ -D__DEBUG_MAILBOX__ -D__DEBUG_LUASCRIPTS__ -D__DEBUG_NET__"]
)
AC_SUBST(DEBUG_FLAGS)

# check if we want to use otserv.lua instead config.lua
AC_ARG_ENABLE(otconfig, [  --enable-otconfig      use otserv.lua configuration], [ OPTIONAL_FLAGS="$OPTIONAL_FLAGS -D__LUA_NAME_ALTER__"])

# check if we want skulls
AC_ARG_ENABLE(skulls, [  --enable-skulls      enable skull system], [OPTIONAL_FLAGS="$OPTIONAL_FLAGS -D__SKULLSYSTEM__"])

# check if we want otserv allocator
AC_ARG_ENABLE(otallocator, [  --enable-otallocator   enable otserv custom allocator], [OPTIONAL_FLAGS="$OPTIONAL_FLAGS -D__OTSERV_ALLOCATOR__"])

AC_SUBST(OPTIONAL_FLAGS)

AM_CONDITIONAL(USE_MYSQL, false)
AM_CONDITIONAL(USE_PGSQL, false)
AM_CONDITIONAL(USE_ODBC, false)
AM_CONDITIONAL(USE_SQLITE, true)
SQLITE_FLAGS=-D__USE_SQLITE__

#check if we want mysql
AC_ARG_ENABLE(mysql, [  --enable-mysql          enable MySQL support], [
	AM_CONDITIONAL(USE_MYSQL, true)
	AM_CONDITIONAL(USE_SQLITE, false)
	MYSQL_FLAGS=-D__USE_MYSQL__
	SQLITE_FLAGS=
	AC_SUBST(MYSQL_FLAGS)
])

#check if we want postgresql
AC_ARG_ENABLE(pgsql, [  --enable-pgsql          enable PostgreSQL support], [
	AM_CONDITIONAL(USE_PGSQL, true)
	AM_CONDITIONAL(USE_SQLITE, false)
	PGSQL_FLAGS=-D__USE_PGSQL__
	SQLITE_FLAGS=
	AC_SUBST(PGSQL_FLAGS)
])

#check if we want odbc
AC_ARG_ENABLE(odbc, [  --enable-odbc           enable ODBC support], [
	AM_CONDITIONAL(USE_ODBC, true)
	AM_CONDITIONAL(USE_SQLITE, false)
	ODBC_FLAGS=-D__USE_ODBC__
	SQLITE_FLAGS=
	AC_SUBST(ODBC_FLAGS)
])

#check if we want sqlite
AC_ARG_ENABLE(sqlite, [  --enable-sqlite         enable SQLite support (default))], [
	AM_CONDITIONAL(USE_SQLITE, true)
	SQLITE_FLAGS=-D__USE_SQLITE__
])

AC_SUBST(SQLITE_FLAGS)

#check if we want the profiler
AC_ARG_ENABLE(profiler, [  --enable-profiler       enable profiler support], [PROFILER_FLAGS=-pg])
AC_SUBST(PROFILER_FLAGS)

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([arpa/inet.h netdb.h netinet/in.h stddef.h stdint.h stdlib.h sys/socket.h sys/timeb.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_SIZE_T
AC_STRUCT_TM
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T
AC_CHECK_TYPES([ptrdiff_t])

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_REALLOC
AC_CHECK_FUNCS([floor ftime gethostbyname gethostname memset pow sqrt strcasecmp strncasecmp strstr strtol])

# check for libxml2
AM_PATH_XML2(2.6.5, , AC_MSG_ERROR([you need libxml2 >= 2.6.5 to compile otserv!]))
LIBS="$LIBS $XML_LIBS"

#check GMP
AC_CHECK_HEADERS([gmp.h], ,[AC_MSG_ERROR("GMP header not found.")])
AC_CHECK_LIB(gmp, __gmpz_init2, ,[AC_MSG_ERROR("Linking against GMP failed.")])

# check for boost
AC_CHECK_LIB(boost_thread-gcc-mt, main, , [
	AC_CHECK_LIB(boost_thread-mt, main, , [
		AC_CHECK_LIB(boost_thread, main, , [
			AC_MSG_ERROR("Linking against boost::thread library failed.")])
	])
])
AC_CHECK_LIB(boost_regex-gcc-mt, main, , [
	AC_CHECK_LIB(boost_regex-mt, main, , [
		AC_CHECK_LIB(boost_regex, main, , [
			AC_MSG_ERROR("Linking against boost::regex library failed.")])
	])
])
AC_CHECK_LIB(boost_system-gcc-mt, main, , [
	AC_CHECK_LIB(boost_system-mt, main, , [
		AC_CHECK_LIB(boost_system, main, , [
			AC_MSG_ERROR("Linking against boost::system library failed.")])
	])
])
AC_CHECK_LIB(boost_date_time-gcc-mt, main, , [
	AC_CHECK_LIB(boost_date_time-mt, main, , [
		AC_CHECK_LIB(boost_date_time, main, , [
			AC_MSG_ERROR("Linking against boost::date-time library failed.")])
	])
])

#check boost asio
AC_CHECK_HEADERS([boost/asio.hpp], ,[AC_MSG_ERROR("boost::asio header not found.")])

#check lua 5.1
PKG_CHECK_MODULES(LUA, lua5.1 >= 5.1, , [
	AC_CHECK_HEADERS([lua.h],[],[AC_MSG_ERROR("lua header not found.")])
	AC_CHECK_LIB(lua, main, , [AC_MSG_ERROR("Linking against lua failed.")])
])
AC_SUBST(LUA_CFLAGS)
AC_SUBST(LUA_LIBS)

# check for mysql if it is enabled
if test -n "$MYSQL_FLAGS"; then
	AC_CHECK_HEADERS([mysql/mysql.h],[MYSQL_LIBS=-lmysqlclient],[AC_MSG_ERROR("mysql headers missing.")])
	# It would be nice if this function didn't cache its value
	# If we could clear the cache value of this
	# function, or not cache it at all, we wouldn't need to
	# re-run the configuration.ac after we've copied the libs
	AC_CHECK_LIB(mysqlclient, main,[],[
		# 64-bit libs are in a different dir
		if test -d /usr/lib64/mysql; then
			echo "NOTICE -- Copying mysql-client files into the /usr/lib64 directory."
			echo "NOTICE -- If mysql-client is not found, please re-run the configure script."
			cp -R -s -f /usr/lib64/mysql/libmysqlclient* /usr/lib64
			AC_CHECK_LIB(mysqlclient, main,[], [AC_MSG_ERROR("Linking against mysql-client failed.")])
		elif test -d /usr/lib/mysql; then
			echo "NOTICE -- Copying mysql-client files into the /usr/lib directory."
			echo "NOTICE -- If mysql-client is not found, please re-run the configure script."
			cp -R -s -f /usr/lib/mysql/libmysqlclient* /usr/lib
			AC_CHECK_LIB(mysqlclient, main,[], [AC_MSG_ERROR("Linking against mysql-client failed.")])
		else
			AC_MSG_ERROR("Linking against mysql-client failed.")
		fi
	])
	AC_SUBST(MYSQL_LIBS)
fi

# check for postgresql if it is enabled
if test -n "$PGSQL_FLAGS"; then 
	AC_CHECK_HEADERS([postgresql/libpq-fe.h],[/usr/include/libpq-fe.h],[PGSQL_LIBS=-lpg],[AC_MSG_ERROR("postgresql headers missing.")])
	AC_CHECK_LIB(pq, main,[],[AC_MSG_ERROR("Linking against libpq failed.")])
	AC_SUBST(PGSQL_LIBS)
fi

# check for odbc if it is enabled
if test -n "$ODBC_FLAGS"; then
	AC_CHECK_HEADERS([sql.h sqlext.h sqltypes.h],[ODBC_LIBS=-lodbc],[AC_MSG_ERROR("unixodbc headers missing.")])
	AC_CHECK_LIB(odbc, main, [], [AC_MSG_ERROR("Linking against odbc failed.")])
	AC_SUBST(ODBC_LIBS)
fi

# check for sqlite if it is enabled
if test -n "$SQLITE_FLAGS"; then 
	AC_CHECK_HEADERS([sqlite3.h],[SQLITE_LIBS=-lsqlite3],[AC_MSG_ERROR("sqlite3 headers missing.")])
	AC_CHECK_LIB(sqlite3, main,[],[AC_MSG_ERROR("Linking against sqlite3 failed.")])
	AC_SUBST(SQLITE_LIBS)
fi

AC_CONFIG_FILES([Makefile data/Makefile])
AC_OUTPUT

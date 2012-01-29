
-- Visual Studio libraries directories
local vs_libdirs = 
	{
		"$(BOOST_LIB_PATH)",
		"$(GMP_LIB_PATH)",
		"$(LIBXML_LIB_PATH)",
		"$(MYSQL_LIB_PATH)",
		"$(SQLITE_LIB_PATH)",
		"$(PSQL_LIB_PATH)",
		"$(LUA_LIB_PATH)"
	}
	
-- Visual Studio includes directories
local vs_includedirs =
	{
		"$(BOOST_INCLUDE_PATH)",
		"$(GMP_INCLUDE_PATH)",
		"$(LIBXML_INCLUDE_PATH)",
		"$(MYSQL_INCLUDE_PATH)",
		"$(SQLITE_INCLUDE_PATH)",
		"$(PSQL_INCLUDE_PATH)",
		"$(LUA_INCLUDE_PATH)"
	}
	
-- Visual Studio preprocessor definitions
local vs_defines =
	{
		"_WIN32_WINNT=0x0501",
		"_CRT_SECURE_NO_DEPRECATE",
		"_CRT_SECURE_NO_WARNINGS",
		"_CRT_NON_CONFORMING_SWPRINTFS",
		"__OLD_GUILD_SYSTEM__",
		"NOMINMAX"
	}
	
-- Build options
local options =
	{
		"mysql",
		"sqlite",
		"postgre",
		"odbc"
	}

solution "otserv"
	configurations { "Release", "Debug" }
	
project "otserv"
	targetname "otserv"
	language "C++"
	kind "ConsoleApp"
	location "build"
	files
	{
		"*.cpp",
		"*.h"
	}
	
	configuration "Release"
		targetdir "build/release"
		flags
		{
			"OptimizeSpeed",
			--"NoFramePointer",
			-- SSE2 is present in any AMD/INTEL CPU since 2004. It
			-- won't hurt anyone if enabled.
			"EnableSSE2"
		}
		
	configuration "Debug"
		targetdir "build/debug"
		flags { "Symbols" }
	
	-- Configuration for Visual Studio projects
	configuration { "vs* or windows" }
		defines { vs_defines }
		includedirs { vs_includedirs }
		libdirs { vs_libdirs }
	
	-- Configurations built on top of GCC/G++
	configuration { "linux or codelite or codeblocks or gmake" }
		buildoptions
		{
			"-std=c++0x",
			"-pedantic",
			"-W",
			"-Wall",
			"-Wextra",
			"-Wno-unused-parameter"			
		}
		links
		{
			"lua5.1",
			"xml2",
			"gmp",
			"sqlite3",
			"mysqlclient",
			"boost_system",
			"boost_thread",
			"boost_regex",
			"pthread"
		}
	
	-- Mac OS X support for Xcode.
	configuration { "macosx" }
		links
		{
			"lua",
			"xml2",
			"mysqlclient",
			"boost_thread-mt",
			"boost_regex-mt",
			"boost_system"
		}
		
	-- Get rid of the build folder along with the generated files
	if _ACTION == "clean" then
		os.rmdir("build")
	end


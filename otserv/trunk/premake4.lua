
solution "Otserv"
	configurations { "Release", "Debug" }
	
project "Otserv"
	targetname "otserv"
	language "C++"
	kind "ConsoleApp"
	
	files { "*.cpp", "*.h" }
	
	configuration "Release"
		targetdir "build/release"
		defines "NDEBUG"
		flags { "Optmize" }
		
	configuration "Debug"
		targedit "build/debug"
		defines "__DEBUG__"
		flags { "Symbols" }
		
	configuration "vs*
		defines 
		{
			"_CRT_SECURE_NO_DEPRECATE",
			"_CRT_SECURE_NO_WARNINGS",
			"_CRT_NON_CONFORMING_SWPRINTFS",
			"NOMINMAX"
		}
		
	configuration { "linux", "gmake" }
		links
		{
			"lua5.1",
			"xml2",
			"gmp",
			"sqlite3",
			"mysqlclient",
			"boost_system",
			"boost_thread",
			"boost_regex"
		}


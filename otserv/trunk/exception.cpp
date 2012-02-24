//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Exception Handler class
//////////////////////////////////////////////////////////////////////
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//////////////////////////////////////////////////////////////////////

#include "otpch.h"

#include <boost/thread.hpp>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <ctime>
#include <cstdlib>
#include <map>
#include "exception.h"
#include "configmanager.h"

extern ConfigManager g_config;

#ifdef __WINDOWS__
	int ExceptionHandler::ref_counter = 0;

	
#endif

ExceptionHandler::ExceptionHandler()
{
	isInstalled = false;
}

ExceptionHandler::~ExceptionHandler()
{
	if(isInstalled){
		RemoveHandler();
	}
}

bool ExceptionHandler::InstallHandler()
{

#ifdef __WINDOWS__
	++ref_counter;
	if(ref_counter == 1){
		SetUnhandledExceptionFilter(ExceptionHandler::MiniDumpExceptionHandler);
	}
#endif

	isInstalled = true;
	return true;
}

bool ExceptionHandler::RemoveHandler()
{
	if(!isInstalled){
		return false;
	}

#ifdef __WINDOWS__
	--ref_counter;
	if(ref_counter == 0){
		SetUnhandledExceptionFilter(NULL);
	}
#endif

	isInstalled = false;
	return true;
}

#ifdef __WINDOWS__

long ExceptionHandler::MiniDumpExceptionHandler(EXCEPTION_POINTERS* exceptionPointers /*= NULL*/)
{
	// Alerts the user about what is happening
	std::cout << "Unhandled exception. Generating minidump..." << std::endl;

	// Get system time
	SYSTEMTIME systemTime;
	GetSystemTime(&systemTime);

	// Format file name
	// "otserv_DD-MM-YYYY_HH-MM-SS.mdmp"
	char fileName[64] = {"\0"};
	sprintf(fileName, "otserv_%02u-%02u-%04u_%02u-%02u-%02u.mdmp",
		systemTime.wDay, systemTime.wMonth, systemTime.wYear,
		systemTime.wHour, systemTime.wMinute, systemTime.wSecond);

	// Create the dump file
	HANDLE hFile = CreateFileA(fileName, GENERIC_READ|GENERIC_WRITE,
		FILE_SHARE_DELETE|FILE_SHARE_READ|FILE_SHARE_WRITE,
		NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	// If we cannot create the file, then we cannot dump the memory
	if(!hFile || hFile == INVALID_HANDLE_VALUE){
		std::cout << "Cannot create dump file. Error: " << GetLastError() << std::endl;
		return EXCEPTION_CONTINUE_SEARCH;
	}

	// Collect the exception information
	MINIDUMP_EXCEPTION_INFORMATION exceptionInformation;
	exceptionInformation.ClientPointers = FALSE;
	exceptionInformation.ExceptionPointers = exceptionPointers;
	exceptionInformation.ThreadId = GetCurrentThreadId();

	// Get the process and it's Id
	HANDLE hProcess = GetCurrentProcess();
	DWORD ProcessId = GetProcessId(hProcess);

	// Dump flags
	MINIDUMP_TYPE flags = (MINIDUMP_TYPE)(MiniDumpNormal);

	// Write dump to file
	BOOL dumpResult = MiniDumpWriteDump(hProcess, ProcessId, hFile, flags,
		&exceptionInformation, NULL, NULL);

	// Delete the dump file if we cannot generate the crash trace
	if(!dumpResult){
		std::cout << "Cannot generate minidump. Error: " << GetLastError() << std::endl;

		//Close file and delete it
		CloseHandle(hFile);
		DeleteFileA(fileName);

		return EXCEPTION_CONTINUE_SEARCH;
	}

	// Close dump file
	CloseHandle(hFile);

	return EXCEPTION_EXECUTE_HANDLER;
}
#endif


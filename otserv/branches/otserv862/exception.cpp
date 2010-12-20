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
#else //Unix/Linux
	#include <execinfo.h>
	#include <signal.h>
	#include <ucontext.h>
	#include <sys/time.h>
	#include <sys/resource.h> /* POSIX.1-2001 */

	extern time_t start_time;
	void _SigHandler(int signum, siginfo_t *info, void* secret);
	#ifndef COMPILER_STRING
		#define COMPILER_STRING ""
	#endif

	#define COMPILATION_DATE  __DATE__ " " __TIME__
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
 //Unix/Linux
#else
	struct sigaction sa;
	sa.sa_sigaction = &_SigHandler;
	sigemptyset (&sa.sa_mask);
	sa.sa_flags = SA_RESTART | SA_SIGINFO;

	sigaction(SIGILL, &sa, NULL);		// illegal instruction
	sigaction(SIGSEGV, &sa, NULL);	// segmentation fault
	sigaction(SIGFPE, &sa, NULL);		// floating-point exception
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
//Unix/Linux
#else
	signal(SIGILL, SIG_DFL);	// illegal instruction
	signal(SIGSEGV, SIG_DFL);	// segmentation fault
	signal(SIGFPE, SIG_DFL);	// floating-point exception
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

//Unix/Linux
#else
#define BACKTRACE_DEPTH 128
void _SigHandler(int signum, siginfo_t *info, void* secret)
{
	bool file;

	int addrs;
	void* buffer[BACKTRACE_DEPTH];
	char** symbols;

	ucontext_t context = *(ucontext_t*)secret;
	rusage resources;
	rlimit resourcelimit;
	greg_t esp = 0;
	tm *ts;
	char date_buff[80];

	std::ostream *outdriver;
	std::cout << "Error: generating report file..." <<std::endl;
	std::ofstream output("report.txt",std::ios_base::app);
	if(output.fail()){
		outdriver = &std::cout;
		file = false;
	}
	else{
		file = true;
		outdriver = &output;
	}

	time_t rawtime;
	time(&rawtime);
	*outdriver << "*****************************************************" << std::endl;
	*outdriver << "Error report - " << std::ctime(&rawtime) << std::endl;
	*outdriver << "Compiler info - " << COMPILER_STRING << std::endl;
	*outdriver << "Compilation Date - " << COMPILATION_DATE << std::endl << std::endl;

	if(getrusage(RUSAGE_SELF, &resources) != -1)
	{
		//- global memory information
		if(getrlimit(RLIMIT_AS, &resourcelimit) != -1)
		{
			// note: This is not POSIX standard, but it is available in Unix System V release 4, Linux, and 4.3 BSD
			long memusage = resources.ru_ixrss + resources.ru_idrss + resources.ru_isrss;
			long memtotal = resourcelimit.rlim_max;
			long memavail = memtotal - memusage;
			long memload = long(float(memusage / memtotal) * 100.f);
			*outdriver << "Memory load: " << memload << "K " << std::endl
				<< "Total memory: " << memtotal << "K "
				<< "available: " << memavail << "K" << std::endl;
		}
		//-process info
		// creation time
		ts = localtime(&start_time);
		strftime(date_buff, 80, "%d-%m-%Y %H:%M:%S", ts);
		// kernel time
		*outdriver << "Kernel time: " << (resources.ru_stime.tv_sec / 3600)
			<< ":" << ((resources.ru_stime.tv_sec % 3600) / 60)
			<< ":" << ((resources.ru_stime.tv_sec % 3600) % 60)
			<< "." << (resources.ru_stime.tv_usec / 1000)
			<< std::endl;
		// user time
		*outdriver << "User time: " << (resources.ru_utime.tv_sec / 3600)
			<< ":" << ((resources.ru_utime.tv_sec % 3600) / 60)
			<< ":" << ((resources.ru_utime.tv_sec % 3600) % 60)
			<< "." << (resources.ru_utime.tv_usec / 1000)
			<< std::endl;
	}
	// TODO: Process thread count (is it really needed anymore?)
	*outdriver << std::endl;


	outdriver->flags(std::ios::hex | std::ios::showbase);
	*outdriver << "Signal: " << signum;

	{
	#if __WORDSIZE == 32
		*outdriver << " at eip = " << context.uc_mcontext.gregs[REG_EIP] << std::endl;
		*outdriver << "eax = " << context.uc_mcontext.gregs[REG_EAX] << std::endl;
		*outdriver << "ebx = " << context.uc_mcontext.gregs[REG_EBX] << std::endl;
		*outdriver << "ecx = " << context.uc_mcontext.gregs[REG_ECX] << std::endl;
		*outdriver << "edx = " << context.uc_mcontext.gregs[REG_EDX] << std::endl;
		*outdriver << "esi = " << context.uc_mcontext.gregs[REG_ESI] << std::endl;
		*outdriver << "edi = " << context.uc_mcontext.gregs[REG_EDI] << std::endl;
		*outdriver << "ebp = " << context.uc_mcontext.gregs[REG_EBP] << std::endl;
		*outdriver << "esp = " << context.uc_mcontext.gregs[REG_ESP] << std::endl;
		*outdriver << "efl = " << context.uc_mcontext.gregs[REG_EFL] << std::endl;
		esp = context.uc_mcontext.gregs[REG_ESP];
	#else // 64-bit
		*outdriver << " at rip = " << context.uc_mcontext.gregs[REG_RIP] << std::endl;
		*outdriver << "rax = " << context.uc_mcontext.gregs[REG_RAX] << std::endl;
		*outdriver << "rbx = " << context.uc_mcontext.gregs[REG_RBX] << std::endl;
		*outdriver << "rcx = " << context.uc_mcontext.gregs[REG_RCX] << std::endl;
		*outdriver << "rdx = " << context.uc_mcontext.gregs[REG_RDX] << std::endl;
		*outdriver << "rsi = " << context.uc_mcontext.gregs[REG_RSI] << std::endl;
		*outdriver << "rdi = " << context.uc_mcontext.gregs[REG_RDI] << std::endl;
		*outdriver << "rbp = " << context.uc_mcontext.gregs[REG_RBP] << std::endl;
		*outdriver << "rsp = " << context.uc_mcontext.gregs[REG_RSP] << std::endl;
		*outdriver << "efl = " << context.uc_mcontext.gregs[REG_EFL] << std::endl;
		esp = context.uc_mcontext.gregs[REG_RSP];
	#endif
	}
	outdriver->flush();
	*outdriver << std::endl;

	// stack backtrace
	addrs = backtrace(buffer, BACKTRACE_DEPTH);
	symbols = backtrace_symbols(buffer, addrs);
	if(symbols != NULL && addrs != 0) {
		*outdriver << "---Stack Trace---" << std::endl;
		if(esp != 0) {
			*outdriver << "From: " << (unsigned long)esp <<
				" to: " << (unsigned long)(esp+addrs) << std::endl;
		}
		for(int i = 0; i != addrs; ++i)
		{
			*outdriver << symbols[i] << std::endl;
		}
	}
	outdriver->flush();

	if(file) {
		((std::ofstream*)outdriver)->close();
	}

	_exit(1);
}

#endif
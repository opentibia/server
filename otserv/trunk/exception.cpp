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

#if defined __EXCEPTION_TRACER__

#include <iostream>
#include <fstream>
#include <iomanip>
#include <ctime>
#include <stdlib.h>
#include <map>

#include <boost/thread.hpp>
#include "exception.h"
#include "configmanager.h"

extern ConfigManager g_config;

#if defined WIN32 || defined __WINDOWS__

	#if defined _MSC_VER || defined __USE_MINIDUMP__
		#include "dbghelp.h"
		
		// based on dbghelp.h
		typedef BOOL (WINAPI *MINIDUMPWRITEDUMP)(HANDLE hProcess, DWORD dwPid, HANDLE hFile, MINIDUMP_TYPE DumpType,
			CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
			CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
			CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam);

		int ExceptionHandler::ref_counter = 0;

	#elif __GNUC__
		#include <excpt.h>
		#include <tlhelp32.h>

		unsigned long max_off;
		unsigned long min_off;
		FunctionMap functionMap;
		bool ExceptionHandler::isMapLoaded = false;
		boost::recursive_mutex maploadlock;
		typedef std::map<unsigned long, char*> FunctionMap;

		EXCEPTION_DISPOSITION
		__cdecl _SEHHandler(
			struct _EXCEPTION_RECORD *ExceptionRecord,
			void * EstablisherFrame,
			struct _CONTEXT *ContextRecord,
			void * DispatcherContext
		);
		void printPointer(std::ostream* output,unsigned long p);
	#endif

#else //Unix/Linux
	#include <execinfo.h>
	#include <signal.h>
	#include <ucontext.h>

	#include <sys/time.h>
	#include <sys/resource.h> /* POSIX.1-2001 */

	extern time_t start_time;
	void _SigHandler(int signum, siginfo_t *info, void* secret);
#endif

#ifndef COMPILER_STRING
#ifdef __GNUC__
#define COMPILER_STRING  "gcc " __VERSION__
#else
#define COMPILER_STRING  ""
#endif
#endif

#define COMPILATION_DATE  __DATE__ " " __TIME__

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
#if defined WIN32 || defined __WINDOWS__
	#if defined _MSC_VER || defined __USE_MINIDUMP__

		++ref_counter;
		if(ref_counter == 1){
			::SetUnhandledExceptionFilter(ExceptionHandler::MiniDumpExceptionHandler);
		}

	#elif __GNUC__
		boost::recursive_mutex::scoped_lock lockObj(maploadlock);
		if(maploaded == false)
			LoadMap();
		if(isInstalled == true)
			return false;

		SEHChain *prevSEH;
		__asm__ ("movl %%fs:0,%%eax;movl %%eax,%0;":"=r"(prevSEH)::"%eax" );
		chain.prev = prevSEH;
		chain.SEHfunction = (void*)&_SEHHandler;
		__asm__("movl %0,%%eax;movl %%eax,%%fs:0;": : "g" (&chain):"%eax");
	#endif

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

#if defined WIN32 || defined __WINDOWS__

	#if defined _MSC_VER || defined __USE_MINIDUMP__

		--ref_counter;
		if(ref_counter == 0){
			::SetUnhandledExceptionFilter(NULL);
		}

	#elif __GNUC__

		/*
		mov eax,[chain.prev]
		mov fs:[0],eax
		*/
		__asm__ ("movl %0,%%eax;movl %%eax,%%fs:0;"::"r"(chain.prev):"%eax" );

	#endif

//Unix/Linux
#else
	signal(SIGILL, SIG_DFL);	// illegal instruction
	signal(SIGSEGV, SIG_DFL);	// segmentation fault
	signal(SIGFPE, SIG_DFL);	// floating-point exception
#endif

	isInstalled = false;
	return true;
}

#if defined WIN32 || defined __WINDOWS__
	#if defined _MSC_VER || defined __USE_MINIDUMP__

		long ExceptionHandler::MiniDumpExceptionHandler(struct _EXCEPTION_POINTERS *pExceptionInfo)
		{
			HMODULE hDll = NULL;
			char szAppPath[_MAX_PATH];
			std::string strAppDirectory;

			GetModuleFileName(NULL, szAppPath, _MAX_PATH);
			strAppDirectory = szAppPath;
			strAppDirectory = strAppDirectory.substr(0, strAppDirectory.rfind("\\"));
			if(strAppDirectory.rfind('\\') != strAppDirectory.size()){
				strAppDirectory += '\\';
			}

			std::string strFileNameDbgHelp = strAppDirectory + "DBGHELP.DLL";
			hDll = ::LoadLibrary(strFileNameDbgHelp.c_str());

			if(!hDll){
				// load any version we can
				hDll = ::LoadLibrary("DBGHELP.DLL");
			}

			if(!hDll){
				std::cout << "Could not generate report - DBGHELP.DLL could not be found." << std::endl;
				return EXCEPTION_CONTINUE_SEARCH;
			}

			MINIDUMPWRITEDUMP pDump = (MINIDUMPWRITEDUMP)::GetProcAddress( hDll, "MiniDumpWriteDump");
			if(!pDump){
				std::cout << "Could not generate report - DBGHELP.DLL is to old." << std::endl;
				return EXCEPTION_CONTINUE_SEARCH;
			}

			SYSTEMTIME stLocalTime;
			GetLocalTime(&stLocalTime);

			char dumpfile[250] = {'\0'};
			sprintf(dumpfile, "%04d-%02d-%02d_%02d%02d%02d.dmp", 
				stLocalTime.wYear, stLocalTime.wMonth, stLocalTime.wDay,
				stLocalTime.wHour, stLocalTime.wMinute, stLocalTime.wSecond);

			std::string strFileNameDump = strAppDirectory + dumpfile;

			HANDLE hFile = ::CreateFile(strFileNameDump.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS,
										FILE_ATTRIBUTE_NORMAL, NULL );

			if(hFile == INVALID_HANDLE_VALUE){
				std::cout << "Could not create memory dump file." << std::endl;
				return EXCEPTION_EXECUTE_HANDLER;
			}

			_MINIDUMP_EXCEPTION_INFORMATION ExInfo;

			ExInfo.ThreadId = ::GetCurrentThreadId();
			ExInfo.ExceptionPointers = pExceptionInfo;
			ExInfo.ClientPointers = NULL;

			std::cout << "Generating minidump file... " << dumpfile << std::endl;

			BOOL bOK = pDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &ExInfo, NULL, NULL );
			if(!bOK){
				std::cout << "Could not dump memory to file." << std::endl;
				::CloseHandle(hFile);
				return EXCEPTION_CONTINUE_SEARCH;
			}

			::CloseHandle(hFile);
			return EXCEPTION_EXECUTE_HANDLER;
		}

	#elif __GNUC__

		char* getFunctionName(unsigned long addr, unsigned long& start)
		{
			FunctionMap::iterator functions;
			if(addr >= min_off && addr <= max_off){
				for(functions = functionMap.begin(); functions != functionMap.end(); ++functions) {
					if(functions->first > addr && functions != functionMap.begin()){
						functions--;
						start = functions->first;
						return functions->second;
						break;
					}
				}
			}
			return NULL;
		}

		EXCEPTION_DISPOSITION
		__cdecl _SEHHandler(
			struct _EXCEPTION_RECORD *ExceptionRecord,
			void * EstablisherFrame,
			struct _CONTEXT *ContextRecord,
			void * DispatcherContext
			){
			//
			unsigned long *esp;
			unsigned long *next_ret;
			unsigned long stack_val;
			unsigned long *stacklimit;
			unsigned long *stackstart;
			unsigned long nparameters = 0;
			unsigned long file,foundRetAddress = 0;
			_MEMORY_BASIC_INFORMATION mbi;

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

			//system and process info
			//- global memory information
			MEMORYSTATUSEX mstate;
			mstate.dwLength = sizeof(mstate);
			if(GlobalMemoryStatusEx(&mstate)){
				*outdriver << "Memory load: " << mstate.dwMemoryLoad << std::endl <<
					"Total phys: " << mstate.ullTotalPhys/1024 << " K available phys: " <<
					mstate.ullAvailPhys/1024 << " K" << std::endl;
			}
			else{
				*outdriver << "Memory load: Error" << std::endl;
			}
			//-process info
			FILETIME FTcreation,FTexit,FTkernel,FTuser;
			SYSTEMTIME systemtime;
			GetProcessTimes(GetCurrentProcess(),&FTcreation,&FTexit,&FTkernel,&FTuser);
			// creation time
			FileTimeToSystemTime(&FTcreation,&systemtime);
			*outdriver << "Start time: " << systemtime.wDay << "-" <<
				systemtime.wMonth << "-" << systemtime.wYear << "  " <<
				systemtime.wHour << ":" << systemtime.wMinute << ":" <<
				systemtime.wSecond << std::endl;
			// kernel time
			unsigned long miliseconds;
			miliseconds = FTkernel.dwHighDateTime * 429497 + FTkernel.dwLowDateTime/10000;
			*outdriver << "Kernel time: " << miliseconds/3600000;
			miliseconds = miliseconds - (miliseconds/3600000)*3600000;
			*outdriver << ":" << miliseconds/60000;
			miliseconds = miliseconds - (miliseconds/60000)*60000;
			*outdriver << ":" << miliseconds/1000;
			miliseconds = miliseconds - (miliseconds/1000)*1000;
			*outdriver << "." << miliseconds << std::endl;
			// user time
			miliseconds = FTuser.dwHighDateTime * 429497 + FTuser.dwLowDateTime/10000;
			*outdriver << "User time: " << miliseconds/3600000;
			miliseconds = miliseconds - (miliseconds/3600000)*3600000;
			*outdriver << ":" << miliseconds/60000;
			miliseconds = miliseconds - (miliseconds/60000)*60000;
			*outdriver << ":" << miliseconds/1000;
			miliseconds = miliseconds - (miliseconds/1000)*1000;
			*outdriver << "." << miliseconds << std::endl;


			// n threads
			PROCESSENTRY32 uProcess;
			HANDLE lSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
			BOOL r;
			if(lSnapShot != 0)
			{
				uProcess.dwSize = sizeof(uProcess);
				r = Process32First(lSnapShot, &uProcess);
				while(r)
				{
					if(uProcess.th32ProcessID == GetCurrentProcessId()){
						*outdriver << "Threads: " << uProcess.cntThreads << std::endl;
						break;
					}
					r = Process32Next(lSnapShot, &uProcess);
				}
				CloseHandle (lSnapShot);
			}

			*outdriver << std::endl;
			//exception header type and eip
			outdriver->flags(std::ios::hex | std::ios::showbase);
			*outdriver << "Exception: " << (unsigned long)ExceptionRecord->ExceptionCode <<
				" at eip = " << (unsigned long)ExceptionRecord->ExceptionAddress;
			FunctionMap::iterator functions;
			unsigned long functionAddr;
			char* functionName = getFunctionName((unsigned long)ExceptionRecord->ExceptionAddress, functionAddr);
			if(functionName){
				*outdriver << "(" << functionName << " - " << functionAddr <<")";
			}
			*outdriver << std::endl ;

			//registers
			*outdriver << "eax = ";printPointer(outdriver,ContextRecord->Eax);*outdriver << std::endl;
			*outdriver << "ebx = ";printPointer(outdriver,ContextRecord->Ebx);*outdriver << std::endl;
			*outdriver << "ecx = ";printPointer(outdriver,ContextRecord->Ecx);*outdriver << std::endl;
			*outdriver << "edx = ";printPointer(outdriver,ContextRecord->Edx);*outdriver << std::endl;
			*outdriver << "esi = ";printPointer(outdriver,ContextRecord->Esi);*outdriver << std::endl;
			*outdriver << "edi = ";printPointer(outdriver,ContextRecord->Edi);*outdriver << std::endl;
			*outdriver << "ebp = ";printPointer(outdriver,ContextRecord->Ebp);*outdriver << std::endl;
			*outdriver << "esp = ";printPointer(outdriver,ContextRecord->Esp);*outdriver << std::endl;
			*outdriver << "efl = " << ContextRecord->EFlags << std::endl;
			*outdriver << std::endl;

			//stack dump
			esp = (unsigned long *)(ContextRecord->Esp);
			VirtualQuery(esp, &mbi, sizeof(mbi));
			stacklimit = (unsigned long*)((unsigned long)(mbi.BaseAddress) + mbi.RegionSize);

			*outdriver << "---Stack Trace---" << std::endl;
			*outdriver << "From: " << (unsigned long)esp <<
				" to: " << (unsigned long)stacklimit << std::endl;

			stackstart = esp;
			next_ret = (unsigned long*)(ContextRecord->Ebp);
			unsigned long frame_param_counter;
			frame_param_counter = 0;
			while(esp < stacklimit){
				stack_val = *esp;
				if(foundRetAddress)
					nparameters++;

				if(esp - stackstart < 20 || nparameters < 10 || std::abs(esp - next_ret) < 10 || frame_param_counter < 8){
					*outdriver  << (unsigned long)esp << " | ";
					printPointer(outdriver,stack_val);
					if(esp == next_ret){
						*outdriver << " \\\\\\\\\\\\ stack frame //////";
					}
					else if(esp - next_ret == 1){
						*outdriver << " <-- ret" ;
					}
					else if(esp - next_ret == 2){
						next_ret = (unsigned long*)*(esp - 2);
						frame_param_counter = 0;
					}
					frame_param_counter++;
					*outdriver<< std::endl;
				}
				if(stack_val >= min_off && stack_val <= max_off){
					foundRetAddress++;
					//
					unsigned long functionAddr;
					char* functionName = getFunctionName(stack_val, functionAddr);
					output << (unsigned long)esp << "  " << functionName << "(" <<
						functionAddr << ")" << std::endl;
				}
				esp++;
			}
			*outdriver << "*****************************************************" << std::endl;
			if(file)
				((std::ofstream*)outdriver)->close();
			if(g_config.getNumber(ConfigManager::SHOW_CRASH_WINDOW))
				MessageBoxA(NULL,"Please send the file report.txt to support service. Thanks","Error",MB_OK |MB_ICONERROR);
			std::cout << "Error report generated. Killing server." <<std::endl;
			exit(1); //force exit
			return ExceptionContinueSearch;
		}

		void printPointer(std::ostream* output,unsigned long p)
		{
			*output << p;
			if(IsBadReadPtr((void*)p,4) == 0){
				*output << " -> " << *(unsigned long*)p;
			}
		}

		bool ExceptionHandler::LoadMap()
		{
			if(maploaded == true){
				return false;
			}

			functionMap.clear();
			isInstalled = false;
			//load map file if exists
			char line[1024];
			FILE* input = fopen("otserv.map", "r");
			min_off = 0xFFFFFF;
			max_off = 0;
			long n = 0;
			if(!input){
				std::cout << "Failed loading symbols. otserv.map not found. " << std::endl;
				std::cout << "Go to http://otfans.net/showthread.php?t=4718 for more info." << std::endl;
				system("pause");
				exit(1);
				return false;
			}

			//read until found .text           0x00401000
			while(fgets(line, 1024, input)){
				if(memcmp(line,".text",5) == 0)
					break;
			}

			if(feof(input)){
				return false;
			}

			char tofind[] = "0x";
			char lib[] = ".a(";
			while(fgets(line, 1024, input)){
				char* pos = strstr(line, lib);
				if(pos)
					break; //not load libs
				pos = strstr(line, tofind);
				if(pos){
					//read hex offset
					char hexnumber[12];
					strncpy(hexnumber, pos, 10);
					hexnumber[10] = 0;
					char* pEnd;
					unsigned long offset = strtol(hexnumber, &pEnd, 0);
					if(offset){
						//read function name
						char* pos2 = pos + 12;
						while(*pos2 != 0){
							if(*pos2 != ' ')
								break;
							pos2++;
						}
						if(*pos2 == 0 || (*pos2 == '0' && *(pos2+1) == 'x'))
							continue;

						char* name = new char[strlen(pos2)+1];
						strcpy(name, pos2);
						name[strlen(pos2) - 1] = 0;
						functionMap[offset] = name;
						if(offset > max_off)
							max_off = offset;
						if(offset < min_off)
							min_off = offset;
						n++;
					}
				}
			}
			// close file
			fclose(input);
			//std::cout << "Loaded " << n << " stack symbols" <<std::endl;
			maploaded = true;
			return true;
		}

		void ExceptionHandler::dumpStack()
		{
			unsigned long *esp;
			unsigned long *next_ret;
			unsigned long stack_val;
			unsigned long *stacklimit;
			unsigned long *stackstart;
			unsigned long nparameters = 0;
			unsigned long foundRetAddress = 0;
			_MEMORY_BASIC_INFORMATION mbi;

			std::cout << "Error: generating report file..." << std::endl;
			std::ofstream output("report.txt",std::ios_base::app);
			output.flags(std::ios::hex | std::ios::showbase);
			time_t rawtime;
			time(&rawtime);
			output << "*****************************************************" << std::endl;
			output << "Stack dump - " << std::ctime(&rawtime) << std::endl;
			output << "Compiler info - " << COMPILER_STRING << std::endl;
			output << "Compilation Date - " << COMPILATION_DATE << std::endl << std::endl;

			__asm__ ("movl %%esp, %0;":"=r"(esp)::);

			VirtualQuery(esp, &mbi, sizeof(mbi));
			stacklimit = (unsigned long*)((unsigned long)(mbi.BaseAddress) + mbi.RegionSize);

			output << "---Stack Trace---" << std::endl;
			output << "From: " << (unsigned long)esp <<
				" to: " << (unsigned long)stacklimit << std::endl;

			stackstart = esp;
			__asm__ ("movl %%ebp, %0;":"=r"(next_ret)::);

			unsigned long frame_param_counter;
			frame_param_counter = 0;
			while(esp < stacklimit){
				stack_val = *esp;
				if(foundRetAddress)
					nparameters++;

				if(esp - stackstart < 20 || nparameters < 10 || std::abs(esp - next_ret) < 10 || frame_param_counter < 8){
					output  << (unsigned long)esp << " | ";
					printPointer(&output, stack_val);
					if(esp == next_ret){
						output << " \\\\\\\\\\\\ stack frame //////";
					}
					else if(esp - next_ret == 1){
						output << " <-- ret" ;
					}
					else if(esp - next_ret == 2){
						next_ret = (unsigned long*)*(esp - 2);
						frame_param_counter = 0;
					}
					frame_param_counter++;
					output << std::endl;
				}
				if(stack_val >= min_off && stack_val <= max_off){
					foundRetAddress++;
					unsigned long functionAddr;
					char* functionName = getFunctionName(stack_val, functionAddr);
					output << (unsigned long)esp << "  " << functionName << "(" <<
						functionAddr << ")" << std::endl;
				}
				esp++;
			}
			output << "*****************************************************" << std::endl;
			output.close();
		}
	#endif

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

void ExceptionHandler::dumpStack()
{
	return;
}
#endif

#endif

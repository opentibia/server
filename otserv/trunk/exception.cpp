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

#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
#include <ctime>
#include <stdlib.h> 
#include <map>

#include "otsystem.h"
#include "exception.h"

#if defined WIN32 || defined __WINDOWS__
#include "excpt.h"
#include "tlhelp32.h"
#endif

unsigned long max_off;
unsigned long min_off;
FunctionMap functionMap;
bool maploaded = false;
OTSYS_THREAD_LOCKVAR maploadlock;

#if defined WIN32 || defined __WINDOWS__
EXCEPTION_DISPOSITION
 __cdecl _SEHHandler(
     struct _EXCEPTION_RECORD *ExceptionRecord,
     void * EstablisherFrame,
     struct _CONTEXT *ContextRecord,
     void * DispatcherContext
     );
void printPointer(std::ostream* output,unsigned long p);
#endif

#ifdef __GNUC__
#define COMPILER_STRING  "gcc " __VERSION__
#else
#define COMPILER_STRING  ""
#endif

#define COMPILATION_DATE  __DATE__ " " __TIME__

ExceptionHandler::ExceptionHandler(){
	installed = false;
}

ExceptionHandler::~ExceptionHandler(){
	if(installed ==true)
		RemoveHandler();
}

bool ExceptionHandler::InstallHandler(){
	#if defined WIN32 || defined __WINDOWS__
	OTSYS_THREAD_LOCK_CLASS lockObj(maploadlock);
	if(maploaded == false)
		LoadMap();
	if( installed == true)
		return false;
	/*
		mov eax,fs:[0]
		mov [prevSEH],eax
		mov [chain].prev,eax
		mov [chain].SEHfunction,_SEHHandler
		lea eax,[chain]
		mov fs:[0],eax
	*/
	#ifdef __GNUC__
	SEHChain *prevSEH;
	__asm__ ("movl %%fs:0,%%eax;movl %%eax,%0;":"=r"(prevSEH)::"%eax" );
	chain.prev = prevSEH;
	chain.SEHfunction = (void*)&_SEHHandler;
	__asm__("movl %0,%%eax;movl %%eax,%%fs:0;": : "g" (&chain):"%eax");
	#endif//__GNUC__
	#endif//WIN32 || defined __WINDOWS__
	installed = true;
	return true;
}


bool ExceptionHandler::RemoveHandler(){
	if(installed == false)
		return false;
	#if defined WIN32 || defined __WINDOWS__
	/*
		mov eax,[chain.prev]
		mov fs:[0],eax
	*/
	#ifdef __GNUC__
	__asm__ ("movl %0,%%eax;movl %%eax,%%fs:0;"::"r"(chain.prev):"%eax" );
	#endif //__GNUC__
	#endif //WIN32 || defined __WINDOWS__
	installed = false;
	return true;
}

#if defined WIN32 || defined __WINDOWS__
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
	MEMORYSTATUS mstate;
	GlobalMemoryStatus(&mstate);
	*outdriver << "Memory load: " << mstate.dwMemoryLoad << std::endl <<
		"Total phys: " << mstate.dwTotalPhys/1024 << " K availble phys: " << 
		mstate.dwAvailPhys/1024 << " K" << std::endl;
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
	if((unsigned long)(ExceptionRecord->ExceptionAddress) >= min_off &&
		(unsigned long)(ExceptionRecord->ExceptionAddress) <= max_off){
		for(functions = functionMap.begin(); functions != functionMap.end(); ++functions) {
			if(functions->first > (unsigned long)(ExceptionRecord->ExceptionAddress) && 
					functions != functionMap.begin()) {
				functions--;
				*outdriver << "(" <<functions->second << ")" ;
				break;
			}
		}
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
	while(esp<stacklimit){
		stack_val = *esp;
		if(foundRetAddress)
			nparameters++;
		if(esp - stackstart < 20 || nparameters < 10 || std::abs(esp - next_ret) < 10){
			*outdriver  << (unsigned long)esp << " | ";
			printPointer(outdriver,stack_val);
			if(esp == next_ret){
				*outdriver << " \\\\\\\\\\\\ stack frame //////";
			}
			else if(esp - next_ret == 1){
				*outdriver << " <-- ret" ;
			}
			else if(esp - next_ret == 9){
				next_ret = (unsigned long*)*(esp - 9);
			}
			*outdriver<< std::endl;
		}
		if(stack_val >= min_off && stack_val <= max_off){
			foundRetAddress++;
			//
			FunctionMap::iterator functions;
			for(functions = functionMap.begin(); functions != functionMap.end(); ++functions) {
				if(functions->first > stack_val && functions != functionMap.begin()) {
					functions--;
					*outdriver  << (unsigned long)esp << "  " << 
					functions->second <<"("  << stack_val <<")" << std::endl;
					break;
				}
			}
		}
		esp++;
	}
	*outdriver << "*****************************************************" << std::endl;
	if(file)
		((std::ofstream*)outdriver)->close();
	MessageBox(NULL,"Please send the file report.txt to support service ;). Thanks","Error",MB_OK |MB_ICONERROR);
	std::cout << "Error report generated. Killing server." <<std::endl;
	exit(1); //force exit
	return ExceptionContinueSearch;
}

void printPointer(std::ostream* output,unsigned long p){
	*output << p;
	if(IsBadReadPtr((void*)p,4) == 0){
		*output << " -> " << *(unsigned long*)p;
	}
}

#endif //WIN32 || defined __WINDOWS__

#ifdef __GNUC__
bool ExceptionHandler::LoadMap(){
	if(maploaded == true){
		return false;
	}
	installed = false;
	//load map file if exists
	std::string line;
	std::ifstream input("otserv.map");
	min_off = 0xFFFFFF;
	max_off = 0;
	long n = 0;
    if (input.fail()){
		std::cout << "Failed loading symbols. otserv.map not found. " << std::endl;
		std::cout << "Go to http://otfans.net/index.php?showtopic=1716 for more info." << std::endl;
		system("pause");
		exit(1);
        return false;
	}
    //read until found .text           0x00401000
    while (getline(input,line,'\n')) {
		if(line.substr(0,5) == ".text")
			break;
	}
     
	if(input.eof()){
		return false;
	}
	
	std::string tofind = "0x";
    std::string space = " ";
    std::string lib = ".a(";
    while (getline(input,line,'\n')) {
		std::string::size_type pos = line.find(lib,0);
        if(pos != std::string::npos)
        	break;	//not load libs
		pos = line.find(tofind,0);
        if(pos != std::string::npos){
			//read hex offset
			std::string hexnumber = line.substr(pos,10);
			char *pEnd;
			unsigned long offset = strtol(hexnumber.c_str(),&pEnd,0);
			if(offset){
				//read function name
				std::string::size_type pos2 = line.find_first_not_of(space,pos+10);
				if(line[pos2] == '0' && line[pos2+1] == 'x')
					continue;
				std::string *savestring = new std::string;
				*savestring = line.substr(pos2,line.size() - pos2);
				functionMap[offset] = *savestring;
				if(offset > max_off)
					max_off = offset;
				if(offset < min_off)
					min_off = offset;
				n++;
			}
		}
    }
    // close file
    input.close();
	//std::cout << "Loaded " << n << " stack symbols" <<std::endl;
	maploaded = true;
	return true;
}
#else
bool ExceptionHandler::LoadMap(){
	return true;
};
#endif //__GNUC__

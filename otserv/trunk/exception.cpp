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
#include <ctime>
#include <stdlib.h> 
#include <map>

#include "exception.h"
#include "otsystem.h"
#include "excpt.h"

unsigned long max_off;
unsigned long min_off;
FunctionMap functionMap;
bool maploaded = false;
OTSYS_THREAD_LOCKVAR maploadlock;

EXCEPTION_DISPOSITION
 __cdecl _SEHHandler(
     struct _EXCEPTION_RECORD *ExceptionRecord,
     void * EstablisherFrame,
     struct _CONTEXT *ContextRecord,
     void * DispatcherContext
     );

ExceptionHandler::ExceptionHandler(){
}


ExceptionHandler::~ExceptionHandler(){
	if(installed ==true)
		RemoveHandler();
}



bool ExceptionHandler::InstallHandler(){
	OTSYS_THREAD_LOCK(maploadlock);
	if(maploaded == false)
		LoadMap();
	OTSYS_THREAD_UNLOCK(maploadlock);
	if( installed == true)
		return false;
		/*
			mov eax,fs:[0]
			mov eax,[prevSEH]
			mov [chain].prevSEH,eax
			mov [chain].SEHfunction,_SEHHandler
			lea eax,[chain]
			mov fs:[0],eax
		*/
		SEHChain *prevSEH;
	__asm__ ("movl %%fs:0,%%eax;movl %%eax,%0;":"=r"(prevSEH)::"%eax" );
	chain.prev = prevSEH;
	chain.SEHfunction = (void*)&_SEHHandler;
	__asm__("movl %0,%%eax;movl %%eax,%%fs:0;": : "g" (&chain):"%eax");
	
	installed = true;
	return true;
}


bool ExceptionHandler::RemoveHandler(){
	if(installed == false)
		return false;
	/*
		mov eax,[chain.prev]
		mov fs:[0],eax
	*/
	__asm__ ("movl %0,%%eax;movl %%eax,%%fs:0;"::"r"(chain.prev):"%eax" );
	installed = false;
	return true;
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
	unsigned long stack_val;
	unsigned long *stacklimit;
	char buffer[12];
	_MEMORY_BASIC_INFORMATION mbi;

	esp = (unsigned long *)(ContextRecord->Esp);
	VirtualQuery(esp, &mbi, sizeof(mbi));
	stacklimit = (unsigned long*)((unsigned long)(mbi.BaseAddress) + mbi.RegionSize);
	
	std::ostream *outdriver;
	
	std::ofstream output("report.txt",std::ios_base::app);
	if(output.fail()){
		outdriver = &std::cout;
	}
	else{
		outdriver = &output;
	}
	
	time_t rawtime;
	time(&rawtime);
	*outdriver << "*****************************************************" << std::endl;
	*outdriver << "Error report - " << std::ctime(&rawtime) << std::endl;
	ltoa((unsigned long)(ExceptionRecord->ExceptionCode),buffer,16);
	*outdriver << "Exception: 0x" << buffer << " at eip = 0x";
	ltoa((unsigned long)(ExceptionRecord->ExceptionAddress),buffer,16);
	*outdriver << buffer;
	FunctionMap::iterator functions;
	for(functions = functionMap.begin(); functions != functionMap.end(); ++functions) {
		if(functions->first > (unsigned long)(ExceptionRecord->ExceptionAddress)) {
			functions--;						
			*outdriver << "(" <<functions->second << ")" ;
			break;
		}
	}
	
	*outdriver << std::endl << std::endl;
	*outdriver << "---Stack Trace---" << std::endl;
	ltoa((unsigned long)esp,buffer,16);
	*outdriver << "From: 0x" << buffer;
	ltoa((unsigned long)stacklimit,buffer,16);
	*outdriver << " to: 0x" << buffer << std::endl;
	
	while(esp<stacklimit){
		stack_val = *esp;
		if(stack_val >= min_off && stack_val <= max_off){
			//
			FunctionMap::iterator functions;
			for(functions = functionMap.begin(); functions != functionMap.end(); ++functions) {
				if(functions->first > stack_val) {				
					functions--;
					ltoa((unsigned long)esp,buffer,16);
					*outdriver << "0x" << buffer << "  ";
					ltoa(stack_val,buffer,16);
					*outdriver << functions->second <<"(0x" << buffer <<")" << std::endl;
					break;
				}
			}		
		}
		esp++;
	}
	*outdriver << "*****************************************************" << std::endl;
	MessageBox(NULL,"Please send the file report.txt to support service ;). Thanks","Error",MB_OK |MB_ICONERROR);
	exit(1); //force exit
	return ExceptionContinueSearch;
}

bool ExceptionHandler::LoadMap(){
	OTSYS_THREAD_LOCK(maploadlock);
	if(maploaded == true){
		OTSYS_THREAD_UNLOCK(maploadlock);
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
		OTSYS_THREAD_UNLOCK(maploadlock);
        return false;
	}
    //read until found .text           0x00401000
    while (getline(input,line,'\n')) {
		if(line.substr(0,5) == ".text")
			break;
	}
     
	if(input.eof()){
		OTSYS_THREAD_UNLOCK(maploadlock);
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
	std::cout << "Loaded " << n << " stack symbols" <<std::endl;
	maploaded = true;
	OTSYS_THREAD_UNLOCK(maploadlock);
	return true;
}

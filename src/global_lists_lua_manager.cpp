#include "otpch.h"
#include "global_lists_lua_manager.h"
#include "global_list_lua_manager.h"
#include "abstract_lua_t.h"
#include <stdio.h>
#include <iostream>



#ifdef __GLOBAL_LIST_LUA_INTERFACE__

//it returns false if there is already a list with that name
bool global_lists_lua_manager::createNewList(std::string name)
{
if (listOfLists.find(name)==listOfLists.end()) {
    global_list_lua_manager* g=new global_list_lua_manager;
    if (g) {
        listOfLists[name]=g;
        return(true);
        }
    else
        std::cout << "Error creating new list named "<< name << "\n.";
    }
return(false);
}

bool global_lists_lua_manager::exists(std::string name)
{
return (listOfLists.find(name)!=listOfLists.end());
}

bool global_lists_lua_manager::deleteList(std::string name)
{
ListOfLists::iterator it = listOfLists.find(name);
if (it!=listOfLists.end()) {
    if (readerList == it->second)
        readerList = NULL;
    (it->second)->deleteAll();
    delete(it->second);
    listOfLists.erase(it);
    return(true);
    }
else
    return(false);
}

global_lists_lua_manager::global_lists_lua_manager()
{
readerList=NULL;
}

global_lists_lua_manager::~global_lists_lua_manager()
{
destroy();
}

bool global_lists_lua_manager::deleteValue(std::string name, abstract_lua_t key)
{
bool ret;
ListOfLists::iterator list = listOfLists.find(name);
if (list!=listOfLists.end()) {
    ret=(list->second)->deleteValue(key);
    return(ret);
    }
return(false);
}


void global_lists_lua_manager::destroy(void)
{
for (ListOfLists::iterator it=listOfLists.begin();it!=listOfLists.end();it++) {
    (it->second)->deleteAll();
    delete(it->second);
    }
listOfLists.erase(listOfLists.begin(),listOfLists.end());
}

bool global_lists_lua_manager::setValue(std::string name, abstract_lua_t key, abstract_lua_t value)
{
std::map<std::string,global_list_lua_manager*>::iterator it = listOfLists.find(name);
if (it!=listOfLists.end()) {
    (it->second)->setValue(key,value);
    return(true);
    }
return(false);
}


bool global_lists_lua_manager::getValue(std::string name, abstract_lua_t  key, abstract_lua_t &result)
{
std::map<std::string,global_list_lua_manager*>::iterator it = listOfLists.find(name);
if (it!=listOfLists.end()) {
    (it->second)->getValue(key,result);
    return(true);
    }
return(false);
}

int32_t global_lists_lua_manager::countValues(std::string name, abstract_lua_t value)
{
std::map<std::string,global_list_lua_manager*>::iterator it = listOfLists.find(name);
if (it!=listOfLists.end()) {
    return((it->second)->countValues(value));
    }
return(0);
}

bool global_lists_lua_manager::findValue(std::string name, abstract_lua_t value, abstract_lua_t &key, int n/*=1*/)
{
std::map<std::string,global_list_lua_manager*>::iterator it = listOfLists.find(name);
if (it!=listOfLists.end()) {
    return((it->second)->findValue(value, key, n));
    }
return(false);
}

unsigned global_lists_lua_manager::getSize(std::string name)
{
std::map<std::string,global_list_lua_manager*>::iterator it = listOfLists.find(name);
if (it!=listOfLists.end()) {
    return((it->second)->getSize());
    }
return(0);
}

bool global_lists_lua_manager::isEmpty(std::string name)
{
std::map<std::string,global_list_lua_manager*>::iterator it = listOfLists.find(name);
if (it!=listOfLists.end()) {
    return((it->second)->isEmpty());
    }
return(true);
}


bool global_lists_lua_manager::withList(std::string name)
{
std::map<std::string,global_list_lua_manager*>::iterator it = listOfLists.find(name);
if (it!=listOfLists.end()) {
    readerList=it->second;
    return(true);
    }
return(false);
}


/*
void global_lists_lua_manager::resetReader(void)
{

}

void global_lists_lua_manager::setReaderToEnd(std::string name)
{
}

bool global_lists_lua_manager::incReader(std::string name, int incr)
{
}

bool global_lists_lua_manager::read(std::string name, abstract_lua_t &key, abstract_lua_t &value)
{
}

bool global_lists_lua_manager::write(std::string name, abstract_lua_t value)
{
}

bool global_lists_lua_manager::changeValue(std::string name, abstract_lua_t value)
{
}

bool global_lists_lua_manager::setReaderToKey(std::string name, abstract_lua_t key)
{
}

*/

#endif

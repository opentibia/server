#include "otpch.h"
#include "global_list_lua_manager.h"


#ifdef __GLOBAL_LIST_LUA_INTERFACE__


/******beginning of the global_list_lua_manager********/

global_list_lua_manager::global_list_lua_manager()
{
reader = list.begin();
}


global_list_lua_manager::~global_list_lua_manager()
{
deleteAll();
}

void global_list_lua_manager::deleteAll(void)
{
list.erase(list.begin(),list.end());
reader = list.begin();
}


bool global_list_lua_manager::deleteValue(abstract_lua_t key)
{
std::map<abstract_lua_t,abstract_lua_t>::iterator it = list.find(key);
if (it!=list.end()) {
    if (reader==it)
        reader++;
    list.erase(it);
    return(true);
    }
return(false);
}

bool global_list_lua_manager::deleteActualValue(void)
{
std::map<abstract_lua_t,abstract_lua_t>::iterator it = reader;
if (it!=list.end()) {
    reader++;
    list.erase(it);
    return(true);
    }
return(false);
}

bool global_list_lua_manager::read(abstract_lua_t &key, abstract_lua_t &value)
{
if (reader!=list.end()) {
    key = reader->first;
    value = reader->second;
    return(true);
    }
return(false);
}

bool global_list_lua_manager::changeValue(abstract_lua_t value)
{
    if (reader!=list.end()) {
        if (!value.isNil())
            reader->second = value;
        else
            deleteActualValue();
        return(true);
        }
    return(false);
}

void global_list_lua_manager::resetReader(void)
{
    reader = list.begin();
}

void global_list_lua_manager::setReaderToEnd(void)
{
    reader = list.end();
}

bool global_list_lua_manager::incReader(int incr)
{
    if (incr>0) {
        for (int aux=1; aux<=incr; aux++) {
             if (reader!=list.end())
                 reader++;
             else
                 return(false);
            }
        }
    else if (incr<0) {
        for (int aux=incr; aux>=1; aux--) {
             if (reader!=list.begin())
                 reader--;
             else
                 return(false);
            }
        }
    return(true);
}

bool global_list_lua_manager::setReaderToKey(abstract_lua_t key)
{
std::map<abstract_lua_t,abstract_lua_t>::iterator it = list.find(key);
if (it!=list.end()) {
    reader = it;
    return(true);
    }
return(false);
}

bool global_list_lua_manager::getValue(abstract_lua_t key, abstract_lua_t &result)
{
std::map<abstract_lua_t,abstract_lua_t>::iterator it = list.find(key);
if (it!=list.end()) {
    result = it->second;
    return(true);
    }
return(false);
}

void global_list_lua_manager::setValue(abstract_lua_t key, abstract_lua_t value)
{
list[key] = value;
}

//retorna a enésima valor de key que tenha valor value em &key
//retorna true em caso de sucesso
bool global_list_lua_manager::findValue(abstract_lua_t value, abstract_lua_t &key, int n/*=1*/)
{
if (n<1) {
    std::cout << "Invalid value for n at global_list_lua_manager::findValue.\n";
    return(false);
    }
abstract_lua_t retorno;
int contador = 0;
std::map<abstract_lua_t,abstract_lua_t>::iterator it = list.begin();
while((it!=list.end()) && (contador<n)) {
       if (it->second==value) {
           retorno = it->first;
           contador++;
           }
       it++;
     }
if (contador==n) { //achou
    key = retorno;
    return(true);
    }
else
    return(false);
}

//conta quantos valores VALUE existem em uma lista
int32_t global_list_lua_manager::countValues(abstract_lua_t value)
{
    std::map<abstract_lua_t,abstract_lua_t>::iterator it;
    int32_t ret = 0;
    for (it = list.begin(); it!=list.end(); it++) {
         if (it->second == value)
             ret++;
         }
    return(ret);
}

#endif


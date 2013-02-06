#include "definitions.h"

#ifndef __global_lists_lua_manager__
#define __global_lists_lua_manager__

#include <iostream>
#include "global_list_lua_manager.h"
#include "abstract_lua_t.h"

#ifdef __GLOBAL_LIST_LUA_INTERFACE__

class global_lists_lua_manager {
    typedef std::map <std::string,global_list_lua_manager*> ListOfLists;
    protected:
        ListOfLists listOfLists;
        global_list_lua_manager *readerList;
    public:
        global_lists_lua_manager();
        ~global_lists_lua_manager();
        bool createNewList(std::string name);
        bool deleteValue(std::string name, abstract_lua_t key);
        bool deleteList(std::string name);
        void destroy(void);
        bool setValue(std::string name, abstract_lua_t key, abstract_lua_t value);
        bool getValue(std::string name, abstract_lua_t  key, abstract_lua_t &result);
        int32_t countValues(std::string name, abstract_lua_t value);
        bool findValue(std::string name, abstract_lua_t value, abstract_lua_t &key, int n=1);
        bool isEmpty(std::string name);
        bool exists(std::string name);
        unsigned getSize(std::string name);
        bool withList(std::string name);
        global_list_lua_manager* getReader(void) { return(readerList); }

    };

#endif

#endif

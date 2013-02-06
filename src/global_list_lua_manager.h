#include "definitions.h"

#ifndef __global_list_lua_manager__
#define __global_list_lua_manager__

#ifdef __GLOBAL_LIST_LUA_INTERFACE__

#include "abstract_lua_t.h"
#include <iostream>

#include "definitions.h"

class global_list_lua_manager
{
    protected:
        std::map <abstract_lua_t ,abstract_lua_t> list;
        std::map<abstract_lua_t,abstract_lua_t>::iterator reader;
    public:
        global_list_lua_manager();
        ~global_list_lua_manager();
        bool getValue(abstract_lua_t key, abstract_lua_t &result);
        void setValue(abstract_lua_t key, abstract_lua_t value);
        bool deleteValue(abstract_lua_t key);
        void deleteAll(void); //deletes all items at the list
        int32_t countValues(abstract_lua_t value);
        bool findValue(abstract_lua_t value, abstract_lua_t &key, int n=1);
        unsigned getSize(void) { return(list.size()); };
        bool isEmpty(void) { return (list.empty());};
        void resetReader(void);
        void setReaderToEnd(void);
        bool incReader(int incr);
        bool read(abstract_lua_t &key, abstract_lua_t &value);
        bool deleteActualValue(void);
        bool changeValue(abstract_lua_t value);
        bool setReaderToKey(abstract_lua_t key);
};

#endif

#endif

#include "definitions.h"

#ifdef __GLOBAL_LIST_LUA_INTERFACE__

#ifndef __ABSTRACT_LUA_T__
#define __ABSTRACT_LUA_T__

#include <map>
#include <string>
#include <iostream>

enum type_abstract_lua { NONE_LUA_T, REAL_LUA_T, STRING_LUA_T, BOOL_LUA_T };

class abstract_lua_t
{
	type_abstract_lua tipo;
	double d;
	std::string s;
	bool b;
	public:
		abstract_lua_t() { tipo=NONE_LUA_T; };
		//bool operator == (abstract_lua_t &x) { return((!((*this)<x)) && (!(x<(*this))));};
		bool operator == (abstract_lua_t &x) const { return(this->isEqual(x));};
		bool isString(void) const { return(tipo==STRING_LUA_T); };
		bool isReal(void) const { return(tipo==REAL_LUA_T); };
		bool isBool(void) const { return(tipo==BOOL_LUA_T); };
		bool isNil(void) const { return(tipo==NONE_LUA_T);};
		bool getString(std::string &v) const ;
		bool getReal(double &v) const;
		bool getBool(bool &v) const;
		void set(std::string s);
		void set(double d);
		void setBool(bool b);
		friend std::ostream& operator << (std::ostream &s, const abstract_lua_t x);
		friend bool operator < (const abstract_lua_t a,const abstract_lua_t b);
	private:
		bool isEqual(const abstract_lua_t a) const;
};
#endif

#endif

#include "otpch.h"
#include "abstract_lua_t.h"
#include <iostream>


#ifdef __GLOBAL_LIST_LUA_INTERFACE__
bool abstract_lua_t::getString(std::string &v) const
{
if (isString()) {
	v = s;
	return(true);
	}
else
    return(false);
}

bool abstract_lua_t::getReal(double &v) const
{
if (isReal()) {
	v = d;
	return(true);
	}
else
	return(false);
}

bool abstract_lua_t::getBool(bool &v) const
{
if (isBool()) {
	v = b;
	return(true);
	}
else
	return(false);
}

void abstract_lua_t::set(std::string v)
{
	tipo = STRING_LUA_T;
	s = v;
}

void abstract_lua_t::set(double v)
{
	tipo = REAL_LUA_T;
	d = v;
}

void abstract_lua_t::setBool(bool v)
{
	tipo = BOOL_LUA_T;
	b = v;
}


bool operator < (const abstract_lua_t a, const abstract_lua_t b)
{
if(int(a.tipo) != int(b.tipo))
	return(a.tipo < b.tipo);
if(a.isReal())
	return(a.d < b.d);
if(a.isString())
	return (a.s < b.s);
if(a.isBool())
	return (int(a.b) < int(b.b));
return(false); //both are empty
}

bool abstract_lua_t::isEqual(const abstract_lua_t a) const
{
	if (int(a.tipo) != int(this->tipo))
		return(false);
	if (a.isReal())
		return(a.d == this->d);
	if (a.isString())
		return(a.s == this->s);
	if (a.isBool())
		return(a.b == this->b);
	return(true); //both are empty
}


std::ostream& operator << (std::ostream &c, const abstract_lua_t x)
{
if (x.isString())
	return(c << x.s);
if (x.isReal())
	return(c << x.d);
if (x.isBool()){
	if (x.b)
		return (c << "true");
	else
		return (c << "false");
	}
return(c << "nil");
}
#endif


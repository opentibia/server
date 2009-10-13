/////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
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

#ifndef OTSERV_ENUM_H__
#define OTSERV_ENUM_H__

#include <assert.h>
#include <stdexcept>

class enum_conversion_error : public std::logic_error {
public:
	enum_conversion_error(const std::string& err) : logic_error(err) {}
};

// Iteration

template <class ET>
class enum_iterator {
	typedef typename ET::EnumToString::iterator _internal;
	_internal i;
public:
	enum_iterator(const enum_iterator<ET>& o) : i(o.i) {}
	enum_iterator(const _internal& i) : i(i) {}
	enum_iterator(const ET& e) {
		e.init();
		i = e.enum_to_string.find(e);
		if(i == e.enum_to_string.end()){
			std::ostringstream os;
			os << "Enum " << e.name() << " value out of range (" << (int)e.e << ")";
			throw enum_conversion_error(os.str());
		}
	}

	// Get the value of the iterator
	ET operator* () {return ET(typename ET::enum_type(i->first.value()));}
	// Evil reinterpret cast, /should/ never break
	const ET* operator->() {return reinterpret_cast<const ET*>(&(i->first));}

	// Move the iterator
	const enum_iterator<ET>& operator++() {
		++i;
		return *this;
	}
	enum_iterator<ET> operator++(int) {
		enum_iterator<ET> r = *this;
		++i;
		return r;
	}
	
	// Compare the iterator
	bool operator==(const enum_iterator<ET>& o) const {return i == o.i;}
	bool operator==(const ET& e) const                {return i->first.e == e.e;}
	bool operator!=(const enum_iterator<ET>& o) const {return i != o.i;}
	bool operator!=(const ET& e) const                {return i->first.e != e.e;}
	bool operator<=(const enum_iterator<ET>& o) const {return i <= o.i;}
	bool operator<=(const ET& e) const                {return i->first.e <= e.e;}
	bool operator>=(const enum_iterator<ET>& o) const {return i >= o.i;}
	bool operator>=(const ET& e) const                {return i->first.e >= e.e;}
	bool operator< (const enum_iterator<ET>& o) const {return i < o.i;}
	bool operator< (const ET& e) const                {return i->first.e <  e.e;}
	bool operator> (const enum_iterator<ET>& o) const {return i > o.i;}
	bool operator> (const ET& e) const                {return i->first.e >  e.e;}
};

template <class E, int size_> class BitEnum;

template <class E, int size_>
class Enum {
protected:
	E e;

public:
	// Some useful types
	typedef std::map<Enum<E, size_>, std::vector<std::string> > EnumToString;
	typedef std::map<std::string, Enum<E, size_> > StringToEnum;
	// Required for some name-resolving of BitEnums
	typedef Enum<E, size_> base_class;
	typedef E enum_type;

public:
	Enum() : e(E(0)) {}
	Enum(E e) : e(e) {}
	Enum(const Enum<E, size_>& o) : e(o.e) {}
	explicit Enum(enum_iterator<Enum<E, size_> >& o) : e(E(o->value())) {}
	explicit Enum(int e) : e(E(e)) {}

	// Classes
	typedef enum_iterator< Enum<E, size_> > iterator;
	typedef iterator const_iterator; // const_iterator is the same

	/**
	 * The size required by an array to use this enum as a key.
	 */
	enum {size = size_};

	// Static utility functions
	
	// Conversion to some types..
	E evalue() const{return e;}
	int value() const {return int(e);}

private:
	void _boolean_true() const {}
	typedef void (Enum<E, size_>::*_boolean_type)() const;
public:
	operator _boolean_type() const {
		if(e != E(0))
			return &Enum<E, size_>::_boolean_true;
		return NULL;
	}
	// Short way to call ->value
	int operator*() const {
		return value();
	}

	/**
	 * Converts an Enum value to a string
	 * will throw if the value does not exist
	 *
	 * @param _e The enum value to convert to a string
	 */
	static std::string toString(const Enum<E, size_>& _e) {
		init();
		typename EnumToString::const_iterator i = enum_to_string.find(_e);
		if(i == enum_to_string.end()) {
			std::ostringstream os;
			os << "Enum " << enum_name << " value out of range (" << (int)_e.e << ")";
			throw enum_conversion_error(os.str());
		}
		return i->second.front();
	}

	/**
	 * Get all string values associated with one enum value
	 * will throw if the value does not exist
	 *
	 * @param _e The enum value to convert to a string
	 */
	static std::vector<std::string> toStrings(const Enum<E, size_>& _e) {
		init();
		typename EnumToString::const_iterator i = enum_to_string.find(_e);
		if(i == enum_to_string.end())
			return std::vector<std::string>();
		return i->second;
	}

	/**
	 * Gets an enum value, given the string value, will assert if the value does not exist
	 *
	 * @param str The string name of the enum value
	 * @return The Enum value corresponding
	 **/
	static Enum<E, size_> fromString(const std::string& str) {
		init();
		typename StringToEnum::const_iterator i = string_to_enum.find(str);
		if(i == string_to_enum.end()) {
			std::ostringstream os;
			os << "Enum " << enum_name << " value does not exist (" << str << ")";
			throw enum_conversion_error(os.str());
		}
		return i->second;
	}

	/**
	 * Constructs enum value from an integer, will throw if the value is invalid
	 *
	 * @param id The string int to convert to an enum value
	 * @return The Enum value corresponding
	 **/
	static Enum<E, size_> fromInteger(int id) {
		init();
		typename EnumToString::const_iterator i = enum_to_string.find(Enum<E, size_>(id));
		if(i == enum_to_string.end()) {
			std::ostringstream os;
			os << "Enum " << enum_name << " value is invalid (" << id << ")";
			throw enum_conversion_error(os.str());
		}
		return i->first;
	}

	/**
	 * Gets an enum value, given the string value, will assert if the value does not exist
	 * This version is case-insensitive
	 *
	 * @param str The string name of the enum value, case does not matter
	 * @return The Enum value corresponding
	 **/
	static Enum<E, size_> fromStringI(const std::string& str) {
		init();
		typename StringToEnum::const_iterator i = lstring_to_enum.find(str);
		if(i == lstring_to_enum.end()) {
			std::ostringstream os;
			os << "Enum " << enum_name << " value out of range (" << str << ")";
			throw enum_conversion_error(os.str());
		}
		return i->second;
	}

	/**
	 * Check if the given int is a valid enum value (in this enum).
	 *
	 * @param v The value to check
	 * @return returns true if the value is part of this enum
	 */
	static bool exists(int v) {
		init();
		typename EnumToString::const_iterator i = enum_to_string.find(E(v));
		return i != enum_to_string.end();
	}

	/**
	 * Returns the name of this enum
	 */
	static std::string name() {
		init();
		return enum_name;
	}

	/**
	 * Returns this enum value converted to a string.
	 * throws if value is not part of the enum
	 *
	 * @return The name of this value
	 */
	std::string toString() const {
		return toString(*this);
	}

	/**
	 * Returns a list of all string values of this enum value (may be empty)
	 *
	 * @return The name(s) of this value
	 */
	std::vector<std::string> toStrings() const {
		return toStrings(*this);
	}

	/**
	 * Returns true if this is a valid value
	 */
	bool exists() {
		return exists(value());
	}

	// Iteration
	/**
	 * Returns an iterator to the first element
	 */
	static iterator begin() {
		init();
		return iterator(enum_to_string.begin());
	}

	/**
	 * Returns an iterator to a virtual element beyond the last element
	 */
	static iterator end()   {
		init();
		return iterator(enum_to_string.end());
	}

protected: // Private stuff

	static void initialize();

	// This is declared here so it can be inlined
	static void init() {
		if(initialized)
			return;
		initialized = true;
		initialize();
	}

	static void initAddValue(E _e, std::string str, bool real_name) {
		enum_to_string[_e].push_back(str);

		string_to_enum[str] = _e;
		std::transform(str.begin(), str.end(), str.begin(), tolower);
		lstring_to_enum[str] = _e;
	}

	static bool initialized;
	static std::string enum_name;
	static EnumToString enum_to_string;
	static StringToEnum string_to_enum;
	static StringToEnum lstring_to_enum;


public: // Operators

	// Comparison
	bool operator==(const Enum<E, size_>& o) const {return e == o.e;}
	bool operator!=(const Enum<E, size_>& o) const {return e != o.e;}
	bool operator<=(const Enum<E, size_>& o) const {return e <= o.e;}
	bool operator>=(const Enum<E, size_>& o) const {return e >= o.e;}
	bool operator< (const Enum<E, size_>& o) const {return e <  o.e;}
	bool operator> (const Enum<E, size_>& o) const {return e >  o.e;}

	friend class enum_iterator< Enum<E, size_> >;
	friend class enum_iterator< BitEnum<E, size_> >;
};


/**
 * BitEnum, an enum that can be ORed
 * Note that the classes are NOT polymorphic, so never pass this as an Enum&
 */
template <class E, int size_ =  -1 /* Will always cause error when trying to use */>
class BitEnum : public Enum<E, size_> {
public:
	BitEnum() {};
	BitEnum(E e) : Enum<E, size_>(e) {}
	BitEnum(const Enum<E, size_>& e) : Enum<E, size_>(e) {}
	BitEnum(const BitEnum<E, size_>& e) : Enum<E, size_>(e.e) {}
	explicit BitEnum(int e) : Enum<E, size_>(e) {}

	BitEnum<E, size_> operator~() const {return E(~int(Enum<E, size_>::e));}

	BitEnum<E, size_> operator|(const BitEnum<E, size_>& o) const {return E(int(Enum<E, size_>::e) | int(o.e));}
	BitEnum<E, size_> operator&(const BitEnum<E, size_>& o) const {return E(int(Enum<E, size_>::e) & int(o.e));}

	BitEnum<E, size_>& operator|=(const BitEnum<E, size_>& o) {Enum<E, size_>::e = E(int(Enum<E, size_>::e) | int(o.e)); return *this;}
	BitEnum<E, size_>& operator&=(const BitEnum<E, size_>& o) {Enum<E, size_>::e = E(int(Enum<E, size_>::e) & int(o.e)); return *this;}
	
	// Classes
	typedef enum_iterator< BitEnum<E, size_> > iterator;
	typedef iterator const_iterator; // const_iterator is the same

	/**
	 * Converts an Enum value to a string
	 * will be on bitfield form (A|B|C)
	 *
	 * @param _e The enum value to convert to a string
	 */
	static std::string toString(const Enum<E, size_>& _e) {
		if(Enum<E, size_>::exists(_e.value())){
			return Enum<E, size_>::toString(_e);
		}

		std::ostringstream os;
		bool first = true;
		for(int i = 1; ; i <<= 1){
			if((_e.value() & i) == i){
				if(!first)
					os << " | ";
				first = false;
				os << Enum<E, size_>::toString(E(i));
			}
			if(i == (1 << 30))
				break;
		}

		return os.str();
	}

	std::string toString() const {
		return toString(*this);
	}

	/**
	 * Returns the index of the first bit set in this enum
	 * Returns 0 if no bits are set
	 */
	int index() const {
		for(int i = 1, j = 0; ; i <<= 1, ++j){
			if((int(Enum<E, size_>::e) & i) == i)
				return j;
			if(i == (1 << 30))
				break;
		}
		return 0;
	}


	// Iteration
	// We need to overload here to get the correct type (we shadow the default here)
	/**
	 * Returns an iterator to the first element
	 */
	static iterator begin() {
		Enum<E, size_>::init();
		return iterator(Enum<E, size_>::enum_to_string.begin());
	}

	/**
	 * Returns an iterator to a virtual element beyond the last element
	 */
	static iterator end()   {
		Enum<E, size_>::init();
		return iterator(Enum<E, size_>::enum_to_string.end());
	}
};

template <class E, int size_>
std::ostream& operator<<(std::ostream& os, const BitEnum<E, size_>& e) {
	return os << BitEnum<E, size_>::toString(e);
}

template <class E, int size_>
std::ostream& operator<<(std::ostream& os, const Enum<E, size_>& e) {
	return os << Enum<E, size_>::toString(e);
}

template <class E, int size_>
int operator<<(int i, const Enum<E, size_>& e) {
	return 1 << e.value();
}

#endif

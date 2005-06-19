//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
//
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

#ifndef __TEMPLATES_H__
#define __TEMPLATES_H__

#include <vector>

// cross compatibility vc++ and gcc
#ifdef __GNUC__
#include <ext/hash_map>
#else
#include <hash_map>
#endif

#include "creature.h"

template<class T> class AutoList
{
public:
	AutoList(unsigned long _id) {
		listid = _id;
		list[_id] = (T*)this; //reinterpret_cast<T*>(this);
	}
	virtual ~AutoList() { list.erase(listid); }

	// cross compatibility vc++ and gcc
	#ifdef __GNUC__
	static __gnu_cxx::hash_map<unsigned long, T*> list;
  typedef __gnu_cxx::hash_map<unsigned long, T*> list_type;
	#else
	static stdext::hash_map<unsigned long, T*> list;
  typedef stdext::hash_map<unsigned long, T*> list_type;
	#endif

	typedef typename list_type::iterator listiterator;

private:

	AutoList(const AutoList& p);
	const AutoList& operator=(const AutoList& rhs);

protected:
	unsigned long listid;
};

template<class T> class AutoID {
public:
	AutoID() {
		unsigned long newid = count;
		bool wrapped = false;
		while (find(list.begin(), list.end(), newid) != list.end()) {
			if(count + 1 == T::max_id) {
				wrapped = true;
				count = T::min_id;
			}

			newid = count;
			++count;
		}

		if(newid == count) {
			if(count + 1 == T::max_id) {
				wrapped = true;
				count = T::min_id;
			}
			else
				++count;
		}

		id = newid;
		list.push_back(id);
	}

	virtual ~AutoID() {
		list_iterator it = std::find(list.begin(), list.end(), id);
		if(it != list.end())
			list.erase(it);
	}

	typedef std::vector<unsigned long> list_type;
	typedef typename list_type::iterator list_iterator;

	unsigned long id;

protected:
	static list_type list;
	static unsigned long count;
};

#endif

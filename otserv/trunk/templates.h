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

#include <set>
#include <map>


#include "creature.h"
#include "otsystem.h"

template<class T> class AutoList
{
public:
	AutoList(){}
	
	~AutoList(){
		list.clear();
	}
	
	void addList(T* t){
		list[t->getID()] = t;
	}
	
	void removeList(unsigned long _id){
		list.erase(_id);
	}
	
	typedef std::map<unsigned long, T*> list_type;
	list_type list;

	typedef typename list_type::iterator listiterator;
};

class AutoID {
public:
	AutoID() {
		OTSYS_THREAD_LOCK_CLASS lockClass(autoIDLock);
		count++;
		if(count >= 0xFFFFFF)
			count = 1000;
		
		while(list.find(count) != list.end()){
			if(count >= 0xFFFFFF)
				count = 1000;
			else
				count++;
		}
		list.insert(count);
		auto_id = count;
	}
	virtual ~AutoID(){
		list_type::iterator it = list.find(auto_id);
		if(it != list.end())
			list.erase(it);
	}

	typedef std::set<unsigned long> list_type;

	unsigned long auto_id;
	static OTSYS_THREAD_LOCKVAR autoIDLock;
	
protected:
	static unsigned long count;
	static list_type list;

};

#endif

//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// memory allocator
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

#ifndef __OTSERV_ALLOCATOR_H
#define __OTSERV_ALLOCATOR_H

#include <map>
#include <boost/pool/pool.hpp>
#include "otsystem.h"

struct poolTag {
  size_t poolbytes;
};

class PoolManager {
public:
	static PoolManager& getInstance() {
		static PoolManager instance;
		return instance;
	}

  void* allocate(size_t size) {
    Pools::iterator it;
    OTSYS_THREAD_LOCK_CLASS(poolLock, NULL);

    for(it = pools.begin(); it != pools.end(); ++it) {
      if(it->first >= size + sizeof(poolTag)) {
        poolTag* tag = reinterpret_cast<poolTag*>(it->second->ordered_malloc());
        tag->poolbytes = it->first;
        return tag + 1;
      }
    }

    poolTag* tag = reinterpret_cast<poolTag*>(std::malloc(size + sizeof(poolTag)));
    tag->poolbytes = 0;
    return tag + 1;
	}

	void deallocate(void* deletable) {
    if(deletable == NULL)
      return;

    poolTag* const tag = reinterpret_cast<poolTag*>(deletable) - 1U;
    if(tag->poolbytes) {
      Pools::iterator it;
      OTSYS_THREAD_LOCK_CLASS(poolLock, NULL);

      it = pools.find(tag->poolbytes);
      it->second->ordered_free(tag);
    }
    else
      std::free(tag);
	}

	~PoolManager() {
    Pools::iterator it = pools.begin();
    while(it != pools.end()) {
      delete it->second;
      it = pools.erase(it);
    }
	}

private:
  void addPool(size_t size, size_t next_size) {
    pools[size] = new boost::pool<>(size, next_size); 
  }

	PoolManager() {
    OTSYS_THREAD_LOCKVARINIT(poolLock);

    addPool(32, 1024);
    addPool(64, 1024);
    addPool(128, 1024);
    addPool(256, 1024);
    addPool(512, 1024);
    addPool(1024, 1024);
    addPool(2048, 1024);
    addPool(4096, 1024);
    addPool(8192, 1024);
    addPool(16384, 1024);
    addPool(18432, 1024);
  }

	PoolManager(const PoolManager&);
	const PoolManager& operator=(const PoolManager&);

  typedef std::map<size_t, boost::pool<>* > Pools;
  Pools pools;
  OTSYS_THREAD_LOCKVAR poolLock;
};

#endif
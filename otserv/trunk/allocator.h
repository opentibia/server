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

class PoolManager {
public:
	static PoolManager& getInstance() {
		static PoolManager instance;
		return instance;
	}

  void* allocate(size_t size) {
    std::map<size_t, poolBlock* >::iterator it;
    for(it = pools.begin(); it != pools.end(); ++it) {
      if(it->first >= size) {
        OTSYS_THREAD_LOCK_CLASS(it->second->lock, NULL);
        return it->second->pool->ordered_malloc();
      }
    }

    return NULL;

    /*
    if(it == pools.end()) {
      pools[size] = new boost::pool<>(size, 1024);
      return pools[size]->ordered_malloc();
    }
    else
      return it->second->ordered_malloc();
    */
	}

	void deallocate(void* deletable /*, size_t size*/) {
    if(deletable == NULL)
      return;

    std::map<size_t, poolBlock* >::iterator it;
    for(it = pools.begin(); it != pools.end(); ++it) {
      if(it->second->pool->is_from(deletable)) {
        OTSYS_THREAD_LOCK_CLASS(it->second->lock, NULL);
        it->second->pool->ordered_free(deletable);
      }
    }

    //pools[size]->ordered_free(deletable);
	}

	~PoolManager() {
    std::map<size_t, poolBlock* >::iterator it = pools.begin();
    while(it != pools.end()) {
      delete it->second->pool;
      delete it->second;
      it = pools.erase(it);
    }
	}

private:
  void addPool(size_t size, size_t next_size) {
    poolBlock* pb = new poolBlock;
    OTSYS_THREAD_LOCKVARINIT(pb->lock);
    pb->pool = new boost::pool<>(size, next_size);

    pools[size] = pb;
  }

	PoolManager() {
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
  
  struct poolBlock {
    OTSYS_THREAD_LOCKVAR lock;
     boost::pool<>* pool;
  };

  std::map<size_t, poolBlock* > pools;
};

#endif
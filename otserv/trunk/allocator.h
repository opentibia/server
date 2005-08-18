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

#ifdef __OTSERV_ALLOCATOR__

#include <memory>
#include <limits>
#include <cstdlib>
#include <map>
#include <fstream>
#include <ctime>
#include <boost/pool/pool.hpp>

template<typename T>
class dummyallocator {
public:
	typedef std::size_t size_type;
	typedef std::ptrdiff_t difference_type;
	typedef T* pointer;
	typedef const T* const_pointer;
	typedef T& reference;
	typedef const T& const_reference;
	typedef T value_type;
	template <class U> struct rebind {
		typedef dummyallocator<U> other;
	};
	dummyallocator(  ) throw(  )                          {}
	dummyallocator(const dummyallocator&) throw(  )          {}
	template <class U>
		dummyallocator(const dummyallocator<U>&) throw(  )       {}
	~dummyallocator(  ) throw(  )                         {}
	pointer address(reference x) const                 {return &x;}
	const_pointer address(const_reference x) const     {return &x;}
	pointer allocate(size_type n, void* hint = 0) {
		return static_cast<T*>(std::malloc(n * sizeof(T)) );
	}
	void deallocate(pointer p, size_type n) {
		std::free(static_cast<void*>(p));
	}
	size_type max_size(  ) const throw(  ) {
		return std::numeric_limits<size_type>::max() / sizeof(T);
	}
	void construct(pointer p, const T& val) {
		new(static_cast<void*>(p)) T(val);
	}
	void destroy(pointer p) {
		p->~T(  );
	}
};

void* operator new(size_t bytes, int dummy);
void* operator new(size_t bytes);
void* operator new[](size_t bytes);
void operator delete(void *p);
void operator delete[](void* p);

#ifdef _MSC_VER
void operator delete(void* p, int dummy);
void operator delete[](void* p, int dummy);
#endif

#ifdef __OTSERV_ALLOCATOR_STATS__
OTSYS_THREAD_RETURN allocatorStatsThread(void *a);
#endif

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
		OTSYS_THREAD_LOCK(poolLock, NULL);
		
		for(it = pools.begin(); it != pools.end(); ++it) {
			if(it->first >= size + sizeof(poolTag)) {
				//poolTag* tag = reinterpret_cast<poolTag*>(it->second->ordered_malloc());
				poolTag* tag = reinterpret_cast<poolTag*>(it->second->malloc());
				tag->poolbytes = it->first;
				#ifdef __OTSERV_ALLOCATOR_STATS__
				poolsStats[it->first]->allocations++;
				poolsStats[it->first]->unused+= it->first - (size + sizeof(poolTag));
				#endif
				OTSYS_THREAD_UNLOCK(poolLock, NULL);
				return tag + 1;
			}
		}
		
		poolTag* tag = reinterpret_cast<poolTag*>(std::malloc(size + sizeof(poolTag)));
		#ifdef __OTSERV_ALLOCATOR_STATS__
		poolsStats[0]->allocations++;
		poolsStats[0]->unused += size;
		#endif
		tag->poolbytes = 0;
		OTSYS_THREAD_UNLOCK(poolLock, NULL);
		return tag + 1;
	}
	
	void deallocate(void* deletable) {
		if(deletable == NULL)
			return;
		
		poolTag* const tag = reinterpret_cast<poolTag*>(deletable) - 1U;
		OTSYS_THREAD_LOCK(poolLock, NULL);
		if(tag->poolbytes) {
			Pools::iterator it;
			
			it = pools.find(tag->poolbytes);
			//it->second->ordered_free(tag);
			it->second->free(tag);
			#ifdef __OTSERV_ALLOCATOR_STATS__
			poolsStats[it->first]->deallocations++;
			#endif
		}
		else{
			std::free(tag);
			#ifdef __OTSERV_ALLOCATOR_STATS__
			poolsStats[0]->deallocations++;
			#endif
		}
		OTSYS_THREAD_UNLOCK(poolLock, NULL);
	}
	
	/*
	void releaseMemory(){
		Pools::iterator it;
		for(it = pools.begin(); it != pools.end(); ++it) {
			it->second->release_memory();
		}
	}
	*/
	
	#ifdef __OTSERV_ALLOCATOR_STATS__
	void dumpStats(){
		time_t rawtime;
		time(&rawtime);
		std::ofstream output("mem_dump.txt",std::ios_base::app);
		output << "Otserv Allocator Stats: " << std::ctime(&rawtime);
		PoolsStats::iterator it;
		for(it = poolsStats.begin(); it != poolsStats.end(); ++it) {
			output << (int)(it->first) << " alloc: " << (int)(it->second->allocations) << 
				" dealloc: " << (int)(it->second->deallocations) << 
				" unused: " << (int)(it->second->unused);
			if(it->second->allocations != 0 && it->first != 0){
				output << " avg: " << (int)((it->first) - (it->second->unused)/(it->second->allocations)) << 
				" %unused: " << (int)((it->second->unused)*100/(it->second->allocations)/(it->first));
			}
			output << " N: " << ((int)(it->second->allocations) - (int)(it->second->deallocations)) <<
				std::endl;
		}
		output << std::endl;
		output.close();
	}
	#endif

	~PoolManager() {
		Pools::iterator it = pools.begin();
		while(it != pools.end()) {
			std::free(it->second);
			pools.erase(it++);
		}
		#ifdef __OTSERV_ALLOCATOR_STATS__
		PoolsStats::iterator it2;
		for(it2 = poolsStats.begin(); it2 != poolsStats.end(); ++it2) {
			std::free(it2->second);
			poolsStats.erase(it2++);
		}
		#endif
	}
	
private:
	void addPool(size_t size, size_t next_size) {
		pools[size] = new(0) boost::pool<boost::default_user_allocator_malloc_free>(size, next_size); 
		#ifdef __OTSERV_ALLOCATOR_STATS__
		t_PoolStats * tmp = new(0) t_PoolStats;
		tmp->unused = 0;
		tmp->allocations = 0;
		tmp->deallocations = 0;
		poolsStats[size] = tmp;
		#endif
	}
	
	PoolManager() {
		OTSYS_THREAD_LOCKVARINIT(poolLock);
		addPool(32, 16384);
		addPool(64, 16384);
		addPool(128, 4096);
		addPool(256, 256);
		addPool(512, 256);
		addPool(768, 256);
		addPool(2048, 128);
		addPool(4096, 128);
		addPool(8192, 128);
		addPool(18432, 128);
		#ifdef __OTSERV_ALLOCATOR_STATS__
		t_PoolStats * tmp = new(0) t_PoolStats;
		tmp->unused = 0;
		tmp->allocations = 0;
		tmp->deallocations = 0;
		poolsStats[0] = tmp;
		#endif
	}
	
	PoolManager(const PoolManager&);
	const PoolManager& operator=(const PoolManager&);
	
	typedef std::map<size_t, boost::pool<boost::default_user_allocator_malloc_free >*, std::less<size_t >, 
		dummyallocator<std::pair<const size_t, boost::pool<boost::default_user_allocator_malloc_free>* > > > Pools;
	
	Pools pools;
	#ifdef __OTSERV_ALLOCATOR_STATS__
	struct t_PoolStats{
		int allocations;
		int deallocations;
		int unused;
	};
	typedef std::map<size_t, t_PoolStats*, std::less<size_t >, 
		dummyallocator<std::pair<const size_t, t_PoolStats* > > > PoolsStats;
	PoolsStats poolsStats;
	#endif
	OTSYS_THREAD_LOCKVAR poolLock;
};

#endif

#endif

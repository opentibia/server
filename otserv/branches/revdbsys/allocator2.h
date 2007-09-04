//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// memory allocator2
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

#ifndef __OTSERV_ALLOCATOR2_H
#define __OTSERV_ALLOCATOR2_H

#ifdef __OTSERV_ALLOCATOR_H
#error Do not include both allocators!
#endif

#include <memory>
#include <cstdlib>
#include <map>
#include <fstream>
#include <iostream>
#include <ctime>
#include "math.h"

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

struct poolTag {
	size_t poolbytes;
};

class PoolManager {
public:
	static PoolManager& getInstance() {
		return m_instance;
	}

	void* allocate(size_t size) {
		OTSYS_THREAD_LOCK(poolLock, NULL);
		uint32_t* tag = (uint32_t*)malloc(size + sizeof(uint32_t));
		tag[0] = size;
		m_nAllocate++;
		m_allocatedSize = m_allocatedSize + size;
		m_allocatedSize2 = m_allocatedSize2 + size*size;
		m_usage = m_usage + size;
		if(m_usage > m_maxUsage)
			m_maxUsage = m_usage;
			
		OTSYS_THREAD_UNLOCK(poolLock, NULL);
		return (void*)(tag + 1);
	}

	void deallocate(void* deletable) {
		if(deletable == NULL)
			return;
			
		OTSYS_THREAD_LOCK(poolLock, NULL);
		uint32_t* tag = (uint32_t*)deletable;
		tag = tag - 1;
		m_usage = m_usage - tag[0];
		m_nDeallocate--;
		free(tag);
		OTSYS_THREAD_UNLOCK(poolLock, NULL);
	}

	~PoolManager() {
		//OTSYS_THREAD_LOCKVARRELEASE(poolLock);
	}

	void restStats()
	{
		m_nAllocate = 0;
		m_nDeallocate = 0;
		m_maxUsage = 0;
		m_usage = 0;
		m_allocatedSize = 0;
		m_allocatedSize2 = 0;
	}

	void dumpStats(const char* msg)
	{
		time_t rawtime;
		time(&rawtime);
		std::ofstream output("mem_dump.txt",std::ios_base::app);
		output << msg << " - " << std::ctime(&rawtime);
		output << "Allocate: " << m_nAllocate << std::endl;
		output << "Dellocate: " << m_nAllocate << std::endl;
		output << "Size total: " << m_allocatedSize << std::endl;
		float avg = 0.f, sd = 0.f;
		if(m_nAllocate != 0){
			avg = (m_allocatedSize*1.f)/m_nAllocate;
		}
		if(m_nAllocate > 1){
			sd = sqrt((m_allocatedSize2/(1.f*m_nAllocate - 1) - avg*avg));
		}
		output << "Avg Size: " << avg << " ± " << sd << std::endl;
		output << "Max usage:: " << m_maxUsage << std::endl;
		output << std::endl;
		output.close();
	}

private:

	PoolManager() {
		OTSYS_THREAD_LOCKVARINIT(poolLock);
		restStats();
	}

	PoolManager(const PoolManager&);
	const PoolManager& operator=(const PoolManager&);

	uint32_t m_nAllocate;
	uint32_t m_nDeallocate;
	uint32_t m_maxUsage;
	int32_t m_usage;
	uint32_t m_allocatedSize;
	uint32_t m_allocatedSize2;

	OTSYS_THREAD_LOCKVAR poolLock;
	static PoolManager m_instance;
};


#endif

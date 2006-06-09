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


#ifndef __OTSERV_RSA_H__
#define __OTSERV_RSA_H__

#include "otsystem.h"

#ifdef __RSA_MIRACL__
#include <big.h>
#else //default GMP
#include "gmp.h"
#endif

class RSA{
public:
	static RSA* getInstance();	
	void setKey(char* p, char* q, char* d);
	bool decrypt(char* msg,long size);
	
protected:
	RSA();
	~RSA();
	
	static RSA* instance;
	
	bool m_keySet;
	
	OTSYS_THREAD_LOCKVAR rsaLock;
	
	#ifdef __RSA_MIRACL__
	Big m_p, m_q, m_u, m_d, m_dp, m_dq;
	#else //default GMP
	mpz_t m_p, m_q, m_u, m_d, m_dp, m_dq;
	#endif
};

#endif

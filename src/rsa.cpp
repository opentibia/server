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
#include "otpch.h"
#include "rsa.h"

#include <iostream>

RSA::RSA()
{
  m_keySet = false;
  mpz_init2(m_p, 1024);
  mpz_init2(m_q, 1024);
  mpz_init(m_n);
  mpz_init2(m_d, 1024);
  mpz_init(m_e);
}

RSA::~RSA()
{
  mpz_clear(m_p);
  mpz_clear(m_q);
  mpz_clear(m_n);
  mpz_clear(m_d);
  mpz_clear(m_e);
}

bool RSA::setKey(const std::string& file)
{
  //loads p,q and d from a file
  FILE* f = fopen(file.c_str(), "r");
  if(!f){
    return false;
  }

  char p[513], q[513];
  if(!fgets(p, 513, f) || !fgets(q, 513, f)){
    fclose(f);
    return false;
  }
  
  fclose(f);

  setKey(p, q);
  return true;
}

void RSA::setKey(const char* p, const char* q)
{
  boost::recursive_mutex::scoped_lock lockClass(rsaLock);

  mpz_set_str(m_p, p, 10);
  mpz_set_str(m_q, q, 10);
  
  // e = 65537
  mpz_set_ui(m_e, 65537);
  
  // n = p * q
  mpz_mul(m_n, m_p, m_q);
  
  // TODO, since n is too small we won't be able to decode big numbers.
  //if(mpz_sizeinbase(m_n, 2) <= 1024){
  //  std::cout << " WARNING: 2^1023 < n < 2^1024 ";
  //}
  
  // d = e^-1 mod (p - 1)(q - 1)
  mpz_t p_1, q_1, pq_1;
  mpz_init2(p_1, 1024);
  mpz_init2(q_1, 1024);
  mpz_init2(pq_1, 1024);
  
  mpz_sub_ui(p_1, m_p, 1);
  mpz_sub_ui(q_1, m_q, 1);
  
  // pq_1 = (p -1)(q - 1)
  mpz_mul(pq_1, p_1, q_1);
  
  // m_d = m_e^-1 mod (p - 1)(q - 1)
  mpz_invert(m_d, m_e, pq_1);
  
  mpz_clear(p_1);
  mpz_clear(q_1);
  mpz_clear(pq_1);
}

bool RSA::encrypt(char* msg)
{
  boost::recursive_mutex::scoped_lock lockClass(rsaLock);
  
  mpz_t plain, c;
  mpz_init2(plain, 1024);
  mpz_init2(c, 1024);

  mpz_import(plain, 128, 1, 1, 0, 0, msg);
  
  // c = m^e mod n
  mpz_powm(c, plain, m_e, m_n);
  
  size_t count = (mpz_sizeinbase(c, 2) + 7)/8;
  
  memset(msg, 0, 128 - count);
  mpz_export(&msg[128 - count], NULL, 1, 1, 0, 0, c);

  mpz_clear(c);
  mpz_clear(plain);

  return true;
}

bool RSA::decrypt(char* msg)
{
  boost::recursive_mutex::scoped_lock lockClass(rsaLock);

  mpz_t c, m;
  mpz_init2(c, 1024);
  mpz_init2(m, 1024);

  mpz_import(c, 128, 1, 1, 0, 0, msg);
  
  // m = c^d mod n
  mpz_powm(m, c, m_d, m_n);
  
  size_t count = (mpz_sizeinbase(m, 2) + 7)/8;
  
  memset(msg, 0, 128 - count);
  mpz_export(&msg[128 - count], NULL, 1, 1, 0, 0, m);

  mpz_clear(c);
  mpz_clear(m);

  return true;
}

int32_t RSA::getKeySize()
{
  size_t count = (mpz_sizeinbase(m_n, 2) + 7)/8;
  int32_t a = count/128;
  return a*128;
}

void RSA::getPublicKey(char* buffer)
{
  size_t count = (mpz_sizeinbase(m_n, 2) + 7)/8;
  memset(buffer, 0, 128 - count);
  mpz_export(&buffer[128 - count], NULL, 1, 1, 0, 0, m_n);
}

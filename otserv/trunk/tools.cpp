//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Various functions.
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
// $Id$
//////////////////////////////////////////////////////////////////////
// $Log$
// Revision 1.1  2002/04/05 18:56:11  acrimon
// Adding a file class.
//
//////////////////////////////////////////////////////////////////////

#include <stdio.h>
//#include <pthread.h>

void hexdump(unsigned char *data, int len) {
  int i;
  for (; len > 0; data += 16, len -= 16) {
    for (i = 0; i < 16 && i < len; i++)
      fprintf(stderr, "%02x ", data[i]);
    for (; i < 16; i++)
      fprintf(stderr, "   ");
    fprintf(stderr, " ");
    for (i = 0; i < 16 && i < len; i++)
      fprintf(stderr, "%c", (data[i] & 0x70) < 32 ? '·' : data[i]);
    fprintf(stderr, "\n");
  }
}

#if 0
pthread_t *detach(void *(*fn)(void *), void *arg) {
  pthread_t *thread = new pthread_t();
  if (pthread_create(thread, NULL, fn, arg))
    perror("pthread");
  return thread;
}
#endif

char upchar(char c) {
  if (c >= 'a' && c <= 'z')
    return c - 'a' + 'A';
  else if (c == 'ä')
    return 'Ä';
  else if (c == 'ö')
    return 'Ö';
  else if (c == 'ü')
    return 'Ü';
  else
    return c;
}

void upper(char *upstr, char *str) {
  for (; *str; str++, upstr++)
    *upstr = upchar(*str);
  *upstr = '\0';
}

void upper(char *upstr, char *str, int n) {
  for (; *str && n; str++, upstr++, n--)
    *upstr = upchar(*str);
  if (n) *upstr = '\0';
}

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
// Revision 1.3  2002/05/28 13:55:57  shivoc
// some minor changes
//
// Revision 1.2  2002/04/05 20:02:23  acrimon
// *** empty log message ***
//
// Revision 1.1  2002/04/05 18:56:11  acrimon
// Adding a file class.
//
//////////////////////////////////////////////////////////////////////

#include <stdio.h>
//#include <pthread.h>

//////////////////////////////////////////////////
// dump a part of the memory to stderr.
void hexdump(unsigned char *_data, int _len) {
    int i;
    for (; _len > 0; _data += 16, _len -= 16) {
        for (i = 0; i < 16 && i < _len; i++)
            fprintf(stderr, "%02x ", _data[i]);
        for (; i < 16; i++)
            fprintf(stderr, "   ");
        fprintf(stderr, " ");
        for (i = 0; i < 16 && i < _len; i++)
            fprintf(stderr, "%c", (_data[i] & 0x70) < 32 ? '·' : _data[i]);
        fprintf(stderr, "\n");
    }
}

#if 0
//////////////////////////////////////////////////
// Enable asynchronous function calls.
// This function does not wait for return of the called function;
// instead, this function returns immediately.
// The called function must be of type void *fn(void *).
// You can use the pointer to the function for anything you want to.
// Return: a thread handle.
pthread_t *detach(void *(*_fn)(void *), void *_arg) {
    pthread_t *thread = new pthread_t();
    if (pthread_create(thread, NULL, _fn, _arg))
        perror("pthread");
    return thread;
}
#endif

//////////////////////////////////////////////////
// Upcase a char.
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

//////////////////////////////////////////////////
// Upcase a 0-terminated string.
void upper(char *upstr, char *str) {
    for (; *str; str++, upstr++)
        *upstr = upchar(*str);
    *upstr = '\0';
}

//////////////////////////////////////////////////
// Upcase a 0-terminated string, but at most n chars.
void upper(char *upstr, char *str, int n) {
    for (; *str && n; str++, upstr++, n--)
        *upstr = upchar(*str);
    if (n) *upstr = '\0';
}

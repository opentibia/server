/* OpenTibia - an opensource roleplaying game
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef __definitions_h
#define __definitions_h

// constants...
// width and length of the map...
const unsigned TC_MSIZEX = 1000;
const unsigned TC_MSIZEY = 1000;

// how much we shift the startposition away from 0,0
// 0,0 would cause massive client crashes...
const unsigned TC_MINX = 1000;
const unsigned TC_MINY = 1000;

// and we define our maximum X/Y...
const unsigned TC_MAXX = TC_MINX+TC_MSIZEX;
const unsigned TC_MAXY = TC_MINY+TC_MSIZEY;

#ifdef __LINUX__
typedef int SOCKET;
#endif

#endif // __definitions_h

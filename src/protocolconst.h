//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Defines limits of the protocol
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

#ifndef __OTSERV_PROTOCOL_CONST_H__
#define __OTSERV_PROTOCOL_CONST_H__

// This files defines limits of the protocol


// The maximum size of a single packet sent over the network
// This also indirectly limits the size of writeables etc.
#define NETWORKMESSAGE_MAXSIZE 15340

// This defines the range of how many tiles to send to the client
// This cannot change without recoding the client (ie: never without OTClient)
const int Map_maxClientViewportX = 8;
const int Map_maxClientViewportY = 6;

// The size of the viewport, this is how far creatures will react / see other creatures
// (IE limit of NPC AI)
// These values *must* be larger than the client viewport
const int Map_maxViewportX = 11;
const int Map_maxViewportY = 11;

#endif

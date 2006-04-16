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

#ifndef __NETWORK_MESSAGE_H__
#define __NETWORK_MESSAGE_H__


#include "definitions.h"
#include "otsystem.h"
#include "const76.h"

class Creature;
class Player;
class Item;
class Position;


class NetworkMessage
{
public:
  // constructor/destructor
  NetworkMessage();
  virtual ~NetworkMessage();


  // resets the internal buffer to an empty message
  void Reset();


  // socket functions
  bool ReadFromSocket(SOCKET socket);
  bool WriteToSocket(SOCKET socket);


  // simply read functions for incoming message
  unsigned char  GetByte();
  unsigned short GetU16();
  unsigned short GetItemId();
  unsigned int   GetU32();
  std::string    GetString();
  std::string	 GetRaw();
  Position       GetPosition();


  // skips count unknown/unused bytes in an incoming message
  void SkipBytes(int count);


  // simply write functions for outgoing message
  void AddByte(unsigned char  value);
  void AddU16 (unsigned short value);
  void AddU32 (unsigned int   value);

  void AddString(const std::string &value);
  void AddString(const char* value);


  // write functions for complex types
  void AddPosition(const Position &pos);
	void AddItem(unsigned short id, unsigned char count);
	void AddItem(const Item *item);
	void AddItemId(const Item *item);
  void AddCreature(const Creature *creature, bool known, unsigned int remove);

  int getMessageLength(){
      return m_MsgSize;
      }

	void JoinMessages(NetworkMessage &add);

	
protected:
  inline bool canAdd(int size){
    return (size + m_ReadPos < NETWORKMESSAGE_MAXSIZE - 16);
  };
  int m_MsgSize;
  int m_ReadPos;

  unsigned char m_MsgBuf[NETWORKMESSAGE_MAXSIZE];
};


#endif // #ifndef __NETWORK_MESSAGE_H__

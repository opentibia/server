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

#include <string>
#include <iostream>
#include <sstream>

#include "networkmessage.h"

#include "item.h"
#include "container.h"
#include "creature.h"
#include "player.h"

#include "position.h"



/******************************************************************************/

NetworkMessage::NetworkMessage()
{
  Reset();
}

NetworkMessage::~NetworkMessage()
{
}


/******************************************************************************/

void NetworkMessage::Reset()
{
  m_MsgSize = 0;
  m_ReadPos = 2;  
}


/******************************************************************************/

bool NetworkMessage::ReadFromSocket(SOCKET socket)
{
	// just read the size to avoid reading 2 messages at once
	m_MsgSize = recv(socket, (char*)m_MsgBuf, 2, 0);
	
	// for now we expect 2 bytes at once, it should not be splitted
	int datasize = m_MsgBuf[0] | m_MsgBuf[1] >> 8;
	if ((m_MsgSize != 2) || (datasize > NETWORKMESSAGE_MAXSIZE-2))
	{
		int errnum;
#if defined WIN32 || defined __WINDOWS__
		errnum = ::WSAGetLastError();
				if(errnum == EWOULDBLOCK) {
			m_MsgSize = 0;
			return true;
		}
#else
		errnum = errno;
#endif

		Reset();
		return false;
	}

	// read the real data
	m_MsgSize += recv(socket, (char*)m_MsgBuf+2, datasize, 0);

	// we got something unexpected/incomplete
	if ((m_MsgSize <= 2) || ((m_MsgBuf[0] | m_MsgBuf[1] >> 8) != m_MsgSize-2))
	{
		Reset();
		return false;
	}

	// ok, ...reading starts after the size
	m_ReadPos = 2;

	return true;
}


bool NetworkMessage::WriteToSocket(SOCKET socket)
{
  if (m_MsgSize == 0)
    return true;

  m_MsgBuf[0] = (unsigned char)(m_MsgSize);
  m_MsgBuf[1] = (unsigned char)(m_MsgSize >> 8);
  
	bool ret = true;
  int sendBytes = 0;
	int flags;

#if defined WIN32 || defined __WINDOWS__
	// Set the socket I/O mode; iMode = 0 for blocking; iMode != 0 for non-blocking
  unsigned long mode = 1;
  ioctlsocket(socket, FIONBIO, &mode);

	flags = 0;
#else
	flags = MSG_DONTWAIT;
#endif

  do
  {
    int b = send(socket, (char*)m_MsgBuf+sendBytes, std::min(m_MsgSize-sendBytes+2, 1000), flags);

		if (b <= 0) {
			ret = false;
			break;
		}

    sendBytes += b;
  }
  while (sendBytes < m_MsgSize+2);

#if defined WIN32 || defined __WINDOWS__
  mode = 0;
  ioctlsocket(socket, FIONBIO, &mode);
#endif

  return ret;
}


/******************************************************************************/


unsigned char NetworkMessage::GetByte()
{
  return m_MsgBuf[m_ReadPos++];
}


unsigned short NetworkMessage::GetU16()
{
  unsigned short v = ((m_MsgBuf[m_ReadPos]) | (m_MsgBuf[m_ReadPos+1] << 8));
  m_ReadPos += 2;
  return v;
}

unsigned int NetworkMessage::GetU32()
{
  unsigned int v = ((m_MsgBuf[m_ReadPos  ]      ) | (m_MsgBuf[m_ReadPos+1] <<  8) |
                    (m_MsgBuf[m_ReadPos+2] << 16) | (m_MsgBuf[m_ReadPos+3] << 24));
  m_ReadPos += 4;
  return v;
}


std::string NetworkMessage::GetString()
{
  int stringlen = GetU16();
  if (stringlen >= (16384 - m_ReadPos))
	  return std::string();

  char* v = (char*)(m_MsgBuf+m_ReadPos);
  m_ReadPos += stringlen;
  return std::string(v, stringlen);
}

std::string NetworkMessage::GetRaw(){
  int stringlen = m_MsgSize- m_ReadPos;
  if (stringlen >= (16384 - m_ReadPos))
	  return std::string();

  char* v = (char*)(m_MsgBuf+m_ReadPos);
  m_ReadPos += stringlen;
  return std::string(v, stringlen);
}

Position NetworkMessage::GetPosition() {
  Position pos;
  pos.x = GetU16();
  pos.y = GetU16();
  pos.z = GetByte();
  return pos;
}


void NetworkMessage::SkipBytes(int count)
{
  m_ReadPos += count;
}


/******************************************************************************/


void NetworkMessage::AddByte(unsigned char value)
{
  if(!canAdd(1))
    return;
  m_MsgBuf[m_ReadPos++] = value;
  m_MsgSize++;
}


void NetworkMessage::AddU16(unsigned short value)
{
  if(!canAdd(2))
    return;
  m_MsgBuf[m_ReadPos++] = (unsigned char)(value);
  m_MsgBuf[m_ReadPos++] = (unsigned char)(value >> 8);
  m_MsgSize += 2;
}


void NetworkMessage::AddU32(unsigned int value)
{
  if(!canAdd(4))
    return;
  m_MsgBuf[m_ReadPos++] = (unsigned char)(value);
  m_MsgBuf[m_ReadPos++] = (unsigned char)(value >>  8);
  m_MsgBuf[m_ReadPos++] = (unsigned char)(value >> 16);
  m_MsgBuf[m_ReadPos++] = (unsigned char)(value >> 24);
  m_MsgSize += 4;
}


void NetworkMessage::AddString(const std::string &value)
{
  AddString(value.c_str());
}


void NetworkMessage::AddString(const char* value)
{
  unsigned short stringlen = (unsigned short) strlen(value);
  if(!canAdd(stringlen+2))
    return;
  AddU16(stringlen);
  strcpy((char*)m_MsgBuf + m_ReadPos, value);
  m_ReadPos += stringlen;
  m_MsgSize += stringlen;
}


/******************************************************************************/


void NetworkMessage::AddPosition(const Position &pos)
{
  AddU16(pos.x);
  AddU16(pos.y);
  AddByte(pos.z);
}


void NetworkMessage::AddItem(unsigned short id, unsigned char count)
{
	AddU16(id);

	const ItemType &it = Item::items[id];
	if(it.stackable || it.multitype || it.fluidcontainer)
		AddByte(count);
}

void NetworkMessage::AddItem(const Item *item)
{
  AddU16(item->getID());

  if (item->isStackable() || item->isMultiType() || item->isFluidContainer())
    AddByte(item->getItemCountOrSubtype());
}

void NetworkMessage::JoinMessages(NetworkMessage &add){
	if(!canAdd(add.m_MsgSize))
      return;
	memcpy(&m_MsgBuf[m_ReadPos],&(add.m_MsgBuf[2]),add.m_MsgSize);
	m_ReadPos += add.m_MsgSize;
  	m_MsgSize += add.m_MsgSize;
}

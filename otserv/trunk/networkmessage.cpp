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
#else
	errnum = errno;
#endif

		if(errnum == EWOULDBLOCK) {
			m_MsgSize = 0;
			return true;
		}

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
  m_MsgBuf[m_ReadPos++] = value;
  m_MsgSize++;
}


void NetworkMessage::AddU16(unsigned short value)
{
  m_MsgBuf[m_ReadPos++] = (unsigned char)(value);
  m_MsgBuf[m_ReadPos++] = (unsigned char)(value >> 8);
  m_MsgSize += 2;
}


void NetworkMessage::AddU32(unsigned int value)
{
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


void NetworkMessage::AddItem(const Item *item)
{
  AddU16(item->getID());

  if (item->isStackable() || item->isMultiType())
    AddByte(item->getItemCountOrSubtype());
}

/******************************************************************************/
////////////////////////// Will be moved to protocol ///////////////////////////
void NetworkMessage::AddTextMessage(MessageClasses mclass, const char* message)
{
  AddByte(0xB4);
  AddByte(mclass);
  AddString(message);
}


void NetworkMessage::AddAnimatedText(const Position &pos, unsigned char color, std::string text)
{
#ifdef __DEBUG__
	if(text.length() == 0) {
		std::cout << "Warning: 0-Length string in AddAnimatedText()" << std::endl;
	}
#endif

  AddByte(0x84); 
  AddPosition(pos);
  AddByte(color);
  AddString(text);
}



void NetworkMessage::AddMagicEffect(const Position &pos, unsigned char type)
{
  AddByte(0x83);
  AddPosition(pos);
  AddByte(type);
}


void NetworkMessage::AddDistanceShoot(const Position &from, const Position &to, unsigned char type)
{
  AddByte(0x85); 
  AddPosition(from);
  AddPosition(to);
  AddByte(type);
}


void NetworkMessage::AddCreature(const Creature *creature, bool known, unsigned int remove)
{
  if (known)
  {
    AddByte(0x62);
    AddByte(0x00);
    AddU32(creature->getID());
  }
  else
  {
    AddByte(0x61);
    AddByte(0x00);
		//AddU32(0);
	  AddU32(remove);
    AddU32(creature->getID());
    AddString(creature->getName());
  }

  AddByte(creature->health*100/creature->healthmax);
  AddByte((unsigned char)creature->getDirection());

  AddByte(creature->looktype);
  AddByte(creature->lookhead);
  AddByte(creature->lookbody);
  AddByte(creature->looklegs);
  AddByte(creature->lookfeet);

  AddByte(0x00); //
  AddByte(0xDC); // 

  AddU16(creature->getSpeed());
  
  AddByte(0x00); //
  AddByte(0x00); //
}


void NetworkMessage::AddPlayerStats(const Player *player)
{
  AddByte(0xA0);
  AddU16(player->health);
  AddU16(player->healthmax);
  AddU16(player->cap);
  AddU32(player->experience);
  AddByte(player->level);
  AddByte(0x07); //new
  AddU16(player->mana);
  AddU16(player->manamax);
  AddByte(player->maglevel);
  AddByte(0x20); //new
}

void NetworkMessage::AddPlayerSkills(const Player *player)
{
	AddByte(0xA1);

  AddByte(player->skills[SKILL_FIST  ][SKILL_LEVEL]);
  AddByte(player->skills[SKILL_FIST  ][SKILL_PERCENT]);
  AddByte(player->skills[SKILL_CLUB  ][SKILL_LEVEL]);
  AddByte(player->skills[SKILL_CLUB  ][SKILL_PERCENT]);
  AddByte(player->skills[SKILL_SWORD ][SKILL_LEVEL]);
  AddByte(player->skills[SKILL_SWORD  ][SKILL_PERCENT]);
  AddByte(player->skills[SKILL_AXE   ][SKILL_LEVEL]);
  AddByte(player->skills[SKILL_AXE  ][SKILL_PERCENT]);
  AddByte(player->skills[SKILL_DIST  ][SKILL_LEVEL]);
  AddByte(player->skills[SKILL_DIST  ][SKILL_PERCENT]);
  AddByte(player->skills[SKILL_SHIELD][SKILL_LEVEL]);
  AddByte(player->skills[SKILL_SHIELD  ][SKILL_PERCENT]);
  AddByte(player->skills[SKILL_FISH  ][SKILL_LEVEL]);
  AddByte(player->skills[SKILL_FISH  ][SKILL_PERCENT]);
}


void NetworkMessage::AddPlayerInventoryItem(const Player *player, int item)
{
  if (player->getItem(item) == NULL)
  {
    AddByte(0x79);
    AddByte(item);
  }
  else
  {
    AddByte(0x78);
    AddByte(item);
    AddItem(player->getItem(item));
  }
}


void NetworkMessage::AddCreatureSpeak(const Creature *creature, unsigned char type, std::string text, unsigned short channelId)
{
  AddByte(0xAA);
  AddString(creature->getName());
  AddByte(type);
  if (type <= 3 || type == 16 || type == SPEAK_MONSTER)
    AddPosition(creature->pos);
  if(type == 5)
    AddU16(channelId);
  AddString(text);
}

void NetworkMessage::AddCreatureHealth(const Creature *creature)
{
  AddByte(0x8C);
  AddU32(creature->getID());
  AddByte(creature->health*100/creature->healthmax);
}
/////////////////////////////////////////////////////////////////////////////////
void NetworkMessage::JoinMessages(NetworkMessage &add){
	memcpy(&m_MsgBuf[m_ReadPos],&(add.m_MsgBuf[2]),add.m_MsgSize);
	m_ReadPos += add.m_MsgSize;
  	m_MsgSize += add.m_MsgSize;
}

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

#include "networkmessage.h"
#include "creature.h"
#include "position.h"
#include "item.h"

NetworkMessage::NetworkMessage()
{
	Reset();
}

NetworkMessage::~NetworkMessage()
{

}

uint8_t NetworkMessage::GetByte()
{
	return m_MsgBuf[m_ReadPos++];
}

uint16_t NetworkMessage::GetU16()
{
	uint16_t v = *(uint16_t*)(m_MsgBuf + m_ReadPos);
	m_ReadPos += 2;
	return v;
}

uint16_t NetworkMessage::GetSpriteId()
{
	return GetU16();
}

uint32_t NetworkMessage::GetU32()
{
	uint32_t v = *(uint32_t*)(m_MsgBuf + m_ReadPos);
	m_ReadPos += 4;
	return v;
}

uint32_t NetworkMessage::PeekU32() const
{
	uint32_t v = *(uint32_t*)(m_MsgBuf + m_ReadPos);
	return v;
}

uint64_t NetworkMessage::GetU64() const
{
	uint64_t v = *(uint64_t*)(m_MsgBuf + m_ReadPos);
	return v;
}

std::string NetworkMessage::GetString()
{
	uint16_t stringlen = GetU16();
	if(stringlen >= (NETWORKMESSAGE_MAXSIZE - m_ReadPos))
		return std::string();

	char* v = (char*)(m_MsgBuf + m_ReadPos);
	m_ReadPos += stringlen;
	return std::string(v, stringlen);
}

std::string NetworkMessage::GetRaw()
{
	uint16_t stringlen = m_MsgSize- m_ReadPos;
	if(stringlen >= (NETWORKMESSAGE_MAXSIZE - m_ReadPos))
		return std::string();

	char* v = (char*)(m_MsgBuf + m_ReadPos);
	m_ReadPos += stringlen;
	return std::string(v, stringlen);
}

Position NetworkMessage::GetPosition()
{
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

void NetworkMessage::AddByte(uint8_t value)
{
	if(!canAdd(1))
		return;
	m_MsgBuf[m_ReadPos++] = value;
	m_MsgSize++;
}

void NetworkMessage::AddU16(uint16_t value)
{
	if(!canAdd(2))
		return;
	*(uint16_t*)(m_MsgBuf + m_ReadPos) = value;
	m_ReadPos += 2; m_MsgSize += 2;
}

void NetworkMessage::AddU32(uint32_t value)
{
	if(!canAdd(4))
		return;
	*(uint32_t*)(m_MsgBuf + m_ReadPos) = value;
	m_ReadPos += 4; m_MsgSize += 4;
}

void NetworkMessage::AddU64(uint64_t value)
{
	if(!canAdd(8))
		return;
	*(uint64_t*)(m_MsgBuf + m_ReadPos) = value;
	m_ReadPos += 8; m_MsgSize += 8;
}

void NetworkMessage::AddString(const char* value)
{
	uint32_t stringlen = (uint32_t)strlen(value);
	if(!canAdd(stringlen+2) || stringlen > 8192)
		return;

	AddU16(stringlen);
	strcpy((char*)(m_MsgBuf + m_ReadPos), value);
	m_ReadPos += stringlen;
	m_MsgSize += stringlen;
}

void NetworkMessage::AddBytes(const char* bytes, uint32_t size)
{
	if(!canAdd(size) || size > 8192)
		return;

	memcpy(m_MsgBuf + m_ReadPos, bytes, size);
	m_ReadPos += size;
	m_MsgSize += size;
}

void NetworkMessage::AddPaddingBytes(uint32_t n)
{
	if(!canAdd(n))
		return;

	memset((void*)&m_MsgBuf[m_ReadPos], 0x33, n);
	m_MsgSize = m_MsgSize + n;
}

void NetworkMessage::AddString(const std::string &value)
{
	AddString(value.c_str());
}

void NetworkMessage::AddPosition(const Position& pos)
{
	AddU16(pos.x);
	AddU16(pos.y);
	AddByte(pos.z);
}

void NetworkMessage::AddItem(uint16_t id, uint8_t count)
{
	const ItemType &it = Item::items[id];

	AddU16(it.clientId);

	if(it.stackable){
		AddByte(count);
	}
	else if(it.isSplash() || it.isFluidContainer()){
		uint32_t fluidIndex = count % 8;
		AddByte(fluidMap[fluidIndex].value());
	}
}

void NetworkMessage::AddItem(const Item* item)
{
	const ItemType &it = Item::items[item->getID()];

	AddU16(it.clientId);

	if(it.stackable){
		AddByte(item->getSubType());
	}
	else if(it.isSplash() || it.isFluidContainer()){
		uint32_t fluidIndex = item->getSubType() % 8;
		AddByte(fluidMap[fluidIndex].value());
	}
}

void NetworkMessage::AddItemId(const Item *item)
{
	const ItemType &it = Item::items[item->getID()];
	AddU16(it.clientId);
}

void NetworkMessage::AddItemId(uint16_t itemId)
{
	const ItemType &it = Item::items[itemId];
	AddU16(it.clientId);
}

int32_t NetworkMessage::getMessageLength() const
{
	return m_MsgSize;
}

void NetworkMessage::setMessageLength(int32_t newSize)
{
	m_MsgSize = newSize;
}

int32_t NetworkMessage::getReadPos() const
{
	return m_ReadPos;
}

void NetworkMessage::setReadPos(int32_t pos)
{
	m_ReadPos = pos;
}

int32_t NetworkMessage::decodeHeader()
{
	int32_t size = (int32_t)(m_MsgBuf[0] | m_MsgBuf[1] << 8);
	m_MsgSize = size;
	return size;
}

char* NetworkMessage::getBuffer()
{
	return (char*)&m_MsgBuf[0];
}

char* NetworkMessage::getBodyBuffer()
{
	m_ReadPos = 2; return (char*)&m_MsgBuf[header_length];
}

void NetworkMessage::Reset()
{
	m_MsgSize = 0;
	m_ReadPos = 8;
}

bool NetworkMessage::canAdd(uint32_t size) const
{
	return size + m_ReadPos < max_body_length;
}

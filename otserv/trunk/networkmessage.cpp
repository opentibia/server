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
#include "rsa.h"


long fluidMap[] = {FLUID_EMPTY_1, FLUID_BLUE_1, FLUID_RED_1, FLUID_BROWN_1, 
	FLUID_GREEN_1, FLUID_YELLOW_1, FLUID_WHITE_1, FLUID_PURPLE_1};

/******************************************************************************/

NetworkMessage::NetworkMessage()
{
	m_encryptionEnabled = false;
	m_keyset = false;
	Reset();
}

NetworkMessage::~NetworkMessage()
{
}


/******************************************************************************/

void NetworkMessage::Reset()
{
	m_MsgSize = 0;
	m_ReadPos = 4;
}


/******************************************************************************/

bool NetworkMessage::ReadFromSocket(SOCKET socket)
{
	// just read the size to avoid reading 2 messages at once
	m_MsgSize = recv(socket, (char*)m_MsgBuf, 2, 0);
	
	// for now we expect 2 bytes at once, it should not be splitted
	int datasize = m_MsgBuf[0] | m_MsgBuf[1] << 8;
	if((m_MsgSize != 2) || (datasize > NETWORKMESSAGE_MAXSIZE-2)){
		int errnum;
#if defined WIN32 || defined __WINDOWS__
		errnum = ::WSAGetLastError();
		if(errnum == EWOULDBLOCK){
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
	if((m_MsgSize <= 2) || ((m_MsgBuf[0] | m_MsgBuf[1] << 8) != m_MsgSize-2)){
		Reset();
		return false;
	}
	m_ReadPos = 2;
	//decrypt
	if(m_encryptionEnabled){
		if(!m_keyset){
			std::cout << "Failure: [NetworkMessage::ReadFromSocket]. Key not set" << std::endl;
			return false;
		}
		if((m_MsgSize - 2) % 8 != 0){
			std::cout << "Failure: [NetworkMessage::ReadFromSocket]. Not valid encrypted message size" << std::endl;
			return false;
		}
		XTEA_decrypt();
		int tmp = GetU16();
		if(tmp > m_MsgSize - 4){
			std::cout << "Failure: [NetworkMessage::ReadFromSocket]. Not valid unencrypted message size" << std::endl;
			return false;
		}
		m_MsgSize = tmp;
	}
	return true;
}


bool NetworkMessage::WriteToSocket(SOCKET socket)
{
	if (m_MsgSize == 0)
		return true;

	m_MsgBuf[2] = (unsigned char)(m_MsgSize);
	m_MsgBuf[3] = (unsigned char)(m_MsgSize >> 8);
  
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
	int retry = 0;
	
	int start;
	if(m_encryptionEnabled){
		if(!m_keyset){
			std::cout << "Failure: [NetworkMessage::ReadFromSocket]. Key not set" << std::endl;
			return false;
		}
		start = 0;
		XTEA_encrypt();
	}
	else{
		start = 2;
	}
	
  	do{
    	int b = send(socket, (char*)m_MsgBuf+sendBytes+start, std::min(m_MsgSize-sendBytes+2, 1000), flags);
		if(b <= 0){
#if defined WIN32 || defined __WINDOWS__
			int errnum = ::WSAGetLastError();
			if(errnum == EWOULDBLOCK){
				b = 0;
				OTSYS_SLEEP(10);
				retry++;
				if(retry == 10){
					ret = false;
					break;
				}
			}
			else
#endif
			{
				ret = false;
				break;
			}
		}
    	sendBytes += b;
	}while(sendBytes < m_MsgSize+2);

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

unsigned short NetworkMessage::GetItemId()
{
	unsigned short v = this->GetU16();
	return Item::items.reverseLookUp(v);
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

std::string NetworkMessage::GetRaw()
{
	int stringlen = m_MsgSize- m_ReadPos;
	if (stringlen >= (16384 - m_ReadPos))
		return std::string();

	char* v = (char*)(m_MsgBuf+m_ReadPos);
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
	unsigned long stringlen = (unsigned long) strlen(value);
	if(!canAdd(stringlen+2) || stringlen > 8192)
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
	const ItemType &it = Item::items[id];

	AddU16(it.clientId);

	if(it.stackable || it.isRune()){
		AddByte(count);
	}
	else if(it.isSplash() || it.isFluidContainer()){
		long fluidIndex = count % 8;
		AddByte(fluidMap[fluidIndex]);
	}
	
}

void NetworkMessage::AddItem(const Item *item)
{
	const ItemType &it = Item::items[item->getID()];

	AddU16(it.clientId);

	if(it.stackable || it.isRune()){
    	AddByte(item->getItemCountOrSubtype());
	}
	else if(it.isSplash() || it.isFluidContainer()){
		long fluidIndex = item->getItemCountOrSubtype() % 8;
		AddByte(fluidMap[fluidIndex]);
	}
}

void NetworkMessage::AddItemId(const Item *item)
{
	const ItemType &it = Item::items[item->getID()];

	AddU16(it.clientId);
}

void NetworkMessage::JoinMessages(NetworkMessage &add)
{
	if(!canAdd(add.m_MsgSize))
		return;
	memcpy(&m_MsgBuf[m_ReadPos],&(add.m_MsgBuf[4]),add.m_MsgSize);
	m_ReadPos += add.m_MsgSize;
  	m_MsgSize += add.m_MsgSize;
}

void NetworkMessage::setEncryptionState(bool state)
{
	m_encryptionEnabled = state;
}

void NetworkMessage::setEncryptionKey(const unsigned long* key)
{
	memcpy(m_key, key, 16);
	m_keyset = true;
}


void NetworkMessage::XTEA_encrypt()
{
	unsigned long k[4];
	k[0] = m_key[0]; k[1] = m_key[1]; k[2] = m_key[2]; k[3] = m_key[3];
	
	//add bytes until reach 8 multiple
	unsigned long n;
	if(((m_MsgSize + 2) % 8) != 0){
		n = 8 - ((m_MsgSize + 2) % 8);
		memset((void*)&m_MsgBuf[m_ReadPos], 0, n);
		m_MsgSize = m_MsgSize + n;
	}
	
	unsigned long read_pos = 0;
	unsigned long* buffer = (unsigned long*)&m_MsgBuf[2];
	while(read_pos < m_MsgSize/4){
		unsigned long v0 = buffer[read_pos], v1 = buffer[read_pos + 1];
		unsigned long delta = 0x61C88647;
		unsigned long sum = 0;
		
		for(unsigned long i = 0; i<32; i++) {
			v0 += ((v1 << 4 ^ v1 >> 5) + v1) ^ (sum + k[sum & 3]);
			sum -= delta;
			v1 += ((v0 << 4 ^ v0 >> 5) + v0) ^ (sum + k[sum>>11 & 3]);
		}
		buffer[read_pos] = v0; buffer[read_pos + 1] = v1;
		read_pos = read_pos + 2;
	}
	m_MsgSize = m_MsgSize + 2;
	m_MsgBuf[0] = (unsigned char)(m_MsgSize);
	m_MsgBuf[1] = (unsigned char)(m_MsgSize >> 8);
	
}

void NetworkMessage::XTEA_decrypt()
{
	unsigned long k[4];
	k[0] = m_key[0]; k[1] = m_key[1]; k[2] = m_key[2]; k[3] = m_key[3];
	
	unsigned long* buffer = (unsigned long*)&m_MsgBuf[2];
	unsigned long read_pos = 0;
	while(read_pos < m_MsgSize/4){
		unsigned long v0 = buffer[read_pos], v1 = buffer[read_pos + 1];
		unsigned long delta = 0x61C88647;
		unsigned long sum = 0xC6EF3720;
		
		for(unsigned long i = 0; i<32; i++) {
			v1 -= ((v0 << 4 ^ v0 >> 5) + v0) ^ (sum + k[sum>>11 & 3]);
			sum += delta;
			v0 -= ((v1 << 4 ^ v1 >> 5) + v1) ^ (sum + k[sum & 3]);
		}
		buffer[read_pos] = v0; buffer[read_pos + 1] = v1;
		read_pos = read_pos + 2;
	}
}

bool NetworkMessage::RSA_decrypt()
{
	if(m_MsgSize - m_ReadPos != 128){
		std::cout << "Warning: [NetworkMessage::RSA_decrypt()]. Not valid packet size" << std::endl;
		return false;
	}
	
	RSA* rsa = RSA::getInstance();
	if(!rsa->decrypt((char*)&m_MsgBuf[m_ReadPos], 128)){
		return false;
	}
	
	if(GetByte() != 0){
		std::cout << "Warning: [NetworkMessage::RSA_decrypt()]. First byte != 0" << std::endl;
		return false;
	}
	
	return true;
}

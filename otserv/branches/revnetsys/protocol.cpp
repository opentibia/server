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

#include "definitions.h"
#include "protocol.h"
#include "outputmessage.h"
#include "rsa.h"

void Protocol::onSendMessage(OutputMessage* msg)
{
	std::cout << "Protocol::onSendMessage" << std::endl;
	if(m_encryptionEnabled){
		std::cout << "Protocol::onSendMessage - encrypt" << std::endl;
		XTEA_encrypt(*msg);
	}
}

void Protocol::XTEA_encrypt(OutputMessage& msg)
{
	uint32_t k[4];
	k[0] = m_key[0]; k[1] = m_key[1]; k[2] = m_key[2]; k[3] = m_key[3];
	
	int32_t messageLength = msg.getMessageLength();
	//adding the size of unencrypted header
	messageLength = messageLength + 2;
	msg.setMessageLength(messageLength);
	
	//add bytes until reach 8 multiple
	uint32_t n;
	if((messageLength % 8) != 0){
		n = 8 - (messageLength % 8);
		msg.AddPaddingBytes(n);
	}
	
	int read_pos = 0;
	uint32_t* buffer = (uint32_t*)msg.getBodyBuffer();
	while(read_pos < messageLength/4){
		uint32_t v0 = buffer[read_pos], v1 = buffer[read_pos + 1];
		uint32_t delta = 0x61C88647;
		uint32_t sum = 0;
		
		for(int32_t i = 0; i<32; i++) {
			v0 += ((v1 << 4 ^ v1 >> 5) + v1) ^ (sum + k[sum & 3]);
			sum -= delta;
			v1 += ((v0 << 4 ^ v0 >> 5) + v0) ^ (sum + k[sum>>11 & 3]);
		}
		buffer[read_pos] = v0; buffer[read_pos + 1] = v1;
		read_pos = read_pos + 2;
	}
	msg.addCryptoHeader();
}

bool Protocol::XTEA_decrypt(NetworkMessage& msg)
{
	if((msg.getMessageLength() - 2) % 8 != 0){
		std::cout << "Failure: [Protocol::XTEA_decrypt]. Not valid encrypted message size" << std::endl;
		return false;
	}
	
	//
	uint32_t k[4];
	k[0] = m_key[0]; k[1] = m_key[1]; k[2] = m_key[2]; k[3] = m_key[3];
	
	uint32_t* buffer = (uint32_t*)msg.getBodyBuffer();
	int read_pos = 0;
	int32_t messageLength = msg.getMessageLength();
	while(read_pos < messageLength/4){
		uint32_t v0 = buffer[read_pos], v1 = buffer[read_pos + 1];
		uint32_t delta = 0x61C88647;
		uint32_t sum = 0xC6EF3720;
		
		for(int32_t i = 0; i<32; i++) {
			v1 -= ((v0 << 4 ^ v0 >> 5) + v0) ^ (sum + k[sum>>11 & 3]);
			sum += delta;
			v0 -= ((v1 << 4 ^ v1 >> 5) + v1) ^ (sum + k[sum & 3]);
		}
		buffer[read_pos] = v0; buffer[read_pos + 1] = v1;
		read_pos = read_pos + 2;
	}
	//
	
	int tmp = msg.GetU16();
	if(tmp > msg.getMessageLength() - 4){
		std::cout << "Failure: [Protocol::XTEA_decrypt]. Not valid unencrypted message size" << std::endl;
		return false;
	}
	
	msg.setMessageLength(tmp);
	return true;
}

bool Protocol::RSA_decrypt(RSA* rsa, NetworkMessage& msg)
{
	if(msg.getMessageLength() - msg.getReadPos() != 128){
		std::cout << "Warning: [Protocol::RSA_decrypt]. Not valid packet size" << std::endl;
		return false;
	}
	
	if(!rsa->decrypt((char*)(msg.getBuffer() + msg.getReadPos()), 128)){
		return false;
	}
	
	if(msg.GetByte() != 0){
		std::cout << "Warning: [Protocol::RSA_decrypt]. First byte != 0" << std::endl;
		return false;
	}
	
	return true;
}

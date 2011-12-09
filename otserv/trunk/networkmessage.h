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

#ifndef __OTSERV_NETWORK_MESSAGE_H__
#define __OTSERV_NETWORK_MESSAGE_H__

#include "definitions.h"
#include "otsystem.h"
#include "const.h"
#include <string>
#include <boost/shared_ptr.hpp>

class Item;
class Creature;
class Player;
class Position;
class RSA;

class NetworkMessage {
public:
	enum { header_length = 2 };
	enum { crypto_length = 4 };
	enum { xtea_multiple = 8 };
	enum { max_body_length = NETWORKMESSAGE_MAXSIZE - header_length - crypto_length - xtea_multiple };

	NetworkMessage();
	virtual ~NetworkMessage();

	// simply read functions for incoming message
	uint8_t GetByte();
	uint16_t GetU16();
	uint32_t GetU32();
	uint32_t PeekU32();

	uint16_t GetSpriteId();

	std::string GetString();
	std::string GetRaw();
	Position GetPosition();

	// skips count unknown/unused bytes in an incoming message
	void SkipBytes(const int& count);

	// simply write functions for outgoing message
	void AddByte(const uint8_t& value);
	void AddU16(const uint16_t& value);
	void AddU32(const uint32_t& value);
	void AddBytes(const char* bytes, const uint32_t& size);
	void AddPaddingBytes(const uint32_t& n);

	void AddString(const std::string& value);
	void AddString(const char* value);

	// write functions for complex types
	void AddPosition(const Position &pos);
	void AddItem(const uint16_t& id, const uint8_t& count);
	void AddItem(const Item *item);
	void AddItemId(const Item *item);
	void AddItemId(const uint16_t &itemId);
	void AddCreature(const Creature *creature, bool known, unsigned int remove);

	const int32_t& getMessageLength() const;
	void setMessageLength(const int32_t& newSize);
	const int32_t& getReadPos() const;
	void setReadPos(const int32_t& pos);

	int32_t decodeHeader();

	char* getBuffer();
	const char* getBuffer() const;
	char* getBodyBuffer();

#ifdef __TRACK_NETWORK__
	virtual void Track(std::string file, long line, std::string func) {};
	virtual void clearTrack() {};
#endif


protected:
	void Reset();
	bool canAdd(const uint32_t& size) const;

	int32_t m_MsgSize;
	int32_t m_ReadPos;

	uint8_t m_MsgBuf[NETWORKMESSAGE_MAXSIZE];
};

typedef boost::shared_ptr<NetworkMessage> NetworkMessage_ptr;

#endif // #ifndef __NETWORK_MESSAGE_H__

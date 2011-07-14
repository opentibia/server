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

#ifndef __OTSERV_TOOLS_H__
#define __OTSERV_TOOLS_H__

#include "definitions.h"
#include "enums.h"
#include "const.h"
#include <libxml/parser.h>
#include <boost/tokenizer.hpp>
#include <vector>
#include <iostream>
#include <ctime>
#include <cstring>

typedef boost::tokenizer<boost::char_separator<char> > tokenizer;

enum DistributionType_t {
	DISTRO_UNIFORM,
	DISTRO_SQUARE,
	DISTRO_NORMAL
};

inline uint16_t swap_uint16(uint16_t x)
{
	return (x & 0xFF00) >> 8 | (x & 0x00FF) << 8;
}

inline uint32_t swap_uint32(uint32_t x)
{
	return (x & 0xFF000000) >> 24
		 | (x & 0x00FF0000) >> 8
		 | (x & 0x0000FF00) << 8
		 | (x & 0x000000FF) << 24;
}

inline uint64_t swap_uint64(uint64_t x)
{
	return (x & 0xFF00000000000000ULL) >> 56
		 | (x & 0x00FF000000000000ULL) >> 40
		 | (x & 0x0000FF0000000000ULL) >> 24
		 | (x & 0x000000FF00000000ULL) >> 8
		 | (x & 0x00000000FF000000) << 8
		 | (x & 0x0000000000FF0000) << 24
		 | (x & 0x000000000000FF00) << 40
		 | (x & 0x00000000000000FF) << 56;
}

inline int16_t swap_int16(int16_t x)
{
	return (int16_t)swap_uint16((uint16_t)x);
}

inline int32_t swap_int32(int32_t x)
{
	return (int32_t)swap_uint32((uint32_t)x);
}

/*inline float swap_float32(float x)
{
	uint32_t ui = *((uint32_t *)(void *)&x);
	ui = swap_uint32(ui);

	return *((float *)(void *)&ui);
}*/

void replaceString(std::string& str, const std::string sought, const std::string replacement);
void trim_right(std::string& source, const std::string& t = "\n\t ");
void trim_left(std::string& source, const std::string& t = "\n\t ");
void trim(std::string& source, const std::string& t = "\n\t ");
void toLowerCaseString(std::string& source);
void toUpperCaseString(std::string& source);
std::string asLowerCaseString(const std::string& source);
std::string asUpperCaseString(const std::string& source);
bool booleanString(std::string source);
bool utf8ToLatin1(char* intext, std::string& outtext);
bool readXMLInteger(xmlNodePtr node, const char* tag, int32_t& value);
bool readXMLInteger64(xmlNodePtr node, const char* tag, uint64_t& value);
bool readXMLFloat(xmlNodePtr node, const char* tag, float& value);
bool readXMLString(xmlNodePtr node, const char* tag, std::string& value);
bool readXMLContentString(xmlNodePtr node, std::string& value);
std::vector<std::string> explodeString(const std::string& inString, const std::string& separator);
bool hasBitSet(uint32_t flag, uint32_t flags);

uint32_t rand24b();
float box_muller(float m, float s);

int random_range(int lowest_number, int highest_number, DistributionType_t type = DISTRO_UNIFORM, float deviation = 0.25);

void hexdump(unsigned char *_data, int _len);
char upchar(char c);

std::string urlEncode(const char* str);
std::string urlEncode(const std::string& str);

bool passwordTest(std::string plain, std::string &hash);

std::string convertIPToString(uint32_t ip);
//buffer should have at least 21 bytes. dd/mm/yyyy  hh:mm:ss
void formatDate(time_t time, char* buffer);
//buffer should have at least 16 bytes
void formatDateShort(time_t time, char* buffer);

MagicEffectClasses getMagicEffect(const std::string& strValue);
ShootType_t getShootType(const std::string& strValue);
Ammo_t getAmmoType(const std::string& strValue);
AmmoAction_t getAmmoAction(const std::string& strValue);

std::string getViolationReasonString(int32_t reasonId);
std::string getViolationActionString(violationAction_t actionId, bool ipBanishment);
std::string playerSexAdjectiveString(PlayerSex_t sex);
std::string playerSexSubjectString(PlayerSex_t sex);

uint32_t adlerChecksum(uint8_t *data, int32_t len);

void showTime(std::stringstream& str, uint32_t time);
uint32_t parseTime(const std::string& time);
std::string parseParams(tokenizer::iterator &it, tokenizer::iterator end);
std::string ucfirst(std::string);

#endif

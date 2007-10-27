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

#include "otsystem.h"
#include "const80.h"

#include <string>
#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <cmath>

#include <libxml/parser.h>


enum DistributionType_t{
	DISTRO_NORMAL,
	DISTRO_SQUARE
};

bool fileExists(const char* filename);
void replaceString(std::string& str, const std::string sought, const std::string replacement);
void trim_right(std::string& source, const std::string& t);
void trim_left(std::string& source, const std::string& t);
void toLowerCaseString(std::string& source);

bool readXMLInteger(xmlNodePtr node, const char* tag, int& value);
bool readXMLInteger64(xmlNodePtr node, const char* tag, uint64_t& value);
bool readXMLFloat(xmlNodePtr node, const char* tag, float& value);
bool readXMLString(xmlNodePtr node, const char* tag, std::string& value);

int random_range(int lowest_number, int highest_number, DistributionType_t type = DISTRO_NORMAL);

void hexdump(unsigned char *_data, int _len);
char upchar(char c);

std::string urlEncode(const char* str);
std::string urlEncode(const std::string& str);

bool passwordTest(const std::string &plain, std::string &hash);

//buffer should be at least 17 bytes
void formatIP(uint32_t ip, char* buffer);
//buffer should have at least 21 bytes. dd/mm/yyyy  hh:mm:ss
void formatDate(time_t time, char* buffer);
//buffer should have at least 16 bytes
void formatDate2(time_t time, char* buffer);

MagicEffectClasses getMagicEffect(const std::string& strValue);
ShootType_t getShootType(const std::string& strValue);
Ammo_t getAmmoType(const std::string& strValue);

#endif

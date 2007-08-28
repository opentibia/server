//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Various functions.
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
#include "boost/asio.hpp"

#include "math.h"
#include "tools.h"
#include "configmanager.h"
#include "md5.h"
#include <sstream>
#include <iomanip>

extern ConfigManager g_config;

using namespace boost::asio::ip;
using namespace boost;

bool fileExists(const char* filename)
{
	FILE* f = fopen(filename, "rb");
	bool exists = (f != NULL);
	if(f != NULL)
		fclose(f);

	return exists;
}

void replaceString(std::string& str, const std::string sought, const std::string replacement)
{
	size_t pos = 0;
	size_t start = 0;
	size_t soughtLen = sought.length();
	size_t replaceLen = replacement.length();
	while((pos = str.find(sought, start)) != std::string::npos){
		str = str.substr(0, pos) + replacement + str.substr(pos + soughtLen);
		start = pos + replaceLen;
	}
}

void trim_right(std::string& source, const std::string& t)
{
	source.erase(source.find_last_not_of(t)+1);
}

void trim_left(std::string& source, const std::string& t)
{
	source.erase(0, source.find_first_not_of(t));
}

void toLowerCaseString(std::string& source)
{
	std::transform(source.begin(), source.end(), source.begin(), tolower);
}

bool readXMLInteger(xmlNodePtr node, const char* tag, int& value)
{
	char* nodeValue = (char*)xmlGetProp(node, (xmlChar*)tag);
	if(nodeValue){
		value = atoi(nodeValue);
		xmlFreeOTSERV(nodeValue);
		return true;
	}

	return false;
}

bool readXMLInteger64(xmlNodePtr node, const char* tag, uint64_t& value)
{
	char* nodeValue = (char*)xmlGetProp(node, (xmlChar*)tag);
	if(nodeValue){
		value = ATOI64(nodeValue);
		xmlFreeOTSERV(nodeValue);
		return true;
	}

	return false;
}

bool readXMLFloat(xmlNodePtr node, const char* tag, float& value)
{
	char* nodeValue = (char*)xmlGetProp(node, (xmlChar*)tag);
	if(nodeValue){
		value = atof(nodeValue);
		xmlFreeOTSERV(nodeValue);
		return true;
	}

	return false;
}

bool readXMLString(xmlNodePtr node, const char* tag, std::string& value)
{
	char* nodeValue = (char*)xmlGetProp(node, (xmlChar*)tag);
	if(nodeValue){
		value = nodeValue;
		xmlFreeOTSERV(nodeValue);
		return true;
	}

	return false;
}

#define RAND_MAX24 16777216
uint32_t rand24b()
{
	return (rand() << 12) ^ (rand()) & (0xFFFFFF);
}

int random_range(int lowest_number, int highest_number, DistributionType_t type /*= DISTRO_NORMAL*/)
{
	if(highest_number == lowest_number){
		return lowest_number;
	}
	
	if(lowest_number > highest_number){
		int nTmp = highest_number;
		highest_number = lowest_number;
		lowest_number = nTmp;
	}

	int range = highest_number - lowest_number;
	
	if(type == DISTRO_NORMAL){
		int r = rand24b() % range;
		return lowest_number + r + 1;
	}
	else{
		float r = 1.f -sqrt((1.f*rand24b())/RAND_MAX24);
		return lowest_number + (int)((float)range * r);
	}
}

// dump a part of the memory to stderr.
void hexdump(unsigned char *_data, int _len) {
	int i;
	for(; _len > 0; _data += 16, _len -= 16) {
		for (i = 0; i < 16 && i < _len; i++)
			fprintf(stderr, "%02x ", _data[i]);
		for(; i < 16; i++)
			fprintf(stderr, "   ");
		
		fprintf(stderr, " ");
		for(i = 0; i < 16 && i < _len; i++)
			fprintf(stderr, "%c", (_data[i] & 0x70) < 32 ? '·' : _data[i]);
		
		fprintf(stderr, "\n");
	}
}

// Upcase a char.
char upchar(char c) {
  if (c >= 'a' && c <= 'z')
      return c - 'a' + 'A';
  else if (c == 'à')
      return 'À';
  else if (c == 'á')
      return 'Á';
  else if (c == 'â')
      return 'Â';
  else if (c == 'ã')
      return 'Ã';
  else if (c == 'ä')
      return 'Ä';
  else if (c == 'å')
      return 'Å';
  else if (c == 'æ')
      return 'Æ';
  else if (c == 'ç')
      return 'Ç';
  else if (c == 'è')
      return 'È';
  else if (c == 'é')
      return 'É';
  else if (c == 'ê')
      return 'Ê';
  else if (c == 'ë')
      return 'Ë';
  else if (c == 'ì')
      return 'Ì';
  else if (c == 'í')
      return 'Í';
  else if (c == 'î')
      return 'Î';
  else if (c == 'ï')
      return 'Ï';
  else if (c == 'ð')
      return 'Ð';
  else if (c == 'ñ')
      return 'Ñ';
  else if (c == 'ò')
      return 'Ò';
  else if (c == 'ó')
      return 'Ó';
  else if (c == 'ô')
      return 'Ô';
  else if (c == 'õ')
      return 'Õ';
  else if (c == 'ö')
      return 'Ö';
  else if (c == 'ø')
      return 'Ø';
  else if (c == 'ù')
      return 'Ù';
  else if (c == 'ú')
      return 'Ú';
  else if (c == 'û')
      return 'Û';
  else if (c == 'ü')
      return 'Ü';
  else if (c == 'ý')
      return 'Ý';
  else if (c == 'þ')
      return 'Þ';
  else if (c == 'ÿ')
      return 'ß';
  else
      return c;
}

std::string urlEncode(const std::string& str)
{
	return urlEncode(str.c_str());
}

std::string urlEncode(const char* str)
{
	std::string out;
	const char* it;
	for(it = str; *it != 0; it++){
		char ch = *it;
		if(!(ch >= '0' && ch <= '9') &&
			!(ch >= 'A' && ch <= 'Z') &&
			!(ch >= 'a' && ch <= 'z')){
				char tmp[4];
				sprintf(tmp, "%%%02X", ch);
				out = out + tmp;
			}
		else{
			out = out + *it;
		}
	}
	return out;
}

uint32_t getIPSocket(const tcp::socket& s)
{
	asio::error error;
	const tcp::endpoint endpoint = s.remote_endpoint(asio::assign_error(error));
	if(!error){
		return endpoint.address().to_v4().to_ulong();
	}
	
	/*
	sockaddr_in sain;
	socklen_t salen = sizeof(sockaddr_in);

	if(getpeername(s, (sockaddr*)&sain, &salen) == 0){
#if defined WIN32 || defined __WINDOWS__
		return sain.sin_addr.S_un.S_addr;
#else
		return sain.sin_addr.s_addr;
#endif
	}
*/
	return 0;
}

bool passwordTest(const std::string &plain, std::string &hash)
{
	if(g_config.getNumber(ConfigManager::USE_MD5_PASS) == PASSWORD_TYPE_MD5){
		MD5_CTX m_md5;
		std::stringstream hexStream;
		std::string plainHash;

		MD5Init(&m_md5, 0);
		MD5Update(&m_md5, (const unsigned char*)plain.c_str(), plain.length());
		MD5Final(&m_md5);

		hexStream.flags(std::ios::hex);
		for(uint32_t i = 0; i < 16; ++i){
			hexStream << std::setw(2) << std::setfill('0') << (uint32_t)m_md5.digest[i];
		}

		plainHash = hexStream.str();
		std::transform(plainHash.begin(), plainHash.end(), plainHash.begin(), upchar);
		std::transform(hash.begin(), hash.end(), hash.begin(), upchar);
		if(plainHash == hash){
			return true;
		}
		else{
			return false;
		}
	}
	else{
		if(plain == hash){
			return true;
		}
		else{
			return false;
		}
	}
}

//buffer should have at least 17 bytes
void formatIP(uint32_t ip, char* buffer)
{
	sprintf(buffer, "%d.%d.%d.%d", ip & 0xFF, (ip >> 8) & 0xFF, (ip >> 16) & 0xFF, (ip >> 24));
}

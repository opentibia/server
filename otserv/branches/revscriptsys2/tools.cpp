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
// Inc., 59 Temple Place - S
		} catch(enum_conversion_error&e){}uite 330, Boston, MA  02111-1307, USA.
//////////////////////////////////////////////////////////////////////
#include "otpch.h"

#include "tools.h"
#include "md5.h"
#include "sha1.h"
#include "configmanager.h"
#include <iomanip>

extern ConfigManager g_config;

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

void trim(std::string& source, const std::string& t)
{
	trim_left(source, t);
	trim_right(source, t);
}

void toLowerCaseString(std::string& source)
{
	std::transform(source.begin(), source.end(), source.begin(), tolower);
}

void toUpperCaseString(std::string& source)
{
	std::transform(source.begin(), source.end(), source.begin(), upchar);
}

std::string asLowerCaseString(const std::string& source)
{
	std::string s = source;
	toLowerCaseString(s);
	return s;
}

std::string asUpperCaseString(const std::string& source)
{
	std::string s = source;
	toUpperCaseString(s);
	return s;
}

int strcasecmp(const std::string& s1, const std::string& s2)
{
	return strcasecmp(s1.c_str(), s2.c_str());
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

#if (defined __WINDOWS__ || defined WIN32) && !defined __GNUC__
bool readXMLInteger(xmlNodePtr node, const char* tag, int32_t& value)
{
	char* nodeValue = (char*)xmlGetProp(node, (xmlChar*)tag);
	if(nodeValue){
		value = atoi(nodeValue);
		xmlFreeOTSERV(nodeValue);
		return true;
	}

	return false;
}
#endif

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

bool utf8ToLatin1(char* intext, std::string& outtext)
{
	outtext = "";

	if(intext == NULL){
		return false;
	}

	int inlen  = strlen(intext);
	if(inlen == 0){
		return false;
	}

	int outlen = inlen*2;
	unsigned char* outbuf = new unsigned char[outlen];
	int res = UTF8Toisolat1(outbuf, &outlen, (unsigned char*)intext, &inlen);
	if(res < 0){
		delete[] outbuf;
		return false;
	}

	outbuf[outlen] = '\0';
	outtext = (char*)outbuf;
	delete[] outbuf;
	return true;
}

bool readXMLString(xmlNodePtr node, const char* tag, std::string& value)
{
	char* nodeValue = (char*)xmlGetProp(node, (xmlChar*)tag);
	if(nodeValue){
		if(!utf8ToLatin1(nodeValue, value)){
			value = nodeValue;
		}

		xmlFreeOTSERV(nodeValue);
		return true;
	}

	return false;
}

bool readXMLContentString(xmlNodePtr node, std::string& value)
{
	char* nodeValue = (char*)xmlNodeGetContent(node);
	if(nodeValue){
		if(!utf8ToLatin1(nodeValue, value)){
			value = nodeValue;
		}

		xmlFreeOTSERV(nodeValue);
		return true;
	}

	return false;
}

std::vector<std::string> explodeString(const std::string& inString, const std::string& separator)
{
	std::vector<std::string> returnVector;
	std::string::size_type start = 0;
	std::string::size_type end = 0;

	while((end=inString.find (separator, start)) != std::string::npos){
		returnVector.push_back (inString.substr (start, end-start));
		start = end+separator.size();
	}

	returnVector.push_back (inString.substr (start));
	return returnVector;
}

bool hasBitSet(uint32_t flag, uint32_t flags)
{
	return ((flags & flag) == flag);
}

#define RAND_MAX24 16777216
uint32_t rand24b()
{
	return ((rand() << 12) ^ ((rand()) & (0xFFFFFF)) );
}

float box_muller(float m, float s)
{
	// normal random variate generator
	// mean m, standard deviation s

	float x1, x2, w, y1;
	static float y2;
	static int use_last = 0;

	if(use_last)			// use value from previous call
	{
		y1 = y2;
		use_last = 0;
	}
	else
	{
		do {
			double r1 = (((float)(rand()) / RAND_MAX));
			double r2 = (((float)(rand()) / RAND_MAX));

			x1 = 2.0 * r1 - 1.0;
			x2 = 2.0 * r2 - 1.0;
			w = x1 * x1 + x2 * x2;
		} while ( w >= 1.0 );

		w = sqrt( (-2.0 * log( w ) ) / w );
		y1 = x1 * w;
		y2 = x2 * w;
		use_last = 1;
	}

	return( m + y1 * s );
}

int random_range(int lowest_number, int highest_number, DistributionType_t type /*= DISTRO_UNIFORM*/)
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

	if(type == DISTRO_UNIFORM){
		int r = rand24b() % (range + 1);
		return lowest_number + r;
	}
	else if(type == DISTRO_NORMAL){
		float value = box_muller(0.5, 0.25);

		if(value < 0){
			value = 0;
		}else if(value > 1){
			value = 1;
		}

		return lowest_number + (int)((float)range * value);
	}
	else{
		float r = 1.f - sqrt((1.f*rand24b())/RAND_MAX24);
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
			fprintf(stderr, "%c", (_data[i] & 0x70) < 32 ? '?' : _data[i]);

		fprintf(stderr, "\n");
	}
}

// Upcase a char.
char upchar(char c)
{
	if((c >= 97 && c <= 122) || (c <= -1 && c >= -32)){
		return c-32;
	}
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

bool passwordTest(std::string plain, std::string &hash)
{
	// Salt it beforehand
	plain += g_config.getString(ConfigManager::PASSWORD_SALT);

	switch(g_config.getNumber(ConfigManager::PASSWORD_TYPE)){
	case PASSWORD_TYPE_PLAIN:
	{
		if(plain == hash){
			return true;
		}
		break;
	}
	case PASSWORD_TYPE_MD5:
	{
		MD5_CTX m_md5;
		std::stringstream hexStream;

		MD5Init(&m_md5, 0);
		MD5Update(&m_md5, (const unsigned char*)plain.c_str(), plain.length());
		MD5Final(&m_md5);

		hexStream.flags(std::ios::hex | std::ios::uppercase);
		for(uint32_t i = 0; i < 16; ++i){
			hexStream << std::setw(2) << std::setfill('0') << (uint32_t)m_md5.digest[i];
		}

		std::transform(hash.begin(), hash.end(), hash.begin(), upchar);
		if(hexStream.str() == hash){
			return true;
		}
		break;
	}
	case PASSWORD_TYPE_SHA1:
	{
		SHA1 sha1;
		unsigned sha1Hash[5];
		std::stringstream hexStream;

		sha1.Input((const unsigned char*)plain.c_str(), plain.length());
		sha1.Result(sha1Hash);

		hexStream.flags(std::ios::hex | std::ios::uppercase);
		for(uint32_t i = 0; i < 5; ++i){
			hexStream << std::setw(8) << std::setfill('0') << (uint32_t)sha1Hash[i];
		}

		std::transform(hash.begin(), hash.end(), hash.begin(), upchar);
		if(hexStream.str() == hash){
			return true;
		}

		break;
	}
	}
	return false;
}

std::string convertIPToString(uint32_t ip)
{
	char buffer[20];
	sprintf(buffer, "%d.%d.%d.%d", ip & 0xFF, (ip >> 8) & 0xFF, (ip >> 16) & 0xFF, (ip >> 24));
	return buffer;
}

//buffer should have at least 21 bytes
void formatDate(time_t time, char* buffer)
{
	const tm* tms = localtime(&time);
	if(tms){
		sprintf(buffer, "%02d/%02d/%04d %02d:%02d:%02d", tms->tm_mday, tms->tm_mon + 1, tms->tm_year + 1900,
			tms->tm_hour, tms->tm_min, tms->tm_sec);
	}
	else{
		sprintf(buffer, "UNIX Time : %d", (int)time);
	}
}

//buffer should have at least 16 bytes
void formatDateShort(time_t time, char* buffer)
{
	const tm* tms = localtime(&time);
	if(tms){
		strftime(buffer, 12, "%d %b %Y", tms);
	}
	else{
		sprintf(buffer, "UNIX Time : %d", (int)time);
	}
}

std::string getViolationReasonString(int32_t reasonId)
{
	switch(reasonId)
	{
		case 0:
			return "Offensive Name";
		case 1:
			return "Invalid Name Format";
		case 2:
			return "Unsuitable Name";
		case 3:
			return "Name Inciting Rule Violation";
		case 4:
			return "Offensive Statement";
		case 5:
			return "Spamming";
		case 6:
			return "Illegal Advertising";
		case 7:
			return "Off-Topic Public Statement";
		case 8:
			return "Non-English Public Statement";
		case 9:
			return "Inciting Rule Violation";
		case 10:
			return "Bug Abuse";
		case 11:
			return "Game Weakness Abuse";
		case 12:
			return "Using Unofficial Software to Play";
		case 13:
			return "Hacking";
		case 14:
			return "Multi-Clienting";
		case 15:
			return "Account Trading or Sharing";
		case 16:
			return "Threatening Gamemaster";
		case 17:
			return "Pretending to Have Influence on Rule Enforcement";
		case 18:
			return "False Report to Gamemaster";
		case 19:
			return "Destructive Behaviour";
	}

	return "Unknown Reason";
}

std::string getViolationActionString(ViolationAction actionId, bool ipBanishment)
{
	std::string action;

	if(actionId == ACTION_NOTATION)
		action = "Notation";
	else if(actionId == ACTION_NAMEREPORT)
		action = "Name Report";
	else if(actionId == ACTION_BANREPORT)
		action = "Name Report + Banishment";
	else if(actionId == ACTION_BANFINAL)
		action = "Banishment + Final Warning";
	else if(actionId == ACTION_BANREPORTFINAL)
		action = "Name Report + Banishment + Final Warning";
	else if(actionId == ACTION_STATEMENT)
		action = "Statement Report";
	else if(actionId == ACTION_DELETION)
		action = "Deletion";
	else if(actionId == ACTION_BANISHMENT)
			action = "Banishment";
	
	if(ipBanishment)
		action += " + IP Banishment";

	return action;
}

std::string playerSexAdjectiveString(PlayerSex sex)
{
	if(sex.value() % 2 == 0)
		return "her";
	else
		return "his";
}

std::string playerSexSubjectString(PlayerSex sex)
{
	if(sex.value() % 2 == 0)
		return "She";
	else
		return "He";
}

#define MOD_ADLER 65521
uint32_t adlerChecksum(uint8_t *data, int32_t len)
{
	if(len < 0){
		std::cout << "[Error] adlerChecksum. len < 0" << std::endl;
		return 0;
	}

	uint32_t a = 1, b = 0;
	while (len > 0)
	{
		size_t tlen = len > 5552 ? 5552 : len;
		len -= tlen;
		do
		{
			a += *data++;
			b += a;
		} while (--tlen);

		a %= MOD_ADLER;
		b %= MOD_ADLER;
		}

	return (b << 16) | a;
}

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
		int r = rand24b() % (range + 1);
		return lowest_number + r;
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
			fprintf(stderr, "%c", (_data[i] & 0x70) < 32 ? '·' : _data[i]);

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

bool passwordTest(const std::string &plain, std::string &hash)
{
	if(g_config.getNumber(ConfigManager::PASSWORD_TYPE) == PASSWORD_TYPE_MD5){
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
	}
	else{
		if(plain == hash){
			return true;
		}
	}
	return false;
}

//buffer should have at least 17 bytes
void formatIP(uint32_t ip, char* buffer)
{
	sprintf(buffer, "%d.%d.%d.%d", ip & 0xFF, (ip >> 8) & 0xFF, (ip >> 16) & 0xFF, (ip >> 24));
}

//buffer should have at least 21 bytes
void formatDate(time_t time, char* buffer)
{
	const tm* tms = localtime(&time);
	if(tms){
		sprintf(buffer, "%02d/%02d/%04d  %02d:%02d:%02d", tms->tm_mday, tms->tm_mon + 1, tms->tm_year + 1900,
			tms->tm_hour, tms->tm_min, tms->tm_sec);
	}
	else{
		sprintf(buffer, "UNIX Time : %d", (int)time);
	}
}

//buffer should have at least 16 bytes
void formatDate2(time_t time, char* buffer)
{
	const tm* tms = localtime(&time);
	if(tms){
		strftime(buffer, 12, "%d %b %Y", tms);
	}
	else{
		sprintf(buffer, "UNIX Time : %d", (int)time);
	}
}

struct MagicEffectNames{
	const char* name;
	MagicEffectClasses effect;
};

struct ShootTypeNames{
	const char* name;
	ShootType_t shoot;
};

struct AmmoTypeNames{
	const char* name;
	Ammo_t ammoType;
};

struct AmmoActionNames{
	const char* name;
	AmmoAction_t ammoAction;
};

MagicEffectNames magicEffectNames[] = {
	{"redspark", NM_ME_DRAW_BLOOD},
	{"bluebubble", NM_ME_LOSE_ENERGY},
	{"poff", NM_ME_PUFF},
	{"yellowspark", NM_ME_BLOCKHIT},
	{"explosionarea", NM_ME_EXPLOSION_AREA},
	{"explosion", NM_ME_EXPLOSION_DAMAGE},
	{"firearea", NM_ME_FIRE_AREA},
	{"yellowbubble", NM_ME_YELLOW_RINGS},
	{"greenbubble", NM_ME_POISON_RINGS},
	{"blackspark", NM_ME_HIT_AREA},
	{"energyarea", NM_ME_ENERGY_AREA},
	{"energy", NM_ME_ENERGY_DAMAGE},
	{"blueshimmer", NM_ME_MAGIC_ENERGY},
	{"redshimmer", NM_ME_MAGIC_BLOOD},
	{"greenshimmer", NM_ME_MAGIC_POISON},
	{"fire", NM_ME_HITBY_FIRE},
	{"greenspark", NM_ME_POISON},
	{"mortarea", NM_ME_MORT_AREA},
	{"greennote", NM_ME_SOUND_GREEN},
	{"rednote", NM_ME_SOUND_RED},
	{"poison", NM_ME_POISON_AREA},
	{"yellownote", NM_ME_SOUND_YELLOW},
	{"purplenote", NM_ME_SOUND_PURPLE},
	{"bluenote", NM_ME_SOUND_BLUE},
	{"whitenote", NM_ME_SOUND_WHITE},
	{"bubbles", NM_ME_BUBBLES},
	{"dice", NM_ME_CRAPS},
	{"giftwraps", NM_ME_GIFT_WRAPS},
	{"yellowfirework", NM_ME_FIREWORK_YELLOW},
	{"redfirework", NM_ME_FIREWORK_RED},
	{"bluefirework", NM_ME_FIREWORK_BLUE},
	{"stun", NM_ME_STUN},
	{"sleep", NM_ME_SLEEP},
	{"watercreature", NM_ME_WATERCREATURE},
	{"groundshaker", NM_ME_GROUNDSHAKER}
};

ShootTypeNames shootTypeNames[] = {
	{"spear", NM_SHOOT_SPEAR},
	{"bolt", NM_SHOOT_BOLT},
	{"arrow", NM_SHOOT_ARROW},
	{"fire", NM_SHOOT_FIRE},
	{"energy", NM_SHOOT_ENERGY},
	{"poisonarrow", NM_SHOOT_POISONARROW},
	{"burstarrow", NM_SHOOT_BURSTARROW},
	{"throwingstar", NM_SHOOT_THROWINGSTAR},
	{"throwingknife", NM_SHOOT_THROWINGKNIFE},
	{"smallstone", NM_SHOOT_SMALLSTONE},
	{"suddendeath", NM_SHOOT_SUDDENDEATH},
	{"largerock", NM_SHOOT_LARGEROCK},
	{"snowball", NM_SHOOT_SNOWBALL},
	{"powerbolt", NM_SHOOT_POWERBOLT},
	{"poison", NM_SHOOT_POISONFIELD},
	{"infernalbolt", NM_SHOOT_INFERNALBOLT},
	{"huntingspear", NM_SHOOT_HUNTINGSPEAR},
	{"enchantedspear", NM_SHOOT_ENCHANTEDSPEAR},
	{"redstar", NM_SHOOT_REDSTAR},
	{"greenstar", NM_SHOOT_GREENSTAR},
	{"royalspear", NM_SHOOT_ROYALSPEAR},
	{"sniperarrow", NM_SHOOT_SNIPERARROW},
	{"onyxarrow", NM_SHOOT_ONYXARROW},
	{"piercingbolt", NM_SHOOT_PIERCINGBOLT},
	{"whirlwindsword", NM_SHOOT_WHIRLWINDSWORD},
	{"whirlwindaxe", NM_SHOOT_WHIRLWINDAXE},
	{"whirlwindclub", NM_SHOOT_WHIRLWINDCLUB},
	{"etherealspear", NM_SHOOT_ETHEREALSPEAR}
};

AmmoTypeNames ammoTypeNames[] = {
	{"spear", AMMO_SPEAR},
	{"bolt", AMMO_BOLT},
	{"arrow", AMMO_ARROW},
	{"poisonarrow", AMMO_ARROW},
	{"burstarrow", AMMO_ARROW},
	{"throwingstar", AMMO_THROWINGSTAR},
	{"throwingknife", AMMO_THROWINGKNIFE},
	{"smallstone", AMMO_STONE},
	{"largerock", AMMO_STONE},
	{"snowball", AMMO_SNOWBALL},
	{"powerbolt", AMMO_BOLT},
	{"infernalbolt", AMMO_BOLT},
	{"huntingspear", AMMO_SPEAR},
	{"enchantedspear", AMMO_SPEAR},
	{"royalspear", AMMO_SPEAR},
	{"sniperarrow", AMMO_ARROW},
	{"onyxarrow", AMMO_ARROW},
	{"piercingbolt", AMMO_BOLT},
	{"etherealspear", AMMO_SPEAR}
};

AmmoActionNames ammoActionNames[] = {
	{"move", AMMOACTION_MOVE},
	{"moveback", AMMOACTION_MOVEBACK},
	{"removecharge", AMMOACTION_REMOVECHARGE},
	{"removecount", AMMOACTION_REMOVECOUNT}
};

MagicEffectClasses getMagicEffect(const std::string& strValue)
{
	for(uint32_t i = 0; i < sizeof(magicEffectNames)/sizeof(MagicEffectNames); ++i){
		if(strValue == magicEffectNames[i].name){
			return magicEffectNames[i].effect;
		}
	}
	return NM_ME_UNK;
}

ShootType_t getShootType(const std::string& strValue)
{
	for(uint32_t i = 0; i < sizeof(shootTypeNames)/sizeof(ShootTypeNames); ++i){
		if(strValue == shootTypeNames[i].name){
			return shootTypeNames[i].shoot;
		}
	}
	return NM_SHOOT_UNK;
}

Ammo_t getAmmoType(const std::string& strValue)
{
	for(uint32_t i = 0; i < sizeof(ammoTypeNames)/sizeof(AmmoTypeNames); ++i){
		if(strValue == ammoTypeNames[i].name){
			return ammoTypeNames[i].ammoType;
		}
	}
	return AMMO_NONE;
}

AmmoAction_t getAmmoAction(const std::string& strValue)
{
	for(uint32_t i = 0; i < sizeof(ammoActionNames)/sizeof(AmmoActionNames); ++i){
		if(strValue == ammoActionNames[i].name){
			return ammoActionNames[i].ammoAction;
		}
	}
	return AMMOACTION_NONE;
}

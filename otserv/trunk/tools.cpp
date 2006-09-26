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
#include "otpch.h"

#include "tools.h"

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
	int pos = 0;
	int start = 0;
	int soughtLen = sought.length();
	int replaceLen = replacement.length();
	while((pos = str.find(sought, start)) != std::string::npos){
		str = str.substr(0, pos) + replacement + str.substr(pos + soughtLen);
		start = pos + replaceLen;
	}
}

void trim_right(std::string &source, const std::string &t)
{
	source.erase(source.find_last_not_of(t)+1);
}

void trim_left(std::string &source, const std::string &t)
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

int random_range(int lowest_number, int highest_number)
{
	if(lowest_number > highest_number){
		int nTmp = highest_number;
		highest_number = lowest_number;
		lowest_number = nTmp;
	}

	double range = highest_number - lowest_number + 1;
	return lowest_number + int(range * rand()/(RAND_MAX + 1.0));
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

uint32_t getIPSocket(SOCKET s)
{
	sockaddr_in sain;
	socklen_t salen = sizeof(sockaddr_in);

	if(getpeername(s, (sockaddr*)&sain, &salen) == 0){
#if defined WIN32 || defined __WINDOWS__
		return sain.sin_addr.S_un.S_addr;
#else
		return sain.sin_addr.s_addr;
#endif
	}

	return 0;
}

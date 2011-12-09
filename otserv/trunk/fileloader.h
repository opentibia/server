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

#ifndef __OTSERV_FILELOADER_H__
#define __OTSERV_FILELOADER_H__

#include "definitions.h"
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>

struct NodeStruct;

typedef NodeStruct* NODE;

struct NodeStruct {
	NodeStruct();
	
	unsigned long start;
	unsigned long propsSize;
	unsigned long type;
	NodeStruct* next;
	NodeStruct* child;

	static void clearNet(NodeStruct* root);

private:
	static void clearNext(NodeStruct* node);
	static void clearChild(NodeStruct* node);
};

#define NO_NODE 0

enum FILELOADER_ERRORS{
	ERROR_NONE,
	ERROR_INVALID_FILE_VERSION,
	ERROR_CAN_NOT_OPEN,
	ERROR_CAN_NOT_CREATE,
	ERROR_EOF,
	ERROR_SEEK_ERROR,
	ERROR_NOT_OPEN,
	ERROR_INVALID_NODE,
	ERROR_INVALID_FORMAT,
	ERROR_TELL_ERROR,
	ERROR_COULDNOTWRITE,
	ERROR_CACHE_ERROR
};

class PropStream;

class FileLoader {
public:
	FileLoader();
	virtual ~FileLoader();

	bool openFile(const char* filename, bool write, bool caching = false);
	const unsigned char* getProps(const NODE, unsigned long &size);
	bool getProps(const NODE, PropStream& props);
	NODE getChildNode(const NODE parent, unsigned long &type);
	NODE getNextNode(const NODE prev, unsigned long &type);

	void startNode(unsigned char type);
	void endNode();
	int setProps(void* data, unsigned short size);

	int getError(){return m_lastError;}
	void clearError(){m_lastError = ERROR_NONE;}

protected:
	enum SPECIAL_BYTES{
		NODE_START = 0xFE,
		NODE_END = 0xFF,
		ESCAPE_CHAR = 0xFD
	};

	bool parseNode(NODE node);

	bool readByte(int &value);
	bool readBytes(unsigned char* buffer, unsigned int size, long pos);
	bool checks(const NODE node);
	bool safeSeek(unsigned long pos);
	bool safeTell(long &pos);

public:
	bool writeData(const void* data, int size, bool unescape);
	
protected:
	FILE* m_file;
	FILELOADER_ERRORS m_lastError;
	NODE m_root;
	unsigned long m_buffer_size;
	unsigned char* m_buffer;

	bool m_use_cache;
	struct _cache{
		unsigned long loaded;
		unsigned long base;
		unsigned char* data;
		size_t size;
	};
	#define CACHE_BLOCKS 3
	unsigned long m_cache_size;
	_cache m_cached_data[CACHE_BLOCKS];
	#define NO_VALID_CACHE 0xFFFFFFFF
	unsigned long m_cache_index;
	unsigned long m_cache_offset;
	inline unsigned long getCacheBlock(unsigned long pos);
	long loadCacheBlock(unsigned long pos);
};

class PropStream {
public:
	PropStream();

	void init(const char* a, unsigned long size);
	int64_t size();

	bool GET_UINT32(uint32_t& ret);
	bool GET_INT32(int32_t& ret);
	bool GET_UINT16(uint16_t& ret);
	bool GET_INT16(int16_t& ret);
	bool GET_FLOAT(float& ret);
	bool GET_UINT8(uint8_t& ret);
	bool GET_INT8(int8_t& ret);
	bool GET_CHAR(int8_t& ret);
	bool GET_STRING(std::string& ret);
	bool GET_LSTRING(std::string& ret);
	bool GET_NSTRING(std::string& ret, unsigned short str_len);
	bool GET_RAWSTRING(char* buffer, unsigned short str_len);
	bool SKIP_N(int32_t n);	

protected:
	template <typename T>
	bool GET_VALUE(T& ret){
		if(size() < (long)sizeof(T)){
			return false;
		}
		ret = *((T*)p);
		p = p + sizeof(T);
		return true;
	}

	const char* p;
	const char* end;
};

class PropWriteStream{
public:
	PropWriteStream();
	~PropWriteStream();

	const char* getStream(uint32_t& _size) const;

	void ADD_UINT32(uint32_t ret);
	void ADD_INT32(int32_t ret);
	void ADD_UINT16(uint16_t ret);
	void ADD_INT16(int16_t ret);
	void ADD_FLOAT(float ret);
	void ADD_UINT8(uint8_t ret);
	void ADD_INT8(int8_t ret);
	void ADD_STRING(const std::string& add);
	void ADD_LSTRING(const std::string& add);

protected:
	template <typename T>
	void ADD_VALUE(T add){
		if((buffer_size - size) < sizeof(T)){
			buffer_size = buffer_size + sizeof(T) + 0x1F;
			buffer = (char*)realloc(buffer,buffer_size);
		}

		memcpy(&buffer[size], &add, sizeof(T));
		size = size + sizeof(T);
	}

	char* buffer;
	uint32_t buffer_size;
	uint32_t size;
};

#endif // __OTSERV_FILELOADER_H__

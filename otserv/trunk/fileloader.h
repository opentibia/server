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

#include <string>
#include <stdio.h>
#include <stdlib.h>

struct NodeStruct;

typedef NodeStruct* NODE;

struct NodeStruct{
	NodeStruct(){
		start = 0;
		propsSize = 0;
		next = 0;
		child = 0;
		type = 0;
	}
	~NodeStruct(){
		//
	}
	unsigned long start;
	unsigned long propsSize;
	unsigned long type;
	NodeStruct* next;
	NodeStruct* child;

	static void clearNet(NodeStruct* root){
		if(root){
			clearChild(root);
		}
	}

private:
	static void clearNext(NodeStruct* node){
		NodeStruct* deleteNode = node;
		NodeStruct* nextNode;
		while(deleteNode){
			if(deleteNode->child){
				clearChild(deleteNode->child);
			}
			nextNode = deleteNode->next;
			delete deleteNode;
			deleteNode = nextNode;
		}
	}

	static void clearChild(NodeStruct* node){
		if(node->child){
			clearChild(node->child);
		}
		if(node->next){
			clearNext(node->next);
		}
		delete node;
	}
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
	ERROR_CACHE_ERROR,
};

class PropStream;

class FileLoader{
public:
	FileLoader();
	virtual ~FileLoader();

	bool openFile(const char* filename, bool write, bool caching = false);
	const unsigned char* getProps(const NODE, unsigned long &size);
	bool getProps(const NODE, PropStream& props);
	const NODE getChildNode(const NODE parent, unsigned long &type);
	const NODE getNextNode(const NODE prev, unsigned long &type);

	void startNode(unsigned char type);
	void endNode();
	int setProps(void* data, unsigned short size);

	int getError(){return m_lastError;}
	void clearError(){m_lastError = ERROR_NONE;}

protected:
	enum SPECIAL_BYTES{
		NODE_START = 0xFE,
		NODE_END = 0xFF,
		ESCAPE_CHAR = 0xFD,
	};

	bool parseNode(NODE node);

	inline bool readByte(int &value);
	inline bool readBytes(unsigned char* buffer, unsigned int size, long pos);
	inline bool checks(const NODE node);
	inline bool safeSeek(unsigned long pos);
	inline bool safeTell(long &pos);

public:
	inline bool writeData(const void* data, int size, bool unescape){
		for(int i = 0; i < size; ++i) {
			unsigned char c = *(((unsigned char*)data) + i);
			if(unescape && (c == NODE_START || c == NODE_END || c == ESCAPE_CHAR)) {
				unsigned char escape = ESCAPE_CHAR;
				size_t value = fwrite(&escape, 1, 1, m_file);
				if(value != 1) {
					m_lastError = ERROR_COULDNOTWRITE;
					return false;
				}
			}
			size_t value = fwrite(&c, 1, 1, m_file);
			if(value != 1) {
				m_lastError = ERROR_COULDNOTWRITE;
				return false;
			}
		}

		return true;
	}
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

class PropStream{
public:
	PropStream(){end = NULL; p = NULL;}
	~PropStream(){};

	void init(const char* a, unsigned long size){
		p = a;
		end = a + size;
	}

	int64_t size(){return end-p;}

	template <typename T>
	inline bool GET_STRUCT(T* &ret){
		if(size() < (long)sizeof(T)){
			ret = NULL;
			return false;
		}
		ret = (T*)p;
		p = p + sizeof(T);
		return true;
	}

	template <typename T>
	inline bool GET_VALUE(T &ret){
		if(size() < (long)sizeof(T)){
			return false;
		}
		ret = *((T*)p);
		p = p + sizeof(T);
		return true;
	}

	inline bool GET_ULONG(uint32_t &ret){
		return GET_VALUE(ret);
	}

	inline bool GET_USHORT(uint16_t &ret){
		return GET_VALUE(ret);
	}

	inline bool GET_UCHAR(uint8_t &ret){
		return GET_VALUE(ret);
	}

	inline bool GET_STRING(std::string& ret){
		char* str;
		uint16_t str_len;

		if(!GET_USHORT(str_len)){
			return false;
		}
		if(size() < (int32_t)str_len){
			return false;
		}
		str = new char[str_len+1];
		memcpy(str, p, str_len);
		str[str_len] = 0;
		ret.assign(str, str_len);
		delete[] str;
		p = p + str_len;
		return true;
	}

	inline bool GET_LSTRING(std::string& ret){
		char* str;
		uint32_t str_len;

		if(!GET_ULONG(str_len)){
			return false;
		}
		if(size() < (int32_t)str_len){
			return false;
		}
		str = new char[str_len+1];
		memcpy(str, p, str_len);
		str[str_len] = 0;
		ret.assign(str, str_len);
		delete[] str;
		p = p + str_len;
		return true;
	}

	inline bool GET_NSTRING(unsigned short str_len, std::string& ret){
		char* str;

		if(size() < (int32_t)str_len){
			return false;
		}
		str = new char[str_len+1];
		memcpy(str, p, str_len);
		str[str_len] = 0;
		ret.assign(str, str_len); // String can contain 0s
		delete[] str;
		p = p + str_len;
		return true;
	}

	inline bool SKIP_N(int32_t n){
		if(size() < n){
			return false;
		}
		p = p + n;
		return true;
	}


protected:
	const char* p;
	const char* end;
};

class PropWriteStream{
public:
	PropWriteStream(){buffer = (char*)malloc(32*sizeof(char)); buffer_size = 32; size = 0; memset(buffer, 0, 32*sizeof(char));}
	~PropWriteStream(){free(buffer);};

	const char* getStream(uint32_t& _size) const{
		_size = size;
		return buffer;
	}

	//TODO: might need temp buffer and zero fill the memory chunk allocated by realloc
	template <typename T>
	inline void ADD_TYPE(T* add){
		if((buffer_size - size) < sizeof(T)){
			buffer_size = buffer_size + ((sizeof(T) + 0x1F) & 0xFFFFFFE0);
			buffer = (char*)realloc(buffer, buffer_size);
		}

		memcpy(&buffer[size], (char*)add, sizeof(T));
		size = size + sizeof(T);
	}

	template <typename T>
	inline void ADD_VALUE(T add){
		if((buffer_size - size) < sizeof(T)){
			buffer_size = buffer_size + ((sizeof(T) + 0x1F) & 0xFFFFFFE0);
			buffer = (char*)realloc(buffer,buffer_size);
		}

		memcpy(&buffer[size], &add, sizeof(T));
		size = size + sizeof(T);
	}

	inline void ADD_ULONG(uint32_t ret){
		ADD_VALUE(ret);
	}

	inline void ADD_USHORT(uint16_t ret){
		ADD_VALUE(ret);
	}

	inline void ADD_UCHAR(uint8_t ret){
		ADD_VALUE(ret);
	}

	inline void ADD_STRING(const std::string& add){
		uint16_t str_len = (uint16_t)add.size();

		ADD_USHORT(str_len);

		if((buffer_size - size) < str_len){
			buffer_size = buffer_size + ((str_len + 0x1F) & 0xFFFFFFE0);
			buffer = (char*)realloc(buffer, buffer_size);
		}

		memcpy(&buffer[size], add.c_str(), str_len);
		size = size + str_len;
	}

	inline void ADD_LSTRING(const std::string& add){
		uint32_t str_len = (uint32_t)add.size();

		ADD_ULONG(str_len);

		if((buffer_size - size) < str_len){
			buffer_size = buffer_size + ((str_len + 0x1F) & 0xFFFFFFE0);
			buffer = (char*)realloc(buffer, buffer_size);
		}

		memcpy(&buffer[size], add.c_str(), str_len);
		size = size + str_len;
	}

protected:
	char* buffer;
	uint32_t buffer_size;
	uint32_t size;
};

#endif

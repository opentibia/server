//////////////////////////////////////////////////////////////////////
// OTItemEditor
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
#include "stdio.h"

typedef unsigned long NODE;

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

	inline bool readByte(int &value);
	inline bool checks(const NODE node);
	inline bool safeSeek(unsigned long pos);
	inline bool safeTell(long &pos);
	//inline bool writeData(void* data, int size, bool unescape);
public:
	inline bool FileLoader::writeData(const void* data, int size, bool unescape){
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
	unsigned long m_buffer_size;
	unsigned char* m_buffer;

	bool m_use_cache;
	struct _cache{
		unsigned long loaded;
		unsigned long base;
		unsigned long size;
		unsigned char* data;
	};
	#define CACHE_BLOCKS 3
	unsigned long m_cache_size;
	_cache m_cached_data[CACHE_BLOCKS];
	long m_cache_index;
	long m_cache_offset;
	inline long getCacheBlock(unsigned long pos);
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

	template <typename T>
	inline bool GET_STRUCT(T* &ret){
		if(size() < sizeof(T)){
			ret = NULL;
			return false;
		}
		ret = (T*)p;
		p = p + sizeof(T);
		return true;
	}
	
	template <typename T>
	inline bool GET_VALUE(T &ret){
		if(size() < sizeof(T)){
			return false;
		}
		ret = *((T*)p);
		p = p + sizeof(T);
		return true;
	}
	
	inline bool GET_ULONG(unsigned long &ret){
		return GET_VALUE(ret);
	}
	
	inline bool GET_USHORT(unsigned short &ret){
		return GET_VALUE(ret);
	}
	
	inline bool GET_UCHAR(unsigned char &ret){
		return GET_VALUE(ret);
	}
	
	inline bool GET_STRING(std::string &ret){
		char* str;
		unsigned short str_len;
		
		if(!GET_USHORT(str_len)){
			return false;
		}
		if(size() < str_len){
			return false;
		}
		str = new char[str_len+1];
		memcpy(str, p, str_len);
		str[str_len] = 0;
		ret = str;
		delete[] str;
		p = p + str_len;
		return true;
	}
	
	
protected:
	long size(){return end - p;};
	const char* p;
	const char* end;
};

class PropWriteStream{
public:
	PropWriteStream(){buffer = (char*)malloc(32*sizeof(char)); buffer_size = 32; size = 0;}
	~PropWriteStream(){free(buffer);};

	char* getStream(unsigned long& _size) const{
		_size = size;
		return buffer;
	}

	template <typename T>
	inline bool ADD_TYPE(T* add){
		if((buffer_size - size) < sizeof(T)){
			buffer_size = buffer_size + ((sizeof(T) + 0x1F) & 0xFFFFFFE0);
			buffer = (char*)realloc(buffer, buffer_size);
		}

		memcpy(&buffer[size], (char*)add, sizeof(T));
		size = size + sizeof(T);

		return true;
	}
	
	template <typename T>
	inline bool ADD_VALUE(T add){
		if((buffer_size - size) < sizeof(T)){
			buffer_size = buffer_size + ((sizeof(T) + 0x1F) & 0xFFFFFFE0);
			buffer = (char*)realloc(buffer,buffer_size);
		}
		
		memcpy(&buffer[size], &add, sizeof(T));
		size = size + sizeof(T);
		
		return true;
	}
	
	inline bool ADD_ULONG(unsigned long ret){
		return ADD_VALUE(ret);
	}
	
	inline bool ADD_USHORT(unsigned short ret){
		return ADD_VALUE(ret);
	}
	
	inline bool ADD_UCHAR(unsigned char ret){
		return ADD_VALUE(ret);
	}
	
	inline bool ADD_STRING(const std::string& add){
		unsigned short str_len = add.size();
		
		if(!ADD_USHORT(str_len)){
			return false;
		}
		if((buffer_size - size) < str_len){
			buffer_size = buffer_size + ((str_len + 0x1F) & 0xFFFFFFE0);
			buffer = (char*)realloc(buffer, buffer_size);
		}
		memcpy(&buffer[size], add.c_str(), str_len);
		size = size + str_len;
		return true;
	}
	
	
protected:
	char* buffer;
	unsigned long buffer_size;
	unsigned long size;
};




#endif

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
#include "otpch.h"

#include "fileloader.h"

#ifndef min
#define min(a,b) ( a < b ? a : b)
#endif
#ifndef max
#define max(a,b) ( a > b ? a : b)
#endif

FileLoader::FileLoader()
{
	m_file = NULL;
	m_buffer = new unsigned char[1024];
	m_buffer_size = 1024;
	m_lastError = ERROR_NONE;
	//cache
	m_use_cache = false;
	m_cache_size = 0;
	m_cache_index = -1;
	m_cache_offset = -1;
	memset(m_cached_data, 0, sizeof(m_cached_data));
}


FileLoader::~FileLoader()
{
	if(m_file){
		fclose(m_file);
		m_file = NULL;
	}

	delete[] m_buffer;

	for(int i = 0; i < CACHE_BLOCKS; i++){
		if(m_cached_data[i].data)
			delete[] m_cached_data[i].data;
	}
}


bool FileLoader::openFile(const char* filename, bool write, bool caching /*= false*/)
{
	if(write) {
		m_file = fopen(filename, "wb");
		if(m_file) {
				unsigned long version = 0;
				writeData(&version, sizeof(version), false);
				return true;
		}
		else{
			m_lastError = ERROR_CAN_NOT_CREATE;
			return false;
		}
	}
	else {
		unsigned long version;
		m_file = fopen(filename, "rb");
		if(m_file){
			fread(&version, sizeof(unsigned long), 1, m_file);
			if(version > 0){
				fclose(m_file);
				m_lastError = ERROR_INVALID_FILE_VERSION;
				return false;
			}
			else{
				if(caching){
					m_use_cache = true;
					fseek(m_file, 0, SEEK_END);
					int file_size = ftell(m_file);
					m_cache_size = min(32768, max(file_size/20, 8192)) & ~0x1FFF;
				}
				return true;
			}
		}
		else{
			m_lastError = ERROR_CAN_NOT_OPEN;
			return false;
		}
	}
}

const unsigned char* FileLoader::getProps(const NODE node, unsigned long &size)
{
	if(!checks(node))
		return NULL;
	
	if(!safeSeek(node)){
		return NULL;
	}
	
	int byte, position;
	if(!readByte(byte))
		return NULL;

	if(byte != NODE_START){
		m_lastError = ERROR_INVALID_FORMAT;
		return NULL;
	}
	//read node type
	if(!readByte(byte))
		return NO_NODE;
	
	position = 0;
	while(1){
		if(!readByte(byte))
			return NULL;
		
		if(byte == NODE_END || byte ==NODE_START)
			break;
		else if(byte == ESCAPE_CHAR){
			if(!readByte(byte))
				return NULL;
		}
		if(position >= m_buffer_size){
			unsigned char *tmp = new unsigned char[m_buffer_size+1024];
			memcpy(tmp, m_buffer, m_buffer_size);
			m_buffer_size = m_buffer_size + 1024;
			delete[] m_buffer;
			m_buffer = tmp;
		}
		m_buffer[position] = byte;
		position++;
	}

	size = position;
	return m_buffer;
}

bool FileLoader::getProps(const NODE node, PropStream &props)
{
	unsigned long size;
	const unsigned char* a = getProps(node, size);
	if(!a){
		props.init(NULL, 0);
		return false;
	}
	else{
		props.init((char*)a, size);
		return true;
	}
}

int FileLoader::setProps(void* data, unsigned short size)
{
	//data
	if(!writeData(data, size, true))
		return getError();

	return ERROR_NONE;
}

void FileLoader::startNode(unsigned char type)
{
	unsigned char nodeBegin = NODE_START;
	writeData(&nodeBegin, sizeof(nodeBegin), false);
	writeData(&type, sizeof(type), true);
}

void FileLoader::endNode()
{
	unsigned char nodeEnd = NODE_END;
	writeData(&nodeEnd, sizeof(nodeEnd), false);
}

const NODE FileLoader::getChildNode(const NODE parent, unsigned long &type)
{
	if(!checks(1))
		return NO_NODE;

	int seek_pos, byte;
	long tmp;
	if(!parent){
		seek_pos = 4;
	}
	else{
		seek_pos = parent;
	}

	if(!safeSeek(seek_pos)){
		return NO_NODE;
	}

	if(!readByte(byte))
		return NO_NODE;

	if(byte != NODE_START){
		m_lastError = ERROR_INVALID_FORMAT;
		return NO_NODE;
	}

	if(!parent){
		if(safeTell(tmp)){
			if(!readByte(byte))
				return NO_NODE;
					
			type = byte;
			return (NODE)tmp;
		}
		else{
			return NO_NODE;
		}
	}
	else{
		if(!readByte(byte))
			return NO_NODE;
		
		while(1){
			if(!readByte(byte))
				return NO_NODE;
		
			if(byte == NODE_END){
				return NO_NODE;
			}
			else if(byte == NODE_START){
				if(safeTell(tmp)){
					if(!readByte(byte))
						return NO_NODE;
					
					type = byte;
					return (NODE)tmp;
				}
				else{
					return NO_NODE;
				}
			}
			else if(byte == ESCAPE_CHAR){
				if(!readByte(byte))
					return NO_NODE;
			}
		}
	}
	return NO_NODE;

}

const NODE FileLoader::getNextNode(const NODE prev, unsigned long &type)
{
	if(!checks(prev))
		return NO_NODE;

	if(!safeSeek(prev)){
		return NO_NODE;
	}

	int byte;
	long tmp;
	if(!readByte(byte))
		return NO_NODE;

	if(byte != NODE_START){
		m_lastError = ERROR_INVALID_FORMAT;
		return NO_NODE;
	}
	if(!readByte(byte))
		return NO_NODE;

	int level;
	level = 1;
	while(1){
		if(!readByte(byte))
			return NO_NODE;
		
		if(byte == NODE_END){
			level--;
			if(level == 0){
				if(!readByte(byte))
					return NO_NODE;
				
				if(byte == NODE_END){
					return NO_NODE;
				}
				else if(byte != NODE_START){
					m_lastError = ERROR_INVALID_FORMAT;
					return NO_NODE;
				}
				else{
					if(safeTell(tmp)){
						if(!readByte(byte))
							return NO_NODE;

						type = byte;
						return (NODE)tmp;
					}
					else{
						return NO_NODE;
					}
				}
			}
		}
		else if(byte == NODE_START){
			level++;
		}
		else if(byte == ESCAPE_CHAR){
			if(!readByte(byte))
				return NO_NODE;
		}
	}
	return NO_NODE;
}


inline bool FileLoader::readByte(int &value)
{
	if(m_use_cache){
		if(m_cache_index < 0){
			m_lastError = ERROR_CACHE_ERROR;
			return false;
		}
		if(m_cache_offset >= m_cached_data[m_cache_index].size){
			long pos = m_cache_offset + m_cached_data[m_cache_index].base;
			long tmp = getCacheBlock(pos);
			if(tmp < 0)
				return false;

			m_cache_index = tmp;
			m_cache_offset = pos - m_cached_data[m_cache_index].base;
		}
		value = m_cached_data[m_cache_index].data[m_cache_offset];
		m_cache_offset++;
		return true;
	}
	else{
		value = fgetc(m_file);
		if(value == EOF){
			m_lastError = ERROR_EOF;
			return false;
		}
		else
			return true;
	}
}
/*
inline bool FileLoader::writeData(void* data, int size, bool unescape)
{
	for(int i = 0; i < size; ++i) {
		unsigned char c = *(((unsigned char*)data) + i);

		if(unescape && (c == NODE_START || c == NODE_END || c == ESCAPE_CHAR)) {
			unsigned char escape = ESCAPE_CHAR;
			int value = fwrite(&escape, 1, 1, m_file);
			if(value != 1) {
				m_lastError = ERROR_COULDNOTWRITE;
				return false;
			}
		}

		int value = fwrite(&c, 1, 1, m_file);
		if(value != 1) {
			m_lastError = ERROR_COULDNOTWRITE;
			return false;
		}
	}

	return true;
}
*/
inline bool FileLoader::checks(const NODE node)
{
	if(!m_file){
		m_lastError = ERROR_NOT_OPEN;
		return false;
	}
	if(!node){
		m_lastError = ERROR_INVALID_NODE;
		return false;
	}

	return true;
}

inline bool FileLoader::safeSeek(unsigned long pos)
{
	if(m_use_cache){
		long i = getCacheBlock(pos);
		if(i < 0)
			return false;

		m_cache_index = i;
		m_cache_offset = pos - m_cached_data[i].base;
	}
	else{
		if(fseek(m_file, pos, SEEK_SET)){
			m_lastError = ERROR_SEEK_ERROR;
			return false;
		}
	}
	return true;
}



inline bool FileLoader::safeTell(long &pos)
{
	if(m_use_cache){
		if(m_cache_index < 0){
			m_lastError = ERROR_CACHE_ERROR;
			return false;
		}

		pos = m_cached_data[m_cache_index].base + m_cache_offset - 1;
		return true;
	}
	else{
		pos = ftell(m_file);
		if(pos == -1){
			m_lastError = ERROR_TELL_ERROR;
			return false;
		}
		else{
			pos = pos - 1;
			return true;
		}
	}
}

inline long FileLoader::getCacheBlock(unsigned long pos)
{
	bool found = false;
	long i;
	long base_pos = pos & ~(m_cache_size - 1);
	for(i = 0; i < CACHE_BLOCKS; i++){
		if(m_cached_data[i].loaded){
			if(m_cached_data[i].base == base_pos){
				found = true;
				break;
			}
		}
	}
	if(!found){
		i = loadCacheBlock(pos);
	}
	return i;
}

long FileLoader::loadCacheBlock(unsigned long pos)
{
	long i;
	long loading_cache = -1;
	long base_pos = pos & ~(m_cache_size - 1);
	for(i = 0; i < CACHE_BLOCKS; i++){
		if(!m_cached_data[i].loaded){
			loading_cache = i;
			break;
		}
	}
	if(loading_cache == -1){
		for(i = 0; i < CACHE_BLOCKS; i++){
			if(std::abs(((int32_t)(m_cached_data[i].base)) - base_pos) > 2*m_cache_size){
				loading_cache = i;
				break;
			}
		}
		if(loading_cache == -1){
			loading_cache = 0;
		}
	}
			
	if(m_cached_data[loading_cache].data == NULL){
		m_cached_data[loading_cache].data = new unsigned char[m_cache_size];
	}

	m_cached_data[loading_cache].base = base_pos;

	if(fseek(m_file, m_cached_data[loading_cache].base, SEEK_SET)){
		m_lastError = ERROR_SEEK_ERROR;
		return -1;
	}

	long size = fread(m_cached_data[loading_cache].data, 1, m_cache_size, m_file);
	m_cached_data[loading_cache].size = size;

	if(size < pos - m_cached_data[loading_cache].base){
		m_lastError = ERROR_SEEK_ERROR;
		return -1;
	}

	m_cached_data[loading_cache].loaded = 1;

	return loading_cache;
}

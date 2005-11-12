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
	ERROR_COULDNOTWRITE
};



class FileLoader{
public:
	FileLoader();
	virtual ~FileLoader();

	bool openFile(const char* filename, bool write);
	const unsigned char* getProps(const NODE, unsigned long &size);
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
	inline bool FileLoader::writeData(void* data, int size, bool unescape){
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


	FILE *m_file;
	FILELOADER_ERRORS m_lastError;
	unsigned long m_buffer_size;
	unsigned char *m_buffer;
};

#endif

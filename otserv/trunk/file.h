//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// File loads and saves binary files.
// Using a constructor, you can read a file into memory.
// Assigning files to each other, you get a second copy.
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
// $Id$
//////////////////////////////////////////////////////////////////////
// $Log$
// Revision 1.2  2002/04/05 20:02:23  acrimon
// *** empty log message ***
//
// Revision 1.1  2002/04/05 18:56:11  acrimon
// Adding a file class.
//
//////////////////////////////////////////////////////////////////////

#include <fstream.h>

class File {
private:
  bool error;
  long size;    // the size of the data
public:
  char *data;   // user accessable data
  File();
  File(const File& _file);
  File& operator=(const File& _file);
  File(char *filename);
  File(istream I);
  ~File();
  long getsize() { return size; }
  // void save();   // not yet written
};

class TextFile : public File {
  char *curpos;
  int marks;
public:
  TextFile();
  TextFile(const TextFile& _file);
  TextFile& operator=(const TextFile& _file);
  TextFile(char *filename);
  //TextFile(istream I);
  int splitlines(char ** &A, char delim = '\n');
  char *extractnextname();   // some complicated function
  int getmarks();
};

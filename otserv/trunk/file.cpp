//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// File loads and saves binary files.
// TextFile treats them as text files.
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
// Revision 1.1  2002/04/05 18:56:11  acrimon
// Adding a file class.
//
//////////////////////////////////////////////////////////////////////

#include <iostream.h>
#include "file.h"
#include "tools.h"

File::File() {
  error = false;
  data = NULL;
  size = 0;
}

File::File(const File& _file) {
  data = NULL;
  *this = _file;
}

File& File::operator=(const File& _file) {
  error = _file.error;
  size = _file.size;
  if (data) delete data;
  if (_file.data) {
    data = new char[size + 1];
    for (int i = size; i >= 0; i--)
      data[i] = _file.data[i];
  } else
    data = NULL;
  return *this;
}

File::File(char *filename) {
    ifstream I(filename);
    if (!I.good()) {
        cerr <<"Datei " <<filename <<" kann nicht gelesen werden\n";
        error = true;
	size = 0;
    }
    I.seekg(-1, ios::end);
    size = I.tellg();
    if (!size) {
      data = NULL;
      size = 0;
      error = false;
      return;
    }
    data = new char[size + 1];
    data[size] = '\0';
    I.seekg(0, ios::beg);
    I.read(data, size);
    if (!I.good()) {
        cerr << "Fehler beim Lesen von Datei " << filename << endl;
        error = true;
    }
    I.close();
    error = false;
}

// Ich weiß noch nicht ob das für cin funktioniert.
File::File(istream I) {
    if (!I.good()) {
        cerr <<"Eingabedatei kann nicht gelesen werden\n";
        error = true;
	size = 0;
    }
    I.seekg(-1, ios::end);
    size = I.tellg();
    if (!size) {
        cerr << "Eingabedatei ist leer.\n";
        error = true;
    }
    data = new char[size + 1];
    data[size] = '\0';
    I.seekg(0, ios::beg);
    I.read(data, size);
    if (!I.good()) {
        cerr << "Fehler beim Lesen der Eingabedatei." << endl;
        error = true;
    }
    error = false;
}

File::~File() {
  if (data) delete data;
}

TextFile::TextFile() : File() {
  curpos = data;
}

TextFile::TextFile(char *filename) : File(filename) {
  curpos = data;
}

TextFile::TextFile(const TextFile& _file) {
  data = NULL;
  *this = _file;
}

TextFile& TextFile::operator=(const TextFile& _file) {
  char *oldcurpos = _file.curpos;
  long relpos = _file.curpos - _file.data;
  *((File *) this) = *((File *) &_file);
  curpos = oldcurpos ? (data + relpos) : NULL;
  return *this;
}

// Aufsplitten der Datei zeilenweise in Array A
// (Speicher für A wird in der Funktion belegt)
// Rückgabewert: Anzahl der gelesenen Zeilen
// Wenn delim angegeben wird, dann wird er statt '\n' als Zeilentrenner genommen
int TextFile::splitlines(char ** &A, char delim = '\n') {
    int lines = 1;
    int i = 0;
    for (; i < size; i++) {   // Zeilen zählen
        if (data[i] == delim)
            data[i] = 0;
        if (data[i] == 0)
            lines++;
    }
    A = new char *[lines];
    A[0] = data;
    i = 0;
    for (int c = 1; c < lines; A[c++] = data + i) // und eintragen
        while (data[i++])
            ;
    return lines;
}

char *TextFile::extractnextname() {
  char *namebeg = curpos;
  bool inname = true;
  char *lastempty = NULL;
  bool seperator = false;
  int paren = 0;
  marks = 0;
  if (*curpos == '\0') {
    curpos = data;
    return NULL;
  }
  for (;;) {
    if (upchar(*curpos) >= 'A' && upchar(*curpos) <= 'Z' ||
	upchar(*curpos) == 'Ä' || upchar(*curpos) == 'Ö' || upchar(*curpos) == 'Ü' ||
	*curpos == 'ß' ||
	*curpos == '\'' || *curpos == '.') {
      curpos++;
    } else if (*curpos == ' ' || *curpos == '-') {
      if (lastempty + 1 == curpos) {
	lastempty[0] = '\0';
	curpos++;
	break;
      } else {
	lastempty = curpos;
	curpos++;
      }
    } else if (*curpos == '=' || *curpos == '<' || *curpos == '\n') {
      seperator = true;
      if (lastempty + 1 == curpos)
	lastempty[0] = '\0';
      else
	curpos[0] = '\0';
      curpos++;
      break;
    } else if (*curpos == '?') {
      curpos[0] = '\0';
      marks++;
      curpos++;
      break;
    } else if (*curpos == ':' || *curpos == ',') {
      curpos[0] = '\0';
      curpos++;
      break;
    } else if (*curpos == '(') {
      paren++;
      if (lastempty + 1 == curpos)
	lastempty[0] = '\0';
      else
	curpos[0] = '\0';
      curpos++;
      break;
    } else if (*curpos == '\0') {
      return namebeg;
    } else {
      cout << "char " << *curpos << " occured inname" << endl;
      curpos++;
    }
  }
  if (!seperator) for (;;) {
    while (paren) {
      if (*curpos == '(')
	paren++;
      else if (*curpos == ')')
	paren--;
      curpos++;
    }
    if (*curpos == '=' || *curpos == '<' || *curpos == '\n') {
      curpos++;
      break;
    } else if (*curpos == '(') {
      paren++;
    } else if (*curpos == '\0') {
      return namebeg;
    } else if (*curpos == '?') {
      marks++;
    }
    curpos++;
  }
  for (;;) {
    while (paren) {
      if (*curpos == '(')
	paren++;
      else if (*curpos == ')')
	paren--;
      curpos++;
    }
    if (*curpos == '(') {
      paren++;
      curpos++;
    } else if (*curpos == '\0') {
      return namebeg;
    } else if (upchar(*curpos) >= 'A' && upchar(*curpos) <= 'Z' ||
	       upchar(*curpos) == 'Ä' || upchar(*curpos) == 'Ö' || upchar(*curpos) == 'Ü') {
      return namebeg;
    } else
      curpos++;
  }
  
  return namebeg;
}

int TextFile::getmarks() {
  return marks;
}

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


#ifndef texcept_h
#define texcept_h

#include <string>

class texception {
    public:
        texception(std::string description, bool critical)
            : desc(description), crit(critical) {
            } // texception(string description, boolean critical) 

        texception(bool critical) : crit(critical) {
        } // texception(bool critical) 

        bool isCritical() { return crit; }

        std::string toString() { return desc; }

    protected:
        std::string desc;
        bool crit;
}; // class texception 

class nospace : public texception {
    public:
        nospace() : texception("no space on map!",false) {
        } // nospace() : crit(false), desc("no space on map!") 
}; // class nospace : public texception 

class wrong_creature : public texception {
    public:
        wrong_creature() : texception("wrong creature index!", false) {
        } // wrong_creature() : texception("wrong creature index!", false) 
}; // class wrong_creature : public texception 

#endif

//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Account Loader/Saver implemented through XML
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


#ifndef __OTSERV_IOACCOUNTXML_H__
#define __OTSERV_IOACCOUNTXML_H__

#include <string>
#include "definitions.h"
#include "ioaccount.h"

/** Player-Loaders implemented with XML */
class IOAccountXML : protected IOAccount {
  public:
	/** Get a textual description of what source is used
	  * \returns Name of the source*/
	virtual char* getSourceDescription(){return "XML";};
	virtual Account loadAccount(unsigned long accno);
		virtual bool getPassword(unsigned long accno, const std::string &name, std::string &password);
	IOAccountXML();
	virtual ~IOAccountXML(){};
};

#endif

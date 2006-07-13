//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Base class for the map serialization
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

#include "definitions.h"
#include "iomapserialize.h"

#if defined USE_SQL_ENGINE
#include "iomapserializesql.h"
#else
#include "iomapserializexml.h"
#endif

#include "iomapserializexml.h"

IOMapSerialize* IOMapSerialize::_instance = NULL;

IOMapSerialize* IOMapSerialize::getInstance()
{
	if(!_instance){
#if defined USE_SQL_ENGINE
		_instance = new IOMapSerializeSQL;
#else
		_instance = new IOMapSerializeXML;
#endif
	}

	return _instance;
}

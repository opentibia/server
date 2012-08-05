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

#ifndef __OTSERV_MOUNT_H__
#define __OTSERV_MOUNT_H__

#include "definitions.h"
#include "enums.h"
#include <vector>
#include <string>
#include <map>
#include <list>

struct Mount{
	Mount() : mountId(0), lookType(0), speed(0), attackSpeed(0), isPremium(false), isDefault(false), name("") {}
	uint32_t mountId;
	uint32_t lookType;
	int32_t speed;
	int32_t attackSpeed;
	bool isPremium;
	bool isDefault;
	std::string name;
};

typedef std::map<uint32_t, Mount > MountMap;

class Mounts
{
public:
	~Mounts();

	static Mounts* getInstance()
	{
		static Mounts instance;
		return &instance;
	}

	bool loadFromXml(const std::string& datadir);
	bool reload();

	bool getMount(uint32_t value, Mount& mount, bool isId = false);
	MountMap getMounts() {return mounts;}

private:
	Mounts();

	std::string m_datadir;
	MountMap mounts;
};

#endif

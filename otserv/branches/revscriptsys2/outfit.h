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

#ifndef __OTSERV_OUTFIT_H__
#define __OTSERV_OUTFIT_H__

#include "enums.h"

struct OutfitType
{
	OutfitType(){
		lookHead   = 0;
		lookBody   = 0;
		lookLegs   = 0;
		lookFeet   = 0;
		lookType   = 0;
		lookTypeEx = 0;
		lookAddons = 0;
	}

	uint32_t lookType;
	uint32_t lookTypeEx;
	uint32_t lookHead;
	uint32_t lookBody;
	uint32_t lookLegs;
	uint32_t lookFeet;
	uint32_t lookAddons;
};

struct Outfit
{
	Outfit() : outfitId(0), lookType(0), addons(0), isPremium(false), isDefault(false), name("") {}
	uint32_t outfitId;
	uint32_t lookType;
	uint32_t addons;
	bool isPremium;
	bool isDefault;
	std::string name;
};

typedef std::list<Outfit> OutfitList;
typedef std::map<uint32_t, Outfit > OutfitMap;

class Outfits
{
public:
	~Outfits();

	static Outfits* getInstance()
	{
		static Outfits instance;
		return &instance;
	}

	bool loadFromXml(const std::string& datadir);

	uint32_t getOutfitId(uint32_t lookType);
	bool getOutfit(uint32_t lookType, Outfit& outfit);
	bool getOutfit(uint32_t outfitId, PlayerSex sex, Outfit& outfit);
	const OutfitMap& getOutfits(PlayerSex playersex);

private:
	Outfits();
	OutfitList allOutfits;
	std::map<PlayerSex, OutfitMap > outfitMaps;
};

#endif

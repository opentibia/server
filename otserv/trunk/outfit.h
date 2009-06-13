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

#include <vector>
#include <string>
#include <map>
#include "enums.h"

struct Outfit{
	Outfit() : outfitId(0), lookType(0), addons(0), premium(false), name("") {}
	uint32_t outfitId;
	uint32_t lookType;
	uint32_t addons;
	bool premium;
	std::string name;
};

//typedef std::map<uint32_t, OutfitList > OutfitMap;

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

	uint32_t Outfits::getOutfitId(uint32_t lookType);
	bool getOutfit(uint32_t lookType, Outfit& outfit);
	bool getOutfit(uint32_t outfitId, playersex_t sex, Outfit& outfit);
	const OutfitMap& getOutfits(playersex_t playersex);
	
private:
	Outfits();
	OutfitList allOutfits;
	OutfitMap femaleMap;
	OutfitMap maleMap;
};

#endif

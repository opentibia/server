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

#include "definitions.h"
#include "enums.h"
#include <vector>
#include <string>
#include <map>
#include <list>

struct Outfit{
	Outfit();
	
	uint32_t outfitId;
	uint32_t lookType;
	uint32_t addons;
	bool isPremium;
	bool isDefault;
	std::string name;
};

typedef std::list<Outfit> OutfitList;
typedef std::map<uint32_t, Outfit> OutfitMap;

class Outfits {
	Outfits();
	
public:
	static Outfits* getInstance();

	bool loadFromXml(const std::string& datadir);
	bool reload();

	const uint32_t& getOutfitId(const uint32_t& lookType);
	bool getOutfit(const uint32_t& lookType, Outfit& outfit);
	bool getOutfit(const uint32_t& outfitId, const PlayerSex_t& sex, Outfit& outfit);
	const OutfitMap& getOutfits(const PlayerSex_t& playersex);

private:
	std::string m_datadir;
	OutfitList allOutfits;
	std::map<PlayerSex_t, OutfitMap > outfitMaps;
};

#endif

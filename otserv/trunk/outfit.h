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

#include <list>
#include <vector>
#include <string>

struct Outfit{
    uint32_t looktype;
    uint32_t addons;
};

typedef std::list<Outfit*> OutfitListType;

class OutfitList
{
public:
	OutfitList();
	~OutfitList();

	void addOutfit(const Outfit& outfit);
	const OutfitListType& getOutfits() const;
	bool isInList(uint32_t looktype, uint32_t addons) const;
	
private:
	OutfitListType m_list;
};

class Outfits
{
public:
	~Outfits();
	
	static Outfits* getInstance();
	
	bool loadFromXml(const std::string& datadir);
	const OutfitListType& getOutfits(uint32_t type);
	const OutfitList& getOutfitList(uint32_t type);
	
private:
	Outfits();
	static Outfits* _instance;
	typedef std::vector<OutfitList*> OutfitsListVector;
	OutfitsListVector m_list;
	
	OutfitList m_female_list;
	OutfitList m_male_list;
};

#endif

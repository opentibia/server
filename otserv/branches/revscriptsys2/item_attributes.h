//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
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


#ifndef __OTSERV_ITEM_ATTRIBUTES_H__
#define __OTSERV_ITEM_ATTRIBUTES_H__

#include "definitions.h"
#include <string>
#include <map>

class PropWriteStream;
class PropStream;

class ItemAttribute
{
public:
	ItemAttribute();
	ItemAttribute(const std::string& str);
	ItemAttribute(int32_t i);
	ItemAttribute(float f);
	ItemAttribute(bool b);
	ItemAttribute(const ItemAttribute& o);
	ItemAttribute& operator=(const ItemAttribute& o);
	~ItemAttribute();

	void serialize(PropWriteStream& stream) const;
	bool unserialize(PropStream& stream);

	void clear();

	void set(const std::string& str);
	void set(int32_t i);
	void set(float f);
	void set(bool b);

	const std::string* getString() const;
	const int32_t* getInteger() const;
	const float* getFloat() const;
	const bool* getBoolean() const;
	boost::any get() const;
	
private:
	enum Type {
		STRING = 1,
		INTEGER = 2,
		FLOAT = 3,
		BOOLEAN = 4,
		NONE = 0
	} type;
	char data[sizeof std::string];
};

class ItemAttributes
{
public:
	ItemAttributes();
	ItemAttributes(const ItemAttributes &i);
	virtual ~ItemAttributes();

	// Save / load
	void serializeAttributeMap(PropWriteStream& stream) const;
	bool unserializeAttributeMap(PropStream& stream);

public:
	void setAttribute(const std::string& key, const std::string& value);
	void setAttribute(const std::string& key, int32_t value);
	void setAttribute(const std::string& key, float value);
	void setAttribute(const std::string& key, bool set);
	// returns NULL if the attribute is not set
	const std::string* getStringAttribute(const std::string& key) const;
	const int32_t* getIntegerAttribute(const std::string& key) const;
	const float* getFloatAttribute(const std::string& key) const;
	const bool* getBooleanAttribute(const std::string& key) const;
	boost::any getAttribute(const std::string& key) const;
	// Returns true if the attribute (of that type) exists
	bool hasStringAttribute(const std::string& key) const;
	bool hasIntegerAttribute(const std::string& key) const;
	bool hasFloatAttribute(const std::string& key) const;
	bool hasBooleanAttribute(const std::string& key) const;


	void eraseAttribute(const std::string& key);

protected:

	typedef std::map<std::string, ItemAttribute> AttributeMap;
	AttributeMap* attributes;

	void createAttributes();
};

#endif

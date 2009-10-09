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

#include "otpch.h"

#include "item_attributes.h"
#include "fileloader.h"

ItemAttributes::ItemAttributes() :
	attributes(NULL)
{
}

ItemAttributes::ItemAttributes(const ItemAttributes& o)
{
	if(o.attributes)
		attributes = new AttributeMap(*o.attributes);
}

ItemAttributes::~ItemAttributes()
{
	delete attributes;
}

void ItemAttributes::createAttributes()
{
	if(!attributes)
		attributes = new AttributeMap;
}

void ItemAttributes::setAttribute(const std::string& key, const std::string& value)
{
	createAttributes();
	(*attributes)[key].set(value);
}

void ItemAttributes::setAttribute(const std::string& key, int32_t value)
{
	createAttributes();
	(*attributes)[key].set(value);
}

void ItemAttributes::setAttribute(const std::string& key, float value)
{
	createAttributes();
	(*attributes)[key].set(value);
}

void ItemAttributes::setAttribute(const std::string& key, bool value)
{
	createAttributes();
	(*attributes)[key].set(value);
}

void ItemAttributes::eraseAttribute(const std::string& key)
{
	if(!attributes)
		return;

	AttributeMap::iterator iter = attributes->find(key);

	if(iter != attributes->end())
		attributes->erase(iter);
}

const std::string* ItemAttributes::getStringAttribute(const std::string& key) const
{
	if(!attributes)
		return NULL;

	AttributeMap::iterator iter = attributes->find(key);
	if(iter != attributes->end())
		return iter->second.getString();
	return NULL;
}

const int32_t* ItemAttributes::getIntegerAttribute(const std::string& key) const
{
	if(!attributes)
		return NULL;

	AttributeMap::iterator iter = attributes->find(key);
	if(iter != attributes->end())
		return iter->second.getInteger();
	return NULL;
}

const float* ItemAttributes::getFloatAttribute(const std::string& key) const
{
	if(!attributes)
		return NULL;

	AttributeMap::iterator iter = attributes->find(key);
	if(iter != attributes->end())
		return iter->second.getFloat();
	return NULL;
}

const bool* ItemAttributes::getBooleanAttribute(const std::string& key) const
{
	if(!attributes)
		return NULL;

	AttributeMap::iterator iter = attributes->find(key);
	if(iter != attributes->end())
		return iter->second.getBoolean();
	return NULL;
}

boost::any ItemAttributes::getAttribute(const std::string& key) const
{
	if(!attributes)
		return boost::any();

	AttributeMap::iterator iter = attributes->find(key);
	if(iter != attributes->end())
		return iter->second.get();
	return boost::any();
}

bool ItemAttributes::hasStringAttribute(const std::string& key) const
{
	return getStringAttribute(key) != NULL;
}

bool ItemAttributes::hasIntegerAttribute(const std::string& key) const
{
	return getIntegerAttribute(key) != NULL;
}

bool ItemAttributes::hasFloatAttribute(const std::string& key) const
{
	return getFloatAttribute(key) != NULL;
}

bool ItemAttributes::hasBooleanAttribute(const std::string& key) const
{
	return getBooleanAttribute(key) != NULL;
}


// Attribute type
// Can hold either int, bool or std::string
// Without using new to allocate them

ItemAttribute::ItemAttribute() : type(ItemAttribute::NONE)
{
}


ItemAttribute::ItemAttribute(const std::string& str) : type(ItemAttribute::STRING)
{
	new(data) std::string(str);
}

ItemAttribute::ItemAttribute(int32_t i) : type(ItemAttribute::INTEGER)
{
	*reinterpret_cast<int*>(data) = i;
}

ItemAttribute::ItemAttribute(float f) : type(ItemAttribute::FLOAT)
{
	*reinterpret_cast<float*>(data) = f;
}

ItemAttribute::ItemAttribute(bool b)
{
	*reinterpret_cast<bool*>(data) = b;
}

ItemAttribute::ItemAttribute(const ItemAttribute& o) : type(ItemAttribute::NONE)
{
	*this = o;
}

ItemAttribute& ItemAttribute::operator=(const ItemAttribute& o)
{
	if(&o == this)
		return *this;

	clear();
	type = o.type;
	if(type == STRING)
		new(data) std::string(*reinterpret_cast<const std::string*>(&o.data));
	else if(type == INTEGER)
		*reinterpret_cast<int32_t*>(data) = *reinterpret_cast<const int32_t*>(&o.data);
	else if(type == FLOAT)
		*reinterpret_cast<float*>(data) = *reinterpret_cast<const float*>(&o.data);
	else if(type == BOOLEAN)
		*reinterpret_cast<bool*>(data) = *reinterpret_cast<const bool*>(&o.data);
	else
		type = NONE;

	return *this;

}

ItemAttribute::~ItemAttribute()
{
	clear();
}

void ItemAttribute::clear()
{
	if(type == STRING){
		(reinterpret_cast<std::string*>(&data))->~basic_string();
		type = NONE;
	}
}

void ItemAttribute::set(const std::string& str)
{
	clear();
	type = STRING;
	new(data) std::string(str);
}

void ItemAttribute::set(int32_t i)
{
	clear();
	type = INTEGER;
	*reinterpret_cast<int32_t*>(&data) = i;
}

void ItemAttribute::set(float y)
{
	clear();
	type = FLOAT;
	*reinterpret_cast<float*>(&data) = y;
}

void ItemAttribute::set(bool b)
{
	clear();
	type = BOOLEAN;
	*reinterpret_cast<bool*>(&data) = b;
}


const std::string* ItemAttribute::getString() const
{
	if(type == STRING)
		return reinterpret_cast<const std::string*>(&data);
	return NULL;
}

const int32_t* ItemAttribute::getInteger() const
{
	if(type == INTEGER)
		return reinterpret_cast<const int32_t*>(&data);
	return NULL;
}

const float* ItemAttribute::getFloat() const
{
	if(type == FLOAT)
		return reinterpret_cast<const float*>(&data);
	return NULL;
}

const bool* ItemAttribute::getBoolean() const
{
	if(type == BOOLEAN)
		return reinterpret_cast<const bool*>(&data);
	return NULL;
}

boost::any ItemAttribute::get() const
{
	switch(type){
		case STRING:
			return *reinterpret_cast<const std::string*>(&data);
		case INTEGER:
			return *reinterpret_cast<const int*>(&data);
		case FLOAT:
			return *reinterpret_cast<const float*>(&data);
		case BOOLEAN:
			return *reinterpret_cast<const bool*>(&data);
		default:
			break;
	}
	return boost::any();
}

bool ItemAttributes::unserializeAttributeMap(PropStream& stream)
{

	uint16_t n;
	if(stream.GET_USHORT(n)){
		createAttributes();

		std::string key;
		ItemAttribute attrib;

		while(n--){
			if(!stream.GET_STRING(key))
				return false;
			if(!attrib.unserialize(stream))
				return false;
			(*attributes)[key] = attrib;
		}
	}
	return true;
}

void ItemAttributes::serializeAttributeMap(PropWriteStream& stream) const
{
	// Maximum of 65535 attributes per item
	stream.ADD_USHORT(std::min((size_t)0xFFFF, attributes->size()));

	AttributeMap::const_iterator attribute = attributes->begin();
	int i = 0;
	while(attribute != attributes->end() && i <= 0xFFFF){
		const std::string& key = attribute->first;
		if(key.size() > 0xFFFF)
			stream.ADD_STRING(key.substr(0, 65535));
		else
			stream.ADD_STRING(key);

		attribute->second.serialize(stream);
		++attribute, ++i;
	}
}

bool ItemAttribute::unserialize(PropStream& stream)
{
	// Read type
	uint8_t rtype;
	stream.GET_UCHAR(rtype);

	// Read contents
	switch(rtype){
		case STRING:
		{
			std::string str;
			if(!stream.GET_LSTRING(str))
				return false;
			set(str);
			break;
		}
		case INTEGER:
			uint32_t u32;
			if(!stream.GET_ULONG(u32))
				return false;
			set(*reinterpret_cast<int32_t*>(&u32));
			break;
		case FLOAT:
		{
			uint32_t u32;
			if(!stream.GET_ULONG(u32))
				return false;
			set(*reinterpret_cast<float*>(&u32));
			break;
		}
		case BOOLEAN:
		{
			uint8_t b;
			if(!stream.GET_UCHAR(b))
				return false;
			set(b != 0);
		}
		default:
			break;
	}
	return true;
}

void ItemAttribute::serialize(PropWriteStream& stream) const
{
	// Write type
	stream.ADD_UCHAR((unsigned char)(type));

	// Write contents
	switch(type){
		case STRING:
			stream.ADD_LSTRING(*getString());
			break;
		case INTEGER:
			stream.ADD_ULONG(*(uint32_t*)getInteger());
			break;
		case FLOAT:
			stream.ADD_ULONG(*(uint32_t*)getFloat());
			break;
		case BOOLEAN:
			stream.ADD_UCHAR(*(uint8_t*)getBoolean());
		default:
			break;
	}
}

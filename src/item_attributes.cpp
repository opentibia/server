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

// attribute type
// can hold either int, bool or std::string
// without using new to allocate them

ItemAttribute::ItemAttribute() : m_type(ItemAttribute::NONE)
{

}

ItemAttribute::ItemAttribute(const std::string& v) : m_type(ItemAttribute::STRING)
{
  m_var.string = new std::string(v);
}

ItemAttribute::ItemAttribute(int32_t v) : m_type(ItemAttribute::INTEGER)
{
  m_var.signedInteger = v;
}

ItemAttribute::ItemAttribute(float v) : m_type(ItemAttribute::FLOAT)
{
  m_var.signedFloat = v;
}

ItemAttribute::ItemAttribute(bool v) : m_type(ItemAttribute::BOOLEAN)
{
  m_var.boolean = v;
}

ItemAttribute::ItemAttribute(const ItemAttribute& o) : m_type(ItemAttribute::NONE)
{
  *this = o;
}

ItemAttribute& ItemAttribute::operator=(const ItemAttribute& o)
{
  if(&o == this)
    return *this;

  m_type = o.m_type;
  m_var = o.m_var;

  return *this;
}

ItemAttribute::~ItemAttribute()
{
  dealloc();
}

void ItemAttribute::dealloc(){
  if(m_type == ItemAttribute::STRING)
    delete m_var.string;
}

void ItemAttribute::set(const std::string& v)
{
  dealloc();
  
  m_type = STRING;
  m_var.string = new std::string(v);
}

void ItemAttribute::set(int32_t v)
{
  dealloc();
  
  m_type = INTEGER;
  m_var.signedInteger = v;
}

void ItemAttribute::set(float v)
{
  dealloc();
  
  m_type = FLOAT;
  m_var.signedFloat = v;
}

void ItemAttribute::set(bool v)
{
  dealloc();
  
  m_type = BOOLEAN;
  m_var.boolean = v;
}

const std::string* ItemAttribute::getString() const
{
  if(m_type == STRING){
    return m_var.string;
  }
  
  return NULL;
}

const int32_t* ItemAttribute::getInteger() const
{
  if(m_type == INTEGER)
    return &m_var.signedInteger;
  
  return NULL;
}

const float* ItemAttribute::getFloat() const
{
  if(m_type == FLOAT)
    return &m_var.signedFloat;
  
  return NULL;
}

const bool* ItemAttribute::getBoolean() const
{
  if(m_type == BOOLEAN)
    return &m_var.boolean;
  
  return NULL;
}

boost::any ItemAttribute::get() const
{
  switch(m_type){
    case STRING: return *m_var.string;
    case INTEGER: return m_var.signedInteger;
    case FLOAT: return m_var.signedFloat;
    case BOOLEAN: return m_var.boolean;
    default: break;
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
  // maximum of 65535 attributes per item
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
  // dealloc possible string value
  dealloc();
  
  // read type
  stream.GET_UCHAR(reinterpret_cast<uint8_t&>(m_type));
  
  // do not call here set(...) or any other function depending on m_type which may result in deallocating phantom string !

  // read contents
  switch(m_type){
    case STRING: {
      m_var.string = new std::string;   
      if(!stream.GET_LSTRING(*m_var.string))
        return false;
      
      break;
    }
    
    case INTEGER: 
    case FLOAT: {
      if(!stream.GET_ULONG(m_var.unsignedInteger))
        return false;
      
      break;
    }
    
    case BOOLEAN: {
      if(!stream.GET_UCHAR(m_var.unsignedChar))
        return false;
        
      break;
    }
    
    default: {
      m_type = NONE; 
      break;
    }
  }
  
  return true;
}

void ItemAttribute::serialize(PropWriteStream& stream) const
{
  // write type
  stream.ADD_UCHAR(static_cast<uint8_t>(m_type));

  // write contents
  switch(m_type){
    case STRING: {
      stream.ADD_LSTRING(*m_var.string);
      break;
    }
    
    case INTEGER:
    case FLOAT: {
      stream.ADD_ULONG(m_var.unsignedInteger);
      break;
    }
    
    case BOOLEAN: {
      stream.ADD_UCHAR(m_var.unsignedChar);
      break;
    }
    
    default: break;
  }
}

/* -*- C++ -*- */

/****************************************************************************
** Copyright (c) 2001-2014
**
** This file is part of the QuickFIX FIX Engine
**
** This file may be distributed under the terms of the quickfixengine.org
** license as defined by quickfixengine.org and appearing in the file
** LICENSE included in the packaging of this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.quickfixengine.org/LICENSE for licensing information.
**
** Contact ask@quickfixengine.org if any conditions of this licensing are
** not clear to you.
**
****************************************************************************/

#ifndef FIX_DATADICTIONARY_H
#define FIX_DATADICTIONARY_H

#ifdef _MSC_VER
#pragma warning( disable : 4503 4355 4786 4290 )
#endif

#include "Fields.h"
#include "FieldMap.h"
#include "DOMDocument.h"
#include "Exceptions.h"
#include <set>
#include <map>
#include <string.h>
#include "ValidationRules.h"

namespace FIX
{
class FieldMap;
class Message;

/**
 * Represents a data dictionary for a version of %FIX.
 *
 * Generally loaded from an XML document.  The DataDictionary is also
 * responsible for validation beyond the basic structure of a message.
 */

class DataDictionary
{
  typedef std::set < int > MsgFields;
  typedef std::map < std::string, MsgFields > MsgTypeToField;
  typedef std::set < std::string > MsgTypes;
  typedef std::set < int > Fields;
  typedef std::map < int, bool > NonBodyFields;
  typedef std::vector< int > OrderedFields;
  typedef message_order OrderedFieldsArray;
  typedef std::map < int, TYPE::Type > FieldTypes;
  typedef std::set < std::string > Values;
  typedef std::map < int, Values > FieldToValue;
  typedef std::map < int, std::string > FieldToName;
  typedef std::map < std::string, int > NameToField;
  typedef std::map < std::pair < int, std::string > , std::string  > ValueToName;
  // while FieldToGroup structure seems to be overcomplicated
  // in reality it yields a lot of performance because:
  // 1) avoids memory copying;
  // 2) first lookup is done by comparing integers and not string objects
  // TODO: use hash_map with good hashing algorithm
  typedef std::map < std::string, std::pair < int, DataDictionary* > > FieldPresenceMap;
  typedef std::map < int, FieldPresenceMap > FieldToGroup;

public:
  DataDictionary();
  DataDictionary( const DataDictionary& copy );
  DataDictionary( std::istream& stream ) throw( ConfigError );
  DataDictionary( const std::string& url ) throw( ConfigError );
  virtual ~DataDictionary();

  void readFromURL( const std::string& url ) throw( ConfigError );
  void readFromDocument( DOMDocumentPtr pDoc ) throw( ConfigError );
  void readFromStream( std::istream& stream ) throw( ConfigError );

  message_order const& getOrderedFields() const;

  // storage functions
  void setVersion( const std::string& beginString )
  {
    m_beginString = beginString;
    m_hasVersion = true;
  }
  std::string getVersion() const
  {
    return m_beginString.getString();
  }

  void addField( int field )
  {
    m_fields.insert( field );
    m_orderedFields.push_back( field );
  }

  void addFieldName( int field, const std::string& name )
  {
    if( m_names.insert( std::make_pair(name, field) ).second == false )
      throw ConfigError( "Field named " + name + " defined multiple times" );
    m_fieldNames[field] = name;
  }

  bool getFieldName( int field, std::string& name ) const
  {
    FieldToName::const_iterator i = m_fieldNames.find( field );
    if(i == m_fieldNames.end()) return false;
    name = i->second;
    return true;
  }

  bool getFieldTag( const std::string& name, int& field ) const
  {
    NameToField::const_iterator i = m_names.find( name );
    if(i == m_names.end()) return false;
    field = i->second;
    return true;
  }

  void addValueName( int field, const std::string& value, const std::string& name )
  {
    m_valueNames[std::make_pair(field, value)] = name;
  }

  bool getValueName( int field, const std::string& value, std::string& name ) const
  {
    ValueToName::const_iterator i = m_valueNames.find( std::make_pair(field, value) );
    if(i == m_valueNames.end()) return false;
    name = i->second;
    return true;
  }

  bool isField( int field ) const
  {
    return m_fields.find( field ) != m_fields.end();
  }

  void addMsgType( const std::string& msgType )
  {
    m_messages.insert( msgType );
  }

  bool isMsgType( const std::string& msgType ) const
  {
    return m_messages.find( msgType ) != m_messages.end();
  }

  void addMsgField( const std::string& msgType, int field )
  {
    m_messageFields[ msgType ].insert( field );
  }

  bool isMsgField( const std::string& msgType, int field ) const
  {
    MsgTypeToField::const_iterator i = m_messageFields.find( msgType );
    if ( i == m_messageFields.end() ) return false;
    return i->second.find( field ) != i->second.end();
  }

  void addHeaderField( int field, bool required )
  {
    m_headerFields[ field ] = required;
  }

  bool isHeaderField( int field ) const
  {
    return m_headerFields.find( field ) != m_headerFields.end();
  }

  void addTrailerField( int field, bool required )
  {
    m_trailerFields[ field ] = required;
  }

  bool isTrailerField( int field ) const
  {
    return m_trailerFields.find( field ) != m_trailerFields.end();
  }

  void addFieldType( int field, FIX::TYPE::Type type )
  {
    m_fieldTypes[ field ] = type;

    if( type == FIX::TYPE::Data )
      m_dataFields.insert( field );
  }

  bool getFieldType( int field, FIX::TYPE::Type& type ) const
  {
    FieldTypes::const_iterator i = m_fieldTypes.find( field );
    if ( i == m_fieldTypes.end() ) return false;
    type = i->second;
    return true;
  }

  void addRequiredField( const std::string& msgType, int field )
  {
    m_requiredFields[ msgType ].insert( field );
  }

  bool isRequiredField( const std::string& msgType, int field ) const
  {
    MsgTypeToField::const_iterator i = m_requiredFields.find( msgType );
    if ( i == m_requiredFields.end() ) return false;
    return i->second.find( field ) != i->second.end();
  }

  void addFieldValue( int field, const std::string& value )
  {
    m_fieldValues[ field ].insert( value );
  }

  bool hasFieldValue( int field ) const
  {
    FieldToValue::const_iterator i = m_fieldValues.find( field );
    return i != m_fieldValues.end();
  }

  bool isFieldValue( int field, const std::string& value ) const;

  void addGroup( const std::string& msg, int field, int delim,
                 const DataDictionary& dataDictionary )
  {
    DataDictionary * pDD = new DataDictionary( dataDictionary );
    pDD->setVersion( getVersion() );

    FieldPresenceMap& presenceMap = m_groups[ field ];
    presenceMap[ msg ] = std::make_pair( delim, pDD );
  }

  bool isGroup( const std::string& msg, int field ) const;

  bool getGroup( const std::string& msg, int field, int& delim,
                 const DataDictionary*& pDataDictionary ) const;

  bool isDataField( int field ) const
  {
    MsgFields::const_iterator iter = m_dataFields.find( field );
    return iter != m_dataFields.end();
  }

  bool isMultipleValueField( int field ) const
  {
    FieldTypes::const_iterator i = m_fieldTypes.find( field );
    return i != m_fieldTypes.end() 
      && (i->second == TYPE::MultipleValueString 
          || i->second == TYPE::MultipleCharValue 
          || i->second == TYPE::MultipleStringValue );
  }

  /// Validate a message.
  static void validate( int direction,
                        const Message& message,
                        const DataDictionary* const pSessionDD,
                        const DataDictionary* const pAppID,
                        const ValidationRules* vrptr ) throw( FIX::Exception );

  void validate( int direction, const Message& message, const ValidationRules* vrptr = 0 ) const throw ( FIX::Exception )
  { validate( direction, message, false, vrptr ); }
  void validate( int direction, const Message& message, bool bodyOnly, const ValidationRules* vrptr = 0 ) const throw( FIX::Exception )
  { validate( direction, message, bodyOnly ? (DataDictionary*)0 : this, this, vrptr ); }

  DataDictionary& operator=( const DataDictionary& rhs );

private:
  /// Iterate through fields while applying checks.
  void iterate( int direction, const FieldMap& map, const MsgType& msgType, const ValidationRules* vrptr = 0 ) const;

  /// Check if message type is defined in spec.
  void checkMsgType( int direction, const MsgType& msgType, const ValidationRules* vrptr ) const
  throw( InvalidMessageType );

  /// If we need to check for the tag in the dictionary
  bool shouldCheckTag( const std::string& msgType, const FieldBase& field ) const
  {
    if( field.getTag() >= FIELD::UserMin )
      return false;
    else
      return true;
  }

  /// Check if field tag number is defined in spec.
  void checkValidTagNumber( int direction, const MsgType& msgType, const FieldBase& field, const ValidationRules* vrptr = 0 ) const
  throw( InvalidTagNumber );
  void checkValidFormat( int direction, const std::string& msgType, const FieldBase& field, const ValidationRules* vrptr = 0 ) const
  throw( IncorrectDataFormat );

  void checkValue( int direction, const std::string& msgType, const FieldBase& field, const ValidationRules* vrptr = 0 ) const
  throw( IncorrectTagValue );

  /// Check if a field has a value.
  void checkHasValue( int direction, const std::string& msgType, const FieldBase& field, const ValidationRules* vrptr = 0 ) const
  throw( NoTagValue );

  /// Check if a field is in this message type.
  void checkIsInMessage ( int direction, const FieldBase& field, const MsgType& msgType, const ValidationRules* vrptr = 0 ) const
  throw( TagNotDefinedForMessage );

  /// Check if group count matches number of groups in
  void checkGroupCount
  ( int direction, const FieldBase& field, const FieldMap& fieldMap, const MsgType& msgType, const ValidationRules* vrptr = 0 ) const
  throw( RepeatingGroupCountMismatch );

  /// Check if a message has all required fields.
  void checkHasRequired
  ( int direction, const FieldMap& header, const FieldMap& body, const FieldMap& trailer,
    const MsgType& msgType,
    const ValidationRules* vrptr ) const
  throw( RequiredTagMissing );

  int lookupXMLFieldNumber( DOMDocument*, DOMNode* ) const;
  int lookupXMLFieldNumber( DOMDocument*, const std::string& name ) const;
  int addXMLComponentFields( DOMDocument*, DOMNode*, const std::string& msgtype, DataDictionary&, bool );
  void addXMLGroup( DOMDocument*, DOMNode*, const std::string& msgtype, DataDictionary&, bool );
  TYPE::Type XMLTypeToType( const std::string& xmlType ) const;

  bool m_hasVersion;
  //bool m_checkFieldsOutOfOrder;
  //bool m_checkFieldsHaveValues;
  //bool m_checkUserDefinedFields;
  BeginString m_beginString;
  MsgTypeToField m_messageFields;
  MsgTypeToField m_requiredFields;
  MsgTypes m_messages;
  Fields m_fields;
  OrderedFields m_orderedFields;
  mutable OrderedFieldsArray m_orderedFieldsArray;
  NonBodyFields m_headerFields;
  NonBodyFields m_trailerFields;
  FieldTypes m_fieldTypes;
  FieldToValue m_fieldValues;
  FieldToName m_fieldNames;
  NameToField m_names;
  ValueToName m_valueNames;
  FieldToGroup m_groups;
  MsgFields m_dataFields;
};
}

#endif //FIX_DATADICTIONARY_H

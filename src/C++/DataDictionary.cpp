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

#ifdef _MSC_VER
#include "stdafx.h"
#else
#include "config.h"
#endif

#include "DataDictionary.h"
#include "Message.h"
#include <fstream>
#include <memory>

#include "PUGIXML_DOMDocument.h"

#ifdef _MSC_VER
#define RESET_AUTO_PTR(OLD, NEW) OLD = NEW;
#else
#define RESET_AUTO_PTR(OLD, NEW) OLD.reset( NEW.release() );
#endif

namespace FIX
{
DataDictionary::DataDictionary()
: m_hasVersion( false )
{}

DataDictionary::DataDictionary( std::istream& stream )
throw( ConfigError )
: m_hasVersion( false )
{
  readFromStream( stream );
}

DataDictionary::DataDictionary( const std::string& url )
throw( ConfigError )
: m_hasVersion( false ),
  m_orderedFieldsArray(0)
{
  readFromURL( url );
}

DataDictionary::DataDictionary( const DataDictionary& copy )
{
  *this = copy;
}

DataDictionary::~DataDictionary()
{
  FieldToGroup::iterator i;
  for ( i = m_groups.begin(); i != m_groups.end(); ++i )
  {
    const FieldPresenceMap& presenceMap = i->second;

    FieldPresenceMap::const_iterator iter = presenceMap.begin();
    for ( ; iter != presenceMap.end(); ++iter )
      delete iter->second.second;
  }
}

DataDictionary& DataDictionary::operator=( const DataDictionary& rhs )
{
  m_hasVersion = rhs.m_hasVersion;
  m_beginString = rhs.m_beginString;
  m_messageFields = rhs.m_messageFields;
  m_requiredFields = rhs.m_requiredFields;
  m_messages = rhs.m_messages;
  m_fields = rhs.m_fields;
  m_orderedFields = rhs.m_orderedFields;
  m_orderedFieldsArray = rhs.m_orderedFieldsArray;
  m_headerFields = rhs.m_headerFields;
  m_trailerFields = rhs.m_trailerFields;
  m_fieldTypes = rhs.m_fieldTypes;
  m_fieldValues = rhs.m_fieldValues;
  m_fieldNames = rhs.m_fieldNames;
  m_names = rhs.m_names;
  m_valueNames = rhs.m_valueNames;
  m_dataFields = rhs.m_dataFields;

  FieldToGroup::const_iterator i = rhs.m_groups.begin();
  for ( ; i != rhs.m_groups.end(); ++i )
  {
    const FieldPresenceMap& presenceMap = i->second;

    FieldPresenceMap::const_iterator iter = presenceMap.begin();
    for ( ; iter != presenceMap.end(); ++iter )
    {
      addGroup( iter->first, i->first, iter->second.first, *iter->second.second );
  }
  }
  return *this;
}

void DataDictionary::validate( int direction,
                               const Message& message,
                               const DataDictionary* const pSessionDD,
                               const DataDictionary* const pAppDD,
                               const ValidationRules* vrptr )
throw( FIX::Exception )
{  
  if ( !ValidationRules::shouldValidate(vrptr) ) 
    return;
  const Header& header = message.getHeader();
  const BeginString& beginString = FIELD_GET_REF( header, BeginString );
  const MsgType& msgType = FIELD_GET_REF( header, MsgType );
  if ( pSessionDD != 0 && pSessionDD->m_hasVersion )
  {
    if( pSessionDD->getVersion() != beginString )
    {
      if( !ValidationRules::shouldTolerateVersionMismatch( vrptr, direction) )
        throw UnsupportedVersion();
    }
  }

  int tag = 0;

  if ( pAppDD != 0 && pAppDD->m_hasVersion )
  {
    pAppDD->checkMsgType( direction, msgType, vrptr );
    if ( !message.hasValidStructure(tag) && !ValidationRules::shouldTolerateOutOfOrderTag( vrptr, direction, msgType, tag ) )
      throw TagOutOfOrder(tag);
    pAppDD->checkHasRequired( direction, message.getHeader(), message, message.getTrailer(), msgType, vrptr );
  }

  if( pSessionDD != 0 )
  {
    pSessionDD->iterate( direction, message.getHeader(), msgType, vrptr );
    pSessionDD->iterate( direction, message.getTrailer(), msgType, vrptr );
  }

  if( pAppDD != 0 )
  {
    pAppDD->iterate( direction, message, msgType, vrptr );
  }
}

void DataDictionary::iterate( int direction, const FieldMap& map, const MsgType& msgType, const ValidationRules* vrptr ) const
{
  if ( !ValidationRules::shouldValidate(vrptr) ) 
    return;
  int lastField = 0;

  FieldMap::iterator i;
  for ( i = map.begin(); i != map.end(); ++i )
  {
    const FieldBase& field = i->second;
    if( i != map.begin() && (field.getTag() == lastField) )
      throw RepeatedTag( lastField );
    checkHasValue( direction, msgType, field, vrptr );

    if ( m_hasVersion )
    {
      checkValidFormat( direction, msgType, field, vrptr );
      checkValue( direction, msgType, field, vrptr );
    }

    if ( m_beginString.getValue().length() && shouldCheckTag( msgType, field, vrptr ) )
    {
      checkValidTagNumber( direction, msgType, field, vrptr );
      if ( !Message::isHeaderField( field, this )
           && !Message::isTrailerField( field, this ) )
      {
        checkIsInMessage( direction, field, msgType, vrptr );
        checkGroupCount( direction, field, map, msgType );
      }
    }
    lastField = field.getTag();
  }
}

void DataDictionary::readFromURL( const std::string& url )
throw( ConfigError )
{
  DOMDocumentPtr pDoc = DOMDocumentPtr(new PUGIXML_DOMDocument());

  if(!pDoc->load(url))
    throw ConfigError(url + ": Could not parse data dictionary file");

  try
  {
    readFromDocument( pDoc );
  }
  catch( ConfigError& e )
  {
    throw ConfigError( url + ": " + e.what() );
  }
}

void DataDictionary::readFromStream( std::istream& stream )
throw( ConfigError )
{
  DOMDocumentPtr pDoc = DOMDocumentPtr(new PUGIXML_DOMDocument());

  if(!pDoc->load(stream))
    throw ConfigError("Could not parse data dictionary stream");

  readFromDocument( pDoc );
}

void DataDictionary::readFromDocument( DOMDocumentPtr pDoc )
throw( ConfigError )
{
  // VERSION
  DOMNodePtr pFixNode = pDoc->getNode("/fix");
  if(!pFixNode.get())
    throw ConfigError("Could not parse data dictionary file"
                      ", or no <fix> node found at root");
  DOMAttributesPtr attrs = pFixNode->getAttributes();
  std::string type = "FIX";
  if(attrs->get("type", type))
  {
    if(type != "FIX" && type != "FIXT")
      throw ConfigError("type attribute must be FIX or FIXT");
  }
  std::string major;
  if(!attrs->get("major", major))
    throw ConfigError("major attribute not found on <fix>");
  std::string minor;
  if(!attrs->get("minor", minor))
    throw ConfigError("minor attribute not found on <fix>");
  setVersion(type + "." + major + "." + minor);

  // FIELDS
  DOMNodePtr pFieldsNode = pDoc->getNode("/fix/fields");
  if(!pFieldsNode.get())
    throw ConfigError("<fields> section not found in data dictionary");

  DOMNodePtr pFieldNode = pFieldsNode->getFirstChildNode();
  if(!pFieldNode.get()) throw ConfigError("No fields defined");

  while(pFieldNode.get())
  {
    if(pFieldNode->getName() == "field")
    {
      DOMAttributesPtr attrs = pFieldNode->getAttributes();
      std::string name;
      if(!attrs->get("name", name))
        throw ConfigError("<field> does not have a name attribute");
      std::string number;
      if(!attrs->get("number", number))
        throw ConfigError("<field> " + name + " does not have a number attribute");
      int num = atoi(number.c_str());
      std::string type;
      if(!attrs->get("type", type))
        throw ConfigError("<field> " + name + " does not have a type attribute");
      addField(num);
      addFieldType(num, XMLTypeToType(type));
      addFieldName(num, name);

      DOMNodePtr pFieldValueNode = pFieldNode->getFirstChildNode();
      while(pFieldValueNode.get())
      {
        if(pFieldValueNode->getName() == "value")
        {
          DOMAttributesPtr attrs = pFieldValueNode->getAttributes();
          std::string enumeration;
          if(!attrs->get("enum", enumeration))
            throw ConfigError("<value> does not have enum attribute in field " + name);
          addFieldValue(num, enumeration);
          std::string description;
          if(attrs->get("description", description))
            addValueName(num, enumeration, description);
        }
        RESET_AUTO_PTR(pFieldValueNode, pFieldValueNode->getNextSiblingNode());
      }
    }
    RESET_AUTO_PTR(pFieldNode, pFieldNode->getNextSiblingNode());
  }

  // HEADER
  if( type == "FIXT" || (type == "FIX" && major < "5") )
  {
    DOMNodePtr pHeaderNode = pDoc->getNode("/fix/header");
    if(!pHeaderNode.get())
      throw ConfigError("<header> section not found in data dictionary");

    DOMNodePtr pHeaderFieldNode = pHeaderNode->getFirstChildNode();
    if(!pHeaderFieldNode.get()) throw ConfigError("No header fields defined");

    while(pHeaderFieldNode.get())
    {
      if(pHeaderFieldNode->getName() == "field" || pHeaderFieldNode->getName() == "group" )
      {
        DOMAttributesPtr attrs = pHeaderFieldNode->getAttributes();
        std::string name;
        if(!attrs->get("name", name))
          throw ConfigError("<field> does not have a name attribute");
        std::string required = "false";
        attrs->get("required", required);
        addHeaderField(lookupXMLFieldNumber(pDoc.get(), name), required == "true");
      }
      if(pHeaderFieldNode->getName() == "group")
      {
        DOMAttributesPtr attrs = pHeaderFieldNode->getAttributes();
        std::string required;
        attrs->get("required", required);
        bool isRequired = (required == "Y" || required == "y");
        addXMLGroup(pDoc.get(), pHeaderFieldNode.get(), "_header_", *this, isRequired);
      }

      RESET_AUTO_PTR(pHeaderFieldNode, pHeaderFieldNode->getNextSiblingNode());
    }
  }

  // TRAILER
    if( type == "FIXT" || (type == "FIX" && major < "5") )
    {
    DOMNodePtr pTrailerNode = pDoc->getNode("/fix/trailer");
    if(!pTrailerNode.get())
      throw ConfigError("<trailer> section not found in data dictionary");

    DOMNodePtr pTrailerFieldNode = pTrailerNode->getFirstChildNode();
    if(!pTrailerFieldNode.get()) throw ConfigError("No trailer fields defined");

    while(pTrailerFieldNode.get())
    {
      if(pTrailerFieldNode->getName() == "field" || pTrailerFieldNode->getName() == "group" )
      {
        DOMAttributesPtr attrs = pTrailerFieldNode->getAttributes();
        std::string name;
        if(!attrs->get("name", name))
          throw ConfigError("<field> does not have a name attribute");
        std::string required = "false";
        attrs->get("required", required);
        addTrailerField(lookupXMLFieldNumber(pDoc.get(), name), required == "true");
      }
      if(pTrailerFieldNode->getName() == "group")
      {
        DOMAttributesPtr attrs = pTrailerFieldNode->getAttributes();
        std::string required;
        attrs->get("required", required);
        bool isRequired = (required == "Y" || required == "y");
        addXMLGroup(pDoc.get(), pTrailerFieldNode.get(), "_trailer_", *this, isRequired);
      }

      RESET_AUTO_PTR(pTrailerFieldNode, pTrailerFieldNode->getNextSiblingNode());
    }
  }

  // MSGTYPE
  DOMNodePtr pMessagesNode = pDoc->getNode("/fix/messages");
  if(!pMessagesNode.get())
    throw ConfigError("<messages> section not found in data dictionary");

  DOMNodePtr pMessageNode = pMessagesNode->getFirstChildNode();
  if(!pMessageNode.get()) throw ConfigError("No messages defined");

  while(pMessageNode.get())
  {
    if(pMessageNode->getName() == "message")
    {
      DOMAttributesPtr attrs = pMessageNode->getAttributes();
      std::string msgtype;
      if(!attrs->get("msgtype", msgtype))
        throw ConfigError("<field> does not have a name attribute");
      addMsgType(msgtype);

      std::string name;
      if(attrs->get("name", name))
        addValueName( 35, msgtype, name );

      DOMNodePtr pMessageFieldNode = pMessageNode->getFirstChildNode();
      while( pMessageFieldNode.get() )
      {
        if(pMessageFieldNode->getName() == "field"
           || pMessageFieldNode->getName() == "group")
        {
          DOMAttributesPtr attrs = pMessageFieldNode->getAttributes();
          std::string name;
          if(!attrs->get("name", name))
            throw ConfigError("<field> does not have a name attribute");
          int num = lookupXMLFieldNumber(pDoc.get(), name);
          addMsgField(msgtype, num);

          std::string required;
          if(attrs->get("required", required)
             && (required == "Y" || required == "y"))
          {
            addRequiredField(msgtype, num);
          }
        }
        else if(pMessageFieldNode->getName() == "component")
        {
          DOMAttributesPtr attrs = pMessageFieldNode->getAttributes();
          std::string required;
          attrs->get("required", required);
          bool isRequired = (required == "Y" || required == "y");
          addXMLComponentFields(pDoc.get(), pMessageFieldNode.get(),
                                msgtype, *this, isRequired);
        }
        if(pMessageFieldNode->getName() == "group")
        {
          DOMAttributesPtr attrs = pMessageFieldNode->getAttributes();
          std::string required;
          attrs->get("required", required);
          bool isRequired = (required == "Y" || required == "y");
          addXMLGroup(pDoc.get(), pMessageFieldNode.get(), msgtype, *this, isRequired);
        }
        RESET_AUTO_PTR(pMessageFieldNode,
                       pMessageFieldNode->getNextSiblingNode());
      }
    }
    RESET_AUTO_PTR(pMessageNode, pMessageNode->getNextSiblingNode());
  }
}

message_order const& DataDictionary::getOrderedFields() const
{
  if( m_orderedFieldsArray ) return m_orderedFieldsArray;

  int * tmp = new int[m_orderedFields.size() + 1];
  int * i = tmp;

  OrderedFields::const_iterator iter;
  for( iter = m_orderedFields.begin(); iter != m_orderedFields.end(); *(i++) = *(iter++) ) {}
  *i = 0;

  m_orderedFieldsArray = message_order(tmp);
  delete [] tmp;

  return m_orderedFieldsArray;
}

int DataDictionary::lookupXMLFieldNumber( DOMDocument* pDoc, DOMNode* pNode ) const
{
  DOMAttributesPtr attrs = pNode->getAttributes();
  std::string name;
  if(!attrs->get("name", name))
    throw ConfigError("No name given to field");
  return lookupXMLFieldNumber( pDoc, name );
}

int DataDictionary::lookupXMLFieldNumber
( DOMDocument* pDoc, const std::string& name ) const
{
  NameToField::const_iterator i = m_names.find(name);
  if( i == m_names.end() )
    throw ConfigError("Field " + name + " not defined in fields section");
  return i->second;
}

int DataDictionary::addXMLComponentFields( DOMDocument* pDoc, DOMNode* pNode,
                                            const std::string& msgtype,
                                            DataDictionary& DD,
                                            bool componentRequired )
{
  int firstField = 0;

  DOMAttributesPtr attrs = pNode->getAttributes();
  std::string name;
  if(!attrs->get("name", name))
    throw ConfigError("No name given to component");

  DOMNodePtr pComponentNode =
    pDoc->getNode("/fix/components/component[@name='" + name + "']");
  if(pComponentNode.get() == 0)
    throw ConfigError("Component not found");

  DOMNodePtr pComponentFieldNode = pComponentNode->getFirstChildNode();
  while(pComponentFieldNode.get())
  {
    if(pComponentFieldNode->getName() == "field"
       || pComponentFieldNode->getName() == "group")
    {
      DOMAttributesPtr attrs = pComponentFieldNode->getAttributes();
      std::string name;
      if(!attrs->get("name", name))
        throw ConfigError("No name given to field");
      int field = lookupXMLFieldNumber(pDoc, name);
      if( firstField == 0 ) firstField = field;

      std::string required;
      if(attrs->get("required", required)
         && (required == "Y" || required =="y")
         && componentRequired)
      {
        addRequiredField(msgtype, field);
      }

      DD.addField(field);
      DD.addMsgField(msgtype, field);
    }
    if(pComponentFieldNode->getName() == "component")
    {
      DOMAttributesPtr attrs = pComponentFieldNode->getAttributes();
      std::string required;
      attrs->get("required", required);
      bool isRequired = (required == "Y" || required == "y");
      addXMLComponentFields(pDoc, pComponentFieldNode.get(),
                            msgtype, DD, isRequired);
    }
    if(pComponentFieldNode->getName() == "group")
    {
      DOMAttributesPtr attrs = pComponentFieldNode->getAttributes();
      std::string required;
      attrs->get("required", required);
      bool isRequired = (required == "Y" || required == "y");
      addXMLGroup(pDoc, pComponentFieldNode.get(), msgtype, DD, isRequired);
    }
    RESET_AUTO_PTR(pComponentFieldNode,
      pComponentFieldNode->getNextSiblingNode());
  }
  return firstField;
}

void DataDictionary::addXMLGroup( DOMDocument* pDoc, DOMNode* pNode,
                                  const std::string& msgtype,
                                  DataDictionary& DD, bool groupRequired  )
{
  DOMAttributesPtr attrs = pNode->getAttributes();
  std::string name;
  if(!attrs->get("name", name))
    throw ConfigError("No name given to group");
  int group = lookupXMLFieldNumber( pDoc, name );
  int delim = 0;
  int field = 0;
  DataDictionary groupDD;
  DOMNodePtr node = pNode->getFirstChildNode();
  while(node.get())
  {
    if( node->getName() == "field" )
    {
      field = lookupXMLFieldNumber( pDoc, node.get() );
      groupDD.addField( field );

      DOMAttributesPtr attrs = node->getAttributes();
      std::string required;
      if( attrs->get("required", required)
         && ( required == "Y" || required =="y" )
         && groupRequired )
      {
        groupDD.addRequiredField(msgtype, field);
      }
    }
    else if( node->getName() == "component" )
    {
      field = addXMLComponentFields( pDoc, node.get(), msgtype, groupDD, false );
    }
    else if( node->getName() == "group" )
    {
      field = lookupXMLFieldNumber( pDoc, node.get() );
      groupDD.addField( field );
      DOMAttributesPtr attrs = node->getAttributes();
      std::string required;
      if( attrs->get("required", required )
         && ( required == "Y" || required =="y" )
         && groupRequired)
      {
        groupDD.addRequiredField(msgtype, field);
      }
      bool isRequired = false;
      if( attrs->get("required", required) )
      isRequired = (required == "Y" || required == "y");
      addXMLGroup( pDoc, node.get(), msgtype, groupDD, isRequired );
    }
    if( delim == 0 ) delim = field;
    RESET_AUTO_PTR(node, node->getNextSiblingNode());
  }

  if( delim ) DD.addGroup( msgtype, group, delim, groupDD );
}

TYPE::Type DataDictionary::XMLTypeToType( const std::string& type ) const
{
  if ( m_beginString < "FIX.4.2" && type == "CHAR" )
    return TYPE::String;

  if ( type == "STRING" ) return TYPE::String;
  if ( type == "CHAR" ) return TYPE::Char;
  if ( type == "PRICE" ) return TYPE::Price;
  if ( type == "INT" ) return TYPE::Int;
  if ( type == "AMT" ) return TYPE::Amt;
  if ( type == "QTY" ) return TYPE::Qty;
  if ( type == "CURRENCY" ) return TYPE::Currency;
  if ( type == "MULTIPLEVALUESTRING" ) return TYPE::MultipleValueString;
  if ( type == "MULTIPLESTRINGVALUE" ) return TYPE::MultipleStringValue;
  if ( type == "MULTIPLECHARVALUE" ) return TYPE::MultipleCharValue;
  if ( type == "EXCHANGE" ) return TYPE::Exchange;
  if ( type == "UTCTIMESTAMP" ) return TYPE::UtcTimeStamp;
  if ( type == "BOOLEAN" ) return TYPE::Boolean;
  if ( type == "LOCALMKTDATE" ) return TYPE::LocalMktDate;
  if ( type == "DATA" ) return TYPE::Data;
  if ( type == "FLOAT" ) return TYPE::Float;
  if ( type == "PRICEOFFSET" ) return TYPE::PriceOffset;
  if ( type == "MONTHYEAR" ) return TYPE::MonthYear;
  if ( type == "DAYOFMONTH" ) return TYPE::DayOfMonth;
  if ( type == "UTCDATE" ) return TYPE::UtcDate;
  if ( type == "UTCDATEONLY" ) return TYPE::UtcDateOnly;
  if ( type == "UTCTIMEONLY" ) return TYPE::UtcTimeOnly;
  if ( type == "NUMINGROUP" ) return TYPE::NumInGroup;
  if ( type == "PERCENTAGE" ) return TYPE::Percentage;
  if ( type == "SEQNUM" ) return TYPE::SeqNum;
  if ( type == "LENGTH" ) return TYPE::Length;
  if ( type == "COUNTRY" ) return TYPE::Country;
  if ( type == "TIME" ) return TYPE::UtcTimeStamp;
  return TYPE::Unknown;
}

bool DataDictionary::isFieldValue( int field, const std::string& value ) const
{
  FieldToValue::const_iterator i = m_fieldValues.find( field );
  if ( i == m_fieldValues.end() )
  {
    return false;
  }
  if( !isMultipleValueField( field ) )
  {
    return i->second.find( value ) != i->second.end();
  }

  // MultipleValue
  std::string::size_type startPos = 0;
  std::string::size_type endPos = 0;
  do
  {
    endPos = value.find_first_of(' ', startPos);
    std::string singleValue =
      value.substr( startPos, endPos - startPos );
    if( i->second.find( singleValue ) == i->second.end() )
      return false;
    startPos = endPos + 1;
  } while( endPos != std::string::npos );
  return true;
}

bool DataDictionary::isGroup( const std::string& msg, int field ) const
{
  FieldToGroup::const_iterator i = m_groups.find( field );
  if ( i == m_groups.end() ) return false;

  const FieldPresenceMap& presenceMap = i->second;

  FieldPresenceMap::const_iterator iter = presenceMap.find( msg );
  return ( iter != presenceMap.end() );
}

bool DataDictionary::getGroup( const std::string& msg, int field, int& delim,
               const DataDictionary*& pDataDictionary ) const
{
  FieldToGroup::const_iterator i = m_groups.find( field );
  if ( i == m_groups.end() ) 
    return false;

  const FieldPresenceMap& presenceMap = i->second;

  FieldPresenceMap::const_iterator iter = presenceMap.find( msg );
  if( iter == presenceMap.end() )
    return false;

  std::pair < int, DataDictionary* > pair = iter->second;
  delim = pair.first;
  pDataDictionary = pair.second;
  return true;
}

void DataDictionary::checkMsgType( int direction, const MsgType& msgType, const ValidationRules* vrptr ) const
throw( InvalidMessageType )
{
  if ( !isMsgType( msgType.getValue() ) && !ValidationRules::shouldTolerateMissingMessageType( vrptr, direction ) )
    throw InvalidMessageType();
}

void DataDictionary::checkValidTagNumber( int direction, const MsgType& msgType, const FieldBase& field, const ValidationRules* vrptr ) const
throw( InvalidTagNumber )
{
  //copy of checkIsInMessage
  if( m_fields.find( field.getTag() ) == m_fields.end() )
  {
    if ( ValidationRules::shouldTolerateUnknownTag( vrptr, direction, msgType, field.getTag() ) ) 
      return;
    throw InvalidTagNumber( field.getTag(), field.getTagAsString());
  }
}

void DataDictionary::checkValue( int direction, const std::string& msgType, const FieldBase& field, const ValidationRules* vrptr ) const
throw( IncorrectTagValue )
{
  if ( !hasFieldValue( field.getTag() ) ) return ;

  const std::string& value = field.getString();
  if ( !isFieldValue( field.getTag(), value ) )
  {
    if( !ValidationRules::shouldTolerateTagValue( vrptr, direction, msgType, field.getTag() ) )
      throw IncorrectTagValue( field.getTag(), field.getTagAsString());
  }
}

void DataDictionary::checkHasValue( int direction, const std::string& msgType, const FieldBase& field, const ValidationRules* vrptr ) const
throw( NoTagValue )
{
  if ( !field.getString().length() && !ValidationRules::shouldTolerateEmptyTag( vrptr, direction, msgType, field.getTag() ) )
    throw NoTagValue( field.getTag(), field.getTagAsString());
}

void DataDictionary::checkIsInMessage (
  int direction,
  const FieldBase& field,
  const MsgType& msgType,
  const ValidationRules* vrptr ) const
throw( TagNotDefinedForMessage )
{
  if ( !isMsgField( msgType, field.getTag() ) )
  {
    if ( ! ValidationRules::shouldTolerateUnknownTag( vrptr, direction, msgType, field.getTag() ) )
      throw TagNotDefinedForMessage( field.getTag(), field.getTagAsString());
  }
}

void DataDictionary::checkGroupCount (
  int direction,
  const FieldBase& field,
  const FieldMap& fieldMap,
  const MsgType& msgType,
  const ValidationRules* vrptr ) const
throw( RepeatingGroupCountMismatch )
{
  int fieldNum = field.getTag();
  if( isGroup(msgType, fieldNum) )
  {
    if( (int)fieldMap.groupCount(fieldNum)
      != IntConvertor::convert(field.getString()) &&
      ! ValidationRules::shouldTolerateRepeatingGroupCountMismatch( vrptr, direction, msgType, field.getTag() )
      )
    {
      throw RepeatingGroupCountMismatch(fieldNum, IntConvertor::convert(fieldNum) );
    }
  }
}

void DataDictionary::checkHasRequired
( int direction, const FieldMap& header, const FieldMap& body, const FieldMap& trailer,
  const MsgType& msgType,
  const ValidationRules* vrptr ) const
throw( RequiredTagMissing )
{
  NonBodyFields::const_iterator iNBF;
  for( iNBF = m_headerFields.begin(); iNBF != m_headerFields.end(); ++iNBF )
  {
    if( iNBF->second == true && !header.isSetField(iNBF->first) )
    {
      if ( ! ValidationRules::shouldTolerateMissingTag( vrptr, direction, msgType, iNBF->first ) ) 
        throw RequiredTagMissing( iNBF->first, IntConvertor::convert(iNBF->first) );
    }
  }

  for( iNBF = m_trailerFields.begin(); iNBF != m_trailerFields.end(); ++iNBF )
  {
    if( iNBF->second == true && !trailer.isSetField(iNBF->first) )
      if ( ! ValidationRules::shouldTolerateMissingTag( vrptr, direction, msgType, iNBF->first ) ) 
        throw RequiredTagMissing( iNBF->first, IntConvertor::convert(iNBF->first) );
  }

  MsgTypeToField::const_iterator iM
    = m_requiredFields.find( msgType.getString() );
  if ( iM == m_requiredFields.end() ) return ;

  const MsgFields& fields = iM->second;
  MsgFields::const_iterator iF;
  for( iF = fields.begin(); iF != fields.end(); ++iF )
  {
    if( !body.isSetField(*iF) )
      if ( ! ValidationRules::shouldTolerateMissingTag( vrptr, direction, msgType, *iF ) ) 
        throw RequiredTagMissing( *iF, IntConvertor::convert(*iF) );
  }

  FieldMap::g_iterator groups;
  for( groups = body.g_begin(); groups != body.g_end(); ++groups )
  {
    int delim;
    const DataDictionary* DD = 0;
    int field = groups->first;
    if( getGroup( msgType.getValue(), field, delim, DD ) )
    {
      std::vector<FieldMap*>::const_iterator group;
      for( group = groups->second.begin(); group != groups->second.end(); ++group )
        DD->checkHasRequired( direction, **group, **group, **group, msgType, vrptr );
    }
  }
}

void DataDictionary::checkValidFormat( int direction, const std::string& msgType, const FieldBase& field, const ValidationRules* vrptr ) const
  throw( IncorrectDataFormat )
{
  try
  {
    TYPE::Type type = TYPE::Unknown;
    getFieldType( field.getTag(), type );
    switch ( type )
    {
    case TYPE::String:
      STRING_CONVERTOR::convert( field.getString() ); break;
    case TYPE::Char:
      CHAR_CONVERTOR::convert( field.getString() ); break;
    case TYPE::Price:
      PRICE_CONVERTOR::convert( field.getString() ); break;
    case TYPE::Int:
      INT_CONVERTOR::convert( field.getString() ); break;
    case TYPE::Amt:
      AMT_CONVERTOR::convert( field.getString() ); break;
    case TYPE::Qty:
      QTY_CONVERTOR::convert( field.getString() ); break;
    case TYPE::Currency:
      CURRENCY_CONVERTOR::convert( field.getString() ); break;
    case TYPE::MultipleValueString:
      MULTIPLEVALUESTRING_CONVERTOR::convert( field.getString() ); break;
    case TYPE::MultipleStringValue:
      MULTIPLESTRINGVALUE_CONVERTOR::convert( field.getString() ); break;
    case TYPE::MultipleCharValue:
      MULTIPLECHARVALUE_CONVERTOR::convert( field.getString() ); break;
    case TYPE::Exchange:
      EXCHANGE_CONVERTOR::convert( field.getString() ); break;
    case TYPE::UtcTimeStamp:
      UTCTIMESTAMP_CONVERTOR::convert( field.getString() ); break;
    case TYPE::Boolean:
      BOOLEAN_CONVERTOR::convert( field.getString() ); break;
    case TYPE::LocalMktDate:
      LOCALMKTDATE_CONVERTOR::convert( field.getString() ); break;
    case TYPE::Data:
      DATA_CONVERTOR::convert( field.getString() ); break;
    case TYPE::Float:
      FLOAT_CONVERTOR::convert( field.getString() ); break;
    case TYPE::PriceOffset:
      PRICEOFFSET_CONVERTOR::convert( field.getString() ); break;
    case TYPE::MonthYear:
      MONTHYEAR_CONVERTOR::convert( field.getString() ); break;
    case TYPE::DayOfMonth:
      DAYOFMONTH_CONVERTOR::convert( field.getString() ); break;
    case TYPE::UtcDate:
      UTCDATE_CONVERTOR::convert( field.getString() ); break;
    case TYPE::UtcTimeOnly:
      UTCTIMEONLY_CONVERTOR::convert( field.getString() ); break;
    case TYPE::NumInGroup:
      NUMINGROUP_CONVERTOR::convert( field.getString() ); break;
    case TYPE::Percentage:
      PERCENTAGE_CONVERTOR::convert( field.getString() ); break;
    case TYPE::SeqNum:
      SEQNUM_CONVERTOR::convert( field.getString() ); break;
    case TYPE::Length:
      LENGTH_CONVERTOR::convert( field.getString() ); break;
    case TYPE::Country:
      COUNTRY_CONVERTOR::convert( field.getString() ); break;
    case TYPE::TzTimeOnly:
      TZTIMEONLY_CONVERTOR::convert( field.getString() ); break;
    case TYPE::TzTimeStamp:
      TZTIMESTAMP_CONVERTOR::convert( field.getString() ); break;
    case TYPE::XmlData:
      XMLDATA_CONVERTOR::convert( field.getString() ); break;
    case TYPE::Language:
      LANGUAGE_CONVERTOR::convert( field.getString() ); break;
    case TYPE::Unknown: break;
    }
  }
  catch ( FieldConvertError& )
  { 
    if( !ValidationRules::shouldTolerateBadFormatTag( vrptr, direction, msgType, field.getTag() ) )
      throw IncorrectDataFormat( field.getTag(), field.getTagAsString() );
  }
}
}

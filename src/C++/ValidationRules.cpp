/****************************************************************************
** Copyright (c) 2001-2016
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

#include "ValidationRules.h"
#include "Field.h"

#include <iostream>
#include <functional>
#include <algorithm>


// trim from start
static std::string ltrim( std::string s )
{
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
  return s;
}

// trim from end
static std::string rtrim(std::string s)
{
  s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
  return s;
}

// trim from both ends
static std::string trim(std::string s)
{
  return ltrim(rtrim(s));
}

typedef std::function<void(const std::string&)> SplitCallback;
void split( const std::string& string, const std::string& delimiter, SplitCallback cb )
{
  size_t next = -1, current = next;
  do {
    current = next+1;
    next = string.find_first_of( delimiter, current );
    cb( trim( string.substr( current, next - current ) ) );
  } while( next != std::string::npos);
}
void setInserter( std::set<int>& set, const std::string& string )
{
  int i;
  FIX::IntConvertor::convert( string, i );
  set.insert(i);
}

std::set<int> setFromSplit( const std::string& string, const std::string& delimiter ) 
{
  std::set<int> ret;
  split( string, delimiter, bind( setInserter, std::ref(ret), std::placeholders::_1 ) );
  return ret;
}

namespace FIX
{
const std::string ValidationRules::AnyMsgType = "?";

bool ValidationRules::shouldValidate( const ValidationRules* vr)
{
  if ( !vr ) return true;
  return vr->shouldValidate();
}

bool ValidationRules::shouldValidateFieldsOutOfOrder( const ValidationRules* vr)
{
  if( !ValidationRules::shouldValidate(vr) ) 
    return false;
  if( !vr )
    return true;
  return vr->shouldValidateFieldsOutOfOrder();
}

bool ValidationRules::shouldValidateLength( const ValidationRules* vr ) 
{
  if( !ValidationRules::shouldValidate(vr) ) 
    return false;
  if( !vr )
    return true;
  return vr->shouldValidateLength();
}

bool ValidationRules::shouldValidateChecksum( const ValidationRules* vr ) 
{
  if( !ValidationRules::shouldValidate(vr) ) 
    return false;
  if( !vr )
    return true;
  return vr->shouldValidateChecksum();
}

bool ValidationRules::shouldCheckTag( const ValidationRules* vr, const FieldBase& field )
{
  if( !ValidationRules::shouldValidate(vr) ) 
    return false;
  if ( !vr ) 
    return true;
  return vr->shouldCheckTag( field );
}

bool ValidationRules::shouldTolerateMissingTag ( const ValidationRules* vr, const std::string& msgType, int tag )
{
  if( !ValidationRules::shouldValidate( vr ) )
    return true;
  if ( !vr )
    return false;
  return vr->shouldTolerateMissingTag( msgType, tag );
}

bool ValidationRules::shouldTolerateMissingTag ( const ValidationRules* vr, const std::string& msgType, const FieldBase& field )
{
  if( !ValidationRules::shouldValidate( vr ) )
    return true;
  if ( !vr )
    return false;
  return vr->shouldTolerateMissingTag( msgType, field );
}


bool ValidationRules::shouldTolerateTagValue ( const ValidationRules* vr, const FieldBase& field )
{
  if( !ValidationRules::shouldValidate( vr ) )
    return true;
  if ( !vr ) 
    return false;
  return vr->shouldTolerateTagValue( field );
}

bool ValidationRules::shouldAllowTag ( const ValidationRules* vr, const std::string& msgType, const FieldBase& field )
{
  if( !ValidationRules::shouldValidate( vr ) )
    return true;
  if ( !vr ) 
    return false;
  return vr->shouldAllowTag( msgType, field );
}

ValidationRules::ValidationRules ()
: m_validate(true),
m_validateBounds(true),
m_allowedFields(),
m_allowedEmptyFields(),
m_allowedMissingFields()
{
}

ValidationRules::~ValidationRules ()
{
}

void ValidationRules::setShouldValidate( bool validate )
{
  m_validate = validate;
}

void ValidationRules::setValidateBounds( bool validatebounds )
{
  m_validateBounds = validatebounds;
}

void ValidationRules::setAllowedFields ( const std::string& allowedfieldstr ) 
{
  split( allowedfieldstr, ";", std::bind( &ValidationRules::addAllowedFieldGroup, this, std::placeholders::_1 ) );
}

void ValidationRules::setValidationRules ( const std::string& validationrulesstr ) 
{
  split( validationrulesstr, ",", std::bind( &ValidationRules::addValidationRule, this, std::placeholders::_1 ) );
}

void ValidationRules::setValidateLength ( bool validatelength)
{
  m_validateLength = validatelength;
}

void ValidationRules::setValidateChecksum ( bool validatechecksum)
{
  m_validateChecksum = validatechecksum;
}

void ValidationRules::setValidateFieldsOutOfOrder ( bool validatefieldsoutoforder )
{
  m_validateFieldsOutOfOrder = validatefieldsoutoforder;
}

void ValidationRules::setValidateFieldsHaveValues ( bool validatefieldshavevalues )
{
  m_validateFieldsHaveValues = validatefieldshavevalues;
}

void ValidationRules::setValidateUserDefinedFields ( bool validateuserdefinedfields )
{
  m_validateUserDefinedFields = validateuserdefinedfields;
}


bool ValidationRules::shouldValidate ( ) const
{
  return m_validate;
}

bool ValidationRules::shouldCheckTag ( const FieldBase& field ) const
{
  return true;
}

bool ValidationRules::shouldTolerateMissingTag ( const std::string& msgType, int tag ) const
{
  return mapHasValue( m_allowedMissingFields, msgType, tag );
  return false;
}

bool ValidationRules::shouldTolerateMissingTag ( const std::string& msgType, const FieldBase& field ) const
{
  return mapHasValue( m_allowedMissingFields, msgType, field.getTag() );
  return false;
}

bool ValidationRules::shouldTolerateTagValue ( const FieldBase& field ) const
{
  return false;
}

bool ValidationRules::shouldAllowTag (const std::string& msgType, const FieldBase& field) const
{
  return mapHasValue( m_allowedFields, msgType, field.getTag() );
}

bool ValidationRules::shouldValidateLength ( ) const 
{
  return m_validateLength;
}

bool ValidationRules::shouldValidateChecksum ( ) const
{
  return m_validateChecksum;
}

bool ValidationRules::shouldValidateFieldsOutOfOrder ( ) const
{
  return m_validateFieldsOutOfOrder;
}

bool ValidationRules::shouldValidateFieldsHaveValues ( ) const
{
  return m_validateFieldsHaveValues;
}

bool ValidationRules::shouldValidateUserDefinedFields ( ) const
{
  return m_validateUserDefinedFields;
}

void ValidationRules::addAllowedFieldGroup ( const std::string& afgstring )
{
  AllowedFieldGroup(afgstring, this);
}

void ValidationRules::addValidationRule ( const std::string& validationrulesstr ) 
{
  ValidationRule( validationrulesstr, this );
}


ValidationRules::AllowedFieldGroup::AllowedFieldGroup( const std::string& descriptor, ValidationRules* vrptr )
{
  split( descriptor, ":", bind( &AllowedFieldGroup::onGroupElement, this, std::placeholders::_1 ) );
  vrptr->m_allowedFields[messageType] = allowedFields;
}

void ValidationRules::AllowedFieldGroup::onGroupElement ( const std::string& string )
{
  if (messageType.size() == 0)
  {
    messageType = string;
  } else {
    allowedFields = setFromSplit( string, "," );
  }
}

ValidationRules::ValidationRule::ValidationRule( const std::string& descriptor, ValidationRules* vrptr) 
: inbound(-1),
rejectType(-1),
messageType(),
tag(-1),
parseSteps(0)
{
  split( descriptor, "-", bind( &ValidationRule::onRuleElement, this, std::placeholders::_1 ) );
  if( inbound != 0 && inbound != 1 )
    return;
  if ( rejectType == 2 )
    vrptr->safeAddMsgTypeValue( vrptr->m_allowedMissingFields, messageType, tag );   
  if ( rejectType == 0  || rejectType == 1  || rejectType == 4  || rejectType == 5 )
    vrptr->safeAddMsgTypeValue( vrptr->m_allowedFields, messageType, tag );   
}

void ValidationRules::ValidationRule::onRuleElement( const std::string& string )
{
  parseSteps++;
  if( inbound < 0 )
  {
    if( parseSteps > 1 )
      return; //I'm invalid
    try
    {
      inbound = IntConvertor::convert( string );
    }
    catch( FieldConvertError& e ) 
    { }
  }
  else if( rejectType < 0 )
  {
    if( parseSteps > 2 )
      return; //I'm invalid
    try
    {
      rejectType = IntConvertor::convert( string );
    }
    catch( FieldConvertError& e )
    { }
  }
  else if( messageType.size() < 1 )
  {
    if( parseSteps > 3 )
      return; //I'm invalid
    messageType = string;
  }
  else if( tag < 0 )
  {
    if( parseSteps > 4 )
      return; //I'm invalid
    try
    {
      tag = IntConvertor::convert( string );
    }
    catch( FieldConvertError& e )
    { }
  }
}

}

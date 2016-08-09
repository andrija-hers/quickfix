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
#include "Values.h"

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

bool ValidationRules::shouldCheckTag( const ValidationRules* vr, const std::string& msgType, int tag )
{
  if( !ValidationRules::shouldValidate(vr) ) 
    return false;
  if ( !vr ) 
    return true;
  return vr->shouldCheckTag( msgType, tag );
}

bool ValidationRules::shouldTolerateBadFormatTag ( const ValidationRules* vr, int direction, const std::string& msgType, int tag )
{
  if( !ValidationRules::shouldValidate( vr ) )
    return true;
  if ( !vr )
    return false;
  return vr->shouldTolerateBadFormatTag( direction, msgType, tag );
}

bool ValidationRules::shouldTolerateMissingTag ( const ValidationRules* vr, int direction, const std::string& msgType, int tag )
{
  if( !ValidationRules::shouldValidate( vr ) )
    return true;
  if ( !vr )
    return false;
  return vr->shouldTolerateMissingTag( direction, msgType, tag );
}

bool ValidationRules::shouldTolerateMissingMessageType ( const ValidationRules* vr, int direction )
{
  if( !ValidationRules::shouldValidate( vr ) )
    return true;
  if ( !vr )
    return false;
  return vr->shouldTolerateMissingMessageType( direction );
}

bool ValidationRules::shouldTolerateUnknownTag ( const ValidationRules* vr, int direction, const std::string& msgType, int tag )
{
  if( !ValidationRules::shouldValidate( vr ) )
    return true;
  if ( !vr )
    return false;
  return vr->shouldTolerateUnknownTag( direction, msgType, tag );
}

bool ValidationRules::shouldTolerateEmptyTag ( const ValidationRules* vr, int direction, const std::string& msgType, int tag )
{
  if( !ValidationRules::shouldValidate( vr ) )
    return true;
  if ( !vr )
    return false;
  return vr->shouldTolerateEmptyTag( direction, msgType, tag );
}

bool ValidationRules::shouldTolerateOutOfOrderTag ( const ValidationRules* vr, int direction, const std::string& msgType, int tag )
{
  if( !ValidationRules::shouldValidate( vr ) )
    return true;
  if ( !vr )
    return false;
  return vr->shouldTolerateOutOfOrderTag( direction, msgType, tag );
}

bool ValidationRules::shouldTolerateDuplicateTag ( const ValidationRules* vr, int direction, const std::string& msgType, int tag )
{
  if( !ValidationRules::shouldValidate( vr ) )
    return true;
  if ( !vr )
    return false;
  return vr->shouldTolerateDuplicateTag( direction, msgType, tag );
}

bool ValidationRules::shouldTolerateRepeatingGroupCountMismatch ( const ValidationRules* vr, int direction, const std::string& msgType, int tag ) {
  if( !ValidationRules::shouldValidate( vr ) )
    return true;
  if ( !vr ) 
    return false;
  return vr->shouldTolerateRepeatingGroupCountMismatch( direction, msgType, tag );
}

bool ValidationRules::shouldTolerateVersionMismatch ( const ValidationRules* vr, int direction ) {
  if( !ValidationRules::shouldValidate( vr ) )
    return true;
  if ( !vr ) 
    return false;
  return vr->shouldTolerateVersionMismatch( direction );
}

bool ValidationRules::shouldTolerateTagValue ( const ValidationRules* vr, int direction, const std::string& msgType, int tag )
{
  if( !ValidationRules::shouldValidate( vr ) )
    return true;
  if ( !vr ) 
    return false;
  return vr->shouldTolerateTagValue( direction, msgType, tag );
}

ValidationRules::ValidationRules ()
: m_validate(true),
m_validateBounds(true),
m_allowedFields(),
m_badFormatFields(),
m_outOfBoundsFields(),
m_missingFields(),
m_repeatingGroupMismatches(),
m_unknownFields(),
m_emptyFields(),
m_outOfOrderFields(),
m_duplicateFields()
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

bool ValidationRules::shouldCheckTag ( const std::string& msgType, int tag ) const
{
  return !mapHasValue( m_allowedFields, msgType, tag );
}

bool ValidationRules::shouldTolerateBadFormatTag ( int direction, const std::string& msgType, int tag ) const
{
  return standardAllowCheck( m_badFormatFields, direction, msgType, tag);
}

bool ValidationRules::shouldTolerateMissingTag ( int direction, const std::string& msgType, int tag ) const
{
  return standardAllowCheck( m_missingFields, direction, msgType, tag);
}

bool ValidationRules::shouldTolerateMissingMessageType ( int direction ) const
{
  return standardAllowCheck( m_missingFields, direction, "?", 35 );
}

bool ValidationRules::shouldTolerateVersionMismatch ( int direction ) const
{
  return standardAllowCheck( m_versionMismatches, direction, "?", -1 );
}

bool ValidationRules::shouldTolerateEmptyTag ( int direction, const std::string& msgType, int tag ) const
{
  return standardAllowCheck( m_emptyFields, direction, msgType, tag);
}

bool ValidationRules::shouldTolerateOutOfOrderTag ( int direction, const std::string& msgType, int tag ) const
{
  if (!m_validateFieldsOutOfOrder)
  {
    return true;
  }
  return standardAllowCheck( m_outOfOrderFields, direction, msgType, tag);
}

bool ValidationRules::shouldTolerateDuplicateTag ( int direction, const std::string& msgType, int tag ) const
{
  return standardAllowCheck( m_duplicateFields, direction, msgType, tag);
}

bool ValidationRules::shouldTolerateUnknownTag ( int direction, const std::string& msgType, int tag ) const
{
  return standardAllowCheck( m_unknownFields, direction, msgType, tag );
}

bool ValidationRules::shouldTolerateTagValue ( int direction, const std::string& msgType, int tag ) const
{
  if (!m_validateBounds) {
    return true;
  }
  return standardAllowCheck( m_outOfBoundsFields, direction, msgType, tag );
}

bool ValidationRules::shouldTolerateRepeatingGroupCountMismatch ( int direction, const std::string& msgType, int tag ) const
{
  return standardAllowCheck( m_repeatingGroupMismatches, direction, msgType, tag );
}

bool ValidationRules::shouldValidateLength ( ) const 
{
  return m_validateLength;
}

bool ValidationRules::shouldValidateChecksum ( ) const
{
  return m_validateChecksum;
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

bool ValidationRules::standardAllowCheck( const ValidationRules::DirectionAwareMsgTypeMap& dmtm, int direction, const std::string& msgType, int tag ) const
{
  if( mapHasValue( m_allowedFields, msgType, tag ) )
    return true;
  return dmtm.shouldAllowTag( direction, msgType, tag );
}

void ValidationRules::DirectionAwareMsgTypeMap::safeAddMsgTypeValue( int inbound, const std::string& msgType, int tag )
{
  if( inbound == 0 )
    ValidationRules::safeAddMsgTypeValue( m_outboundFields, msgType, tag );
  if( inbound == 1 )
    ValidationRules::safeAddMsgTypeValue( m_inboundFields, msgType, tag );
}

bool ValidationRules::DirectionAwareMsgTypeMap::shouldAllowTag( int direction, const std::string& msgType, int tag ) const
{
  if( direction == INCOMING_DIRECTION )
    return ValidationRules::mapHasValue( m_inboundFields, msgType, tag );
  if( direction == OUTGOING_DIRECTION )
    return ValidationRules::mapHasValue( m_outboundFields, msgType, tag );
  return false;
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
messageType("?"),
tag(-1),
parseSteps(0)
{
  split( descriptor, "-", bind( &ValidationRule::onRuleElement, this, std::placeholders::_1 ) );
  if( inbound != 0 && inbound != 1 )
    return;
  if ( rejectType == 104 )
    vrptr->m_versionMismatches.safeAddMsgTypeValue( inbound, messageType, tag );
  if ( rejectType == 7 )
    vrptr->m_duplicateFields.safeAddMsgTypeValue( inbound, messageType, tag );
  if ( rejectType == 6 )
    vrptr->m_outOfOrderFields.safeAddMsgTypeValue( inbound, messageType, tag );
  if ( rejectType == 5 )
    vrptr->m_emptyFields.safeAddMsgTypeValue( inbound, messageType, tag );
  if ( rejectType == 4 )
    vrptr->m_unknownFields.safeAddMsgTypeValue( inbound, messageType, tag );
  if ( rejectType == 3 )
    vrptr->m_repeatingGroupMismatches.safeAddMsgTypeValue( inbound, messageType, tag );
  if ( rejectType == 2 )
    vrptr->m_missingFields.safeAddMsgTypeValue( inbound, messageType, tag );
  if ( rejectType == 1 ) 
    vrptr->m_outOfBoundsFields.safeAddMsgTypeValue( inbound, messageType, tag );
  if ( rejectType == 0 )
    vrptr->m_badFormatFields.safeAddMsgTypeValue( inbound, messageType, tag );
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

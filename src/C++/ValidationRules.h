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

#ifndef FIX_VALIDATION_RULES_H
#define FIX_VALIDATION_RULES_H

#include <string>
#include <map>
#include <set>
#include <stdexcept>

namespace FIX
{
class FieldBase;

class ValidationRules
{
  public:
    static bool shouldValidate( const ValidationRules* vr );
    static bool shouldValidateFieldsOutOfOrder( const ValidationRules* vr );
    static bool shouldValidateLength( const ValidationRules* vr );
    static bool shouldValidateChecksum( const ValidationRules* vr );
    static bool shouldCheckTag( const ValidationRules* vr, const FieldBase& field );
    static bool shouldTolerateMissingTag ( const ValidationRules* vr, const std::string& msgType, int tag );
    static bool shouldTolerateMissingTag ( const ValidationRules* vr, const std::string& msgType, const FieldBase& field );
    static bool shouldTolerateTagValue ( const ValidationRules* vr, const FieldBase& field );
    static bool shouldAllowTag ( const ValidationRules* vr, const std::string& msgType, const FieldBase& field );
  public:
    ValidationRules();
    virtual ~ValidationRules();

    void setShouldValidate ( bool validate );
    void setValidateBounds ( bool validatebounds );
    void setAllowedFields ( const std::string& allowedfieldstr );
    void setValidationRules ( const std::string& validationrulesstr );
    void setValidateLength ( bool validatelength );
    void setValidateChecksum (bool validatechecksum );
    void setValidateFieldsOutOfOrder ( bool validatefieldsoutoforder );
    void setValidateFieldsHaveValues ( bool validatefieldshavevalues );
    void setValidateUserDefinedFields ( bool validateuserdefinedfields );

    bool shouldValidate ( ) const;
    bool shouldCheckTag ( const FieldBase& field ) const;
    bool shouldTolerateMissingTag ( const std::string& msgType, int tag ) const;
    bool shouldTolerateMissingTag ( const std::string& msgType, const FieldBase& field ) const;
    bool shouldTolerateTagValue ( const FieldBase& field ) const;
    bool shouldAllowTag ( const std::string& msgType, const FieldBase& field ) const;
    bool shouldValidateLength ( ) const;
    bool shouldValidateChecksum ( ) const;
    bool shouldValidateFieldsOutOfOrder ( ) const;
    bool shouldValidateFieldsHaveValues ( ) const;
    bool shouldValidateUserDefinedFields ( ) const;

  private:
    static const std::string AnyMsgType;
    typedef std::set<int> MsgTypeValues;
    typedef std::map< std::string, MsgTypeValues > MsgTypeMap;
    bool m_validate;
    bool m_validateBounds;
    bool m_validateLength;
    bool m_validateChecksum;
    bool m_validateFieldsOutOfOrder;
    bool m_validateFieldsHaveValues;
    bool m_validateUserDefinedFields;
    MsgTypeMap m_allowedFields;
    MsgTypeMap m_allowedEmptyFields;
    MsgTypeMap m_allowedMissingFields;
    void addAllowedFieldGroup ( const std::string& afgstring );
    void addValidationRule ( const std::string& validationrulestr );

    MsgTypeMap::const_iterator findMsgTypeValues( const MsgTypeMap& map, const std::string& msgtype ) const
    throw( std::out_of_range )
    {
      MsgTypeMap::const_iterator mi = map.find(msgtype);
      if( mi == map.end() )
      {
        if( msgtype != AnyMsgType )
        {
          mi = map.find( AnyMsgType );
          if( mi == map.end () )
            throw std::out_of_range(msgtype);
        }
        else
        {
          throw std::out_of_range(msgtype);
        }
      }
      return mi;
    }

    MsgTypeMap::iterator findMsgTypeValues( MsgTypeMap& map, const std::string& msgtype )
    throw( std::out_of_range )
    {
      MsgTypeMap::iterator mi = map.find(msgtype);
      if( mi == map.end() )
      {
        if( msgtype != AnyMsgType )
        {
          mi = map.find( AnyMsgType );
          if( mi == map.end () )
            throw std::out_of_range(msgtype);
        }
        else
        {
          throw std::out_of_range(msgtype);
        }
      }
      return mi;
    }

    MsgTypeMap::iterator findOrCreateMsgTypeValues( MsgTypeMap& map, const std::string& msgtype )
    {
      try
      {
        return findMsgTypeValues( map, msgtype );
      }
      catch ( std::out_of_range& e )
      {
        map[msgtype] = MsgTypeValues();
        return findMsgTypeValues( map, msgtype );
      }
    }

    bool mapHasValue ( const MsgTypeMap& map, const std::string& msgtype, int value ) const
    throw( std::out_of_range )
    {
      MsgTypeMap::const_iterator mi;
      try 
      {
        mi = findMsgTypeValues( map, msgtype );
      }
      catch ( std::out_of_range& e ) 
      {
        return false;
      }
      MsgTypeValues::const_iterator si = mi->second.find( value );
      return si != mi->second.end();
    }

    void safeAddMsgTypeValue ( MsgTypeMap& map, const std::string& msgtype, int value ) 
    {
      findOrCreateMsgTypeValues( map, msgtype )->second.insert(value);
    }

    struct AllowedFieldGroup
    {
      AllowedFieldGroup( const std::string& descriptor, ValidationRules* vrptr );
      void onGroupElement ( const std::string& string );
      std::string messageType;
      std::set<int> allowedFields;
    };
    friend struct AllowedFieldGroup;

    struct ValidationRule
    {
      ValidationRule( const std::string& descriptor, ValidationRules* vrptr );
      void onRuleElement( const std::string& string );
      int inbound;
      int rejectType;
      std::string messageType;
      int tag;
      private:
        int parseSteps;
    };
    friend struct ValidationRule;
};
}


#endif //FIX_VALIDATION_RULES_H

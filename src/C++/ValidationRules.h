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
class ValidationRules
{
  public:
    static bool shouldValidate( const ValidationRules* vr );
    static bool shouldValidateLength( const ValidationRules* vr );
    static bool shouldValidateChecksum( const ValidationRules* vr );
    static bool shouldValidateUserDefinedFields( const ValidationRules* vr );
    static bool shouldTolerateBadFormatTag ( const ValidationRules* vr, int direction, const std::string& msgType, int tag );
    static bool shouldTolerateMissingTag ( const ValidationRules* vr, int direction, const std::string& msgType, int tag );
    static bool shouldTolerateMissingMessageType ( const ValidationRules* vr, int direction );
    static bool shouldTolerateRepeatingGroupCountMismatch (const ValidationRules* vr, int direction, const std::string& msgType, int tag);
    static bool shouldTolerateUnknownTag ( const ValidationRules* vr, int direction, const std::string& msgType, int tag );
    static bool shouldTolerateEmptyTag ( const ValidationRules* vr, int direction, const std::string& msgType, int tag );
    static bool shouldTolerateTagValue ( const ValidationRules* vr, int direction, const std::string& msgType, int tag );
    static bool shouldTolerateOutOfOrderTag ( const ValidationRules* vr, int direction, const std::string& msgType, int tag );
    static bool shouldTolerateDuplicateTag ( const ValidationRules* vr, int direction, const std::string& msgType, int tag );
    static bool shouldTolerateVersionMismatch ( const ValidationRules* vr, int direction );
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
    bool shouldTolerateBadFormatTag ( int direction, const std::string& msgType, int tag ) const;
    bool shouldTolerateMissingTag ( int direction, const std::string& msgType, int tag ) const;
    bool shouldTolerateMissingMessageType ( int direction ) const;
    bool shouldTolerateEmptyTag ( int direction, const std::string& msgType, int tag ) const;
    bool shouldTolerateOutOfOrderTag ( int direction, const std::string& msgType, int tag ) const;
    bool shouldTolerateDuplicateTag ( int direction, const std::string& msgType, int tag ) const;
    bool shouldTolerateUnknownTag ( int direction, const std::string& msgType, int tag ) const;
    bool shouldTolerateTagValue ( int direction, const std::string& msgType, int tag ) const;
    bool shouldAllowTag ( int direction, const std::string& msgType, int tag ) const;
    bool shouldTolerateRepeatingGroupCountMismatch ( int direction, const std::string& msgType, int tag ) const;
    bool shouldTolerateVersionMismatch ( int direction ) const;
    bool shouldValidateLength ( ) const;
    bool shouldValidateChecksum ( ) const;
    bool shouldValidateFieldsHaveValues ( ) const;
    bool shouldValidateUserDefinedFields ( ) const;

  private:
    static const std::string AnyMsgType;
    typedef std::set<int> MsgTypeValues;
    typedef std::map< std::string, MsgTypeValues > MsgTypeMap;
    struct DirectionAwareMsgTypeMap {
      private: 
        MsgTypeMap m_inboundFields;
        MsgTypeMap m_outboundFields;
      public: 
        void safeAddMsgTypeValue( int inbound, const std::string& msgType, int tag );
        bool shouldAllowTag( int direction, const std::string& msgType, int tag ) const;
    };
    bool m_validate;
    bool m_validateBounds;
    bool m_validateLength;
    bool m_validateChecksum;
    bool m_validateFieldsOutOfOrder;
    bool m_validateFieldsHaveValues;
    bool m_validateUserDefinedFields;
    MsgTypeMap m_allowedFields;
    DirectionAwareMsgTypeMap m_badFormatFields;
    DirectionAwareMsgTypeMap m_outOfBoundsFields;
    DirectionAwareMsgTypeMap m_missingFields;
    DirectionAwareMsgTypeMap m_repeatingGroupMismatches;
    DirectionAwareMsgTypeMap m_unknownFields;
    DirectionAwareMsgTypeMap m_emptyFields;
    DirectionAwareMsgTypeMap m_outOfOrderFields;
    DirectionAwareMsgTypeMap m_duplicateFields;
    DirectionAwareMsgTypeMap m_versionMismatches;
    void addAllowedFieldGroup ( const std::string& afgstring );
    void addValidationRule ( const std::string& validationrulestr );

    bool standardAllowCheck( const DirectionAwareMsgTypeMap& dmtm, int direction, const std::string& msgType, int tag ) const ;

    static MsgTypeMap::const_iterator findMsgTypeValues( const MsgTypeMap& map, const std::string& msgtype ) 
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

    static MsgTypeMap::iterator findMsgTypeValues( MsgTypeMap& map, const std::string& msgtype )
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

    static MsgTypeMap::iterator findOrCreateMsgTypeValues( MsgTypeMap& map, const std::string& msgtype )
    {
      try
      {
        return findMsgTypeValues( map, msgtype );
      }
      catch ( std::out_of_range& e )
      {
		    (void) e;
        map[msgtype] = MsgTypeValues();
        return findMsgTypeValues( map, msgtype );
      }
    }

    static bool mapHasValue ( const MsgTypeMap& map, const std::string& msgtype, int value )
    {
      MsgTypeMap::const_iterator mi;
      try 
      {
        mi = findMsgTypeValues( map, msgtype );
      }
      catch ( std::out_of_range& e ) 
      {
        (void) e;
        return false;
      }
      MsgTypeValues::const_iterator si = mi->second.find( value );
      return si != mi->second.end();
    }

    static void safeAddMsgTypeValue ( MsgTypeMap& map, const std::string& msgtype, int value ) 
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
    friend struct DirectionAwareMsgTypeMap;
};
}


#endif //FIX_VALIDATION_RULES_H

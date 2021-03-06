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

#ifndef FIX_SESSION_H
#define FIX_SESSION_H

#ifdef _MSC_VER
#pragma warning( disable : 4503 4355 4786 4290 )
#endif

#include "SessionState.h"
#include "TimeRange.h"
#include "SessionID.h"
#include "Responder.h"
#include "Fields.h"
#include "DataDictionaryProvider.h"
#include "Application.h"
#include "Mutex.h"
#include "Log.h"
#include "ValidationRules.h"
#include "ScheduleFactory.h"
#include <utility>
#include <map>
#include <queue>

namespace FIX
{
/// Maintains the state and implements the logic of a %FIX %session.
class ValidationRules;
class Session
{
public:
  Session( Application&, MessageStoreFactory&,
           const SessionID&,
           const DataDictionaryProvider&,
           const ISchedule* pSchedule,
           int heartBtInt, LogFactory* pLogFactory );
  virtual ~Session();

  void doInitialTimestampCheck();
  void logon();
  void logout( const std::string& reason = "" );
  void mustLogout( const std::string& reason = "" );
  void eod();

  bool sentLogon() const { return m_state.sentLogon(); }
  bool sentLogout() const { return m_state.sentLogout(); }
  bool receivedLogon() const { return m_state.receivedLogon(); }
  bool isLoggedOn() const { return receivedLogon() && sentLogon(); }
  void deprecatedReset() throw( IOException ) 
  { generateLogout(); disconnect(); m_state.reset(); }
  void refresh() throw( IOException )
  { m_state.refresh(); }
  void setNextSenderMsgSeqNum( int num ) throw( IOException )
  { m_state.setNextSenderMsgSeqNum( num ); }
  void setNextTargetMsgSeqNum( int num ) throw( IOException )
  { m_state.setNextTargetMsgSeqNum( num ); }

  const SessionID& getSessionID() const
  { return m_sessionID; }
  void setDataDictionaryProvider( const DataDictionaryProvider& dataDictionaryProvider )
  { m_dataDictionaryProvider = dataDictionaryProvider; }
  const DataDictionaryProvider& getDataDictionaryProvider() const
  { return m_dataDictionaryProvider; }

  static bool sendToTarget( Message& message,
                            const std::string& qualifier = "" )
  throw( SessionNotFound );
  static bool sendToTarget( Message& message, const SessionID& sessionID )
  throw( SessionNotFound );
  static bool sendToTarget( Message&,
                            const SenderCompID& senderCompID,
                            const TargetCompID& targetCompID,
                            const std::string& qualifier = "" )
  throw( SessionNotFound );
  static bool sendToTarget( Message& message,
                            const std::string& senderCompID,
                            const std::string& targetCompID,
                            const std::string& qualifier = "" )
  throw( SessionNotFound );

  static std::set<SessionID> getSessions();
  static bool doesSessionExist( const SessionID& );
  static Session* lookupSession( const SessionID& );
  static Session* lookupSession( const std::string&, bool reverse = false );
  static bool isSessionRegistered( const SessionID& );
  static Session* registerSession( const SessionID& );
  static void unregisterSession( const SessionID& );

  static size_t numSessions();

  bool isConnectTime(const UtcTimeStamp& time);
  void registerConnectionAttempt ();
  bool isSessionTime(const UtcTimeStamp& time) const
    { return m_pSchedule->isInRange(time); }
  bool isLogonTime(const UtcTimeStamp& time) const
    { return m_state.manualLoginRequested() || m_pSchedule->isInRange(time); }
  bool isInitiator()
    { return m_state.initiate(); }
  bool isAcceptor()
    { return !m_state.initiate(); }

  const std::string& getSenderDefaultApplVerID()
    { return m_senderDefaultApplVerID; }
  void setSenderDefaultApplVerID( const std::string& senderDefaultApplVerID )
    { m_senderDefaultApplVerID = senderDefaultApplVerID; }

  const std::string& getTargetDefaultApplVerID()
    { return m_targetDefaultApplVerID; }
  void setTargetDefaultApplVerID( const std::string& targetDefaultApplVerID )
    { m_targetDefaultApplVerID = targetDefaultApplVerID; }

  bool getSendRedundantResendRequests()
    { return m_sendRedundantResendRequests; }
  void setSendRedundantResendRequests ( bool value )
    { m_sendRedundantResendRequests = value; } 

  bool getCheckCompId()
    { return m_checkCompId; }
  void setCheckCompId ( bool value )
    { m_checkCompId = value; }

  bool getCheckLatency()
    { return m_checkLatency; }
  void setCheckLatency ( bool value )
    { m_checkLatency = value; }

  int getMaxLatency()
    { return m_maxLatency; }
  void setMaxLatency ( int value )
    { m_maxLatency = value; }

  int getLogonTimeout()
    { return m_state.logonTimeout(); }
  void setLogonTimeout ( int value )
    { m_state.logonTimeout( value ); }

  int getLogoutTimeout()
    { return m_state.logoutTimeout(); }
  void setLogoutTimeout ( int value )
    { m_state.logoutTimeout( value ); }

  bool getRefreshOnLogon()
    { return m_refreshOnLogon; }
  void setRefreshOnLogon( bool value )
    { m_refreshOnLogon = value; } 

  bool getMillisecondsInTimeStamp()
    { return m_millisecondsInTimeStamp; }
  void setMillisecondsInTimeStamp ( bool value )
    { m_millisecondsInTimeStamp = value; }

  bool getPersistMessages()
    { return m_persistMessages; }
  void setPersistMessages ( bool value )
    { m_persistMessages = value; }

  bool getValidateLengthAndChecksum()
    { return m_validationRules.shouldValidateLength() && m_validationRules.shouldValidateChecksum(); }
  void setValidateLengthAndChecksum ( bool value )
    { m_validationRules.setValidateLength( true ); m_validationRules.setValidateChecksum( true ); }
  void setShouldValidate ( bool validate ) 
    { m_validationRules.setShouldValidate( validate ); }
  void setValidateFieldsOutOfOrder ( bool validatefieldsoutoforder )
    { m_validationRules.setValidateFieldsOutOfOrder( validatefieldsoutoforder ); }
  void setValidateFieldsHaveValues ( bool validatefieldshavevalues )
    { m_validationRules.setValidateFieldsHaveValues( validatefieldshavevalues ); }
  void setValidateUserDefinedFields ( bool validateuserdefinedfields ) 
    { m_validationRules.setValidateUserDefinedFields( validateuserdefinedfields ); }
  void setValidateBounds ( bool validatebounds ) 
    { m_validationRules.setValidateBounds( validatebounds ) ; }
  void setAllowedFields ( const std::string &allowedfieldstr ) 
    { m_validationRules.setAllowedFields (allowedfieldstr); }
  void setValidationRules ( const std::string &validationrules )
    { m_validationRules.setValidationRules( validationrules ); }
  void setSchedule( const std::string &scheduledescriptor );

  void setResponder( Responder* pR )
  {
    checkForSessionTime(UtcTimeStamp(), false);
    m_pResponder = pR;
  }

  bool send( Message& );
  Message* messageFromString( const std::string& string )
  throw( FIX::Exception );
  void next();
  void next( const UtcTimeStamp& timeStamp );
  void next( const std::string&, const UtcTimeStamp& timeStamp,  bool queued = false );
  void next( const Message&, const UtcTimeStamp& timeStamp,  bool queued = false );
  void disconnect();
  void autoDisconnect();
  bool shouldConnectPrerequisites( const UtcTimeStamp& timeStamp );

  int getExpectedSenderNum() { return m_state.getNextSenderMsgSeqNum(); }
  int getExpectedTargetNum() { return m_state.getNextTargetMsgSeqNum(); }

  Log* getLog() { return &m_state; }
  const MessageStore* getStore() { return &m_state; }

private:
  typedef std::map < SessionID, Session* > Sessions;
  typedef std::set < SessionID > SessionIDs;

  static bool addSession( Session& );
  static void removeSession( Session& );

  void doNextMessage( const Message&, const UtcTimeStamp& timeStamp, bool queued );
  bool send( const std::string& );
  bool sendRaw( Message&, int msgSeqNum = 0 );
  bool resend( Message& message );
  void persist( const Message&, const std::string& ) throw ( IOException );

  void insertSendingTime( Header& );
  void insertOrigSendingTime( Header&,
                              const UtcTimeStamp& when = UtcTimeStamp () );
  void fill( Header& );

  bool isGoodTime( const SendingTime& sendingTime )
  {
    if ( !m_checkLatency ) return true;
    UtcTimeStamp now;
    return labs( now - sendingTime ) <= m_maxLatency;
  }
  bool checkSessionTime( const UtcTimeStamp& timeStamp );
  bool isTargetTooHigh( const MsgSeqNum& msgSeqNum )
  { return msgSeqNum > ( m_state.getNextTargetMsgSeqNum() ); }
  bool isTargetTooLow( const MsgSeqNum& msgSeqNum )
  { return msgSeqNum < ( m_state.getNextTargetMsgSeqNum() ); }
  bool isCorrectCompID( const SenderCompID& senderCompID,
                        const TargetCompID& targetCompID )
  {
    if( !m_checkCompId ) return true;

    return
      m_sessionID.getSenderCompID().getValue() == targetCompID.getValue()
      && m_sessionID.getTargetCompID().getValue() == senderCompID.getValue();
  }
  bool shouldSendReset();

  bool validLogonState( const MsgType& msgType );
  void fromCallback( const MsgType& msgType, const Message& msg,
                     const SessionID& sessionID );

  void doBadTime( int direction, const Message& msg );
  void doBadCompID( const Message& msg );
  bool doPossDup( const Message& msg );
  bool doTargetTooLow( const Message& msg );
  void doTargetTooHigh( const Message& msg );
  void nextQueued( const UtcTimeStamp& timeStamp );
  bool nextQueued( int num, const UtcTimeStamp& timeStamp );

  void nextLogon( const Message&, const UtcTimeStamp& timeStamp );
  void nextHeartbeat( const Message&, const UtcTimeStamp& timeStamp );
  void nextTestRequest( const Message&, const UtcTimeStamp& timeStamp );
  void nextLogout( const Message&, const UtcTimeStamp& timeStamp );
  void nextReject( const Message&, const UtcTimeStamp& timeStamp );
  void nextSequenceReset( const Message&, const UtcTimeStamp& timeStamp );
  void nextResendRequest( const Message&, const UtcTimeStamp& timeStamp );

  void generateLogon();
  void generateLogon( const Message& );
  void generateResendRequest( const BeginString&, const MsgSeqNum& );
  void generateSequenceReset( int, int );
  void generateHeartbeat();
  void generateHeartbeat( const Message& );
  void generateTestRequest( const std::string& );
  void generateReject( int direction, const Message&, int err, int field = 0 );
  void generateReject( int direction, const Header&, const std::string& messagetext, int err, int field = 0 );
  void generateReject( int direction, const Message&, const std::string& );
  void generateReject( int direction, const Header&, const std::string& );
  void generateBusinessReject( int direction, const Message&, int err, int field = 0 );
  void generateLogout( const std::string& text = "" );

  void populateRejectReason( Message&, int field, const std::string& );
  void populateRejectReason( Message&, const std::string& );

  bool verify( const Message& msg, int direction, 
               bool checkTooHigh = true, bool checkTooLow = true );

  bool set( int s, const Message& m );
  bool get( int s, Message& m ) const;
  bool checkForSessionTime( const UtcTimeStamp& timeStamp, bool disconnecttoo );
  void doTheResetLogic();
  void doTheStandardStateReset();
  void doStateReset( const std::string& reason );

  Application& m_application;
  SessionID m_sessionID;
  std::unique_ptr<const ISchedule> m_pSchedule;

  std::string m_senderDefaultApplVerID;
  std::string m_targetDefaultApplVerID;
  bool m_sendRedundantResendRequests;
  bool m_checkCompId;
  bool m_checkLatency;
  int m_maxLatency;
  bool m_resetOnLogon;
  bool m_resetOnLogout;
  bool m_resetOnDisconnect;
  bool m_resetOnWrongTime;
  bool m_refreshOnLogon;
  bool m_millisecondsInTimeStamp;
  bool m_persistMessages;
  ValidationRules m_validationRules;

  SessionState m_state;
  DataDictionaryProvider m_dataDictionaryProvider;
  MessageStoreFactory& m_messageStoreFactory;
  LogFactory* m_pLogFactory;
  Responder* m_pResponder;
  Mutex m_mutex;

  static Sessions s_sessions;
  static SessionIDs s_sessionIDs;
  static Sessions s_registered;
  static Mutex s_mutex;

};
}

#endif //FIX_SESSION_H

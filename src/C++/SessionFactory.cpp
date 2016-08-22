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

#include "Utility.h"
#include "Values.h"
#include "DataDictionaryProvider.h"
#include "SessionFactory.h"
#include "SessionSettings.h"
#include "Session.h"

#include <memory>

namespace FIX
{
SessionFactory::~SessionFactory()
{
}

Session* SessionFactory::create( const SessionID& sessionID,
                                 const Dictionary& settings ) throw( ConfigError )
{
  std::string connectionType = settings.getString( CONNECTION_TYPE );
  if ( connectionType != "acceptor" && connectionType != "initiator" )
    throw ConfigError( "Invalid ConnectionType" );

  if( connectionType == "acceptor" && settings.has(SESSION_QUALIFIER) )
    throw ConfigError( "SessionQualifier cannot be used with acceptor." );

  bool useDataDictionary = true;
  if ( settings.has( USE_DATA_DICTIONARY ) )
    useDataDictionary = settings.getBool( USE_DATA_DICTIONARY );

  std::string defaultApplVerID;
  if( sessionID.isFIXT() )
  {
    if( !settings.has(DEFAULT_APPLVERID) )
    {
      throw ConfigError("ApplVerID is required for FIXT transport");
    }
    defaultApplVerID = Message::toApplVerID( settings.getString(DEFAULT_APPLVERID) );
  }

  DataDictionaryProvider dataDictionaryProvider;
  if( useDataDictionary )
  {
    if( sessionID.isFIXT() )
    {
      processFixtDataDictionaries(sessionID, settings, dataDictionaryProvider);
    }
    else
    {
      processFixDataDictionary(sessionID, settings, dataDictionaryProvider);
    }
  }

  bool useLocalTime = false;
  if( settings.has(USE_LOCAL_TIME) )
    useLocalTime = settings.getBool( USE_LOCAL_TIME );


  HeartBtInt heartBtInt( 0 );
  if ( connectionType == "initiator" )
  {
    heartBtInt = HeartBtInt( settings.getInt( HEARTBTINT ) );
    if ( heartBtInt <= 0 ) throw ConfigError( "Heartbeat must be greater than zero" );
  }

  ISchedule* pSchedule = 0;

  if ( settings.has( SCHEDULE ) )
    pSchedule = createSchedule( settings.getString( SCHEDULE ) );
  else
    pSchedule = createSchedule(  );



  std::unique_ptr<Session> pSession;
  pSession.reset( new Session( m_application, m_messageStoreFactory,
    sessionID, dataDictionaryProvider, pSchedule,
    heartBtInt, m_pLogFactory ) );

  pSession->setSenderDefaultApplVerID(defaultApplVerID);

  if ( settings.has( SEND_REDUNDANT_RESENDREQUESTS ) )
    pSession->setSendRedundantResendRequests( settings.getBool( SEND_REDUNDANT_RESENDREQUESTS ) );
  if ( settings.has( CHECK_COMPID ) )
    pSession->setCheckCompId( settings.getBool( CHECK_COMPID ) );
  if ( settings.has( CHECK_LATENCY ) )
    pSession->setCheckLatency( settings.getBool( CHECK_LATENCY ) );
  if ( settings.has( MAX_LATENCY ) )
    pSession->setMaxLatency( settings.getInt( MAX_LATENCY ) );
  if ( settings.has( LOGON_TIMEOUT ) )
    pSession->setLogonTimeout( settings.getInt( LOGON_TIMEOUT ) );
  if ( settings.has( LOGOUT_TIMEOUT ) )
    pSession->setLogoutTimeout( settings.getInt( LOGOUT_TIMEOUT ) );
  if ( settings.has( REFRESH_ON_LOGON ) )
    pSession->setRefreshOnLogon( settings.getBool( REFRESH_ON_LOGON ) );
  if ( settings.has( MILLISECONDS_IN_TIMESTAMP ) )
    pSession->setMillisecondsInTimeStamp( settings.getBool( MILLISECONDS_IN_TIMESTAMP ) );
  if ( settings.has( PERSIST_MESSAGES ) )
    pSession->setPersistMessages( settings.getBool( PERSIST_MESSAGES ) );
  if ( settings.has( VALIDATE_LENGTH_AND_CHECKSUM ) )
    pSession->setValidateLengthAndChecksum( settings.getBool( VALIDATE_LENGTH_AND_CHECKSUM ) );
  if ( settings.has( VALIDATE ) )
    pSession->setShouldValidate( settings.getBool( VALIDATE ) );
  if( settings.has( VALIDATE_FIELDS_OUT_OF_ORDER ) )
    pSession->setValidateFieldsOutOfOrder( settings.getBool( VALIDATE_FIELDS_OUT_OF_ORDER ) );
  if( settings.has( VALIDATE_FIELDS_HAVE_VALUES ) )
    pSession->setValidateFieldsHaveValues( settings.getBool( VALIDATE_FIELDS_HAVE_VALUES ) );
  if( settings.has( VALIDATE_USER_DEFINED_FIELDS ) )
    pSession->setValidateUserDefinedFields( settings.getBool( VALIDATE_USER_DEFINED_FIELDS ) );
  if ( settings.has( VALIDATE_BOUNDS ) )
    pSession->setValidateBounds( settings.getBool( VALIDATE_BOUNDS ) );
  if ( settings.has( ALLOWED_FIELDS ) )
    pSession->setAllowedFields( settings.getString( ALLOWED_FIELDS ) );
  if ( settings.has( VALIDATION_RULES ) )
    pSession->setValidationRules( settings.getString( VALIDATION_RULES ) );

  pSession->doInitialTimestampCheck();
   
  return pSession.release();
}

void SessionFactory::destroy( Session* pSession )
{
  delete pSession;
}

ptr::shared_ptr<DataDictionary> SessionFactory::createDataDictionary(const SessionID& sessionID, 
                                                                     const Dictionary& settings, 
                                                                     const std::string& settingsKey) throw(ConfigError)
{
  ptr::shared_ptr<DataDictionary> pDD;
  std::string path = settings.getString( settingsKey );
  Dictionaries::iterator i = m_dictionaries.find( path );
  if ( i != m_dictionaries.end() )
  {
    pDD = i->second;
  }
  else
  {
    pDD = ptr::shared_ptr<DataDictionary>(new DataDictionary( path ));
    m_dictionaries[ path ] = pDD;
  }

  ptr::shared_ptr<DataDictionary> pCopyOfDD = ptr::shared_ptr<DataDictionary>(new DataDictionary(*pDD));

  return pCopyOfDD;
}

void SessionFactory::processFixtDataDictionaries(const SessionID& sessionID, 
                                                 const Dictionary& settings, 
                                                 DataDictionaryProvider& provider) throw(ConfigError)
{
  ptr::shared_ptr<DataDictionary> pDataDictionary = createDataDictionary(sessionID, settings, TRANSPORT_DATA_DICTIONARY);
  provider.addTransportDataDictionary(sessionID.getBeginString(), pDataDictionary);
  
  for(Dictionary::const_iterator data = settings.begin(); data != settings.end(); ++data)
  {
    const std::string& key = data->first;
    const std::string frontKey = key.substr(0, strlen(APP_DATA_DICTIONARY));
    if( frontKey == string_toUpper(APP_DATA_DICTIONARY) )
    {
      if( key == string_toUpper(APP_DATA_DICTIONARY) )
      {
        provider.addApplicationDataDictionary(Message::toApplVerID(settings.getString(DEFAULT_APPLVERID)),
            createDataDictionary(sessionID, settings, APP_DATA_DICTIONARY));
      }
      else
      {
        std::string::size_type offset = key.find('.');
        if( offset == std::string::npos )
          throw ConfigError(std::string("Malformed ") + APP_DATA_DICTIONARY + ": " + key);
        std::string beginStringQualifier = key.substr(offset+1);
        provider.addApplicationDataDictionary(Message::toApplVerID(beginStringQualifier), 
            createDataDictionary(sessionID, settings, key));
      }
    }
  }
}

void SessionFactory::processFixDataDictionary(const SessionID& sessionID, 
                                              const Dictionary& settings, 
                                              DataDictionaryProvider& provider) throw(ConfigError)
{
  ptr::shared_ptr<DataDictionary> pDataDictionary = createDataDictionary(sessionID, settings, DATA_DICTIONARY);
  provider.addTransportDataDictionary(sessionID.getBeginString(), pDataDictionary);
  provider.addApplicationDataDictionary(Message::toApplVerID(sessionID.getBeginString()), pDataDictionary);
}
}

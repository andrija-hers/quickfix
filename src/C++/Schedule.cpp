#include "Schedule.h"
#include "Utility.h"
#include "FieldTypes.h"
#include "FieldConvertors.h"

#include <iostream>

struct ScheduleDescriptorParseResult {
  ScheduleDescriptorParseResult () : 
  weekly( false ),
  days(),
  start(),
  end(),
  autoeod( false ),
  autoreconnect( false ),
  autoreconnectinterval( 0 ),
  autoconnect( false ),
  autodisconnect( false ),
  m_Count( 0 ) {}

  void onElement( const std::string& element) {
    switch( m_Count++ ) 
    {
      case 0:
        setWeekly( element );
        break;
      case 1:
        setDays( element );
        break;
      case 2:
        setStart( element );
        break;
      case 3:
        setEnd( element );
        break;
      case 4:
        setAutoEOD( element );
        break;
      case 5:
        setAutoReconnect( element );
        break;
      case 6:
        setAutoReconnectInterval( element );
        break;
      case 7:
        setAutoConnect( element );
        break;
      case 8:
        setAutoDisconnect( element );
        break;
    }
  }

  bool weekly;
  std::set<int> days;
  FIX::UtcTimeOnly start;
  FIX::UtcTimeOnly end;
  bool autoeod;
  bool autoreconnect;
  int autoreconnectinterval;
  bool autoconnect;
  bool autodisconnect;

  int neededElements() const { return 9; }
  bool allElementsAcquired() const { return m_Count == neededElements(); }
  private:
    int m_Count;
    void setWeekly( const std::string& weeklystr )
    {
      if( weeklystr == "w" || weeklystr == "W" )
      {
        weekly = true;
        return;
      }
      if( weeklystr == "d" || weeklystr == "D" )
      {
        weekly = false;
        return;
      }
      throw FIX::InvalidWeeklyScheduleDescriptorElement(weeklystr);
    }
    void setDays( const std::string& daysstr )
    {
      try
      {
        days = FIX::intSetFromSplit( daysstr, "," );
      }
      catch (...) 
      {
        throw FIX::InvalidWeeklyScheduleDescriptorElement( daysstr );
      }
    }
    void setStart( const std::string& startstr ) 
    {
      try
      {
        start = FIX::UtcTimeOnlyConvertor::convert( makeUpTime( startstr ) );
      }
      catch (...)
      {
        throw FIX::InvalidStartTimeScheduleDescriptorElement( startstr );
      }
    }
    void setEnd( const std::string& endstr ) 
    {
      try
      {
        end = FIX::UtcTimeOnlyConvertor::convert( makeUpTime( endstr ) );
      }
      catch (...)
      {
        throw FIX::InvalidStartTimeScheduleDescriptorElement( endstr );
      }
    }
    void setAutoEOD( const std::string& autoeodstr )
    {
      if( autoeodstr == "AutoEOD" )
      {
        autoeod = true;
        return;
      }
      if( autoeodstr == "NoAutoEOD" )
      {
        autoeod = false;
        return;
      }
      throw FIX::InvalidAutoEODScheduleDescriptorElement( autoeodstr );
    }
    void setAutoReconnect( const std::string& autoreconnectstr )
    {
      if( autoreconnectstr  == "AutoReconnect" )
      {
        autoreconnect = true;
        return;
      }
      if( autoreconnectstr   == "NoAutoReconnect" )
      {
        autoreconnect = false;
        return;
      }
      throw FIX::InvalidAutoReconnectScheduleDescriptorElement( autoreconnectstr );
    }
    void setAutoReconnectInterval( const std::string& autoreconnectintervalstr )
    {
      try
      {
        autoreconnectinterval = FIX::IntConvertor::convert( autoreconnectintervalstr );
      }
      catch (...)
      {
        throw FIX::InvalidAutoReconnectScheduleDescriptorElement( autoreconnectintervalstr );
      }
    }
    void setAutoConnect( const std::string& autoconnectstr )
    {
      if( autoconnectstr == "AutoConnect" )
      {
        autoconnect = true;
        return;
      }
      if( autoconnectstr == "NoAutoConnect" )
      {
        autoconnect = false;
        return;
      }
      throw FIX::InvalidAutoConnectScheduleDescriptorElement( autoconnectstr );
    }
    void setAutoDisconnect( const std::string& autodisconnectstr )
    {
      if( autodisconnectstr == "AutoDisconnect" )
      {
        autodisconnect = true;
        return;
      }
      if( autodisconnectstr == "NoAutoDisconnect" )
      {
        autodisconnect = false;
        return;
      }
      throw FIX::InvalidAutoDisconnectScheduleDescriptorElement( autodisconnectstr );
    }


    std::string makeUpTime( const std::string& maybetime )
    {
      if( 5 == maybetime.length() )
      {
        return maybetime+":00";
      }
      return maybetime;
    }
};

namespace FIX {

static const int sMillisecondsInADay = 24*60*60*1000;
static const int sMillisecondsInAnHour = 60*60*1000;
static const int sMillisecondsInAMinute = 60*1000;
static const int sMillisecondsInASecond = 1000;
static const int sDaysInAWeek = 7;

int Schedule::toWeeklyMilliseconds( const UtcTimeStamp& time )
{
  return (time.getWeekDay()-1)*sMillisecondsInADay +
    time.getHour()*sMillisecondsInAnHour +
    time.getMinute()*sMillisecondsInAMinute +
    time.getSecond()*sMillisecondsInASecond +
    time.getMillisecond();
}

int Schedule::toWeeklyMilliseconds( int day, const UtcTimeOnly& time )
{
  return day*sMillisecondsInADay +
    time.getHour()*sMillisecondsInAnHour +
    time.getMinute()*sMillisecondsInAMinute +
    time.getSecond()*sMillisecondsInASecond +
    time.getMillisecond();
}

Schedule::Schedule(
  bool autoeod,
  bool autoreconnect,
  int autoreconnectinterval,
  bool autoconnect,
  bool autodisconnect
):
  m_autoEOD( autoeod ),
  m_autoReconnect( autoreconnect ),
  m_autoReconnectInterval( autoreconnectinterval ),
  m_autoConnect( autoconnect ),
  m_autoDisconnect( autodisconnect )
{ }


DailySchedule::DailySchedule(
  std::set<int> days,
  const UtcTimeOnly& start,
  const UtcTimeOnly& end,
  bool autoeod,
  bool autoreconnect,
  int autoreconnectinterval,
  bool autoconnect,
  bool autodisconnect
):
  Schedule( autoeod, autoreconnect, autoreconnectinterval, autoconnect, autodisconnect ),
  m_Days( days ),
  m_start( start ),
  m_end( end )
{ }

NormalDailySchedule::NormalDailySchedule(
  std::set<int> days,
  const UtcTimeOnly& start,
  const UtcTimeOnly& end,
  bool autoeod,
  bool autoreconnect,
  int autoreconnectinterval,
  bool autoconnect,
  bool autodisconnect
):
  DailySchedule( days, start, end, autoeod, autoreconnect, autoreconnectinterval, autoconnect, autodisconnect )
{
}

bool NormalDailySchedule::isInRange( const UtcTimeStamp& time ) const
{
  int day = time.getWeekDay()-1;
  if( m_Days.find( day ) == m_Days.end() )
    return false;
  int testms = toWeeklyMilliseconds( time ),
    startms = toWeeklyMilliseconds( day, m_start ),
    endms = toWeeklyMilliseconds( day, m_end );

  /*
  if ( ! (testms >= startms && testms <= endms) )
    std::cout << "NormalDailySchedule isInRange?, test " << testms << " against " << startms << " " << endms << " => " << (testms >= startms && testms <= endms) << std::endl;
  */
  return testms >= startms && testms <= endms;
}

ReverseDailySchedule::ReverseDailySchedule(
  std::set<int> days,
  const UtcTimeOnly& start,
  const UtcTimeOnly& end,
  bool autoeod,
  bool autoreconnect,
  int autoreconnectinterval,
  bool autoconnect,
  bool autodisconnect
):
  DailySchedule( days, start, end, autoeod, autoreconnect, autoreconnectinterval, autoconnect, autodisconnect )
{
}

bool ReverseDailySchedule::isInRange( const UtcTimeStamp& time ) const
{
  int testms = toWeeklyMilliseconds( time );
  DaysType::const_iterator it = m_Days.begin();
  do {
    if( testms >= toWeeklyMilliseconds( (*it), m_start ) &&
      testms <= toWeeklyMilliseconds( (*it)+1, m_end ) )
      return true;
    /*
    std::cout << "Day " << *it << "not ok for ReverseWeeklySchedule"
      << ", testms" << testms
      << ", teststart " << toWeeklyMilliseconds( (*it), m_start )
      << ", testend " << toWeeklyMilliseconds( (*it)+1, m_end )
      << std::endl;
      */
    it++;
  } while ( it != m_Days.end() );
  return false;
}

WeeklySchedule::WeeklySchedule(
  int minday,
  int maxday,
  const UtcTimeOnly& mintime,
  const UtcTimeOnly& maxtime,
  bool autoeod,
  bool autoreconnect,
  int autoreconnectinterval,
  bool autoconnect,
  bool autodisconnect
):
  Schedule( autoeod, autoreconnect, autoreconnectinterval, autoconnect, autodisconnect ),
  m_MinWeeklyMilliseconds( toWeeklyMilliseconds( minday, mintime ) ),
  m_MaxWeeklyMilliseconds( toWeeklyMilliseconds( maxday, maxtime ) )
{ }

NormalWeeklySchedule::NormalWeeklySchedule(
  int minday,
  int maxday,
  const UtcTimeOnly& start,
  const UtcTimeOnly& end,
  bool autoeod,
  bool autoreconnect,
  int autoreconnectinterval,
  bool autoconnect,
  bool autodisconnect
):
  WeeklySchedule( minday, maxday, start, end, autoeod, autoreconnect, autoreconnectinterval, autoconnect, autodisconnect )
{ }

bool NormalWeeklySchedule::isInRange( const UtcTimeStamp& time ) const
{
  if ( isAllPass() )
    return true;
  int wm = toWeeklyMilliseconds( time );
  return wm >= my_min() && wm <= my_max();
}

ReverseWeeklySchedule::ReverseWeeklySchedule(
  int minday,
  int maxday,
  const UtcTimeOnly& start,
  const UtcTimeOnly& end,
  bool autoeod,
  bool autoreconnect,
  int autoreconnectinterval,
  bool autoconnect,
  bool autodisconnect
):
  WeeklySchedule( minday, maxday, start, end, autoeod, autoreconnect, autoreconnectinterval, autoconnect, autodisconnect )
{ }

bool ReverseWeeklySchedule::isInRange( const UtcTimeStamp& time ) const
{
  if ( isAllPass() )
    return true;
  int wm = toWeeklyMilliseconds( time );
  //std::cout << "ReverseWeeklySchedule min " << min() << ", max " << max() << std::endl;
  return wm <= my_min() || wm >= my_max();
}

ISchedule* createSchedule( )
{
  return new NullSchedule;
}

ISchedule* createSchedule( const std::string& descriptor )
{
  ScheduleDescriptorParseResult r;
  split(descriptor, "|", std::bind( &ScheduleDescriptorParseResult::onElement, &r, std::placeholders::_1 ) );
  if( !r.allElementsAcquired() )
  {
    std::stringstream strstream;
    strstream << "Schedule descriptor " << descriptor << " needs to have " << r.neededElements() << " elements delimited by |";
    throw FIX::InvalidNumberOfElementsInScheduleDescriptor( strstream.str() );
  }
  bool reverse = r.end < r.start;
  if( r.weekly )
  {
    int min = r.days.empty() ? 0 : *(r.days.begin()),
      max = r.days.empty() ? 0 : *(r.days.rbegin());
    //std::cout << "weekly, start " << r.start << ", end" << r.end << std::endl;
    reverse = reverse && r.days.size() < 2;
    if( reverse && r.days.size() < 1 )
      throw ImpossibleReverseWeeklySchedule( "Reverse weekly schedule cannot have 0 days defined" );
    return reverse ?
      static_cast<ISchedule*> (
        new ReverseWeeklySchedule(
          min,
          max,
          r.start,
          r.end,
          r.autoeod,
          r.autoreconnect,
          r.autoreconnectinterval,
          r.autoconnect,
          r.autodisconnect
        )
      ) :
      static_cast<ISchedule*> (
        new NormalWeeklySchedule(
          min,
          max,
          r.start,
          r.end,
          r.autoeod,
          r.autoreconnect,
          r.autoreconnectinterval,
          r.autoconnect,
          r.autodisconnect
        )
      );
  }
  return reverse ?
    static_cast<ISchedule*> (
      new ReverseDailySchedule(
        r.days,
        r.start,
        r.end,
        r.autoeod,
        r.autoreconnect,
        r.autoreconnectinterval,
        r.autoconnect,
        r.autodisconnect
      )
    ) :
    static_cast<ISchedule*> (
      new NormalDailySchedule(
        r.days,
        r.start,
        r.end,
        r.autoeod,
        r.autoreconnect,
        r.autoreconnectinterval,
        r.autoconnect,
        r.autodisconnect
      )
    );
  return 0;
}
}

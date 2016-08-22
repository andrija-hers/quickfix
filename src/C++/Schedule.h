#ifndef FIX_SCHEDULE
#define FIX_SCHEDULE

#include "ScheduleFactory.h"

namespace FIX
{
class Schedule : public ISchedule
{
  public:
    Schedule(
      bool autoeod,
      bool autoreconnect,
      int autoreconnectinterval,
      bool autoconnect,
      bool autodisconnect
    );
    virtual bool shouldAutoEOD( ) const 
    { return m_autoEOD; }
    virtual bool shouldAutoReconnect( ) const
    { return m_autoReconnect; }
    virtual bool shouldAutoConnect( ) const
    { return m_autoConnect; }
    virtual bool shouldAutoDisconnect( ) const
    { return m_autoDisconnect; }

  protected:
    bool m_autoEOD;
    bool m_autoReconnect;
    int m_autoReconnectInterval;
    bool m_autoConnect;
    bool m_autoDisconnect;
    static int toWeeklyMilliseconds( const UtcTimeStamp& time );
    static int toWeeklyMilliseconds( int day, const UtcTimeOnly& time );
};

class DailySchedule : public Schedule
{
  public:
    DailySchedule (
      std::set<int> days,
      const UtcTimeOnly& start,
      const UtcTimeOnly& end,
      bool autoeod,
      bool autoreconnect,
      int autoreconnectinterval,
      bool autoconnect,
      bool autodisconnect
    );
  protected:
    typedef std::set<int> DaysType;
    DaysType m_Days;
    UtcTimeOnly m_start;
    UtcTimeOnly m_end;
};

class NormalDailySchedule : public DailySchedule
{
  public:
    NormalDailySchedule(
      std::set<int> days,
      const UtcTimeOnly& start,
      const UtcTimeOnly& end,
      bool autoeod,
      bool autoreconnect,
      int autoreconnectinterval,
      bool autoconnect,
      bool autodisconnect
    );
    virtual bool isInRange( const UtcTimeStamp& time ) const ;
};

class ReverseDailySchedule : public DailySchedule
{
  public:
    ReverseDailySchedule (
      std::set<int> days,
      const UtcTimeOnly& start,
      const UtcTimeOnly& end,
      bool autoeod,
      bool autoreconnect,
      int autoreconnectinterval,
      bool autoconnect,
      bool autodisconnect
    );
    virtual bool isInRange( const UtcTimeStamp& time ) const ;
};

class WeeklySchedule : public Schedule
{
  public:
    WeeklySchedule(
      int minday,
      int maxday,
      const UtcTimeOnly& mintime,
      const UtcTimeOnly& maxtime,
      bool autoeod,
      bool autoreconnect,
      int autoreconnectinterval,
      bool autoconnect,
      bool autodisconnect
    );
  protected:
    int min () const { return m_MinWeeklyMilliseconds; }
    int max () const { return m_MaxWeeklyMilliseconds; }
    bool isAllPass () const { return m_MinWeeklyMilliseconds == m_MaxWeeklyMilliseconds; }
  private:
    int m_MinWeeklyMilliseconds;
    int m_MaxWeeklyMilliseconds;
};

class NormalWeeklySchedule : public WeeklySchedule
{
  public:
    NormalWeeklySchedule(
      int minday,
      int maxday,
      const UtcTimeOnly& mintime,
      const UtcTimeOnly& maxtime,
      bool autoeod,
      bool autoreconnect,
      int autoreconnectinterval,
      bool autoconnect,
      bool autodisconnect
    );
    virtual bool isInRange( const UtcTimeStamp& time ) const ;
};

class ReverseWeeklySchedule : public WeeklySchedule
{
  public:
    ReverseWeeklySchedule(
      int minday,
      int maxday,
      const UtcTimeOnly& mintime,
      const UtcTimeOnly& maxtime,
      bool autoeod,
      bool autoreconnect,
      int autoreconnectinterval,
      bool autoconnect,
      bool autodisconnect
    );
    virtual bool isInRange( const UtcTimeStamp& time ) const ;
};

class NullSchedule : public ISchedule
{
  public:
    NullSchedule () { }
    virtual bool isInRange( const UtcTimeStamp& time ) const { return false; }
    virtual bool shouldAutoEOD( ) const { return false; }
    virtual bool shouldAutoReconnect( ) const { return false; }
    virtual bool shouldAutoConnect( ) const { return false; }
    virtual bool shouldAutoDisconnect( ) const { return false; }
};
}

#endif // FIX_SCHEDULE

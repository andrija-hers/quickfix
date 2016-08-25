#ifndef FIX_SCHEDULE_FACTORY
#define FIX_SCHEDULE_FACTORY

#include "TimeRange.h"

#include <string>
#include <stdexcept>

namespace FIX {

struct ISchedule {
  public:
    virtual bool isInRange( const UtcTimeStamp& time ) const = 0;
    virtual bool shouldAutoEOD( ) const = 0;
    virtual bool shouldAutoReconnect( ) const = 0;
    virtual bool shouldAutoDisconnect( ) const = 0;
    virtual bool shouldAutoConnect( ) const = 0;
    virtual int  reconnectInterval( ) const = 0;
};

class InvalidWeeklyScheduleDescriptorElement : public std::out_of_range
{
  public:
    InvalidWeeklyScheduleDescriptorElement( const std::string& what ):
    std::out_of_range( what+" is not w or W or d or D" ) {}
};

class InvalidDaysScheduleDescriptorElement : public std::out_of_range
{
  public:
    InvalidDaysScheduleDescriptorElement( const std::string& what ):
    std::out_of_range( what+"is not a valid Working days descriptor (like 0,1,4)" ) {}
};

class InvalidStartTimeScheduleDescriptorElement : public std::out_of_range
{
  public:
    InvalidStartTimeScheduleDescriptorElement( const std::string& what ):
    std::out_of_range( what+" is not a valid Time description (like 02:45 or 18:44:30)" ) {}
};

class InvalidEndTimeScheduleDescriptorElement : public std::out_of_range
{
  public:
    InvalidEndTimeScheduleDescriptorElement( const std::string& what ):
    std::out_of_range( what+" is not a valid Time description (like 02:45 or 18:44:30)" ) {}
};

class InvalidAutoEODScheduleDescriptorElement : public std::out_of_range
{
  public:
    InvalidAutoEODScheduleDescriptorElement ( const std::string& what ):
    std::out_of_range( what+" is not a valid AutoEOD description (must be AutoEOD or NoAutoEOD)" ) {}
};

class InvalidAutoReconnectScheduleDescriptorElement : public std::out_of_range
{
  public:
    InvalidAutoReconnectScheduleDescriptorElement ( const std::string& what ):
    std::out_of_range( what+" is not a valid AutoReconnect description (must be AutoReconnect or NoAutoReconnect)" ) {}
};

class InvalidAutoReconnectIntervalScheduleDescriptorElement : public std::out_of_range
{
  public:
    InvalidAutoReconnectIntervalScheduleDescriptorElement ( const std::string& what ):
    std::out_of_range( what+" is not a valid AutoReconnectInterval in seconds (must be positive integer)" ) {}
};

class InvalidAutoConnectScheduleDescriptorElement : public std::out_of_range
{
  public:
    InvalidAutoConnectScheduleDescriptorElement ( const std::string& what ):
    std::out_of_range( what+" is not a valid AutoConnect description (must be AutoConnect or NoAutoConnect)" ) {}
};

class InvalidAutoDisconnectScheduleDescriptorElement : public std::out_of_range
{
  public:
    InvalidAutoDisconnectScheduleDescriptorElement  ( const std::string& what ):
    std::out_of_range( what+" is not a valid AutoDisconnect description (must be AutoDisconnect or NoAutoDisconnect)" ) {}
};

class InvalidNumberOfElementsInScheduleDescriptor : public std::out_of_range
{
  public:
    InvalidNumberOfElementsInScheduleDescriptor ( const std::string& what ):
    std::out_of_range( what ) {}
};

ISchedule* createSchedule( );

ISchedule* createSchedule( const std::string& descriptor );
}

#endif // FIX_SCHEDULE_FACTORY


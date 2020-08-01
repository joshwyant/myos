#ifndef __TIMER_H__
#define __TIMER_H__

#ifdef __cplusplus
#include "driver.h"

extern "C" {
#endif

extern void irq0(); // system timer isr

#ifdef __cplusplus
}  // extern "C"
namespace kernel
{
class TimerDriver
    : public Driver
{
public:
    TimerDriver(String device_name = "timer")
        :  Driver(device_name) {}
    virtual bool timer_handler() = 0;
}; // class TimerDriver

class PITTimerDriver
    : public TimerDriver
{
public:
    PITTimerDriver(String device_name = "timer");
    void start() override;
    bool timer_handler() override;
private:
    int pit_reload;
    unsigned timer_seconds;
    unsigned timer_fractions;
}; // class PITTimerDriver

}  // namespace kernel
#endif // __cplusplus

#endif  // __TIMER_H__

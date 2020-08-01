#include "io.h"
#include "interrupt.h"
#include "kernel.h"
#include "process.h"
#include "timer.h"

using namespace kernel;

int pit_reload;
unsigned timer_seconds;
unsigned timer_fractions;

// Initialize the system timer - the Programmable Interval Timer
PITTimerDriver::PITTimerDriver(String device_name)
    : pit_reload(17898),  // roughly every 15ms
      timer_seconds(0),
      timer_fractions(0),
      TimerDriver(device_name)
{
    outb(0x43, 0x34); // Channel 0: rate generator
    outb(0x40, pit_reload&0xFF); // reload value lo byte channel 0
    outb(0x40, pit_reload>>8); // reload value hi byte channel 0
    // fill descriptor 0x20 (irq 0) for timer handler
    register_isr(0x20,0,(void*)irq0);
}

void PITTimerDriver::start()
{
    irq_unmask(0);
}

// called by wrapper irq0()
extern "C" int handle_timer()
{
    return Kernel::current()
            ->drivers()
            ->timer_driver().get()
            ->timer_handler();
}

bool PITTimerDriver::timer_handler()
{
    bool forced = switch_voluntary;
    if (!forced)
    {
        // Calculate the amount of time passed
        asm volatile ("add %2,%0; adc $0,%1":
                      "=g"(timer_fractions),"=g"(timer_seconds):
                      "g"(3600*pit_reload)); // 3600 is (2**32-1)/pit_freq_hz.
    }
    else
    {
        switch_voluntary = false; // reset switch_voluntary for the next timer tick
    }
    if (!processes.first) return false;
    if (current_process) // If we are already in a process
    {
        if (!forced && current_process->timeslice--)
        {
            return false;
        }
        ProcessNode* n;
        do
        {
            n = processes.first;
            process_node_unlink(n); // Take the node off the head of the queue
            process_node_link(n);   // Put it on the rear of the queue
        } while (n->process->blocked);
    }
    current_process = processes.first->process;
    current_process->timeslice = 3-current_process->priority;
    return true;
}

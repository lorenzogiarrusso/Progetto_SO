#include "../headers/lib.h"

extern struct list_head *ready_queue;
extern int process_count, softblock_count;
extern pcb_PTR current_process;

cpu_t startTime;

void scheduler()
{
    if (current_process != NULL)
        current_process->p_time += (CURRENT_TOD - startTime); // update CPU time

    current_process = removeProcQ(&ready_queue);

    if (current_process != NULL)
    {
        setTIMER(TIMESLICE); // Load timeslice (5 milliseconds) on the PLT
        STCK(startTime);     // sets startTime to the value of the low-order word of the TOD clock divided by the Time Scale
        current_process->p_s.status |= TEBITON;
        LDST(&(current_process->p_s));
    }

    else if (process_count == 1) // SSI is the only process in the system
        HALT();

    else if (process_count > 0 && softblock_count > 0)
    {                                  // No ready processes to dispatch, wait for an interrupt
        setSTATUS(IECON | IMON);       //...the Scheduler must first set the Status register to enable interrupts...
        setTIMER(TIMERVALUE(INT_MAX)); //...and load the PLT with a very large value
        WAIT();
    }

    else if (process_count > 0 && softblock_count == 0) // deadlock!
        PANIC();
}
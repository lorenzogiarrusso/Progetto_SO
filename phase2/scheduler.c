#include "../headers/lib.h"

extern struct list_head ready_queue;
extern int process_count, softblock_count;
extern pcb_PTR current_process;

void scheduler()
{
    if (!emptyProcQ(&ready_queue))
    {
        current_process = removeProcQ(&ready_queue);
        setTIMER(TIMESLICE); // Load timeslice (5 milliseconds) on the PLT
        STCK(startTime);     // sets startTime to the value of the low-order word of the TOD clock divided by the Time Scale
        LDST(&(current_process->p_s));
    }

    else if (process_count == 1)
    { // SSI is the only process in the system
        HALT();
    }
    else if (process_count > 0 && softblock_count > 0)
    { // No ready processes to dispatch, wait for an interrupt
        current_process = NULL;
        setSTATUS(ALLOFF | IECON | IMON); //...the Scheduler must first set the Status register to enable interrupts...
        WAIT();
    }

    else if (process_count > 0 && softblock_count == 0)
    { // deadlock!
        PANIC();
    }
}
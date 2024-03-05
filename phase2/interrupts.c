#include "../headers/lib.h"

void PLT_interruptHandler(), IT_interruptHandler(), nonTimer_interruptHandler(int causeIP);
void startInterrupt(), endInterrupt();
/*
 * Returns the IP field found in the Cause Register.
 */
int getCauseIP()
{
    return getCAUSE() & IMON;
}

/*
 * Redirects to the appropriate interrupt handler.
 */
void interruptHandler()
{
    startInterrupt();

    int causeIP = getCauseIP();
    if (causeIP & LOCALTIMERINT)
        PLT_interruptHandler();
    else if (causeIP & TIMERINTERRUPT)
        IT_interruptHandler();
    else
        nonTimer_interruptHandler(causeIP);

    endInterrupt();
}

/*
 * "Freezes" the current process's CPU timer so that the interrupt won't influence it.
 */
void startInterrupt()
{
    if (current_process != NULL)
    {
        // current_process->p_time += ...
        // ^ aggiorna cpu time del processo
    }
    setSTATUS(getSTATUS() & ~TEBITON);
}

/*
 * "Unfreezes" the current process's CPU timer. If the process still exists it gets put back into execution, otherwise the scheduler is called.
 */
void endInterrupt()
{
    setSTATUS(getSTATUS() | TEBITON);
    // STCK(...)
    if (current_process != NULL)
        LDST((state_t *)BIOSDATAPAGE);
    else
        scheduler();
}

void PLT_interruptHandler()
{
}

/*
 * Interval timer went off! Reactivate all processes awaiting the IT, then re-load it with PSECOND (100ms)
 */
void IT_interruptHandler()
{
}

void nonTimer_interruptHandler(int causeIP)
{
}
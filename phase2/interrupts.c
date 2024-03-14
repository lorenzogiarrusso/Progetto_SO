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
    loadTimer(TIMESLICE);

    copyProcessorStateToPCB(&CurrentProcess->p_s); // TF!?!?!?!

    insertProcQ(ready_queue, currentPCB);

    Scheduler();
}

/*
 * Interval timer went off! Reactivate all processes awaiting the IT, then re-load it with PSECOND (100ms)
 */
void IT_interruptHandler()
{
    //1
    LDIT(PSECOND);

    //2
    pcb_PTR currentPCB = headProcQ(blocked_WaitforClock);

    while(currentPCB != NULL){
        insertProcQ(ready_queue, currentPCB);
        pcb_PTR nextcurrentPCB = currentPCB->next;
        removeProcQ(blocked_WaitforClock);
        outProcQ(blocked_queue, currentPCB);
        currentPCB = nextcurrentPCB;
    }

    //3
    state_t *excState = (state_t *)BIOSDATAPAGE;
    LDST(excState)
}

void nonTimer_interruptHandler(int causeIP)
{
    //1
    int IntlineNo = causeIP;
    int DevNo = 0;//Switch su cosa? Questo è il device che usiamo ma su cosa devo andare a controllare la condizione per cambiare device?
    int devAddrBase = 0x10000054 + ((IntlineNo - 3) * 0x80) + (DevNo * 0x10); //Indirizzo del registro del dispositivo(?)

    //2
    int statusCode = *((int *)devAddrBase); //Salvo lo stato del dispositivo

    //3
    *((int *)devAddrBase) = ACK; //Dico di aver ricevuto lo stato del dispositivo
    
    //4
    //Dove e con scritto cosa?
    pcb_PTR *currentPCB = removeProcQ();//QUALE DIO CANE?

    //5
    currentPCB->v0 = statusCode;

    //6
    insertProcQ(ready_queue, currentPCB);

    //7
    LDST();//Di cosa?
}

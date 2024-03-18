#include "../headers/lib.h"
#include <assert.h>

void PLT_interruptHandler(), IT_interruptHandler(), nonTimer_interruptHandler(int causeIP);
void startInterrupt(), endInterrupt();
/*
 * Returns the IP field found in the Cause Register, but only if interrupts are currently not masked.
 * (forse)
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
    setTIMER(TIMESLICE);

    // Questa linea la lascio per i posteri
    // copyProcessorStateToPCB(&CurrentProcess->p_s); // TF!?!?!?!

    current_process->p_s = exc_state; // Non capisco perche' lo si debba fare ma ok

    insertProcQ(&ready_queue, current_process);

    scheduler();
}

/*
 * Interval timer went off! Reactivate all processes awaiting the IT, then re-load it with PSECOND (100ms)
 */
void IT_interruptHandler()
{
    // 1
    LDIT(PSECOND);

    // 2
    pcb_PTR currentPCB = headProcQ(blocked_WaitforClock);

    while (currentPCB != NULL)
    {
        insertProcQ(ready_queue, currentPCB);
        pcb_PTR nextcurrentPCB = currentPCB->next;
        removeProcQ(blocked_WaitforClock);
        outProcQ(blocked_queue, currentPCB);
        currentPCB = nextcurrentPCB;
    }

    // 3
    state_t *excState = (state_t *)BIOSDATAPAGE;
    LDST(excState)
}

int getHighestPriorityDevNo(int causeIP)
{
    if (causeIP < 3 || causeIP > 7)
        return -1; // should never happen; remove if it actually does not happen

    unsigned int bitMapAddressOffset = 0x04 * (causeIP - 3); // 0x00 if interrupt line is 3, 0x04 if line is 4, 0x08 if 5, etc
    unsigned int baseAddr = 0x10000040;
    unsigned int devBitMap = *(baseAddr + bitMapAddressOffset);
    int i = 0;

    while (devBitMap > 0)
    {
        if (devBitMap % 2 == 1)
            return i;
        i++;
        devBitMap <<= 1;
    }

    return -1;
}

void nonTimer_interruptHandler(int causeIP)
{
    // 1
    int IntlineNo = causeIP;
    int DevNo = getHighestPriorityDevNo(causeIP);
    assert(DevNo >= 0 && DevNo <= 7);
    int devAddrBase = 0x10000054 + ((IntlineNo - 3) * 0x80) + (DevNo * 0x10); // Indirizzo del registro del dispositivo

    // 2
    int statusCode = *((int *)devAddrBase); // Salvo lo status code del registro del dispositivo

    // 3
    *((unsigned int *)devAddrBase) = ACK; // Dico di aver ricevuto lo stato del dispositivo
    //? Non mi convince

    // 4
    // Dove e con scritto cosa?
    // TODO: It is possible that there isn’t any PCB waiting for this device. This can happen if while waiting for the initiated I/O operation to complete, an ancestor of this PCB was terminated. In this case, simply return control to the Current Process.
    pcb_PTR unblockedPCB /*= PCB which initiated this I/O operation and then requested the status response via a SYS2 operation*/;

    // 5
    unblockedPCB->reg_v0 = statusCode;

    // 6
    softblock_count--;
    insertProcQ(&ready_queue, unblockedPCB);

    // 7
    LDST(exc_state);
}

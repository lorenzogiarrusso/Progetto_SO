#include "../headers/lib.h"

extern pcb_PTR current_process;

void passUpOrDie(int except_type, state_t *exceptionState)
{
    if (current_process)
    {
        if (currentProcess->p_supportStruct == NULL)
            die();
        else // Pass up
        {
            (currentProcess->p_supportStruct)->sup_exceptState[except_type] = *exceptionState;
            context_t info_to_pass = (currentProcess->p_supportStruct)->sup_exceptContext[except_type]; // Gather support info
            LDCXT(info_to_pass.c_stackPtr, info_to_pass.c_status, info_to_pass.c_pc);                   // Pass up support info to the support level
        }
    }
}

// TODO: Move somewhere else
/*
 * Terminate the current process and its entire subtree.
 */
void die()
{
    outChild(current_process);      // Remove current process from tree
    terminateTree(current_process); // Remove its whole subtree as well

    current_process = NULL;
    scheduler(); // Let scheduler pick the next process to execute
}

// TODO: Move somewhere else
/*
 * Terminate the subtree of the PCB pointed to by to_kill.
 */
static void terminateTree(pcb_PTR to_kill)
{
    if (to_kill == NULL)
        return;

    while (!(emptyChild(to_kill)))
    {
        terminateTree(removeChild(to_kill)); // Recursion to terminate all the children subtrees
    }

    terminateSingleProcess(to_kill);
}

// TODO: Move somewhere else
/*
 * Terminate JUST the PCB pointed to by to_kill.
 */
static void terminateSingleProcess(pcb_PTR to_kill)
{
    process_count--;
    outProcQ(&ready_queue, to_kill); // Removes to_kill from the Ready Queue if it was there, otherwise does nothing

    // TODO: Rimuovere semafori che to_kill stava usando per attendere qualcosa eventualmente!!!!!!

    freePcb(to_kill);
}

void exceptionHandler()
{
    // Here, BIOSDATAPAGE contains the processor state present during the exception
    // The cause of this exception is encoded in the .ExcCode field of the Cause register (Cause.ExcCode)
    // in the saved exception state.
    // Tip: To get the ExcCode you can use getCAUSE(), the constants GETEXECCODE and CAUSESHIFT.
    state_t *excState = (state_t *)BIOSDATAPAGE;     // Exception state
    unsigned int excCode = getCAUSE() & GETEXECCODE; // Exception code
    excCode = excCode >> CAUSESHIFT;

    if (excCode == IOINTERRUPTS) // Interrupts
    {
        interruptHandler(/**/); // TODO: vedi che parametro va passato all'interrupt handler
    }

    else if (excCode >= 1 && excCode <= 3) // TLB Exceptions
    {
        passUpOrDie(PGFAULTEXCEPT, excState);
    }

    else if ((excCode >= 4 && excCode <= 7) || (excCode >= 9 && excCode <= 12)) // Program Traps
    {
        passUpOrDie(GENERALEXCEPT, excState);
    }

    else if (excCode == SYSEXCEPTION) // Syscalls, meglio spostare la gestione in una funzione a se stante
    {
        int KUp = (getSTATUS() & USERPON) >> 3;

        if (KUp == 0) // Current process was executing in kernel-mode
        {
            if (reg_a0 == SENDMESSAGE) // TODO: il reg_a0 di chi??
            {                          // Destination PCB address in a1, payload in a2

                if (/*destination is in pcbFree_h list*/)
                {
                    // TODO: set the caller's v0 register to DEST_NOT_EXIST
                }
                else if (/*destination is in ready queue*/)
                {
                    // TODO: "message is just pushed into its inbox"
                }
                else if (/*destination is waiting for a message*/)
                {
                    // TODO: awaken it and put it into the Ready Queue
                }
            }
            else if (reg_a0 == RECEIVEMESSAGE) // TODO: il reg_a0 di chi??
            {                                  // Sender or ANYMESSAGE in a1, pointer to an area where the nucleus will store the payload of the message in a2 (NULL if the payload should be ignored)
                if ()
            }
        }

        else // Current process was executing in user-mode! Cannot request SYS1 nor SYS2
        {
            excState->cause = RESVINSTR;
            exceptionHandler(); /*"the Nucleus should simulate a Program Trap exception when a privileged service is
                                requested in user-mode. This is done by setting Cause.ExcCode in the stored exception state to RI
                                (Reserved Instruction), and calling one’s Program Trap exception handler."*/
        }
    }
}
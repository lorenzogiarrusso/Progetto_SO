#include "../headers/lib.h"

extern pcb_PTR current_process;

void exceptionHandler()
{
    // Here, BIOSDATAPAGE contains the processor state present during the exception
    // The cause of this exception is encoded in the .ExcCode field of the Cause register (Cause.ExcCode)
    // in the saved exception state.
    // Tip: To get the ExcCode you can use getCAUSE(), the constants GETEXECCODE and CAUSESHIFT.
    unsigned int excCode = getCAUSE() & GETEXECCODE;
    excCode = excCode >> CAUSESHIFT;

    if (excCode == IOINTERRUPTS) // Interrupts
    {
        interruptHandler(...); // TODO: vedi che parametro va passato all'interrupt handler
    }

    else if (excCode >= 1 && excCode <= 3) // TLB Exceptions
    {
        passUpOrDie(...); // TODO: vedi che parametro va passato
    }

    else if ((excCode >= 4 && excCode <= 7) || (excCode >= 9 && excCode <= 12)) // Program Traps
    {
        passUpOrDie(...); // TODO: vedi che parametro va passato
    }

    else if (excCode == SYSEXCEPTION) // Syscalls
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

        else // Current process was executing in user-mode
        // Per ora non vedo che differenza ci debba essere
        {
        }
    }
}
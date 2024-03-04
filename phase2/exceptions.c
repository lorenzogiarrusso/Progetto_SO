#include "../headers/lib.h"
#include <stddef.h>

//?????? Non compila senza ma non chiedetemi il perche'
void memcpy(void *dest, const void *src, size_t n)
{
    for (size_t i = 0; i < n; i++)
    {
        ((char *)dest)[i] = ((char *)src)[i];
    }
}

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

/*
 * Function called by the exception handler to pass up the handling of all events not handled by the interrupt handler nor the syscall handler.
 */
void passUpOrDie(int except_type, state_t *exceptionState)
{
    if (current_process)
    {
        if (current_process->p_supportStruct == NULL)
            die();
        else // Pass up
        {
            (current_process->p_supportStruct)->sup_exceptState[except_type] = *exceptionState;
            context_t info_to_pass = (current_process->p_supportStruct)->sup_exceptContext[except_type]; // Gather support info
            LDCXT(info_to_pass.stackPtr, info_to_pass.status, info_to_pass.pc);                          // Pass up support info to the support level
        }
    }
}

/*
 * To be called by the exception handler upon receiving a SYS1/SYS2 syscall request.
 * Processes the syscall request depending on the parameters specified in the state pointed to by exc_state.
 */
void syscallHandler(state_t *exc_state)
{
    int KUp = (getSTATUS() & USERPON) >> 3;

    if (KUp == 0) // Current process was executing in kernel-mode
    {
        if (exc_state->reg_a0 == SENDMESSAGE) // Destination PCB address in a1, payload in a2
        {

            pcb_PTR dst = (pcb_PTR)(exc_state->reg_a1);

            if ((dst = outProcQ(&pcbFree_h, dst)) != NULL) // The desired destination is not assigned to a currently existing PCB
            {
                insertProcQ(&pcbFree_h, dst); // outProcQ lo aveva rimosso dalla lista dei PCB liberi
                exc_state->reg_v0 = DEST_NOT_EXIST;
            }
            else
            {
                msg_PTR msg = allocMsg();
                msg->m_payload = exc_state->reg_a2;
                msg->m_sender = current_process;

                if ((dst = outProcQ(&ready_queue, dst)) != NULL)
                {
                    pushMessage(&dst->msg_inbox, msg);
                    insertProcQ(&ready_queue, dst); // outProcQ lo aveva rimosso dalla Ready Queue
                }
                else if (/*TODO: "is destination waiting for a message?"*/)
                {
                    pushMessage(&dst->msg_inbox, msg);

                    if (/*TODO: controlla che stia aspettando un messaggio proprio da me (o da chiunque)*/ 1)
                    {
                        softblock_count--;
                        insertProcQ(&ready_queue, dst);
                    }
                }
            }
            exc_state->pc_epc += WORDLEN;
            LDST(exc_state);
        }
        else if (exc_state->reg_a0 == RECEIVEMESSAGE) // Sender or ANYMESSAGE in a1, pointer to an area where the nucleus will store the payload of the message in a2 (NULL if the payload should be ignored)
        {
            // TODO
            // Destinatario specificato in exc_state->reg_a1, locazione dove copiare il payload specificata in exc_state->reg_a2 (se non NULL)
            // Logica: controlla se nell'inbox c'è già un messaggio con il mittente richiesto (qualsiasi se ANYMESSAGE)
            // Se non c'è, si deve bloccare, aggiungendosi alla lista di processi in attesa di messaggi
            // Per bloccarsi: copia exc_state in current_process->p_s, aggiorna il CPU Time del current process (quando lo implementeremo..), inserisci processo nella lista dei bloccati, chiama scheduler()
        }
    }

    else // Current process was executing in user-mode! => Cannot request SYS1 nor SYS2
    {
        exc_state->cause = RESVINSTR;
        exceptionHandler();
    }
}

/*
 * Handles all exceptions, "redirecting" them to the appropriate function depending on the exception code.
 */
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

    else if (excCode == SYSEXCEPTION) // Syscalls
    {
        syscallHandler(excState);
    }
}
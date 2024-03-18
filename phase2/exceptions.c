#include "../headers/lib.h"
#include <stddef.h>

/*
 * Copies the content pointed to by src in dest. Content has size n.
 */
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

            if ((dst = findProcQ(&pcbFree_h, dst)) != NULL) // If desired destination is not assigned to a currently existing PCB
            {
                // NOTE: vecchia implementazione che usava outProcQ e poi reinseriva il PCB rompeva la FIFOness; qui non importa ma lo cambio per consistenza
                exc_state->reg_v0 = DEST_NOT_EXIST;
            }
            else
            {
                msg_PTR msg = allocMsg();
                if (msg == NULL)
                    exc_state->reg_v0 = MSGNOGOOD;
                else
                {
                    msg->m_payload = exc_state->reg_a2;
                    msg->m_sender = current_process;

                    if ((dst = findProcQ(&ready_queue, dst)) != NULL) // If receiver is in the Ready Queue, we simply push the message into its inbox
                    {
                        pushMessage(&dst->msg_inbox, msg);
                        // NOTE: vecchia implementazione che usava outProcQ e poi reinseriva il PCB rompeva la FIFOness, qui sarebbe stato importante
                    }
                    else if ((dst = findProcQ(&blocked_queue, dst)) != NULL) // Receiver is blocked, we push the message into its inbox and unblock it if it was blocked waiting for this message
                    {
                        pushMessage(&dst->msg_inbox, msg);

                        // If receiver is blocked waiting for any message or for my message, unblock it and add it to the Ready Queue!
                        if ((dst->p_s.reg_a0 == RECEIVEMESSAGE) && (dst->p_s.reg_a1 == ANYMESSAGE || (pcb_PTR)(dst->p_s.reg_a1) == current_process))
                        {
                            outProcQ(&blocked_queue, dst);
                            softblock_count--;
                            insertProcQ(&ready_queue, dst);
                        }
                    }

                    exc_state->reg_v0 = 0; // Message sent succesfully
                }
            }
            exc_state->pc_epc += WORDLEN;
            LDST(exc_state);
        }
        else if (exc_state->reg_a0 == RECEIVEMESSAGE) // Sender or ANYMESSAGE in a1, pointer to an area where the nucleus will store the payload of the message in a2 (NULL if the payload should be ignored)
                                                      // NOTE: a1 eventually contains THE ADDRESS OF the desired sender (treat it like a pointer)
                                                      // After the function returns, caller's v0 must contain the identifier of the extracted message's sender
        {
            if (emptyMessageQ(&(current_process->msg_inbox)))
            {
                // No message in the inbox, block the process
                current_process->p_s = exc_state;
                current_process->p_time = ; // TODO p_time
                insertProcQ(&blocked_queue, current_process);
                softblock_count++;
                scheduler();
            }

            else if (exc_state->reg_a1 == ANYMESSAGE)
            {
                msg_t *received_msg = popMessage(&(current_process->msg_inbox), NULL);

                if (exc_state->reg_a2 != NULL)
                {
                    memcpy(exc_state->reg_a2, received_msg->m_payload, sizeof(received_msg->m_payload));
                }

                exc_state->reg_v0 = (unsigned int)(&(received_msg->m_sender));
                freeMsg(received_msg);
            }

            else
            {
                msg_PTR received_msg = outMsgBySender(&(current_process->msg_inbox), (pcb_PTR)(exc_state->reg_a1));

                if (received_msg == NULL)
                {
                    // No message from the desired sender is in the inbox, block the process
                    current_process->p_s = exc_state;
                    current_process->p_time = ; // TODO p_time
                    insertProcQ(&blocked_queue, current_process);
                    softblock_count++;
                    scheduler();
                }

                else
                {
                    if (exc_state->reg_a2 != NULL)
                        memcpy(exc_state->reg_a2, received_msg->m_payload, sizeof(received_msg->m_payload));

                    exc_state->reg_v0 = (unsigned int)(&(received_msg->m_sender));
                    freeMsg(received_msg);
                }
            }

            exc_state->pc_epc += WORDLEN;
            LDST(exc_state);
        }
    }

    else // Current process was executing in user-mode! => Cannot request SYS1 nor SYS2
    {
        exc_state->cause = PRIVINSTR;
        exceptionHandler();
    }
}

/*
 * Handles all exceptions, "redirecting" them to the appropriate function depending on the exception code.
 */
void exceptionHandler()
{
    // Here, BIOSDATAPAGE contains the processor state present during the exception
    // The cause of this exception is encoded in the .exc_code field of the Cause register (Cause.exc_code)
    // in the saved exception state.
    // Tip: To get the exc_code you can use getCAUSE(), the constants GETEXECCODE and CAUSESHIFT.
    exc_state = (state_t *)BIOSDATAPAGE;              // Exception state
    unsigned int exc_code = getCAUSE() & GETEXECCODE; // Exception code
    exc_code = exc_code >> CAUSESHIFT;

    if (exc_code == IOINTERRUPTS) // Interrupts
    {
        interruptHandler(/**/); // TODO: vedi che parametro va passato all'interrupt handler
    }

    else if (exc_code >= 1 && exc_code <= 3) // TLB Exceptions
    {
        passUpOrDie(PGFAULTEXCEPT, exc_state);
    }

    else if ((exc_code >= 4 && exc_code <= 7) || (exc_code >= 9 && exc_code <= 12)) // Program Traps
    {
        passUpOrDie(GENERALEXCEPT, exc_state);
    }

    else if (exc_code == SYSEXCEPTION) // Syscalls
    {
        syscallHandler(exc_state);
    }
}

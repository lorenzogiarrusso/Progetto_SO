#include "../headers/lib.h"

// Update p_time field of given p
void update_time(pcb_t *p)
{
    int end;
    STCK(end);                      // put current time into end
    p->p_time += (end - startTime); // Update p_time to contain the elapsed time
    startTime = end;
}

/*
 * Copies processor state from src to dst
 */
void copyProcessorState(state_t *dst, state_t *src)
{
    dst->entry_hi = src->entry_hi;
    dst->cause = src->cause;
    dst->status = src->status;
    dst->pc_epc = src->pc_epc;
    for (int i = 0; i < STATE_GPR_LEN; i++) // Iterate on General Purpose Register fields
        dst->gpr[i] = src->gpr[i];
    dst->hi = src->hi;
    dst->lo = src->lo;
}

/*
 * Terminate the current process and its entire subtree, then call scheduler.
 */
void die()
{
    ssi_terminate_process(current_process);
    scheduler(); // Let scheduler pick the next process to execute
}

/*
 * Function to create a msg and put it into destination's inbox.
 * Returns 0 if successful, return MSGNOGOOD if all available messages have already been allocated.
 */
int send_msg(pcb_t *snd, pcb_t *dst, unsigned int payload)
{
    msg_t *msg = allocMsg();
    if (msg == NULL) // Ran out of messages!
        return MSGNOGOOD;

    msg->m_sender = snd;
    msg->m_payload = payload;
    insertMessage(&(dst->msg_inbox), msg);
    return 0;
}

/*
 * Function called by the exception handler to pass up the handling of all events not handled by the interrupt handler nor the syscall handler.
 */
void passUpOrDie(int except_type, state_t *exceptionState)
{
    if (current_process)
    {
        if (current_process->p_supportStruct == NULL)
        {
            // "If the Current Process’s p_supportStruct is NULL, then the exception should be handled
            // as a TerminateProcess: the Current Process and all its progeny are terminated. This is the “die” portion of Pass Up or Die."
            die();
        }
        else // Pass up
        {
            /*
            copyProcessorState(&(current_process->p_supportStruct->sup_exceptState[except_type]), exceptionState);
            context_t info_to_pass = (current_process->p_supportStruct)->sup_exceptContext[except_type]; // Gather support info
            LDCXT(info_to_pass.stackPtr, info_to_pass.status, info_to_pass.pc);                          // Pass up support info to the support level
            */
            copyProcessorState(&(current_process->p_supportStruct->sup_exceptState[except_type]), exceptionState);
            LDCXT(current_process->p_supportStruct->sup_exceptContext[except_type].stackPtr,
                  current_process->p_supportStruct->sup_exceptContext[except_type].status,
                  current_process->p_supportStruct->sup_exceptContext[except_type].pc);
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

    if (KUp == 0) // Process was executing in kernel-mode
    {
        if (exc_state->reg_a0 == SENDMESSAGE) // SYS1. Destination PCB address in a1, payload in a2
        {
            int sendResult;
            pcb_PTR dst = (pcb_PTR)(exc_state->reg_a1);
            pcb_PTR isReady = findProcQ(&ready_queue, dst);
            pcb_PTR isNotAllocated = findProcQ(&pcbFree_h, dst);

            if (isNotAllocated != NULL) // If true, the desired destination is not assigned to a currently existing PCB! => destination does not exist
            {
                exc_state->reg_v0 = DEST_NOT_EXIST;
            }
            else if (isReady != NULL || dst == current_process)
            { // If destination is in the Ready Queue or it is the current process (common in test), we simply push the message into its inbox
                sendResult = send_msg(current_process, dst, exc_state->reg_a2);
                exc_state->reg_v0 = sendResult;
            }

            else // Receiver is blocked, we push the message into its inbox and unblock it
            {
                sendResult = send_msg(current_process, dst, exc_state->reg_a2);
                insertProcQ(&ready_queue, dst);
                exc_state->reg_v0 = sendResult;
            }
            exc_state->pc_epc += WORDLEN; // Increase Program Counter's value...
            LDST(exc_state);              // ...and continue execution
        }
        else if (exc_state->reg_a0 == RECEIVEMESSAGE) // SYS2. Sender or ANYMESSAGE in a1, pointer to an area where the nucleus will store the payload of the message in a2 (NULL if the payload should be ignored)
                                                      // NOTE: a1 eventually contains THE ADDRESS OF the desired sender (treat it like a pointer)
                                                      // After the function returns, caller's v0 must contain the identifier of the extracted message's sender
        {
            struct list_head *msg_inbox = &(current_process->msg_inbox);
            unsigned int snd = exc_state->reg_a1;
            msg_PTR msg = popMessage(msg_inbox, (snd == ANYMESSAGE ? NULL : (pcb_PTR)(snd))); // Fetch the first message sent by snd currently in the inbox. NULL if none.
            if (msg == NULL)
            {
                // No satisfactory message in the inbox: save its state, update its time and block the process
                copyProcessorState(&(current_process->p_s), exc_state);
                update_time(current_process);
                // softblock_count++; ??? non serve perchè non inserito in una queue, forse?

                scheduler();
            }

            else
            {
                // Satisfactory message was found, receive it
                exc_state->reg_v0 = (memaddr)(msg->m_sender); // Caller's v0 must contain the identifier of the extracted message's sender
                if (msg->m_payload != (unsigned int)NULL)
                {
                    unsigned int *payload_ptr = (unsigned int *)exc_state->reg_a2;
                    *payload_ptr = msg->m_payload;
                }
                freeMsg(msg);                 // Mark the message's "slot" as free once again, as it has now served its purpose
                exc_state->pc_epc += WORDLEN; // Increase Program Counter's value...
                LDST(exc_state);              // ...and continue execution
            }
        }
        else if (exc_state->reg_a0 >= 1)
            passUpOrDie(GENERALEXCEPT, exc_state);
    }

    else // Current process was executing in user-mode! => Cannot request SYS1 nor SYS2
    {
        exc_state->cause = (exc_state->cause & CLEAREXECCODE) | (PRIVINSTR << CAUSESHIFT); //"Clear up" exception code, then write PRIVINSTR
        passUpOrDie(GENERALEXCEPT, exc_state);
    }
}

/*
 * Handles all exceptions, "redirecting" them to the appropriate function depending on the exception code.
 */
void exceptionHandler()
{
    // "Here, BIOSDATAPAGE contains the processor state present during the exception
    // The cause of this exception is encoded in the .exc_code field of the Cause register (Cause.exc_code)
    // in the saved exception state.
    // Tip: To get the exc_code you can use getCAUSE(), the constants GETEXECCODE and CAUSESHIFT."
    state_t *exc_state = (state_t *)BIOSDATAPAGE; // Exception state
    int cause = getCAUSE();                       // cause = cause register
    unsigned int exc_code = cause & GETEXECCODE;  // Exception code
    exc_code = exc_code >> CAUSESHIFT;

    if (exc_code == IOINTERRUPTS) // Interrupts
    {
        interruptHandler(cause, exc_state);
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
    else
    {
        PANIC();
    }
}

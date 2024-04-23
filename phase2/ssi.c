#include "../headers/lib.h"
#include <umps/libumps.h>

#define TERMINALLINE 7

// Codice di Lorenzo da qui

unsigned int ssi_request(pcb_PTR caller, ssi_payload_PTR payload);

/*
 * Function to block a PCB on the specified line by inserting it onto the appropriate blocked queue.
 * term > 0 for transmit onto terminal, otherwise for receive from terminal
 */
void blockPCB(pcb_PTR p, int line, int term)
{
    outProcQ(&ready_queue, p);
    struct list_head *correspondingQueue = NULL;
    if (line == IL_DISK)
    {
        correspondingQueue = &blocked_disk;
    }
    else if (line == IL_FLASH)
    {
        correspondingQueue = &blocked_flash;
    }
    else if (line == IL_ETHERNET)
    {
        correspondingQueue = &blocked_eth;
    }
    else if (line == IL_PRINTER)
    {
        correspondingQueue = &blocked_printer;
    }
    else if (line == IL_TERMINAL)
    {
        if (term == 0)
            correspondingQueue = &blocked_terminal_receive;
        else
            correspondingQueue = &blocked_terminal_transmit;
    }

    if (correspondingQueue != NULL)
        insertProcQ(correspondingQueue, p);
    // the condition for the if should never be false at this point!
}

/*
 * Given an address, calculate corresponding device line and number. Then, block the given pcb on said device.
 */
void blockPCBfromAddr(memaddr addr, pcb_PTR p)
{
    // Special handling for terminal devices due to difference between transmit and receive
    for (int i = 0; i <= 7; i++)
    {
        termreg_t *base_addr = (termreg_t *)DEV_REG_ADDR(7, i);
        if ((memaddr) & (base_addr->recv_command) == addr)
        {
            p->dev_n = i;
            blockPCB(p, TERMINALLINE, 0); // 0 => receive
            return;
        }
        else if ((memaddr) & (base_addr->transm_command) == addr)
        {
            p->dev_n = i;
            blockPCB(p, TERMINALLINE, 1); // 1 => transmit
            return;
        }
    }

    // Handle all non-terminal devices
    for (int i = 3; i <= 6; i++)
    {
        for (int j = 0; j <= 7; j++)
        {
            dtpreg_t *base_addr = (dtpreg_t *)DEV_REG_ADDR(i, j);
            if ((memaddr) & (base_addr->command) == addr)
            {
                p->dev_n = j;
                blockPCB(p, i, -1);
                return;
            }
        }
    }
}

void ssi()
{
    while (1)
    {
        unsigned int payload; // To contain the received message

        // Wait for requests; synchronous
        unsigned int snd = SYSCALL(RECEIVEMESSAGE, ANYMESSAGE, (unsigned int)(&payload), 0);

        unsigned int result; // will contain the value returned from ssi_request. If -1 there's no need to send a message to the requester.
        // Otherwise, result itself will be the payload to be sent.

        result = ssi_request((pcb_PTR)snd, (ssi_payload_t *)payload);

        if (result != -1)
            SYSCALL(SENDMESSAGE, (unsigned int)snd, result, 0);
    }
}

/*
 * Instantiate a new PCB as child of caller
 * NOTE: CALLER MUST CHECK IF pcbFree_h IS EMPTY BEFORE CALLING THIS FUNCTION!!
 */
pcb_PTR ssi_create_process(ssi_create_process_PTR p_info, pcb_PTR parent)
{
    // NOTE: CALLER MUST CHECK IF pcbFree_h IS EMPTY BEFORE CALLING THIS FUNCTION!!
    pcb_PTR newPCB = allocPcb();
    newPCB->p_pid = pid_count;
    pid_count++;
    newPCB->p_supportStruct = p_info->support;
    copyProcessorState(&(newPCB->p_s), p_info->state);

    insertChild(parent, newPCB);
    insertProcQ(&ready_queue, newPCB);

    process_count++;
    return newPCB;
}

/*
 * Recursively terminate p and its whole subtree.
 */
void ssi_terminate_process(pcb_PTR p)
{
    if (p)
    {
        while (!emptyChild(p))                     // Iterate on p's children
            ssi_terminate_process(removeChild(p)); // Remove current child from children list, then recursively terminate it
    }
    process_count--;

    // If PCB is in a blocked queue, remove it then decrease softblock_count
    int wasInBlockedQueue = outProcQ(&blocked_terminal_receive, p) || outProcQ(&blocked_terminal_transmit, p) || outProcQ(&blocked_disk, p) || outProcQ(&blocked_flash, p) || outProcQ(&blocked_printer, p) || outProcQ(&blocked_eth, p) || outProcQ(&blocked_IT, p);
    if (wasInBlockedQueue)
        softblock_count--;

    outChild(p); // Utile solo per la chiamata "originale", in quelle ricorsive p è già stato scollegato dal padre
    freePcb(p);  // Terminazione effettiva
}

/*
 * Make caller wait for Pseudo-Clock/Interval Timer
 */
void ssi_waitForIT(pcb_PTR caller)
{
    softblock_count++;
    insertProcQ(&blocked_IT, caller);
}

/*
 * Returns caller's PID if wantParent==NULL, returns parent's pid otherwise.
 */
int ssi_getPID(pcb_PTR caller, void *wantParent)
{
    if (!wantParent)
        return caller->p_pid;
    else
        return caller->p_parent->p_pid;
}

/*
 * Makes caller blocked on the corresponding device's queue
 */
void ssi_DoIO(pcb_PTR caller, ssi_do_io_PTR do_io)
{
    softblock_count++;
    blockPCBfromAddr((memaddr)do_io->commandAddr, caller); // Block caller on the queue corresponding to the specified address
    *(do_io->commandAddr) = do_io->commandValue;
}

/*
 * Function called during ssi() loop to satisfy the received request, represented through the message's payload.
 */
unsigned int ssi_request(pcb_PTR caller, ssi_payload_PTR payload)
{
    unsigned int res = 0;
    switch (payload->service_code)
    {
    case CREATEPROCESS:
        if (emptyProcQ(&pcbFree_h))
            res = NOPROC;
        else
            res = (unsigned int)ssi_create_process((ssi_create_process_PTR)payload->arg, caller);
        break;
    case TERMPROCESS:
        // If arg==NULL terminate caller, otherwise terminate process pointed to by arg
        if (payload->arg)
        {
            ssi_terminate_process(payload->arg);
            res = 0;
        }
        else
        {
            ssi_terminate_process(caller);
            res = -1;
        }
        break;
    case DOIO:
        ssi_DoIO(caller, payload->arg);
        res = -1;
        break;
    case GETTIME:
        res = (unsigned int)caller->p_time;
        break;
    case CLOCKWAIT:
        ssi_waitForIT(caller);
        res = -1;
        break;
    case GETSUPPORTPTR:
        res = (unsigned int)caller->p_supportStruct;
        break;
    case GETPROCESSID:
        res = (unsigned int)ssi_getPID(caller, payload->arg);
        break;
    default:
        ssi_terminate_process(caller);
        res = -1;
        break;
    }
    return res;
}
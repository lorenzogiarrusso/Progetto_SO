#include "../headers/lib.h"
#include <umps/arch.h>

/*
 * Remove p with given device number from given list. Return p if found, NULL otherwise.
 */
pcb_t *unblockPcbByDevNum(int devnum, struct list_head *list)
{
    pcb_PTR tmp;
    list_for_each_entry(tmp, list, p_list)
    {
        if (tmp->dev_n == devnum)
            return outProcQ(list, tmp);
    }
    return NULL; // Not found
}

/*
 * Based on the given bitmap, return the device number associated to the highest priority interrupt currently active.
 * NOTE: DEVICE NUMBER AND INTERRUPT LINE ARE DIFFERENT THINGS BUT THEY HAPPEN TO BOTH HAVE VALUES IN THE 0...7 INTERVAL
 */
int getHighestPriorityDevNo(int bitmap)
{
    if (bitmap & DEV7ON)
        return 7;
    else if (bitmap & DEV6ON)
        return 6;
    else if (bitmap & DEV5ON)
        return 5;
    else if (bitmap & DEV4ON)
        return 4;
    else if (bitmap & DEV3ON)
        return 3;
    else if (bitmap & DEV2ON)
        return 2;
    else if (bitmap & DEV1ON)
        return 1;
    else if (bitmap & DEV0ON)
        return 0;
    return -1; // Should never happen
}

void deviceInterruptHandler(int line, int cause, state_t *exc_state)
{
    pcb_PTR unblockedPCB;

    // SPIEGAZIONE DI QUESTA PARTE: dev_reg_area punta all'area di memoria contenente tutte le bitmap che tengono traccia di quali dispositivi
    // hanno un'interrupt attiva per ogni interrupt line. Accedo alla bitmap relativa alla linea richiesta tramite parametro. Da questa bitmap,
    // estraggo il device number del dispositivo con massima priorita' che sta attualmente chiedendo un'interrupt sulla linea in questione.
    devregarea_t *dev_reg_area = (devregarea_t *)BUS_REG_RAM_BASE;
    unsigned int intDevsBitmap = dev_reg_area->interrupt_dev[line - 3]; // Variable that contains the bitmap of devices on the line on which the interrupt was heard
    unsigned int devStatus;
    unsigned int devNumber;

    devNumber = getHighestPriorityDevNo(intDevsBitmap);

    // Special handling for terminals, as they are handled as 2 sub-devices!
    if (line == IL_TERMINAL)
    {
        termreg_t *dev_reg = (termreg_t *)DEV_REG_ADDR(line, devNumber); // dev_reg = device register
        if (((dev_reg->transm_status) & 0x000000FF) == 5)                // Device is requesting a transmit towards a terminal
        {
            // Output to terminal
            devStatus = dev_reg->transm_status;
            dev_reg->transm_command = ACK; // "Send" acknowledgement to the device
            unblockedPCB = unblockPcbByDevNum(devNumber, &blocked_terminal_transmit);
        }
        else
        {
            // Input from terminal
            devStatus = dev_reg->recv_status;
            dev_reg->recv_command = ACK; // "Send" acknowledgement to the device
            unblockedPCB = unblockPcbByDevNum(devNumber, &blocked_terminal_receive);
        }
    }

    // Handling for non-terminal devices
    else
    {
        dtpreg_t *dev_reg = (dtpreg_t *)DEV_REG_ADDR(line, devNumber);
        devStatus = dev_reg->status;
        dev_reg->command = ACK;

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

        // Terminals are missing because terminal-related interrupt have been dealt with in the other branch of the if

        if (correspondingQueue == NULL)
            unblockedPCB = NULL;
        else
            unblockedPCB = unblockPcbByDevNum(devNumber, correspondingQueue);
    }

    if (unblockedPCB != NULL) // Found a PCB to unblock; unblock it
    {
        unblockedPCB->p_s.reg_v0 = devStatus;
        send_msg(ssi_pcb, unblockedPCB, (memaddr)devStatus); // Send message to unblocked process
        insertProcQ(&ready_queue, unblockedPCB);
        softblock_count--;
    }
    if (current_process != NULL)
    {
        LDST(exc_state); // Resume current process
    }
    else
        scheduler(); // No process was executing before the interrupt, call scheduler
}

// Handle Process Local Timer interrupt
void PLT_interruptHandler(state_t *exc_state)
{
    // Current process ran out of time. Copy processor state, re-insert it into ready queue, call scheduler
    setTIMER(-1);
    update_time(current_process);
    copyProcessorState(&(current_process->p_s), exc_state);
    insertProcQ(&ready_queue, current_process);
    scheduler();
}

/*
 * Interval timer went off! Reactivate all processes awaiting the IT/pseudoclock, then re-load it with PSECOND (100ms)
 */
void IT_interruptHandler(state_t *exc_state)
{
    // 1
    LDIT(PSECOND);

    // 2
    pcb_PTR currentPCB;
    while ((currentPCB = removeProcQ(&blocked_IT)) != NULL) // Reactivate all processes waiting for IT/pseudoclock (aka the whole blocked_IT queue)
    {
        send_msg(ssi_pcb, currentPCB, 0);
        insertProcQ(&ready_queue, currentPCB);
        softblock_count--;
    }

    // 3
    if (current_process != NULL)
        LDST(exc_state); // Resume current process
    else
        scheduler(); // No process was executing before the interrupt, call scheduler
}

/*
 * Redirects to the appropriate interrupt handler.
 */
void interruptHandler(int cause, state_t *exc_state)
{
    if (CAUSE_IP_GET(cause, IL_CPUTIMER))
        PLT_interruptHandler(exc_state);
    else if (CAUSE_IP_GET(cause, IL_TIMER))
        IT_interruptHandler(exc_state);
    else if (CAUSE_IP_GET(cause, IL_DISK))
        deviceInterruptHandler(IL_DISK, cause, exc_state);
    else if (CAUSE_IP_GET(cause, IL_FLASH))
        deviceInterruptHandler(IL_FLASH, cause, exc_state);
    else if (CAUSE_IP_GET(cause, IL_ETHERNET))
        deviceInterruptHandler(IL_ETHERNET, cause, exc_state);
    else if (CAUSE_IP_GET(cause, IL_PRINTER))
        deviceInterruptHandler(IL_PRINTER, cause, exc_state);
    else if (CAUSE_IP_GET(cause, IL_TERMINAL))
        deviceInterruptHandler(IL_TERMINAL, cause, exc_state);
}
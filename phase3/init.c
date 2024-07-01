#include "../headers/lib.h"

memaddr current_addr;

swap_t swap_pool_table[POOLSIZE];
pcb_PTR SSTs_array[UPROCMAX];              // Array of pointers to SSTs
pcb_PTR terminal_processes[UPROCMAX];      // Array of pointers to terminal processes' PCBs
pcb_PTR printer_processes[UPROCMAX];       // Array of pointers to printer processes' PCBs
state_t uproc_states[UPROCMAX];            // Array of U-processes' states
support_t uproc_support_structs[UPROCMAX]; // Array of U-processes' support structures (support_t)
pcb_PTR swap_mutex_process;                // Pointer to the PCB for the swap mutex process
pcb_PTR test_process;                      // Pointer to the PCB for the phase 3 test process

// Initializes SSTs
void init_SSTs()
{
    // TODO

    for (int asid = 1; asid <= UPROCMAX; asid++)
    {
        // ASID 0 is reserved
    }
}

// Wrapper function to request the creation of a process to the SSI; returns
// a pointer to the newly created process.
pcb_PTR createNewProcess(support_PTR support, state_t *state)
{
    ssi_create_process_t tmpCreate;
    tmpCreate.support = support;
    tmpCreate.state = state;

    ssi_payload_t tmpPayload;
    tmpPayload.arg = &tmpCreate;
    tmpPayload.service_code = CREATEPROCESS;

    pcb_PTR newP = NULL;
    SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)&tmpPayload, 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&newP), 0);
    return newP;
}

// Initializes the swap mutex process
void init_swapMutexProc()
{
}

// Initializes a generic page table entry
void init_PTE()
{
}

// Initializes all U-Processes
void init_UProcs()
{
    // Vedi sezione 10.1
    for (int asid = 1; asid <= UPROCMAX; asid++)
    {
        // Initialize state
        //"PC (and s_t9) set to 0x8000.00B0; the address of the start of the .text section"
        uproc_states[asid - 1].pc_epc = (memaddr)UPROCSTARTADDR;
        uproc_states[asid - 1].reg_t9 = (memaddr)UPROCSTARTADDR;

        //"SP set to 0xC000.0000"
        uproc_states[asid - 1].reg_sp = (memaddr)USERSTACKTOP;

        //"Status set for user-mode with all interrupts and the processor Local Timer enabled"
        uproc_states[asid - 1].status = ALLOFF | IEPON | IMON | USERPON | TEBITON;

        //"EntryHi.ASID set to the process’s unique ID; an integer from [1..8]"
        uproc_states[asid - 1].entry_hi = asid << ASIDSHIFT;

        // Sezione 10.1.1
        // Initialize SST
        // TODO
    }
}

// Iniitializes the Swap Pool Table
void init_SPT()
{
    for (int i = 0; i < POOLSIZE; i++)
        swap_pool_table[i].sw_asid = NOPROC; // Invalidates ASID field by assigning -1 to it
}

// Initializes device processes
void init_deviceProcesses()
{
}

void test() {}
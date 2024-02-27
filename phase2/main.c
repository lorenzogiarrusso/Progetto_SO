#include "../headers/lib.h"
#include "p2test.c"

// Declare the Level 3 global variables
unsigned int process_count;
unsigned int softblock_count;
struct list_head ready_queue;
pcb_PTR current_process;
struct list_head blocked_pcbs;
pcb_PTR ssi_pcb;

extern void test(); // from p2test.c

// Non so a cosa serva ma era nelle specifiche
void uTLB_RefillHandler()
{
    setENTRYHI(0x80000000);
    setENTRYLO(0x00000000);
    TLBWR();
    LDST((state_t *)0x0FFFF000);
}

int main(void)
{

    // Initialize Processor0 Pass Up Vector (qualsiasi cosa sia)
    passupvector_t *passupvector = (passupvector_t *)PASSUPVECTOR;
    passupvector->tlb_refill_handler = (memaddr)uTLB_RefillHandler;
    passupvector->tlb_refill_stackPtr = KERNELSTACK;
    passupvector->exception_handler = (memaddr)exceptionHandler;
    passupvector->exception_stackPtr = KERNELSTACK;

    // Initialize phase 1 structures
    initPcbs();
    initMsgs();

    // Initialize the previously declared variables
    process_count = 0;
    softblock_count = 0;
    mkEmptyProcQ(&ready_queue);
    current_process = NULL;
    mkEmptyProcQ(&blocked_pcbs);
    ssi_pcb = NULL;

    // Load the system-wide Interval Timer with 100 milliseconds (constant PSECOND)
    LDIT(PSECOND);

    // SSI PCB
    pcb_PTR first = allocPcb();
    insertProcQ(&ready_queue, first);
    process_count++;
    first->p_s.status |= IEPON | IMON; // Enable interrupts, set interrupt mask to all 1s
    first->p_s.status & ~USERPON;      // TODO?? Enable kernel mode; PENSO? DALLA DOCUMENTAZIONE SEMBRA CHE IL BIT DEVE ESSERE A 0 PER KERNEL MODE
    // TODO: RAMTOP(???) non capisco dove sia lo stack pointer del pcb
    first->p_s.pc_epc = (memaddr)ssi;
    first->p_s.s_t9 = (memaddr)ssi;

    // Instantiate a second process, place its PCB in the Ready Queue, and increment Process Count.
    pcb_PTR second = allocPcb();
    insertProcQ(&ready_queue, second);
    process_count++;
    second->p_s.status |= IEPON | IMON | TEBITON; // Enable interrupts, set interrupt mask to all 1s, enable PLT
    second->p_s.status & ~USERPON;                // TODO?? Enable kernel mode; PENSO? DALLA DOCUMENTAZIONE SEMBRA CHE IL BIT DEVE ESSERE A 0 PER KERNEL MODE
    // TODO: Di nuovo roba del RAMTOP che non so come si faccia
    second->p_s.pc_epc = (memaddr)test;
    second->p_s.s_t9 = (memaddr)test;

    // Call the scheduler
    scheduler();
    return 0;
}

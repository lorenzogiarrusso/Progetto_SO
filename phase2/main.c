#include "../headers/lib.h"
#include "p2test.c"

// Declare the Level 3 global variables
int process_count;   // How many processes currently exist
int softblock_count; // How many processes are currently blocked in one of the queues below
int pid_count;       // Contains the value that will be assigned to the PID of the next allocated process

int startTime; // Value of timer at the start of currently executing PCB

struct list_head ready_queue;               // Queue for processes in Ready state
struct list_head blocked_disk;              // Queue of processes blocked waiting for external disk
struct list_head blocked_flash;             // Queue of processes blocked waiting for flash
struct list_head blocked_IT;                // Queue of processes blocked waiting for pseudoclock
struct list_head blocked_terminal_receive;  // Queue of processes blocked waiting for a terminal receive
struct list_head blocked_terminal_transmit; // Queue of processes blocked waiting for a terminal transmit
struct list_head blocked_eth;               // Queue of processes blocked waiting for Ethernet
struct list_head blocked_printer;           // Queue of processes blocked waiting for printer

pcb_PTR ssi_pcb;         // Pointer to the SSI PCB (used in interrupts.c)
pcb_PTR current_process; // Pointer to the currently-executing PCB

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
    // Initialize Processor0 Pass Up Vector
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
    pid_count = 3; // PID 1 = SSI, PID 2 = test
    mkEmptyProcQ(&ready_queue);
    mkEmptyProcQ(&blocked_disk);
    mkEmptyProcQ(&blocked_flash);
    mkEmptyProcQ(&blocked_IT);
    mkEmptyProcQ(&blocked_terminal_receive);
    mkEmptyProcQ(&blocked_terminal_transmit);
    mkEmptyProcQ(&blocked_eth);
    mkEmptyProcQ(&blocked_printer);
    current_process = NULL;
    ssi_pcb = NULL;

    // Load the system-wide Interval Timer with 100 milliseconds (constant PSECOND)
    LDIT(PSECOND);

    // SSI PCB
    ssi_pcb = allocPcb();
    ssi_pcb->p_pid = 1;
    ssi_pcb->p_s.status = ALLOFF | IEPON | IMON | TEBITON; // Implicitly enable Kernel mode, enable interrupts, set interrupt mask to all 1s, enable timer
    RAMTOP(ssi_pcb->p_s.reg_sp);
    ssi_pcb->p_s.pc_epc = (memaddr)ssi;
    ssi_pcb->p_s.reg_t9 = (memaddr)ssi;
    insertProcQ(&ready_queue, ssi_pcb);
    process_count++;

    // Test PCB
    pcb_PTR test_pcb = allocPcb();
    test_pcb->p_pid = 2;
    test_pcb->p_s.status = ALLOFF | IEPON | IMON | TEBITON; // Implicitly enable Kernel mode, enable interrupts, set interrupt mask to all 1s, enable timer
    RAMTOP(test_pcb->p_s.reg_sp);
    test_pcb->p_s.reg_sp = test_pcb->p_s.reg_sp - 2 * PAGESIZE;
    test_pcb->p_s.pc_epc = (memaddr)test;
    test_pcb->p_s.reg_t9 = (memaddr)test;
    insertProcQ(&ready_queue, test_pcb);
    process_count++;

    // Call scheduler, which should then "execute" the SSI, which should then get blocked waiting for requests, with execution passing to the test PCB
    scheduler();
    return 0;
}

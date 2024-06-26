#ifndef LIB_H
#define LIB_H

#include <umps/libumps.h>
#include <umps/types.h>
#include <umps/arch.h>
#include <limits.h>
#include "const.h"
#include "listx.h"
#include "msg.h"
#include "pcb.h"
#include "types.h"
#include "klog.h"

extern pcb_PTR current_process;
extern struct list_head ready_queue;
extern struct list_head blocked_disk;              // Queue of processes blocked waiting for external disk
extern struct list_head blocked_flash;             // Queue of processes blocked waiting for flash
extern struct list_head blocked_IT;                // Queue of processes blocked waiting for pseudoclock
extern struct list_head blocked_terminal_receive;  // Queue of processes blocked waiting for a terminal receive
extern struct list_head blocked_terminal_transmit; // Queue of processes blocked waiting for a terminal transmit
extern struct list_head blocked_eth;               // Queue of processes blocked waiting for Ethernet
extern struct list_head blocked_printer;           // Queue of processes blocked waiting for printer
extern int process_count;
extern int pid_count;
extern int softblock_count;
extern struct list_head pcbFree_h;
extern int startTime;
extern pcb_PTR ssi_pcb;

void exceptionHandler(), interruptHandler(), scheduler(), ssi(), uTLB_RefillHandler(), copyProcessorState(state_t *dst, state_t *src), update_time(pcb_t *p);
int send_msg(pcb_t *snd, pcb_t *dst, unsigned int payload);
void ssi_terminate_process(pcb_PTR p);

#endif
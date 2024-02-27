#ifndef LIB_H
#define LIB_H

#include <umps/libumps.h>
#include <limits.h>
#include "const.h"
#include "listx.h"
#include "msg.h"
#include "pcb.h"
#include "types.h"


extern pcb_PTR current_process;
extern struct list_head ready_queue;
extern struct list_head blocked_pcbs;
extern unsigned int process_count;
extern unsigned int softblock_count;

void exceptionHandler(), interruptHandler (), scheduler (), ssi(), uTLB_RefillHandler();

#endif
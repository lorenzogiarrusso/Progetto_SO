#ifndef PCB_H_INCLUDED
#define PCB_H_INCLUDED

#include "const.h"
#include "types.h"
#include "listx.h"

void initPcbs();
void freePcb(pcb_PTR p);
pcb_PTR allocPcb();
void mkEmptyProcQ(struct list_head *head);
int emptyProcQ(struct list_head *head);
void insertProcQ(struct list_head *head, pcb_PTR p);
pcb_PTR headProcQ(struct list_head *head);
pcb_PTR removeProcQ(struct list_head *head);
pcb_PTR outProcQ(struct list_head *head, pcb_PTR p);
int emptyChild(pcb_PTR p);
void insertChild(pcb_PTR prnt, pcb_PTR p);
pcb_PTR removeChild(pcb_PTR p);
pcb_PTR outChild(pcb_PTR p);

#endif

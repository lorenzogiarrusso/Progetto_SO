#include "../headers/pcb.h"

static pcb_t pcbTable[MAXPROC];
LIST_HEAD(pcbFree_h);

/*
 * Initialize the pcbFree list to contain all the elements of the static array of MAXPROC PCBs.
 * This method will be called only once during data structure initialization.
 */
void initPcbs()
{
    for (int i = 0; i < MAXPROC; i++)
        list_add(&(pcbTable[i].p_list), &pcbFree_h); // Inserisci ogni elemento dell'array nella lista di PCB liberi
}

/*
 * Provide initial values for the PCB's fields. Used when allocating a "new" PCB.
 * PCBs get reused, so it is important that no previous value persist in a PCB when it gets reallocated.
 */
static void reinitPcb(pcb_PTR p)
{
    INIT_LIST_HEAD(&(p->msg_inbox));
    INIT_LIST_HEAD(&(p->p_child));
    INIT_LIST_HEAD(&(p->p_list));
    p->p_parent = NULL;
    p->p_pid = 0;
    INIT_LIST_HEAD(&(p->p_sib));
    p->p_supportStruct = NULL;
    p->p_time = 0;

    p->p_s.cause = 0;
    p->p_s.entry_hi = 0;
    p->p_s.hi = 0;
    p->p_s.lo = 0;
    p->p_s.pc_epc = 0;
    p->p_s.status = 0;
}

/*
 * Insert the element pointed to by p onto the pcbFree list.
 */
void freePcb(pcb_PTR p)
{
    list_del(&(p->p_list));
    list_add_tail(&(p->p_list), &pcbFree_h); // Inserisci in fondo alla lista di PCB liberi (gestita con FIFO)
}

/*
 * Return NULL if the pcbFree list is empty. Otherwise, remove an element from the pcbFree
 * list, provide initial values for ALL of the PCBs fields (i.e. NULL and/or 0) and then return
 * a pointer to the removed element. PCBs get reused, so it is important that no previous
 * value persist in a PCB when it gets reallocated.
 */
pcb_PTR allocPcb()
{
    struct list_head *extracted = list_next(&pcbFree_h);

    if (extracted == NULL) // Ovvero se la lista di PCB liberi e' vuota
        return NULL;
    else
    {
        pcb_PTR newPCB = container_of(extracted, struct pcb_t, p_list);
        list_del(extracted); // Rimuovi il PCB dalla lista di PCB liberi

        reinitPcb(newPCB);

        return newPCB;
    }
}

/*
 * This method is used to initialize a variable to be head pointer to a process queue.
 */
void mkEmptyProcQ(struct list_head *head)
{
    INIT_LIST_HEAD(head); // Fa puntare sia next che prev all'elemento stesso, indicando una sentinella per lista vuota
}

/*
 * Return TRUE if the queue whose head is pointed to by head is empty. Return FALSE otherwise.
 */
int emptyProcQ(struct list_head *head)
{
    return list_empty(head);
}

/*
 * Enqueues the PCB pointed by p into the process queue whose head pointer is pointed to by head.
 */
void insertProcQ(struct list_head *head, pcb_PTR p)
{
    list_add_tail(&(p->p_list), head); // Inserimento in fondo alla coda (FIFO)
}

/*
 * Return a pointer to the first PCB from the process queue whose head is pointed to by head. Do
 * not remove this PCB from the process queue. Return NULL if the process queue is empty.
 */
pcb_PTR headProcQ(struct list_head *head)
{
    struct list_head *firstElem = list_next(head);
    if (firstElem == NULL) // Ovvero la lista e' vuota
        return NULL;
    else
        return container_of(firstElem, struct pcb_t, p_list);
}

/*
 * Remove the first (i.e. head) element from the process queue whose head pointer is pointed to
 * by head. Return NULL if the process queue was initially empty; otherwise return the pointer
 * to the removed element.
 */
pcb_PTR removeProcQ(struct list_head *head)
{
    pcb_PTR headPcb = headProcQ(head);
    if (headPcb == NULL) // Ovvero la lista e' vuota
        return NULL;
    else
    {
        list_del(head->next); // Rimuovi dalla lista,
        return headPcb;       // poi restituisci il PCB in questione
    }
}

/*
 * Remove the PCB pointed to by p from the process queue whose head pointer is pointed to by
 * head. If the desired entry is not in the indicated queue (an error condition), return NULL;
 * otherwise, return p. Note that p can point to any element of the process queue.
 */
pcb_PTR outProcQ(struct list_head *head, pcb_PTR p)
{
    pcb_t *iter;
    list_for_each_entry(iter, head, p_list)
    { // Itera sui PCB veri e propri nella coda
        if (iter == p)
            return removeProcQ(list_prev(&(p->p_list))); // PCB trovato nella coda e rimosso
    }
    return NULL; // PCB NON trovato nella coda
}

/*
 * Looks for the PCB pointed to by p in the process queue whose head pointer is pointed to by head.
 * If not found, return NULL. If found, returns p WITHOUT removing it from the queue.
 */
pcb_PTR findProcQ(struct list_head *head, pcb_PTR p)
{
    pcb_t *iter;
    list_for_each_entry(iter, head, p_list)
    { // Itera sui PCB veri e propri nella lista
        if (iter == p)
            return p; // PCB trovato nella coda e restituito
    }
    return NULL; // PCB NON trovato nella lista
}

/*
 * Return TRUE if the PCB pointed to by p has no children. Return FALSE otherwise.
 */
int emptyChild(pcb_PTR p)
{
    return emptyProcQ(&(p->p_child));
}

/*
 * Make the PCB pointed to by p a child of the PCB pointed to by prnt.
 */
void insertChild(pcb_PTR prnt, pcb_PTR p)
{
    p->p_parent = prnt;
    if (emptyChild(prnt))
    { // prnt non ha figli, prnt->p_child deve puntare a p->p_sib
        list_add_tail(&(p->p_sib), &(prnt->p_child));
    }
    else
    { // prnt ha gia' dei figli, devo inserire p in fondo alla coda di fratelli
        list_add_tail(&(p->p_sib), &(prnt->p_child));
    }
}

/*
 * Make the first child of the PCB pointed to by p no longer a child of p. Return NULL if initially
 * there were no children of p. Otherwise, return a pointer to this removed first child PCB.
 */
pcb_PTR removeChild(pcb_PTR p)
{
    if (emptyChild(p)) // Ovvero p non ha figli
        return NULL;
    else
    {
        pcb_PTR firstChild = removeProcQ(&(p->p_child));
        return firstChild;
    }
}

/*
 * Make the PCB pointed to by p no longer the child of its parent. If the PCB pointed to by p has
 * no parent, return NULL; otherwise, return p. Note that the element pointed to by p could be
 * in an arbitrary position (i.e. not be the first child of its parent).
 */
pcb_PTR outChild(pcb_PTR p)
{
    pcb_PTR parent = p->p_parent;
    if (parent == NULL)
        return NULL;
    else
    {
        list_del(&(p->p_sib));
        return p;
    }
}

#include "../headers/msg.h"

static msg_t msgTable[MAXMESSAGES];
LIST_HEAD(msgFree_h);

/*
 * Initialize the msgFree list to contain all the elements of the static array of MAXMESSAGES
 * messages. This method will be called only once during data structure initialization.
 */
void initMsgs()
{
    for (int i = 0; i < MAXMESSAGES; i++)
        list_add(&(msgTable[i].m_list), &msgFree_h); // Inserisci ogni elemento dell'array nella lista di messaggi liberi
}

/*
 * Provide initial values for the message's fields. Used when allocating a "new" message.
 * Messages get reused, so it is important that no previous value persist in a message when it gets reallocated.
 */
static void reinitMsg(msg_t *m)
{
    INIT_LIST_HEAD(&(m->m_list));
    m->m_payload = 0;
    m->m_sender = NULL;
}

/*
 * Insert the element pointed to by m onto the msgFree list.
 */
void freeMsg(msg_t *m)
{
    list_del(&(m->m_list));
    list_add_tail(&(m->m_list), &msgFree_h); // Inserisci in fondo alla lista di messaggi liberi (gestita con FIFO)
}

/*
 * Return NULL if the msgFree list is empty. Otherwise, remove an element from the msgFree
 * list, provide initial values for ALL of the messages fields (i.e. NULL and/or 0) and then
 * return a pointer to the removed element. Messages get reused, so it is important that no
 * previous value persist in a message when it gets reallocated.
 */
msg_t *allocMsg()
{
    struct list_head *extracted = list_next(&msgFree_h);

    if (extracted == NULL) // Ovvero se la lista di messaggi liberi e' vuota
        return NULL;
    else
    {
        msg_t *newMsg = container_of(extracted, struct msg_t, m_list);
        list_del(extracted); // Rimuovi il messaggio dalla lista di messaggi liberi

        reinitMsg(newMsg);

        return newMsg;
    }
}

/*
 * Used to initialize a variable to be head pointer to a message queue; returns a pointer to the head
 * of an empty message queue, i.e. NULL.
 */
void mkEmptyMessageQ(struct list_head *head)
{
    INIT_LIST_HEAD(head); // Fa puntare sia next che prev all'elemento stesso, indicando una sentinella per lista vuota
}

/*
 * Returns TRUE if the queue whose tail is pointed to by head is empty, FALSE otherwise.
 */
int emptyMessageQ(struct list_head *head)
{
    return list_empty(head);
}

/*
 * Insert the message pointed to by m at the end of the queue whose head pointer is pointed to by
 * head.
 */
void insertMessage(struct list_head *head, msg_t *m)
{
    list_add_tail(&(m->m_list), head);
}

/*
 * Insert the message pointed to by m at the head of the queue whose head pointer is pointed to by
 * head.
 */
void pushMessage(struct list_head *head, msg_t *m)
{
    list_add(&(m->m_list), head);
}

/*
 * Remove the first element (starting by the head) from the message queue accessed via head whose
 * sender is p_ptr.
 * If p_ptr is NULL, return the first message in the queue. Return NULL if the message queue
 * was empty or if no message from p_ptr was found; otherwise return the pointer to the removed
 * message.
 */
msg_t *popMessage(struct list_head *head, pcb_PTR p_ptr)
{
    msg_t *iter;

    if (p_ptr == NULL && !emptyMessageQ(head))
        return headMessage(head);
    list_for_each_entry(iter, head, m_list)
    { // Itera sui messaggi veri e propri nella lista
        if (iter->m_sender == p_ptr)
        { // Messaggio trovato nella lista
            msg_t *found = iter;
            list_del(&(iter->m_list));
            return found;
        }
    }
    return NULL; // Messaggio NON trovato nella lista
}

/*
 * Return a pointer to the first message from the queue whose head is pointed to by head. Do not
 * remove the message from the queue. Return NULL if the queue is empty.
 */
msg_t *headMessage(struct list_head *head)
{
    struct list_head *firstElem = list_next(head);
    if (firstElem == NULL) // Ovvero la lista e' vuota
        return NULL;
    else
        return container_of(firstElem, struct msg_t, m_list);
}

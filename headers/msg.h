#ifndef MSG_H_INCLUDED
#define MSG_H_INCLUDED

#include "const.h"
#include "types.h"
#include "listx.h"

void initMsgs();
void freeMsg(msg_t *m);
msg_t *allocMsg();
void mkEmptyMessageQ(struct list_head *head);
int emptyMessageQ(struct list_head *head);
void insertMessage(struct list_head *head, msg_t *m);
void pushMessage(struct list_head *head, msg_t *m);
msg_t *popMessage(struct list_head *head, pcb_PTR p_ptr);
msg_t *headMessage(struct list_head *head);
msg_PTR outMsgBySender(struct list_head *head, pcb_PTR snd);

#endif

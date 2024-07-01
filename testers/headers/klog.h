#ifndef KLOG_H
#define KLOG_H

void klog_print(char *str);
void klog_print_dec(unsigned int num);

void klog_print_hex(unsigned int num);

void next_char(void);
void next_line(void);

#endif
#ifndef _LIST_H
#define _LIST_H

/* list.c */
extern void init_list();
extern int add_list(void *, size_t);
extern void clean_list();
extern size_t get_length();
extern int get_list(void *, size_t);

#endif

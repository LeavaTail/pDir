#ifndef _LIST_H
#define _LIST_H

/* list.c */
extern void init_list(void);
extern int add_list(void *, size_t);
extern void clean_list(void);
extern size_t get_length(void);
extern size_t get_listcount(void);
extern int get_list(void *, size_t);

#endif

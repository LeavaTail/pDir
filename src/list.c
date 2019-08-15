/**
 * @file list.c
 * @brief linked list data structure (FIFO)
 * @author LeavaTail
 * @date 2019/08/15
 *
 * HOU TO USE
 * 1. init_list();
 * 2. add_list(&data, sizeof(data));
 * 3. get_list(&data2, sizeof(data));
 * 4. remove_list();
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * ERROR STATUS CODE
 *  1: allocation failed(malloc)
 *  2: refer to none linked-list
 *  3: argument pointer is illegal
 */
#define ALLOCATION_FAILURE	1
#define GETNONE_FAILURE	2
#define ILLEGAL_ARGUMENT_FAILURE	3

/**
 * struct __list_head - Bidirectional linked list.
 * @len:  `data` length (xx bytes)
 * @data: list member
 * @next: next list pointer
 * @prev: previous list pointer
 *
 * General data structure. Bidirectional linked list.
 */
typedef struct __list_head
{
	size_t len;
	void *data;
	struct __list_head *next;
	struct __list_head *prev;
} list_head;

/**
 * list_for_each - iterate over a list
 * @pos: the &struct list_head to use as a loop cursor.
 * @head: the head for your list.
 */
#define list_for_each(pos, head)                                               \
	for (pos = (head)->next; pos != head; pos = pos->next)

/**
 * list_for_each_reverse - iterate over a list reverse
 * @pos: the &struct list_head to use as a loop cursor.
 * @head: the head for your list.
 */
#define list_for_each_reverse(pos, head)                                       \
	for (pos = (head)->prev; pos != head; pos = pos->prev)


//! linked list head pointer
static list_head *head;

//! linked-list data count
static size_t count;

/**
 * init_list - Initialize linked-list
 *
 * linked-list `head` Initialize.(allocation, next/prev pointer set)
 * If Failure allocation, cause normal process termination.
 */
void init_list()
{
	list_head *dummy = malloc(sizeof(*dummy));
	if(!dummy) {
		perror("Initialize linked-list");
		exit(ALLOCATION_FAILURE);
	}
	count = 0;
	dummy->next = dummy;
	dummy->prev = dummy;
	head = dummy;
}

/**
 * add_list - Add data(`d`) to linked-list
 * @d: target data address.
 * @l: target data length (xx bytes)
 *
 * Add data to linked-list `head`.
 *
 * Return: 0 - success
 *         otherwise - error(show ERROR STATUS CODE)
 */
int add_list(void *d, size_t l)
{
	int ret = 0;
	list_head *new = malloc(sizeof(*new));
	if(!new) {
		perror("Add linked-list `list`");
		ret = ALLOCATION_FAILURE;
		goto end;
	}

	new->data = malloc(l);
	if(!new->data) {
		perror("Add linked-list `data`");
		ret = ALLOCATION_FAILURE;
		goto failed_alloc;
	}

	// Set data myself
	new->data = d ? memcpy(new->data, d, l) : NULL;
	new->len = l;
	new->next = head->next;
	new->prev = head;

	// Update data previous/next
	head->next->prev = new;
	head->next = new;
	count++;
	goto end;

failed_alloc:
	free(new);
end:
	return ret;
}

/**
 * clean_list - clean up linked-list
 *
 * clean up linked-list.
 * WARN: Be sure clean up list when use linked list.
 */
void clean_list()
{
	list_head *cursor;
	list_for_each(cursor, head) {
		free(cursor->data);
		free(cursor);
	}
	free(head);
}

/**
 * get_length - get length to linked-list.
 *
 * get length to linked-list(FIFO).
 *
 * Return: 0 - no list
 *         otherwise - first data length
 */
size_t get_length()
{
	return count > 0 ? head->prev->len : 0;
}

/**
 * get_list - get data to linked-list.
 * @d: output data address.
 * @l: output data length (xx bytes)
 *
 * get data to linked-list(FIFO). use this before `get_length()` and
 * allocate sufficient memory.
 * WARN: If specified length less than actual data size, you might
 *       get incorrect data.
 *
 * Return: 0 - success
 *         otherwise - error(show ERROR STATUS CODE)
 */
int get_list(void *d, size_t l)
{
	int ret = 0;
	list_head *list;
	size_t size;

	if(count <= 0) {
		ret = GETNONE_FAILURE;
		goto end;
	}

	if(!d) {
		ret = ILLEGAL_ARGUMENT_FAILURE;
		goto end;
	}

	list = head->prev;
	size = l > list->len ? l : list->len;

	d = list->data ? memcpy(d, list->data, size) : NULL;
	list->prev->next = list->next;
	head->prev = list->prev;

	free(list->data);
	free(list);
	count--;

end:
	return ret;
}

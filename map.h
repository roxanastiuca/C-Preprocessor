/* definition of type hashmap_t and type list_t */

#ifndef MAP_H_
#define MAP_H_	1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

typedef struct node {
	char *symbol; /* the key */
	char *mapping; /* the unique value for that key */
	struct node *next; /* the next node in list */
} node_t, *list_t;

list_t new_node(char *symbol, char *mapping);

typedef struct {
	list_t items; /* list of items in map */
	int items_no; /* number of items */
} hashmap_t;

hashmap_t *new_hashmap(void);
void free_hashmap(hashmap_t *map);
int insert_item(hashmap_t *map, char *symbol, char *mapping);
int delete_item(hashmap_t *map, char *symbol);
char *get_mapping(hashmap_t *map, char *symbol);

/* DEBUG ONLY */
void print_map(hashmap_t *map);

#endif

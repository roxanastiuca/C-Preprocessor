#include <stdlib.h>
#include <string.h>
#include <errno.h>

typedef struct node {
	char *symbol;
	char *mapping;
  	struct node *next;
} node_t, *list_t;

list_t new_node(char *symbol, char *mapping);

typedef struct {
	list_t items;
	int items_no;
} hashmap_t;

hashmap_t* new_hashmap();
void free_hashmap(hashmap_t *map);
int insert_item(hashmap_t *map, char *symbol, char *mapping);
int delete_item(hashmap_t *map, char *symbol);
char *get_mapping(hashmap_t *map, char *symbol);

// DEBUG ONLY
void print_map(hashmap_t *map);
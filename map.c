#include "map.h"

list_t new_node(char *symbol, char *mapping)
{
	list_t node = (list_t) malloc(sizeof(node_t));

	if (node) {
		size_t symbol_len = strlen(symbol);
		size_t mapping_len = strlen(mapping);

		node->symbol = (char *) calloc(1 + symbol_len, sizeof(char));
		if (!node->symbol) {
			free(node);
			return NULL;
		}

		node->mapping = (char *) calloc(1 + mapping_len, sizeof(char));
		if (!node->mapping) {
			free(node->symbol);
			free(node);
			return NULL;
		}

		memcpy(node->symbol, symbol, symbol_len);
		memcpy(node->mapping, mapping, mapping_len);
		node->next = NULL;
	}

	return node;
}

hashmap_t *new_hashmap()
{
	hashmap_t *map = (hashmap_t *) calloc(1, sizeof(hashmap_t));

	if (map) {
		map->items = NULL;
		map->items_no = 0;
	}

	return map;
}

int update_item(list_t item, char *mapping)
{
	free(item->mapping);

	item->mapping = (char *) calloc(1 + strlen(mapping), sizeof(char));
	if (!item->mapping) {
		fprintf(stderr, "Not enough memory for mapping.\n");
		return ENOMEM;
	}

	memcpy(item->mapping, mapping, 1 + strlen(mapping));

	return 0;
}

int insert_item(hashmap_t *map, char *symbol, char *mapping)
{
	list_t last_item = NULL, item = map->items;
	list_t new_item;

	/* Check if symbol already exists. Update it if so. */
	for (; item != NULL; last_item = item, item = item->next) {
		if (strcmp(symbol, item->symbol) == 0) {
			int r = update_item(item, mapping);
			return r;
		}
	}

	new_item = new_node(symbol, mapping);
	if (!new_item) {
		fprintf(stderr, "Not enough memory for new hashmap item.\n");
		return ENOMEM;
	}

	if (last_item == NULL)
		map->items = new_item; /* First insertion in list. */
	else
		last_item->next = new_item; /* Insertion at end of list. */

	map->items_no++;

	return 0;
}


int delete_item(hashmap_t* map, char *symbol)
{
	list_t last_item = NULL, item = map->items;

	/* Search for symbol. */
	for (; item != NULL; last_item = item, item = item->next) {
		if (strcmp(symbol, item->symbol) == 0) {
			break;
		}
	}

	if (item == NULL) {
		return -1; /* item not found; */
	}

	if (last_item == NULL) {
		map->items = item->next; /* Item is first in list. */
	} else {
		last_item->next = item->next;
	}

	free(item->symbol);
	free(item->mapping);
	free(item);

	map->items_no--;

	return 0;
}

char* get_mapping(hashmap_t *map, char *symbol)
{
	list_t item;

	for (item = map->items; item != NULL; item = item->next)
		if (strcmp(symbol, item->symbol) == 0)
			return item->mapping;

	return NULL;
}

void free_hashmap(hashmap_t *map)
{
	list_t item = map->items, aux;

	while (item != NULL) {
		aux = item;
		item = item->next;

		free(aux->symbol);
		free(aux->mapping);
		free(aux);
	}

	free(map);
}
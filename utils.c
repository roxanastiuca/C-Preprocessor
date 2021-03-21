#include "utils.h"

/*
 * Descriptions: frees memory for a vector of dynamically allocated strings.
 */
void free_string_vector(char **vect, int len)
{
	int i;

	for (i = 0; i < len; i++)
		if (vect[i] != NULL)
			free(vect[i]);
	free(vect);
}

/*
 * Description: trims leading whitespace for a given string.
 */
void trim_whitespace(char *str)
{
	char aux[MAXBUF];
	char *start;

	memcpy(aux, str, strlen(str) + 1);

	start = aux;
	while (isspace(*start))
		start++;

	memcpy(str, start, strlen(start) + 1);
}

/*
 * Description: extracts all words in a string using a list of delimiters.
 * Output: 0 for no error, -ENOMEM.
 */
int extract_words(char *str, char ***ref_words, int *words_no)
{
	char buffer[MAXBUF];
	char **words, *token, **words_aux;
	int capacity, idx;

	/* Copy into another buffer because strtok destroys original string. */
	memcpy(buffer, str, MAXBUF);

	words = (char **) calloc(SIZEMIN, sizeof(char *));
	if (!words) {
		fprintf(stderr, "Not enough memory for words.\n");
		return -ENOMEM;
	}

	capacity = SIZEMIN;
	idx = 0;

	token = strtok(buffer, DELIMLIST);

	while (token != NULL) {
		if (idx == capacity) {
			/* Array requires more space. */
			words_aux = (char **) realloc(words,
				(capacity * 2) * sizeof(char *));
			if (words_aux == NULL) {
				/* free space for words */
				free_string_vector(words, idx);
				fprintf(stderr,
					"Not enough memory for realloc.\n");
				return -ENOMEM;
			}
			capacity *= 2;
			words = words_aux;
		}

		words[idx] = (char *) calloc(1 + strlen(token), sizeof(char));
		if (words[idx] == NULL) {
			free_string_vector(words, idx);
			fprintf(stderr, "Not enough memory for word.\n");
			return -ENOMEM;
		}
		memcpy(words[idx++], token, strlen(token));
		token = strtok(NULL, DELIMLIST);
	}

	*ref_words = words;
	*words_no = idx;

	return 0;
}

/*
 * Description: checks if the character pointed to by pos is between quotation
 marks in the string buffer. This happens if the number of quotation marks
 between buffer and pos is odd.
 * Output: 1 if between quotations, 0 if not.
 */
int between_quotations(char *buffer, char *pos)
{
	char *left_mark, *aux;
	int marks_no = 0;

	left_mark = strchr(buffer, '\"');

	if (left_mark == NULL)
		return 0;

	aux = left_mark;

	while (aux != NULL && aux < pos) {
		marks_no++;
		left_mark = aux;
		aux = strchr(aux + 1, '\"');
	}

	return (marks_no % 2 == 1);
}

/*
 * Description: extract symbol and mapping from a string such as:
 symbol=mapping. If str is just "symbol" then mapping is the empty string.
 */
int extract_define(char *str, char **ref_symbol, char **ref_mapping)
{
	char *p = strchr(str, '=');

	if (p != NULL) {
		/* symbol=mapping */
		*ref_mapping = p + 1;
		*p = '\0';
		*ref_symbol = str;
	} else {
		/* symbol (map="") */
		*ref_symbol = str;
		*ref_mapping = str + strlen(str);
	}

	return 0;
}

/*
 * Description: replaces all symbols from buffer with their mappings from
 defmap.
 * Output: number of replaces applied.
 */
int replace_defines(
	hashmap_t *defmap,
	char *buffer, char **words, int words_no)
{

	int start, offset, replaces, i;
	char to_print[MAXBUF]; /* the resulting string */
	char *buffer_copy, *mapping, *pos;

	/* Corner cases where we don't replace (part of directives): */
	if (words_no >= 1
		&& (strcmp(words[0], UNDEF_DIRECTIVE) == 0
		|| strcmp(words[0], IFDEF_DIRECTIVE) == 0
		|| strcmp(words[0], IFNDEF_DIRECTIVE) == 0
		|| strcmp(words[0], INCLUDE_DIRECTIVE) == 0)) {
		return 0;
	}

	start = 0; /* starting index for words that can be replaces */

	/* Corner case for define. Replace only mapping, not symbol: */
	if (words_no >= 1 && strcmp(words[0], DEFINE_DIRECTIVE) == 0)
		start = 2;

	offset = 0; /* offset in to_print where we are copying */
	buffer_copy = buffer;
	replaces = 0; /* number of replaces applied */

	for (i = start; i < words_no; i++) {
		mapping = get_mapping(defmap, words[i]);

		if (mapping != NULL) {
			pos = strstr(buffer, words[i]);

			if (buffer != pos) {
				/* Copy everything until that word: */
				memcpy(to_print + offset, buffer,
					pos - buffer + 1);
				offset += (pos - buffer);
			}

			if (!between_quotations(buffer_copy, pos)) {
				/* Copy mapping: */
				memcpy(to_print + offset, mapping,
					strlen(mapping) + 1);
				offset += strlen(mapping);
			} else {
				/* Don't replace if is between quotation
				 * marks:
				 */
				memcpy(to_print + offset, words[i],
					strlen(words[i]) + 1);
				offset += strlen(words[i]);
			}

			/* Move buffer ahead, after word mapped: */
			buffer = pos + strlen(words[i]);

			replaces++;
		}
	}

	memcpy(to_print + offset, buffer, strlen(buffer) + 1);
	offset += strlen(buffer);
	to_print[offset] = '\0';

	/* Place back into string: */
	buffer = buffer_copy;
	memcpy(buffer, to_print, MAXBUF);

	return replaces;
}

/*
 * Description: finds and opens for reading a file in a list of folders.
 Opens first found file matching its name.
 * Output is NULL if file is not found in any of the folders given.
 */
FILE *find_file(char *file, char **folders, int folders_no)
{
	char path[MAXBUF];
	FILE *fd;
	int i;

	for (i = 0; i < folders_no; i++) {
		strcpy(path, folders[i]);
		strcat(path, "/");
		strcat(path, file);

		fd = fopen(path, "rt");
		if (fd)
			return fd;
	}

	return NULL;
}

#include "utils.h"

void free_string_vector(char **vect, int len) {
	int i;

	for (i = 0; i < len; i++) {
		if (vect[i] != NULL) {
			free(vect[i]);
		}
	}
	free(vect);
}

int extract_define(char *str, char **ref_symbol, char **ref_mapping) {
	char *p;

	if ((p = strchr(str, '=')) != NULL) {
		/* sym=map */
		*ref_mapping = p + 1;
		*p = '\0';
		*ref_symbol = str;
	} else {
		/* sym (map="") */
		*ref_symbol = str;
		*ref_mapping = str + strlen(str);
	}

	return 0;
}

void trim_whitespace(char *str) { /* :) and backslash */
	char aux[MAXBUF];
	memcpy(aux, str, strlen(str) + 1);

	char *start = aux;
	while (isspace(*start))
		start++;

	int i = strlen(start) - 1;
	while (i > 0 && (isspace(start[i])))
		i--;

	start[i] == '\0';
	memcpy(str, start, strlen(start) + 1);
}

int extract_words(char *str, char ***ref_words, int *words_no) {
	/* Copy into another buffer because strtok destroys original string. */
	char buffer[MAXBUF];
	memcpy(buffer, str, MAXBUF);

	char **words = (char **) calloc(SIZEMIN, sizeof(char *));
	if (!words) return ENOMEM;

	int capacity = SIZEMIN;
	int idx = 0;

	char *token = strtok(buffer, DELIMLIST);

	while (token != NULL) {
		if (idx == capacity) {
			/* need more space to stock words array */
			char **words_aux = (char **) realloc(words, (capacity * 2) * sizeof(char*));
			if (words_aux == NULL) {
				/* free space for words */
				free_string_vector(words, idx);
				return ENOMEM;
			}
			capacity *= 2;
			words = words_aux;
		}

		words[idx] = (char *) calloc(1 + strlen(token), sizeof(char));
		if (words[idx] == NULL) {
			free_string_vector(words, idx);
			return ENOMEM;
		}
		memcpy(words[idx++], token, strlen(token));
		token = strtok(NULL, DELIMLIST);
	}

	*ref_words = words;
	*words_no = idx;

	return 0;
}

int init(
	int argc, char *argv[],
	hashmap_t *defmap,
	FILE **fin, FILE **fout,
	char ***folders, int *folders_no
	) {

	char *input_file = NULL;
	*fin = stdin;
	*fout = stdout;
	int r;

	*folders = (char **) calloc(SIZEMIN, sizeof(char *));
	if (!*folders)	return ENOMEM;
	*folders_no = 1; /* reserve first spot for input file folder/current folder */
	int capacity = SIZEMIN;

	int i;

	for (i = 1; i < argc; i++) {
		if (strncmp(argv[i], D_ARG, strlen(D_ARG)) == 0) {
			char *symbol, *mapping;
			if (strlen(argv[i]) == strlen(D_ARG)) {
				r = extract_define(argv[i + 1], &symbol, &mapping); /* -D sym=map */
				if (r)	return r;
				i++;
			} else {
				r = extract_define(argv[i] + 2, &symbol, &mapping); /* -Dsym=map */
				if (r)	return r;
			}
			insert_item(defmap, symbol, mapping);
		} else if (strncmp(argv[i], O_ARG, strlen(O_ARG)) == 0) {
			char *output_file;
			if (strlen(argv[i]) == strlen(O_ARG)) {
				output_file = argv[i + 1];
				i++;
			} else {
				output_file = argv[i] + 2;
			}
			*fout = fopen(output_file, "wt");
		} else if (strncmp(argv[i], I_ARG, strlen(I_ARG)) == 0) {
			if (*folders_no == capacity) {
				char **folders_aux = (char **) realloc(*folders, (capacity * 2) * sizeof(char*));
				if (folders_aux == NULL)	return ENOMEM;
				capacity *= 2;
				*folders = folders_aux;
			}

			char *dir;
			if (strlen(argv[i]) == strlen(I_ARG)) {
				dir = argv[i + 1];
				i++;
			} else {
				dir = argv[i] + 2;
			}

			(*folders)[*folders_no] = (char *) calloc(1 + strlen(dir), sizeof(char));
			if ((*folders)[*folders_no] == NULL)	return ENOMEM;

			memcpy((*folders)[(*folders_no)++], dir, strlen(dir));
		} else {
			if (*fin == stdin) {
				/* input file is the first positional argument. */
				input_file = argv[i];
				*fin = fopen(input_file, "rt");
				if (!*fin)	return ENOENT;
			} else if (*fout == stdout) {
				/* output file is the second possible positional argument. */
				*fout = fopen(argv[i], "wt");
				if (!*fout)	return ENOENT;

			} else {
				return EINVAL;
			}
		}
	}

	if (input_file == NULL) {
		(*folders)[0] = (char *) calloc(2, sizeof(char));
		if ((*folders)[0] == NULL)	return ENOMEM;
		strcpy((*folders)[0], ".");
	} else {
		char *end_of_path = strrchr(input_file, '/');
		if (end_of_path == NULL) {
			(*folders)[0] = (char *) calloc(2, sizeof(char));
			if ((*folders)[0] == NULL)	return ENOMEM;
			strcpy((*folders)[0], ".");
		} else {
			*end_of_path = '\0';
			(*folders)[0] = (char *) calloc(1 + strlen(input_file), sizeof(char));
			if ((*folders)[0] == NULL)	return ENOMEM;
			strcpy((*folders)[0], input_file);
		}
	}

	return 0;
}

void end_program(hashmap_t *map, FILE *fin, FILE *fout, char **folders, int folders_no) {
	// free_hashmap(map);
	// fclose(fin);
	// fclose(fout);
	// free_string_vector(folders, folders_no);
}

int between_quotations(char *buffer, char *pos) {
	char *left_mark = strchr(buffer, '\"');

	if (left_mark == NULL) {
		return 0;
	}

	char *aux = left_mark;
	int marks_no = 0;

	while (aux != NULL && aux < pos) {
		marks_no++;
		left_mark = aux;
		aux = strchr(aux + 1, '\"');
	}

	return (marks_no % 2 == 1);
}

int replace_defines(
	hashmap_t *defmap,
	char *buffer, char **words, int words_no
	) {

	/* corner cases where we don't replace: */
	if (words_no >= 1
		&& (strcmp(words[0], UNDEF_DIRECTIVE) == 0
		|| strcmp(words[0], IFDEF_DIRECTIVE) == 0
		|| strcmp(words[0], IFNDEF_DIRECTIVE) == 0
		|| strcmp(words[0], INCLUDE_DIRECTIVE) == 0)) {
		return 0;
	}

	int start = 0;

	/* corner case for define. replace only mapping */
	if (words_no >= 1 && strcmp(words[0], DEFINE_DIRECTIVE) == 0) {
		start = 2;
	}

	int offset = 0;
	char to_print[MAXBUF];
	char *buffer_copy = buffer;

	int replaces = 0;
	int i;

	for (i = start; i < words_no; i++) {
		char *mapping = get_mapping(defmap, words[i]);

		if (mapping != NULL) {
			char *pos = strstr(buffer, words[i]);

			if (buffer != pos) {
				/* copy everything until that word */
				memcpy(to_print + offset, buffer, pos - buffer);
				offset += (pos - buffer);
			}

			if (!between_quotations(buffer_copy, pos)) {
				/* copy mapping */
				memcpy(to_print + offset, mapping, strlen(mapping));
				offset += strlen(mapping);
			} else {
				/* don't replace if is between "symbol" */
				memcpy(to_print + offset, words[i], strlen(words[i]));
				offset += strlen(words[i]);
			}

			/* move buffer ahead, after word mapped */
			buffer = pos + strlen(words[i]);

			replaces++;
		}
	}

	memcpy(to_print + offset, buffer, strlen(buffer));
	offset += strlen(buffer);
	to_print[offset] = '\0';

	/* place back into string */
	buffer = buffer_copy;
	memcpy(buffer, to_print, MAXBUF);

	return replaces;
}

int handle_define(
	FILE *fin,
	hashmap_t *defmap,
	char *buffer,
	char **words, int words_no
	) {

	char mapping[MAXBUF];

	if (words_no >= 3) {
		int offset = MAPPING_OFFSET + strlen(words[1]);
		memcpy(mapping, buffer + offset, strlen(buffer + offset) + 1);
		mapping[strlen(buffer + offset) - 1] = '\0';
		trim_whitespace(mapping);

		while (buffer[strlen(buffer) - 2] == '\\') {
			fgets(buffer, MAXBUF, fin);

			char aux[MAXBUF];
			memcpy(aux, buffer, MAXBUF);

			aux[strlen(aux) - 1] = '\0';
			trim_whitespace(aux);

			mapping[strlen(mapping) - 1] = '\0';
			strcat(mapping, " ");
			strcat(mapping, aux);
		}
	} else {
		mapping[0] = '\0';
	}

	int r = insert_item(defmap, words[1], mapping);
	return r;
}

FILE *find_file(char *file, char **folders, int folders_no) {
	int i;
	
	for (i = 0; i < folders_no; i++) {
		char path[MAXBUF];
		strcpy(path, folders[i]);
		strcat(path, "/");
		strcat(path, file);

		FILE *fd = fopen(path, "rt");
		if (fd)	return fd;
	}

	return NULL;
}

int preprocess_file(hashmap_t*, FILE*, FILE*, char**, int, int);

int handle_directive(
	FILE *fin, FILE *fout,
	char **folders, int folders_no,
	hashmap_t *defmap,
	char *buffer, char **words, int words_no,
	int *condition
	) {

	int r = 0;

	if (*condition && strcmp(words[0], DEFINE_DIRECTIVE) == 0) {
		r = handle_define(fin, defmap, buffer, words, words_no);
		if (r)	return r;
	} else if (*condition && strcmp(words[0], UNDEF_DIRECTIVE) == 0) {
		delete_item(defmap, words[1]);
	} else if (*condition && strcmp(words[0], IF_DIRECTIVE) == 0) {
		int cond = (strcmp(words[1], "0") == 0) ? 0 : 1;
		r = preprocess_file(defmap, fin, fout, folders, folders_no, cond);
	} else if (strcmp(words[0], ELSE_DIRECTIVE) == 0) {
		*condition = *condition ? 0 : 1; 
	} else if (strcmp(words[0], ELIF_DIRECTIVE) == 0) {
		*condition = *condition ? 0 : (strcmp(words[1], "0") == 0) ? 0 : 1;
	} else if (*condition && strcmp(words[0], IFDEF_DIRECTIVE) == 0) {
		int cond = (get_mapping(defmap, words[1]) == NULL) ? 0 : 1;
		r = preprocess_file(defmap, fin, fout, folders, folders_no, cond);
	} else if (*condition && strcmp(words[0], IFNDEF_DIRECTIVE) == 0) {
		int cond = (get_mapping(defmap, words[1]) != NULL) ? 0 : 1;
		r = preprocess_file(defmap, fin, fout, folders, folders_no, cond);
	} else if (*condition && strcmp(words[0], INCLUDE_DIRECTIVE) == 0) {
		FILE *file = find_file(words[1], folders, folders_no);
		if (file == NULL) {
			return ENOENT;
		}
		r = preprocess_file(defmap, file, fout, folders, folders_no, 1);
		fprintf(fout, "\n");
		fclose(file);
	}

	return r;
}

int preprocess_file(
	hashmap_t *defmap,
	FILE *fin, FILE *fout,
	char **folders, int folders_no,
	int condition
	) {

	int r = 0;

	char buffer[MAXBUF];

	int start_of_file = 1;
	int stop = 0;

	while (!stop && fgets(buffer, MAXBUF, fin) != NULL) {
		char **words;
		int words_no;
		r = extract_words(buffer, &words, &words_no);
		if (r)	return r;

		if (replace_defines(defmap, buffer, words, words_no)) {
			/* extract the words again */
			free_string_vector(words, words_no);
			r = extract_words(buffer, &words, &words_no);
			if (r)	return r;
		}

		if (words_no == 0) {
			if (!start_of_file) /* don't print empty lines at beggining of file */
				fprintf(fout, "%s", buffer);
		} else if (words[0][0] == '#') {
			if (strcmp(words[0], ENDIF_DIRECTIVE) == 0)		stop = 1;
			r = handle_directive(fin, fout, folders, folders_no,
				defmap, buffer, words, words_no, &condition);
			if(r)	stop = 1;
		} else {
			if (condition) {
				start_of_file = 0;
				fprintf(fout, "%s", buffer);
			}
		}

		free_string_vector(words, words_no);
	}

	return r;
}

int main(int argc, char *argv[]) {
	hashmap_t *defmap = new_hashmap();
	if (!defmap)	return ENOMEM;

	FILE *fin;
	FILE *fout;
	char **folders;
	int folders_no;

	int r = init(argc, argv, defmap, &fin, &fout, &folders, &folders_no);
	printf("trece de init cu r = %d\n", r);
	if (r) {
		end_program(defmap, fin, fout, folders, folders_no);
		return r;
	}

	r = preprocess_file(defmap, fin, fout, folders, folders_no, 1);
	printf("trece de preprocess_file cu r = %d\n", r);
	if (r) {
		end_program(defmap, fin, fout, folders, folders_no);
		return r;
	}

	end_program(defmap, fin, fout, folders, folders_no);
	return 0;
}
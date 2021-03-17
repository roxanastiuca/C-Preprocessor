#include "utils.h"

void free_string_vector(char **vect, int len) {
	for (int i = 0; i < len; i++) {
		free(vect[i]);
	}
	free(vect);
}

int extract_define(char *str, char **ref_symbol, char **ref_mapping) {
	char *p;

	if ((p = strchr(str, '=')) != NULL) {
		// sym=map
		*ref_mapping = p + 1;
		*p = '\0';
		*ref_symbol = str;
	} else {
		// sym (map="")
		*ref_symbol = str;
		*ref_mapping = str + strlen(str);
	}

	return 0;
}

int extract_words(char *str, char ***ref_words, int *words_no) {
	// Copy into another buffer because strtok destroys original string.
	char buffer[MAXBUF];
	memcpy(buffer, str, MAXBUF);

	char **words = (char **) calloc(WORDSMIN, sizeof(char *));
	if (!words) return ENOMEM;

	int capacity = WORDSMIN;
	int idx = 0;

	char *token = strtok(buffer, DELIMLIST);

	while (token != NULL) {
		if (idx == capacity) {
			// need more space to stock words array
			char **words_aux = (char **) realloc(words, (capacity * 2) * sizeof(char*));
			if (words_aux == NULL) {
				// free space for words
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

int init(int argc, char *argv[], hashmap_t *defmap, char **input_file, FILE **fout) {
	*input_file = NULL;
	*fout = stdout;
	int r;

	for (int i = 1; i < argc; i++) {
		if (strncmp(argv[i], D_ARG, strlen(D_ARG)) == 0) {
			char *symbol, *mapping;
			if (strlen(argv[i]) == strlen(D_ARG)) {
				r = extract_define(argv[i + 1], &symbol, &mapping); // -D sym=map
				if (r)	return r;
				i++;
			} else {
				r = extract_define(argv[i] + 2, &symbol, &mapping); // -Dsym=map
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

		} else {
			if (*input_file == NULL) {
				// input file is the first positional argument.
				*input_file = argv[i];
			} else if (*fout == stdout) {
				// output file is the second possible positional argument.
				*fout = fopen(argv[i], "wt");
			}
		}
	}

	return 0;
}

void end_program(hashmap_t *map, FILE *fout) {
	free_hashmap(map);
	fclose(fout);
}

int replace_defines(
	hashmap_t *defmap,
	char *buffer, char **words, int words_no,
	char *to_print
	) {
	
	int offset = 0;

	for (int i = 0; i < words_no; i++) {
		char *mapping = get_mapping(defmap, words[i]);

		if (mapping != NULL) {
			// find final mapping (allows for define in define)
			char *aux_mapping = mapping;
			while (aux_mapping != NULL) {
				mapping = aux_mapping;
				aux_mapping = get_mapping(defmap, mapping);
			}

			// printf("mapping found = [%s]\n", mapping);

			char *pos = strstr(buffer, words[i]);

			if (buffer != pos) {
				// copy everything until that word
				// printf("bufer dif pos\n");
				memcpy(to_print + offset, buffer, pos - buffer);
				offset += (pos - buffer);
			}

			// printf("to_print 1 = [%s]\n", to_print);

			// copy mapping
			memcpy(to_print + offset, mapping, strlen(mapping));
			offset += strlen(mapping);

			// printf("to_print 2 = [%s]\n", to_print);

			// move buffer ahead, after word mapped
			buffer = pos + strlen(words[i]);
		}
	}

	// printf("a mai ramas din buffer: [%s]\n", buffer);

	memcpy(to_print + offset, buffer, strlen(buffer));
	offset += strlen(buffer);
	to_print[offset] = '\0';

	return 0;
}

int preprocess_file(hashmap_t *defmap, char *input_file, FILE *fout) {
	int r = 0;
	FILE *fin;

	if (input_file == NULL) {
		fin = stdin;
	} else {
		fin = fopen(input_file, "rt");
	}

	char buffer[MAXBUF];
	char to_print[MAXBUF];

	int start_of_file = 1;

	while (fgets(buffer, MAXBUF, fin) != NULL) {
		// printf("buffer = [%s]\n", buffer);

		char **words;
		int words_no;
		r = extract_words(buffer, &words, &words_no);
		if (r)	return r;

		if (words_no == 0) {
			// if (!start_of_file) // don't print empty lines at beggining of file
				fprintf(fout, "%s", buffer);
		} else if (strcmp(words[0], DEFINE_DIRECTIVE) == 0) {
			// TODO not words[2] but end of line
			// fprintf(fout, "\n");
			buffer[strlen(buffer) - 1] = '\0'; // trim trailing whitespace (\n character)
			char *mapping = buffer + (MAPPING_OFFSET + strlen(words[1]));
			r = insert_item(defmap, words[1], words_no >= 3 ? mapping : "");
		} else if (strcmp(words[0], UNDEF_DIRECTIVE) == 0) {
			delete_item(defmap, words[1]);
		} else if (strcmp(words[0], IF_DIRECTIVE) == 0) {

		} else if (strcmp(words[0], IFDEF_DIRECTIVE) == 0) {

		} else if (strcmp(words[0], IFNDEF_DIRECTIVE) == 0) {

		} else if (strcmp(words[0], INCLUDE_DIRECTIVE) == 0) {

		} else {
			start_of_file = 0;

			replace_defines(defmap, buffer, words, words_no, to_print);
			fprintf(fout, "%s", to_print);
		}

		free_string_vector(words, words_no);
	}

	fclose(fin);
	return r;
}

int main(int argc, char *argv[]) {
	hashmap_t *defmap = new_hashmap();
	if (!defmap)	return ENOMEM;

	FILE *fout;
	char *input_file;

	int r = init(argc, argv, defmap, &input_file, &fout);
	if (r) {
		end_program(defmap, fout);
		return r;
	}

	r = preprocess_file(defmap, input_file, fout);
	if (r) {
		end_program(defmap, fout);
		return r;
	}

	// print_map(defmap);

	end_program(defmap, fout);
	return 0;
}
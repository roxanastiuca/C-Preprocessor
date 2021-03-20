#include "utils.h"

int preprocess_file(hashmap_t*, FILE*, FILE*, char**, int, int);

/*
 * Description: initiates program, by parssing command line arguments and
 initializing files and the array of folders.
 * Output: 0 for no error, ENOMEM, ENOENT, EINVAL.
 */
int init(
	int argc, char *argv[],
	hashmap_t *defmap,
	FILE **fin, FILE **fout,
	char ***folders, int *folders_no)
{

	char *input_file = NULL, *symbol, *mapping, *output_file,
		**folders_aux, *dir;
	int r, i, capacity = SIZEMIN;

	*fin = stdin;
	*fout = stdout;

	*folders = (char **) calloc(SIZEMIN, sizeof(char *));
	if (!*folders)
		return ENOMEM;

	/* Reserve first spot for input file folder/current folder (this has
	 * the highest priority when searching for a file):
	 */
	*folders_no = 1;

	for (i = 1; i < argc; i++) {
		if (strncmp(argv[i], D_ARG, strlen(D_ARG)) == 0) {
			if (strlen(argv[i]) == strlen(D_ARG)) {
				/* -D symbol=mapping */
				r = extract_define(argv[i + 1], &symbol,
					&mapping);
				if (r)
					return r;
				i++;
			} else {
				/* -Dsymbol=mapping */
				r = extract_define(argv[i] + 2, &symbol,
					&mapping);
				if (r)
					return r;
			}

			insert_item(defmap, symbol, mapping);
		} else if (strncmp(argv[i], O_ARG, strlen(O_ARG)) == 0) {
			if (strlen(argv[i]) == strlen(O_ARG)) {
				output_file = argv[i + 1];
				i++;
			} else {
				output_file = argv[i] + 2;
			}

			*fout = fopen(output_file, "wt");
			if (!*fout)
				return ENOENT;
		} else if (strncmp(argv[i], I_ARG, strlen(I_ARG)) == 0) {
			if (*folders_no == capacity) {
				/* Array requires more space. */
				folders_aux = (char **) realloc(*folders,
					(capacity * 2) * sizeof(char *));
				if (folders_aux == NULL) {
					fprintf(stderr,
					"Not enough memory for realloc.\n");
					return ENOMEM;
				}
				capacity *= 2;
				*folders = folders_aux;
			}

			if (strlen(argv[i]) == strlen(I_ARG)) {
				/* -I folder */
				dir = argv[i + 1];
				i++;
			} else {
				/* -Ifolder */
				dir = argv[i] + 2;
			}

			(*folders)[*folders_no] = (char *)
				calloc(1 + strlen(dir), sizeof(char));
			if ((*folders)[*folders_no] == NULL) {
				fprintf(stderr,
					"Not enough memory for folder.\n");
				return ENOMEM;
			}

			memcpy((*folders)[(*folders_no)++], dir, strlen(dir));
		} else {
			if (*fin == stdin) {
				/* First positional argument. */
				input_file = argv[i];
				*fin = fopen(input_file, "rt");
				if (!*fin)
					return ENOENT;
			} else if (*fout == stdout) {
				/* Second possible positional argument. */
				*fout = fopen(argv[i], "wt");
				if (!*fout)
					return ENOENT;
			} else {
				/* No third positional argument accepted. */
				return EINVAL;
			}
		}
	}

	if (input_file == NULL) {
		/* First folder to be checked is current working directory. */
		(*folders)[0] = (char *) calloc(2, sizeof(char));
		if ((*folders)[0] == NULL) {
			fprintf(stderr, "Not enough memory for folder 0.\n");
			return ENOMEM;
		}
		strcpy((*folders)[0], ".");
	} else {
		/* First folder to be checked is folder where input file is. */
		char *end_of_path = strrchr(input_file, '/');

		if (end_of_path == NULL) {
			(*folders)[0] = (char *) calloc(2, sizeof(char));
			if ((*folders)[0] == NULL) {
				fprintf(stderr,
					"Not enough memory for folder 0.\n");
				return ENOMEM;
			}
			strcpy((*folders)[0], ".");
		} else {
			*end_of_path = '\0';
			(*folders)[0] = (char *) calloc(1 + strlen(input_file),
				sizeof(char));
			if ((*folders)[0] == NULL) {
				fprintf(stderr,
					"Not enough memory for folder 0.\n");
				return ENOMEM;
			}
			strcpy((*folders)[0], input_file);
		}
	}

	return 0;
}

/*
 * Description: handles a #define directive. Adds symbol and mapping to
 map.
 * Output: 0 for no error, ENOMEM.
 */
int handle_define(
	FILE *fin,
	hashmap_t *defmap,
	char *buffer,
	char **words, int words_no)
{

	char mapping[MAXBUF];
	char aux[MAXBUF];
	int offset;
	int r;

	if (words_no >= 3) {
		offset = MAPPING_OFFSET + strlen(words[1]);
		memcpy(mapping, buffer + offset, strlen(buffer + offset) + 1);
		mapping[strlen(buffer + offset) - 1] = '\0';
		trim_whitespace(mapping);

		while (buffer[strlen(buffer) - 2] == '\\') {
			/* Multi-line define. */
			fgets(buffer, MAXBUF, fin);
			memcpy(aux, buffer, MAXBUF);

			aux[strlen(aux) - 1] = '\0';
			trim_whitespace(aux);

			mapping[strlen(mapping) - 1] = '\0';
			strcat(mapping, SPACE);
			strcat(mapping, aux);
		}
	} else {
		/* Mapping was not given. It defaults to empty string. */
		mapping[0] = '\0';
	}

	r = insert_item(defmap, words[1], mapping);
	return r;
}

/*
 * Description: buffer contains a directive which has to be handled.
 Ignore certain directives if *condition is not true (it means we are in
 a section of code after a conditional directive which evaluated to false).
 * Output: 0 for no error, ENOMEM.
 */
int handle_directive(
	FILE *fin, FILE *fout,
	char **folders, int folders_no,
	hashmap_t *defmap,
	char *buffer, char **words, int words_no,
	int *condition)
{

	int r = 0;
	int cond;
	FILE *file;

	if (*condition && strcmp(words[0], DEFINE_DIRECTIVE) == 0) {
		r = handle_define(fin, defmap, buffer, words, words_no);
		if (r)
			return r;
	} else if (*condition && strcmp(words[0], UNDEF_DIRECTIVE) == 0) {
		delete_item(defmap, words[1]);
	} else if (*condition && strcmp(words[0], IF_DIRECTIVE) == 0) {
		cond = (strcmp(words[1], "0") == 0) ? 0 : 1;
		r = preprocess_file(defmap, fin, fout, folders, folders_no, cond);
	} else if (strcmp(words[0], ELSE_DIRECTIVE) == 0) {
		*condition = *condition ? 0 : 1;
	} else if (strcmp(words[0], ELIF_DIRECTIVE) == 0) {
		*condition = *condition ? 0 : (strcmp(words[1], "0") == 0) ? 0 : 1;
	} else if (*condition && strcmp(words[0], IFDEF_DIRECTIVE) == 0) {
		cond = (get_mapping(defmap, words[1]) == NULL) ? 0 : 1;
		r = preprocess_file(defmap, fin, fout, folders, folders_no, cond);
	} else if (*condition && strcmp(words[0], IFNDEF_DIRECTIVE) == 0) {
		cond = (get_mapping(defmap, words[1]) != NULL) ? 0 : 1;
		r = preprocess_file(defmap, fin, fout, folders, folders_no, cond);
	} else if (*condition && strcmp(words[0], INCLUDE_DIRECTIVE) == 0) {
		file = find_file(words[1], folders, folders_no);
		if (file == NULL)
			return ENOENT;

		r = preprocess_file(defmap, file, fout, folders, folders_no, 1);
		fprintf(fout, "\n");
		fclose(file);
	}

	return r;
}

/*
 * Description: preproccesing a .c or .h file, by resolving its directives
 and replacing defines. Conditional directives are resolved in their own
 call of this function. Variable 'condition' is 0 if we are in a section of
 code after a conditional directive which evaluated to false.
 Output: 0 for no error, ENOMEM.
 */
int preprocess_file(
	hashmap_t *defmap,
	FILE *fin, FILE *fout,
	char **folders, int folders_no,
	int condition)
{
	int r = 0;

	char buffer[MAXBUF];
	char **words;

	int words_no;

	int start_of_file = 1;
	int stop = 0; /* 1 if while has to stop */

	while (!stop && fgets(buffer, MAXBUF, fin) != NULL) {
		r = extract_words(buffer, &words, &words_no);
		if (r)
			return r;

		if (replace_defines(defmap, buffer, words, words_no)) {
			/* Extract the words again (some were replaced): */
			free_string_vector(words, words_no);
			r = extract_words(buffer, &words, &words_no);
			if (r)
				return r;
		}

		if (words_no == 0) {
			if (!start_of_file)
				fprintf(fout, "%s", buffer);
		} else if (words[0][0] == '#') {
			if (strcmp(words[0], ENDIF_DIRECTIVE) == 0)
				stop = 1;
			r = handle_directive(fin, fout, folders, folders_no,
				defmap, buffer, words, words_no, &condition);
			if (r)
				stop = 1;
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

/*
 * Description: final operations before ending the execution.
 */
void end_program(
	hashmap_t *map,
	FILE *fin, FILE *fout,
	char **folders, int folders_no)
{
	free_hashmap(map);
	if (fin != NULL)
		fclose(fin);
	if (fout != NULL)
		fclose(fout);
	free_string_vector(folders, folders_no);
}

int main(int argc, char *argv[])
{
	hashmap_t *defmap;
	FILE *fin, *fout;
	char **folders;
	int folders_no, r;

	defmap = new_hashmap();
	if (!defmap) {
		fprintf(stderr, "Not enough memory for hashmap.\n");
		return ENOMEM;
	}

	r = init(argc, argv, defmap, &fin, &fout, &folders, &folders_no);
	if (r) {
		end_program(defmap, fin, fout, folders, folders_no);
		return r;
	}

	r = preprocess_file(defmap, fin, fout, folders, folders_no, 1);
	if (r) {
		end_program(defmap, fin, fout, folders, folders_no);
		return r;
	}

	end_program(defmap, fin, fout, folders, folders_no);
	return 0;
}

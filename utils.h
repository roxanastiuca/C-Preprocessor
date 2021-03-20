/* util defines and functions for processing strings */

#ifndef UTILS_H_
#define UTILS_H_	1

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "map.h"

#define D_ARG "-D"
#define O_ARG "-o"
#define I_ARG "-I"

#define DEFINE_DIRECTIVE "#define"
#define UNDEF_DIRECTIVE "#undef"
#define IF_DIRECTIVE "#if"
#define ELSE_DIRECTIVE "#else"
#define ELIF_DIRECTIVE "#elif"
#define ENDIF_DIRECTIVE "#endif"
#define IFDEF_DIRECTIVE "#ifdef"
#define IFNDEF_DIRECTIVE "#ifndef"
#define INCLUDE_DIRECTIVE "#include"

#define DELIMLIST "\t []{}<>=+-*%!&|^,:;()\\\n\""
#define SPACE " "

#define MAXBUF 257
#define SIZEMIN 4
#define MAPPING_OFFSET 9

void free_string_vector(char **vect, int len);
void trim_whitespace(char *str);
int extract_words(char *str, char ***ref_words, int *words_no);
int between_quotations(char *buffer, char *pos);
int extract_define(char *str, char **ref_symbol, char **ref_mapping);
int replace_defines(hashmap_t *defmap, char *buffer,
	char **words, int words_no);
FILE *find_file(char *file, char **folders, int folders_no);

#endif

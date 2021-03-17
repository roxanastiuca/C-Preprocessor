#include <stdio.h>
#include <stdlib.h>
#include "map.h"

#define D_ARG "-D"
#define O_ARG "-o"
#define I_ARG "-I"

#define DEFINE_DIRECTIVE 	"#define"
#define IF_DIRECTIVE 		"#if"
#define IFDEF_DIRECTIVE 	"#ifdef"
#define IFNDEF_DIRECTIVE 	"#ifndef"
#define INCLUDE_DIRECTIVE 	"#include"

#define MAIN "main"

#define DELIMLIST "\t []{}<>=+-*/%!&|^.,:;()\\\n"

#define MAXBUF 257
#define WORDSMIN 4

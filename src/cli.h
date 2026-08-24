#ifndef CLI_H
#define CLI_H
#include <stdint.h>
#include "lib.h"

#define TNT_USERSEED (1 << 0)
#define TNT_NONEWLINE (1 << 1)
#define TNT_ZEROTERM (1 << 2)
#define TNT_ECHO (1 << 3)

typedef struct {
	uint64_t seed;
	char delim;
	char *ifile;
	char *ofile;
	tnt_token_t *echo;
	size_t echo_count;
	unsigned flags;
	char *generator;
	char *permutator;
	size_t perm_iter;
	uint64_t n;
} tnt_cli_t;

tnt_cli_t tnt_cli(int argc, char **argv, int *err);

#endif


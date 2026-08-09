#ifndef CLI_H
#define CLI_H
#include <stdint.h>

#define TNT_USERSEED (1 << 0)
#define TNT_NONEWLINE (1 << 1)
#define TNT_ZEROTERM (1 << 2)
#define TNT_ECHO (1 << 3)

typedef struct {
	uint64_t seed;
	char delim;
	char *ifile;
	char *ofile;
	const char **echo;
	int echo_count;
	unsigned flags;
	char *generator;
	uint64_t n;
} tnt_cli_t;

tnt_cli_t tnt_cli(int argc, char **argv);

#endif


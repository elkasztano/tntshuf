#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "cli.h"

void print_help_text(char *progname);

tnt_cli_t tnt_cli(int argc, char **argv) {
	int c, option_index = 0;

	tnt_cli_t cli;

	/* default values */
	cli.seed = 0;
	cli.delim = '\n';
	cli.echo = NULL;
	cli.echo_count = 0;
	cli.ifile = NULL;
	cli.ofile = NULL;
	cli.flags = 0;
	cli.generator = "xoshiro256pp";
	cli.n = UINT64_MAX;

	while( 1 ) {
		static struct option long_options[] = {
			{ "input-file", required_argument, 0, 'i' },
			{ "output-file", required_argument, 0, 'o' },
			{ "seed", required_argument, 0, 's' },
			{ "delimiter", required_argument, 0, 'd' },
			{ "generator", required_argument, 0, 'g' },
			{ "head-count", required_argument, 0, 'n' },
			{ "no-newline", no_argument, 0, 'l' },
			{ "zero-terminated", no_argument, 0, 'z' },
			{ "echo", no_argument, 0, 'e' },
			{ "help", no_argument, 0, 'h' },
			{ "version", no_argument, 0, 'V' },
			{ 0, 0, 0, 0 }
		};

		c = getopt_long(argc, argv, "hVi:o:s:d:g:n:lze", long_options, &option_index);

		if(c == -1)
			break;

		switch(c) {
			case 'i':
				cli.ifile = optarg;
				break;
			case 'o':
				cli.ofile = optarg;
				break;
			case 's':
				cli.seed = strtoull(optarg, NULL, 10);
				cli.flags |= TNT_USERSEED;
				break;
			case 'd':
				cli.delim = *optarg; /* first character of string */
				break;
			case 'g':
				cli.generator = optarg;
				break;
			case 'n':
				cli.n = strtoull(optarg, NULL, 10);
				break;
			case 'l':
				cli.flags |= TNT_NONEWLINE;
				break;
			case 'z':
				cli.flags |= TNT_ZEROTERM;
				cli.delim = '\0';
				break;
			case 'e':
				cli.flags |= TNT_ECHO;
				break;
			case 'h':
				print_help_text(*argv);
				exit(0);
			case 'V':
				printf("tntshuf 1.4\n");
				exit(0);
			case '?':
				print_help_text(*argv);
				exit(1);
			default:
				print_help_text(*argv);
				exit(1);
		}
	}

	/* handle remaining arguments for 'echo' flag */
	cli.echo_count = argc - optind;
	if (cli.echo_count > 0) {
		cli.echo = (const char **)&argv[optind];
	}

	return(cli);
}

void print_help_text(char *progname) {
	printf( "Usage: %s [options]\n"
			"-i, --input-file ........ input file path, defaults to stdin\n"
			"-o, --output-file ....... output file path, overwrites without prompt, defaults to stdout\n"
			"-e, --echo .............. treat each argument as input line\n"
			"-s, --seed .............. seed for the PRNG\n"
			"-d, --delimiter ......... delimiter, defaults to newline\n"
			"-g, --generator ......... select PRNG, values: \"splitmix64\", \"xorshift64star\",\n"
			"                          \"xoroshiro1024pp\", default: \"xoshiro256pp\"\n"
			"-n, --head-count ........ output only first N elements from shuffled list\n"
			"-l, --no-newline ........ omit new line at the end\n"
			"-z, --zero-terminated ... set delimiter to NULL ('\\0')\n"
			"-h, --help .............. show help and exit\n"
			"-V, --version ........... show version and exit\n",
			progname);
}


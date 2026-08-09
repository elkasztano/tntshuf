#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include "cli.h"
#include "lib.h"

int check_err(int err);

int main(int argc, char **argv) {
	int err = TNT_OK;
	char **tokens = NULL;
	size_t count = 0;
	size_t k = 0;

	/* get cli */
	tnt_cli_t cli = tnt_cli(argc, argv);

	err = tnt_select_prng(cli.generator);
	if (check_err(err))
		goto cleanup;

	err = tnt_prng_init(cli.seed, cli.flags);
	if (check_err(err))
		goto cleanup;

	if (cli.flags & TNT_ECHO) {
		tokens = (char **)cli.echo;
		count = cli.echo_count;
	} else {
		err = tnt_read_tokens(cli.ifile, cli.delim, &tokens, &count);
		if (check_err(err))
			goto cleanup;
	}

	if (cli.n < count)
		k = cli.n;
	else
		k = count;

	tnt_shuffle_tokens(tokens, count, k);

	err = tnt_output_tokens(cli.ofile, tokens, k, cli.delim, cli.flags);
	if (check_err(err))
		goto cleanup; 

cleanup:
	if (!(cli.flags & TNT_ECHO) && tokens != NULL)
		tnt_free_tokens(tokens, count);

	return (err != TNT_OK) ? EXIT_FAILURE : EXIT_SUCCESS;
}

int check_err(int err) {
	if (err != TNT_OK) {
		fprintf(stderr, "ERROR: \x1b[91m%s\x1b[0m\n",
				tnt_err_str(err));
		return 1;
	}
	return 0;
}


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "cli.h"
#include "lib.h"

static int check_err(int err);

int main(int argc, char **argv) {
	int err = TNT_OK;
	tnt_token_t *tokens = NULL;
	char *raw_buf = NULL;
	size_t count = 0;
	size_t k = 0;

	/* Parse command line options */
	tnt_cli_t cli = tnt_cli(argc, argv, &err);
	if (check_err(err)) {
		goto cleanup;
	}

	if (cli.generator != NULL) {

		err = tnt_select_prng(cli.generator);
		if (check_err(err)) {
			goto cleanup;
		}

		err = tnt_prng_init(cli.seed, cli.flags);
		if (check_err(err)) {
			goto cleanup;
		}

	} else if (cli.permutator != NULL) {

		err = tnt_select_perm(cli.permutator);
		if (check_err(err)) {
			goto cleanup;
		}

	}

	if (cli.flags & TNT_ECHO) {
		tokens = cli.echo;
		count = cli.echo_count;
	} else {
		err = tnt_read_tokens(cli.ifile, cli.delim, &tokens, &count, &raw_buf);
		if (check_err(err)) {
			goto cleanup;
		}
	}

	k = (cli.n < count) ? cli.n : count;

	if (cli.generator != NULL) {
		tnt_shuffle_tokens(tokens, count, k);
	} else if (cli.permutator != NULL) {
		for (size_t i = 0; i < cli.perm_iter; i++) {
			err = tnt_permutate_deterministic(tokens, count, k);
			if (check_err(err)) {
				goto cleanup;
			}
		}
	}

	err = tnt_output_tokens(cli.ofile, tokens, k, cli.delim, cli.flags);
	if (check_err(err)) {
		goto cleanup;
	}

cleanup:
	free(tokens);

	/* raw_buf is either a valid malloc pointer or NULL */
	free(raw_buf);

	return (err != TNT_OK) ? EXIT_FAILURE : EXIT_SUCCESS;
}

static int check_err(int err) {
	if (err != TNT_OK) {
		fprintf(stderr, "ERROR: \x1b[91m%s\x1b[0m\n",
			tnt_err_str(err));
		return 1;
	}
	return 0;
}

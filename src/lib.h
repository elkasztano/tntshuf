#ifndef LIB_H
#define LIB_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define TNT_OK               0
#define TNT_ERR_NOMEM        1
#define TNT_ERR_INVALID      2
#define TNT_ERR_URANDOM_OPEN 3
#define TNT_ERR_URANDOM_READ 4
#define TNT_ERR_INVALID_SEED 5
#define TNT_ERR_UNKNOWN_GEN  6
#define TNT_ERR_IFILE        7
#define TNT_ERR_OFILE        8

typedef struct {
	char *ptr;
	uint32_t len;
} tnt_token_t;

/* select PRNG */
int tnt_select_prng(char *selection);

/* PRNG initialization wrapper */
int tnt_prng_init(uint64_t seed, unsigned flags);

/* Initialize PRNG with specified seed */
void tnt_prng_init_seed(uint64_t seed);

/* Initialize PRNG seed from /dev/urandom */
int tnt_prng_init_random(uint64_t *state, size_t n);

/* Returns 0 on success, non-zero error code on failure
 * if file is NULL the function reads from stdin */
int tnt_read_tokens(const char *file_path, char delim, tnt_token_t **tokens_out, size_t *count, char **raw_buf_out);

/* Fisher-Yates shuffle of tokens array */
void tnt_shuffle_tokens(tnt_token_t *tokens, size_t count, size_t k);

/* Output tokens, stripping delimiter from last token */
int tnt_output_tokens(const char *file_path, const tnt_token_t *tokens, size_t count, char delim, unsigned flags);

/* Human readable errors */
const char *tnt_err_str(int err);

#endif


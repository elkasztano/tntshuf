#include "lib.h"
#include "cli.h"
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#define TNT_MAX_STATESIZE 16

static uint64_t x;
static int p;
static uint64_t state[TNT_MAX_STATESIZE];
static uint64_t (*next)(void);
static int (*perm_fn)(tnt_token_t*, size_t, size_t);
static int state_size = TNT_MAX_STATESIZE;

static inline uint64_t rl(const uint64_t v, int k) {
	return (v << k) | (v >> (64 - k));
}

/* Xoshiro256++ implementation
 * https://prng.di.unimi.it/xoshiro256plusplus.c
 */
uint64_t xoshiro256pp(void) {
	const uint64_t result = rl(state[0] + state[3], 23) + state[0];

	const uint64_t t = state[1] << 17;

	state[2] ^= state[0];
	state[3] ^= state[1];
	state[1] ^= state[2];
	state[0] ^= state[3];

	state[2] ^= t;

	state[3] = rl(state[3], 45);

	return result;
}

static uint64_t splitmix64(void) {
	uint64_t z = (state[0] += 0x9e3779b97f4a7c15);
	z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
	z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
	return z ^ (z >> 31);
}

void state_init() {
	for (int i = 0; i < state_size; i++) {
		state[i] = splitmix64();
	}
}

/* state[0] must not be zero */
static uint64_t xorshift64star(void) {
	state[0] ^= state[0] >> 24;
	state[0] ^= state[0] << 11;
	state[0] ^= state[0] >> 29;
	return state[0] * 0x2545F4914F6CDD1DULL;
}

/* Xoroshiro1024PlusPlus implementation
 * https://prng.di.unimi.it/xoroshiro1024plusplus.c
 */
uint64_t xoroshiro1024pp(void) {
	const int q = p;
	const uint64_t s0 = state[p = (p + 1) & 15];
	uint64_t s15 = state[q];
	const uint64_t result = rl(s0 + s15, 23) + s15;

	s15 ^= s0;
	state[q] = rl(s0, 25) ^ s15 ^ (s15 << 27);
	state[p] = rl(s15, 36);

	return result;
}

/* Extract top k elements in Milk Shuffle order into tokens[0..k-1] */
int milk_perm(tnt_token_t *tokens, size_t count, size_t k) {
	tnt_token_t *temp;
	size_t left = 0;
	size_t right;
	size_t idx = 0;

	if (!tokens || count == 0 || k == 0) {
		return TNT_OK;
	}

	if (k > count) {
		k = count;
	}

	right = count - 1;
	temp = malloc(k * sizeof(tnt_token_t));
	if (!temp) {
		return TNT_ERR_NOMEM;
	}

	while (left <= right && idx < k) {
		temp[idx++] = tokens[left++];
		if (left <= right && idx < k) {
			temp[idx++] = tokens[right--];
		}
	}

	/* Copy extracted subset back into tokens array */
	memcpy(tokens, temp, k * sizeof(tnt_token_t));

	free(temp);
	return TNT_OK;
}

/* Perform a Mongean Shuffle and extract top k elements into tokens[0..k-1] */
int monge_perm(tnt_token_t *tokens, size_t count, size_t k) {
	tnt_token_t *temp;
	size_t top;
	size_t bottom;

	if (!tokens || count == 0 || k == 0) {
		return TNT_OK;
	}

	if (k > count) {
		k = count;
	}

	temp = malloc(count * sizeof(tnt_token_t));
	if (!temp) {
		return TNT_ERR_NOMEM;
	}

	/* Starting pivot position in destination array */
	top = count / 2;
	bottom = top;

	temp[top] = tokens[0];

	/* Alternate placing elements at the top (left) or bottom (right) */
	for (size_t i = 1; i < count; i++) {
		if (i % 2 == 1) {
			top--;
			temp[top] = tokens[i];
		} else {
			bottom++;
			temp[bottom] = tokens[i];
		}
	}

	/* Copy extracted subset back into tokens array */
	memcpy(tokens, temp, k * sizeof(tnt_token_t));

	free(temp);
	return TNT_OK;
}

int tnt_select_prng(char *selection) {
	if (!strcmp(selection, "xoshiro256pp")) {
		state_size = 4;
		next = &xoshiro256pp;
	} else if (!strcmp(selection, "xoroshiro1024pp")) {
		state_size = 16;
		next = &xoroshiro1024pp;
	} else if (!strcmp(selection, "xorshift64star")) {
		state_size = 1;
		next = &xorshift64star;
	} else if (!strcmp(selection, "splitmix64")) {
		state_size = 1;
		next = &splitmix64;
	} else {
		return TNT_ERR_UNKNOWN_GEN;
	}
	return TNT_OK;
}

int tnt_select_perm(char *selection) {
	if (!strcmp(selection, "milk")) {
		perm_fn = &milk_perm;
	} else if (!strcmp(selection, "monge")) {
		perm_fn = &monge_perm;
	} else {
		return TNT_ERR_UNKNOWN_PERM;
	}
	return TNT_OK;
}

/* simple wrapper function for deterministic permutation algorithms */
int tnt_permutate_deterministic(tnt_token_t *tokens, size_t count, size_t k) {
	return perm_fn(tokens, count, k);
}

void tnt_prng_init_seed(uint64_t seed) {
	x = seed;
}

int tnt_prng_init_random(uint64_t *state, size_t n) {
	int fd = open("/dev/urandom", O_RDONLY);

	if (fd < 0) {
		return TNT_ERR_URANDOM_OPEN;
	}

	size_t bytes_to_read = n * sizeof(uint64_t);
	if (read(fd, state, bytes_to_read) != (ssize_t)bytes_to_read) {
		close(fd);
		return TNT_ERR_URANDOM_READ;
	}

	close(fd);

	/* check for invalid seed */
	uint64_t check = 0;
	for (size_t i = 0; i < n; i++)
		check |= state[i];

	if (check == 0)
		return TNT_ERR_INVALID_SEED;
	else
		return TNT_OK;
}

int tnt_prng_init_deterministic(uint64_t *state, size_t n, uint64_t seed) {
	x = seed;
	uint64_t check = 0;
	for (size_t i = 0; i < n; i++) {
		state[i] = splitmix64();
		check |= state[i];
	}

	if (check == 0)
		return TNT_ERR_INVALID_SEED;
	else
		return TNT_OK;
}

int tnt_prng_init(uint64_t seed, unsigned flags) {
	if (flags & TNT_USERSEED) {
		tnt_prng_init_deterministic(state, state_size, seed);
		return TNT_OK;
	} else {
		return tnt_prng_init_random(state, state_size);
	}
}

/* Lemire's FastRange algorithm (unbiased 64-bit rejection sampling)
 * Lemire, D. (2018). Fast Random Integer Generation in an Interval.
 * arXiv:1805.10941
 */
uint64_t fr64_unbiased(uint64_t range64) {
	if (range64 <= 1) return 0;

	uint64_t rnd64 = next();
	__uint128_t prod128 = (__uint128_t)rnd64 * range64;
	uint64_t prod64 = (uint64_t)prod128;

	if (prod64 < range64) {
		uint64_t thresh64 = -range64 % range64;
		while (prod64 < thresh64) {
			rnd64 = next();
			prod128 = (__uint128_t)rnd64 * range64;
			prod64 = (uint64_t)prod128;
		}
	}

	return (uint64_t)(prod128 >> 64);
}

/* caller is responsible for freeing raw_buf_out */
int tnt_read_tokens(const char *file_path, char delim, tnt_token_t **tokens_out, size_t *count, char **raw_buf_out) {
	FILE *file = stdin;
	int should_close = 0;

	if (file_path) {
		file = fopen(file_path, "r");
		if (!file) {
			return TNT_ERR_IFILE;
		}
		should_close = 1;
	}

	/* set 64 KiB stream input buffer */
	setvbuf(file, NULL, _IOFBF, 64 * 1024);

	size_t raw_cap = 64 * 1024;
	size_t raw_len = 0;
	char *raw_buf = malloc(raw_cap);
	if (!raw_buf) {
		if (should_close) fclose(file);
		return TNT_ERR_NOMEM;
	}

	size_t ptr_cap = 1024;
	*count = 0;
	tnt_token_t *tokens = malloc(ptr_cap * sizeof(tnt_token_t));
	if (!tokens) {
		free(raw_buf);
		if (should_close) fclose(file);
		return TNT_ERR_NOMEM;
	}

	/* read input stream in 64 KiB chunks directly into raw_buf */
	size_t bytes_read;
	while ((bytes_read = fread(raw_buf + raw_len, 1, raw_cap - raw_len, file)) > 0) {
		raw_len += bytes_read;
		if (raw_len == raw_cap) {
			raw_cap *= 2;
			char *new_raw = realloc(raw_buf, raw_cap);
			if (!new_raw) {
				free(raw_buf);
				free(tokens);
				if (should_close) fclose(file);
				return TNT_ERR_NOMEM;
			}
			raw_buf = new_raw;
		}
	}

	if (should_close) {
		fclose(file);
	}

	/* ensure room for null terminator */
	if (raw_len == raw_cap) {
		char *new_raw = realloc(raw_buf, raw_cap + 1);
		if (!new_raw) {
			free(raw_buf);
			free(tokens);
			return TNT_ERR_NOMEM;
		}
		raw_buf = new_raw;
	}
	raw_buf[raw_len] = '\0';

	/* tokenize in-place without copying strings */
	char *start = raw_buf;
	for (size_t i = 0; i < raw_len; i++) {
		if (raw_buf[i] == delim) {
			raw_buf[i] = '\0';
			if (*count >= ptr_cap) {
				ptr_cap *= 2;
				tnt_token_t *new_toks = realloc(tokens, ptr_cap * sizeof(tnt_token_t));
				if (!new_toks) {
					free(raw_buf);
					free(tokens);
					return TNT_ERR_NOMEM;
				}
				tokens = new_toks;
			}
			tokens[*count].ptr = start;
			tokens[*count].len = (uint32_t)(raw_buf + i - start);
			(*count)++;
			start = raw_buf + i + 1;
		}
	}

	/* store final token if line is not delimiter terminated */
	if (start < raw_buf + raw_len && *start != '\0') {
		if (*count >= ptr_cap) {
			ptr_cap *= 2;
			tnt_token_t *new_toks = realloc(tokens, ptr_cap * sizeof(tnt_token_t));
			if (!new_toks) {
				free(raw_buf);
				free(tokens);
				return TNT_ERR_NOMEM;
			}
			tokens = new_toks;
		}
		tokens[*count].ptr = start;
		tokens[*count].len = (uint32_t)(raw_buf + raw_len - start);
		(*count)++;
	}

	*tokens_out = tokens;
	*raw_buf_out = raw_buf;
	return TNT_OK;
}

int tnt_output_tokens(const char *file_path, const tnt_token_t *tokens, size_t count, char delim, unsigned flags) {
	FILE *file = stdout;
	int should_close = 0;

	if (count == 0) {
		return TNT_OK;
	}

	if (file_path) {
		file = fopen(file_path, "w");
		if (!file) {
			return TNT_ERR_OFILE;
		}
		should_close = 1;
	}

	setvbuf(file, NULL, _IOFBF, 64 * 1024);

	for (size_t i = 0; i < count - 1; i++) {
		if (fwrite(tokens[i].ptr, 1, tokens[i].len, file) != tokens[i].len ||
		    fputc(delim, file) == EOF) {
			if (should_close) fclose(file);
			return TNT_ERR_OFILE;
		}
	}

	if (fwrite(tokens[count - 1].ptr, 1, tokens[count - 1].len, file) != tokens[count - 1].len) {
		if (should_close) fclose(file);
		return TNT_ERR_OFILE;
	}

	char closing = '\n';
	if (flags & TNT_ZEROTERM)
		closing = '\0';
	if (!(flags & TNT_NONEWLINE)) {
		if (fputc(closing, file) == EOF) {
			if (should_close) fclose(file);
			return TNT_ERR_OFILE;
		}
	}

	if (should_close) {
		fclose(file);
	}
	return TNT_OK;
}

void tnt_shuffle_tokens(tnt_token_t *tokens, size_t count, size_t k) {
	if (k > count || k < 1) return;

	for (size_t i = 0; i < k; i++) {
		uint64_t range = count - i;
		uint64_t j = i + fr64_unbiased(range);

		tnt_token_t temp = tokens[i];
		tokens[i] = tokens[j];
		tokens[j] = temp;
	}
}

const char *tnt_err_str(int err) {
	switch(err) {
		case TNT_ERR_NOMEM:
			return "out of memory";
		case TNT_ERR_INVALID:
			return "invalid input";
		case TNT_ERR_URANDOM_OPEN:
			return "could not open /dev/urandom";
		case TNT_ERR_URANDOM_READ:
			return "could not read from /dev/urandom";
		case TNT_ERR_INVALID_SEED:
			return "invalid seed for PRNG";
		case TNT_ERR_UNKNOWN_GEN:
			return "unknown pseudo random number generator";
		case TNT_ERR_IFILE:
			return "file input error";
		case TNT_ERR_OFILE:
			return "file output error";
		case TNT_ERR_UNKNOWN_PERM:
			return "unknown deterministic permutation algorithm";
		default:
			return "undefined error";
	}
}


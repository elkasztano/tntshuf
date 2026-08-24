# tntshuf

Blow up the order of your lists. `tntshuf` is a lightweight C command-line utility partially inspired by GNU shuf, developed as a toy project on a 2014 Acer Aspire Nitro V15 running Debian 13.

`tntshuf` shuffles input tokens read from stdin and writes them to stdout, with support for arbitrary delimiters and selectable PRNGs, as well as strictly deterministic permutation algorithms. It also supports file-based I/O and shuffling command-line arguments.

## Features

- **Fisher-Yates shuffling algorithm** for random permutations
- **Selectable PRNGs**: Xoshiro256PlusPlus (recommended), Xoroshiro1024PlusPlus (for larger lists), SplitMix64 or Xorshift64Star
- **Selectable deterministic permutation algorithms**: Milk shuffle, Mongean shuffle
- **Arbitrary delimiters**: Use any character to separate tokens (defaults to newline)
- **stdin/stdout I/O**: Simple, composable pipeline behavior
- **Optional file-based I/O**: Specify input and/or output file
- **Shuffling command-line arguments**: Treat arguments as input lines
- **Lightweight**: Minimal dependencies, fast execution

## Limitations

- **Memory-bound**: All tokens must fit in memory (Fisher-Yates requires loading the entire input)
- **Limited number of sequences**: The 64-bit PRNGs can only produce at most 2^64 distinct sequences.
  For lists with more than 20 elements, Xoshiro256PlusPlus or Xoroshiro1024PlusPlus is recommended.

## Building

### Prerequisites

- **C compiler** — `gcc`, `clang`, or compatible (set via `CC` environment variable)
- **GNU Make**

### Compile

Navigate to the `tntshuf` directory.
```bash
make
```
This produces the binary at `target/tntshuf`.

### Install

The currently preferred way is to just create a symlink in a directory in your `PATH`.
```bash
ln -s $PWD/target/tntshuf ~/bin
```

### Remove build artifacts

```bash
make clean
```

## Usage

```bash
tntshuf [options]
```
### Options

| Option | Long Form           | Argument   | Description                                                                    |
| ------ | ------------------- | ---------- | ------------------------------------------------------------------------------ |
| `-i`   | `--input-file`      | `FILE`     | Input file path, defaults to stdin.                                            |
| `-o`   | `--output-file`     | `FILE`     | Output file path, defaults to stdout. _Overwrites without prompt._             |
| `-s`   | `--seed`            | `SEED`     | Seed value for the PRNG. If not provided, the seed is read from `/dev/urandom`.|
| `-d`   | `--delimiter`       | `CHAR`     | Token delimiter (any single character). Defaults to newline (`\n`).            |
| `-g`   | `--generator`       | `PRNG`     | Select the underlying PRNG algorithm (see [Generators](#generators) below).    |
| `-p`   | `--permutator`      | `PERM[:N]` | Select deterministic permutation algorithm and optional iteration count `N` (default: 1; see [Permutators](#permutators)). |
| `-n`   | `--head-count`      | `N`        | Output only the first N elements from the shuffled list.                       |
| `-l`   | `--no-newline`      | —          | Omit the trailing newline from the output.                                     |
| `-z`   | `--zero-terminated` | —          | Set delimiter to NULL (`'\0'`).                                                |
| `-e`   | `--echo`            | —          | Treat each argument as input line.                                             |
| `-h`   | `--help`            | —          | Show help and exit.                                                            |
| `-V`   | `--version`         | —          | Show version and exit.                                                         |

#### Generators

The `--generator` option accepts the following PRNG algorithms:

- **`xoshiro256pp`** (default)
- `splitmix64`
- `xorshift64star`
- `xoroshiro1024pp`

The array will be shuffled using the [Fisher-Yates algorithm](#fisher-yates-shuffle).

#### Permutators

The `--permutator` option accepts the following permutation algorithms:

- `milk` (see [Milk Shuffle](#milk-shuffle))
- `monge` (see [Mongean Shuffle](#mongean-shuffle))

These permutation algorithms are strictly deterministic and don't need a seed.

Generators and Permutators are **mutually exclusive**. Following standard POSIX command-line conventions, if both flags are provided, **the last option specified on the command line takes precedence**.

### Examples

- Shuffle lines from a file and print to stdout:
  ```bash
  tntshuf -i test.txt
  ```
- Shuffle tokens with a custom delimiter (e.g., comma):
  ```bash
  echo -n "a,b,c,d" | tntshuf -d ","
  ```
- Shuffle and write to an output file:
  ```bash
  tntshuf -i test.txt -o output.txt
  ```
- Shuffle with a specific seed and PRNG:
  ```bash
  echo -n "1,2,3,4,5" | tntshuf -d "," -s 42 -g splitmix64
  ```
- Output only the first 3 shuffled elements:
  ```bash
  echo -n "a,b,c,d,e,f" | tntshuf -d "," -n 3
  ```
- Shuffle null-terminated tokens (e.g., for `find -print0`):
  ```bash
  find . -print0 | tntshuf -z | tr '\0' '\n'
  ```
- Omit the trailing newline from the output:
  ```bash
  echo -n "a,b,c" | tntshuf -d "," -l
  ```
- Select the Xorshift64Star PRNG:
  ```bash
  cat test.txt | tntshuf --generator xorshift64star
  ```
- Treat arguments as input line and use the Xorshift64Star PRNG:
  ```bash
  tntshuf -e apple banana cherry date -g xorshift64star
  ```
- Permutate array using the milk algorithm:
  ```bash
  seq 1 10 | tntshuf -p milk
  ```
- Simulate card shuffle with four consecutive milk permutations:
  ```bash
  tntshuf -i deck.txt -p milk:4
  ```

## Algorithms

### Fisher-Yates Shuffle

The core shuffling algorithm is the **Fisher-Yates shuffle** (also known as the Knuth shuffle), a classic algorithm for generating uniformly random permutations of a finite sequence in O(n) time.

- **Reference**: [Fisher-Yates shuffle – Wikipedia](https://en.wikipedia.org/wiki/Fisher%E2%80%93Yates_shuffle)

### SplitMix64 PRNG

**SplitMix64** was designed by **Sebastiano Vigna**. It is a fast, high-quality pseudorandom number generator with excellent statistical properties and is suitable for initializing larger PRNGs. In this project, it is used to initialize the larger state of Xoshiro256PlusPlus, but it can also be used as PRNG for the Fisher Yates algorithm.

- **Reference**: [SplitMix64 – Sebastiano Vigna](http://xorshift.di.unimi.it/splitmix64.c)

### Xorshift64Star PRNG

An alternative PRNG is **Xorshift64Star**, also developed by **Sebastiano Vigna** and based on **George Marsaglia's Xorshift** family of generators. It provides a good balance of speed and statistical quality.

- **Reference**: [Xorshift – Wikipedia](https://en.wikipedia.org/wiki/Xorshift)
- **Paper**: Vigna, S. (2015). "An Experimental Exploration of Marsaglia's xorshift Generators, Scrambled." [arXiv:1402.6246](https://arxiv.org/abs/1402.6246)
- **Original Xorshift**: Marsaglia, G. (2003). "Xorshift RNGs." *Journal of Statistical Software*, 8(14), 1–6. [https://www.jstatsoft.org/v08/i14/](https://www.jstatsoft.org/v08/i14/)

### Xoshiro256PlusPlus PRNG

**Xoshiro256PlusPlus** (xorshift/rotate with 256 bits) is a successor-generation PRNG designed by **Sebastiano Vigna** and **David Blackman**. It offers superior speed and statistical quality compared to earlier generators, with a 256-bit state suitable for general-purpose and scientific applications.

- **Reference**: [Xoshiro/Xoroshiro – Sebastiano Vigna](http://xorshift.di.unimi.it/)
- **Paper**: Blackman, D. & Vigna, S. (2021). "Scrambled Linear Pseudorandom Number Generators." [arXiv:1805.01407](https://arxiv.org/abs/1805.01407)

### Xoroshiro1024PlusPlus PRNG

**Xoroshiro1024PlusPlus** (xorshift/rotate with 1024 bits) is an extended variant of the Xoroshiro family, designed by **Sebastiano Vigna** and **David Blackman**. With a 1024-bit state, it provides exceptional statistical quality and an extremely long period (2^1024 - 1), making it suitable for demanding applications requiring very high-quality randomness.

- **Reference**: [Xoshiro/Xoroshiro – Sebastiano Vigna](http://xorshift.di.unimi.it/)
- **Paper**: Blackman, D. & Vigna, S. (2021). "Scrambled Linear Pseudorandom Number Generators." [arXiv:1805.01407](https://arxiv.org/abs/1805.01407)

### Fastrange Algorithm

**Fastrange** (also called "nearly divisionless" random integer generation) is a method for uniformly mapping a 64-bit random integer to a range [0, p) using 128-bit multiplication and bit-shifting, avoiding expensive modulo operations. Developed by **Daniel Lemire**, it provides constant-time, efficient range reduction on modern processors.

- **Reference**: Lemire, D. (2018). "Fast Random Integer Generation in an Interval." [arXiv:1805.10941](https://arxiv.org/abs/1805.10941)

### Deterministic Permutators

#### Milk Shuffle
Also known as the *Fold Shuffle* or *Outside-In Interleave*.
* **Mechanism:** Alternately selects elements from the front (top) and back (bottom) of the input array.
* **Pattern:** `[1, 2, 3, 4, 5, 6]` -> `[1, 6, 2, 5, 3, 4]`
* **Properties:** Populates odd output positions from the front half of the array and even output positions from the reversed back half.

#### Mongean Shuffle
A deterministic card shuffle named after French mathematician Gaspard Monge.
* **Mechanism:** Places the initial element at a central pivot, then alternately places subsequent elements above (top) and below (bottom) the stack.
* **Pattern:** `[1, 2, 3, 4, 5, 6]` -> `[6, 4, 2, 1, 3, 5]`
* **Properties:** Gathers all even-indexed elements at the front in reverse order and all odd-indexed elements at the back in original order.

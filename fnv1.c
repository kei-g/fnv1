#include "fnv1.h"

#include <cpuid.h>  /* for __get_cpuid */
#include <errno.h>  /* for errno */
#include <fcntl.h>  /* for open */
#include <getopt.h> /* for getopt_long, struct option */
#include <stdio.h>  /* for fprintf, stderr */
#include <stdlib.h> /* for aligned_alloc, EXIT_FAILURE, EXIT_SUCCESS */
#include <string.h> /* for memset, strerror */
#include <time.h>   /* for CLOCK_REALTIME, clock_gettime */
#include <unistd.h> /* for STDIN_FILENO, read, close */

_Static_assert(sizeof(fnv1_t) == 61U, "sizeof(fnv1_t) must be 61");
_Static_assert(sizeof(uint128_u) == 16U, "sizeof(uint128_u) must be 16");
_Static_assert(sizeof(uint256_u) == 32U, "sizeof(uint256_u) must be 32");
_Static_assert(sizeof(uint512_u) == 64U, "sizeof(uint512_u) must be 64");
_Static_assert(sizeof(uint1024_u) == 128U, "sizeof(uint1024_u) must be 128");

static const char *basename(const char *argv0);
static int fnv1_exec(fnv1_t *fnv1);
static int parse_args(fnv1_t *fnv1, int argc, char *argv[]);

int main(int argc, char *argv[])
{
	fnv1_t fnv1;
	int idx = parse_args(&fnv1, argc, argv);
	if (idx < 0)
		return EXIT_FAILURE;
	fnv1.hash = aligned_alloc(64, 4ULL << fnv1.bitidx);
	memset(fnv1.hash, 0, 4ULL << fnv1.bitidx);
	fnv1.readbuf = aligned_alloc(64, fnv1.readbufsize);
	memset(fnv1.readbuf, 0, fnv1.readbufsize);
	fnv1.work = aligned_alloc(64, 512);
	memset(fnv1.work, 0, 512);
	if (idx == argc) {
		fnv1.fd = STDIN_FILENO;
		fnv1.filename = NULL;
		return fnv1_exec(&fnv1) ? EXIT_FAILURE : EXIT_SUCCESS;
	}
	for (; idx < argc; idx++) {
		fnv1.filename = argv[idx];
		fnv1.fd = open(fnv1.filename, O_ASYNC | O_RDONLY);
		if (fnv1.fd < 0) {
			fprintf(stderr, "%s: open: %s: %s\n", fnv1.progname, fnv1.filename, strerror(errno));
			return EXIT_FAILURE;
		}
		int err = fnv1_exec(&fnv1);
		if (close(fnv1.fd) < 0) {
			fprintf(stderr, "%s: close: %s: %s\n", fnv1.progname, fnv1.filename, strerror(errno));
			return EXIT_FAILURE;
		}
		if (err) {
			fprintf(stderr, "%s: read: %s: %s\n", fnv1.progname, fnv1.filename, strerror(err));
			return EXIT_FAILURE;
		}
	}
	return EXIT_SUCCESS;
}

int fnv1_errno(void)
{
	return errno;
}

static const char *basename(const char *argv0)
{
	const char *last = NULL;
	for (const char *cur = argv0; *cur; cur++)
		if (*cur == '/')
			last = cur;
	return last ? last + 1 : argv0;
}

#include "dump.h"

static int fnv1_exec(fnv1_t *fnv1)
{
	struct timespec begin, now;
	clock_gettime(CLOCK_REALTIME, &begin);
	int err = (*fnv1->function)(fnv1);
	clock_gettime(CLOCK_REALTIME, &now);
	if (err)
		fprintf(stderr, "%s: read: %s\n", fnv1->progname, strerror(errno));
	if (!err) {
		fnv1_dump_f dump[] = {
			fnv1_dump_hash32, fnv1_dump_hash64, fnv1_dump_hash128,
			fnv1_dump_hash256, fnv1_dump_hash512, fnv1_dump_hash1024
		};
		(*dump[fnv1->bitidx])(fnv1);
		if (fnv1->clock) {
			now.tv_nsec -= begin.tv_nsec;
			now.tv_sec -= begin.tv_sec;
			while (now.tv_nsec < 0) {
				now.tv_nsec += 1000000000;
				now.tv_sec--;
			}
			uintmax_t elapsed = (uintmax_t)now.tv_sec * 1000000 + now.tv_nsec / 1000;
			fprintf(stderr, "%s: %ju(usec)\n", fnv1->progname, elapsed);
		}
	}
	return err;
}

static int parse_bitlength(fnv1_t *fnv1);
static int parse_buffersize(fnv1_t *fnv1);
static int verify_avx(const fnv1_t *fnv1, int avx1, int avx2, int avx5);

#ifndef NDEBUG
static int verify_offsets(const fnv1_t *fnv1);
#endif /* NDEBUG */

#include "hash.h"

static int parse_args(fnv1_t *fnv1, int argc, char *argv[])
{
	fnv1->binary = 0;
	fnv1->bitidx = 1;
	fnv1->clock = 0;
	fnv1->progname = basename(argv[0]);
	fnv1->readbufsize = 65536;
#ifndef NDEBUG
	if (verify_offsets(fnv1) < 0)
		return -1;
#endif /* NDEBUG */
	struct option lopts[] = {
		{ "avalanche", no_argument, NULL, 'a', },
		{ "avx1", no_argument, NULL, 1, },
		{ "avx2", no_argument, NULL, 2, },
		{ "avx5", no_argument, NULL, 3, },
		{ "avx512", no_argument, NULL, 3, },
		{ "bits", required_argument, NULL, 'b', },
		{ "binary", no_argument, NULL, 4, },
		{ "buffer-size", required_argument, NULL, 5, },
		{ "clock", no_argument, NULL, 6, },
		{ "no-avalanche", no_argument, NULL, 'n', },
		{ NULL, no_argument, NULL, 0, },
	};
	int avalanche = 0, avx1 = 0, avx2 = 0, avx5 = 0, idx, no_avalanche = 0;
	for (;;) {
		int c = getopt_long(argc, argv, "ab:n", lopts, &idx);
		if (c < 0)
			break;
		else if (c == 1)
			avx1 = 1;
		else if (c == 2)
			avx2 = 1;
		else if (c == 3)
			avx5 = 1;
		else if (c == 4)
			fnv1->binary = 1;
		else if (c == 5) {
			if (parse_buffersize(fnv1) < 0)
				return -1;
		} else if (c == 6)
			fnv1->clock = 1;
		else if (c == 'a')
			avalanche = 1;
		else if (c == 'b') {
			if (parse_bitlength(fnv1) < 0)
				return -1;
		} else if (c == 'n')
			no_avalanche = 1;
		else
			return -1;
	}
	if (avalanche && no_avalanche) {
		fprintf(stderr, "%s: both avalanche and no-avalanche are specified simulteneously\n", fnv1->progname);
		return -1;
	}
	if (verify_avx(fnv1, avx1, avx2, avx5) < 0)
		return -1;
	fnv1->avalanche = avalanche || !no_avalanche;
	fnv1_f functions[] = {
		fnv1_32, fnv1_64, fnv1_128,
		fnv1_256, fnv1_512, fnv1_1024
	};
	if (fnv1->avalanche) {
		if (avx1)
			functions[5] = fnv1a_1024_avx1;
		else if (avx2)
			functions[5] = fnv1a_1024_avx2;
		else if (avx5)
			functions[5] = fnv1a_1024_avx512;
	}
	fnv1->function = functions[fnv1->bitidx];
	return optind;
}

static int parse_bitlength(fnv1_t *fnv1)
{
	char *ep;
	unsigned long bits = strtoul(optarg, &ep, 10);
	if (errno == ERANGE) {
		fprintf(stderr, "%s: %s\n", fnv1->progname, strerror(ERANGE));
		return -1;
	}
	if (*ep) {
		fprintf(stderr, "%s: invalid option '%s'\n", fnv1->progname, optarg);
		return -1;
	}
	for (fnv1->bitidx = 0; fnv1->bitidx < 6; fnv1->bitidx++)
		if (bits == 32UL << fnv1->bitidx)
			break;
	if (5 < fnv1->bitidx) {
		fprintf(stderr, "%s: bit length must be 32, 64, 128, 256, 512, or 1024\n", fnv1->progname);
		return -1;
	}
	return 0;
}

static int parse_buffersize(fnv1_t *fnv1)
{
	char *ep;
	unsigned long bufsiz = strtoul(optarg, &ep, 10);
	if (errno == ERANGE) {
		fprintf(stderr, "%s: %s\n", fnv1->progname, strerror(ERANGE));
		return -1;
	}
	switch (*ep) {
	case '\0':
		fnv1->readbufsize = (bufsiz + 127) & ~127;
		if (bufsiz & 127)
			fprintf(stderr, "%s: buffer size is rounded from %lu to %zu\n", fnv1->progname, bufsiz, fnv1->readbufsize);
		break;
	case 'k':
	case 'K':
		fnv1->readbufsize = (size_t)bufsiz * 1024;
		break;
	case 'm':
	case 'M':
		fnv1->readbufsize = (size_t)bufsiz * 1024 * 1024;
		break;
	default:
		fprintf(stderr, "%s: invalid option '%s'\n", fnv1->progname, optarg);
		return -1;
	}
	return 0;
}

static int verify_avx(const fnv1_t *fnv1, int avx1, int avx2, int avx5)
{
	if ((avx1 && avx2) || (avx2 && avx5) || (avx5 && avx1)) {
		fprintf(stderr, "%s: multiple avx flags are specified simulteneously\n", fnv1->progname);
		return -1;
	}
	uint32_t feature1, feature7, a, b, c, d;
	if (!__get_cpuid(1, &a, &b, &feature1, &d)) {
		fprintf(stderr, "%s: cpuid(1) failed\n", fnv1->progname);
		return -1;
	}
	if (!__get_cpuid(7, &a, &feature7, &c, &d)) {
		fprintf(stderr, "%s: cpuid(7) failed\n", fnv1->progname);
		return -1;
	}
	if (avx1 && !(feature1 & bit_AVX)) {
		fprintf(stderr, "%s: avx1 is not supported on this machine, ecx=0x%08x\n", fnv1->progname, feature1);
		return -1;
	}
	if (avx2 && !(feature7 & bit_AVX2)) {
		fprintf(stderr, "%s: avx2 is not supported on this machine, ebx=0x%08x\n", fnv1->progname, feature7);
		return -1;
	}
	if (avx5 && !(feature7 & bit_AVX512F)) {
		fprintf(stderr, "%s: avx512 is not supported on this machine, ebx=0x%08x\n", fnv1->progname, feature7);
		return -1;
	}
	return 0;
}

#ifndef NDEBUG
static int verify_offsets(const fnv1_t *fnv1)
{
	if ((uintptr_t)&((fnv1_t *)0)->fd != 1) {
		fprintf(stderr, "%s: offsetof(fnv1_t, fd) must be 1\n", fnv1->progname);
		return -1;
	}
	if ((uintptr_t)&((fnv1_t *)0)->filename != 5) {
		fprintf(stderr, "%s: offsetof(fnv1_t, filename) must be 5\n", fnv1->progname);
		return -1;
	}
	if ((uintptr_t)&((fnv1_t *)0)->function != 13) {
		fprintf(stderr, "%s: offsetof(fnv1_t, function) must be 13\n", fnv1->progname);
		return -1;
	}
	if ((uintptr_t)&((fnv1_t *)0)->hash != 21) {
		fprintf(stderr, "%s: offsetof(fnv1_t, hash) must be 21\n", fnv1->progname);
		return -1;
	}
	if ((uintptr_t)&((fnv1_t *)0)->progname != 29) {
		fprintf(stderr, "%s: offsetof(fnv1_t, progname) must be 29\n", fnv1->progname);
		return -1;
	}
	if ((uintptr_t)&((fnv1_t *)0)->readbuf != 37) {
		fprintf(stderr, "%s: offsetof(fnv1_t, readbuf) must be 37\n", fnv1->progname);
		return -1;
	}
	if ((uintptr_t)&((fnv1_t *)0)->readbufsize != 45) {
		fprintf(stderr, "%s: offsetof(fnv1_t, readbufsize) must be 45\n", fnv1->progname);
		return -1;
	}
	if ((uintptr_t)&((fnv1_t *)0)->work != 53) {
		fprintf(stderr, "%s: offsetof(fnv1_t, work) must be 53\n", fnv1->progname);
		return -1;
	}
	return 0;
}
#endif /* NDEBUG */

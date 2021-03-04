#include "dump.h"

#include <stdio.h>  /* for printf */
#include <unistd.h> /* for STDOUT_FILENO, write */

void fnv1_dump_hash1024(const fnv1_t *fnv1)
{
	const uint1024_u *hash = fnv1->hash;
	if (fnv1->binary) {
		write(STDOUT_FILENO, hash, sizeof(*hash));
		return;
	}
	for (int i = 0; i < 16; i++) {
		printf("%016zx", hash->vector_qword[15 - i]);
	}
	printf("\n");
}

void fnv1_dump_hash128(const fnv1_t *fnv1)
{
	const uint128_u *hash = fnv1->hash;
	if (fnv1->binary) {
		write(STDOUT_FILENO, hash, sizeof(*hash));
		return;
	}
	for (int i = 0; i < 2; i++) {
		printf("%016zx", hash->vector_qword[1 - i]);
	}
	printf("\n");
}

void fnv1_dump_hash256(const fnv1_t *fnv1)
{
	const uint256_u *hash = fnv1->hash;
	if (fnv1->binary) {
		write(STDOUT_FILENO, hash, sizeof(*hash));
		return;
	}
	for (int i = 0; i < 4; i++) {
		printf("%016zx", hash->vector_qword[3 - i]);
	}
	printf("\n");
}

void fnv1_dump_hash32(const fnv1_t *fnv1)
{
	const uint32_t *hash = fnv1->hash;
	if (fnv1->binary) {
		write(STDOUT_FILENO, hash, sizeof(*hash));
		return;
	}
	printf("%08x\n", *hash);
}

void fnv1_dump_hash512(const fnv1_t *fnv1)
{
	const uint512_u *hash = fnv1->hash;
	if (fnv1->binary) {
		write(STDOUT_FILENO, hash, sizeof(*hash));
		return;
	}
	for (int i = 0; i < 8; i++) {
		printf("%016zx", hash->vector_qword[7 - i]);
	}
	printf("\n");
}

void fnv1_dump_hash64(const fnv1_t *fnv1)
{
	const uint64_t *hash = fnv1->hash;
	if (fnv1->binary) {
		write(STDOUT_FILENO, hash, sizeof(*hash));
		return;
	}
	printf("%016zx\n", *hash);
}

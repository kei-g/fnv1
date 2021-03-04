#include "fnv1.h"
#include "hash.h"

#include <errno.h>  /* for errno */
#include <string.h> /* for memset */
#include <unistd.h> /* for read */

#define FNV1_PRIME1024_HB   680
#define FNV1_PRIME1024_LSW  0x18d
#define FNV1_PRIME128_HB    88
#define FNV1_PRIME128_LSW   0x13b
#define FNV1_PRIME256_HB    168
#define FNV1_PRIME256_LSW   0x163
#define FNV1_PRIME32_HB     24
#define FNV1_PRIME32_LSW    0x193
#define FNV1_PRIME512_HB    344
#define FNV1_PRIME512_LSW   0x157
#define FNV1_PRIME64_HB     40
#define FNV1_PRIME64_LSW    0x1b3

// FNV1_PRIME1024_HB  = 680
// FNV1_PRIME1024_LSW = 0x18d
static void fnv1_umulp1024(uint1024_u *hash, void *work)
{
	uint64_t *const a = work;
	a[0] = hash->vector_qword[0] << 40;
	for (int i = 1; i < 6; i++)
		a[i] = hash->vector_qword[i - 1] >> 24 | hash->vector_qword[i] << 40;
	uint128_u *const b = (uint128_u *)work + 4;
	for (int i = 0; i < 15; i++)
		b[i].packed = (__uint128_t)hash->vector_qword[i] * FNV1_PRIME1024_LSW;
	uint64_t b15 = hash->hi.hi.hi.hi * FNV1_PRIME1024_LSW;
	hash->vector_qword[0] = b[0].lo;
	uint128_u c = { .lo = 0 };
	for (int i = 0; i < 9; i++) {
		c.packed = (__uint128_t)b[i].hi + b[i + 1].lo + c.hi;
		hash->vector_qword[i + 1] = c.lo;
	}
	for (int i = 9; i < 14; i++) {
		c.packed = (__uint128_t)a[i - 9] + b[i].hi + b[i + 1].lo + c.hi;
		hash->vector_qword[i + 1] = c.lo;
	}
	hash->vector_qword[15] = a[5] + b[14].hi + b15 + c.hi;
}

// FNV1_PRIME128_HB  = 88
// FNV1_PRIME128_LSW = 0x13b
static void fnv1_umulp128(uint128_u *hash)
{
	uint64_t a1 = hash->lo << (FNV1_PRIME128_HB - 64);
	uint128_u b0 = { .packed = (__uint128_t)hash->lo * FNV1_PRIME128_LSW };
	uint64_t b1 = hash->hi * FNV1_PRIME128_LSW;
	hash->lo = b0.lo;
	hash->hi = a1 + b0.hi + b1;
}

// FNV1_PRIME256_HB  = 168
// FNV1_PRIME256_LSW = 0x163
static void fnv1_umulp256(uint256_u *hash, void *work)
{
	uint64_t a2 = hash->vector_qword[0] << (FNV1_PRIME256_HB - 128);
	uint64_t a3 = hash->vector_qword[0] >> (128 + 64 - FNV1_PRIME256_HB) | hash->vector_qword[1] << (FNV1_PRIME256_HB - 128);
	uint128_u b[3];
	for (int i = 0; i < 3; i++)
		b[i].packed = (__uint128_t)hash->vector_qword[i] * FNV1_PRIME256_LSW;
	uint64_t b3 = hash->vector_qword[3] * FNV1_PRIME256_LSW;
	hash->vector_qword[0] = b[0].lo;
	uint128_u c = { .lo = 0 };
	c.packed = (__uint128_t)b[0].hi + b[1].lo + c.hi;
	hash->vector_qword[1] = c.lo;
	c.packed = (__uint128_t)a2 + b[1].hi + b[2].lo + c.hi;
	hash->vector_qword[2] = c.lo;
	hash->vector_qword[3] = a3 + b3 + c.hi;
}

// FNV1_PRIME32_HS  = 24
// FNV1_PRIME32_LSW = 0x193
static void fnv1_umulp32(uint32_t *hash)
{
	*hash *= 1U << FNV1_PRIME32_HB | FNV1_PRIME32_LSW;
}

// FNV1_PRIME512_HB  = 344
// FNV1_PRIME512_LSW = 0x157
static void fnv1_umulp512(uint512_u *hash, void *work)
{
	uint64_t *const a = work;
	a[0] = hash->vector_qword[0] << 24;
	for (int i = 0; i < 2; i++)
		a[i + 1] = hash->vector_qword[i] >> 40 | hash->vector_qword[i + 1] << 24;
	uint128_u *const b = (uint128_u *)work + 2;
	for (int i = 0; i < 7; i++)
		b[i].packed = (__uint128_t)hash->vector_qword[i] * FNV1_PRIME512_LSW;
	uint64_t b7 = hash->hi.hi.hi * FNV1_PRIME512_LSW;
	hash->vector_qword[0] = b[0].lo;
	uint128_u c = { .lo = 0 };
	for (int i = 0; i < 4; i++) {
		c.packed = (__uint128_t)b[i].hi + b[i + 1].lo + c.hi;
		hash->vector_qword[i + 1] = c.lo;
	}
	for (int i = 4; i < 6; i++) {
		c.packed = (__uint128_t)a[i - 4] + b[i].hi + b[i + 1].lo + c.hi;
		hash->vector_qword[i + 1] = c.lo;
	}
	hash->vector_qword[7] = a[2] + b7 + b[6].hi + c.hi;
}

// FNV1_PRIME64_HB = 40
// FNV1_PRIME64_LSW = 0x1b3
static void fnv1_umulp64(uint64_t *hash)
{
	*hash *= 1UL << FNV1_PRIME64_HB | FNV1_PRIME64_LSW;
}

int fnv1_1024(const fnv1_t *fnv1)
{
	uint1024_u *hash = fnv1->hash;
	hash->packed = FNV1_BASIS1024.packed;
	for (;;) {
		ssize_t rlen = read(fnv1->fd, fnv1->readbuf, fnv1->readbufsize);
		if (rlen < 0)
			return errno;
		if (rlen == 0)
			break;
		if (rlen < fnv1->readbufsize)
			memset((char *)fnv1->readbuf + rlen, 0, sizeof(*hash) - (rlen % sizeof(*hash)));
		const uint1024_u *value = fnv1->readbuf;
		if (fnv1->avalanche)
			for (ssize_t i = 0; i < (rlen + sizeof(*hash) - 1) / sizeof(*hash); i++, value++) {
				hash->packed ^= value->packed;
				fnv1_umulp1024(hash, fnv1->work);
			}
		else
			for (ssize_t i = 0; i < (rlen + sizeof(*hash) - 1) / sizeof(*hash); i++, value++) {
				fnv1_umulp1024(hash, fnv1->work);
				hash->packed ^= value->packed;
			}
		if (rlen < fnv1->readbufsize)
			break;
	}
	return 0;
}

int fnv1_128(const fnv1_t *fnv1)
{
	uint128_u *hash = fnv1->hash;
	uint128_u *value = fnv1->readbuf;
	hash->packed = FNV1_BASIS128.packed;
	for (;;) {
		ssize_t rlen = read(fnv1->fd, value, fnv1->readbufsize);
		if (rlen < 0)
			return errno;
		if (rlen == 0)
			break;
		if (rlen < fnv1->readbufsize)
			memset((char *)value + rlen, 0, sizeof(*value) - (rlen % sizeof(*value)));
		if (fnv1->avalanche)
			for (ssize_t i = 0; i < (rlen + sizeof(*value) - 1) / sizeof(*value); i++) {
				hash->packed ^= value[i].packed;
				fnv1_umulp128(hash);
			}
		else
			for (ssize_t i = 0; i < (rlen + sizeof(*value) - 1) / sizeof(*value); i++) {
				fnv1_umulp128(hash);
				hash->packed ^= value[i].packed;
			}
		if (rlen < fnv1->readbufsize)
			break;
	}
	return 0;
}

int fnv1_256(const fnv1_t *fnv1)
{
	uint256_u *hash = fnv1->hash;
	hash->packed = FNV1_BASIS256.packed;
	for (;;) {
		ssize_t rlen = read(fnv1->fd, fnv1->readbuf, fnv1->readbufsize);
		if (rlen < 0)
			return errno;
		if (rlen == 0)
			break;
		if (rlen < fnv1->readbufsize)
			memset((char *)fnv1->readbuf + rlen, 0, sizeof(*hash) - (rlen % sizeof(*hash)));
		const uint256_u *value = fnv1->readbuf;
		if (fnv1->avalanche)
			for (ssize_t i = 0; i < (rlen + sizeof(*hash) - 1) / sizeof(*hash); i++, value++) {
				hash->packed ^= value->packed;
				fnv1_umulp256(hash, fnv1->work);
			}
		else
			for (ssize_t i = 0; i < (rlen + sizeof(*hash) - 1) / sizeof(*hash); i++, value++) {
				fnv1_umulp256(hash, fnv1->work);
				hash->packed ^= value->packed;
			}
		if (rlen < fnv1->readbufsize)
			break;
	}
	return 0;
}

int fnv1_32(const fnv1_t *fnv1)
{
	uint32_t *hash = fnv1->hash;
	uint32_t *value = fnv1->readbuf;
	*hash = FNV1_BASIS32;
	for (;;) {
		ssize_t rlen = read(fnv1->fd, value, fnv1->readbufsize);
		if (rlen < 0)
			return errno;
		if (rlen == 0)
			break;
		if (rlen < fnv1->readbufsize)
			memset((char *)value + rlen, 0, sizeof(*value) - (rlen % sizeof(*value)));
		if (fnv1->avalanche)
			for (ssize_t i = 0; i < (rlen + sizeof(*value) - 1) / sizeof(*value); i++) {
				*hash ^= value[i];
				fnv1_umulp32(hash);
			}
		else
			for (ssize_t i = 0; i < (rlen + sizeof(*value) - 1) / sizeof(*value); i++) {
				fnv1_umulp32(hash);
				*hash ^= value[i];
			}
		if (rlen < fnv1->readbufsize)
			break;
	}
	return 0;
}

int fnv1_512(const fnv1_t *fnv1)
{
	uint512_u *const hash = fnv1->hash;
	hash->packed = FNV1_BASIS512.packed;
	for (;;) {
		ssize_t rlen = read(fnv1->fd, fnv1->readbuf, fnv1->readbufsize);
		if (rlen < 0)
			return errno;
		if (rlen == 0)
			break;
		if (rlen < fnv1->readbufsize)
			memset((char *)fnv1->readbuf + rlen, 0, sizeof(*hash) - (rlen % sizeof(*hash)));
		const uint512_u *value = fnv1->readbuf;
		if (fnv1->avalanche)
			for (ssize_t i = 0; i < (rlen + sizeof(*hash) - 1) / sizeof(*hash); i++, value++) {
				hash->packed ^= value->packed;
				fnv1_umulp512(hash, fnv1->work);
			}
		else
			for (ssize_t i = 0; i < (rlen + sizeof(*hash) - 1) / sizeof(*hash); i++, value++) {
				fnv1_umulp512(hash, fnv1->work);
				hash->packed ^= value->packed;
			}
		if (rlen < fnv1->readbufsize)
			break;
	}
	return 0;
}

int fnv1_64(const fnv1_t *fnv1)
{
	uint64_t *hash = fnv1->hash;
	uint64_t *value = fnv1->readbuf;
	*hash = FNV1_BASIS64;
	for (;;) {
		ssize_t rlen = read(fnv1->fd, value, fnv1->readbufsize);
		if (rlen < 0)
			return errno;
		if (rlen == 0)
			break;
		if (rlen < fnv1->readbufsize)
			memset((char *)value + rlen, 0, sizeof(*value) - (rlen % sizeof(*value)));
		if (fnv1->avalanche)
			for (ssize_t i = 0; i < (rlen + sizeof(*value) - 1) / sizeof(*value); i++) {
				*hash ^= value[i];
				fnv1_umulp64(hash);
			}
		else
			for (ssize_t i = 0; i < (rlen + sizeof(*value) - 1) / sizeof(*value); i++) {
				fnv1_umulp64(hash);
				*hash ^= value[i];
			}
		if (rlen < fnv1->readbufsize)
			break;
	}
	return 0;
}

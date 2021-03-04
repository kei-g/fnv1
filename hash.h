#ifndef __include_fnv1_hash_h__
#define __include_fnv1_hash_h__

#include "typedef.h"

int fnv1_1024(const fnv1_t *fnv1);
int fnv1_128(const fnv1_t *fnv1);
int fnv1_256(const fnv1_t *fnv1);
int fnv1_32(const fnv1_t *fnv1);
int fnv1_512(const fnv1_t *fnv1);
int fnv1_64(const fnv1_t *fnv1);

extern int fnv1a_1024_avx1(const fnv1_t *fnv1);
extern int fnv1a_1024_avx2(const fnv1_t *fnv1);
extern int fnv1a_1024_avx512(const fnv1_t *fnv1);

#endif /* __include_fnv1_hash_h__ */

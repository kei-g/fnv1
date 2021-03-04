#ifndef __include_fnv1_typedef_h__
#define __include_fnv1_typedef_h__

#define PACKED __attribute__((__packed__))

typedef struct _fnv1 fnv1_t;

typedef void (*fnv1_dump_f)(const fnv1_t *fnv1);
typedef int (*fnv1_f)(const fnv1_t *fnv1);

#include <stdint.h>    /* for uint32_t, uint64_t, uint8_t */
#include <sys/types.h> /* for size_t */

struct PACKED _fnv1 {
	uint8_t avalanche : 1;
	uint8_t binary : 1;
	uint8_t bitidx : 3;
	uint8_t clock : 1;
	int fd;
	const char *filename;
	fnv1_f function;
	void *hash;
	const char *progname;
	void *readbuf;
	size_t readbufsize;
	void *work;
};

typedef union PACKED {
	struct PACKED {
		uint64_t lo;
		uint64_t hi;
	};
	__uint128_t packed;
	uint32_t vector_dword[4];
	uint64_t vector_qword[2];
} uint128_u;

typedef union PACKED {
	struct PACKED {
		uint128_u lo;
		uint128_u hi;
	};
	unsigned long long __attribute__((__vector_size__(32), __aligned__(32))) packed;
	uint32_t vector_dword[8];
	uint64_t vector_qword[4];
} uint256_u;

typedef union PACKED {
	struct PACKED {
		uint256_u lo;
		uint256_u hi;
	};
	unsigned long long __attribute__((__vector_size__(64), __aligned__(64))) packed;
	uint32_t vector_dword[16];
	uint64_t vector_qword[8];
} uint512_u;

typedef union PACKED {
	struct PACKED {
		uint512_u lo;
		uint512_u hi;
	};
	unsigned long long __attribute__((__vector_size__(128), __aligned__(128))) packed;
	uint32_t vector_dword[32];
	uint64_t vector_qword[16];
} uint1024_u;

#endif /* __include_fnv1_typedef_h__ */

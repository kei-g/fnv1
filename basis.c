#include "fnv1.h"

const uint32_t FNV1_BASIS32 = 0x811c9dc5;

const uint64_t FNV1_BASIS64 = 0xcbf29ce484222325;

const uint128_u FNV1_BASIS128 = {
	.hi = 0x6c62272e07bb0142, .lo = 0x62b821756295c58d,
};

const uint256_u FNV1_BASIS256 = {
	.hi = { .hi = 0xdd268dbcaac55036, .lo = 0x2d98c384c4e576cc, },
	.lo = { .hi = 0xc8b1536847b6bbb3, .lo = 0x1023b4c8caee0535, },
};

const uint512_u FNV1_BASIS512 = {
	.hi = {
		.hi = { .hi = 0xb86db0b1171f4416, .lo = 0xdca1e50f309990ac, },
		.lo = { .hi = 0xac87d059c9000000, .lo = 0x0000000000000d21, },
	},
	.lo = {
		.hi = { .hi = 0xe948f68a34c192f6, .lo = 0x2ea79bc942dbe7ce, },
		.lo = { .hi = 0x182036415f56e34b, .lo = 0xac982aac4afe9fd9, },
	},
};

const uint1024_u FNV1_BASIS1024 = {
	.hi = {
		.hi = {
			.hi = { .hi = 0, .lo = 0x005f7a76758ecc4d, },
			.lo = { .hi = 0x32e56d5a591028b7, .lo = 0x4b29fc4223fdada1, },
		},
		.lo = {
			.hi = { .hi = 0x6c3bf34eda3674da, .lo = 0x9a21d90000000000, },
			.lo = { .hi = 0, .lo = 0, },
		},
	},
	.lo = {
		.hi = {
			.hi = { .hi = 0, .lo = 0, },
			.lo = { .hi = 0, .lo = 0x000000000004c6d7, },
		},
		.lo = {
			.hi = { .hi = 0xeb6e73802734510a, .lo = 0x555f256cc005ae55, },
			.lo = { .hi = 0x6bde8cc9c6a93b21, .lo = 0xaff4b16c71ee90b3, },
		},
	},
};

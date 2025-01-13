/*
 * The MIT License (MIT)
 *
 * Copyright (c) Adel Mamin
 *
 * Source: https://github.com/adel-mamin/amast
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * @file
 * COBS/ZPE API unit tests.
 */

#include <string.h>

#include "common/macros.h"
#include "cobszpe/cobszpe.h"

static void test_cobszpe_encode_run(
    const unsigned char *src,
    int src_size,
    const unsigned char *val,
    int val_size
) {
    unsigned char dst[AM_COBSZPE_ENCODED_SIZE_FOR(src_size)];
    int ret = am_cobszpe_encode(dst, (int)sizeof(dst), src, src_size);

    AM_ASSERT(ret == val_size);
    AM_ASSERT(0 == memcmp(val, dst, (size_t)val_size));
}

static void test_cobszpe_encode(void) {
    {
        /* clang-format off */
        static const unsigned char src[] = {
            0x45, 0x00, 0x00,
            0x2c, 0x4c, 0x79, 0x00, 0x00,
            0x40, 0x06, 0x4f, 0x37
        };
        static const unsigned char val[] = {
            0xE2, 0x45,
            0xE4, 0x2c, 0x4c, 0x79,
            0x05, 0x40, 0x06, 0x4f, 0x37,
            0x00
        };
        /* clang-format on */
        test_cobszpe_encode_run(src, AM_COUNTOF(src), val, AM_COUNTOF(val));
    }
    {
        static const unsigned char src[] = {0x00};
        static const unsigned char val[] = {0x01, 0x00};
        test_cobszpe_encode_run(src, AM_COUNTOF(src), val, AM_COUNTOF(val));
    }
    {
        static const unsigned char src[] = {0x00, 0x00};
        static const unsigned char val[] = {0xE1, 0x00};
        test_cobszpe_encode_run(src, AM_COUNTOF(src), val, AM_COUNTOF(val));
    }
    {
        static const unsigned char src[] = {0x00, 0x00, 0x00};
        static const unsigned char val[] = {0xE1, 0x01, 0x00};
        test_cobszpe_encode_run(src, AM_COUNTOF(src), val, AM_COUNTOF(val));
    }
    {
        static const unsigned char src[] = {0x00, 0x00, 0x00, 0x00};
        static const unsigned char val[] = {0xE1, 0xE1, 0x00};
        test_cobszpe_encode_run(src, AM_COUNTOF(src), val, AM_COUNTOF(val));
    }

#define COBSZPE_ZEROLESS_10 5, 4, 3, 2, 1, 3, 5, 7, 8, 9,
#define COBSZPE_ZEROLESS_20 COBSZPE_ZEROLESS_10 COBSZPE_ZEROLESS_10
#define COBSZPE_ZEROLESS_40 COBSZPE_ZEROLESS_20 COBSZPE_ZEROLESS_20
#define COBSZPE_ZEROLESS_80 COBSZPE_ZEROLESS_40 COBSZPE_ZEROLESS_40
#define COBSZPE_ZEROLESS_100 COBSZPE_ZEROLESS_80 COBSZPE_ZEROLESS_20
#define COBSZPE_ZEROLESS_200 COBSZPE_ZEROLESS_100 COBSZPE_ZEROLESS_100
#define COBSZPE_ZEROLESS_223 COBSZPE_ZEROLESS_200 COBSZPE_ZEROLESS_20 6, 1, 4

    {
        unsigned char src[AM_COBSZPE_ZEROLESS_MAX] = {COBSZPE_ZEROLESS_223};
        unsigned char val[1 + AM_COBSZPE_ZEROLESS_MAX + 1] = {
            AM_COBSZPE_ZEROLESS_MAX + 1, COBSZPE_ZEROLESS_223, 0
        };
        test_cobszpe_encode_run(src, AM_COUNTOF(src), val, AM_COUNTOF(val));
    }
    {
        unsigned char src[1 + AM_COBSZPE_ZEROLESS_MAX] = {
            0, COBSZPE_ZEROLESS_223
        };
        unsigned char val[1 + 1 + AM_COBSZPE_ZEROLESS_MAX + 1] = {
            0x1, AM_COBSZPE_ZEROLESS_MAX + 1, COBSZPE_ZEROLESS_223, 0
        };
        test_cobszpe_encode_run(src, AM_COUNTOF(src), val, AM_COUNTOF(val));
    }
    {
        unsigned char src[2 + AM_COBSZPE_ZEROLESS_MAX] = {
            0, 0, COBSZPE_ZEROLESS_223
        };
        unsigned char val[1 + 1 + AM_COBSZPE_ZEROLESS_MAX + 1] = {
            0xE1, AM_COBSZPE_ZEROLESS_MAX + 1, COBSZPE_ZEROLESS_223, 0
        };
        test_cobszpe_encode_run(src, AM_COUNTOF(src), val, AM_COUNTOF(val));
    }
    {
        unsigned char src[3 + AM_COBSZPE_ZEROLESS_MAX] = {
            0, 0, 0, COBSZPE_ZEROLESS_223
        };
        unsigned char val[1 + 1 + 1 + AM_COBSZPE_ZEROLESS_MAX + 1] = {
            0xE1, 0x01, AM_COBSZPE_ZEROLESS_MAX + 1, COBSZPE_ZEROLESS_223, 0
        };
        test_cobszpe_encode_run(src, AM_COUNTOF(src), val, AM_COUNTOF(val));
    }
    {
        unsigned char src[4 + AM_COBSZPE_ZEROLESS_MAX] = {
            0, 0, 0, 0, COBSZPE_ZEROLESS_223
        };
        unsigned char val[1 + 1 + 1 + AM_COBSZPE_ZEROLESS_MAX + 1] = {
            0xE1, 0xE1, AM_COBSZPE_ZEROLESS_MAX + 1, COBSZPE_ZEROLESS_223, 0
        };
        test_cobszpe_encode_run(src, AM_COUNTOF(src), val, AM_COUNTOF(val));
    }
    {
        unsigned char src[AM_COBSZPE_ZEROLESS_MAX + 1] = {
            COBSZPE_ZEROLESS_223, 0
        };
        unsigned char val[1 + AM_COBSZPE_ZEROLESS_MAX + 1 + 1] = {
            AM_COBSZPE_ZEROLESS_MAX + 1, COBSZPE_ZEROLESS_223, 0x1, 0
        };
        test_cobszpe_encode_run(src, AM_COUNTOF(src), val, AM_COUNTOF(val));
    }
    {
        unsigned char src[AM_COBSZPE_ZEROLESS_MAX + 2] = {
            COBSZPE_ZEROLESS_223, 0, 0
        };
        unsigned char val[1 + AM_COBSZPE_ZEROLESS_MAX + 1 + 1] = {
            AM_COBSZPE_ZEROLESS_MAX + 1, COBSZPE_ZEROLESS_223, 0xE1, 0
        };
        test_cobszpe_encode_run(src, AM_COUNTOF(src), val, AM_COUNTOF(val));
    }
    {
        unsigned char src[AM_COBSZPE_ZEROLESS_MAX + 3] = {
            COBSZPE_ZEROLESS_223, 0, 0, 0
        };
        unsigned char val[1 + AM_COBSZPE_ZEROLESS_MAX + 1 + 1 + 1] = {
            AM_COBSZPE_ZEROLESS_MAX + 1, COBSZPE_ZEROLESS_223, 0xE1, 0x01, 0
        };
        test_cobszpe_encode_run(src, AM_COUNTOF(src), val, AM_COUNTOF(val));
    }
    {
        unsigned char src[] = {COBSZPE_ZEROLESS_223, COBSZPE_ZEROLESS_223};
        unsigned char val[] = {
            AM_COBSZPE_ZEROLESS_MAX + 1,
            COBSZPE_ZEROLESS_223,
            AM_COBSZPE_ZEROLESS_MAX + 1,
            COBSZPE_ZEROLESS_223,
            0
        };
        test_cobszpe_encode_run(src, AM_COUNTOF(src), val, AM_COUNTOF(val));
    }

#define COBSZPE_ZEROLESS_29 COBSZPE_ZEROLESS_20 5, 4, 3, 2, 1, 3, 5, 7, 8

    {
        unsigned char src[] = {COBSZPE_ZEROLESS_29, 0, 0};
        unsigned char val[] = {0xFE, COBSZPE_ZEROLESS_29, 0};
        test_cobszpe_encode_run(src, AM_COUNTOF(src), val, AM_COUNTOF(val));
    }
    {
        unsigned char src[] = {COBSZPE_ZEROLESS_29, 0};
        unsigned char val[] = {30, COBSZPE_ZEROLESS_29, 0};
        test_cobszpe_encode_run(src, AM_COUNTOF(src), val, AM_COUNTOF(val));
    }

#define COBSZPE_ZEROLESS_30 COBSZPE_ZEROLESS_29, 5

    {
        unsigned char src[] = {COBSZPE_ZEROLESS_30, 0, 0};
        unsigned char val[] = {0xFF, COBSZPE_ZEROLESS_30, 0};
        test_cobszpe_encode_run(src, AM_COUNTOF(src), val, AM_COUNTOF(val));
    }
    {
        unsigned char src[] = {COBSZPE_ZEROLESS_30, 0};
        unsigned char val[] = {31, COBSZPE_ZEROLESS_30, 0};
        test_cobszpe_encode_run(src, AM_COUNTOF(src), val, AM_COUNTOF(val));
    }
    {
        unsigned char src[] = {COBSZPE_ZEROLESS_30, 0, 0, 0};
        unsigned char val[] = {0xFF, COBSZPE_ZEROLESS_30, 1, 0};
        test_cobszpe_encode_run(src, AM_COUNTOF(src), val, AM_COUNTOF(val));
    }

#define COBSZPE_ZEROLESS_31 COBSZPE_ZEROLESS_30, 5

    {
        unsigned char src[] = {COBSZPE_ZEROLESS_31, 0, 0};
        unsigned char val[] = {32, COBSZPE_ZEROLESS_31, 1, 0};
        test_cobszpe_encode_run(src, AM_COUNTOF(src), val, AM_COUNTOF(val));
    }
    {
        unsigned char src[] = {COBSZPE_ZEROLESS_31, 0};
        unsigned char val[] = {32, COBSZPE_ZEROLESS_31, 0};
        test_cobszpe_encode_run(src, AM_COUNTOF(src), val, AM_COUNTOF(val));
    }
    {
        unsigned char src[] = {COBSZPE_ZEROLESS_31, 0, 0, 0};
        unsigned char val[] = {32, COBSZPE_ZEROLESS_31, 0xE1, 0};
        test_cobszpe_encode_run(src, AM_COUNTOF(src), val, AM_COUNTOF(val));
    }
}

int main(void) {
    test_cobszpe_encode();

    return 0;
}

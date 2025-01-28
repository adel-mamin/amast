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

#include "common/macros.h"
#include "crc/crc.h"

static void test_crc16(void) {
    {
        unsigned char data[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
        static const unsigned int crc_init = 0xFFFF;
        unsigned int crc = crc16(data, AM_COUNTOF(data), crc_init);
        AM_ASSERT(0x29b1 == crc);

        crc = crc_init;
        for (int i = 0; i < AM_COUNTOF(data); ++i) {
            crc = crc16(&data[i], 1, crc);
        }
        AM_ASSERT(0x29b1 == crc);
    }

    {
        unsigned char data[] = {
            '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '0'
        };
        static const unsigned int crc_init = 0xFFFF;
        unsigned int crc = crc16(data, AM_COUNTOF(data), crc_init);
        data[9] = (unsigned char)(crc >> 8u);
        data[10] = (unsigned char)crc;
        crc = crc16(&data[9], 2, crc);
        AM_ASSERT(0 == crc);
    }
}

static void test_crc24(void) {
    {
        unsigned char data[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
        static const unsigned long crc_init = 0;
        unsigned long crc = crc24(data, AM_COUNTOF(data), crc_init);
        AM_ASSERT(0xcde703 == crc);

        crc = crc_init;
        for (int i = 0; i < AM_COUNTOF(data); ++i) {
            crc = crc24(&data[i], 1, crc);
        }
        AM_ASSERT(0xcde703 == crc);
    }

    {
        unsigned char data[] = {
            '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '0', '0'
        };
        static const unsigned long crc_init = 0;
        unsigned long crc = crc24(data, AM_COUNTOF(data), crc_init);
        data[9] = (unsigned char)(crc >> 16u);
        data[10] = (unsigned char)(crc >> 8u);
        data[11] = (unsigned char)crc;
        crc = crc24(&data[9], 3, crc);
        AM_ASSERT(0 == crc);
    }
}

static void test_crc32(void) {
    {
        unsigned char data[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
        static const unsigned long crc_init = 0xFFFFFFFF;
        unsigned long crc = crc32(data, AM_COUNTOF(data), crc_init);
        AM_ASSERT(0x0376e6e7 == crc);

        crc = crc_init;
        for (int i = 0; i < AM_COUNTOF(data); ++i) {
            crc = crc32(&data[i], 1, crc);
        }
        AM_ASSERT(0x0376e6e7 == crc);
    }

    {
        unsigned char data[] = {
            '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '0', '0', '0'
        };
        static const unsigned long crc_init = 0;
        unsigned long crc = crc32(data, AM_COUNTOF(data), crc_init);
        data[9] = (unsigned char)(crc >> 24U);
        data[10] = (unsigned char)(crc >> 16U);
        data[11] = (unsigned char)(crc >> 8U);
        data[12] = (unsigned char)crc;
        crc = crc32(&data[9], 4, crc);
        AM_ASSERT(0 == crc);
    }
}

int main(void) {
    test_crc16();
    test_crc24();
    test_crc32();

    return 0;
}

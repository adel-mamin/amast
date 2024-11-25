/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2016-2018 Adel Mamin
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
 * CRC utilities.
 */

#ifndef AM_CRC_H_INCLUDED
#define AM_CRC_H_INCLUDED

/**
 * Compute CRC16.
 *
 * The parameter set:
 * Name   : "CRC-16/CITT-FALSE"
 * Width  : 16
 * Poly   : 0x1021
 * Init   : 0xFFFF
 * RefIn  : False
 * RefOut : False
 * XorOut : 0x0000
 *
 * See
 * http://reveng.sourceforge.net/crc-catalogue/16.htm#crc.cat.crc-16-ccitt-false
 * for details.
 *
 * @param data  data for which the checksum is to be calculated
 *              NULL is a valid value if size is zero.
 * @param size  data size [bytes]
 * @param crc   initial CRC value
 *              Must be 0xFFFF at start or the result of previous crc16() call,
 *              if the data is provided by chunks, in which case crc
 *              is the CRC value of the previous data chunk.
 * @return the calculated CRC value.
 */
unsigned int crc16(const unsigned char *data, int size, unsigned int crc);

/**
 * Compute CRC24.
 *
 * The parameter set:
 * Name   : "CRC-24/LTE-A"
 * Width  : 24
 * Poly   : 0x864cfb
 * Init   : 0x000000
 * RefIn  : False
 * RefOut : False
 * XorOut : 0x000000
 * Check  : 0xcde703
 *
 * See
 * http://reveng.sourceforge.net/crc-catalogue/17plus.htm#crc.cat-bits.24
 * for details.
 *
 * @param data  data for which the checksum is to be calculated
 *              NULL is a valid value if size is zero.
 * @param size  data array size [bytes]
 * @param crc   initial CRC value
 *              Must be 0x000000 at start or the result of previous crc24()
 *              call, if the data is provided by chunks, in which case \a crc
 *              is the CRC value of the previous data chunk.
 * @return the calculated CRC value.
 */
unsigned long crc24(const unsigned char *data, int size, unsigned long crc);

/**
 * Compute CRC32.
 *
 * The parameter set:
 * Name   : "CRC-32/MPEG-2"
 * Width  : 32
 * Poly   : 0x04C11DB7
 * Init   : 0xFFFFFFFF
 * RefIn  : False
 * RefOut : False
 * XorOut : 0x00000000
 * Check  : 0x0376e6e7
 *
 * See
 * http://www.ross.net/crc/download/crc_v3.txt
 * http://reveng.sourceforge.net/crc-catalogue/17plus.htm (CRC-32/MPEG-2)
 * for details.
 *
 * @param data  data for which the checksum is to be calculated
 *              NULL is a valid value if size is zero
 * @param size  data array size [bytes]
 * @param crc   initial CRC value
 *              Must be 0xFFFFFFFF at start or the result of previous crc32()
 *              call, if the data is provided by chunks, in which case crc
 *              is the CRC value of the previous data chunk.
 * @return the calculated CRC value
 */
unsigned long crc32(const unsigned char *data, int size, unsigned long crc);

/**
 * The 8 bit Fletcher algorithm implementation.
 * Reference: RFC-1145 (used in the TCP standard)
 * @param data  data buffer
 * @param size  data buffer size [bytes]
 * @param ch_a  first 8 bit checksum
 *              Clear to zero before the first call.
 * @param ch_b  second 8 bit checksum
 *              Clear to zero before the first call.
 */
void crc_fletcher8(
    const unsigned char *data,
    int size,
    unsigned char *ck_a,
    unsigned char *ck_b
);

#endif /* AM_CRC_H_INCLUDED */

/*
 * loggertools
 * Copyright (C) 2004-2007 Max Kellermann <max@duempel.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the License
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include <sys/types.h>

#include "filser.h"

static unsigned char calc_crc_char(unsigned char d, unsigned char crc) {
    unsigned char tmp;
    const unsigned char crcpoly = 0x69;
    int count;

    for (count = 8; --count >= 0; d <<= 1) {
        tmp = crc ^ d;
        crc <<= 1;
        if (tmp & 0x80)
            crc ^= crcpoly;
    }
    return crc;
}

unsigned char filser_calc_crc(const void *p0, size_t len) {
    const unsigned char *p = p0;
    size_t i;
    unsigned char crc = 0xff;

    for (i = 0; i < len; i++)
        crc = calc_crc_char(p[i], crc);

    return crc;
}

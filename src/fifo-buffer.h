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
 *
 * $Id$
 */

#ifndef __FIFO_BUFFER_H
#define __FIFO_BUFFER_H

#include <stddef.h>

typedef struct fifo_buffer *fifo_buffer_t;


#ifdef __cplusplus
extern "C" {
#endif

int fifo_buffer_new(size_t size, fifo_buffer_t *buffer_r);

void fifo_buffer_delete(fifo_buffer_t *buffer_r);

void fifo_buffer_clear(fifo_buffer_t buffer);

const void *fifo_buffer_read(const fifo_buffer_t buffer, size_t *length_r);

void fifo_buffer_consume(fifo_buffer_t buffer, size_t length);

void *fifo_buffer_write(fifo_buffer_t buffer, size_t *max_length_r);

void fifo_buffer_append(fifo_buffer_t buffer, size_t length);

#ifdef __cplusplus
}
#endif


#endif
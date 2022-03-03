/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Damien P. George
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
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef MICROPY_INCLUDED_EXTMOD_VFS_MAP_H
#define MICROPY_INCLUDED_EXTMOD_VFS_MAP_H

#include "py/builtin.h"
#include "py/obj.h"
#include "py/reader.h"

typedef struct _mp_obj_vfs_map_t mp_obj_vfs_map_t;

extern const mp_obj_type_t mp_type_vfs_map;
extern const mp_obj_type_t mp_type_vfs_map_fileio;
extern const mp_obj_type_t mp_type_vfs_map_textio;

mp_import_stat_t mp_vfs_map_search_filesystem(mp_obj_vfs_map_t *self, const char *path, size_t *size_out, const uint8_t **data_out);
mp_obj_t mp_vfs_map_file_open(mp_obj_t self_in, mp_obj_t path_in, mp_obj_t mode_in);

mp_uint_t mp_vfs_map_readbyte(void *data);
const uint8_t *mp_vfs_map_readchunk(void *data, size_t len);
void mp_vfs_map_new_reader(mp_reader_t *reader, mp_obj_t map_file);

#endif // MICROPY_INCLUDED_EXTMOD_VFS_MAP_H

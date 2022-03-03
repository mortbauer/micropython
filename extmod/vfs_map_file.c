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

#include <string.h>

#include "py/runtime.h"
#include "py/stream.h"
#include "extmod/vfs_map.h"

#if MICROPY_VFS_MAP

typedef struct _mp_obj_vfs_map_file_t {
    mp_obj_base_t base;
    size_t file_size;
    size_t file_offset;
    const uint8_t *file_data;
} mp_obj_vfs_map_file_t;

mp_obj_t mp_vfs_map_file_open(mp_obj_t self_in, mp_obj_t path_in, mp_obj_t mode_in) {
    mp_obj_vfs_map_t *self = MP_OBJ_TO_PTR(self_in);

    const char *mode_s = mp_obj_str_get_str(mode_in);
    const mp_obj_type_t *type = &mp_type_vfs_map_textio;
    while (*mode_s) {
        switch (*mode_s++) {
            case 'r':
                break;
            case 'w':
            case 'a':
            case '+':
                mp_raise_OSError(MP_EROFS);
                #if MICROPY_PY_IO_FILEIO
            // If we don't have io.FileIO, then files are in text mode implicitly
            case 'b':
                type = &mp_type_vfs_map_fileio;
                break;
            case 't':
                type = &mp_type_vfs_map_textio;
                break;
                #endif
        }
    }

    mp_obj_vfs_map_file_t *o = m_new_obj(mp_obj_vfs_map_file_t);
    o->base.type = type;
    o->file_offset = 0;

    const char *path = mp_obj_str_get_str(path_in);
    mp_import_stat_t stat = mp_vfs_map_search_filesystem(self, path, &o->file_size, &o->file_data);
    if (stat == MP_IMPORT_STAT_NO_EXIST) {
        mp_raise_OSError(MP_ENOENT);
    }

    return MP_OBJ_FROM_PTR(o);
}

STATIC mp_obj_t vfs_map_file___exit__(size_t n_args, const mp_obj_t *args) {
    (void)n_args;
    return mp_stream_close(args[0]);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(vfs_map_file___exit___obj, 4, 4, vfs_map_file___exit__);

STATIC mp_uint_t vfs_map_file_read(mp_obj_t o_in, void *buf, mp_uint_t size, int *errcode) {
    mp_obj_vfs_map_file_t *self = MP_OBJ_TO_PTR(o_in);
    size_t remain = self->file_size - self->file_offset;
    if (size > remain) {
        size = remain;
    }
    memcpy(buf, self->file_data + self->file_offset, size);
    self->file_offset += size;
    return size;
}

STATIC mp_uint_t vfs_map_file_ioctl(mp_obj_t o_in, mp_uint_t request, uintptr_t arg, int *errcode) {
    mp_obj_vfs_map_file_t *self = MP_OBJ_TO_PTR(o_in);

    switch (request) {
        case MP_STREAM_FLUSH:
            return 0;
        case MP_STREAM_SEEK: {
            struct mp_stream_seek_t *s = (struct mp_stream_seek_t *)arg;
            if (s->whence == 0) { // SEEK_SET
                self->file_offset = MIN(self->file_size, (size_t)s->offset);
            } else if (s->whence == 1) { // SEEK_CUR
                self->file_offset = MIN(self->file_size, self->file_offset + s->offset);
            } else { // SEEK_END
                self->file_offset = self->file_size;
            }
            s->offset = self->file_offset;
            return 0;
        }
        case MP_STREAM_CLOSE:
            return 0;
        case MP_STREAM_POLL: {
            mp_uint_t ret = 0;
            if (arg & MP_STREAM_POLL_RD) {
                ret |= MP_STREAM_POLL_RD;
            }
            return ret;
        }
        default:
            *errcode = MP_EINVAL;
            return MP_STREAM_ERROR;
    }
}

STATIC const mp_rom_map_elem_t vfs_map_rawfile_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&mp_stream_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_readinto), MP_ROM_PTR(&mp_stream_readinto_obj) },
    { MP_ROM_QSTR(MP_QSTR_readline), MP_ROM_PTR(&mp_stream_unbuffered_readline_obj) },
    { MP_ROM_QSTR(MP_QSTR_readlines), MP_ROM_PTR(&mp_stream_unbuffered_readlines_obj) },
    { MP_ROM_QSTR(MP_QSTR_seek), MP_ROM_PTR(&mp_stream_seek_obj) },
    { MP_ROM_QSTR(MP_QSTR_tell), MP_ROM_PTR(&mp_stream_tell_obj) },
    { MP_ROM_QSTR(MP_QSTR_flush), MP_ROM_PTR(&mp_stream_flush_obj) },
    { MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&mp_stream_close_obj) },
    { MP_ROM_QSTR(MP_QSTR___enter__), MP_ROM_PTR(&mp_identity_obj) },
    { MP_ROM_QSTR(MP_QSTR___exit__), MP_ROM_PTR(&vfs_map_file___exit___obj) },
};
STATIC MP_DEFINE_CONST_DICT(vfs_map_rawfile_locals_dict, vfs_map_rawfile_locals_dict_table);

#if MICROPY_PY_IO_FILEIO
STATIC const mp_stream_p_t vfs_map_fileio_stream_p = {
    .read = vfs_map_file_read,
    .ioctl = vfs_map_file_ioctl,
};

const mp_obj_type_t mp_type_vfs_map_fileio = {
    { &mp_type_type },
    .name = MP_QSTR_FileIO,
    .getiter = mp_identity_getiter,
    .iternext = mp_stream_unbuffered_iter,
    .protocol = &vfs_map_fileio_stream_p,
    .locals_dict = (mp_obj_dict_t *)&vfs_map_rawfile_locals_dict,
};
#endif

STATIC const mp_stream_p_t vfs_map_textio_stream_p = {
    .read = vfs_map_file_read,
    .ioctl = vfs_map_file_ioctl,
    .is_text = true,
};

const mp_obj_type_t mp_type_vfs_map_textio = {
    { &mp_type_type },
    .name = MP_QSTR_TextIOWrapper,
    .getiter = mp_identity_getiter,
    .iternext = mp_stream_unbuffered_iter,
    .protocol = &vfs_map_textio_stream_p,
    .locals_dict = (mp_obj_dict_t *)&vfs_map_rawfile_locals_dict,
};

mp_uint_t mp_vfs_map_readbyte(void *data) {
    mp_obj_vfs_map_file_t *self = data;
    if (self->file_offset >= self->file_size) {
        return MP_READER_EOF;
    }
    return self->file_data[self->file_offset++];
}

STATIC void mp_vfs_map_close(void *data) {
    (void)data;
}

const uint8_t *mp_vfs_map_readchunk(void *data, size_t len) {
    mp_obj_vfs_map_file_t *self = data;
    const uint8_t *ptr = self->file_data + self->file_offset;
    self->file_offset += len;
    return ptr;
}

void mp_vfs_map_new_reader(mp_reader_t *reader, mp_obj_t map_file) {
    reader->data = MP_OBJ_TO_PTR(map_file);
    reader->readbyte = mp_vfs_map_readbyte;
    reader->close = mp_vfs_map_close;
}

#endif // MICROPY_VFS_MAP

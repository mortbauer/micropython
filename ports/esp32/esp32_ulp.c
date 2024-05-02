/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 "Andreas Valder" <andreas.valder@serioese.gmbh>
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

#include "py/runtime.h"
#include "py/parsenumbase.h"
#include "py/smallint.h"
#include "py/objint.h"

#if CONFIG_IDF_TARGET_ESP32
#include "esp32/ulp.h"
#elif CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3
#if CONFIG_ULP_COPROC_TYPE_RISCV
#include "ulp_riscv.h"
#else
#include "esp32/ulp.h"
#endif
#endif

#if CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3

#include "esp_err.h"

typedef struct _esp32_ulp_obj_t {
    mp_obj_base_t base;
} esp32_ulp_obj_t;

const mp_obj_type_t esp32_ulp_type;

// singleton ULP object
static const esp32_ulp_obj_t esp32_ulp_obj = {{&esp32_ulp_type}};

static mp_obj_t esp32_ulp_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    // check arguments
    mp_arg_check_num(n_args, n_kw, 0, 0, false);

    // return constant object
    return (mp_obj_t)&esp32_ulp_obj;
}

static mp_obj_t esp32_ulp_set_wakeup_period(mp_obj_t self_in, mp_obj_t period_index_in, mp_obj_t period_us_in) {
    mp_uint_t period_index = mp_obj_get_int(period_index_in);
    mp_uint_t period_us = mp_obj_get_int(period_us_in);
    int _errno = ulp_set_wakeup_period(period_index, period_us);
    if (_errno != ESP_OK) {
        mp_raise_OSError(_errno);
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(esp32_ulp_set_wakeup_period_obj, esp32_ulp_set_wakeup_period);

#if (CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3) && CONFIG_ULP_COPROC_TYPE_FSM
    static mp_obj_t esp32_ulp_load_binary(mp_obj_t self_in, mp_obj_t load_addr_in, mp_obj_t program_binary_in) {
        mp_uint_t load_addr = mp_obj_get_int(load_addr_in);

        mp_buffer_info_t bufinfo;
        mp_get_buffer_raise(program_binary_in, &bufinfo, MP_BUFFER_READ);

        int _errno = ulp_load_binary(load_addr, bufinfo.buf, bufinfo.len / sizeof(uint32_t));
        if (_errno != ESP_OK) {
            mp_raise_OSError(_errno);
        }
        return mp_const_none;
    }
    static MP_DEFINE_CONST_FUN_OBJ_3(esp32_ulp_load_binary_obj, esp32_ulp_load_binary);

    static mp_obj_t esp32_ulp_run(mp_obj_t self_in, mp_obj_t entry_point_in) {
        mp_uint_t entry_point = mp_obj_get_int(entry_point_in);
        int _errno = ulp_run(entry_point / sizeof(uint32_t));
        if (_errno != ESP_OK) {
            mp_raise_OSError(_errno);
        }
        return mp_const_none;
    }
    static MP_DEFINE_CONST_FUN_OBJ_2(esp32_ulp_run_obj, esp32_ulp_run);
#elif (CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3) && CONFIG_ULP_COPROC_TYPE_RISCV

    extern const uint8_t bin_start[] asm("_binary_ulp_main_bin_start");
    extern const uint8_t bin_end[]   asm("_binary_ulp_main_bin_end");

    static mp_obj_t esp32_ulp_riscv_load_default_binary(mp_obj_t self_in) {
        int _errno = ulp_riscv_load_binary(bin_start,(bin_end-bin_start));

        if (_errno != ESP_OK) {
            mp_raise_OSError(_errno);
        }
        return mp_const_none;
    }
    static MP_DEFINE_CONST_FUN_OBJ_1(esp32_ulp_riscv_load_default_binary_obj, esp32_ulp_riscv_load_default_binary);

    static mp_obj_t esp32_ulp_riscv_load_binary(mp_obj_t self_in, mp_obj_t program_binary_in) {
        mp_buffer_info_t bufinfo;
        mp_get_buffer_raise(program_binary_in, &bufinfo, MP_BUFFER_READ);
        int _errno = ulp_riscv_load_binary(bufinfo.buf, bufinfo.len);

        if (_errno != ESP_OK) {
            mp_raise_OSError(_errno);
        }
        return mp_const_none;
    }
    static MP_DEFINE_CONST_FUN_OBJ_2(esp32_ulp_riscv_load_binary_obj, esp32_ulp_riscv_load_binary);

    static mp_obj_t esp32_ulp_riscv_run(mp_obj_t self_in) {

        int _errno = ulp_riscv_run();
        if (_errno != ESP_OK) {
            mp_raise_OSError(_errno);
        }
        return mp_const_none;
    }
    static MP_DEFINE_CONST_FUN_OBJ_1(esp32_ulp_riscv_run_obj, esp32_ulp_riscv_run);

    // static mp_obj_t esp32_ulp_riscv_reset(mp_obj_t self_in) {
    //
    //     ulp_riscv_reset();
    //     return mp_const_none;
    // }
    // static MP_DEFINE_CONST_FUN_OBJ_1(esp32_ulp_riscv_reset_obj, esp32_ulp_riscv_reset);

    static mp_obj_t esp32_ulp_riscv_halt(mp_obj_t self_in) {

        ulp_riscv_halt();
        return mp_const_none;
    }
    static MP_DEFINE_CONST_FUN_OBJ_1(esp32_ulp_riscv_halt_obj, esp32_ulp_riscv_halt);

    static mp_obj_t esp32_ulp_riscv_timer_stop(mp_obj_t self_in) {

        ulp_riscv_timer_stop();
        return mp_const_none;
    }
    static MP_DEFINE_CONST_FUN_OBJ_1(esp32_ulp_riscv_timer_stop_obj, esp32_ulp_riscv_timer_stop);


    static mp_obj_t esp32_ulp_riscv_timer_resume(mp_obj_t self_in) {

        ulp_riscv_timer_resume();
        return mp_const_none;
    }
    static MP_DEFINE_CONST_FUN_OBJ_1(esp32_ulp_riscv_timer_resume_obj, esp32_ulp_riscv_timer_resume);

    // #include "ulp_genhdr/ulp_my_main.h"
    // static mp_obj_t esp32_ulp_riscv_get_var(mp_obj_t self_in) {
    //     return MP_OBJ_NEW_SMALL_INT(ulp_spi_result);
    // }
    // static MP_DEFINE_CONST_FUN_OBJ_1(esp32_ulp_riscv_get_var_obj, esp32_ulp_riscv_get_var);
#endif


#if ((CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3) && CONFIG_ULP_COPROC_TYPE_RISCV)
    #include "ulp_genhdr/esp32_ulpconst_qstr.h"

    static void ulp_isr_handler_func(void *arg) {
        mp_obj_t handler = MP_STATE_PORT(ulp_isr_handler);
        mp_sched_schedule(handler, mp_const_none);
    }

    static mp_obj_t esp32_ulp_riscv_isr_register(size_t n_pos_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
        enum {
            ARG_handler,
            ARG_arg,
            ARG_mask,
        };
        //{ MP_QSTR_d0,              MP_ARG_KW_ONLY  | MP_ARG_OBJ,   {.u_obj = MP_OBJ_NULL} },
        static const mp_arg_t allowed_args[] = {
            { MP_QSTR_handler,     MP_ARG_OBJ,   {.u_obj = mp_const_none} },
            { MP_QSTR_arg,          MP_ARG_KW_ONLY  | MP_ARG_OBJ,   {.u_obj = mp_const_none} },
            { MP_QSTR_mask,         MP_ARG_KW_ONLY  | MP_ARG_INT,   {.u_int = 0} },
        };
        mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
        mp_arg_parse_all(n_pos_args - 1, pos_args +1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
        mp_obj_t handler = args[ARG_handler].u_obj;
        mp_obj_t arg = args[ARG_arg].u_obj;
        mp_obj_t mask = args[ARG_mask].u_int;
        if (handler != mp_const_none){
            MP_STATE_PORT(ulp_isr_handler) = handler;
            esp_err_t err = ulp_riscv_isr_register(ulp_isr_handler_func,arg,mask);
            if (err != ESP_OK){
                if (err == ESP_ERR_INVALID_ARG){
                    mp_raise_ValueError(MP_ERROR_TEXT("Invalid arguments"));
                } else if (err == ESP_ERR_INVALID_STATE){
                    mp_raise_ValueError(MP_ERROR_TEXT("Invalid state"));
                }
            }
        } else {
            mp_raise_ValueError(MP_ERROR_TEXT("Handler can not be None"));
        }
        return mp_const_none;
    }
    static mp_obj_t esp32_ulp_riscv_isr_deregister(size_t n_pos_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
        enum {
            ARG_handler,
            ARG_arg,
            ARG_mask,
        };
        //{ MP_QSTR_d0,              MP_ARG_KW_ONLY  | MP_ARG_OBJ,   {.u_obj = MP_OBJ_NULL} },
        static const mp_arg_t allowed_args[] = {
            { MP_QSTR_handler,      MP_ARG_OBJ,   {.u_obj = mp_const_none} },
            { MP_QSTR_arg,          MP_ARG_KW_ONLY  | MP_ARG_OBJ,   {.u_obj = mp_const_none} },
            { MP_QSTR_mask,         MP_ARG_KW_ONLY  | MP_ARG_INT,   {.u_int = 0} },
        };
        mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
        mp_arg_parse_all(n_pos_args -1 , pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
        mp_obj_t handler = args[ARG_handler].u_obj;
        mp_obj_t arg = args[ARG_arg].u_obj;
        mp_obj_t mask = args[ARG_mask].u_int;
        if (handler != mp_const_none){
            esp_err_t err = ulp_riscv_isr_deregister(ulp_isr_handler_func,arg,mask);
            if (err != ESP_OK){
                if (err == ESP_ERR_INVALID_ARG){
                    mp_raise_ValueError(MP_ERROR_TEXT("Invalid arguments"));
                } else if (err == ESP_ERR_INVALID_STATE){
                    mp_raise_ValueError(MP_ERROR_TEXT("Invalid state"));
                }
            }
        } else {
            mp_raise_ValueError(MP_ERROR_TEXT("Handler can not be None"));
        }
        return mp_const_none;
    }
    static MP_DEFINE_CONST_FUN_OBJ_KW(esp32_ulp_riscv_isr_register_obj, 1, esp32_ulp_riscv_isr_register);
    static MP_DEFINE_CONST_FUN_OBJ_KW(esp32_ulp_riscv_isr_deregister_obj, 1, esp32_ulp_riscv_isr_deregister);
#endif

static const mp_rom_map_elem_t esp32_ulp_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_set_wakeup_period), MP_ROM_PTR(&esp32_ulp_set_wakeup_period_obj) },
    { MP_ROM_QSTR(MP_QSTR_RESERVE_MEM), MP_ROM_INT(CONFIG_ULP_COPROC_RESERVE_MEM) },
    #if (CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3) && CONFIG_ULP_COPROC_TYPE_RISCV
    #include "ulp_genhdr/esp32_ulpconst_mpz.h"
    { MP_ROM_QSTR(MP_QSTR_riscv_load_default_binary), MP_ROM_PTR(&esp32_ulp_riscv_load_default_binary_obj) },
    { MP_ROM_QSTR(MP_QSTR_riscv_load_binary), MP_ROM_PTR(&esp32_ulp_riscv_load_binary_obj) },
    { MP_ROM_QSTR(MP_QSTR_riscv_isr_register), MP_ROM_PTR(&esp32_ulp_riscv_isr_register_obj) },
    { MP_ROM_QSTR(MP_QSTR_riscv_isr_deregister), MP_ROM_PTR(&esp32_ulp_riscv_isr_deregister_obj) },
    { MP_ROM_QSTR(MP_QSTR_riscv_run), MP_ROM_PTR(&esp32_ulp_riscv_run_obj) },
    { MP_ROM_QSTR(MP_QSTR_RISCV_SW_INT), MP_ROM_INT(ULP_RISCV_SW_INT) },
    { MP_ROM_QSTR(MP_QSTR_RISCV_TRAP_INT), MP_ROM_INT(ULP_RISCV_TRAP_INT) },
    // { MP_ROM_QSTR(MP_QSTR_riscv_reset), MP_ROM_PTR(&esp32_ulp_riscv_reset_obj) },
    { MP_ROM_QSTR(MP_QSTR_riscv_halt), MP_ROM_PTR(&esp32_ulp_riscv_halt_obj) },
    { MP_ROM_QSTR(MP_QSTR_riscv_timer_stop), MP_ROM_PTR(&esp32_ulp_riscv_timer_stop_obj) },
    { MP_ROM_QSTR(MP_QSTR_riscv_timer_resume), MP_ROM_PTR(&esp32_ulp_riscv_timer_resume_obj) },
    // { MP_ROM_QSTR(MP_QSTR_riscv_get_var), MP_ROM_PTR(&esp32_ulp_riscv_get_var_obj) },
    #else
    { MP_ROM_QSTR(MP_QSTR_load_binary), MP_ROM_PTR(&esp32_ulp_load_binary_obj) },
    { MP_ROM_QSTR(MP_QSTR_run), MP_ROM_PTR(&esp32_ulp_run_obj) },
    #endif

};
static MP_DEFINE_CONST_DICT(esp32_ulp_locals_dict, esp32_ulp_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    esp32_ulp_type,
    MP_QSTR_ULP,
    MP_TYPE_FLAG_NONE,
    make_new, esp32_ulp_make_new,
    locals_dict, &esp32_ulp_locals_dict
    );

MP_REGISTER_ROOT_POINTER(mp_obj_t ulp_isr_handler);
#endif // CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3

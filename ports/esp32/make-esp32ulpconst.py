"""
This script reads in the given CMSIS device include file (eg stm32f405xx.h),
extracts relevant peripheral constants, and creates qstrs, mpz's and constants
for the stm module.
"""

# Use a pattern rule here so that make will only call make-stmconst.py once to
# make both modstm_const.h and modstm_qstr.h
# $(HEADER_BUILD)/%_const.h $(BUILD)/%_qstr.h: $(CMSIS_MCU_HDR) make-stmconst.py | $(HEADER_BUILD)
#         $(ECHO) "GEN stmconst $@"
#         $(Q)$(PYTHON) make-stmconst.py --qstr $(GEN_STMCONST_QSTR) --mpz $(GEN_STMCONST_MPZ) $(CMSIS_MCU_HDR) > $(GEN_STMCONST_HDR)

from __future__ import print_function

import re
import os
import json
import logging
import argparse

# Python 2/3 compatibility
import platform

logger = logging.getLogger(__name__)


def parse_file(filename):

    regex_pattern = '(\w+)\s\=\s*(?:"([^"]*)"|(\S+))'

    with open(filename, 'r') as file:
        content = file.read()

    shared_variables = {}

    matches = re.findall(regex_pattern, content)

    for m in matches:

        variable = m[0]
        address = m[2]
        # Check if the key starts with "ulp_"
        if variable.startswith("ulp_"):
            # Remove "ulp_" from the beginning of the key
            variable = variable[4:]

            shared_variables[variable.strip()] = address.strip()

    return shared_variables

def create_for_embedded_ulp(args,shared_variables):
    with open(os.path.join(args.output_dir,args.qstr_filename), "wt") as h_file:
        for key, value in shared_variables.items():
            assert 0 <= int(value, 16) <= 0xFFFFFFFF
            print(
                "STATIC const mp_obj_int_t mpz_%08x = {{&mp_type_int}, "
                "{.neg=0, .fixed_dig=1, .alloc=2, .len=2, "
                ".dig=(uint16_t*)(const uint16_t[]){%#x, %#x}}};"
                % (int(value, 16), int(value, 16) & 0xFFFF, (int(value, 16) >> 16) & 0xFFFF),
                file=h_file,
            )

    with open(os.path.join(args.output_dir,args.mpz_filename), "wt") as mpz_file:
        for key, value in shared_variables.items():
            print(
                "{MP_ROM_QSTR(MP_QSTR_%s), MP_ROM_PTR( & mpz_%08x)},"
                % (key.upper(), int(value, 16)),
                file=mpz_file,
            )

    with open(os.path.splitext(args.file)[0]+'.h','rb') as _inf:
        with open(os.path.join(args.output_dir,args.header_filename), "wb") as header_file:
            header_file.write(_inf.read())
    #{MP_ROM_QSTR(MP_QSTR_VOLTAGE), MP_ROM_PTR( & mpz_50000130)},
    data = {k:int(v,16) for k,v in shared_variables.items()}
    with open(os.path.join(args.output_dir,'map.json'), "w") as map_file:
        map_file.write(json.dumps(data))

def create_for_dynamic_ulp(args,shared_variables):
    data = {k:int(v,16) for k,v in shared_variables.items()}
    print(json.dumps(data))

def main():
    parser = argparse.ArgumentParser(description="Extract ULP constants from a C header file.")
    parser.add_argument(
        "-l",
        "--log-level",
        dest="log_level",
        default="info",
        help="Specify the log level"
    )
    subparsers = parser.add_subparsers()
    embedded_parser = subparsers.add_parser('create-for-embedded', help='create headers for embedded ulp')
    embedded_parser.set_defaults(command=create_for_embedded_ulp)
    dynamic_parser = subparsers.add_parser('create-for-dynamic', help='create python helpers for dynamic ulp')
    dynamic_parser.set_defaults(command=create_for_dynamic_ulp)

    embedded_parser.add_argument("file", help="input file")

    embedded_parser.add_argument(
        "-d",
        "--output-dir",
        dest="output_dir",
        default="build",
        help="Specify the output dir for all files"
    )
    embedded_parser.add_argument(
        "-q",
        "--qstr",
        dest="qstr_filename",
        default="esp32_ulpconst_qstr.h",
        help="Specified the name of the generated qstr header file",
    )
    embedded_parser.add_argument(
        "-m",
        "--mpz",
        dest="mpz_filename",
        default="esp32_ulpconst_mpz.h",
        help="the destination file of the generated mpz header",
    )
    embedded_parser.add_argument(
        "--header",
        dest="header_filename",
        default="ulp_main.h",
        help="the destination file of the generated header",
    )

    dynamic_parser.add_argument("file", help="input file")

    dynamic_parser.add_argument(
        "-d",
        "--output-dir",
        dest="output_dir",
        default="build",
        help="Specify the output dir for all files"
    )
    args = parser.parse_args()
    kwargs = vars(args)
    if not hasattr(args,'command'):
        parser.print_help()
    else:

        logging.basicConfig(
            format="%(levelname)s [%(asctime)s] %(name)s - %(message)s",
            datefmt="%Y-%m-%d %H:%M:%S",
            level=args.log_level.upper()
        )

        shared_variables = parse_file(args.file)

        logger.info("// Automatically generating from %s by make-esp32ulpconst.py", args.file)

        if not os.path.isdir(args.output_dir):
            try:
                os.makedirs(args.output_dir)
            except FileExistsError:
                pass

        args.command(args,shared_variables)
        return shared_variables

if __name__ == "__main__":
    main()

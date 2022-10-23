#!python3
'''
This tool create c-header file with data for every charmap file in source folder.
Charmap files could be found in /usr/share/i18n/charmaps/
Usage: conv.py <source_folder> <output_file>
'''

import re
import gzip
import sys
import glob
import os


def parse_str(s):
    ''' If string consist encodign info - return (char code,  Unicode code point)
        for example string '<U045B> /x9e         CYRILLIC SMALL LETTER TSHE (Serbocroatian)  '
        will return  (158, 1115)
    '''
    pattern = "(?<=<U)([0-9A-Fa-f]{4})(?=>)|((?<=/x)[0-9A-Fa-f]{2})"
    res = re.findall(pattern, s)
    if len(res) == 2 and len(res[0][0]) and len(res[1][1]):
        return (int(res[1][1], 16), int(res[0][0], 16))
    return None


def parse_file(filename):
    ''' parse some charmap.gz file (it could be found in /usr/share/i18n/charmaps/ ) 
        return list of integers (unicode), started with 0x80 char code'''
    map = []
    exp_ascii = 0x80  # ожидаемый код
    with gzip.open(filename, "rt") as f:
        for ln in f.readlines():
            d = parse_str(ln)
            if d:
                # Для некоторых табличных кодов могут быть пропуски в файле.
                # такие коды отслеживаем и заполняем нулем.
                while exp_ascii < d[0]:
                    map.append(0x26a0)
                    exp_ascii += 1
                if exp_ascii == d[0]:
                    map.append(d[1])
                    exp_ascii = d[0]+1

    return map


def main():
    if len(sys.argv) < 3:
        print("This tool create c-header file with data for every charmap file in source folder.\n",
              "Charmap files could be found in /usr/share/i18n/charmaps/\n",
              "Usage: conv.py <source_folder> <output_file>")
        return

    folder_path = sys.argv[1]
    ouput_file = sys.argv[2]
    files = [f for f in glob.glob(os.path.join(
        folder_path, "*.gz")) if os.path.isfile(f)]
    files.sort()

    with open(ouput_file, "wt") as of:
        of.writelines(["#pragma once\n\n",
                       "/* This file was automatically generated with conv.py utility */\n\n"])
        of.write("static unsigned int cp_data [][128] = {\n")
        charmaps = []
        for file in files:
            ar = parse_file(file)
            # charset name is file name w/o extension
            name = os.path.basename(file).lower().split('.')[0]
            if len(ar):
                if (len(charmaps)):
                    of.write(",\n")
                of.write(f"  // {name}\n  ")
                ar_str = '{' + str(ar)[1:-1] + '}'
                of.write(ar_str)
                charmaps.append(name)

        of.write('};\n\n')
        of.write('static const char* cp_names[] = {  // ')
        for s in charmaps:
            of.write(f',\n  "{s}"')
        of.write('\n};\n')

        of.write(f'static unsigned long cp_size = {len(charmaps)};\n')
        max_name_len = len(max(charmaps, key=len)) if len(charmaps) else 0
        of.write(
            f'static unsigned long max_cp_name_len = {max_name_len};\n\n')


if __name__ == "__main__":
    main()

#!/usr/bin/env python3

import re


def get_var_name(fname):

    return fname.replace('-', '.').replace('/', '_').split('.')[0]


def import_html(fname):

    varname = get_var_name(fname)
    sout = 'const char *{}_name PROGMEM = "/{}";\n'.format(varname, fname)
    sout += 'const char *{}_html PROGMEM = "\\\n'.format(varname)
    for line in open(fname, 'r'):
        line = line.replace('"', '\\"')
        line = line.rstrip()
        line += '\\n\\\n'
        sout += line
    sout += '";\n\n'
    return sout


def import_bin(fname, line_bytes=16):

    varname = get_var_name(fname)
    fcont = open(fname, 'rb').read()
    flen = len(fcont)
    sout = '#define {}_LEN {}\n'.format(varname.upper(), flen)
    sout += 'const char *{}_name PROGMEM = "/{}";\n'.format(varname, fname)
    sout += 'const char {}_bin[{}_LEN] PROGMEM = {}\n'.format(varname, varname.upper(), '{')
    first = True
    cnt = 0
    for b in fcont:
        if first:
            first = False
        else:
            sout += ',{}'.format('\n' if ((cnt % line_bytes) == 0) else ' ')
        sout += '0x{:02X}'.format(b)
        cnt += 1
    sout += '};\n\n'
    return sout


with open('../jumpOsketch/webi.h', 'w') as fout:

    print('Creating webi.h header file')
    fout.write('#ifndef __WEBI_H__\n#define __WEBI_H__\n\n')
    print('  Embed html file index.h')
    fout.write(import_html('index.html'))
    print('  Embed binary file jquery-3.5.1.min.js')
    fout.write(import_bin('jquery-3.5.1.min.js'))
    fout.write('#endif __WEBI_H__\n')
    print('Done')

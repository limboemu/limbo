#! /usr/bin/env python

# Reduce the number of relocations using a single string for the
# keys in the _pcre_utt table.

import re

fin = open('pcre_tables.c')
data = fin.read()
fin.close()

mo = re.search(r'const ucp_type_table _pcre_utt\[] = {', data)
assert mo, '_pcre_utt not found'
before = data[:mo.start()]
table_decl = data[mo.start():mo.end()]
table_start = mo.end()

mo = re.compile('};').search(data, table_start)
assert mo, 'end of _pcre_utt not found'
after = data[mo.end():]
table_end = mo.start()

table = data[table_start:table_end].strip()

rs = '\s*\{\s*"(?P<name>[^"]*)",\s*(?P<type>PT_[A-Z]*),\s*(?P<value>(?:0|ucp_[A-Za-z_]*))\s*},?\s*$'
r = re.compile(rs)

lines = []
names = []
pos = 0
for line in table.split('\n'):
    mo = r.match(line)
    assert mo, 'line not recognized'
    name, type, value = mo.groups()
    lines.append('  { %d, %s, %s }' % (pos, type, value))
    names.append(name)
    # +1 for the '\0'
    pos += len(name) + 1

table = ',\n'.join(lines)

names = ['  "%s\\0"' % n for n in names]
names_string = ' \n'.join(names) + ';'

data = before + \
       'const char _pcre_ucp_names[] =\n' + \
       names_string + \
       '\n\n' + \
       table_decl + \
       '\n' + \
       table + \
       '\n};' + \
       after

fout = open('pcre_tables.c', 'w')
fout.write(data)
fout.close()

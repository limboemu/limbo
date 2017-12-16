#!/bin/sh

if test $# = 1 ; then 
  ORIGINAL=$1
else
  echo "Usage: update.sh /path/to/libasyncns" 1>&2
  exit 1
fi

if test -f $ORIGINAL/libasyncns/asyncns.c ; then : ; else
  echo "Usage: update.sh /path/to/libasyncns" 1>&2
  exit 1
fi

for i in asyncns.c asyncns.h ; do
  sed -e 's/\([^a-z]\)asyncns_/\1_g_asyncns_/g' \
      -e 's/^asyncns_/_g_asyncns_/' \
      -e 's/<config\.h>/"g-asyncns\.h"/' \
    $ORIGINAL/libasyncns/$i > $i
done

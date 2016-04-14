#! /bin/sh

egrep '^#([^i]|if).*[^\]$' "${builddir:-.}/glibconfig.h" > glibconfig.cpp

INCLUDES="-include ${top_builddir:-..}/config.h"
INCLUDES="$INCLUDES -include glibconfig.cpp $GLIB_DEBUG_FLAGS"

cpp -P -DINCLUDE_INTERNAL_SYMBOLS -DINCLUDE_VARIABLES -DG_STDIO_NO_WRAP_ON_UNIX -DALL_FILES $INCLUDES "${srcdir:-.}/glib.symbols" | sed -e '/^$/d' -e 's/ G_GNUC.*$//' -e 's/ PRIVATE$//' | sort > expected-abi
rm -f glibconfig.cpp

nm -D -g --defined-only .libs/libglib-2.0.so | cut -d ' ' -f 3 | egrep -v '^(__bss_start|_edata|_end)' | sort > actual-abi

diff -u expected-abi actual-abi && rm -f expected-abi actual-abi

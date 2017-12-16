#ifndef DEBUGGER_H
#define DEBUGGER_H

#include <stdio.h>
#include <iostream>
#include <sstream>
#include <sys/types.h>

void dumpBacktrace(std::ostream& os, void** buffer, size_t count);
size_t captureBacktrace(void** buffer, size_t max);

#endif

#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

void fatal(const char *fmt, ...) __attribute__ ((noreturn, format (printf, 1, 2)));
void panic(const char *fmt, ...) __attribute__ ((noreturn, format (printf, 1, 2)));

#endif //DEBUG_H


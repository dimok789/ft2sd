#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#ifdef __cplusplus
extern "C"{
#endif

#include <stdarg.h>

void console_print_pos(int x, int y, const char *format, ...);
void console_printf(int newline, const char *format, ...);

void console_cleanup();

uint32_t console_acquire(void *context);
uint32_t console_release(void *context);

#ifdef __cplusplus
}
#endif

#endif /* _CONSOLE_H_ */

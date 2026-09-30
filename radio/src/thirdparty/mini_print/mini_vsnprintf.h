/**
 * mini_vsnprintf — tiny vsnprintf/snprintf
 *
 * Supported conversions:  %d  %X  %p  %s  %c  %%
 * Number padding:         width (e.g. %8d) and zero-pad (e.g. %08X)
 *                         applied to %d/%X/%p; %s/%c ignore width.
 *
 * No floats, no 64-bit, no precision, no length modifiers (l/h), no %u/%x.
 * Integers/pointers are 32-bit.  %X is uppercase hex.
 * %p prints 0x + 32-bit address (lowercase).  NULL %s prints as empty.
 *
 */
#ifndef MINI_VSNPRINTF_H
#define MINI_VSNPRINTF_H

#include <stdarg.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__GNUC__) || defined(__clang__)
#define MINI_PRINTF_FMT(fmt_idx, va_idx) \
    __attribute__((format(__printf__, fmt_idx, va_idx)))
#else
#define MINI_PRINTF_FMT(fmt_idx, va_idx)
#endif

/**
 * Write at most `size-1` characters into `buf`, always NUL-terminate when
 * `size > 0`.  Returns the number of characters that would have been written
 * (excluding NUL), matching ISO C vsnprintf.  `buf` may be NULL if `size == 0`.
 */
int mini_vsnprintf(char *buf, size_t size, const char *fmt, va_list ap);

int mini_snprintf(char *buf, size_t size, const char *fmt, ...)
    MINI_PRINTF_FMT(3, 4);

/**
 * Tiny strtoll subset for parsing console/config numbers without dragging
 * in newlib's stdio/reentrancy machinery (__sf + _impure_*).
 *
 * - base 0: optional sign, then 0x/0X prefix selects hex, else decimal.
 * - base 16: optional sign, optional 0x/0X prefix, then hex digits.
 * - other base: optional sign, then digits below base (e.g. 10).
 * - *endptr (unless NULL) points at the first unconsumed character, or at
 *   `str` when no digits were consumed - mirroring strtoll.
 *
 * Deliberate differences vs strtoll: no whitespace skipping, no octal, and
 * wraparound instead of saturation on >64 bit overflow.
 */
long long mini_strtoll(const char *str, char **endptr, int base);

#ifdef __cplusplus
}
#endif

#endif /* MINI_VSNPRINTF_H */

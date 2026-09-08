/**
 * Size-first vsnprintf: one pass, 32-bit ints, shared base-10/16 conversion.
 *
 */
#include "mini_vsnprintf.h"

int mini_vsnprintf(char *buf, size_t size, const char *fmt, va_list ap)
{
    size_t pos = 0;

    /* Store if a non-NUL slot remains. Always advance the would-be length. */
#define PUT(c) \
    do { \
        if (pos + 1U < size) \
            buf[pos] = (char)(c); \
        pos++; \
    } while (0)

    while (*fmt) {
        int zero, width, neg, n, base, pre;
        unsigned u;
        const char *s;
        const char *digs;
        char tmp[10];
        char spec;

        if (*fmt != '%') {
            PUT(*fmt++);
            continue;
        }
        fmt++;

        if (*fmt == '%') {
            PUT(*fmt++);
            continue;
        }

        /* number padding: '0' flag + decimal width, consumed for every spec */
        zero = 0;
        if (*fmt == '0') {
            zero = 1;
            fmt++;
        }
        width = 0;
        while (*fmt >= '0' && *fmt <= '9')
            width = width * 10 + (*fmt++ - '0');

        spec = *fmt;
        if (spec)
            fmt++;

        if (spec == 's') {
            s = va_arg(ap, const char *);
            if (!s)
                s = "";
            while (*s)
                PUT(*s++);
        } else if (spec == 'c') {
            PUT(va_arg(ap, int));
        } else if (spec == 'd' || spec == 'u' || spec == 'x' ||
                   spec == 'X' || spec == 'p') {
            neg = 0;
            pre = 0; /* 2 if %p → leading "0x" */
            digs = (spec == 'X') ? "0123456789ABCDEF" : "0123456789abcdef";

            if (spec == 'd') {
                int v = va_arg(ap, int);
                if (v < 0) {
                    neg = 1;
                    u = 0u - (unsigned)v; /* defined for INT_MIN */
                } else {
                    u = (unsigned)v;
                }
                base = 10;
#if defined(DEBUG)
            } else if (spec == 'p') {
                u = (unsigned)(size_t)va_arg(ap, void *);
                base = 16;
                pre = 2;
#endif
            } else {
                u = va_arg(ap, unsigned);
                base = (spec == 'u') ? 10 : 16;
            }

            n = 0;
            do {
                tmp[n++] = digs[u % (unsigned)base];
                u /= (unsigned)base;
            } while (u);

            width -= n + neg + pre;
            if (zero) {
                if (neg) {
                    PUT('-');
                    neg = 0;
                }
#if defined(DEBUG)
                if (pre) {
                    PUT('0');
                    PUT('x');
                    pre = 0;
                }
#endif
            }
            while (width-- > 0)
                PUT(zero ? '0' : ' ');
            if (neg)
                PUT('-');
#if defined(DEBUG)
            if (pre) {
                PUT('0');
                PUT('x');
            }
#endif
            while (n--)
                PUT(tmp[n]);
        } else if (spec) {
            PUT(spec);
        }
    }

#undef PUT

    if (size)
        buf[pos < size ? pos : size - 1U] = '\0';

    return (int)pos;
}

int mini_snprintf(char *buf, size_t size, const char *fmt, ...)
{
    va_list ap;
    int n;

    va_start(ap, fmt);
    n = mini_vsnprintf(buf, size, fmt, ap);
    va_end(ap);
    return n;
}

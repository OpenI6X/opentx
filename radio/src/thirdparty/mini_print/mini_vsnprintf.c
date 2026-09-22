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

        /* '0' flag + width + optional precision (.N, used by %s) */
        zero = 0;
        if (*fmt == '0') {
            zero = 1;
            fmt++;
        }
        width = 0;
        while (*fmt >= '0' && *fmt <= '9')
            width = width * 10 + (*fmt++ - '0');

// not needed, use string limit instead of precision for simplicity
#if defined(PRINTF_DOT_MODIFIER)
        int prec = -1; /* -1 = unlimited */
        if (*fmt == '.') {
            prec = 0;
            fmt++;
            while (*fmt >= '0' && *fmt <= '9')
                prec = prec * 10 + (*fmt++ - '0');
        }
#endif
        spec = *fmt;
        if (spec)
            fmt++;

        if (spec == 's') {
            s = va_arg(ap, const char *);
            if (!s)
                s = "";
            /* %3s or %.3s: max chars; no padding. 0 / omitted = unlimited */
#if defined(PRINTF_DOT_MODIFIER)
            n = (prec >= 0) ? prec : (width ? width : 0xff);
#else
            n = width ? width : 0xff;
#endif
            while (*s && n--)
                PUT(*s++);
        } else if (spec == 'c') {
            PUT(va_arg(ap, int));
        } else if (spec == 'd' || spec == 'u' || spec == 'x' ||
                   spec == 'X' || spec == 'p') {
            neg = 0;
            pre = 0; /* 2 if %p → leading "0x" */
#if defined(DEBUG)
            digs = (spec == 'X') ? "0123456789ABCDEF" : "0123456789abcdef";
#else
            digs = "0123456789ABCDEF";
#endif
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

long long mini_strtoll(const char *str, char **endptr, int base)
{
    const char *s = str;
    unsigned long long v = 0;
    int neg = 0;
    int any = 0;

    if (*s == '-' || *s == '+') {
        neg = (*s == '-');
        ++s;
    }

    if (base == 0) {
        if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
            base = 16;
            s += 2;
        }
        else {
            base = 10;
        }
    }
    else if (base == 16) {
        if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
            s += 2;
        }
    }

    for (; *s != '\0'; ++s) {
        int d;
        if (*s >= '0' && *s <= '9')
            d = *s - '0';
        else if (*s >= 'a' && *s <= 'f')
            d = *s - 'a' + 10;
        else if (*s >= 'A' && *s <= 'F')
            d = *s - 'A' + 10;
        else
            break;
        if (d >= base)
            break;
        v = v * (unsigned)base + (unsigned)d;
        any = 1;
    }

    if (endptr != NULL)
        *endptr = (char *)(any ? s : str);

    if (!any)
        return 0;
    if (neg)
        return -(long long)v;
    return (long long)v;
}

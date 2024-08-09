/**
 * Tiny implementation of sprintf for ExpressLRS menu handling needs.
 * String input always expect length passed as parameter.
 * @author Jan Kozak (ajjjjjjjj)
 */
#include <cstdarg>

void tiny_sprintf(char *arr, char const *fmt, char argsLen, ...) {
  va_list args;
  va_start(args, argsLen);
  char ch;
  int int_temp;
  char char_temp;
  char *string_temp;

  while ((ch = *fmt++)) {
    if ('%' == ch) {
      switch ((ch = *fmt++)) {
        case 'c':
          char_temp = va_arg(args, int);
          *arr++ = char_temp;
          break;
        case 's':
          int_temp = va_arg(args, int); // reuse as string length
          string_temp = va_arg(args, char *);
          arr = strAppend(arr, string_temp, int_temp);
          break;
        case 'd':
          int_temp = va_arg(args, int);
          arr = strAppendSigned(arr, int_temp);
          break;
      }
    } else {
      *arr++ = ch;
    }
  }
  va_end(args);
  *arr = '\0';
}

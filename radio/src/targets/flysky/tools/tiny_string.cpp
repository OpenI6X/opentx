/**
 * Minimal implementation of sprintf for elrsv2 needs.
 */
#include <cstdarg>

void tiny_sprintf(char *arr, char const *fmt, char len, char num, ...) {
  va_list args;
  va_start(args, num);
  char ch;
  unsigned int length = 0;
  int int_temp;
  char char_temp;
  char *string_temp;

  while ((ch = *fmt++)) {
    if ('%' == ch) {
      switch ((ch = *fmt++)) {
        case 'c':
          char_temp = va_arg(args, int);
          arr[length] = char_temp;
          length++;
          break;
        case 's':
          string_temp = va_arg(args, char *);
          memcpy(&arr[length], string_temp, len);
          length += len;
          break;
        case 'u':
          int_temp = va_arg(args, int);
          char *tmp_ptr = strAppendUnsigned(&arr[length], int_temp);
          length += (tmp_ptr - &arr[length]);
          break;
      }
    }
    else {
        arr[length] = ch;
        length++;
    }
  }
  va_end(args);
  arr[length] = '\0';
}

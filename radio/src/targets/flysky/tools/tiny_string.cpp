/**
 * Minimal implementation of sprintf and itoa for elrsv2 needs.
 */
#include <cstdarg>

int tiny_itoa(int number, char *arr) {
  int r, len, digits = 1, i = 0;
  const uint8_t radix = 10;

  if (number > 99) digits++;
  if (number > 9) digits++;
  
  len = digits;
  arr[i] = '0';

  while (number != 0) {
    digits--;
    r = number % radix;
    arr[i + digits] = r + '0';
    number /= radix;
  }

  return len;
}

void tiny_sprintf(char *arr, char const *fmt, char len, char num, ...) {
  va_list args;
  va_start(args, num);
  char ch;
  int length = 0;
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
          length += tiny_itoa(int_temp, &arr[length]);
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

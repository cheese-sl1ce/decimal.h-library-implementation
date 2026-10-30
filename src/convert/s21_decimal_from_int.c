#include <stddef.h>
#include <stdint.h>

#include "../s21_decimal_internal.h"

int s21_int_to_mantissa(int src, s21_decimal *dst) {
  int exit_code = 0;
  if (dst == NULL)
    exit_code = 1;
  else {
    s21_zero_mantissa(dst);
    uint32_t abs_value =
        (src < 0) ? (uint32_t)(-(int64_t)src)
                  : (uint32_t)src;  // защита от переполнения при INT_MIN
    dst->bits[0] = (int)abs_value;
  }
  return exit_code;
}

int s21_from_int_to_decimal(int src, s21_decimal *dst) {
  int exit_code = 0;
  if (dst == NULL)
    exit_code = 1;
  else {
    s21_zero_decimal(dst);
    if (src != 0) {
      int sign = src >= 0 ? 0 : 1;
      s21_int_to_mantissa(src, dst);
      s21_set_sign(dst, sign);
    }
  }
  return exit_code;
}
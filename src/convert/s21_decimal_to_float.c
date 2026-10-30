#include <stdint.h>

#include "../s21_decimal_internal.h"

int s21_from_decimal_to_float(s21_decimal src, float *dst) {
  int exit_code = 0;
  if (dst == NULL) {
    exit_code = 1;
  } else {
    int scale = s21_get_scale(src);
    if (scale < 0 || scale > 28) {
      exit_code = 1;
    } else {
      double result = 0.0;
      for (int i = 2; i >= 0; i--) {  // Собираем мантиссу в double
        result = result * 4294967296.0 + (uint32_t)src.bits[i];
      }
      for (int i = 0; i < scale; i++) {
        result /= 10.0;
      }
      if (s21_get_sign(src)) {
        result = -result;
      }
      *dst = (float)result;
    }
  }
  return exit_code;
}
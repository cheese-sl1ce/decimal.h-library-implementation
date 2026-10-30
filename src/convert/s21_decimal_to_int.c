#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#include "../s21_decimal_internal.h"

int s21_mantissa_to_int(s21_decimal value, int *dst) {
  int exit_code = 0;
  s21_decimal tmp = value;
  s21_truncate_abs(tmp, &tmp);
  if (dst == NULL)
    exit_code = 1;
  else if (tmp.bits[1] != 0 || tmp.bits[2] != 0)
    exit_code = 1;
  else {
    uint32_t magnitude = (uint32_t)tmp.bits[0];
    if (magnitude > (uint32_t)INT_MAX + 1u) {
      exit_code = 1;  // смотрим переполнение
    } else {
      *dst = (int)magnitude;
    }
  }
  return exit_code;
}

static int s21_apply_int_sign(int sign, int magnitude, int *dst) {
  int exit_code = 0;
  if (sign == 0 && magnitude < 0) {
    exit_code = 1;
  } else if (sign == 0) {
    *dst = magnitude;
  } else if (magnitude == INT_MIN) {
    *dst = INT_MIN;
  } else {
    *dst = -magnitude;
  }
  return exit_code;
}

int s21_from_decimal_to_int(s21_decimal src, int *dst) {
  int exit_code = 0;
  if (dst == NULL) {
    exit_code = 1;
  } else {
    int scale = s21_get_scale(src);
    if (scale < 0 || scale > 28) {
      exit_code = 1;
    } else {
      int sign = s21_get_sign(src);
      s21_decimal abs_val = src;
      s21_set_sign(&abs_val, 0);

      int magnitude = 0;
      exit_code = s21_mantissa_to_int(abs_val, &magnitude);
      if (exit_code == 0) {
        exit_code = s21_apply_int_sign(sign, magnitude, dst);
      }
    }
  }
  return exit_code;
}
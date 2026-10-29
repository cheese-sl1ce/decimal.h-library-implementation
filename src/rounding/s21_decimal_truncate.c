#include <stddef.h>

#include "../s21_decimal_internal.h"

int s21_truncate_abs(s21_decimal value, s21_decimal *res) {
  int exit_code = 0;
  if (res == NULL) {
    exit_code = 1;
  } else {
    *res = value;
    int scale = s21_get_scale(*res);
    if (scale != 0) {
      int rem = 0;
      for (int i = 0; i < scale; i++) {
        s21_div_by_10(res, &rem);
      }
      s21_set_scale(res, 0);
    }
    s21_set_sign(res, 0);
  }

  return exit_code;
}

int s21_truncate(s21_decimal value, s21_decimal *result) {
  if (result == NULL) return 1;

  int sign = s21_get_sign(value);
  s21_decimal abs_val = value;
  s21_set_sign(&abs_val, 0);

  s21_decimal truncated;
  int exit_code = s21_truncate_abs(abs_val, &truncated);

  if (exit_code == 0) {
    s21_set_sign(&truncated, sign);
    s21_normalize_scale(&truncated);
    *result = truncated;
  }
  return exit_code;
}
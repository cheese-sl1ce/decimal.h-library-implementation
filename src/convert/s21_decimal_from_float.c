#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../s21_decimal_internal.h"

// проверка что float в допустимом диапазоне
int s21_check_float_range(float src) {
  int exit_code = 0;
  float abs_src = fabsf(src);
  if (isnan(src) || isinf(src))  // обработка NaN и Infinity
    exit_code = 1;
  else if (abs_src > S21_MAX_FLOAT)
    exit_code = 1;
  else if (abs_src != 0.0f && abs_src < S21_MIN_FLOAT)
    exit_code = 1;
  return exit_code;
}

// разбиение float на массив значащих цифр
int s21_extract_significant_digits(float src, int *digits, int *count) {
  int exit_code = 0;
  if (digits == NULL || count == NULL)
    exit_code = 1;
  else {
    *count = 0;
    if (isnan(src) || isinf(src))
      exit_code = 1;
    else if (src != 0.0f) {
      float abs_src = fabsf(src);
      char buf[32];
      snprintf(buf, sizeof(buf), "%.7e", abs_src);
      int i = 0;
      if (buf[i] == '-' || buf[i] == '+') i++;
      digits[(*count)++] = buf[i] - '0';
      i++;
      if (buf[i] == '.') i++;
      while (buf[i] >= '0' && buf[i] <= '9') {
        digits[(*count)++] = buf[i] - '0';
        i++;
      }
      while (*count > 1 && digits[*count - 1] == 0) {
        (*count)--;
      }
    }
  }
  return exit_code;
}

static int s21_round_float_check_up(const int *digits, int keep, int count) {
  int first_dropped = digits[keep];  // первая отбрасываемая цифра
  int rest_nonzero = 0;
  int need_up = 0;
  int i = keep + 1;

  while (i < count && !rest_nonzero) {  // ищем ненулевые цифры
    if (digits[i] != 0) rest_nonzero = 1;
    i++;
  }
  // применяем банковское округление 7-й значащей цифры
  if (first_dropped > 5) {
    need_up = 1;
  } else if (first_dropped == 5) {
    if (rest_nonzero || digits[keep - 1] % 2 != 0) need_up = 1;
  }
  return need_up;
}

// прибавление единицы к числу (представленному в виде массива цифр)
static void s21_round_float_increment(int *digits, int keep) {
  int i = keep - 1;
  int carry = 1;

  while (i >= 0 && carry) {
    digits[i]++;
    if (digits[i] < 10) {
      carry = 0;
    } else {
      digits[i] = 0;
      i--;
    }
  }
  if (carry) {
    digits[0] = 1;
    for (int j = 1; j < keep; j++) digits[j] = 0;
  }
}

// округление до 7 значащих цифр
int s21_round_float_7_digits(int *digits, int *count) {
  int exit_code = 0;
  if (digits == NULL || count == NULL) {
    exit_code = 1;
  } else if (*count > 7) {
    int keep = 7;
    if (s21_round_float_check_up(digits, keep, *count)) {
      s21_round_float_increment(digits, keep);
    }
    *count = keep;
  }
  return exit_code;
}

// проверка переноса после округления до 7 цифр (все нули после 1)
static int s21_check_round_carry(const int *digits, int count, int old_first,
                                 int before) {
  int result = 0;
  if (before > 7 && old_first == 9 && digits[0] == 1) {
    int zero = 1;
    for (int i = 1; i < count && zero; i++) zero = (digits[i] == 0);
    result = zero;
  }
  return result;
}

// умножение мантиссы на 10
static int s21_scale_up(s21_decimal *dst, int *scale) {
  int exit_code = 0;
  while (*scale < 0 && !exit_code) {
    if (s21_mul_by_10(dst))
      exit_code = 1;
    else
      (*scale)++;
  }
  return exit_code;
}

// деление с банковским округлением
static int s21_scale_down(s21_decimal *dst, int *scale) {
  int exit_code = 0;
  while (*scale > 28 && !exit_code) {
    int rem = 0;
    s21_div_by_10(dst, &rem);
    if (rem > 5 || (rem == 5 && (dst->bits[0] & 1))) {
      s21_decimal one = {0}, tmp;
      one.bits[0] = 1;
      if (s21_add_mantissa(*dst, one, &tmp))
        exit_code = 1;
      else
        *dst = tmp;
    }
    if (!exit_code) (*scale)--;
  }
  return exit_code;
}

static int s21_float_get_exp10(float abs_src) {
  char buf[32];
  snprintf(buf, sizeof(buf), "%.7e", abs_src);
  char *e = strchr(buf, 'e');
  return e ? atoi(e + 1) : 0;
}

int s21_from_float_to_decimal(float src, s21_decimal *dst) {
  int exit_code = 0;
  if (dst == NULL) {
    exit_code = 1;
  } else {
    s21_zero_decimal(dst);
    if (s21_check_float_range(src)) {
      exit_code = 1;
    } else if (src != 0.0f) {
      int sign = src < 0, digits[16] = {0}, count = 0;
      float abs_src = fabsf(src);
      s21_extract_significant_digits(abs_src, digits, &count);

      int old_first = digits[0], before = count;
      s21_round_float_7_digits(digits, &count);

      int exp10 = s21_float_get_exp10(abs_src);
      if (s21_check_round_carry(digits, count, old_first, before)) exp10++;

      int scale = (count - 1) - exp10;
      for (int i = 0; i < count; i++) {
        dst->bits[0] = dst->bits[0] * 10 + digits[i];
      }

      exit_code = s21_scale_up(dst, &scale);
      if (!exit_code) exit_code = s21_scale_down(dst, &scale);
      if (!exit_code) {
        s21_set_scale(dst, scale);
        s21_set_sign(dst, sign);
        s21_normalize_scale(dst);
      }
    }
  }
  return exit_code;
}
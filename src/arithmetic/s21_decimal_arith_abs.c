#include <stdint.h>

#include "../s21_decimal_internal.h"

// ----- Вспомогательные функции для корректной обработки чисел, -----
// ----- вышедших за ограничение 96-битной мантисы decimal -----

static void s21_mul10_128(s21_decimal value, uint32_t out[4]) {
  uint64_t carry = 0;
  for (int i = 0; i < 3; i++) {
    uint64_t p = (uint64_t)(uint32_t)value.bits[i] * 10u + carry;
    out[i] = (uint32_t)p;
    carry = p >> 32;
  }
  out[3] = (uint32_t)carry;
}

static int s21_cmp_128(const uint32_t a[4], const uint32_t b[4]) {
  int res = 0;
  for (int i = 3; i >= 0 && res == 0; i--) {
    if (a[i] != b[i]) res = a[i] > b[i] ? 1 : -1;
  }
  return res;
}

static void s21_sub_128(uint32_t a[4], const uint32_t b[4]) {
  int64_t borrow = 0;
  for (int i = 0; i < 4; i++) {
    int64_t d = (int64_t)a[i] - (int64_t)b[i] - borrow;
    if (d < 0) {
      a[i] = (uint32_t)(d + ((int64_t)1 << 32));
      borrow = 1;
    } else {
      a[i] = (uint32_t)d;
      borrow = 0;
    }
  }
}

// подготовка чисел, в случае, если при попытке их сложения
// мантиса переполнилась (и выравнивание масштабов не сработало)
static void s21_add_abs_fix_align(s21_decimal *a, s21_decimal *b,
                                  int *exit_code) {
  int align_err = s21_align_scales(a, b);
  while (align_err && *exit_code == 0) {
    int sa = s21_get_scale(*a);
    int sb = s21_get_scale(*b);
    if (sa == 0 && sb == 0) {
      *exit_code = 1;
    } else if (sa >=
               sb) {  // уменьшаем точность для предотвращения переполнения
      s21_bank_round(a, 1);
    } else {
      s21_bank_round(b, 1);
    }
    if (*exit_code == 0) align_err = s21_align_scales(a, b);
  }
}

int s21_add_abs(s21_decimal a, s21_decimal b, s21_decimal *res) {
  int exit_code = 0;
  s21_zero_decimal(res);

  s21_add_abs_fix_align(&a, &b, &exit_code);

  if (exit_code == 0) {
    int scale = s21_get_scale(a);
    s21_decimal sum;
    s21_zero_decimal(&sum);
    int add_err = s21_add_mantissa(a, b, &sum);

    // если переполнение 96 бит, то уменьшаем точность
    while (add_err && scale > 0) {
      s21_bank_round(&a, 1);
      s21_bank_round(&b, 1);
      scale--;
      s21_zero_decimal(&sum);
      add_err = s21_add_mantissa(a, b, &sum);
    }

    if (add_err) {
      exit_code = 1;
    } else {
      s21_set_scale(&sum, scale);
      s21_set_sign(&sum, 0);
      *res = sum;
    }
  }
  return exit_code;
}

int s21_sub_abs(s21_decimal a, s21_decimal b, s21_decimal *res) {
  int exit_code = 0;
  s21_zero_decimal(res);

  int align_err = s21_align_scales(&a, &b);
  if (align_err) {
    exit_code = 1;
  } else {
    int scale = s21_get_scale(a);
    s21_decimal diff;
    s21_zero_decimal(&diff);
    s21_sub_mantissa(a, b, &diff);
    s21_set_scale(&diff, scale);
    s21_set_sign(&diff, 0);
    *res = diff;
  }
  return exit_code;
}

int s21_mul_abs(s21_decimal a, s21_decimal b, s21_decimal *res) {
  int exit_code = 0;
  s21_zero_decimal(res);

  int total_scale = s21_get_scale(a) + s21_get_scale(b);  // итоговый масштаб
  s21_decimal product;
  s21_zero_decimal(&product);

  if (s21_mul_mantissa(a, b, &product)) {
    exit_code = 1;  // переполнение мантисы
  } else if (s21_is_zero(product) && !s21_is_zero(a) && !s21_is_zero(b)) {
    exit_code = 2;  // произведение двух ненулевых чисел не может быть нулём
  } else {
    s21_set_scale(&product, total_scale);
    s21_set_sign(&product, 0);
    // если scale > 28, то применяем банковское округление
    if (total_scale > S21_MAX_SCALE) {
      s21_bank_round(&product, total_scale - S21_MAX_SCALE);
      //
      if (s21_is_zero(product) && !s21_is_zero(a) && !s21_is_zero(b)) {
        exit_code = 2;
      }
    }
    if (exit_code == 0) *res = product;
  }
  return exit_code;
}

// один шаг десятичного деления
static int s21_div_step(s21_decimal *quotient, s21_decimal *remainder,
                        s21_decimal b) {
  int status = 0;

  uint32_t r128[4];
  s21_mul10_128(*remainder, r128);

  // приведение делителя к 128-битному формату для сравнения с остатком
  uint32_t b128[4] = {(uint32_t)b.bits[0], (uint32_t)b.bits[1],
                      (uint32_t)b.bits[2], 0};

  int digit = 0;
  while (s21_cmp_128(r128, b128) >= 0) {
    s21_sub_128(r128, b128);
    digit++;
  }

  s21_decimal ten = {{10, 0, 0, 0}};
  s21_decimal q10, q_new;
  s21_zero_decimal(&q10);
  s21_zero_decimal(&q_new);

  if (s21_mul_mantissa(*quotient, ten,
                       &q10)) {  // проверка переполнения мантисы
    status = 1;
  } else {
    s21_decimal d = {{digit, 0, 0, 0}};
    if (s21_add_mantissa(q10, d, &q_new)) {  // приписываем цифру к частному
      status = 1;
    } else {
      *quotient = q_new;
      // установка нового остатка после всех вычитаний
      remainder->bits[0] = (int)r128[0];
      remainder->bits[1] = (int)r128[1];
      remainder->bits[2] = (int)r128[2];
    }
  }

  return status;
}

// приведение масштаба к допустимому диапазону
static int s21_div_fix_scale(s21_decimal *quotient, int *result_scale) {
  int exit_code = 0;

  // если масштаб делимого меньше делителя (отрицательный масштаб)
  while (*result_scale < 0 && exit_code == 0) {
    if (s21_mul_by_10(quotient)) {
      exit_code = 1;
    } else {
      (*result_scale)++;
    }
  }
  // если слишком много цифр после запятой
  if (exit_code == 0 && *result_scale > S21_MAX_SCALE) {
    int drop = *result_scale - S21_MAX_SCALE;
    s21_bank_round(quotient, drop);
    *result_scale = S21_MAX_SCALE;
  }

  return exit_code;
}

int s21_div_abs(s21_decimal a, s21_decimal b, s21_decimal *res) {
  int exit_code = 0;

  if (res == NULL) {
    exit_code = 1;
  } else {
    s21_zero_decimal(res);

    int scale_a = s21_get_scale(a);
    int scale_b = s21_get_scale(b);
    // remainder - остаток от деления, quotient - частное
    s21_decimal quotient, remainder;
    s21_zero_decimal(&quotient);
    s21_zero_decimal(&remainder);
    s21_div_mantissa(a, b, &quotient, &remainder);

    int extra_digits = 0;
    int done = 0;
    while (!s21_is_zero(remainder) && extra_digits < S21_MAX_SCALE && !done) {
      int step_status = s21_div_step(&quotient, &remainder, b);
      extra_digits++;
      if (step_status != 0) done = 1;
    }

    int result_scale = extra_digits + (scale_a - scale_b);
    exit_code = s21_div_fix_scale(&quotient, &result_scale);

    if (exit_code == 0) {
      if (s21_is_zero(quotient) && !s21_is_zero(a)) {
        exit_code = 2;
      } else {
        s21_set_scale(&quotient, result_scale);
        s21_set_sign(&quotient, 0);
        *res = quotient;
      }
    }
  }

  return exit_code;
}

// обработка переполнения (положительное или отрицательное)
int s21_handle_overflow(s21_decimal *res, int sign) {
  if (res != NULL) s21_zero_decimal(res);
  return sign ? 2 : 1;
}

// обработка ситуации, когда результат операции настолько мал
// по модулю, что его невозможно представить типом decimal
int s21_handle_underflow(s21_decimal *res) {
  if (res != NULL) s21_zero_decimal(res);
  return 2;
}
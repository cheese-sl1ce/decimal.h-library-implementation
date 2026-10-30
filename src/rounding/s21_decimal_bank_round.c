#include "../s21_decimal_internal.h"

// нужно ли прибавлять 1 после усечения числа
static int s21_bank_round_need_up(s21_decimal value, int digits_to_drop) {
  s21_decimal tmp = value;
  int last_rem = 0;  // последняя отброшеная цифра
  int any_nonzero_before = 0;  // были ли после 1 отбрасываемой не нулевые цифры
  int need_up = 0;

  for (int i = 0; i < digits_to_drop; i++) {
    if (last_rem != 0) any_nonzero_before = 1;  // ищем ненулевые цифры
    s21_div_by_10(&tmp, &last_rem);
  }

  int first_dropped = last_rem;  // старшая отбрасываемая цифра
  int rest_nonzero = any_nonzero_before;

  if (first_dropped > 5) {  // округляем вверх
    need_up = 1;
  } else if (first_dropped == 5) {
    if (rest_nonzero) {  // округляем вверх
      need_up = 1;
    } else {  // округляем, чтобы последняя оставшаяся цифра была чётной
      int last_digit = 0;
      s21_decimal copy = tmp;
      s21_div_by_10(&copy, &last_digit);
      if (last_digit % 2 != 0) need_up = 1;
    }
  }
  return need_up;
}

// усечение числа
static void s21_bank_round_truncate(s21_decimal *value, int digits_to_drop,
                                    int new_scale) {
  int rem = 0;
  for (int i = 0; i < digits_to_drop; i++) {
    s21_div_by_10(value, &rem);
  }
  s21_set_scale(value, new_scale);
}

int s21_bank_round(s21_decimal *value, int digits_to_drop) {
  int exit_code = 0;

  if (value == NULL) {
    exit_code = 1;
  } else if (digits_to_drop > 0) {
    int scale = s21_get_scale(*value);

    // нельзя отбросить больше разрядов, чем есть дробных знаков
    if (digits_to_drop > scale) digits_to_drop = scale;

    int need_up = s21_bank_round_need_up(*value, digits_to_drop);
    int new_scale = scale - digits_to_drop;

    s21_decimal tmp = *value;
    s21_bank_round_truncate(&tmp, digits_to_drop, new_scale);

    if (need_up) {
      s21_decimal one = {{1, 0, 0, 0}};
      s21_decimal sum;
      s21_zero_decimal(&sum);
      if (s21_add_mantissa(tmp, one, &sum)) {
        exit_code = 1;
      } else {
        s21_set_scale(&sum, new_scale);
        *value = sum;
      }
    } else {
      *value = tmp;
    }
  }

  return exit_code;
}
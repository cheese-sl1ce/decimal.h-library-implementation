#ifndef S21_DECIMAL_INTERNAL_H
#define S21_DECIMAL_INTERNAL_H

#include <stddef.h>

#include "s21_decimal.h"

#define S21_MAX_SCALE 28
#define S21_SIGN_MASK 0x80000000u
#define S21_SCALE_MASK 0x00FF0000u

// Верхняя граница для float
#define S21_MAX_FLOAT 7.9228163e28f
// Нижняя граница для float
#define S21_MIN_FLOAT 1e-28f

int s21_get_sign(s21_decimal value);
void s21_set_sign(s21_decimal *value, int sign);
int s21_get_scale(s21_decimal value);
void s21_set_scale(s21_decimal *value, int scale);
void s21_zero_decimal(s21_decimal *value);
void s21_zero_mantissa(s21_decimal *value);
void s21_copy_decimal(s21_decimal src, s21_decimal *dst);
int s21_is_zero(s21_decimal value);

int s21_get_bit(s21_decimal value, int bit_index);
void s21_set_bit(s21_decimal *value, int bit_index, int bit);
void s21_shift_left(s21_decimal *value, int shift);
void s21_shift_right(s21_decimal *value, int shift);
int s21_cmp_mantissa(s21_decimal a, s21_decimal b);
int s21_add_mantissa(s21_decimal a, s21_decimal b, s21_decimal *res);
int s21_sub_mantissa(s21_decimal a, s21_decimal b, s21_decimal *res);
int s21_mul_mantissa(s21_decimal a, s21_decimal b, s21_decimal *res);
int s21_div_mantissa(s21_decimal a, s21_decimal b, s21_decimal *res,
                     s21_decimal *rem);
int s21_mul_by_10(s21_decimal *value);
int s21_div_by_10(s21_decimal *value, int *remainder);

int s21_align_scales(s21_decimal *a, s21_decimal *b);
int s21_increase_scale(s21_decimal *value, int delta);
int s21_decrease_scale(s21_decimal *value, int delta);
int s21_normalize_scale(s21_decimal *value);
int s21_fit_scale(s21_decimal *value);

int s21_compare_decimal(s21_decimal a, s21_decimal b);
int s21_compare_abs(s21_decimal a, s21_decimal b);

int s21_truncate_abs(s21_decimal value, s21_decimal *res);
int s21_floor_abs(s21_decimal value, s21_decimal *res);
int s21_round_abs(s21_decimal value, s21_decimal *res);

int s21_extract_significant_digits(float src, int *digits, int *count);
int s21_round_float_7_digits(int *digits, int *count);
int s21_check_float_range(float src);
int s21_int_to_mantissa(int src, s21_decimal *dst);
int s21_mantissa_to_int(s21_decimal value, int *dst);

int s21_add_abs(s21_decimal a, s21_decimal b, s21_decimal *res);
int s21_sub_abs(s21_decimal a, s21_decimal b, s21_decimal *res);
int s21_mul_abs(s21_decimal a, s21_decimal b, s21_decimal *res);
int s21_div_abs(s21_decimal a, s21_decimal b, s21_decimal *res);
int s21_bank_round(s21_decimal *value, int digits_to_drop);
int s21_handle_overflow(s21_decimal *res, int sign);
int s21_handle_underflow(s21_decimal *res);

#endif
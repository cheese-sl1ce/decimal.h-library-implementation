#include <stddef.h>
#include <stdint.h>

#include "../s21_decimal_internal.h"

static const s21_decimal S21_DECIMAL_TEN = {{10, 0, 0, 0}};

int s21_get_bit(s21_decimal value, int bit_index) {
  int bit;
  int word = bit_index / 32;
  int offset = bit_index % 32;
  if (bit_index < 0 || bit_index > 95) {  // проверяем что не выходит за пределы
    bit = 0;  // ошибка
  } else {
    bit = ((uint32_t)value.bits[word] >> offset) &
          1;  // сдвигаемя на остаток и берем младший бит
  }
  return bit;
}

void s21_set_bit(s21_decimal *value, int bit_index, int bit) {
  int word = bit_index / 32;
  int offset = bit_index % 32;
  if (bit_index < 0 || bit_index > 95) {
    return;
  }
  uint32_t mask =
      1u << offset;  // передвигаем единичку на бит который нужно изменить
  if (bit) {
    value->bits[word] |= mask;
  } else {
    value->bits[word] &= ~mask;
  }
}

void s21_shift_left(s21_decimal *value, int shift) {
  if (shift <= 0) {
    return;
  } else if (shift >= 96) {
    s21_zero_mantissa(value);
  } else {
    for (int s = 0; s < shift; s++) {
      uint32_t carry = 0;  // переносимый бит
      for (int word = 0; word < 3; word++) {
        uint32_t w = (uint32_t)value->bits[word];
        uint32_t next_carry =
            (w >> 31) & 1u;  // сдвигаем старший бит в младший и запоминаем,
                             // уйдёт в следующее слово
        w = (w << 1) | carry;  // сдвигаем и добавляем перенос из младшего слова
        value->bits[word] = (int)w;
        carry = next_carry;  // перенос для следующего (старшего) слова
      }
    }
  }
}

void s21_shift_right(s21_decimal *value, int shift) {
  if (shift <= 0) {
    return;
  } else if (shift >= 96) {
    s21_zero_mantissa(value);
  } else {
    for (int s = 0; s < shift; s++) {
      uint32_t carry = 0;
      for (int word = 2; word >= 0; word--) {
        uint32_t w = (uint32_t)value->bits[word];
        uint32_t next_carry = w & 1u;  // младший бит уйдёт вниз
        w = (w >> 1) | (carry << 31);  // сдвигаем и добавляем перенос сверху
        value->bits[word] = (int)w;
        carry = next_carry;
      }
    }
  }
}

// сравнение мантисс без знака 0 - равны, 1 - а больше b, -1 - b больше а
int s21_cmp_mantissa(s21_decimal a, s21_decimal b) {
  int res = 0;
  for (int i = 2; i >= 0; i--) {
    if (a.bits[i] != b.bits[i] && !res) {
      res = (uint32_t)a.bits[i] > (uint32_t)b.bits[i] ? 1 : -1;
    }
  }
  return res;
}

// складываем мантиссы, от младшего слов к старшему
int s21_add_mantissa(s21_decimal a, s21_decimal b, s21_decimal *res) {
  uint32_t r[3] = {0, 0, 0};
  uint32_t carry = 0;
  for (int i = 0; i < 3; i++) {
    uint64_t sum =
        (uint64_t)(uint32_t)a.bits[i] + (uint64_t)(uint32_t)b.bits[i] + carry;
    r[i] = (uint32_t)sum;
    carry = (uint32_t)(sum >> 32);
  }
  if (res != NULL) {
    res->bits[0] = (int)r[0];
    res->bits[1] = (int)r[1];
    res->bits[2] = (int)r[2];
    res->bits[3] = 0;
  }
  return carry != 0 ? 1 : 0;
}

int s21_sub_mantissa(s21_decimal a, s21_decimal b, s21_decimal *res) {
  s21_zero_mantissa(res);
  int64_t borrow = 0;
  for (int i = 0; i < 3; i++) {
    int64_t diff =
        (int64_t)(uint32_t)a.bits[i] - (int64_t)(uint32_t)b.bits[i] - borrow;
    if (diff < 0) {
      res->bits[i] = diff + ((int64_t)1 << 32);
      borrow = 1;
    } else {
      res->bits[i] = diff;
      borrow = 0;
    }
  }
  return borrow != 0 ? 1 : 0;
}

int s21_mul_mantissa(s21_decimal a, s21_decimal b, s21_decimal *res) {
  uint32_t r[6] = {0};
  int exit_code = 0;

  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      uint64_t prod =
          (uint64_t)(uint32_t)a.bits[i] * (uint64_t)(uint32_t)b.bits[j];
      uint64_t carry = prod;
      for (int k = i + j; k < 6 && carry != 0; k++) {
        uint64_t sum = (uint64_t)r[k] + (carry & 0xFFFFFFFFu);
        r[k] = (uint32_t)sum;
        carry = (carry >> 32) + (sum >> 32);
      }
    }
  }

  // Переполнение, если старшие 96 бит не нули
  if (r[3] != 0 || r[4] != 0 || r[5] != 0) {
    exit_code = 1;
  }

  if (res != NULL) {
    res->bits[0] = (int)r[0];
    res->bits[1] = (int)r[1];
    res->bits[2] = (int)r[2];
    res->bits[3] = 0;
  }
  return exit_code;
}

int s21_div_mantissa(s21_decimal a, s21_decimal b, s21_decimal *res,
                     s21_decimal *rem) {
  int exit_code = 0;
  if (s21_is_zero(b)) exit_code = 1;

  s21_zero_mantissa(res);
  s21_zero_mantissa(rem);

  for (int i = 95; i >= 0; i--) {
    // Запоминаем, вылетит ли старший бит остатка при сдвиге
    int overflow = s21_get_bit(*rem, 95);

    s21_shift_left(rem, 1);

    // Сносим i-й бит делимого в младший бит остатк
    if (s21_get_bit(a, i)) {
      s21_set_bit(rem, 0, 1);
    }

    if (overflow || s21_cmp_mantissa(*rem, b) >= 0) {
      s21_sub_mantissa(*rem, b, rem);
      s21_set_bit(res, i, 1);
    }
  }

  return exit_code;
}

int s21_mul_by_10(s21_decimal *value) {
  int exit_code = 0;
  if (value != NULL) {
    s21_decimal tmp = *value;
    int saved_bits3 = value->bits[3];
    exit_code = s21_mul_mantissa(tmp, S21_DECIMAL_TEN, value);
    value->bits[3] = saved_bits3;
  } else {
    exit_code = 1;
  }
  return exit_code;
}

int s21_div_by_10(s21_decimal *value, int *rem) {
  int exit_code = 0;
  if (value == NULL || rem == NULL) {
    exit_code = 1;
  } else {
    *rem = 0;
    for (int i = 2; i >= 0; i--) {
      uint32_t w = value->bits[i];
      uint64_t cur_rem = ((uint64_t)*rem << 32) + w;
      value->bits[i] = (int)(uint32_t)(cur_rem / 10);
      *rem = cur_rem % 10;
    }
  }
  return exit_code;
}
